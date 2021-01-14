/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <queue>
#include <iostream>
#define FINE 0
#define BLOCKED 1

#include <stdio.h>
using namespace std;

class thread {
	public:
	thread(int);
	int idx;
	int reg[REGS_COUNT];
	int pc;
	bool isHalt;
	bool inQ;
	int waiting;
};

class simulator{
	public:
	simulator(int);
	vector<thread> threads;
	int type;
	int numOfCycles;
	int instNum;
	int loadLatency;
	int storeLatency;
	int switchOverhead;
	int threadsNum;
	int haltNum;
	void runInst(thread*);
	thread* contextSwitch(thread*);
	bool findAvailableThread();
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
simulator::simulator(int _type){
	type = _type;
	threadsNum = SIM_GetThreadsNum();
	haltNum = 0;
	for(int i = 0; i < threadsNum; i++){
		threads.push_back(thread(i));
	}
	numOfCycles = 0;
	instNum = 0;
	loadLatency = SIM_GetLoadLat();
	storeLatency = SIM_GetStoreLat();
	if(type == BLOCKED){
		switchOverhead = SIM_GetSwitchCycles();
	}else
	{
		switchOverhead = 0;
	}
}
void simulator::runInst(thread* thread){
	instNum++;
	Instruction inst;
	SIM_MemInstRead(thread->pc, &inst, thread->idx);
	thread->pc++;
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
	    	thread->waiting = loadLatency + 1;
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
	    	thread->waiting = storeLatency + 1;
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

thread* simulator::contextSwitch(thread* runningThread){
	if(runningThread->waiting == 0 && runningThread->isHalt == false && type == BLOCKED){
		return runningThread;
	}
	int nxtThread = (runningThread->idx + 1) % threadsNum;
	for(int i = 0; i < threadsNum; i++){
		if(threads[nxtThread].waiting == 0 && threads[nxtThread].isHalt == false){
			runningThread =  &(threads[nxtThread]); // choosing a thread to run next instruction
			break;
		}
		nxtThread = (1 + nxtThread) % threadsNum;
		if(i == threadsNum - 1 && type == FINE){
			cout << "should never be printed - contextSwitch" << endl;
		}
	}
	for(int i = 0; i < switchOverhead; i++){
		this->nxtCycle(); // context switch overhead
	}
	return runningThread;
}

void simulator::nxtCycle(){
	numOfCycles++;
	for(int i = 0; i < threadsNum; i++){
		if(threads[i].waiting){
			threads[i].waiting--;
		}
	}
}

bool simulator::findAvailableThread(){
	if(threadsNum == haltNum){
		return false;
	}
	while(true){
		for(int i = 0; i < threadsNum; i++){
			if(threads[i].waiting == 0 && threads[i].isHalt == false){
				return true;
			}
		}
		this->nxtCycle(); // idle
	}
	cout << "should never be printed - findAvailableThread" << endl;
	return true;
}

simulator* blockedMT = NULL;
simulator* finegrain = NULL;




void CORE_BlockedMT() {
	blockedMT = new simulator(BLOCKED);
	thread* runningThread = &(blockedMT->threads[0]); // first thread in the thilat olam
	while(blockedMT->threadsNum != blockedMT->haltNum){
		while(runningThread->waiting == 0){
			blockedMT->runInst(runningThread);
			blockedMT->nxtCycle(); // nxtCycle is decreasing waiting and it should not do so.
			if(runningThread->isHalt){
				break;
			}
		}
		if(blockedMT->findAvailableThread() == false){
			break;
		}
		runningThread = blockedMT->contextSwitch(runningThread);
	}
}

void CORE_FinegrainedMT() { 
	finegrain = new simulator(FINE);
	thread* runningThread = &(finegrain->threads[0]);
	while(finegrain->threadsNum != finegrain->haltNum){
		finegrain->runInst(runningThread);
		finegrain->nxtCycle();
		if(finegrain->findAvailableThread() == false){
			break;
		}
		runningThread = finegrain->contextSwitch(runningThread);
	}

}

double CORE_BlockedMT_CPI(){
	double totInst = blockedMT->instNum;
	double totCycles = blockedMT->numOfCycles;
	delete blockedMT;
	return (totCycles / totInst);
}

double CORE_FinegrainedMT_CPI(){
	double totInst = finegrain->instNum;
	double totCycles = finegrain->numOfCycles;
	delete finegrain;
	return (totCycles / totInst);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0; i < REGS_COUNT; i++){
		context[threadid].reg[i] = blockedMT->threads[threadid].reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0; i < REGS_COUNT; i++){
		context[threadid].reg[i] = finegrain->threads[threadid].reg[i];
	}
}
