#pragma once

const bool Debug = true;
// 时间单位：time.Millisecond，不同网络环境rpc速度不同，因此需要乘以一个系数
const int debugMul=1;
// 心跳时间一般要比选举超时小一个数量级
const int HeartbeatTimeout = 25*debugMul;
const int ApplyInterval = 10*debugMul;

const int minRandomizedElectionTime = 300*debugMul;
const int maxRandomizedElectionTime = 500*debugMul;

const int CONSENSUS_TIMEOUT = 500 * debugMul;   
