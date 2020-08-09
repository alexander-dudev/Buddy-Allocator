#pragma once
#include "BuddyAllocator.cpp"
#include "Utils.cpp"
#include <cstdlib>
#include<iostream>
using namespace std;

const int ALLOCATED_MEMORY_IN_BYTES = 100;
const int MEMORY_FOR_ALLOCATOR = 64;

void testBitManipulationLogic();
void testWorkingWithBuddyAllocator();

int main() {
	//testBitManipulationLogic();

	testWorkingWithBuddyAllocator();

	return 0;
}

void testWorkingWithBuddyAllocator() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	BuddyAllocator allocator(pointerToSomeMemory, MEMORY_FOR_ALLOCATOR);

	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	void* allocated = allocator.allocate(16);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	/*
	void* allocated2 = allocator.allocate(16);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;
	*/

	allocator.free(allocated);
	//allocator.free(allocated2);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	free(pointerToSomeMemory);
}

void testBitManipulationLogic() {
	const int ALLOCATED_MEMORY_IN_BYTES = 100;
	const int MEMORY_FOR_ALLOCATOR = 64;

	void* someMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	BuddyAllocator allocator(someMemory, MEMORY_FOR_ALLOCATOR);

	int levels = Utils::calculteNumberOfLevelsFor(MEMORY_FOR_ALLOCATOR);
	int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);

	cout << "All blocks should be free and not split\n";
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	allocator.markBlockAsSplit(0);
	allocator.markBlockAsSplit(1);
	allocator.markBlockAsBusy(4);
	allocator.markBlockAsBusy(5);

	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	allocator.markBlockAsNotSplit(0);
	allocator.markBlockAsNotSplit(1);
	allocator.markBlockAsFree(4);
	allocator.markBlockAsFree(5);

	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	free(someMemory);
}