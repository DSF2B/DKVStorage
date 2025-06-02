#pragma once
#include <boost/any.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>

#include <iostream>
#include<unordered_map>
#include<mutex>


#include "kvserverrpc.pb.h"
#include "raft.h"
#include "skiplist.h"
#include "rpcprovider.h"
#include "mprpcconfig.h"

//raft节点、kvdb、client中间节点
class KvServer: raftKVRpcProtoc::KVServerRpc{
public:
    KvServer()=delete;
    KvServer(int me,int max_raft_state,std::string node_info_filename,short port);

    void dprintfKVDB();
    void executePutOpOnKVDB(Op op);
    void executeAppendOpOnKVDB(Op op);
    void executeGetOpOnKVDB(Op op, std::string *value, bool *exist);
    //get和put本地实现
    void get(const raftKVRpcProtoc::GetRequest *request,
        raftKVRpcProtoc::GetResponse *response);
    void putAppend(const raftKVRpcProtoc::PutAppendRequest *request,
        raftKVRpcProtoc::PutAppendResponse *response);

    void getCommandFromRaft(ApplyMsg message);
    bool ifRequestDuplicate(std::string client_id, int request_id);
    //一直等待raft传来的applyCh
    void readRaftApplyCommandLoop();
    void readSnapShotToInstall(std::string snapshot);
    bool sendMessageToWaitChan(const Op &op, int raft_index);
    // 检查是否需要制作快照，需要的话就向raft之下制作快照
    void ifNeedToSendSnapShotCommand(int reft_index, int proportion);
    // Handler the SnapShot from kv.rf.applyCh
    void getSnapShotFromRaft(ApplyMsg message);
    std::string makeSnapShot();
    //client通过rpc调用kvserver的get、putappend，内部调用本地方法
    void PutAppend(google::protobuf::RpcController *controller, const ::raftKVRpcProtoc::PutAppendRequest *request,
                 ::raftKVRpcProtoc::PutAppendResponse *response, ::google::protobuf::Closure *done) override;
    void Get(google::protobuf::RpcController *controller, const ::raftKVRpcProtoc::GetRequest *request,
           ::raftKVRpcProtoc::GetResponse *response, ::google::protobuf::Closure *done) override;

  /////////////////serialiazation start ///////////////////////////////
private:
    std::mutex mtx_;
    int me_;
    std::shared_ptr<Raft> raft_node_;
    std::shared_ptr<LockQueue<ApplyMsg>> apply_chan_;//与raft节点通信管道
    int max_raft_state_;
    std::string serialized_KVData_;
    SkipList<std::string,std::string> skiplist_;
    // std::unordered_map<std::string,std::string> kvDB_;
    //每个raft的op保存为一个lockqueue
    std::unordered_map<int,LockQueue<Op>*> wait_applychan_;
    //记录每个client最新的请求的id
    std::unordered_map<std::string,int> last_request_id_;

    int last_snapshot_raftlog_index_;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)  //这里面写需要序列话和反序列化的字段
    {
        ar &serialized_KVData_;
        ar &last_request_id_;
    }
    std::string getSnapshotData() {
        serialized_KVData_ = skiplist_.dumpFile();
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << *this;
        serialized_KVData_.clear();
        return ss.str();
    }

    void parseFromString(const std::string &str) {
        std::stringstream ss(str);
        boost::archive::text_iarchive ia(ss);
        ia >> *this;
        skiplist_.loadFile(serialized_KVData_);
        serialized_KVData_.clear();
    }

};