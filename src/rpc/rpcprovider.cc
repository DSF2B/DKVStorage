#include "rpcprovider.h"
#include <mprpcapplication.h>
#include "rpcheader.pb.h"
#include "logger.h"


//发布rpc方法函数接口，提供外部使用,在本地注册服务方法表
void RpcProvider::NotifyService(google::protobuf::Service *service){
    ServiceInfo service_info;
    //用户服务信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    //用户服务名称
    std::string service_name = pserviceDesc->name();
    //service服务的方法的数量
    int methodCnt=pserviceDesc->method_count();
    std::cout<<"service name:"<<service_name<<std::endl;
    LOG_INFO("service_name:%s",service_name.c_str());
    for(int i=0;i<methodCnt;i++){
        //获取服务对象的某个方法的描述
        const google::protobuf::MethodDescriptor* pmethodDesc= pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        // std::cout<<"method name:"<<method_name<<std::endl;
        LOG_INFO("method_name:%s",method_name.c_str());
        service_info.m_methodMap.insert({method_name,pmethodDesc});
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name,service_info});//服务，服务的方法表

}
//启动rpc服务节点，开始提供rpc远程调用服务
void RpcProvider::Run(int nodeIndex, short port){
    char *ipC;
    char hname[128];
    struct hostent *hent;
    gethostname(hname, sizeof(hname));
    hent = gethostbyname(hname);
    for (int i = 0; hent->h_addr_list[i]; i++) {
        ipC = inet_ntoa(*(struct in_addr *)(hent->h_addr_list[i]));  // IP地址
    }
    std::string ip = std::string(ipC);

    std::string node = "node" + std::to_string(nodeIndex);
    std::ofstream outfile;
    outfile.open("server.conf", std::ios::app);  //打开文件并追加写入
    if (!outfile.is_open()) {
        std::cout << "打开文件失败！" << std::endl;
        exit(EXIT_FAILURE);
    }
    outfile << node + "ip=" + ip << std::endl;
    outfile << node + "port=" + std::to_string(port) << std::endl;
    outfile.close();

    muduo::net::InetAddress address(ip,port);
    //创建Tcpserver
    muduo_server_ = std::make_shared<muduo::net::TcpServer> (&event_loop_,address,"RpcProvider");
    //绑定连接回调和读写回调方法,分离网络和业务代码
    muduo_server_->setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));
    muduo_server_->setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,
        std::placeholders::_2,std::placeholders::_3));
    //设置muduo库线程数量
    muduo_server_->setThreadNum(4);
    std::cout<<"rpcprovider start service at ip:"<<ip<<"port"<<port<<std::endl;
    //启动服务
    muduo_server_->start();
    std::cout<<"event start loop"<<std::endl;
    event_loop_.loop();
    
}   

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn){
    if(!conn->connected()){
        std::cout<<"connect shutdown"<<std::endl;
        conn->shutdown();
    }
    std::cout<<"connect success"<<std::endl;
}
/*
    框架内部RpcProvider和RpceConsumer约定protobuf数据格式
    service_name method_name args 定义proto的message类型，进行序列化和反序列化,考虑tcp粘包问题
    header_size(4bytes) + head_str(service_name method_name args_size) + args_str指明哪些是服务名和方法名和参数
    把header_size转为4bytes二进制，而不是大小不定的字符串
    std:;string insert和copy方法
*/
//已经建立连接的读写事件回调，如果远程有rpc服务调用请求，那么OnMessage响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, 
                            muduo::net::Buffer *buff,    
                            muduo::Timestamp){
    std::string recv_buf = buff->retrieveAllAsString();
    std::cout<<"request recved"<<std::endl;
    // //从字符流中读取前四个字节的内容作为header_size
    // uint32_t header_size  = 0;
    // //拷贝四个字节到header_size;
    // //size_t copy(char* s, size_t len, size_t pos = 0) const;
    // //其中，s 是目标字符数组，len 是要复制的字符数，pos 是开始复制的位置。该函数返回实际复制的字符数
    // recv_buf.copy((char*)&header_size,4,0);
    // // std::string header_str = recv_buf.substr(0, 4);
    // // std::memcpy(&header_size, header_str.data(), 4);
    // //获取header的原始字符流，然后反序列化得到具体header
    // std::string rpc_header_str = recv_buf.substr(4,header_size);
    // mprpc::RpcHeader rpcheader;
    // std::string service_name;
    // std::string method_name;
    // uint32_t args_size;

    // 使用protobuf的CodedInputStream来解析数据流
    google::protobuf::io::ArrayInputStream array_input(recv_buf.data(), recv_buf.size());
    google::protobuf::io::CodedInputStream coded_input(&array_input);
    uint32_t header_size{};

    coded_input.ReadVarint32(&header_size);  // 解析header_size
    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str;
    mprpc::RpcHeader rpcheader;
    std::string service_name;
    std::string method_name;
    // 设置读取限制，不必担心数据读多
    google::protobuf::io::CodedInputStream::Limit msg_limit = coded_input.PushLimit(header_size);
    coded_input.ReadString(&rpc_header_str, header_size);
    // 恢复之前的限制，以便安全地继续读取其他数据
    coded_input.PopLimit(msg_limit);
    uint32_t args_size{};

    if(rpcheader.ParseFromString(rpc_header_str)){
        service_name = rpcheader.service_name();
        method_name = rpcheader.method_name();
        args_size = rpcheader.args_size();
    }else{
        std::cout<<"rpc_header_str:"<<rpc_header_str<<" parse error!"<<std::endl;
        
        return ;
    }

    std::string args_str;
    
      // 直接读取args_size长度的字符串数据
    bool read_args_success = coded_input.ReadString(&args_str, args_size);
    if (!read_args_success) {
    // 处理错误：参数数据读取失败
        return;
    }

    auto it=m_serviceMap.find(service_name);
    if(it == m_serviceMap.end()){
        std::cout<<service_name<<" is not exist"<<std::endl;
        return ;
    }
    
    auto mit=it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end()){
        std::cout<<service_name<<":"<<method_name<<" is not exist"<<std::endl;
        return ;
    }
    google::protobuf::Service *service = it->second.m_service;      //获取服务 UserService
    const google::protobuf::MethodDescriptor* method = mit->second; //获取方法 Login

    //生成response和request参数,service->GetRequestPrototype()得到method所需的request类型，Login->LoginRequest
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str)){
        std::cout<<"request parse error,content"<<args_str<<std::endl;
        return ;
    }
    
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    // <typename Class, typename Arg1, typename Arg2>
    //创建一个closure派生类对象，不同参数对应不同派生类，派生类中重写run调用method_，即SendRpcResponse
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>
                                            (this, &RpcProvider::SendRpcResponse, conn, response);

    // if(!response_args->ParseFromString(args_str)){
    //     std::cout<<"request parse error,content"<<args_str<<std::endl;
    //     return ;
    // }
    
    /*
    void UserServiceRpc::CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
        ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        const ::PROTOBUF_NAMESPACE_ID::Message* request,
        ::PROTOBUF_NAMESPACE_ID::Message* response,
        ::google::protobuf::Closure* done)
    */
   //给下面的method方法的调用，绑定一个Closure回调函数

   //框架根据远端rpc请求，调用当前节点上的方法，比如Login
   std::cout<<"provider working"<<std::endl; 
   service->CallMethod(method,nullptr,request,response,done); 

}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response){
    std::cout<<"Response ready to send!"<<std::endl;
    std::string response_str;
    if(response->SerializeToString(&response_str)){    //response序列化
        //  序列化成功后，通过网络把rpc方法执行结果response发送给rpc调用方conn
        conn->send(response_str);
        std::cout<<"Response send!"<<std::endl;
    }else{
        std::cout<<"Serialize response_str error!"<<std::endl;
    }
    // conn->shutdown();
    std::cout<<"response send"<<std::endl;
} 
RpcProvider::~RpcProvider() {
    std::cout << "[func - RpcProvider::~RpcProvider()]: ip和port信息：" << muduo_server_->ipPort() << std::endl;
    event_loop_.quit();
    //    m_muduo_server.   怎么没有stop函数，奇奇怪怪，看csdn上面的教程也没有要停止，甚至上面那个都没有
}