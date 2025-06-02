#include "kvserver.h"


KvServer::KvServer(int me,int max_raft_state,std::string node_info_filename,short port):skiplist_(6){
    //node_info_filename:server节点信息
    //port:
    me_=me;
    std::shared_ptr<Persister> persister = std::make_shared<Persister> (me);    
    max_raft_state_ = max_raft_state;
    apply_chan_=std::make_shared<LockQueue<ApplyMsg>>();
    raft_node_ = std::make_shared<Raft> ();
    //注册rpc服务，既与raft节点通信，又要接受client远程调用
    std::thread t([this,port]{
        RpcProvider provider;
        provider.NotifyService(this);//server
        provider.NotifyService(raft_node_.get());//raft
        provider.Run(me_,port);
    });
    t.detach();

    //开启rpc远程调用服务，要保证所有raft节点都开启rpc接受功能后才能开启rpc远程调用功能
    //睡眠等待其他节点
    std::cout << "raftServer node:" << me_ << " start to sleep to wait all ohter raftnode start!!!!" << std::endl;
    sleep(6);
    std::cout << "raftServer node:" << me_ << " wake up!!!! start to connect other raftnode" << std::endl;
    MprpcConfig config;
    config.LoadConfigFile(node_info_filename.c_str());

    std::vector<std::pair<std::string, short>> ip_port_vt;
    for(int i=0;i<INT_MAX-1;i++){
        std::string node="node"+std::to_string(i);
        std::string node_ip = config.Load(node + "ip");
        std::string node_port_str = config.Load(node + "port");
        if(node_ip.empty()){
            break;
        }
        ip_port_vt.emplace_back(node_ip,atoi(node_port_str.c_str()));
    }
    std::vector<std::shared_ptr<RaftRpcUtil>> servers;
    //连接每个raft节点
    for(int i=0;i<ip_port_vt.size();i++){
        if(i==me_){
            servers.push_back(nullptr);
            continue;
        }
        std::string other_node_ip=ip_port_vt[i].first;
        short other_node_port = ip_port_vt[i].second;
        auto* rpc = new RaftRpcUtil(other_node_ip,other_node_port);
        servers.push_back(std::make_shared<RaftRpcUtil>(rpc));
        std::cout << "node" << me_ << " 连接node" << i << "success!" << std::endl;
    }
    //其他节点启动后再启动server
    sleep(ip_port_vt.size() -me_);
    raft_node_->init(servers,me_,persister,apply_chan_);
    // kv的server直接与raft通信，但kv不直接与raft通信，所以需要把ApplyMsg的chan传递下去用于通信，两者的persist也是共用的
    // m_kvDB; //kvdb初始化
    skiplist_;
    wait_applychan_;
    last_request_id_;
    auto snapshot = persister->readSnapshot();
    if (!snapshot.empty()) {
        readSnapShotToInstall(snapshot);
    }
    //向其他节点宣告自己就是leader
    std::thread t2(&KvServer::readRaftApplyCommandLoop, this);
    t2.join();//由於ReadRaftApplyCommandLoop一直不會結束，达到一直卡在这的目的
}


void KvServer::dprintfKVDB(){
    if(!Debug){
        return ;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    skiplist_.displayList();
}
void KvServer::executePutOpOnKVDB(Op op){
    std::unique_lock<std::mutex> lock(mtx_);
    skiplist_.insertSetElement(op.key_,op.value_);
    last_request_id_[op.client_id_] = op.request_id_;
    lock.unlock();
    dprintfKVDB();
}
void KvServer::executeAppendOpOnKVDB(Op op){
    std::unique_lock<std::mutex> lock(mtx_);
    skiplist_.insertSetElement(op.key_,op.value_);
    last_request_id_[op.client_id_] = op.request_id_;
    lock.unlock();
    dprintfKVDB();

}
void KvServer::executeGetOpOnKVDB(Op op, std::string *value, bool *exist){
    std::unique_lock<std::mutex> lock(mtx_);
    *value="";
    *exist=false;
    if(skiplist_.searchElement(op.key_,op.value_)){
        *exist=true;
    }
    last_request_id_[op.client_id_]=op.request_id_;
    lock.unlock();
}
//本地方法,处理来自client的get rpc，从
void KvServer::get(const raftKVRpcProtoc::GetRequest *request,
    raftKVRpcProtoc::GetResponse *response){
    Op op;
    op.operation_="Get";
    op.key_=request->key();
    op.value_="";
    op.client_id_=request->clientid();
    op.request_id_=request->requestid();

    int raft_index=-1;
    int _=-1;
    bool is_leader=-1;
    raft_node_->start(op,&raft_index,&_,&is_leader);

    if(!is_leader){
        response->set_err(ErrWrongLeader);
        return;
    }
}
//server向众raft节点增加数据
void KvServer::putAppend(const raftKVRpcProtoc::PutAppendRequest *request,
    raftKVRpcProtoc::PutAppendResponse *response){

}
//raft通过applymsg管道传递message，server向状态机传递command
void KvServer::getCommandFromRaft(ApplyMsg message){
    Op op;
    //解析
    op.parseFromString(message.command_);
    if(message.command_index_ < last_snapshot_raftlog_index_){
        return ;
    }
    //command是否重复
    if(!ifRequestDuplicate(op.client_id_,op.request_id_)){
        if(op.operation_=="Put"){
            executePutOpOnKVDB(op);
        }
        if(op.operation_ == "Append"){
            executeAppendOpOnKVDB(op);
        }
    }
    //server也是raft，处理快照问题
    if(max_raft_state_ !=-1){
        //如果raft的log太大（大于指定的比例）就制作快照
        ifNeedToSendSnapShotCommand(message.command_index_,9);
    }
    sendMessageToWaitChan(op,message.command_index_);
}
//检查是否是重复的请求
bool KvServer::ifRequestDuplicate(std::string client_id, int request_id){
    std::lock_guard<std::mutex> lock(mtx_);
    if(last_request_id_.find(client_id) == last_request_id_.end()){
        return false;
    }
    //小于最新的请求的id就是过去的请求
    return request_id <= last_request_id_[client_id];
}
//一直等待raft传来的applyCh
void KvServer::readRaftApplyCommandLoop(){
    while(true){
        //如果只操作applyChan不用拿锁，因为applyChan自己带锁
        auto message = apply_chan_->pop();
        if(message.command_vaild_){
            getCommandFromRaft(message);
        }
        if(message.snapshot_vaild_){
            getSnapShotFromRaft(message);
        }
    }
}


void KvServer::readSnapShotToInstall(std::string snapshot){
    if(snapshot.empty()){
        return ;   
    }
    //从快照中还原出server状态
    parseFromString(snapshot);
}
bool KvServer::sendMessageToWaitChan(const Op &op, int raft_index){
    std::lock_guard<std::mutex> lock(mtx_);
    if(wait_applychan_.find(raft_index) == wait_applychan_.end()){
        return false;
    }
    wait_applychan_[raft_index]->push(op);
    return true;
}
// 检查是否需要制作快照，需要的话就向raft制作快照
void KvServer::ifNeedToSendSnapShotCommand(int raft_index, int proportion){
    if(raft_node_->getRaftStateSize() > max_raft_state_ /10.0){
        //大于0.1最大statesize
        //制作快照
        auto snapshot = makeSnapShot();
        //去掉index之前的log
        raft_node_->snapshot(raft_index,snapshot);
    }
}
// Handler the SnapShot from kv.rf.applyCh
void KvServer::getSnapShotFromRaft(ApplyMsg message){
    std::lock_guard<std::mutex> lock(mtx_);
    if(raft_node_->candInstallSnapshot(message.snapshot_term_,message.snapshot_index_,message.snapshot_)){
        readSnapShotToInstall(message.snapshot_);
        last_snapshot_raftlog_index_=message.snapshot_index_;
    }
}
std::string KvServer::makeSnapShot(){
    std::lock_guard<std::mutex> lock(mtx_);
    std::string snapshot_data = getSnapshotData();//snapshot序列化
    return snapshot_data;
}

void KvServer::PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProtoc::PutAppendRequest *request,
    ::raftKVRpcProtoc::PutAppendResponse *response, ::google::protobuf::Closure *done){
    putAppend(request,response);
    done->Run();
}
void KvServer::Get(google::protobuf::RpcController *controller, const ::raftKVRpcProtoc::GetRequest *request,
    ::raftKVRpcProtoc::GetResponse *response, ::google::protobuf::Closure *done){
    get(request,response);
    done->Run();
}