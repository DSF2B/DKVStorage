#include "persister.h"

void Persister::save(std::string raft_state, std::string snapshot){
    std::lock_guard<std::mutex> lock(mtx_);
    clearRaftStateAndSnapshot();
    //将stase和snapshot写入文件
    raft_state_out_stream_<<raft_state;
    snapshot_out_stream_<<snapshot;
}


std::string Persister::readRaftState(){
    std::lock_guard<std::mutex> lock(mtx_);
    
    std::fstream ifs(raft_state_filename_, std::ios_base::in);
    if (!ifs.good()) {
        return "";
    }
    std::string snapshot;
    ifs >> snapshot;
    ifs.close();
    return snapshot;
}

void Persister::saveRaftState(const std::string &data) {
    std::lock_guard<std::mutex> lock(mtx_);
    // 将raftstate和snapshot写入本地文件
    //清除stream
    clearRaftState();
    raft_state_out_stream_ << data;
    raft_state_size_ += data.size();
}

std::string Persister::readSnapshot(){
    return snapshot_;
}

long long Persister::raftStateSize(){
    return raft_state_size_;
}
Persister::Persister(int me):
    raft_state_filename_("raftstatePersist"+std::to_string(me)+".txt"),
    snapshot_filename_("snapshotPersist"+std::to_string(me)+".txt"),
    raft_state_size_(0)
    {
    //检查文件状态并清空文件
    bool file_open_flag=true;
    // 重新打开文件流并清空文件内容
    std::fstream file(raft_state_filename_,std::ios::out | std::ios::trunc);
    if(file.is_open()){
        file.close();
    }else{
        file_open_flag=false;
    }
    // 重新打开文件流并清空文件内容
    file = std::fstream(snapshot_filename_,std::ios::out | std::ios::trunc);
    if(file.is_open()){
        file.close();
    }else{
        file_open_flag=false;
    }
    if (!file_open_flag) {
    DPrintf("[func-Persister::Persister] file open error");
    }
    raft_state_out_stream_.open(raft_state_filename_);
    snapshot_out_stream_.open(snapshot_filename_);
}
Persister::~Persister(){
    if(raft_state_out_stream_.is_open()){
        raft_state_out_stream_.close();
    }
    if(snapshot_out_stream_.is_open()){
        snapshot_out_stream_.close();
    }
}

void Persister::clearRaftState() {
    raft_state_size_ = 0;
    // 关闭文件流
    if (raft_state_out_stream_.is_open()) {
      raft_state_out_stream_.close();
    }
    // 重新打开文件流并清空文件内容
    raft_state_out_stream_.open(raft_state_filename_, std::ios::out | std::ios::trunc);
}


void Persister::clearSnapshot(){
    raft_state_size_ = 0;
    // 关闭文件流
    if (snapshot_out_stream_.is_open()){
        snapshot_out_stream_.close();
    }
    // 重新打开文件流并清空文件内容
    snapshot_out_stream_.open(snapshot_filename_, std::ios::out | std::ios::trunc);
}
void Persister::clearRaftStateAndSnapshot(){
    clearRaftState();
    clearSnapshot();
}