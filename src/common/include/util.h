#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>
#include <mutex>
#include <condition_variable>
#include <random>
#include <functional>
#include <queue>
#include <thread>

#include "config.h" 
// template<class T>
// class DeferClass {
// public:
//     // 构造函数，用于接受一个通用的函数对象
//     DeferClass(T&& func) : func_(std::forward<T>(func)) {}

//     // 构造函数，用于接受成员函数指针
//     template<class C>
//     explicit DeferClass(C* obj, void (C::*func)()) {
//         func_ = [obj, func]() { (obj->*func)(); };
//     }

//     // 析构函数调用延迟的函数
//     ~DeferClass() { func_(); }

// private:
//     std::function<void()> func_;
// };

#define _CONCAT(a, b) a##b
#define _MAKE_DEFER_(line) DeferClass _CONCAT(defer_placeholder, line) = [&]()

#undef DEFER
#define DEFER _MAKE_DEFER_(__LINE__)

std::chrono::_V2::system_clock::time_point now();

std::chrono::milliseconds getRandomizedElectionTimeout();

void sleepNMilliseconds(int N);


template<typename T>
class LockQueue
{
public:
    void push(const T& data)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(data);
        cond_variable_.notify_one();
    }
    T pop()
    {
        //这里用unique_lock是因为lock_guard不支持手动解锁，而unique_lock支持
        std::unique_lock<std::mutex> lock(mtx_);
        while(queue_.empty()){
            cond_variable_.wait(lock);
        }
        T poped_data = queue_.front();
        queue_.pop();
        return poped_data;
    }
    bool timeoutPop(int timeout,T* resData)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        auto now = std::chrono::system_clock::now();
        auto timeout_time = now+std::chrono::milliseconds(timeout);
        //queue为g空时超时返回false,不超时不为空时pop
        while(queue_.empty()){
            if(cond_variable_.wait_until(lock,timeout_time) == std::cv_status::timeout){
                return false;
            }else{
                continue;
            }
        }
        T poped_data = queue_.front();
        queue_.pop();
        *resData = poped_data;
        return true;
    }
private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cond_variable_;
};
//op是kv传递给raft的command
class Op{
public:
    std::string operation_;//"Get" "Put" "Append"
    std::string key_;
    std::string value_;
    std::string client_id_;//客户端id
    int request_id_;//客户端id请求的request序列号
public:
    std::string asString() const {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        // write class instance to archive
        oa << *this;
        // close archive

        return ss.str();
    }

    bool parseFromString(std::string str) {
        std::stringstream iss(str);
        boost::archive::text_iarchive ia(iss);
        // read class state from archive
        ia >> *this;
        return true;  
    }
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& operation_;
        ar& key_;
        ar& value_;
        ar& client_id_;
        ar& request_id_;
    }
};

const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";
const std::string ErrWrongLeader = "ErrWrongLeader";

bool isReleasePort(unsigned short usPort);

bool getReleasePort(short& port);