/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cache.h"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// *** calc caches ***
	int L1Sets = calcSize(L1Size, BSize, L1Assoc);
	int L2Sets = calcSize(L2Size, BSize, L2Assoc);
	cout << "L1 num of sets: " << L1Sets << "\t L2 num of sets: " << L2Sets << endl;
	cache L1(L1Assoc, L1Sets);
	cache L2(L2Assoc, L2Sets);
	// *** calc caches ***

	double totalCommandCnt = 0; //total num of commands performed
	double totalCyclesCnt = 0; //total num of cycles in all program
	double L1Access = 0; //num of access to L1
	double L2Access = 0; //num of access to L2
	double L1MissCnt = 0; //num of misses in L1
	double L2MissCnt = 0; //num of misses in L2

	while (getline(file, line)) {
		totalCommandCnt++;
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		int L1SetNum = (L1Sets - 1) & (num >> BSize);
		int L2SetNum = (L2Sets - 1) & (num >> BSize);
		int L1BitNum = BitCalc(L1Sets);
		int L1TagNum = num >> (BSize + L1BitNum);
		int L2BitNum =  BitCalc(L2Sets);
		int L2TagNum = num >> (BSize + L2BitNum);

		// cout << "L1 Tag = " << L1TagNum << "\t L1 set = " << L1SetNum << "\t L2 Tag = " << L2TagNum << "\t L2 set = " << L2SetNum << endl;
		// ****** do operations
		int writeBackTagNum = 0;
		if(operation == 'r'){
			L1Access++;
			totalCyclesCnt += L1Cyc;
			bool hit1 = L1.read(L1SetNum, L1TagNum);
			if(!hit1){
				L1MissCnt++;
				L2Access++;
				totalCyclesCnt += L2Cyc;
				bool hit2 = L2.read(L2SetNum, L2TagNum);
				if(!hit2){
					L2MissCnt++;
					totalCyclesCnt += MemCyc;
					if (L2.isLineFull(L2SetNum)){
						L2.evict(L2SetNum, writeBackTagNum); //writing to memory = just erase the line (valid = 0)
						int L1numTranslate = ((writeBackTagNum << L2BitNum) | L2SetNum) << BSize;
						int L1TagTranslate = L1numTranslate >> (BSize + L1BitNum);
						int L1SetTranslate = (L1Sets - 1) & (L1numTranslate >> BSize);
						L1.snoop(L1SetTranslate, L1TagTranslate);
					}
					L2.insert(L2SetNum, L2TagNum);
				}
				if(L1.isLineFull(L1SetNum)){

					bool isDirty = L1.evict(L1SetNum, writeBackTagNum);
					if (isDirty){
						int L2numTranslate = ((writeBackTagNum << L1BitNum) | L1SetNum) << BSize;
						int L2TagTranslate = L2numTranslate >> (BSize + L2BitNum); // overwrite L2TagNum!!!!!!!!!
						int L2SetTranslate = (L2Sets - 1) & (L2numTranslate >> BSize);
						L2.writeBack(L2SetTranslate, L2TagTranslate);
					}
				}
				L1.insert(L1SetNum, L1TagNum);
			}
		}
		else{// operation == 'w'
			L1Access++;
			totalCyclesCnt += L1Cyc;
			bool hit1 = L1.write(L1SetNum, L1TagNum);
			if(!hit1){
				L1MissCnt++;
				L2Access++;
				totalCyclesCnt += L2Cyc;
				if(WrAlloc){
					if(L1.isLineFull(L1SetNum)){
						bool isDirty = L1.evict(L1SetNum, writeBackTagNum);
						if(isDirty){
							int L2numTranslate = ((writeBackTagNum << L1BitNum) | L1SetNum) << BSize;
							int L2TagTranslate = L2numTranslate >> (BSize + L2BitNum);
							int L2SetTranslate = (L2Sets - 1) & (L2numTranslate >> BSize);
							L2.writeBack(L2SetTranslate, L2TagTranslate);	
						}
					}
					bool hit2 = L2.isExist(L2SetNum, L2TagNum);
					if(!hit2){
						L2MissCnt++;
						totalCyclesCnt += MemCyc;
						if(L2.isLineFull(L2SetNum)){
							L2.evict(L2SetNum, writeBackTagNum);
							int L1numTranslate = ((writeBackTagNum << L2BitNum) | L2SetNum) << BSize;
							int L1TagTranslate = L1numTranslate >> (BSize + L1BitNum);
							int L1SetTranslate = (L1Sets - 1) & (L1numTranslate >> BSize);
							L1.snoop(L1SetTranslate, L1TagTranslate);
						}
						L2.insert(L2SetNum, L2TagNum);
					}
					L1.insert(L1SetNum, L1TagNum);
					if(!L1.write(L1SetNum, L1TagNum)){
						cout << "BUUUGGGG in write op" << endl; //should NOT be printed!
					}
				}
				else{//no write allocate
					bool hit2 = L2.isExist(L2SetNum, L2TagNum);
					if(!hit2){
						L2MissCnt++;
						totalCyclesCnt += MemCyc;
					}//write to memory. simulator do nothing
				}
			}
		}
	}

	double L1MissRate = L1MissCnt / L1Access;
	double L2MissRate = L2MissCnt / L2Access;
	double avgAccTime = totalCyclesCnt / totalCommandCnt;




	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);
	L1.~cache();
	L2.~cache();
	return 0;
}
