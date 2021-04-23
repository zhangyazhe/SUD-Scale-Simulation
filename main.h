/*********************************************************************************
  * FileName:  main.h
  * Author:  Yazhe Zhang
  * Date:  2021.4.17
  * Description:  simulation of SUD Scale
**********************************************************************************/

#ifndef SUD_SCALE_SIMULATION_MAIN_H
#define SUD_SCALE_SIMULATION_MAIN_H

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
using namespace std;

/*
 * g_DiskNumOrigin小于g_DiskNumAfterScale时执行扩容操作
 * g_DiskNumOrigin大于g_DiskNumAfterScale时执行缩容操作
 * g_DiskNumOrigin等于g_DiskNumAfterScale时执行数据重分布操作
 */
int g_DiskNumOrigin = 12;
int g_DiskNumAfterScale = 8;
const int g_MaxDiskNum = 10000;
const int g_StripeNum = 6000;
const int g_N = 4;
const int g_K = 3;
int g_Optimal = 1 + (g_K * g_StripeNum * g_N) / (g_DiskNumAfterScale * (g_DiskNumAfterScale - 1));
const int g_Debug = 0;  //是否开启调试模式
const int g_Evaluation = 0;

vector<vector<int> > disks; //用于表示每个节点中存储块的情况
unordered_map<int, vector<int> > block_location;

bool cmp(pair<int, int> p1, pair<int, int> p2);
int G[g_MaxDiskNum][g_MaxDiskNum] = {0}; //表示两个节点之间的边数
void InitDisks();
void InitGraph();
void SUDExpand();
void SUDShrink();
pair<int, int> SelectTravelBlock(int disk, int bottleneck_disk);
int FindTargetDisk(int block_no);
void Redistribute();
void Evaluation();

#endif //SUD_SCALE_SIMULATION_MAIN_H
