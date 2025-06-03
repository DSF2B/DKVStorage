#include <iostream>
#include "raftclient.h"
#include "util.h"

int main(int argc,char** argv){
    if(argc<2){
        exit(EXIT_FAILURE);
    }
    int c=0;
    std::string config_filename;
    while((c =getopt(argc,argv,"f:"))!=-1){
        switch(c){
            case 'f':
                config_filename = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    RaftClient client;
    client.init(config_filename);
    auto start=now();
    int count=500;
    int temp=count;
    while(temp--){
        client.put("x",std::to_string(temp));

        std::string get1=client.get("x");
        std::cout<<"get return :%s"<<get1.c_str()<<std::endl;
    }
    return 0;
}
