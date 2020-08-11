#pragma once
#include "BuddyAllocator.cpp"
#include "Utils.cpp"
#include <cstdlib>
#include<iostream>
using namespace std;

const int ALLOCATED_MEMORY_IN_BYTES = 100;
const int BLOCK_SIZE_FOR_ALLOCATOR_POWER_OF_TWO = 64;
const int BLOCK_SIZE_FOR_ALLOCATOR_NOT_POWER_OF_TWO = 55;

void testBitManipulationLogic();
void testAllocatingAndFreeingAllSmallestBlocks();
void testProvidingBlockSizeWhichIsNotPowerOfTwo();

int main() {
	testAllocatingAndFreeingAllSmallestBlocks();
	//testBitManipulationLogic();
	//testProvidingBlockSizeWhichIsNotPowerOfTwo();

	cout << alignof(int) << endl;
	cout << alignof(max_align_t) << endl;

	return 0;
}

void testAllocatingAndFreeingAllSmallestBlocks() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	BuddyAllocator allocator(pointerToSomeMemory, BLOCK_SIZE_FOR_ALLOCATOR_POWER_OF_TWO);

	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	void* allocated = allocator.allocate(16);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	void* allocated2 = allocator.allocate(16);
	void* allocated3 = allocator.allocate(16);
	void* allocated4 = allocator.allocate(16);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	int* randomInt = (int*)allocated2;
	*randomInt = 8;
	cout << *randomInt << endl;
	cout << (uintptr_t)allocated2 % alignof(max_align_t) << endl;

	double* randomDouble = (double*)allocated3;
	*randomDouble = 8.5;
	cout << *randomDouble << endl;
	cout << (uintptr_t)allocated3 % alignof(max_align_t) << endl;

	allocator.free(allocated);
	allocator.free(allocated2);
	allocator.free(allocated3);
	allocator.free(allocated4);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;

	free(pointerToSomeMemory);
}

void testProvidingBlockSizeWhichIsNotPowerOfTwo() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY_IN_BYTES);
	BuddyAllocator allocator(pointerToSomeMemory, BLOCK_SIZE_FOR_ALLOCATOR_NOT_POWER_OF_TWO);
	allocator.printAllocatorStateUsingBitSet();
	cout << endl;
	
	allocator.free((char*) pointerToSomeMemory - 9);
	allocator.free((char*)pointerToSomeMemory + 7);
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