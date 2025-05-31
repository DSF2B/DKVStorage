#include "raftrpcutil.h"
#include <mprpcchannel.h>
#include "mprpcapplication.h"


RaftRpcUtil::RaftRpcUtil(std::string ip, uint16_t port)
{
    stub_ = new raftRpcProtoc::raftRpc_Stub(new MprpcChannel(ip,port));

}
RaftRpcUtil::~RaftRpcUtil()
{
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
