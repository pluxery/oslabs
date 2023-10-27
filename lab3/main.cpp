#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#define getpid() GetCurrentProcessId()
#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#endif

class Logger
{
public:
    Logger(const std::string &fileName)
    {
        logFile.open(fileName, std::ios_base::app);
        if (!logFile.is_open())
        {
            std::cout << "Failed to open log file." << std::endl;
            exit(1);
        }
    }

    void writeToLog(const std::string &message)
    {
        logFile << message << std::endl;
    }

    ~Logger()
    {
        if (logFile.is_open())
        {
            logFile.close();
        }
    }

private:
    std::ofstream logFile;
};

class CopyProcess
{
public:
    CopyProcess(int &counter, Logger &logger)
        : counter(counter), logger(logger) {}

    void run()
    {
        onStart();

        performCopy();

        onExit();
    }

protected:
    virtual void onStart()
    {
        std::ostringstream os;
        os << "Process: Process ID=" << getpid() << ", Start Time=" << time(NULL) << std::endl;
        logger.writeToLog(os.str());
    }

    virtual void performCopy() = 0;

    virtual void onExit()
    {
        std::ostringstream os;
        os << "Process: Exit Time=" << time(NULL) << std::endl;
        logger.writeToLog(os.str());
    }

protected:
    int &counter;
    Logger &logger;
};

class Copy1Process : public CopyProcess
{
public:
    Copy1Process(int &counter, Logger &logger)
        : CopyProcess(counter, logger) {}

protected:
    void performCopy() override
    {
        std::ostringstream os;
        os << "Copy 1: Counter increased by 10" << std::endl;
        counter += 10;
        logger.writeToLog(os.str());
    }
};

class Copy2Process : public CopyProcess
{
public:
    Copy2Process(int &counter, Logger &logger)
        : CopyProcess(counter, logger) {}

protected:
    void performCopy() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        std::ostringstream os;
        os << "Copy 2: Counter doubled" << std::endl;
        counter *= 2;
        logger.writeToLog(os.str());

        std::this_thread::sleep_for(std::chrono::seconds(2));

        os.str("");
        counter /= 2;
        os << "Copy 2: Counter halved" << std::endl;
        logger.writeToLog(os.str());
    }
};

class CommandProcess
{
public:
    CommandProcess(int &counter, Logger &logger)
        : counter(counter), logger(logger) {}

    void run()
    {
        std::string input;
        std::cout << "Enter command: exit, <counter value>" << std::endl;
        std::getline(std::cin, input);

        if (!input.empty())
        {
            if (input == "exit")
            {
                exit(0);
            }
            counter = std::stoi(input);
        }
    }

private:
    int &counter;
    Logger &logger;
};

class MainProcess
{
public:
    MainProcess(Logger &logger)
        : counter(0), logger(logger) {}

    void run()
    {
        std::thread copy1Thread, copy2Thread, commandThread;

        std::ostringstream os;
        os << "Main process: Process ID=" << getpid() << ", Start Time=" << time(NULL) << std::endl;
        logger.writeToLog(os.str());

        while (true)
        {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            time_t currentTime = time(NULL);
            struct tm *timeinfo;
            char dateTimeBuffer[80];

            timeinfo = localtime(&currentTime);
            strftime(dateTimeBuffer, sizeof(dateTimeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);

            os.str("");
            os << dateTimeBuffer << ".";

#if defined(_WIN32)
            SYSTEMTIME sysTime;
            GetSystemTime(&sysTime);
            os << sysTime.wMilliseconds;
#else
            struct timeval timeVal;
            gettimeofday(&timeVal, NULL);
            os << timeVal.tv_usec / 1000;
#endif

            os << ": Process ID=" << getpid() << ", Counter=" << counter << std::endl;
            logger.writeToLog(os.str());

            if (!copy1Thread.joinable())
            {
                copy1Thread = std::thread([&]()
                                          {
                    Copy1Process copy1(counter, logger);
                    copy1.run();
                    copy1Thread.detach(); });
            }
            else
            {
                logger.writeToLog("copy1 running");
            }

            if (!copy2Thread.joinable())
            {
                copy2Thread = std::thread([&]()
                                          {
                    Copy2Process copy2(counter, logger);
                    copy2.run();
                    copy2Thread.detach(); });
            }
            else
            {
                logger.writeToLog("copy2 running");
            }

            if (!commandThread.joinable())
            {
                commandThread = std::thread([&]()
                                            {
                    CommandProcess command(counter, logger);
                    command.run();
                    commandThread.detach(); });
            }
        }
    }

private:
    int counter;
    Logger &logger;
};

int main()
{
    Logger logger("log.txt");
    MainProcess mainProcess(logger);
    mainProcess.run();

    return 0;
}