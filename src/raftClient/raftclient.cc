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
        servers_.push_back(std::make_shared<RaftServerRpcUtil>(rpc));
    }

}
std::string RaftClient::get(std::string key){

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

}

