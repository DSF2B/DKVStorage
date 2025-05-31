#include "mprpcchannel.h"
#include "mprpccontroller.h"


// header_size(4bytes) + head_str(service_name method_name args_size) + args_str指明哪些是服务名和方法名和参数
void MprpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor *method,
    google::protobuf::RpcController *controller,//response无法返回时，用于返回错误信息
    const google::protobuf::Message *request,
    google::protobuf::Message *response,
    google::protobuf::Closure *done)
{
    // 服务调用者调用callmethod，callmethod内部完成序列化和反序列化以及网络通信
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    // 获取参数的序列化字符串长度args_size
    int args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        // std::cout << "serialize request error" << std::endl;
        controller->SetFailed("serialize request error!");
        return;
    }
    // 定义rpc请求的header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        // std::cout << "serialize rpc header error" << std::endl;
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    // 组织待发送的rpc请求字符串
    //  header_size(4bytes) + head_str(service_name method_name args_size) + args_str指明哪些是服务名和方法名和参数]
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // 网络通信发送   int socket(int domain, int type, int protocol);
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        // std::cout << "create socket error,errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"create socket error,errno:%d",errno);
        controller->SetFailed(errtext);
        return ;
    }
    //读取配置文件中的rpcserver信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    //rpc调用方向zk上查询该服务所在的host信息

    std::string method_path = "/"+service_name+"/"+method_name;
    std::string ip=ip_;
    uint16_t port = port_;

    std::cout<<"find method on ip:"<<ip<<"port"<<port<<std::endl;
    
    //初始化socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        // std::cout << "connect error!errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"connect error!errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return ;
    }
    // 发送rpc请求
    if (send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1)
    {
        // std::cout << "send error!errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"send error!errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }
    // 接受rpcg响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if ((recv_size = recv(clientfd, recv_buf, 1024, 0)) == -1)
    {
        // std::cout << "recv error!errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"recv error!errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }
    // 将响应写入response
    // std::string response_str(recv_buf, 0, recv_size);//recv_buf遇到\0，后面的数据就无法读取了
    
    if (!response->ParseFromArray(recv_buf,recv_size))
    { // response反序列化
        //失败
        // std::cout << "parse error! response_str:" << recv_buf << std::endl;
        char errtext[2048] = {0};
        sprintf(errtext,"parse error! response_str:%s",recv_buf);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }
    close(clientfd);
}
MprpcChannel::MprpcChannel(std::string ip,uint16_t port):ip_(ip),port_(port){
}
