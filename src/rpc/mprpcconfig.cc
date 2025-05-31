#include "mprpcconfig.h"
#include <iostream>
void MprpcConfig::LoadConfigFile(const char *config_file){
    FILE* pf = fopen(config_file,"r");
    if(pf==nullptr){
        std::cout<<config_file<<" is not exists"<<std::endl;
        exit(EXIT_FAILURE);
    }
    // 循环读取文件的每一行，直到文件末尾，feof()用于检查文件流 pf 是否已到达文件末尾
    while(!feof(pf)){
        char buf[512]={0};
        fgets(buf,512,pf);//fgets 函数逐行读取文件内容

        std::string read_buf(buf);
        Trim(read_buf);
        //解析src_buf
        if(read_buf[0] == '#' || read_buf.empty()){
            continue;
        }
        int idx=read_buf.find('=');
        if(idx==-1){//不合法
            continue;
        }
        std::string key;
        std::string value;

        key=read_buf.substr(0,idx);
        Trim(key);
        //rpcserverip=127.0.0.1\n
        int endidx=read_buf.find('\n',idx);//从idx开始查找
        value = read_buf.substr(idx+1,endidx-idx-1);
        Trim(value);
        m_configMap.insert({key,value});
    }
    fclose(pf);
}
std::string MprpcConfig::Load(const std::string &key){
    // return m_configMap[key];
    auto it=m_configMap.find(key);
    if(it!=m_configMap.end()){
        return it->second;
    }
    return "";
}

void MprpcConfig::Trim(std::string &src_buf){
    int idx=src_buf.find_first_not_of(' ');
    if(idx!=-1){
        //说明字符串前有空格
        src_buf = src_buf.substr(idx,src_buf.size()-idx);
    }
    //去除后面多余的空格
    idx=src_buf.find_last_not_of(' ');
    if(idx!=-1){
        //说明字符串前有空格
        src_buf = src_buf.substr(0,idx+1);
    }
}
