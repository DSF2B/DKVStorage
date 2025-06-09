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
    int node=0;
    std::string config_filename;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 29999);
    unsigned short random_port = dis(gen);
    while((c =getopt(argc,argv,"n:f:"))!=-1){
        switch(c){
            case 'n':
                node=atoi(optarg);
                break;
            case 'f':
                config_filename = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    auto kv_server=new KvServer(node,500,config_filename,random_port);
    return 0;
}