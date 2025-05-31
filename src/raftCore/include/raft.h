#pragma once
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/any.hpp>


#include <memory>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "raftrpc.pb.h"
#include "raftrpcutil.h"
#include "applymsg.h"
#include "util.h"
#include "persister.h"
#include "config.h"

///////////////选民的投票状态
constexpr int Killed = 0;
constexpr int Voted = 1;   //该节点本轮已投出票
constexpr int Expire = 2;  //网络分区了，投票消息、或该竞选者落伍了
constexpr int Normal = 3;   //
class Raft: public raftRpcProtoc::raftRpc{
public:
    //日志+心跳
    void appendEntries(const raftRpcProtoc::AppendEntriesRequest *request,raftRpcProtoc::AppendEntriesResponse *response);
    //定期向状态机写入日志
    void applyTicker();
    //拍摄快照
    bool candInstallSnapshot(int last_include_term,int last_include_index,std::string snapshot);
    //发起选举
    void doElection();
    //leader发起心跳
    void doHeartbeat();
    
    //监测是否发起选举
    void electionTimeoutTicker();
    //获取日志
    std::vector<ApplyMsg> getApplyLogs();
    //获取新命令的index
    int getNewCommandIndex();
    //获取当前日志信息
    void getPrevLogInfo(int server,int* preindex,int* preterm);
    //检查当前节点是否是leader
    void getState(int* term,bool* isLeader);
    //安装快照
    void installSnapshot(const raftRpcProtoc::InstallSnapshotRequest *request, raftRpcProtoc::InstallSnapshotResponse* response);
    //检查是否该发送心跳，如果是则执行doHeartbeat()
    void leaderHeartbeatTicker();
    //leader节点发送快照
    void leaderSendSnapshot(int server);
    //leader更新commitIndex
    void leaderUpdateCommitIndex();
    //判断对象index日志是否匹配
    bool matchLog(int log_index,int log_term);
    //持久化当前状态
    void persist();

    //请求投票
    void requestVote(const raftRpcProtoc::RequestVoteRequest *request, raftRpcProtoc::RequestVoteResponse *response);
    //判断当前节点是否有最新日志
    bool upToDate(int index,int term);
    int getLastLogIndex();
    int getLastLogTerm();
    void getLastLogIndexAndTerm(int* last_log_index,int* last_log_term);
    int getLogTermFromLogIndex(int log_index);
    int getRaftStateSize();
    int getSlicesIndexFromLogIndex(int log_index);
    //请求其他节点给自己投票
    bool sendRequestVote(int server,std::shared_ptr<raftRpcProtoc::RequestVoteRequest> request,std::shared_ptr<raftRpcProtoc::RequestVoteResponse> response,std::shared_ptr<int> vote_num);
    //向其他节点发送日志
    bool sendAppendEntries(int server,std::shared_ptr<raftRpcProtoc::AppendEntriesRequest> request,std::shared_ptr<raftRpcProtoc::AppendEntriesResponse> response,std::shared_ptr<int> append_num);
    //给上层kvserver发送消息
    void pushMsgToKvServer(ApplyMsg msg);
    //读取持久化数据
    void readPersist(std::string data);
    //持久化数据
    std::string persistData();
    //更新快照
    void snapshot(int index,std::string snapshot);
    //启动
    void start(Op command,int* new_log_index,int* new_log_term,bool* is_leader);
    //初始化
    void init(std::vector<std::shared_ptr<RaftRpcUtil>> peers, int me, std::shared_ptr<Persister> persister,
        std::shared_ptr<LockQueue<ApplyMsg>> apply_ch);


 public:
 // 重写基类方法,因为rpc远程调用真正调用的是这个方法
 //序列化，反序列化等操作rpc框架都已经做完了，因此这里只需要获取值然后真正调用本地方法即可。
 void AppendEntries(google::protobuf::RpcController *controller, const raftRpcProtoc::AppendEntriesRequest *request,
                    raftRpcProtoc::AppendEntriesResponse *response, ::google::protobuf::Closure *done) override;
 void InstallSnapshot(google::protobuf::RpcController *controller,
                      const raftRpcProtoc::InstallSnapshotRequest *request,
                      ::raftRpcProtoc::InstallSnapshotResponse *response, ::google::protobuf::Closure *done) override;
 void RequestVote(google::protobuf::RpcController *controller, const raftRpcProtoc::RequestVoteRequest *request,
                  raftRpcProtoc::RequestVoteResponse *response, google::protobuf::Closure *done) override;

private:
 // for persist

    class BoostPersistRaftNode {
        public:
        friend class boost::serialization::access;
        // When the class Archive corresponds to an output archive, the
        // & operator is defined similar to <<.  Likewise, when the class Archive
        // is a type of input archive the & operator is defined similar to >>.
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar &current_term_;
            ar &voted_for_;
            ar &last_snapshot_include_index_;
            ar &last_snapshot_include_term_;
            ar &logs_;
        }
        int current_term_;
        int voted_for_;
        int last_snapshot_include_index_;
        int last_snapshot_include_term_;
        std::vector<std::string> logs_;
        std::unordered_map<std::string, int> umap_;
    };

private:
    std::mutex mtx_;
    std::vector<std::shared_ptr<RaftRpcUtil>> peers_;//其他raft节点
    std::shared_ptr<Persister> persister_;//用于持久化数据
    int me_;//表示自己的编号
    int current_term_;//目前的任期
    int votedfor_;//记录当前给谁投票
    std::vector<raftRpcProtoc::LogEntry> logs_;//log，包括指令集和任期号
    int commit_index_;//提交的index
    int last_appiled_;//上一个提交给状态机的log的index

    //leader维护每个节点的下一个index和已经匹配的index
    std::vector<int> next_index_;
    std::vector<int> match_index_;

    enum Status{
        Follower,
        Candidate,
        Leader
    };
    Status status_;
    std::shared_ptr<LockQueue<ApplyMsg>> apply_chan_;//与外部client通信管道，client从这里取日志
    std::chrono::_V2::system_clock::time_point last_rest_election_time_;//选举超时时间
    std::chrono::_V2::system_clock::time_point last_rest_heartbeat_time_;//心跳超时时间
    //用于传入快照点
    //存储快照中的最后一个日志的index和term
    int last_snapshot_include_index_;
    int last_snapshot_include_term_;
    //协程
    // std::unique_ptr<monsoon::IOManager> io_manager_=nullptr;


};