#include "cache.h"
using namespace std;

line::line(int waysNum, int set){
        _set = set;
        _waysNum = waysNum;
        for (int i = 0; i < _waysNum; i++){
                waysVec.push_back(make_pair(make_pair(0, false), false));
                lru.push_back(i);
        }
}


// ******line methods*******
void line::lruUpdate(int way){
        int temp = lru[way];
        lru[way] = _waysNum - 1;
        for (int i = 0; i < _waysNum; i++){
                if((i != way) && (lru[i] > temp)){
                        lru[i]--;
                }
        }
}
int line::WayToEvict(){
        for(int way = 0; way < _waysNum; way++){
                if(lru[way] == 0){
                        return way;
                }
        }
        cout << "BUGGGGG in wayToEvict" << endl; //shouldnt be printed 
        return 1;
}
// ******line methods*******

cache::cache(int waysNum, int setsNum){
        _setsNum = setsNum;
        _waysNum = 1 << waysNum;
        for(int i = 0; i < setsNum; i++){
                lines[i] = new line(_waysNum, i);
        }
}
cache::~cache(){
        for(int i = 0; i < _setsNum; i++){
                delete lines[i];
        }
}

// ******cache methods*******
bool cache::read(int set, int tag){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                bool valid = setLine->waysVec[way].second;
                if(valid){
                        if(setLine->waysVec[way].first.first == tag){
                                setLine->lruUpdate(way);
                                return true;
                        }
                }
        }
        return false;
}

bool cache::isLineFull(int set){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                if(setLine->waysVec[way].second == false){
                        return false;
                }
        }
        return true;
}

bool cache::evict(int set, int &writeBackTagNum){
        line* setLine = lines[set];
        int wayToEvict = setLine->WayToEvict();
        bool dirty = setLine->waysVec[wayToEvict].first.second;
        if(dirty){
                writeBackTagNum = setLine->waysVec[wayToEvict].first.first;
        }
        setLine->waysVec[wayToEvict].second = false;
        return dirty;
}

void cache::insert(int set, int tag){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                if(setLine->waysVec[way].second == false){ // if valid is false
                        setLine->waysVec[way].first.first = tag;
                        setLine->waysVec[way].second = true; // set valid to true
                        setLine->waysVec[way].first.second = false; //set dirty to false
                        setLine->lruUpdate(way);
                        return;
                }
        }
        cout << "sanity check in insert method - NOT TO BE PRINTED" << endl;
}

void cache::writeBack(int set, int tag){
        line* setLine = lines[set];
        int cnt = 0;
        for(int way = 0; way < _waysNum; way++){
                if(setLine->waysVec[way].first.first == tag){
                        setLine->waysVec[way].first.second = true;
                        setLine->lruUpdate(way);
                        cnt++;
                }
        }
        if(cnt != 1){
                cout << "bug in writeback - " << cnt << endl;
        }
}

bool cache::write(int set, int tag){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                if( setLine->waysVec[way].second && (setLine->waysVec[way].first.first == tag)){
                        setLine->waysVec[way].first.second = true;
                        setLine->lruUpdate(way);
                        return true;
                }
        }
        return false;
}

bool cache::isExist(int set, int tag){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                if(setLine->waysVec[way].second){ // is valid
                        if (setLine->waysVec[way].first.first == tag){ // compare to tag
                                return true;
                        } 
                }
        }
        return false;
}

void cache::snoop(int set, int tag){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){    
                if(setLine->waysVec[way].second){ // is valid
                        if (setLine->waysVec[way].first.first == tag){ // compare to tag
                                setLine->waysVec[way].second = 0;
                        }
                }
        }
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
