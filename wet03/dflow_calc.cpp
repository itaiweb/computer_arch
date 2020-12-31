/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>

using namespace std;

// class representing a single node in the dependency tree.
class node
{
public:
    node(int, int);
    ~node(){};
    int index; // who am I
    int src1; //dependent 1
    int src2; //dependent 2
    int weight; //execution time
    int weightSum; //execution time from entry
    bool isExit; //can the program end after me?

};

node::node(int _index, int _weight) {
    index = _index;
    src1 = -1;
    src2 = -1;
    weight = _weight;
    weightSum = 0;
    isExit = true;
}

void updateNode (node&, node&, int);

// create dependency tree, and save all relevant data.
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    vector<node*>* myGraph = new vector<node*>;
    node* entry = new node(0, 0);
    entry->isExit = false;
    myGraph->push_back(entry);
    for (unsigned int i = 0; i < numOfInsts; i++) {
        node* newNode = new node(i+1, opsLatency[progTrace[i].opcode]);
        myGraph->push_back(newNode);
    }
    
    for (unsigned int i = 0; i < numOfInsts; i++) {
        unsigned int writeToReg = progTrace[i].dstIdx;
        for (unsigned int j = i + 1; j < numOfInsts; j++) {
            if(progTrace[j].src1Idx == writeToReg){ // check dependency
                updateNode(*(*myGraph)[j+1], *(*myGraph)[i+1], 1);
            }
            if(progTrace[j].src2Idx == writeToReg){    
                updateNode(*(*myGraph)[j+1], *(*myGraph)[i+1], 2);
            }
            // stop the dependency check for certain register if it was written to again.
            if(progTrace[j].dstIdx == (int)writeToReg) { 
                break;
            }
        } 
    }

    return (void*)myGraph;
}

void freeProgCtx(ProgCtx ctx) {
    vector<node*>* myGraph = (vector<node*>*)ctx;
    for(unsigned int i = 0; i < myGraph->size(); i++){
        delete (*myGraph)[i];
    }
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    vector<node*>* myGraph = (vector<node*>*)ctx;
    if((theInst > (myGraph->size() -1)) || theInst < 0){
        return -1;
    }
    return (*myGraph)[theInst+1]->weightSum;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    vector<node*>* myGraph = (vector<node*>*)ctx;
    if((theInst > (myGraph->size() -1)) || theInst < 0){
        return -1;
    }
    // +1 offset because entry is in 0.
    if((*myGraph)[theInst+1]->src1 == -1){
        *src1DepInst = -1;
    } else {
        *src1DepInst = (*myGraph)[theInst+1]->src1-1;
    }
    if((*myGraph)[theInst+1]->src2 == -1){
        *src2DepInst = -1;
    } else {
        *src2DepInst = (*myGraph)[theInst+1]->src2-1;
    }

    return 0;
}

int getProgDepth(ProgCtx ctx) {
    vector<node*>* myGraph = (vector<node*>*)ctx;
    int maxDepth = 0;
    for(unsigned int i = 0; i < myGraph->size(); i++){
        if(((*myGraph)[i]->weightSum + (*myGraph)[i]->weight > maxDepth) && ((*myGraph)[i]->isExit == true)){
            maxDepth = (*myGraph)[i]->weightSum + (*myGraph)[i]->weight;
        }
    }
    return maxDepth;
}

void updateNode (node& nodeToUpdate, node& rootNode, int src) {
    rootNode.isExit = false; //rootNode is not exit because there is someone dependent on him.
    if(src == 1){
        nodeToUpdate.src1 = rootNode.index;
    } else {
        nodeToUpdate.src2 = rootNode.index;
    }
    int currentWeightSum = rootNode.weightSum + rootNode.weight; 
    if(currentWeightSum > nodeToUpdate.weightSum){ //check if weightSum needs update from second source.
        nodeToUpdate.weightSum = currentWeightSum;
    }
}
