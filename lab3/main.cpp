#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else

#include <unistd.h>
#include <iomanip>

#endif

class Logger {
private:
    std::ofstream logFile;

public:
    Logger(const std::string &filePath) {
        logFile.open(filePath, std::ios_base::app);
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void writeLog(const std::string &message) {
        if (logFile.is_open()) {
            logFile << message << std::endl;
        }
    }
};

class TimeUtility {
public:
    static std::string getCurrentTime() {
        std::stringstream ss;
        auto now = std::chrono::system_clock::now();
        auto timeNow = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

#ifdef _WIN32
        struct tm timeInfo;
        localtime_s(&timeInfo, &timeNow);
#else
        struct tm timeInfo;
        localtime_r(&timeNow, &timeInfo);
#endif

        ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
        ss << '.';
        ss << std::setfill('0') << std::setw(3) << ms.count();

        return ss.str();
    }
};

class Timer {
private:
    int &counter;
    bool &allowCounterModification;
    std::chrono::milliseconds interval;
    std::thread timerThread;

public:
    Timer(int &counter, bool &allowCounterModification)
            : counter(counter), allowCounterModification(allowCounterModification), interval(300) {}

    ~Timer() {
        if (timerThread.joinable()) {
            timerThread.join();
        }
    }

    void start() {
        timerThread = std::thread([this]() {
            while (allowCounterModification) {
                std::this_thread::sleep_for(interval);
                if (allowCounterModification) {
                    ++counter;
                }
            }
        });
    }
};

class Copy {
private:
    std::string processId;
    int &counter;
    bool &allowCounterModification;
    Logger &logger;

public:
    Copy(const std::string &processId, int &counter, bool &allowCounterModification, Logger &logger)
            : processId(processId), counter(counter), allowCounterModification(allowCounterModification),
              logger(logger) {}

    void run() {
        std::string copyId;
        std::stringstream copyMessage;

        // Generate unique copyId
        {
            std::stringstream ss;
            ss << processId << "_" << std::this_thread::get_id();
            copyId = ss.str();
        }

        // Calculate modified counter value
        int copyCounter = counter;
        if (copyId.length() % 2 == 0) {
            copyCounter += 10;
        } else {
            copyCounter *= 2;
        }

        // Prepare copy message
        copyMessage << "Copy " << copyId << " started at " << TimeUtility::getCurrentTime() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Simulate some work
        copyMessage << "Modified counter value: " << copyCounter << std::endl;
        copyMessage << "Copy " << copyId << " exited at " << TimeUtility::getCurrentTime();

        // Write copy message to log
        logger.writeLog(copyMessage.str());
    }
};

class Program {
private:
    int counter;
    bool allowCounterModification;
    bool canSpawnCopies;
    Logger logger;
    Timer timer;

public:
    Program(const std::string &logFilePath)
            : counter(0), allowCounterModification(false), canSpawnCopies(true), logger(logFilePath),
              timer(counter, allowCounterModification) {}

    void run() {
        // Get process ID
#ifdef _WIN32
        DWORD processId = GetCurrentProcessId();
#else
        pid_t processId = getpid();
#endif

        // Open log file and write startup message
        logger.writeLog("Process ID: " + std::to_string(processId) + ", started at " + TimeUtility::getCurrentTime());

        timer.start();

        while (true) {
            // Check if any copy is still running
            if (!allowCounterModification) {
                std::cout << "Cannot spawn copies. A copy is still running." << std::endl;
            }

            // Process user command
            std::string command;
            std::cout << "Enter command (set_counter, spawn_copies, quit): ";
            std::cin >> command;

            if (command == "set_counter") {
                std::cout << "Enter new counter value: ";
                std::cin >> counter;
                allowCounterModification = true;
            } else if (command == "spawn_copies") {
                if (allowCounterModification) {
                    allowCounterModification = false;

                    // Spawn copies
                    Copy copy1(std::to_string(processId), counter, allowCounterModification, logger);
                    Copy copy2(std::to_string(processId), counter, allowCounterModification, logger);

                    std::thread copy1Thread(&Copy::run, &copy1);
                    std::thread copy2Thread(&Copy::run, &copy2);

                    copy1Thread.join();
                    copy2Thread.join();
                } else {
                    std::cout << "Cannot spawn copies. Counter modification is not allowed." << std::endl;
                }
            } else if (command == "quit") {
                // Exit the program
                allowCounterModification = false;
                canSpawnCopies = false;
                break;
            } else {
                std::cout << "Invalid command." << std::endl;
            }

            // Write current time, process ID, and counter value to log
            logger.writeLog(TimeUtility::getCurrentTime() + ", Process ID: " + std::to_string(processId)
                            + ", Counter: " + std::to_string(counter));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Write exit message to log file
        logger.writeLog("Process ID: " + std::to_string(processId) + " exited at " + TimeUtility::getCurrentTime());
    }
};

int main() {
    Program program("log.txt");
    program.run();

    return 0;
}