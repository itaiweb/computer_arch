/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <vector>
#include <map>
#include <math.h>

#include <bitset> //REMOVE


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
	void updateFsm(int entry, int idx, bool taken);

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

void btb::updateFsm(int entry, int idx, bool taken){
	if(taken){
		if(fsm[entry][idx] < 3){
			fsm[entry][idx]++;
		}
	} else {
		if(fsm[entry][idx] > 0){
			fsm[entry][idx]--;
		}
	}
}

/////////////////////// end classes //////////////////////////////

//////////////////////  helper functions ////////////////////////

unsigned extractTag(uint32_t pc, int tagSize, int btbSize);
bool isBtbEntryExist(uint32_t pc);
bool predict(uint32_t pc, unsigned history, unsigned tag);
bool predictGlobalT(uint32_t pc, unsigned history);
bool getPredictionFromFsmTable(int tag, int idx);
void updateHistory(unsigned* history, bool taken);
void createBtbEntry(uint32_t targetPc, unsigned tag, uint32_t pc);
void resetBtbEntry(uint32_t targetPc, unsigned tag, uint32_t pc);
void updateTable(uint32_t pc, bool taken, uint32_t targetPc);
void updateGlobalT(uint32_t pc, bool taken, unsigned history);
unsigned findBtbEntry(uint32_t pc);
int calcBtbSize();
void printBTB(uint32_t pc); //REMOVE

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
	unsigned btbEntry = findBtbEntry(pc);
	bitset<32> pcBIN(pc); //REMOVE
	cout << "pc in binary:    " << pcBIN << endl; //REMOVE
	printBTB(pc); // REMOVE
	if(isBtbEntryExist(pc)){
		bool prediction = predict(pc, myBtb->btbLines[btbEntry].history, btbEntry);
		if(prediction == TAKEN){
			*dst = myBtb->btbLines[btbEntry].target;
		} else {
			*dst = pc + 4;
		}
		return prediction;
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
	unsigned btbEntry = findBtbEntry(pc);
	if(myBtb->btbLines.find(btbEntry) != myBtb->btbLines.end()) {
		if(myBtb->btbLines[btbEntry].tag != pcTag){
			resetBtbEntry(targetPc, pcTag, pc); //found entry but with different tag.
		}
	} else {
		createBtbEntry(targetPc, pcTag, pc);
	}
	updateTable(pc, taken, targetPc);
	if(myBtb->isGlobalHist){
		updateHistory(&myBtb->globalHistory, taken);
	} else {
		updateHistory(&myBtb->btbLines[btbEntry].history, taken);
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

bool isBtbEntryExist(uint32_t pc){
	unsigned tag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);
	bool exist = false;
	unsigned btbEntry = findBtbEntry(pc);
	if(myBtb->btbLines.find(btbEntry) != myBtb->btbLines.end()) {
		if(myBtb->btbLines[btbEntry].tag == tag){
			exist = true;
		}
	}
	return exist;
}

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

bool predict(uint32_t pc, unsigned history, unsigned btbEntry){
	if(myBtb->isGlobalTable){
		if(myBtb->isGlobalHist){
			return predictGlobalT(pc, myBtb->globalHistory);	
		} else { // local history
			return predictGlobalT(pc, history);
		}
	} else { // local table
		if(myBtb->isGlobalHist){
			return getPredictionFromFsmTable(btbEntry, myBtb->globalHistory);
		} else { // local history
			return getPredictionFromFsmTable(btbEntry, history);
		}
	}
	return false;
}

bool predictGlobalT(uint32_t pc, unsigned history){
	bool prediction = false;
	unsigned cleanPc = (pc >> 2);
	if(myBtb->Shared == 0){ // not using share
		prediction = getPredictionFromFsmTable(0, history);
	} else if(myBtb->Shared == 1) { // using share lsb
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		prediction = getPredictionFromFsmTable(0, idx);
	} else { //using share mid
		cleanPc = (cleanPc >> 14);
		cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
		unsigned idx = (cleanPc ^ history);
		prediction = getPredictionFromFsmTable(0, idx);
	}
	return prediction;
}

bool getPredictionFromFsmTable(int entry, int idx){
	int state = myBtb->fsm[entry][idx];
	if(state ==0 || state == 1){
		return false;
	}
	return true;
}

void updateHistory(unsigned* history, bool taken){
	*history = ((*history) << 1);
	*history += taken;
	*history = ((*history) & ((1 << myBtb->historySize) - 1));
}

void updateTable(uint32_t pc, bool taken, uint32_t targetPc){
	unsigned btbEntry = findBtbEntry(pc);
	if(myBtb->isGlobalTable){
		if(myBtb->isGlobalHist){
			updateGlobalT(pc, taken, myBtb->globalHistory);
		} else {
			updateGlobalT(pc, taken, myBtb->btbLines[btbEntry].history);
		}
	} else { // local table
		if(myBtb->isGlobalHist){
			myBtb->updateFsm(btbEntry, myBtb->globalHistory, taken);
		} else { // local history
			myBtb->updateFsm(btbEntry, myBtb->btbLines[btbEntry].history, taken);
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
	myBtb->btbLines[btbEntry] = btbLine(tag, targetPc);
	if(!myBtb->isGlobalTable){
		vector<unsigned> initFsm;
		myBtb->fsm[btbEntry] = initFsm;
		for(int i = 0; i < (1 << myBtb->historySize); i++){
			myBtb->fsm[btbEntry].push_back(myBtb->fsmState);
		}
	}
	
}

void resetBtbEntry(uint32_t targetPc, unsigned tag, uint32_t pc){
	unsigned btbEntry = findBtbEntry(pc);
	if(!myBtb->isGlobalTable){
		for(auto fsmItr = myBtb->fsm[btbEntry].begin(); fsmItr != myBtb->fsm[btbEntry].end(); fsmItr++){
			*fsmItr = myBtb->fsmState;
		}
	}
	myBtb->btbLines[btbEntry].tag = tag;
	myBtb->btbLines[btbEntry].target = targetPc;
	myBtb->btbLines[btbEntry].history = 0;
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
void printBTB(uint32_t pc){ //REMOVE
	cout << "BTB Table:" << endl;
	for(auto i = myBtb->btbLines.begin(); i != myBtb->btbLines.end(); i++){
		bitset<26> tag(i->second.tag);
		cout << "-------------------------------------------------------------------------------" << endl;
		cout << "entry:  " << i->first;
		cout << "  | tag:  ";
		cout << tag;
		bitset<8> hist(i->second.history);
		cout << "  | history:  " << hist;
		cout << "  | target:  ";
		printf("0x%X |", i->second.target);
		if(!myBtb->isGlobalTable){
			if(!myBtb->isGlobalHist){
				cout << " FSM[" << i->second.history << "] state:  " << myBtb->fsm[i->first][i->second.history] << endl;
			}
			else{
				cout << " FSM state:  " << myBtb->fsm[i->first][myBtb->globalHistory] << endl;	
			}
		} else {
			cout << endl;
		}
	}
	cout << "-------------------------------------------------------------------------------" << endl;
	if(myBtb->isGlobalHist){
		bitset<8> x(myBtb->globalHistory);
		cout << "Global History:" << x << endl;
		cout << "-------------------------------------------------------------------------------" << endl;
	}

	unsigned cleanPc = (pc >> 2);
	if(myBtb->isGlobalTable){
		cout << "**Global table: " << endl;
		cout << "\tindex\tstate" << endl;
		for(int i = 0; i < (1 << myBtb->historySize); i++){
			cout << "\t  " << i << "   |   " << myBtb->fsm[0][i] << endl;
		}
		if(myBtb->isGlobalHist){
			cout << " #################### FSM #####################" << endl;
			cout << "global tables and global history:" << endl;
			if(myBtb->Shared == 0){ // not using share
				cout << "not using share:" << endl;
				cout << myBtb->fsm[0][myBtb->globalHistory];
			} else if(myBtb->Shared == 1) { // using share lsb
				cout << "using share lsb:  ";
				cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
				unsigned idx = (cleanPc ^ myBtb->globalHistory);
				bitset<8> idxBIN(idx);
				cout << "the fsm xor result is: " << idxBIN << " and the fsm state: " << myBtb->fsm[0][idx] << endl;
			} else { //using share mid
				cout << "not using mid:" << endl;
				cleanPc = (cleanPc >> 14);
				cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
				unsigned idx = (cleanPc ^ myBtb->globalHistory);
				bitset<8> idxBIN(idx);
				cout << "the fsm xor result is: " << idxBIN << " and the fsm state: " << myBtb->fsm[0][idx] << endl;
			}	
		} else { //local history
			unsigned tag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);
			bool exist = false;
			unsigned btbEntry = findBtbEntry(pc);
			if(myBtb->btbLines.find(btbEntry) != myBtb->btbLines.end()) {
				if(myBtb->btbLines[btbEntry].tag == tag){
					exist = true;
				}
			}
			if(exist){
				unsigned history = myBtb->btbLines[btbEntry].history;
				cout << "global tables and local history" << endl;
				if(myBtb->Shared == 0){ // not using share
					cout << "not using share:" << endl;
					cout << myBtb->fsm[0][history] << endl;
				} else if(myBtb->Shared == 1) { // using share lsb
					cout << "using share lsb:  ";
					cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
					unsigned idx = (cleanPc ^ history);
					bitset<8> idxBIN(idx);
					cout << "the fsm xor result is: " << idxBIN << " and the fsm state: " << myBtb->fsm[0][idx] << endl;
				} else { //using share mid
					cout << "not using mid:" << endl;
					cleanPc = (cleanPc >> 14);
					cleanPc = cleanPc & ((1 << myBtb->historySize) - 1);
					unsigned idx = (cleanPc ^ history);
					bitset<8> idxBIN(idx);
					cout << "the fsm xor result is: " << idxBIN << " and the fsm state: " << myBtb->fsm[0][idx] << endl;
				}
			}
		}
	}
	cout << "-----------------------------------------------------------------------" << endl;
}

