#pragma once
#include "kvserverrpc.pb.h"
#include "mprpcchannel.h"
#include "rpcprovider.h"


class RaftServerRpcUtil{
public:
    RaftServerRpcUtil(std::string ip,short port);
    ~RaftServerRpcUtil();
    bool get(raftKVRpcProtoc::GetRequest* request, raftKVRpcProtoc::GetResponse* response);
    bool putAppend(raftKVRpcProtoc::PutAppendRequest* request,raftKVRpcProtoc::PutAppendResponse* response);
private:
    raftKVRpcProtoc::KVServerRpc_Stub *stub_;
};