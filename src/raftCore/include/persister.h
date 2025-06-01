#pragma once
#include <string>
#include "util.h"
#include <fstream>
#include <mutex>


class Persister{
public:
    void save(std::string raftstate, std::string snapshot);
    std::string readRaftState();
    void saveRaftState(const std::string &data);
    std::string readSnapshot();
    long long raftStateSize();
    explicit Persister(int me);
    ~Persister();
private:
    void clearRaftState();
    void clearSnapshot();
    void clearRaftStateAndSnapshot();

private:
    std::mutex mtx_;
    //状态机数据
    std::string raft_state_;
    //快照
    std::string snapshot_;

    const std::string raft_state_filename_;
    const std::string snapshot_filename_;
    //保存raftState的输出流
    std::ofstream raft_state_out_stream_;
    //保存snapshot的输出流
    std::ofstream snapshot_out_stream_;
    //保存raftStateSize的大小 避免每次都读取文件来获取具体的大小
    long long raft_state_size_;
};