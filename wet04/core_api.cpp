/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <queue>

#include <stdio.h>
using namespace std;

class thread {
	public:
	thread(int);
	int idx;
	int reg[REGS_COUNT];
	int pc;
	bool isHalt;
	int waiting;
};

class simulator{
	public:
	simulator();
	vector<thread> threads;
	queue<int> readyQ;
	int numOfCycles;
	int instNum;
	int loadLatency;
	int storeLatency;
	int switchOverhead;
	int threadsNum;
	int haltNum;
	void runInst(thread*);
	void contextSwitch(thread*);
	void nxtCycle();
};

/*
THREAD methods
*/
thread::thread(int _idx){
	for(int i = 0; i < REGS_COUNT; i++){
		reg[i] = 0;
	}
	idx = _idx;
	pc = 0;
	isHalt = false;
	waiting = 0;
}

/*
SIMULATOR methods
*/
simulator::simulator(){
	threadsNum = SIM_GetThreadsNum();
	haltNum = 0;
	for(int i = 0; i < threadsNum; i++){
		threads.push_back(thread(i));
		readyQ.push(i);
	}
	numOfCycles = 0;
	instNum = 0;
	loadLatency = SIM_GetLoadLat();
	storeLatency = SIM_GetStoreLat();
	switchOverhead = SIM_GetSwitchCycles();

}
void simulator::runInst(thread* thread){
	Instruction inst;
	SIM_MemInstRead(thread->pc, &inst, thread->idx);
	switch (inst.opcode)
	{
	case CMD_NOP:
		break;
	case CMD_ADD:     // dst <- src1 + src2
		thread->reg[inst.dst_index] = thread->reg[inst.src1_index] + thread->reg[inst.src2_index_imm];
		break;
    	case CMD_SUB:     // dst <- src1 - src2
	    	thread->reg[inst.dst_index] = thread->reg[inst.src1_index] - thread->reg[inst.src2_index_imm];
	    	break;
    	case CMD_ADDI:    // dst <- src1 + imm
	    	thread->reg[inst.dst_index] = thread->reg[inst.src1_index] + inst.src2_index_imm;
	    	break;
    	case CMD_SUBI:    // dst <- src1 - imm
	    	thread->reg[inst.dst_index] = thread->reg[inst.src1_index] - inst.src2_index_imm;
	    	break;
    	case CMD_LOAD:    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
	    	if(inst.isSrc2Imm){
			int32_t dst;
			SIM_MemDataRead(thread->reg[inst.src1_index] + inst.src2_index_imm, &dst);
			thread->reg[inst.dst_index] = dst;
		}else{
			int32_t dst;
			SIM_MemDataRead(thread->reg[inst.src1_index] + thread->reg[inst.src2_index_imm], &dst);
			thread->reg[inst.dst_index] = dst;	
		}
	    	break;
    	case CMD_STORE:   // Mem[dst + src2] <- src1  (src2 may be an immediate)
	    	if(inst.isSrc2Imm){
			int32_t dst = thread->reg[inst.dst_index];
			SIM_MemDataWrite(dst + inst.src2_index_imm, thread->reg[inst.src1_index]);
		}else{
			int32_t dst = thread->reg[inst.dst_index];
			SIM_MemDataWrite(dst + thread->reg[inst.src2_index_imm], thread->reg[inst.src1_index]);	
		}
	    	break;
	case CMD_HALT:
		thread->isHalt = true;
		haltNum++;
		break;
	default:
		break;
	}

}

void simulator::contextSwitch(thread* runningThread){

}

void simulator::nxtCycle(){

}

simulator* blockedMT = NULL;
simulator* finegrain = NULL;




void CORE_BlockedMT() {
	bool isFirst = true;
	thread* runningThread;
	blockedMT = new simulator();
	if(blockedMT->readyQ.empty() == false){
		runningThread = &(blockedMT->threads[blockedMT->readyQ.front()]);
		blockedMT->readyQ.pop();
	}

	while(blockedMT->threadsNum != blockedMT->haltNum){
		while(blockedMT->readyQ.empty() == false){
			if(isFirst == false){
				blockedMT->contextSwitch(runningThread); // switch only if current waiting != 0
				blockedMT->nxtCycle();
			}
			isFirst = false;
			while(!(runningThread->waiting)){
				blockedMT->runInst(runningThread);
				blockedMT->nxtCycle();
			}
		}
		blockedMT->nxtCycle();
	}
}

void CORE_FinegrainedMT() {
	thread* runningThread;
	blockedMT = new simulator();

	while(blockedMT->threadsNum != blockedMT->haltNum){
		while(blockedMT->readyQ.empty() == false){
			runningThread = &(blockedMT->threads[blockedMT->readyQ.front()]);
			blockedMT->readyQ.pop();
			blockedMT->runInst(runningThread);
			blockedMT->nxtCycle();
		}
		blockedMT->nxtCycle(); //adding to queue by minimum thread idx
	}
}

double CORE_BlockedMT_CPI(){

	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
