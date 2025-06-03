set(SRC_LIST testraftserver.cc)

add_executable(testRaftServer ${SRC_LIST})

target_link_libraries(testRaftServer skip_list_on_raft rpc_lib protobuf muduo_net muduo_base pthread)


set(SRC_LIST2 testraftclient.cc)

add_executable(testRaftClient ${SRC_LIST2} ${src_common} ${src_raftClient})

target_link_libraries(testRaftClient skip_list_on_raft rpc_lib protobuf muduo_net muduo_base pthread)
