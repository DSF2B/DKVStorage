#pragma once
#include <string>
#include "util.h"
#include <fstream>
#include <mutex>


class Persister{
public:
    std::string readRaftState();
    void saveRaftState(const std::string &data);
private:
    void clearRaftState();

private:
    std::mutex mtx_;
    std::string raft_state_;
    std::string snapshot_;
    const std::string raftStateFileName_;
    const std::string snapshotFileName_;
    std::ofstream raftStateOutStream_;
    std::ofstream snapshotOutStream_;
    long long raftStateSize_;

};