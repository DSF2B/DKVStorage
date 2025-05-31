#pragma once
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>

static std::string delimiter = ":";

template <typename K, typename V>
class Node{
public:
    Node(K k,V v,int level);
    ~Node();
    K getKey() const;
    V getValue() const;
    void setValue(V v);
    // Node<K,V>** forward_;//一个Node指针数组，存储该节点在每一层的下一个节点的地址
    std::vector<Node<K,V>*> forward_;
    int node_level_;
private:
    K key_;
    V value_;
};

template <typename K, typename V>
Node<K, V>::Node(K k,V v,int level): key_(k), value_(v), node_level_(level) {
    // forward_ = new Node<K, V> *[level + 1];
    // memset(forward_, 0, sizeof(Node<K, V> *) * (level + 1));
    forward_ = std::vector<Node<K, V>*>(level + 1, nullptr);
};

template <typename K, typename V>
Node<K, V>::~Node() {
    // delete[] forward_;
}

template <typename K, typename V>
K Node<K, V>::getKey() const {
    return key_;
};

template <typename K, typename V>
V Node<K, V>::getValue() const {
    return value_;
};
template <typename K, typename V>
void Node<K, V>::setValue(V value) {
    this->value = value;
};

// Class template to implement node
template <typename K, typename V>
class SkipListDump {
public:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar &key_dump_vt_;
        ar &val_dump_vt_;
    }
    std::vector<K> key_dump_vt_;
    std::vector<V> val_dump_vt_;
public:
    void insert(const Node<K, V> &node);
};

template <typename K, typename V>
void SkipListDump<K, V>::insert(const Node<K, V> &node) {
    key_dump_vt_.emplace_back(node.getKey());
    val_dump_vt_.emplace_back(node.getValue());
}


template <typename K, typename V>
class SkipList{
public:
    SkipList(int max_level);
    ~SkipList();
    int getRandomLevel();
    Node<K,V>*createNode(K k,V v,int level);
    int insertElement(const K k, const V v);
    void displayList();
    bool searchElement(K k, V& v);
    void deleteElement(K k);
    void insertSetElement(K& k,V& v);
    std::string dumpFile();
    void loadFile(const std::string &dump_str);
    void clear(Node<K,V>* cur);//递归
    int size();
private:
    void getKVFromString(const std::string &str,std::string *key,std::string* value);
    bool isVaildString(const std::string &str);
private:
    int max_level_;
    int skip_list_level_;//current level
    Node<K,V> *head_;
    std::ofstream file_writer_;
    std::ifstream file_reader_;
    int element_count_;
    std::mutex mtx_;
};

template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level){
    max_level_ = max_level;
    skip_list_level_ = 0;
    element_count_ = 0;
    K k;
    V v;
    head_ = new Node<K,V> (k,v,max_level_);
}

template <typename K, typename V>
SkipList<K, V>::~SkipList(){
    if(file_reader_.is_open()){
        file_reader_.close();
    }
    if(file_writer_.is_open()){
        file_reader_.close();
    }
    //递归删除跳表内容
    if(head_->forward_[0]!=nullptr){
        clear(head_->forward_[0]);
    }
    delete head_;
}

template <typename K, typename V>
int SkipList<K, V>::getRandomLevel(){
    int k=1;
    while(rand()%2 == 1){
        k++;
    }
    k = k<max_level_?k:max_level_;
    return k;
}

template <typename K, typename V>
Node<K,V>* SkipList<K, V>::createNode(K k,V v,int level){
    Node<K,V> *n = new Node(k,v,level);
    return n;
}

template <typename K, typename V>
int SkipList<K, V>::insertElement(K k,V v){
    std::lock_guard<std::mutex> lock(mtx_);
    Node<K,V>* cur = head_;
    // Node<K, V> *update[max_level_+1];
    std::vector<Node<K,V>*> update(max_level_+1,nullptr);
    for(int i=skip_list_level_;i>=0;i--){
        //每层升序排列，找到第一个大于k的节点
        //下一次循环从上一层跳下去
        while(cur->forward_[i] != nullptr && cur->forward_[i]->getKey() < k){
            cur=cur->forward_[i];
        }
        update[i] = cur;
    }
    //返回底层
    cur=cur->forward_[0];
    if(cur!=nullptr && cur->getKey() == k){
        //该key已经存在
        std::cout<<"key:"<<k<<"exists"<<std::endl;
        return 1;
    }
    if(cur==nullptr || cur->getKey()!=k){
        int random_level = getRandomLevel();

        if(random_level>skip_list_level_){
            //如果增加了层数,给新节点加上额外的层
            for(int i=skip_list_level_;i<random_level+1;i++){
                update[i]=head_;
            }
            skip_list_level_=random_level;
        }
        Node<K,V>* new_node = createNode(k,v,random_level);

        for(int i=0;i<random_level+1;i++){
            new_node->forward_[i]=update[i]->forward_[i];
            update[i]->forward_[i]=new_node;
        }
        std::cout<<"insert key:"<<k<<"value:"<<v<<std::endl;
        element_count_++;
    }
    return 0;
}

template <typename K, typename V>
void SkipList<K, V>::displayList(){
    std::cout << "\n*****Skip List*****"<< "\n";
    for(int i=0;i<skip_list_level_+1;i++){
        Node<K,V>*node=head_->forward_[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->getKey() << ":" << node->getValue() << ";";
            node = node->forward_[i];
        }
        std::cout << std::endl;
    }
}

template <typename K, typename V>
bool SkipList<K, V>::searchElement(K k, V& v){
    Node<K,V>* cur=head_;

    for(int i=skip_list_level_;i>=0;i--){
        while(cur->forward_[i] && cur->forward_[i]->getKey() < k){
            cur=cur->forward_[i];
        }
    }
    cur=cur->forward_[0];
    if(cur && cur->getKey() == k){
        v=cur->getValue();
        return true;
    }
    return false;
}   

template <typename K, typename V>
void SkipList<K, V>::deleteElement(K k){
    std::lock_guard<std::mutex> lock(mtx_);
    Node<K,V> *cur=head_;
    // Node<K,V> *update[max_level_+1];
    std::vector<Node<K,V>*> update(max_level_+1,nullptr);
    for(int i=skip_list_level_;i>=0;i--){
        while(cur->forward_[i] && cur->forward_[i]->getKey() < k){
            cur=cur->forward_[i];
        }
        update[i]=cur;
    }
    cur=cur->forward_[0];
    if(cur!=nullptr && cur->getKey() == k){
        for(int i=0;i<skip_list_level_+1;i++){
            if(update[i]->forward_[i] != cur){
                break;
            }
            update[i]->forward_[i] = cur->forward_[i];
        }
        //删掉该节点后，去掉空的层
        while(skip_list_level_ > 0 && head_->forward_[skip_list_level_] == 0){
            skip_list_level_--;
        }
        delete cur;
        element_count_--;        
    }
    return;
}

template <typename K, typename V>
void SkipList<K, V>::insertSetElement(K& k,V& v){
    //插入元素，如果存在，改变其值
    V oldValue;
    if(searchElement(k,oldValue)){
        deleteElement(k);
    }
    insertElement(k,v);
}

template <typename K, typename V>
std::string SkipList<K, V>::dumpFile(){
    Node<K,V>* node = head_->forward_[0];
    SkipListDump<K,V> dumper;
    while(node != nullptr){
        dumper.insert(*node);
        node=node->forward_[0];
    }
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa<<dumper;
    return ss.str();
}

template <typename K, typename V>
void SkipList<K, V>::loadFile(const std::string &dump_str){
    if (dump_str.empty()){
        return;
    }
    SkipListDump<K, V> dumper;
    std::stringstream iss(dump_str);
    boost::archive::text_iarchive ia(iss);
    ia >> dumper;
    for (int i = 0; i < dumper.key_dump_vt_.size(); ++i) {
        insertElement(dumper.key_dump_vt_[i], dumper.val_dump_vt_[i]);
    }
}

template <typename K, typename V>
void SkipList<K, V>::clear(Node<K,V>* cur){//递归
    if (cur->forward_[0] != nullptr) {
        clear(cur->forward_[0]);
    }
    delete cur;
}
template <typename K, typename V>
int SkipList<K, V>::size(){
    return element_count_;
}

template <typename K, typename V>
void SkipList<K, V>::getKVFromString(const std::string &str,std::string *k,std::string* v){
    if(isVaildString(str)){
        return ;
    }
    *k=str.substr(0,str.find(delimiter));
    *v=str.substr(str.find(delimiter)+1,str.length());
}

template <typename K, typename V>
bool SkipList<K, V>::isVaildString(const std::string &str){
    if(str.empty()){
        return false;
    }
    if(str.find(delimiter) == std::string::npos){
        return false;
    }
    return true;
}