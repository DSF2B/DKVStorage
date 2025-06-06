#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
//框架提供的用于发布rpc服务的网络对象类,向其他rpc节点请求服务

class RpcProvider
{
public:
    ~RpcProvider();
    //应当可以接受任意的Service(包括派生类UserServiceRpc等)，通知需要调用的服务
    void NotifyService(google::protobuf::Service *service);
    //启动rpc服务节点，开始提供rpc远程调用服务
    void Run(int nodeIndex, short port,std::string node_info_filename);
private:
    muduo::net::EventLoop event_loop_;
    std::shared_ptr<muduo::net::TcpServer> muduo_server_;
    //服务类型信息
    struct ServiceInfo{
        google::protobuf::Service *m_service;//服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;//服务方法
    };
    //存储注册的服务对象及其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;
    //新的socket连接回调函数
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    //已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);
    //closure回调操作，用于执行完本地方法后序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};