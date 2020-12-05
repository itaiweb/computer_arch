#include <map>
#include <vector>
#include <iostream>
#include <queue>
using namespace std; 

int calcSize(int cacheSize, int blockSize, int waysNum);
int BitCalc(int num);

class line{
        public:
        line(int waysNum, int set);
        line() {};
        int _set;
        int _waysNum;
        vector <pair<pair<int, bool>, bool>> way; // vector[way] has a pair consist of first->tag&dirty, second->valid_bit
        vector <int> lru; //vector[way] - smallest is evicted.
        //void lruUpdate(int);

};


class cache{
        public:
        cache(int waysNum, int setsNum);
        ~cache();
        int _waysNum;
        int _setsNum;
        map <int, line*> lines;// line is a map[setNum] - gives a line class which has all data of a certain set inside a cache.
        bool read(int set, int tag); //return true if hit
        bool isLineFull(int set);
        bool evict(int set); //return dirty
        void insert(int set, int tag); //just put the tag in place
        void writeBack(int set, int tag);

};