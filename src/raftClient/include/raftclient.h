#pragma once



#include <vector>
#include <string>


#include "kvserverrpc.pb.h"
#include "raftserverrpcutil.h"
#include "mprpcconfig.h"
#include "util.h"
class RaftClient
{
public:
    RaftClient();
      //对外暴露的三个功能和初始化
    void init(std::string config_filename);
    std::string get(std::string key);
    void put(std::string key, std::string value);
    void append(std::string key, std::string value);

private:
    std::vector<std::shared_ptr<RaftServerRpcUtil>> servers_;
    std::string client_id_;
    int request_id_;
    int recent_leader_id_;
    std::string randomClientId();//返回随机的clientid
    void putAppend(std::string key,std::string value,std::string op);
};


