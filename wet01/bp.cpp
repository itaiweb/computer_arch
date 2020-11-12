/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <vector>
#include <math.h>

using namespace std;
//////////////// classes ///////////////////

//class btbLine
class btbLine
{
public:
	btbLine(unsigned _tag, unsigned _target, unsigned _history);
	~btbLine();
	unsigned tag;
	unsigned target;
	unsigned history;
};

btbLine::btbLine(unsigned _tag, unsigned _target, unsigned _history)
{
	tag = _tag;
	target = _target;
	history = _history;
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
	vector<btbLine> btbLines;
	vector<vector<int>> fsm;
	
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
}

btb::~btb()
{
}

void btb::updateFsm(int row, int col, bool taken){
	if(taken){
		if(fsm[row][col] < 3){
			fsm[row][col]++;
		}
	} else {
		if(fsm[row][col] > 0){
			fsm[row][col]--;
		}
	}
}

/////////////////////// end classes //////////////////////////////

//////////////////////  helper functions ////////////////////////

unsigned extractTag(uint32_t pc, int tagSize, int btbSize);

////////////////////// end  helper functions ////////////////////////

btb* myBtb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	myBtb = new btb(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	uint32_t pcTag = extractTag(pc, myBtb->tagSize, myBtb->btbSize);

	cout << "tag size is: " << myBtb->tagSize << endl;
	cout << "btb size is: " << myBtb->btbSize << endl;
	printf("0x%X\n", pc);
	printf("0x%X\n", pcTag);

	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
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

////////////////////// end  helper functions ////////////////////////

