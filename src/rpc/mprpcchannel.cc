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

    if(client_fd_==-1){
        std::string errMsg;

        std::cout<<"connect fail"<<std::endl;
        bool rt=newConnect(ip_.c_str(),port_,&errMsg);
        if(!rt){
            controller->SetFailed(errMsg);
            DPrintf("[func-MprpcChannel::CallMethod]重连接ip：{%s} port{%d}失败", ip_.c_str(), port_);
            return;
        }else{
            DPrintf("[func-MprpcChannel::CallMethod]连接ip：{%s} port{%d}成功", ip_.c_str(), port_);

        }
    }
    // 服务调用者调用callmethod，callmethod内部完成序列化和反序列化以及网络通信
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    // 获取参数的序列化字符串长度args_size
    uint32_t args_size{};
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

    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str))
    {
        // std::cout << "serialize rpc header error" << std::endl;
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    // 组织待发送的rpc请求字符串
    //  header_size(4bytes) + head_str(service_name method_name args_size) + args_str指明哪些是服务名和方法名和参数]
    std::string send_rpc_str;
    
    
    // 使用protobuf的CodedOutputStream来构建发送的数据流
    {
        // 创建一个StringOutputStream用于写入send_rpc_str
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);

        // 先写入header的长度（变长编码）
        coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));

        // 不需要手动写入header_size，因为上面的WriteVarint32已经包含了header的长度信息
        // 然后写入rpc_header本身
        coded_output.WriteString(rpc_header_str);
    }
    send_rpc_str += args_str;

    std::cout<<"try to connect ip:"<<ip_<<"port"<<port_<<std::endl;

    //失败会重试连接再发送，重试连接失败会直接return
    while(send(client_fd_, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1)
    {
        std::cout << "send error!errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"send error!errno:%d",errno);
        controller->SetFailed(errtext);
        close(client_fd_);
        client_fd_=-1;
        std::string errMsg;
        bool rt=newConnect(ip_.c_str(),port_,&errMsg);
        if(!rt){
            controller->SetFailed(errMsg);
            return;
        }
    }
    std::cout<<"request sended"<<std::endl;

    // 接受rpcg响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if ((recv_size = recv(client_fd_, recv_buf, 1024, 0)) == -1)
    {
        close(client_fd_);
        client_fd_=-1;
        std::cout << "recv error!errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"recv error!errno:%d",errno);
        controller->SetFailed(errtext);
        return;
    }
    std::cout<<"response recved"<<std::endl;
    // 将响应写入response
    // std::string response_str(recv_buf, 0, recv_size);//recv_buf遇到\0，后面的数据就无法读取了
    
    if (!response->ParseFromArray(recv_buf,recv_size))
    { // response反序列化
        //失败
        // std::cout << "parse error! response_str:" << recv_buf << std::endl;
        char errtext[2048] = {0};
        sprintf(errtext,"parse error! response_str:%s",recv_buf);
        controller->SetFailed(errtext);
        return;
    }
}

bool MprpcChannel::newConnect(const char* ip,uint16_t port, std::string* errMsg)
{
    // 网络通信发送   int socket(int domain, int type, int protocol);
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        // std::cout << "create socket error,errno:" << errno << std::endl;
        char errtext[512] = {0};
        sprintf(errtext,"create socket error,errno:%d",errno);
        client_fd_=-1;
        *errMsg = errtext;
        return false;
    }
    //初始化socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << "connect error!errno:" << errno << std::endl;
        close(client_fd_);
        char errtext[512] = {0};
        sprintf(errtext,"connect error!errno:%d",errno);
        client_fd_=-1;
        *errMsg = errtext;
        return false;
    }
    client_fd_=clientfd;
    return true;
}
MprpcChannel::MprpcChannel(std::string ip,uint16_t port,bool connectNow):ip_(ip),port_(port){
    if (!connectNow) {
        return;
    }  //可以允许延迟连接
    std::string errMsg;
    auto rt = newConnect(ip.c_str(), port, &errMsg);
    int tryCount = 3;
    while (!rt && tryCount--) {
        std::cout << errMsg << std::endl;
        rt = newConnect(ip.c_str(), port, &errMsg);
    }
}
