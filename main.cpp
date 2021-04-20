/*********************************************************************************
  * FileName:  main.cpp
  * Author:  Yazhe Zhang
  * Date:  2021.4.17
  * Description:  simulation of SUD Scale
**********************************************************************************/

#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>
#include <map>
#include <assert.h>
#include <math.h>
#include "main.h"

using namespace std;

/*InitDisk中用于对节点中的块数排序*/
bool cmp(pair<int, int> p1, pair<int, int> p2){
    return p1.second < p2.second;
}

/**
 * @brief   随机生成节点中的数据。采用的方法为，首先通过shuffle随机选择g_N个节点存放
            第一个条带。之后按照各个节点中存储的块数进行排序，选出块数最少的g_N个节点放置下一个条带。因为
            g_N、条带长度、节点数之间满足整除关系，所以最终每个节点中的块数一定相同
 **/
void InitDisks() {
    vector<int> vec_temp;
    for (int i = 0; i < g_DiskNumOrigin; i++) {
        vec_temp.push_back(i);
    }
    for (int i = 0; i < g_DiskNumOrigin; i++) {
        vector<int> disk;
        disks.push_back(disk);
    }
    int cur_stripe_num = 0;
    int quit_shuffle = 0;
    while (true) {
        //获取随机数种子
        unsigned seed = chrono::system_clock::now().time_since_epoch().count();
        shuffle(vec_temp.begin(), vec_temp.end(), default_random_engine(seed));
        for (int i = 0; i < g_N; i++) {
            int select = vec_temp[i];
            disks[select].push_back(cur_stripe_num);
            block_location[cur_stripe_num].push_back(select);
            if (disks[select].size() >= g_N * g_StripeNum / g_DiskNumOrigin) {
                quit_shuffle = 1;
            }
        }
        cur_stripe_num++;
        if (quit_shuffle == 1) break;
    }
    /*完成了前期的shuffle，接下来按照每个节点中块数升序排序。思路为构建vector<pair<节点号, 块数> >，
    然后根据块数排序.这个方法时间复杂度较高，有待改进*/
    vector<pair<int, int> > pii;
    while (cur_stripe_num < g_StripeNum) {
        for (int i = 0; i < g_DiskNumOrigin; i++) {
            pii.push_back(make_pair(i, disks[i].size()));
        }
        sort(pii.begin(), pii.end(), cmp);
        for (int i = 0; i < g_N; i++) {
            disks[pii[i].first].push_back(cur_stripe_num);
            block_location[cur_stripe_num].push_back(pii[i].first);
        }
        cur_stripe_num++;
        pii.clear();
    }
    if (g_Debug == 1) {
        cout << "===== InitDisks =====" << endl;
        //测试
        cout << "各节点中块数：" << endl;
        for (int i = 0; i < g_DiskNumOrigin; i++) {
            cout << disks[i].size() << " ";
        }
        cout << endl << endl;
        cout << "各节点中的块号：" << endl;
        for (int i = 0; i < g_DiskNumOrigin; i++) {
            cout << "disk" << i << ": ";
            for (int j = 0; j < disks[i].size(); j++) {
                cout << disks[i][j] << " ";
            }
            cout << endl << endl;
        }
        //测试哈希表
        cout << "测试哈希表：" << endl;
        for (int i = 0; i < g_StripeNum; i++) {
            cout << "stripe " << i << ": ";
            for (int j = 0; j < block_location[i].size(); j++) {
                cout << block_location[i][j] << " ";
            }
            cout << endl;
        }

        cout << "===== end =====" << endl << endl;
    }
}

/**
 * @brief   根据随机生成的数据初始化图
 */
void InitGraph() {
    //根据block_location可以方便地知道哪些节点之间应该有边
    for (int i = 0; i < g_StripeNum; i++) {
        for (int j = 0; j < block_location[i].size() - 1; j++) {
            for (int k = j + 1; k < block_location[i].size(); k++) {
                G[block_location[i][j]][block_location[i][k]] ++;
                G[block_location[i][k]][block_location[i][j]] ++;
            }
        }
    }
    //测试
    if (g_Debug == 1) {
        cout << "===== InitGraph =====" << endl;
        cout << "测试邻接矩阵：" << endl;
        for (int i = 0; i < g_DiskNumOrigin; i++) {
            for (int j = 0; j < g_DiskNumOrigin; j++) {
                cout << G[i][j] << " ";
            }
            cout << endl;
        }
        cout << "===== end =====" << endl << endl;
    }
}

/**
 * @brief   从disk中选择一个将要被迁移到新节点的块
 * @param   disk    需要被迁移的块所在的节点
 * @param   bottleneck_disk 与disk恢复形成瓶颈的节点
 * @return  返回一个pair，pair的第一项为disk中要被迁移的块号，第二项为迁移目标节点
 */
pair<int, int> SelectTravelBlock(int disk, int bottleneck_disk){
    int has_found = 0;//标记是否找到了符合要求的块
    pair<int, int> plan_b;//当最优解没有找到时，plan_b记录的是当采取非最优方案时的迁移目标节点
    //遍历disk中的每一个块，判断是否满足迁移条件
    for (int i = 0; i < disks[disk].size(); i++) {
        vector<int> & vec_temp = block_location[disks[disk][i]];
        if (find(vec_temp.begin(), vec_temp.end(), bottleneck_disk) == vec_temp.end()) {
            //当前块没有与bottleneck_disk关联
            continue;
        } else {
            //当前块与bottleneck_disk有关联
            //检查新节点中是否有某个节点没有与当前块在同一条带的块
            for (int j = g_DiskNumOrigin; j < g_DiskNumAfterScale; j++) {
                if (find(vec_temp.begin(), vec_temp.end(), j) != vec_temp.end()) {
                    //当前新节点中已经存放了同一条带的块，这个新节点无法作为目标节点
                    continue;
                } else {
                    //当前新节点中没有与i在同一条带的块
                    //检查当前新节点上是否还有位置
                    if (disks[j].size() >= g_N * g_StripeNum / g_DiskNumAfterScale) {
                        //当前新节点没有位置了
                        continue;
                    } else {
                        //当前新节点上还有位置
                        plan_b = make_pair(disks[disk][i], j);//当最优解无法找到，就放弃最后一个约束条件，采取次优解
                        //检查假设把块迁移到这个新节点后，传输时间是否超过理论最优解
                        int ok_flag = 1;
                        for (int m = 0; m < vec_temp.size(); m++) {
                            if (vec_temp[m] == disk) continue;
                            if (G[j][vec_temp[m]] + 1 > g_Optimal) {
                                ok_flag = 0;
                                break;
                            }
                        }
                        if (ok_flag == 1) {
                            has_found = 1;
                            return make_pair(disks[disk][i], j);
                        }
                    }
                }
            }
        }
    }
    if (has_found == 0) {
        cout << "采用次优解：";
        return plan_b;
    }
}

/**
 * @brief   扩容函数
 */
void SUDExpand() {
    //计算每个节点需要迁移几个块
    int travel_num = g_StripeNum * g_N / g_DiskNumOrigin - g_StripeNum * g_N / g_DiskNumAfterScale;
    assert(travel_num > 0);
    //进行travel_num轮迁移，每轮每个节点迁移一个块
    int bottleneck_disk = 0;
    int bottleneck;
    while (travel_num--) {
        for (int i = 0; i < g_DiskNumOrigin; i++) {
            bottleneck = 0;
            for (int j = 0; j < g_DiskNumAfterScale; j++) {
                if (G[i][j] > bottleneck) {
                    bottleneck = G[i][j];
                    bottleneck_disk = j;
                }
            }
            //在disks中增加新节点对应的vector
            for (int j = 0; j < g_DiskNumAfterScale - g_DiskNumOrigin; j++) {
                vector<int> new_disk;
                disks.push_back(new_disk);
            }
            pair<int, int> travel_pair = SelectTravelBlock(i, bottleneck_disk);
            cout << "将" << i << "节点的" << travel_pair.first << "块迁移至" << travel_pair.second << "节点" << endl;
            //对边进行增删调整
            vector<int> & vec_temp = block_location[travel_pair.first];
            int travel_target_disk = travel_pair.second;
            int travel_block_no = travel_pair.first;
            for (int j = 0; j < vec_temp.size(); j++) {
                if (vec_temp[j] == i) continue;
                G[i][vec_temp[j]]--;
                G[vec_temp[j]][i]--;
                G[vec_temp[j]][travel_target_disk]++;
                G[travel_target_disk][vec_temp[j]]++;
            }
            //更新disks
            vector<int>::iterator it = find(disks[i].begin(), disks[i].end(), travel_block_no);
            disks[i].erase(it);
            disks[travel_target_disk].push_back(travel_block_no);
            //更新block_location
            it = find(block_location[travel_block_no].begin(), block_location[travel_block_no].end(), i);
            block_location[travel_block_no].erase(it);
            block_location[travel_block_no].push_back(travel_target_disk);

        }
    }
    //测试
    if (g_Debug == 1) {
        cout << "===== SUDExpand =====" << endl;
        //检查是否达到理想最优解
        cout << "理想最优解为" << g_Optimal << endl;
        cout << "扩展后的邻接矩阵：" << endl;
        int is_optimal = 1;
        for (int i = 0; i < g_DiskNumAfterScale; i++) {
            for (int j = 0; j < g_DiskNumAfterScale; j++) {
                cout << G[i][j] << " ";
                if (G[i][j] > g_Optimal) {
                    is_optimal = 0;
                }
            }
            cout << endl;
        }
        if (is_optimal == 1) {
            cout << "得到理想最优解" << endl;
        } else {
            cout << "未能得到理想最优解" << endl;
        }
        cout << "===== end =====" << endl << endl;
    }
}

/**
 * @brief   缩容函数
 */
void SUDShrink(){
    //TODO
}

/**
 * @brief   评估函数
 */
void Evaluation(){
    //TODO
}


int main() {
    int total_block_num = g_N * g_StripeNum;
    if ((total_block_num % g_DiskNumOrigin != 0) || (total_block_num % g_DiskNumAfterScale != 0)) {
        cout << "Error: 无法保证各节点中块数相同" << endl;
        return 0;
    }
    InitDisks();
    InitGraph();
    if (g_DiskNumOrigin < g_DiskNumAfterScale) {
        SUDExpand();
    } else if (g_DiskNumOrigin > g_DiskNumAfterScale) {
        SUDShrink();
    }
    return 0;
}