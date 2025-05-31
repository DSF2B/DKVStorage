#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>


MprpcConfig MprpcApplication::m_config;
void ShowArgsHelp(){
    std::cout<<"format: command -i <configfile>"<<std::endl;
}

void MprpcApplication::Init(int argc,char **argv){
    if(argc<2){
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
    int c=0;
    std::string config_file;
    //getopt:getopt 是一个标准的库函数，用来处理命令行参数。
    while((c=getopt(argc,argv,"i:"))!=-1){
        switch(c){
            //"i:" 表示程序期望命令行参数中有 -i 选项，且该选项后面必须跟一个参数
            case 'i':
                config_file = optarg;
                break;
            case '?':
                std::cout<<"invaild args!"<<std::endl;
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            case ':':
                std::cout<<"need <configfile>"<<std::endl;
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
    //开始加载配置文件 rpcserver_ip rpcserver_port zookeeper_ip zookeeper_port
    m_config.LoadConfigFile(config_file.c_str());
    // std::cout<<"rpcserverip:"<<m_config.Load("rpcserverip")<<std::endl;
    // std::cout<<"rpcserverport:"<<m_config.Load("rpcserverport")<<std::endl;
    // std::cout<<"zookeeperip:"<<m_config.Load("zookeeperip")<<std::endl;
    // std::cout<<"zookeeperport:"<<m_config.Load("zookeeperport")<<std::endl;

}

MprpcApplication& MprpcApplication::GetInstance(){
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig(){
    return m_config;
}