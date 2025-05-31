#include "persister.h"

std::string Persister::readRaftState() {
    std::lock_guard<std::mutex> lg(mtx_);

    std::fstream ifs(raftStateFileName_, std::ios_base::in);
    if (!ifs.good()) {
    return "";
    }
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

void Persister::saveRaftState(const std::string &data) {
  std::lock_guard<std::mutex> lg(mtx_);
  // 将raftstate和snapshot写入本地文件
  clearRaftState();
  raftStateOutStream_ << data;
  raftStateSize_ += data.size();
}

void Persister::clearRaftState() {
  raftStateSize_ = 0;
  // 关闭文件流
  if (raftStateOutStream_.is_open()) {
    raftStateOutStream_.close();
  }
  // 重新打开文件流并清空文件内容
  raftStateOutStream_.open(raftStateFileName_, std::ios::out | std::ios::trunc);
}