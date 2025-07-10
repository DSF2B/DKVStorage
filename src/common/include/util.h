#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>  // pthread_condition_t
#include <functional>
#include <iostream>
#include <mutex>  // pthread_mutex_t
#include <queue>
#include <random>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>

#include "config.h"
template <class F>
class DeferClass {
 public:
  DeferClass(F&& f) : m_func(std::forward<F>(f)) {}
  DeferClass(const F& f) : m_func(f) {}
  ~DeferClass() { m_func(); }

  DeferClass(const DeferClass& e) = delete;
  DeferClass& operator=(const DeferClass& e) = delete;

 private:
  F m_func;
};


#define _CONCAT(a, b) a##b
#define _MAKE_DEFER_(line) DeferClass _CONCAT(defer_placeholder, line) = [&]()

#undef DEFER
#define DEFER _MAKE_DEFER_(__LINE__)

void DPrintf(const char* format, ...);
void myAssert(bool condition, std::string message = "Assertion failed!");

template <typename... Args>
std::string format(const char* format_str, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format_str, args...) + 1; // "\0"
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::vector<char> buf(size);
    std::snprintf(buf.data(), size, format_str, args...);
    return std::string(buf.data(), buf.data() + size - 1);  // remove '\0'
}

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
 public:
  friend std::ostream& operator<<(std::ostream& os, const Op& obj) {
    os << "[MyClass:operation_{" + obj.operation_ + "},key_{" + obj.key_ + "},value_{" + obj.value_ + "},client_id_{" +
              obj.client_id_ + "},request_id_{" + std::to_string(obj.request_id_) + "}";  // 在这里实现自定义的输出格式
    return os;
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