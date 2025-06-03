#include <iostream>
#include "raft.h"
#include "kvserver.h"
#include "unistd.h"
#include <iostream>
#include <random>

int main(int argc,char**argv){
    if(argc<2){
        exit(EXIT_FAILURE);
    }
    int c=0;
    int node_num=0;
    std::string config_filename;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 29999);
    unsigned short start_port = dis(gen);
    while((c =getopt(argc,argv,"n:f:"))!=-1){
        switch(c){
            case 'n':
                node_num=atoi(optarg);
                break;
            case 'f':
                config_filename = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    std::ofstream file(config_filename,std::ios::out | std::ios::app);
    file.close();
    file = std::ofstream(config_filename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file.close();
        std::cout << config_filename << " 已清空" << std::endl;
    }
    else{
        std::cout << "无法打开 " << config_filename << std::endl;
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<node_num;i++){
        short port=start_port+static_cast<short> (i);
        pid_t pid = fork();//创建一个新的子进程也执行后面的程序
        if(pid==0){
            // 如果是子进程
            auto kv_server=new KvServer(i,500,config_filename,port);
            pause();// 子进程进入等待状态，不会执行 return 语句
        }else if(pid > 0){
            sleep(1);
        }else{
            exit(EXIT_FAILURE);
        }
    }
    pause();
    return 0;
}