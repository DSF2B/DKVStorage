# Total

**解决的问题**
1. **一致性**： 通过Raft算法确保数据的强一致性，使得系统在正常和异常情况下都能够提供一致的数据
视图。
2. **可用性**： 通过分布式节点的复制和自动故障转移，实现高可用性，即使在部分节点故障的情况下，
系统依然能够提供服务。
3. **分区容错**： 处理网络分区的情况，确保系统在分区恢复后能够自动合并数据一致性

![review_raft_total](../images/review_raft_total.png)
项目大概可以分为以下几个部分：
1. **raft节点**：raft算法实现的核心层，负责与其他机器的raft节点沟通，达到 分布式共识 的目的。
2. **raftServer**：负责raft节点与k-v数据库中间的协调服务；负责持久化k-v数据库的数据（可选）。
3. **上层状态机（k-v数据库）**：负责数据存储。
4. **持久层**：负责相关数据的落盘，对于raft节点，根据共识算法要求，必须对一些关键数据进行落盘
处理，以保证节点宕机后重启程序可以恢复关键数据；对于raftServer，可能会有一些k-v数据库的
东西需要落盘持久化。
5. **RPC通信**：在 领导者选举、日志复制、数据查询、心跳等多个Raft重要过程中提供多节点快速简单
的通信能力。

**目前规划中没有实现节点变更功能或对数据库的切片等更进阶的功能，后面考虑学习加入。**

**需要注意的是，分布式式的共识算法实现本身是一个比较严谨的过程，因为其本身的存在是为了多个服务器之间通过共识算法达成一致性的状态，从而避免单个节点不可用而导致整个集群不可用，因此在学习过程中必须要考虑不同情况下节点宕机、断网情况下的影响。**


# SkipList
kv数据库保存键值对，可以使用链表、哈希表、跳表等。

哈希表的风险：在哈希表大小固定的情况下，随着数据不断增多，那么哈希冲突的可能性也会越高。
Redis 采用了「链式哈希」来解决哈希冲突，在不扩容哈希表的前提下，将具有相同哈希值的数据串起来，形成链接起，以便这些数据在表中仍然可以被查询到。

kv数据库采用跳表实现，每个节点包括一个vector<Node<K,V>*> forward_存储每层下一个节点的指针
![skiplist1](../images/skiplist1.png)
**查找**
跳表升序，从最高层开始遍历：
* while(cur->forward_[i] && cur->forward_[i]->getKey() < k)当下一个节点非空且小于目标值，进入下一个节点，这样cur最后就是当前层小于目标值的最大值
* cur进入当前节点的下一层继续查找，直到最底层，判断是否相等即可 

**添加**
* 类似查找，找到并记录在每一层应该插入的位置，即记录每一层该节点的前一个节点
* 如果不存在该节点，就添加，添加节点的层数采用随机算法
* 添加节点的层数算法为:产生一个[0,1]的随机数，小于0.5就继续向上插入，大于0.5就停止，产生该节点的层数。该方法保证了相邻两层之间节点数为1:2,

**删除**
* 类似添加，删除节点首先找到该节点在每一层的位置，即记录每一层该节点的前一个节点
* 对于每一层，如果前一个节点的下一个已经不是要删除的节点，停止遍历剩余的更高层；如果是要删除的节点，执行链表删除。
* 删除节点后，记得删除空的层

跳表负责存储实际的数据，需要落盘，那么就需要持久化，本项目采用boost序列化库将跳表（SkipList）中的数据持久化。

用SkipListDump类接收KV数据，并完成序列化。

**Boost序列化**

为了序列化用户定义类型的对话， serialize() 函数必须定义，它在对象序列化或从字节流中恢复是被调用。 由于 serialize () 函数既用来序列化又用来恢复数据， Boost.Serialization 除了 << 和 >> 之外还提供了 & 操作符。如果使用这个操作符，就不再需要在 serialize() 函数中区分是序列化和恢复了。

serialize () 在对象序列化或恢复时自动调用。它应从来不被明确地调用，所以应生命为私有的。 这样的话， boost::serialization::access 类必须被声明为友元，以允许 Boost.Serialization 能够访问到这个函数。

    template <typename K, typename V>
    class SkipListDump {
    public:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            // 当 Archive 是输出归档（如 text_oarchive）时，
            // ar & var 等价于 ar << var，表示​​将变量序列化到归档​​（如文件/内存流）。
            // 当 Archive 是输入归档（如 text_iarchive）时，
            // ar & var 等价于 ar >> var，表示​​从归档中读取数据并填充变量​​。
            ar &key_dump_vt_;
            ar &val_dump_vt_;
        }
        std::vector<K> key_dump_vt_;
        std::vector<V> val_dump_vt_;
    public:
        void insert(const Node<K, V> &node);
    };

    template <typename K, typename V>
    std::string SkipList<K, V>::dumpFile() {

        Node<K,V>* node = head_->forward_[0]; 
        SkipListDump<K,V> dumper; 
        while(node != nullptr) {  
            dumper.insert(*node);  
            node = node->forward_[0]; 
        }
        std::stringstream ss;  // 初始化字符串流对象ss，用于暂存序列化数据
        boost::archive::text_oarchive oa(ss);  // 创建text_oarchive对象oa，输出目标为ss
        oa << dumper;  // 调用Boost序列化机制：
                    // 隐式触发SkipListDump::serialize()，将key_dump_vt_和val_dump_vt_写入归档
        return ss.str(); 
# RPC
远程方法调用，实际就是给方法提供一层包装，使网络通信可以方便传递。

客户端使用rpcutil调用远程方法，实际通过stub_调用stub_->AppendEntries(&controller,request,response,nullptr);然后到rpc框架内的CallMethod，CallMethod由框架制作者提供，通过class MprpcChannel:public google::protobuf::RpcChannel重写CallMethod调用protobuf服务：CallMethod完成序列化-网络发送-（远程调用）-网络接受-反序列化结果返回给response。

Provider监听远程请求，收到请求执行回调函数OnMessage，解析请求的服务及方法，通过service->CallMethod(method,nullptr,request,response,done); 这里回调函数为SendRpcResponse。这里Provider的CallMethod根据服务和方法调用raftRpc的CallMethod，执行provider.NotifyService(this);注册重写的方法。

RPC框架：
**Channel**
* Channel继承google::protobuf::RpcChannel,重写CallMethod，完成完成序列化-网络发送-（远程调用）-网络接受-反序列化，结果保存到response
    ```
    virtual void CallMethod(const MethodDescriptor* method,
    RpcController* controller, const Message* request,
    Message* response, Closure* done) = 0;
    ```
* 服务接口类class RaftRpcUtil通过stub_->AppendEntries调用Channel的CallMethod
    ```
    bool RaftRpcUtil::AppendEntries(raftRpcProtoc::AppendEntriesRequest *request, raftRpcProtoc::AppendEntriesResponse *response)
    {
        MprpcController controller;
        stub_->AppendEntries(&controller,request,response,nullptr);
        //->channel_->CallMethod(descriptor()->method(0),
                        controller, request, response, done);
        return !controller.Failed();
    }
    ```
这里的框架实现者提供CallMethod的实现，完成
**Provider**
NotifyService(google::protobuf::Service \*service)可接受继承自raftRpcProtoc::raftRpc的子类对象，完成对对这些重写函数的动态注册​​，如raft中的AppendEntries等，获取该类有哪些rpc方法，保存到哈希表中，把str映射到google::protobuf::MethodDescriptor\*，有请求发来时就找到对应方法执行。

Run实现对rpc客户端的监听，收到rpc请求就使用回调函数OnMessage处理，OnMessage完成反序列化，提取出相应方法，从哈希表中找到对应方法，注册回调函数SendRpcResponse（SendRpcResponse完成序列化和响应发送），然后执行CallMethod，执行真正的方法。

```
//获取服务和方法 
google::protobuf::Service *service = it->second.m_service;      
const google::protobuf::MethodDescriptor* method = mit->second; 
//生成response和request参数,service->GetRequestPrototype()得到method所需的request类型，Login->LoginRequest
google::protobuf::Message *request = service->GetRequestPrototype(method).New();
if(!request->ParseFromString(args_str)){
    //解析请求到request
    return ;
}
google::protobuf::Message *response = service->GetResponsePrototype(method).New();
//创建一个closure派生类对象，不同参数对应不同派生类，派生类中重写run调用method_，即SendRpcResponse
google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>
                                        (this, &RpcProvider::SendRpcResponse, conn, response);
service->CallMethod(method,nullptr,request,response,done); 
```
**这里的CallMethod是重载raftRpc的CallMethod，执行对应的方法**
```
void raftRpc::CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                             ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                             const ::PROTOBUF_NAMESPACE_ID::Message* request,
                             ::PROTOBUF_NAMESPACE_ID::Message* response,
                             ::google::protobuf::Closure* done) {
  GOOGLE_DCHECK_EQ(method->service(), file_level_service_descriptors_raftrpc_2eproto[0]);
  switch(method->index()) {
    case 0:
      AppendEntries(controller,
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<const ::raftRpcProtoc::AppendEntriesRequest*>(
                 request),
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<::raftRpcProtoc::AppendEntriesResponse*>(
                 response),
             done);
      break;
    case 1:
      RequestVote(controller,
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<const ::raftRpcProtoc::RequestVoteRequest*>(
                 request),
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<::raftRpcProtoc::RequestVoteResponse*>(
                 response),
             done);
      break;
    case 2:
      InstallSnapshot(controller,
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<const ::raftRpcProtoc::InstallSnapshotRequest*>(
                 request),
             ::PROTOBUF_NAMESPACE_ID::internal::DownCast<::raftRpcProtoc::InstallSnapshotResponse*>(
                 response),
             done);
      break;
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      break;
  }
}
```


**rpc框架定义的通信协议**
header_size(4bytes) + head_str(service_name method_name args_size) + args_str
* header_size：考虑tcp粘包问题，header_size转为固定4bytes32位二进制，使用ReadVarint32i解析
* head_str：header用一个protobuffer序列化，包括服务名称、方法名称、参数长度。
* args_str：存储request,response等参数


# Protobuf

# Raft

## 领导者选举

## 日志复制

## 日志快照

## 持久化

## KVServer

## 安全性

# Boost序列化与Protobuf序列化
Boost序列化库支持多种数据格式，包括二进制、文本和XML。二进制格式（Binary format）提供高效的数据处理速度，适合性能敏感的应用。文本格式（Text format）和XML格式（XML format）则提供了更好的可读性和可编辑性，便于调试和数据交换。

Protobuf（Protocol Buffers），由Google开发，是一个灵活、高效的结构化数据存储格式。它主要使用一种类似于接口描述语言（Interface Description Language，IDL）的语法来定义数据结构。Protobuf编译器可以根据这些定义生成各种编程语言的源代码，从而实现跨语言、跨平台的数据序列化和反序列化。

|特性|Boost序列化|Protobuf序列化|
|------|------|------|
|数据处理|高灵活性，适应复杂结构|高效率，适合大规模数据|
|内存使用|较高	|较低|
|处理时间|可能较长|相对较短|
|应用场景|复杂数据结构|大量数据处理|
