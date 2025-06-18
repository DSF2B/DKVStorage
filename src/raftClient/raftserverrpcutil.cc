#include "raftserverrpcutil.h"
#include "mprpccontroller.h"
//先开启服务器，再尝试连接其他的节点，中间给一个间隔时间，等待其他的rpc服务器节点启动
RaftServerRpcUtil::RaftServerRpcUtil(std::string ip,short port)
{
    stub_=new raftKVRpcProtoc::KVServerRpc_Stub(new MprpcChannel(ip,port,false));
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
    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    }
    return !controller.Failed();
}
