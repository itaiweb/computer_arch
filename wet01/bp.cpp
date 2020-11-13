/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <vector>
#include <map>
#include <math.h>

#define TAKEN 1
#define VALID_BIT_SIZE 1
#define TARGET_PC_SIZE 30

using namespace std;
//////////////// classes ///////////////////

//class btbLine
class btbLine
{
public:
	btbLine(){};
	btbLine(unsigned _tag, unsigned _target);
	btbLine(const btbLine& rhs);
	~btbLine();
	unsigned tag;
	unsigned target;
	unsigned history;
};

btbLine::btbLine(unsigned _tag, unsigned _target)
{
	tag = _tag;
	target = _target;
	history = 0;
}

btbLine::btbLine(const btbLine& rhs): tag(rhs.tag), target(rhs.target), history(rhs.history){
}

btbLine::~btbLine()
{
}

// btb class:
class btb
{	
public:
	btb(unsigned _btbSize, unsigned _historySize, unsigned _tagSize, unsigned _fsmState,
			bool _isGlobalHist, bool _isGlobalTable, int _Shared);
	~btb();
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

	//for GHR:
	unsigned globalHistory;

	//for statistics.
	unsigned flush_num;
	unsigned br_num;
	unsigned size;

	//containers:
	map<unsigned, btbLine> btbLines; // first = btb entry, second = line;
	map<unsigned, vector<unsigned>> fsm;
	//methods:
	void updateFsm(int row, int col, bool taken);

};

btb::btb(unsigned _btbSize, unsigned _historySize, unsigned _tagSize, unsigned _fsmState,
			bool _isGlobalHist, bool _isGlobalTable, int _Shared)
{
	btbSize = _btbSize;
	historySize = _historySize;
	tagSize = _tagSize;
	fsmState = _fsmState;
	isGlobalHist = _isGlobalHist;
	isGlobalTable = _isGlobalTable;
	Shared = _Shared;
	flush_num = 0;
	br_num = 0;
	size = 0;
	globalHistory = 0;
}

btb::~btb()
{
}

void btb::updateFsm(int tag, int idx, bool taken){
	if(taken){
		if(fsm[tag][idx] < 3){
			fsm[tag][idx]++;
		}
	} else {
		if(fsm[tag][idx] > 0){
			fsm[tag][idx]--;
		}
	}
}

/////////////////////// end classes //////////////////////////////

//////////////////////  helper functions ////////////////////////

unsigned extractTag(uint32_t pc, int tagSize, int btbSize);
bool predict(uint32_t pc, unsigned history, unsigned tag);
bool predictGlobalT(uint32_t pc, unsigned tag, unsigned history);
bool getPredictionFromFsmTable(int tag, int idx);
void updateHistory(unsigned* history, bool taken);
void createBtbEntry(uint32_t targetPc, unsigned tag, uint32_t pc);
void updateTable(uint32_t pc, bool taken, uint32_t targetPc);
void updateGlobalT(uint32_t pc, bool taken, unsigned history);
unsigned findBtbEntry(uint32_t pc);
int calcBtbSize();

////////////////////// end  helper functions ////////////////////////

btb* myBtb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	myBtb = new btb(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	if(isGlobalTable){
		vector <unsigned> Globalfsm;
		myBtb->fsm[0] = Globalfsm;
		for(int i = 0; i < (1 << myBtb->historySize); i++){
			myBtb->fsm[0].push_back(fsmState);
		}
	}

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	unsigned pcTag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);

	for(auto itr = myBtb->btbLines.begin(); itr != myBtb->btbLines.end(); itr++){
		if(!(itr->second.tag ^ pcTag)){
			bool prediction = predict(pc, itr->second.history, pcTag);
			if(prediction == TAKEN){
				*dst = itr->second.target;
			} else {
				*dst = pc + 4;
			}
			return prediction;
		}
	}

	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	// for statistics
	myBtb->br_num++;
	if(((pred_dst == targetPc) && !taken) || ((pred_dst != targetPc) && taken)){
		myBtb->flush_num++;
	}
	
	unsigned pcTag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);
	if(myBtb->fsm.find(pcTag) == myBtb->fsm.end()){
		createBtbEntry(targetPc, pcTag, pc);
	}
	updateTable(pc, taken, targetPc);
	if(myBtb->isGlobalHist){
		updateHistory(&myBtb->globalHistory, taken);
	} else {
		for(auto itr = myBtb->btbLines.begin(); itr != myBtb->btbLines.end(); itr++){
			if(itr->second.tag == pcTag){
				updateHistory(&itr->second.history, taken);
			}
		}
	}

	return;
}

void BP_GetStats(SIM_stats *curStats){
	curStats->br_num = myBtb->br_num;
	curStats->flush_num = myBtb->flush_num;
	curStats->size = calcBtbSize();
	return;
}

//////////////////////  helper functions ////////////////////////

unsigned extractTag(uint32_t pc, int tagSize, int btbSize){
	uint32_t pcTag = pc >> 2;
	int shiftCnt = 0;
	while(btbSize != 1){
		btbSize = btbSize >> 1;
		shiftCnt++;
	}
	pcTag = pcTag >> shiftCnt;
	pcTag = pcTag & ((1 << myBtb->tagSize) - 1);
	return pcTag;
}

bool predict(uint32_t pc, unsigned history, unsigned tag){
	if(myBtb->isGlobalTable){
		if(myBtb->isGlobalHist){
			return predictGlobalT(pc, 0, myBtb->globalHistory);	
		} else { // local history
			return predictGlobalT(pc ,tag, history);
		}
	} else { // local table
		if(myBtb->isGlobalHist){
			return getPredictionFromFsmTable(tag, myBtb->globalHistory);
		} else { // local history
			return getPredictionFromFsmTable(tag, history);
		}
	}
	return false;
}

bool predictGlobalT(uint32_t pc, unsigned tag, unsigned history){
	bool prediction = false;
	unsigned cleanPc = (pc >> 2);
	if(myBtb->Shared == 0){ // not using share
		prediction = getPredictionFromFsmTable(tag, history);
	} else if(myBtb->Shared == 1) { // using share lsb
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		prediction = getPredictionFromFsmTable(tag, idx);
	} else { //using share mid
		cleanPc = (cleanPc >> 14);
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		prediction = getPredictionFromFsmTable(tag, idx);
	}
	return prediction;
}

bool getPredictionFromFsmTable(int tag, int idx){
	int state = myBtb->fsm[tag][idx];
	if(state ==0 || state == 1){
		return false;
	}
	return true;
}

unsigned getLocalHistory(unsigned pcTag){
	unsigned history;
	for(auto itr = myBtb->btbLines.begin(); itr != myBtb->btbLines.end(); itr++){
		if(!(itr->second.tag ^ pcTag)){
			history = itr->second.history;
		}
	}
	return history;
}

void updateHistory(unsigned* history, bool taken){
	*history = ((*history) << 1);
	*history += taken;
	*history = ((*history) & ((1 << myBtb->historySize) - 1));
}

void updateTable(uint32_t pc, bool taken, uint32_t targetPc){
	unsigned tag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);
	if(myBtb->isGlobalTable){
		if(myBtb->isGlobalHist){
			updateGlobalT(pc, taken, myBtb->globalHistory);
		} else {
			unsigned localHistory = getLocalHistory(tag);
			updateGlobalT(pc, taken, localHistory);
		}
	} else { // local table
		if(myBtb->isGlobalHist){
			myBtb->updateFsm(tag, myBtb->globalHistory, taken);
		} else { // local history
			myBtb->updateFsm(tag, getLocalHistory(tag), taken);
		}
	}
}

void updateGlobalT(uint32_t pc, bool taken, unsigned history){
	unsigned cleanPc = (pc >> 2);
	if(myBtb->Shared == 0){ // not using share
		myBtb->updateFsm(0, history, taken);
	} else if(myBtb->Shared == 1) { // using share lsb
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		myBtb->updateFsm(0, idx, taken);
	} else { //using share mid
		cleanPc = (cleanPc >> 14);
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		myBtb->updateFsm(0, idx, taken);
	}
}

void createBtbEntry(uint32_t targetPc, unsigned tag, uint32_t pc){
	unsigned btbEntry = findBtbEntry(pc);

	////////////// print btb
	// cout << endl;
	// for(auto i = myBtb->btbLines.begin(); i != myBtb->btbLines.end(); i++){
	// 	cout << "entry: " << i->first;
	// 	cout << " tag: " << i->second.tag;
	// 	cout << " history: " << i->second.history;
	// 	cout << " target: " << i->second.target << endl;
	// }

	if(myBtb->btbLines.find(btbEntry) != myBtb->btbLines.end()){ // line exists and need to change branch.
		if(!myBtb->isGlobalTable){
			// cout << myBtb->fsm.find(tag)->first << endl;
			// cout << myBtb->fsm.end()->first << endl;
			myBtb->fsm.erase(myBtb->fsm.find(myBtb->btbLines[btbEntry].tag));
		}
		myBtb->btbLines[btbEntry].tag = tag;
		myBtb->btbLines[btbEntry].target = targetPc;
		myBtb->btbLines[btbEntry].history = 0;
	} else {
		myBtb->btbLines[btbEntry] = btbLine(tag, targetPc);
	}
	if(!myBtb->isGlobalTable){
		vector<unsigned> initFsm;
		myBtb->fsm[tag] = initFsm;
		for(int i = 0; i < (1 << myBtb->historySize); i++){
			myBtb->fsm[tag].push_back(myBtb->fsmState);
		}
	}
	
}

unsigned findBtbEntry(uint32_t pc){
	unsigned btbEntry = (pc >> 2);
	btbEntry = (btbEntry & ((1 << (myBtb->btbSize - 1)) - 1));
	return btbEntry;
}

int calcBtbSize(){
	int size = 0;
	if(myBtb->isGlobalTable){
		if(myBtb->isGlobalHist){
			size = myBtb->btbSize * (myBtb->tagSize + VALID_BIT_SIZE + TARGET_PC_SIZE) + (2 << myBtb->historySize) +  myBtb->historySize;
		} else { //local history
			size = myBtb->btbSize * (myBtb->tagSize + VALID_BIT_SIZE + TARGET_PC_SIZE + myBtb->historySize) + (2 << myBtb->historySize);
		}
	} else { // local table
		if(myBtb->isGlobalHist){
			size = myBtb->btbSize * (myBtb->tagSize + VALID_BIT_SIZE + TARGET_PC_SIZE + (2 << myBtb->historySize)) + myBtb->historySize;
		} else { // local history
			size = myBtb->btbSize * (myBtb->tagSize + VALID_BIT_SIZE + TARGET_PC_SIZE + myBtb->historySize + (2 << myBtb->historySize));
		}
	}
	return size;
}

////////////////////// end  helper functions ////////////////////////

