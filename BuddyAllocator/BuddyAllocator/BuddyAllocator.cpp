#pragma once
#include "Utils.cpp"
#include <iostream>
using namespace std;

class BuddyAllocator {
	
private:
	void* pointerToBlockBeginning;
	int blockSizeInBytes;

	// offset is needed, so that the allocator can know where its metadata begins (for initial blocks
	// which are not a power of two)

	// the information about "forbidden" blocks should be saved
	// blocks for our inner structures that cannot be freed by the user!

	uint8_t* freeTable;
	uint8_t* splitTable;
	int requiredBytesForFreeTable;
	int requiredBytesForSplitTable;

public:

	BuddyAllocator(void* pointerToBlockBeginning, int blockSizeInBytes) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->blockSizeInBytes = blockSizeInBytes;
		int levels = Utils::calculteNumberOfLevelsFor(blockSizeInBytes);
		int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(blockSizeInBytes);

		// free and split tables
		this->requiredBytesForFreeTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocks);
		// later on could be optimized to use less memory:
		// this->requiredBytesForSplitTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocksWithoutLastLevel);
		this->requiredBytesForSplitTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocks);
		initializeFreeAndSplitTables();
	}

	~BuddyAllocator() {
		delete[] freeTable;
		delete[] splitTable;
	}

	// big 4 needed unless free and split tables are moved inside the blocksss

	void* allocate(int sizeInBytes) {
		int sizeToAllocate = Utils::findClosestBiggerNumberWhichIsPowerOfTwo(sizeInBytes);
		return sizeToAllocate <= blockSizeInBytes ? allocateRecursively(sizeToAllocate, 0, blockSizeInBytes) : nullptr;
	}

	bool free(void* pointerToBlock) {
		int blockIndex = findBlockIndexFrom(pointerToBlock);

		if (!isSplit(blockIndex) && !isFree(blockIndex)) {
			markBlockAsFree(blockIndex);
			mergeFreeBlocks(Utils::getParentIndex(blockIndex));
			return true;
		} else {
			return false;
		}
	}

private:

	void* allocateRecursively(int blockSizeInBytes, int currentBlockIndex, int blockSizeForCurrentLevel) {
		// assuming blockSizeInBytes is a power of two; the caller will find the closestbiggerPowerOfTwo
		// as this is a helper recursive function

		// the block is free
		if (!isSplit(currentBlockIndex) && isFree(currentBlockIndex) && blockSizeInBytes == blockSizeForCurrentLevel) {
			markBlockAsBusy(currentBlockIndex);
			return findPointerBy(currentBlockIndex);
		}

		// the block is free but with larger size
		if (!isSplit(currentBlockIndex) && isFree(currentBlockIndex) && blockSizeInBytes < blockSizeForCurrentLevel) {
			markBlockAsSplit(currentBlockIndex);
			return allocateRecursively(blockSizeInBytes, Utils::getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
		}

		// the block is busy
		if (!isSplit(currentBlockIndex) && !isFree(currentBlockIndex)) {
			return nullptr;
		}

		// searchig recursively an appropriate block from the left and right branches
		void* allocatedBlockFromLeftBranch = allocateRecursively(blockSizeInBytes, Utils::getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);

		return allocatedBlockFromLeftBranch != nullptr ? allocatedBlockFromLeftBranch : allocateRecursively(blockSizeInBytes, Utils::getRightChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
	}

	void mergeFreeBlocks(int parentBlockIndex) {
		if (!isFree(Utils::getLeftChildIndex(parentBlockIndex)) || !isFree(Utils::getRightChildIndex(parentBlockIndex))) {
			return;
		}

		markBlockAsNotSplit(parentBlockIndex);
		markBlockAsFree(parentBlockIndex);

		if (parentBlockIndex == 0) {
			return;
		} else {
			mergeFreeBlocks(Utils::getParentIndex(parentBlockIndex));
		}
	}

	int findBlockIndexFrom(void* pointer) {
		return findBlockIndexRecursively(pointer, pointerToBlockBeginning, 0, blockSizeInBytes);
	}

	// given pointer -> index
	// we need to be sure that the pointer points inside the block
	// we need either level or block size as well
	// we can find it only using a pointer, but will be more difficult - need to know which blocks are split
	// initial value for blockIndexForCurrentLevel : 0
	//                   blockSize = whole initial block size
	int findBlockIndexRecursively(void* pointerToSearchedBlock, void* pointerToBlockAtCurrentLevel, int blockIndexForCurrentLevel, int blockSizeForCurrentLevel) {
		if (pointerToSearchedBlock == pointerToBlockAtCurrentLevel && !isSplit(blockIndexForCurrentLevel)) {
			return blockIndexForCurrentLevel;
		}

		int blockSizeForLowerLevel = blockSizeForCurrentLevel / 2;

		if (pointerToSearchedBlock < (char*)pointerToBlockAtCurrentLevel + blockSizeForLowerLevel) {
			// moving to the left child
			return findBlockIndexRecursively(pointerToSearchedBlock, pointerToBlockAtCurrentLevel, Utils::getLeftChildIndex(blockIndexForCurrentLevel), blockSizeForLowerLevel);
		}
		else {
			// moving to the right child
			return findBlockIndexRecursively(pointerToSearchedBlock, (char*)pointerToBlockAtCurrentLevel + blockSizeForLowerLevel,
				                             Utils::getRightChildIndex(blockIndexForCurrentLevel), blockSizeForLowerLevel);
		}
	}

	// given a nodeIndex -> return a pointer to the node

	// blokove razstoqnie sprqmo nachaloto
	// 1) blockIndex -> distance
	// index -> znaem nivo -> znaem razmer na blok
	// findFirstNodeIndexFor(index) - index -> blokove razstoqnie sprqmo nachaloto
	// ot tam mojem da namerim adresa na bloka i da go vyrnem

	void* findPointerBy(int nodeIndex) {
		int nodeLevel = Utils::getLevelBy(nodeIndex);
		int blockSizeForNodeLevel = Utils::calculateBlockSizePer(nodeLevel, blockSizeInBytes);
		int numberOfBlocksOffsetForNode = nodeIndex - Utils::findFirstNodeIndexFor(nodeLevel);

		return (char*)pointerToBlockBeginning + (numberOfBlocksOffsetForNode * blockSizeForNodeLevel);
	}

public:
	void printAllocatorStateUsingBitSet() {
		int levels = Utils::calculteNumberOfLevelsFor(blockSizeInBytes);
		int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);
		for (int i = 0; i < numberOfPossibleBlocks; i++) {
			cout << "Block with index " << i << " is: " << (isFree(i) ? "free" : "busy") << " and "
				<< (isSplit(i) ? "split" : "not split") << endl;
		}
	}

public:

	void initializeFreeAndSplitTables() {
		freeTable = new uint8_t[requiredBytesForFreeTable];
		splitTable = new uint8_t[requiredBytesForSplitTable];

		for (int i = 0; i < requiredBytesForFreeTable; ++i) {
			this->freeTable[i] = 255;
		}

		for (int i = 0; i < requiredBytesForSplitTable; ++i) {
			this->splitTable[i] = 0;
		}
	}

	int findByteIndexFor(int blockIndex) {
		return blockIndex / Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	int createMaskFor(int blockIndex) {
		int bitIndexInsideSingleByte = blockIndex % Utils::NUMBER_OF_BITS_IN_A_BYTE;
		return Utils::MASK_FOR_BIT_WITH_INDEX_ZERO << bitIndexInsideSingleByte;
	}

	bool isFree(int blockIndex) {
		int byteIndex = findByteIndexFor(blockIndex);
		uint8_t mask = createMaskFor(blockIndex);

		return freeTable[byteIndex] & mask;
	}

	bool isSplit(int blockIndex) {
		int byteIndex = findByteIndexFor(blockIndex);
		uint8_t mask = createMaskFor(blockIndex);

		return splitTable[byteIndex] & mask;
	}

	void markBlockAsFree(int blockIndex) {
		int byteIndex = findByteIndexFor(blockIndex);
		uint8_t mask = createMaskFor(blockIndex);

		freeTable[byteIndex] |= mask;
	}

	void markBlockAsBusy(int blockIndex) {
		int byteIndex = findByteIndexFor(blockIndex);
		uint8_t mask = createMaskFor(blockIndex);

		freeTable[byteIndex] &= ~mask;
	}

	void markBlockAsSplit(int index) {
		int byteIndex = index / 8;
		int bitIndexInsideByte = index % 8;
		uint8_t mask = 1 << bitIndexInsideByte;

		splitTable[byteIndex] |= mask;
	}

	void markBlockAsNotSplit(int index) {
		int byteIndex = index / 8;
		int bitIndexInsideByte = index % 8;
		uint8_t mask = 1 << bitIndexInsideByte;

		splitTable[byteIndex] &= ~mask;
	}
};