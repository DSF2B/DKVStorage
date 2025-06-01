#pragma once
#include "raftrpc.pb.h"

class RaftRpcUtil{
public:
    RaftRpcUtil(std::string ip, uint16_t port);
    ~RaftRpcUtil();
    //内部调用远程方法
    bool AppendEntries(raftRpcProtoc::AppendEntriesRequest *request, raftRpcProtoc::AppendEntriesResponse *response);
    bool RequestVote(raftRpcProtoc::RequestVoteRequest *request, raftRpcProtoc::RequestVoteResponse *response);
    bool InstallSnapshot(raftRpcProtoc::InstallSnapshotRequest *request, raftRpcProtoc::InstallSnapshotResponse *response);
private:
    raftRpcProtoc::raftRpc_Stub *stub_;//通过stub远程调用其他节点上的方法
};