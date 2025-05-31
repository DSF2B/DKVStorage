set(SRC_LIST testSkipList.cc)

add_executable(testSkipList ${SRC_LIST})

target_link_libraries(testSkipList skip_list_on_raft rpc_lib protobuf muduo_net muduo_base pthread)
