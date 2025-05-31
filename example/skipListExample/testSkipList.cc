#include "skiplist.h"

int main(){
    SkipList<int, std::string> skipList(4);
    skipList.insertElement(1, "One");
    skipList.insertElement(2, "Two");
    skipList.insertElement(3, "Three");
    skipList.insertElement(4, "Four");

    skipList.displayList();
    
    // 查找元素
    std::string value;
    if (skipList.searchElement(3, value)) {
        std::cout << "Found key 3: " << value << std::endl;
    } else {
        std::cout << "Key 3 not found." << std::endl;
    }
    
    // 删除元素
    skipList.deleteElement(2);
    std::cout << "After deleting key 2:\n";
    skipList.displayList();
    
    // 序列化跳表到文件
    std::string dump_str = skipList.dumpFile();
    std::cout << "Serialized Skip List: " << dump_str << std::endl;
    
    // 反序列化跳表
    SkipList<int, std::string> newSkipList(4);
    newSkipList.loadFile(dump_str);
    std::cout << "After deserialization:\n";
    newSkipList.displayList();
    
    return 0;
}