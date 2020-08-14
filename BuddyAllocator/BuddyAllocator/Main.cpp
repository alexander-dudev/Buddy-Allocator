#include "BuddyAllocator.cpp"
#include "Utils.cpp"
#include <cstdlib>
#include <iostream>
using namespace std;

const int ALLOCATED_MEMORY = 100;
const int BLOCK_SIZE_POWER_OF_TWO = 64;
const int BLOCK_SIZE_NOT_POWER_OF_TWO = 90;

void testAlignedBlock();
void testNotAlignedBlock();

void testAllocator(void*, int);
void testAllocator2(void*, int);

void testBitManipulationLogic();
void printAlignOfValues();

int main() {
	testAlignedBlock();

	//testNotAlignedBlock();

	//testBitManipulationLogic();
	
	//printAlignOfValues();

	return 0;
}

void testAlignedBlock() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY);

	testAllocator(pointerToSomeMemory, BLOCK_SIZE_POWER_OF_TWO);
	//testAllocator(pointerToSomeMemory, BLOCK_SIZE_NOT_POWER_OF_TWO);

	free(pointerToSomeMemory);
}

void testNotAlignedBlock() {
	void* pointerToSomeMemory = malloc(ALLOCATED_MEMORY);

	//testAllocator((void*)((uint8_t*)pointerToSomeMemory + 2), 2 * BLOCK_SIZE_POWER_OF_TWO);
	//testAllocator((void*)((uint8_t*)pointerToSomeMemory + 2), BLOCK_SIZE_NOT_POWER_OF_TWO);

	free(pointerToSomeMemory);
}


void testAllocator(void* pointerToMemory, int sizeInBytes) {
	BuddyAllocator allocator(pointerToMemory, sizeInBytes);

	allocator.printAllocatorState();
	cout << endl;

	void* allocated1 = allocator.allocate(16);
	allocator.printAllocatorState();
	cout << endl;

	void* allocated2 = allocator.allocate(16);
	void* allocated3 = allocator.allocate(16);
	void* allocated4 = allocator.allocate(16);
	allocator.printAllocatorState();
	cout << endl;

	int* randomInt = (int*)allocated1;
	*randomInt = 8;
	cout << *randomInt << endl;
	cout << (uintptr_t)allocated1 % alignof(max_align_t) << endl;

	double* randomDouble = (double*)allocated2;
	*randomDouble = 8.5;
	cout << *randomDouble << endl;
	cout << (uintptr_t)allocated2 % alignof(max_align_t) << endl;

	allocator.free(allocated1);
	allocator.free(allocated2);
	allocator.free(allocated3);
	allocator.free(allocated4);

	allocator.printAllocatorState();
	cout << endl;
}

void testAllocator2(void* pointerToMemory, int sizeInBytes) {
	BuddyAllocator allocator(pointerToMemory, sizeInBytes);
	allocator.printAllocatorState();
	cout << endl;

	void* allocated1 = allocator.allocate(16);
	void* allocated2 = allocator.allocate(32);
	allocator.printAllocatorState();
	cout << endl;

	allocator.free(allocated1);
	allocator.free(allocated2);
	allocator.printAllocatorState();
	cout << endl;
}

void testBitManipulationLogic() {
	void* someMemory = malloc(BLOCK_SIZE_POWER_OF_TWO);

	BuddyAllocator allocator(someMemory, BLOCK_SIZE_POWER_OF_TWO);
	allocator.printAllocatorState();
	cout << endl;

	allocator.markBlockAsSplit(2);
	allocator.markBlockAsBusy(5);

	allocator.printAllocatorState();
	cout << endl;

	allocator.markBlockAsNotSplit(2);
	allocator.markBlockAsFree(5);

	allocator.printAllocatorState();
	cout << endl;

	free(someMemory);
}

void printAlignOfValues() {
	cout << alignof(int) << endl;
	cout << alignof(double) << endl;
	cout << alignof(max_align_t) << endl;
}