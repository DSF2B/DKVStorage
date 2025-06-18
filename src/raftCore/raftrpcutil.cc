#include "raftrpcutil.h"
#include <mprpcchannel.h>
#include "mprpccontroller.h"


RaftRpcUtil::RaftRpcUtil(std::string ip, uint16_t port)
{
    stub_ = new raftRpcProtoc::raftRpc_Stub(new MprpcChannel(ip,port,true));

}
RaftRpcUtil::~RaftRpcUtil()
{
    std::cout<<"raftRpcUtil exit"<<std::endl;
    delete stub_;
}
//内部调用远程方法
bool RaftRpcUtil::AppendEntries(raftRpcProtoc::AppendEntriesRequest *request, raftRpcProtoc::AppendEntriesResponse *response)
{
    MprpcController controller;
    stub_->AppendEntries(&controller,request,response,nullptr);
    return !controller.Failed();
}
bool RaftRpcUtil::RequestVote(raftRpcProtoc::RequestVoteRequest *request, raftRpcProtoc::RequestVoteResponse *response)
{
    MprpcController controller;
    stub_->RequestVote(&controller,request,response,nullptr);
    return !controller.Failed();
}
bool RaftRpcUtil::InstallSnapshot(raftRpcProtoc::InstallSnapshotRequest *request, raftRpcProtoc::InstallSnapshotResponse *response)
{
    MprpcController controller;
    stub_->InstallSnapshot(&controller,request,response,nullptr);
    return !controller.Failed();
}
