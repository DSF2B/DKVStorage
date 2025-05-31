#pragma once
#include "lockqueue.h"
#include <string>

enum LogLevel{
    INFO,
    ERROR,
};
class Logger{
public:
    void SetLogLevel(LogLevel level);
    void Log(std::string msg);
    static Logger& GetInstance();
private:
    int m_loglevel;
    LockQueue<std::string> m_lockQue;
    //单例
    Logger();
    Logger(const Logger&)=delete;
    Logger(Logger&&)= delete;
};


//LOG_INFO("%d is %s",int string)
#define LOG_INFO(logmsgformat, ...)\
    do\
    {\
        Logger &logger = Logger::GetInstance();\
        logger.SetLogLevel(INFO);\
        char c[1024]={0};\
        snprintf(c,1024,logmsgformat,##__VA_ARGS__);\
        logger.Log(c);\
    }while(0);

#define LOG_ERROR(logmsgformat, ...) \
    do\
    {\
        Logger &logger = Logger::GetInstance();\
        logger.SetLogLevel(ERROR);\
        char c[1024]={0};\
        snprintf(c,1024,logmsgformat,##__VA_ARGS__);\
        logger.Log(c);\
    }while(0);