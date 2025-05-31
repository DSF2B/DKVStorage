#pragma once
#include <string>
class ApplyMsg{
public:
    bool command_vaild_;
    std::string command_;
    int command_index_;
    bool snapshot_vaild_;
    std::string snapshot_;
    int snapshot_term_;
    int snapshot_index_;
public:
    ApplyMsg():
        command_vaild_(false),
        command_(),
        command_index_(-1),
        snapshot_vaild_(-1),
        snapshot_term_(-1),
        snapshot_index_(-1){};
};