#include "cache.h"
using namespace std;

// line class constructor
line::line(int waysNum, int set){
        _set = set;
        _waysNum = waysNum;
        for (int i = 0; i < _waysNum; i++){
                waysVec.push_back(make_pair(make_pair(0, false), false));
                lru.push_back(i);
        }
}


// ******line methods*******

// update line lru according to algorithm from lecture
void line::lruUpdate(int way){
        int temp = lru[way];
        lru[way] = _waysNum - 1;
        for (int i = 0; i < _waysNum; i++){
                if((i != way) && (lru[i] > temp)){
                        lru[i]--;
                }
        }
}

// get the way number to evict the adress from, according to the lru.
int line::WayToEvict(){
        for(int way = 0; way < _waysNum; way++){
                if(lru[way] == 0){
                        return way;
                }
        }
        return 0;
}
// ******end line methods*******

// cache class constructor.
cache::cache(int waysNum, int setsNum){
        _setsNum = setsNum;
        _waysNum = 1 << waysNum;
        for(int i = 0; i < setsNum; i++){
                lines[i] = new line(_waysNum, i);
        }
}

// cache class destructor.
cache::~cache(){
        for(int i = 0; i < _setsNum; i++){
                delete lines[i];
        }
}

// ******cache methods*******

// if adress is found in this line, return true (HIT) and update lru.
// else return false (MISS).
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

// check if cache line is full, or need evict.
bool cache::isLineFull(int set){
        line* setLine = lines[set];
        for(int way = 0; way < _waysNum; way++){
                if(setLine->waysVec[way].second == false){
                        return false;
                }
        }
        return true;
}

// choose the way to evict from, according to lru.
// return true if dirty and need to write back.
bool cache::evict(int set, int &writeBackTagNum){
        line* setLine = lines[set];
        int wayToEvict = setLine->WayToEvict();
        bool dirty = setLine->waysVec[wayToEvict].first.second;
        writeBackTagNum = setLine->waysVec[wayToEvict].first.first;
        setLine->waysVec[wayToEvict].second = false;
        return dirty;
}

// insert a new line to the right cache line and update the lru.
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
}

// method used only by L1, to write back to L2, and update L2 line lru.
void cache::writeBack(int set, int tag){
        line* setLine = lines[set];
        int cnt = 0;
        for(int way = 0; way < _waysNum; way++){
                if((setLine->waysVec[way].first.first == tag) && (setLine->waysVec[way].second)){
                        setLine->waysVec[way].first.second = true;
                        setLine->lruUpdate(way);
                        cnt++;
                }
        }
}

// write to cache line. update dirty bit if HIT (return true) or return false (MISS).
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

// method used by L1, to snoop adress given by L2 in case of eviction.
// in case found in L1 - valid bit is set to 0 in L1 line.
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
// ******end cache methods*******


// calculate the number of sets in each cache.
int calcSize(int cacheSize, int blockSize, int waysNum){
        int setsNum = 1 << (cacheSize - blockSize - waysNum);
        return setsNum;
}

// calculate the number of bits that represent num.
int BitCalc(int num){
        int cnt = 0;
        while(num != 1){
                num = num >> 1;
                cnt++;
        }
        return cnt;
}
