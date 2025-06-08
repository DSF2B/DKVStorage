#pragma once
#include "mprpcapplication.h"
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rpcheader.pb.h"
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <errno.h>
#include <unistd.h>

class MprpcChannel:public google::protobuf::RpcChannel
{
public:
    MprpcChannel(std::string ip,uint16_t port, bool connectNow);
    void CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
        google::protobuf::Message* response, google::protobuf::Closure* done);
    
private:
    std::string ip_;
    uint16_t port_;
    int client_fd_;
    bool newConnect(const char* ip,uint16_t port, std::string* errMsg);
};