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
	bool inQ;
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
	thread* contextSwitch(thread*);
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
	inQ = false;
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
		threads[i].inQ = true;
	}
	numOfCycles = 0;
	instNum = 0;
	loadLatency = SIM_GetLoadLat();
	storeLatency = SIM_GetStoreLat();
	switchOverhead = SIM_GetSwitchCycles();

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
	    	thread->waiting = loadLatency;
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
	    	thread->waiting = storeLatency;
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
	if(runningThread->isHalt){
		if(readyQ.empty() == false){
			runningThread = &(threads[readyQ.front()]); // BUG: cant change pointer!!
			readyQ.pop();
		}
	}else{
		if(runningThread->waiting == 0 || readyQ.empty()){
			// this->nxtCycle();
			return runningThread;
		}
		runningThread = &(threads[readyQ.front()]);
		readyQ.pop();
	}
	for(int i = 0; i < switchOverhead; i++){
		this->nxtCycle();
	}
	runningThread->inQ = false;
	return runningThread;
}

void simulator::nxtCycle(){
	numOfCycles++;
	for (int i = 0; i < threadsNum; i++){
		if(threads[i].isHalt){
			continue;
		}
		else if(threads[i].waiting){
			threads[i].waiting--;
		}
		else if((threads[i].waiting == 0) && (threads[i].inQ == false)){
			readyQ.push(i);
			threads[i].inQ = true;
		}
	}

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
		runningThread->inQ = false;
	}
	while(blockedMT->threadsNum != blockedMT->haltNum){
		while(blockedMT->readyQ.empty() == false){
			if(isFirst == false){
				runningThread = blockedMT->contextSwitch(runningThread); // switch only if current waiting != 0
			}
			isFirst = false;
			while(!(runningThread->waiting)){
				blockedMT->runInst(runningThread);
				if(runningThread->isHalt){
					break;
				}
				blockedMT->nxtCycle();
			}
		}
		blockedMT->nxtCycle();
	}
}

void CORE_FinegrainedMT() {
	thread* runningThread;
	finegrain = new simulator();
	while(finegrain->threadsNum != finegrain->haltNum){
		while(finegrain->readyQ.empty() == false){
			runningThread = &(finegrain->threads[finegrain->readyQ.front()]);
			finegrain->readyQ.pop();
			runningThread->inQ = false;
			finegrain->runInst(runningThread);
			finegrain->nxtCycle();
		}
		finegrain->nxtCycle(); //adding to queue by minimum thread idx
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
		context->reg[i] = blockedMT->threads[threadid].reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0; i < REGS_COUNT; i++){
		context->reg[i] = finegrain->threads[threadid].reg[i];
	}

}
