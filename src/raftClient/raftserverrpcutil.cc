#include "raftserverrpcutil.h"

RaftServerRpcUtil::RaftServerRpcUtil(std::string ip,short port)
{
    stub_=new raftKVRpcProtoc::KVServerRpc_Stub(new MprpcChannel(ip,port));
}
RaftServerRpcUtil::~RaftServerRpcUtil()
{
    delete stub_;
}
bool RaftServerRpcUtil::get(raftKVRpcProtoc::GetRequest* request, raftKVRpcProtoc::GetResponse* response)
{
    MprpcController controller;
    stub_->Get(&controller,request,response,nullptr);
    return !controller.Failed();
}
bool RaftServerRpcUtil::putAppend(raftKVRpcProtoc::PutAppendRequest* request,raftKVRpcProtoc::PutAppendResponse* response)
{
    MprpcController controller;
    stub_->PutAppend(&controller,request,response,nullptr);
    return !controller.Failed();
}
