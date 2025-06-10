#include "raftclient.h"

RaftClient::RaftClient():client_id_(randomClientId()),request_id_(0),recent_leader_id_(0){
}
//对外暴露的三个功能和初始化
void RaftClient::init(std::string config_filename){
    MprpcConfig config;
    config.LoadConfigFile(config_filename.c_str());
    std::vector<std::pair<std::string,short>> ip_port_vt;
    for(int i=0;i<INT_MAX-1;i++){
        std::string node="node"+std::to_string(i);
        std::string node_ip = config.Load(node + "ip");
        std::string node_port_str = config.Load(node + "port");
        if(node_ip.empty()){
            break;
        }
        ip_port_vt.emplace_back(node_ip,atoi(node_port_str.c_str()));
    }
    //连接每个raft节点
    for(const auto& item:ip_port_vt){

        std::string node_ip=item.first;
        short node_port = item.second;
        auto* rpc = new RaftServerRpcUtil(node_ip,node_port);
        servers_.push_back(std::shared_ptr<RaftServerRpcUtil>(rpc));
        // servers_.push_back(std::make_shared<RaftServerRpcUtil>(node_ip,node_port));
    }
}
std::string RaftClient::get(std::string key){
    request_id_++;
    auto request_id = request_id_;
    int server = recent_leader_id_;
    raftKVRpcProtoc::GetRequest request;
    request.set_key(key);
    request.set_clientid(client_id_);
    request.set_requestid(request_id);
    while (true)
    {
        raftKVRpcProtoc::GetResponse response;
        bool ok=servers_[server]->get(&request,&response);
        std::cout<<"ok?"<<response.err()<<"value:"<<response.value()<<std::endl;
        if(!ok || response.err()== ErrWrongLeader){
            server=(server+1)%servers_.size();
            continue;
        }
        if(response.err() == ErrNoKey){
            return "";
        }
        if(response.err() == OK){
            recent_leader_id_=server;
            return response.value();
        }
    }
    return "";
    
}
void RaftClient::put(std::string key, std::string value){
    putAppend(key,value,"Put");
}
void RaftClient::append(std::string key, std::string value){
    putAppend(key,value,"Append");
}
//返回随机的clientid
std::string RaftClient::randomClientId(){
    return std::to_string(rand()) + std::to_string(rand())+std::to_string(rand())+std::to_string(rand());
}
void RaftClient::putAppend(std::string key,std::string value,std::string op){
    request_id_++;
    auto request_id=request_id_;
    auto server=recent_leader_id_;
    while (true)
    {
        raftKVRpcProtoc::PutAppendRequest request;
        request.set_key(key);
        request.set_value(value);
        request.set_clientid(client_id_);
        request.set_op(op);
        request.set_requestid(request_id_);
        raftKVRpcProtoc::PutAppendResponse response;
        bool ok=servers_[server]->putAppend(&request,&response);
        if(!ok || response.err() == ErrWrongLeader){
            server=(server+1)%servers_.size();
            continue;
        }
        if(response.err() == OK){
            recent_leader_id_=server;
            return ;
        }
    }
    
    

}

