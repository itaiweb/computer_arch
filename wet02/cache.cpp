#include "cache.h"
using namespace std;

line::line(int waysNum, int set){
        _set = set;
        _waysNum = waysNum;
        for (int i = 0; i < waysNum; i++){
                way.push_back(make_pair(make_pair(0, false), false));
                lru.push_back(i);
        }
}

// ******line methods*******

// ******line methods*******

cache::cache(int waysNum, int setsNum){
        _setsNum = setsNum;
        _waysNum = waysNum;
        for(int i = 0; i < setsNum; i++){
                lines[i] = new line(waysNum, i);
        }
}
cache::~cache(){
        for(int i = 0; i < _setsNum; i++){
                delete lines[i];
        }
}

// ******cache methods*******
bool cache::read(int set, int tag){
        return false;
}
bool cache::isLineFull(int set){
        return false;
}
bool cache::evict(int set){
        return false;
}
void cache::insert(int set, int tag){

}
void cache::writeBack(int set, int tag){
        
}
// ******cache methods*******



int calcSize(int cacheSize, int blockSize, int waysNum){
        int setsNum = 1 << (cacheSize - blockSize - waysNum);
        return setsNum;
}

int BitCalc(int num){
        int cnt = 0;
        while(num != 1){
                num = num >> 1;
                cnt++;
        }
        return cnt;
}
