#pragma once
#include "BuddyAllocator.cpp"
#include "Utils.cpp"
#include <cstdlib>
#include<iostream>
using namespace std;

const int ALLOCATED_MEMORY_IN_BYTES = 100;
const int MEMORY_FOR_ALLOCATOR = 64;

void testBitManipulationLogic();
void printAllocatorState(Utils&, int);
void testWorkingWithBuddyAllocator();

int main() {
	//testBitManipulationLogic();

	testWorkingWithBuddyAllocator();

	return 0;
}

void testWorkingWithBuddyAllocator() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	BuddyAllocator allocator(pointerToSomeMemory, MEMORY_FOR_ALLOCATOR);

	allocator.printAllocatorStateUsingBoolSet();
	cout << endl;

	void* allocated = allocator.allocateUsingTree(16, 0, MEMORY_FOR_ALLOCATOR);
	allocator.printAllocatorStateUsingBoolSet();
	cout << endl;

	allocator.freeUsingTree(allocated);
	allocator.printAllocatorStateUsingBoolSet();
	cout << endl;

	free(pointerToSomeMemory);
}

void testBitManipulationLogic() {
	const int ALLOCATED_MEMORY_IN_BYTES = 100;
	const int MEMORY_FOR_ALLOCATOR = 64;

	void* someMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	Utils utils(MEMORY_FOR_ALLOCATOR, someMemory);

	int levels = utils.calculteNumberOfLevels(MEMORY_FOR_ALLOCATOR);
	int numberOfPossibleBlocks = utils.calculateNumberOfPossibleBlocks(levels);

	cout << "All blocks should be free and not split\n";
	printAllocatorState(utils, numberOfPossibleBlocks);

	utils.markBlockAsSplit(0);
	utils.markBlockAsSplit(1);
	utils.markBlockAsBusy(4);
	utils.markBlockAsBusy(5);

	printAllocatorState(utils, numberOfPossibleBlocks);

	utils.markBlockAsNotSplit(0);
	utils.markBlockAsNotSplit(1);
	utils.markBlockAsFree(4);
	utils.markBlockAsFree(5);

	printAllocatorState(utils, numberOfPossibleBlocks);

	free(someMemory);
}

void printAllocatorState(Utils& utils, int numberOfPossibleBlocks) {
	for (int i = 0; i < numberOfPossibleBlocks; i++) {
		cout << "Block with index " << i << " is: " << (utils.isFree(i) ? "free" : "busy") << " and "
			<< (utils.isSplit(i) ? "split" : "not split") << endl;
	}
}