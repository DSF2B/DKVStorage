#include "raft.h"


//其他节点调用本节点的函数完成日志+心跳
void Raft::appendEntries(const raftRpcProtoc::AppendEntriesRequest *request,raftRpcProtoc::AppendEntriesResponse *response)
{
    std::lock_guard<std::mutex> lock(mtx_);
    //如果发送entries的term比自己的小，不接收entries
    if(request->term() < current_term_){
        response->set_success(false);
        response->set_term(current_term_);
        response->set_updatenextindex(-100);
        return ;
    }


    if(request->term() < current_term_){
        //如果自己的term落后，把自己设置为follower
        status_=Follower;
        current_term_=request->term();
        votedfor_=-1;
    } 
    if(request->term()!=current_term_){
        persist();
        exit(EXIT_FAILURE);  
    }
    
    //如果发送网络分区，candidate可能收到同一个term的leader的消息
    //此时本节点转为Follower
    status_=Follower;
    //重置选举定时器，告诉当前分区已经有了leader,避免重复选举
    last_rest_election_time_=now();
    //比较日志先后
    if(request->prevlogindex() > getLastLogIndex()){
        //发来的日志的上一个日志比本节点更晚，说明还少了部分日志
        response->set_success(false);
        response->set_term(current_term_);
        response->set_updatenextindex(getLastLogIndex()+1);
        persist();
        return;
    }
    else if(request->prevlogindex() < last_snapshot_include_index_){
        //如果发来的日志的上一个日志比本节点的快照更早，说明leader的日志太旧
        response->set_success(false);
        response->set_term(current_term_);
        response->set_updatenextindex(last_snapshot_include_index_+1);
    }
    //
    if(matchLog(request->prevlogindex(), request->prevlogterm())){
        //leader的index在快照和本节点最新的日志之间，需要从头开始判断哪些是匹配的日志
        //matchLog检查本节点目前最新的日志的term是否和发来的日志之前的term相同
        for(int i=0;i<request->entries_size();i++){
            auto log=request->entries(i);
            if(log.logindex() > getLastLogIndex()){
                //超过本节点的最新节点，直接添加
                logs_.push_back(log);
            }else{
                //比较是否匹配
                if(logs_[getSlicesIndexFromLogIndex(log.logindex())].logterm()==log.logterm()&&
                logs_[getSlicesIndexFromLogIndex(log.logindex())].command()!=log.command()){
                    //log位置的日志，term相同command不同，不符合raft的前向匹配，异常
                    exit(EXIT_FAILURE);
                }
                if(logs_[getSlicesIndexFromLogIndex(log.logindex())].logterm() != log.logterm()){
                    //不匹配就更新
                    logs_[getSlicesIndexFromLogIndex(log.logindex())]=log;
                }
            }
        }
        if(request->leadercommit() > commit_index_){
            //leader提交的日志比本节点提交的日志多
            //所以不能直接把本节点现有的日志全提交，最多只能提交leader已经提交的
            commit_index_=std::min(request->leadercommit(),getLastLogIndex());
        }
        if(commit_index_>getLastLogIndex()){
            persist();
            exit(EXIT_FAILURE);
        }
        response->set_success(true);
        response->set_term(current_term_);
        persist();
        return ;
    }
    else{
        //term都不匹配了,从发过来的日志之前的日志开始检查,知道本节点快照
        response->set_updatenextindex(request->prevlogindex());
        for(int index=request->prevlogindex();index>=last_snapshot_include_index_;index--){
            if(getLogTermFromLogIndex(index)!=getLogTermFromLogIndex(request->prevlogindex())){
                //找到本节点的request->prevlogindex()的term的第一个index，因为该term矛盾了，重发整个该term,优化减少rpc次数
                response->set_updatenextindex(index+1);
                break;
            }
        }
        response->set_success(false);
        response->set_term(current_term_);
        persist();
        return ;
    }

}
//定期向状态机写入日志
void Raft::applyTicker()
{
    while(true){
        std::unique_lock<std::mutex> lock(mtx_);
        auto apply_msgs=getApplyLogs();
        lock.unlock();
        for(auto& message:apply_msgs){
            apply_chan_->push(message);
        }
        sleepNMilliseconds(ApplyInterval);
    }
}
//拍摄快照
bool Raft::candInstallSnapshot(int last_include_term,int last_include_index,std::string snapshot)
{
    return true;
}
//发起选举
void Raft::doElection()
{
    std::lock_guard<std::mutex> g(mtx_);
    if(status_ != Leader){
        //当选举的时候定时器超时就必须重新选举，不然没有选票就会一直卡主
        status_=Candidate;
        //重竞选超时，term也会增加的
        current_term_++;
        //既是自己给自己投，也避免给其他candidate投
        votedfor_ = me_;
        //持久化当前节点状态
        persist();
        std::shared_ptr<int> voted_num=std::make_shared<int> (1);
        //设置当前选举时间
        last_rest_election_time_=now();
        //向每一个节点请求投票
        for(int i=0;i<peers_.size();i++){
            if(i==me_){
                continue;
            }
            int last_log_index=-1,last_log_term=-1;
            getLastLogIndexAndTerm(&last_log_index,&last_log_term);
            std::shared_ptr<raftRpcProtoc::RequestVoteRequest> request_vote_request=std::make_shared<raftRpcProtoc::RequestVoteRequest> ();
            request_vote_request->set_term(current_term_);
            request_vote_request->set_candidateid(me_);
            request_vote_request->set_lastlogindex(last_log_index);
            request_vote_request->set_lastlogterm(last_log_term);
            auto request_vote_response = std::make_shared<raftRpcProtoc::RequestVoteResponse>();
            //创建新线程执行sendRequestVote()
            std::thread t(&Raft::sendRequestVote,this,i,request_vote_request,request_vote_response,voted_num);
            t.detach();
        }
    }

}
//leader发起心跳
void Raft::doHeartbeat()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if(status_ == Leader)
    {
        //成功接收心跳或日志的follower数量
        auto append_nums = std::make_shared<int> (1);
        for(int i=0;i<peers_.size();i++){
            if(i==me_){
                continue;
            }
            if(next_index_[i]<1){
                exit(EXIT_FAILURE);
            }
            //如果需要发给该节点的index小于snapshot那么就把快照发过去
            if(next_index_[i] <last_snapshot_include_index_){
                std::thread t(&Raft::leaderSendSnapshot,this,i);
                t.detach();
                continue;
            }
            //构造response参数
            int pre_log_index=-1,pre_log_term=-1;
            getPrevLogInfo(i,&pre_log_index,&pre_log_term);
            std::shared_ptr<raftRpcProtoc::AppendEntriesRequest> append_entries_request = std::shared_ptr<raftRpcProtoc::AppendEntriesRequest> ();
            append_entries_request->set_term(current_term_);
            append_entries_request->set_leaderid(me_);
            append_entries_request->set_prevlogindex(pre_log_index);
            append_entries_request->clear_entries();
            append_entries_request->set_leadercommit(commit_index_);
            //上一条日志不是快照的最后一个日志，可以从日志数组中拿日志
            if(pre_log_index!=last_snapshot_include_index_){
                for(int j=getSlicesIndexFromLogIndex(pre_log_index)+1;
                j<logs_.size();j++){
                    //add_entries给Entries添加一个LogEntry 对象，返回指向该新对象的指针
                    raftRpcProtoc::LogEntry* send_entry_prt = append_entries_request->add_entries();
                    //对这个空的对象赋值
                    *send_entry_prt = logs_[j];
                }
            }
            //拿全部的日志
            else{
                for(const auto &item: logs_){
                    raftRpcProtoc::LogEntry* send_entry_prt = append_entries_request->add_entries();
                    *send_entry_prt = item;
                }
            }
            int last_log_index = getLastLogIndex();
            if(append_entries_request->prevlogindex() + append_entries_request->entries_size() != last_log_index)
            {
                exit(EXIT_FAILURE);//保证取走所有日志
            }
            std::shared_ptr<raftRpcProtoc::AppendEntriesResponse> append_entries_response = std::make_shared<raftRpcProtoc::AppendEntriesResponse>();
            std::thread t(&Raft::sendAppendEntries,this,i,append_entries_request,append_entries_response,append_nums);
            t.detach();
        }
        last_rest_heartbeat_time_ = now();
    }
}

//监测是否发起选举
void Raft::electionTimeoutTicker()
{
    while(true){
        //如果本节点是leader那么就睡眠
        while(status_ == Leader){
            usleep(HeartbeatTimeout);
        }
        std::chrono::duration<signed long int, std::ratio<1,1000000000>> suitable_sleep_time{};
        std::chrono::system_clock::time_point wake_time{};
        {
            mtx_.lock();
            wake_time = now();
            suitable_sleep_time = getRandomizedElectionTimeout() -(wake_time - last_rest_election_time_);
            mtx_.unlock();
        }
        if(std::chrono::duration<double,std::milli>(suitable_sleep_time).count() > 1){
            auto start = std::chrono::steady_clock::now();
            usleep(std::chrono::duration_cast<std::chrono::microseconds>(suitable_sleep_time).count());
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            if (std::chrono::duration<double, std::milli>(last_rest_election_time_ - wake_time).count() > 0) {
                //说明睡眠的这段时间last_rest_election_time_到了下一个周期，再次睡眠
                continue;
            }
            doElection();
        }
    }
}
//获取日志
std::vector<ApplyMsg> Raft::getApplyLogs()
{
    std::vector<ApplyMsg> apply_msgs;
    if(commit_index_ > getLastLogIndex()){
        exit(EXIT_FAILURE);
    }
    while(last_appiled_<commit_index_){
        last_appiled_++;
        if(logs_[getSlicesIndexFromLogIndex(last_appiled_)].logindex() != last_appiled_){
            exit(EXIT_FAILURE);
        }
        ApplyMsg apply_msg;
        apply_msg.command_vaild_=true;
        apply_msg.snapshot_vaild_=false;
        apply_msg.command_=logs_[getSlicesIndexFromLogIndex(last_appiled_)].command();
        apply_msg.command_index_=last_appiled_;
        apply_msgs.emplace_back(apply_msg);
        return apply_msgs;
    }
    
}
//获取新命令应该分配的Index
int Raft::getNewCommandIndex()
{
    auto last_log_index=getLastLogIndex();
    return last_log_index+1;
}
//获取节点i当前日志信息
void Raft::getPrevLogInfo(int i,int* preindex,int* preterm)
{
    if(next_index_[i] == last_snapshot_include_index_+1){
        //发送快照下一个日志,即第一个日志
        *preindex=last_snapshot_include_index_;
        *preterm=last_snapshot_include_term_;
        return;
    }
    *preindex = next_index_[i]-1;
    *preterm = logs_[getSlicesIndexFromLogIndex(*preindex)].logterm();
}
//检查当前节点是否是leader
void Raft::getState(int* term,bool* isLeader)
{
    std::lock_guard<std::mutex> lock(mtx_);
    *term = current_term_;
    *isLeader=(status_==Leader);
}
//安装快照
void Raft::installSnapshot(const raftRpcProtoc::InstallSnapshotRequest *request, 
    raftRpcProtoc::InstallSnapshotResponse* response)
{


}
//检查是否该发送心跳，如果是则执行doHeartbeat()
void Raft::leaderHeartbeatTicker()
{
    while(true)
    {
        //不是leader就睡眠
        while(status_!=Leader){
            usleep(1000*HeartbeatTimeout);
        }
        std::chrono::duration<signed long int, std::ratio<1, 1000000000>> suitableSleepTime{};
        std::chrono::system_clock::time_point wakeTime{};
        {
            std::lock_guard<std::mutex> lock(mtx_);
            wakeTime = now();
            //距离下次心跳的时间
            suitableSleepTime = std::chrono::milliseconds(HeartbeatTimeout) - (wakeTime -last_rest_heartbeat_time_);
        }
        if(std::chrono::duration<double, std::milli>(suitableSleepTime).count() > 1){
            auto start = std::chrono::steady_clock::now();
            usleep(std::chrono::duration_cast<std::chrono::microseconds>(suitableSleepTime).count());
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
        }
        if (std::chrono::duration<double, std::milli>(last_rest_heartbeat_time_ - wakeTime).count() > 0) {
            //睡眠的这段时间定时器被重置，说明doHeartbeat()已经执行过了，心跳计时器没有超时，继续睡眠
            continue;
        }
        doHeartbeat();
    }   
}
//leader节点发送快照
void Raft::leaderSendSnapshot(int server)
{
    std::unique_lock<std::mutex> lock(mtx_);
    raftRpcProtoc::InstallSnapshotRequest request;
    request.set_leaderid(me_);
    request.set_term(current_term_);
    request.set_lastincludedindex(last_snapshot_include_index_);
    request.set_lastincludedterm(last_snapshot_include_term_);
    request.set_data(persister_->readSnapshot());
    lock.unlock();

    raftRpcProtoc::InstallSnapshotResponse response;
    bool ok=peers_[server]->InstallSnapshot(&request,&response);
    lock.lock();
    if(!ok)return;
    if(status_!=Leader || current_term_ != request.term()){
        return ;
    }
    if(response.term() > current_term_){
        current_term_=response.term();
        votedfor_=-1;
        status_=Follower;
        persist();
        last_rest_election_time_=now();
        return;
    }
    match_index_[server] = request.lastincludedindex();
    next_index_[server] = match_index_[server]+1;
}
//leader更新commitIndex
void Raft::leaderUpdateCommitIndex()
{

}
//判断对象index日志是否匹配
bool Raft::matchLog(int log_index,int log_term)
{
    if(log_index<last_snapshot_include_index_ || log_index > getLastLogIndex()){
        //如果该log比本节点快照早 或 比本节点最新的log晚
        exit(EXIT_FAILURE);
    }
    //是否处于一个term
    return log_term == getLogTermFromLogIndex(log_index);
}
//持久化
void Raft::persist()
{   
    auto data = persistData();
    persister_->saveRaftState(data);
}

//响应投票请求，重写rpc函数，由candidate远程调用该函数为其投票
void Raft::requestVote(const raftRpcProtoc::RequestVoteRequest *request, 
    raftRpcProtoc::RequestVoteResponse *response)
{
    std::lock_guard<std::mutex> lock(mtx_);
    //退出之前调用
    // DeferClass<void> defer(this, &Raft::persist);
    //出现网络分区，该竞选者过时了
    if(request->term() < current_term_){
        response->set_term(current_term_);
        response->set_votegranted(false);
        response->set_votestate(Expire);
        persist();
        return;
    }
    //本节点也是candidata但是term小要转为Follower
    if(request->term() >current_term_){
        status_=Follower;
        current_term_=response->term();
        votedfor_=-1;
    }
    if(request->term() !=current_term_){
        persist();
        exit(EXIT_FAILURE);
    }
    int last_log_term = getLastLogIndex();
    //当candidate的日志比当前节点更新，当前节点给他投票
    //candidate日志太旧
    if(!upToDate(request->lastlogindex(),request->lastlogterm())){
        //如果是candidate的term落后
        if(request->lastlogterm() < last_log_term){
            //LOG
        }
        //index落后
        else{
            //LOG
        }
        response->set_term(current_term_);
        response->set_votegranted(false);
        response->set_votestate(Voted);
        persist();
        return;
    }
    //本节点已经投过票了 && 已经投给过当前节点但发生网络问题
    if(votedfor_!=-1 && votedfor_!=request->candidateid()){
        response->set_term(current_term_);
        response->set_votegranted(false);
        response->set_votestate(Voted);
        persist();
        return ;
    }else{
        //投给candidate
        votedfor_=request->candidateid();
        last_rest_election_time_=now();
        response->set_term(current_term_);
        response->set_votegranted(true);
        response->set_votestate(Normal);
        persist();
        return;
    }
}
//判断candidata的日志是否比当前节点日志更新
bool Raft::upToDate(int index,int term)
{
    int last_term=-1,last_index=-1;
    getLastLogIndexAndTerm(&index,&term);
    if(term>last_term){
        return 1;
    }else if(term==last_term){
        if(index>=last_index){
            return 1;
        }
    }
    return 0;
}
int Raft::getLastLogIndex()
{
    int last_log_index=-1;
    int _ = -1;
    getLastLogIndexAndTerm(&last_log_index,&_);
    return last_log_index;
}
int Raft::getLastLogTerm()
{
    int last_log_term=-1;
    int _ = -1;
    getLastLogIndexAndTerm(&_,&last_log_term);
    return last_log_term;
}
void Raft::getLastLogIndexAndTerm(int* last_log_index,int* last_log_term)
{
    if(logs_.empty()){
        *last_log_index = last_snapshot_include_index_;
        *last_log_term = last_snapshot_include_term_;
        return;
    }else{
        *last_log_index = logs_[logs_.size()-1].logindex();
        *last_log_term = logs_[logs_.size()-1].logterm();
        return ;
    }
}
int Raft::getLogTermFromLogIndex(int log_index)
{
    int last_log_index = getLastLogIndex();
    if(log_index < last_snapshot_include_index_){
        exit(EXIT_FAILURE);
    }
    if(log_index > last_log_index){
        exit(EXIT_FAILURE);
    }
    if(log_index == last_snapshot_include_index_){
        return last_snapshot_include_index_;
    }else{
        return logs_[getSlicesIndexFromLogIndex(log_index)].logterm();
    }
}
int Raft::getRaftStateSize()
{
    return persister_->raftStateSize();
}
//把日志索引(逻辑索引)转换为日志数组下标,就是相对快照的下标
int Raft::getSlicesIndexFromLogIndex(int log_index)
{   
    if(log_index <=last_snapshot_include_index_){
        exit(EXIT_FAILURE);
    }
    int last_log_index = getLastLogIndex();
    if(log_index > last_log_index){
        exit(EXIT_FAILURE);
    }
    int slice_index=log_index - last_snapshot_include_index_ -1;
    return slice_index;
}
//请求其他节点给自己投票，
bool Raft::sendRequestVote(int server,std::shared_ptr<raftRpcProtoc::RequestVoteRequest> request,
    std::shared_ptr<raftRpcProtoc::RequestVoteResponse> response,std::shared_ptr<int> vote_num)
{
    auto start = now();
    bool ok = peers_[server]->RequestVote(request.get(),response.get());
    if(!ok)return ok;
    std::lock_guard<std::mutex> lock(mtx_);
    //如果要求投票的节点的term大于自己的term,说明无法获得其投票
    if(response->term() > current_term_){
        status_=Follower;
        current_term_=response->term();
        votedfor_=-1;
        persist();
        return true;
    }
    else if(response->term() < current_term_){
        return true;
    }
    else{
        if(!response->votegranted()){
            //该节点没给自己投票，结束
            return true;
        }
        *vote_num=*vote_num+1;
        if(*vote_num >= peers_.size()/2+1){
            //获得半数以上投票
            *vote_num=0;
            if(status_ == Leader){
                //已经是leader又被选为leader,不正常
                exit(EXIT_FAILURE);
            }
            status_=Leader;
            int last_log_index = getLastLogIndex();
            for(int i=0;i<next_index_.size();i++){
                //next_index从自己的未持久化的index开始
                next_index_[i]=last_log_index+1;
                //更换leader要从头开始检查
                match_index_[i]=0;
            }
            std::thread t(&Raft::doHeartbeat,this);
            t.detach();
            persist();
        }
        return true;
    }
}
//向其他节点发送日志,调用其AppendEntries远程方法
bool Raft::sendAppendEntries(int i,std::shared_ptr<raftRpcProtoc::AppendEntriesRequest> request,
    std::shared_ptr<raftRpcProtoc::AppendEntriesResponse> response,std::shared_ptr<int> append_num)
{
    bool ok=peers_[i]->AppendEntries(request.get(),response.get());
    if(!ok){
        return ok;
    }
    //如果要求更新的节点的term比自己的还新
    if(response->term() > current_term_){
        //发现自己term过时了，降级为follower
        status_=Follower;
        current_term_= response->term();
        votedfor_=-1;
        persist();
        return ok;
    }
    else if(response->term()< current_term_){
        return ok;
    }
    if(status_!=Leader){
        return ok;
    }
    if(response->term() != current_term_){
        exit(EXIT_FAILURE);
    }
    if(!response->success()){
        if(response->updatenextindex() != -100){
            //失败，从follower提供的index处重试
            next_index_[i]=response->updatenextindex();
        }
    }else{
        //有一个follower接收了日志
        *append_num=*append_num+1;
        match_index_[i] = std::max(match_index_[i],request->prevlogindex()+request->entries_size());
        next_index_[i]=match_index_[i]+1;
        int last_log_index=getLastLogIndex();
        if(next_index_[i]> last_log_index +1){
            exit(EXIT_FAILURE);
        }
        //检查是否可以提交日志
        if(*append_num >= peers_.size()/2+1){
            *append_num=0;//避免重复提交
            if(request->entries_size() > 0 && request->entries(request->entries_size()-1).logterm() == current_term_){
                commit_index_=std::max(commit_index_,request->prevlogindex() + request->entries_size());
            }
            if(commit_index_>last_log_index){
                exit(EXIT_FAILURE);
            }
        }
    }
    return ok;
}
//给上层kvserver发送消息
void Raft::pushMsgToKvServer(ApplyMsg msg)
{
    apply_chan_->push(msg);
}
//读取被持久化的节点
void Raft::readPersist(std::string data)
{
    if(data.empty()){
        return ;
    }
    std::stringstream iss(data);
    boost::archive::text_iarchive ia(iss);
    BoostPersistRaftNode boostPersistRaftNode;
    ia >> boostPersistRaftNode;
    current_term_ = boostPersistRaftNode.current_term_;
    votedfor_ = boostPersistRaftNode.voted_for_;
    last_snapshot_include_index_ = boostPersistRaftNode.last_snapshot_include_index_;
    last_snapshot_include_term_ = boostPersistRaftNode.last_snapshot_include_term_;
    logs_.clear();
    for(auto& item:boostPersistRaftNode.logs_){
        raftRpcProtoc::LogEntry log_entry;
        //log先通过protobuffer序列化，再通过boost序列化持久化
        log_entry.ParseFromString(item);
        logs_.emplace_back(log_entry);
    }
}
//持久化数据,返回序列化后的节点state
std::string Raft::persistData()
{
    BoostPersistRaftNode boostPersistRaftNode;
    boostPersistRaftNode.current_term_=current_term_;
    boostPersistRaftNode.voted_for_=votedfor_;
    boostPersistRaftNode.last_snapshot_include_index_=last_snapshot_include_index_;
    boostPersistRaftNode.last_snapshot_include_term_=last_snapshot_include_term_;
    for(auto& item:logs_){
        //将log结构体通过protoc序列化为st，再通过boost序列化持久化
        boostPersistRaftNode.logs_.push_back(item.SerializeAsString());
    }
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << boostPersistRaftNode;
    return ss.str();
}
//将index之后的log保存为快照
void Raft::snapshot(int index,std::string snapshot)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if(last_snapshot_include_index_ >= index || index>commit_index_){
        return;
    }
    auto last_log_index = getLastLogIndex();
    int new_last_snapshot_include_index = index;
    int new_last_snapshot_include_term = logs_[getSlicesIndexFromLogIndex(index)].logterm();
    std::vector<raftRpcProtoc::LogEntry> trunckedLogs;

    for(int i=index+1;i<=getLastLogIndex();i++){
        trunckedLogs.emplace_back(logs_[getSlicesIndexFromLogIndex(i)]);
    }
    last_snapshot_include_index_=new_last_snapshot_include_index;
    last_snapshot_include_term_ = new_last_snapshot_include_term;
    logs_=trunckedLogs;
    commit_index_=std::max(commit_index_,index);
    last_appiled_=std::max(last_appiled_,index);

    persister_->save(persistData(),snapshot);
    if(logs_.size() + last_snapshot_include_index_ != last_log_index){
        exit(EXIT_FAILURE);  
    }
}
//启动
void Raft::start(Op command,int* new_log_index,int* new_log_term,bool* is_leader){
    std::lock_guard<std::mutex> lock(mtx_);
    if(status_ != Leader){
        *new_log_index=-1;
        *new_log_term=-1;
        *is_leader = false;
        return;
    }
    // leader应该不停的向各个Follower发送AE来维护心跳和保持日志同步，
    // 目前的做法是新的命令来了不会直接执行，而是等待leader的心跳触发
    raftRpcProtoc::LogEntry new_log_entry;
    new_log_entry.set_command(command.asString());
    new_log_entry.set_logterm(current_term_);
    new_log_entry.set_logindex(getNewCommandIndex());
    logs_.emplace_back(new_log_entry);

    int last_log_index=getLastLogIndex();
    persist();
    *new_log_index=new_log_entry.logindex();
    *new_log_term=new_log_entry.logterm();
    *is_leader=true;
}
//初始化
void Raft::init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, 
    int me, 
    std::shared_ptr<Persister> persister,
    std::shared_ptr<LockQueue<ApplyMsg>> apply_ch)
{
    peers_ = peers;
    me_ = me;
    persister_=persister;
    mtx_.lock();
    current_term_=0;
    int votedfor_=-1;
    logs_.clear();
    commit_index_=0;
    last_appiled_=0;
    for(int i=0;i<peers.size();i++){
        next_index_.push_back(0);
        match_index_.push_back(0);
    }
    status_=Follower;
    apply_chan_=apply_ch;
    last_rest_election_time_=now();
    last_rest_heartbeat_time_=now();
    readPersist(persister_->readRaftState());
    if(last_snapshot_include_index_>0){
        last_appiled_=last_snapshot_include_index_;
    }
    last_snapshot_include_index_=0;
    last_snapshot_include_term_=0;
    mtx_.unlock();
    //三个计时器，维护心跳、选举和日志同步
    std::thread t(&Raft::leaderHeartbeatTicker,this);
    t.detach();
    std::thread t2(&Raft::electionTimeoutTicker,this);
    t2.detach();
    std::thread t3(&Raft::applyTicker,this);
    t3.detach();

}



 // 重写基类方法,因为rpc远程调用真正调用的是这个方法
 //序列化，反序列化等操作rpc框架都已经做完了，因此这里只需要获取值然后真正调用本地方法即可。
 void Raft::AppendEntries(google::protobuf::RpcController *controller, 
    const raftRpcProtoc::AppendEntriesRequest *request,
    raftRpcProtoc::AppendEntriesResponse *response, 
    google::protobuf::Closure *done)
{
    appendEntries(request,response);
    done->Run();
}
 void Raft::InstallSnapshot(google::protobuf::RpcController *controller,
    const raftRpcProtoc::InstallSnapshotRequest *request,
    raftRpcProtoc::InstallSnapshotResponse *response, 
    google::protobuf::Closure *done)
{
    installSnapshot(request,response);
    done->Run();
}
 void Raft::RequestVote(google::protobuf::RpcController *controller, 
    const raftRpcProtoc::RequestVoteRequest *request,
    raftRpcProtoc::RequestVoteResponse *response, 
    google::protobuf::Closure *done)
{
    requestVote(request,response);
    done->Run();
}

