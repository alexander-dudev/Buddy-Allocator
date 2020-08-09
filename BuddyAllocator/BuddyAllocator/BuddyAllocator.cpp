#pragma once
#include "Utils.cpp"
#include<iostream>
using namespace std;

class BuddyAllocator {
	
private:
	Utils utils;
	void* pointerToBlockBeginning;
	int sizeInBytes;
	// offset is needed, so that the allocator can know where its metadata begins (for initial blocks
	// which are not a power of two)
	int numberOfPossibleBlocks;

public:

	BuddyAllocator(void* pointerToBlockBeginning, int sizeInBytes) : utils(sizeInBytes, pointerToBlockBeginning) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->sizeInBytes = sizeInBytes;
		int levels = utils.calculteNumberOfLevels(sizeInBytes);
		this->numberOfPossibleBlocks = utils.calculateNumberOfPossibleBlocks(levels);
	}

	void* allocateUsingTree(int blockSizeInBytes, int currentBlockIndex, int blockSizeForCurrentLevel) {
		// assuming blockSizeInBytes is a power of two; the caller will find the closestbiggerPowerOfTwo
		// as this is a helper recursive function

		// free
		if (!utils.isSplitUsingBool(currentBlockIndex) && utils.isFreeUsingBool(currentBlockIndex) && blockSizeInBytes == blockSizeForCurrentLevel) {
			utils.changeFreeStateUsingBool(currentBlockIndex, false);
			return utils.findPointerBy(currentBlockIndex);
		}

		// free but with larger size
		if (!utils.isSplitUsingBool(currentBlockIndex) && utils.isFreeUsingBool(currentBlockIndex) && blockSizeInBytes < blockSizeForCurrentLevel) {
			utils.changeSplitStateUsingBool(currentBlockIndex, true);
			return allocateUsingTree(blockSizeInBytes, utils.getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
		}

		// busy
		if (!utils.isSplitUsingBool(currentBlockIndex) && !utils.isFreeUsingBool(currentBlockIndex)) {
			return nullptr;
		}

		// searchig recursively from the left branch
		void* allocatedBlockFromLeftBranch = allocateUsingTree(blockSizeInBytes, utils.getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);

		return allocatedBlockFromLeftBranch != nullptr ? allocatedBlockFromLeftBranch : allocateUsingTree(blockSizeInBytes, utils.getRightChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
	}

	bool freeUsingTree(void* pointerToBlock) {
		int blockIndex = utils.findBlockIndexFrom(pointerToBlock);

		if (!utils.isFreeUsingBool(blockIndex)) {
			utils.changeFreeStateUsingBool(blockIndex, true);
			mergeBlocks(utils.getParentIndex(blockIndex));
			return true;
		} else {
			return false;
		}
	}

	void mergeBlocks(int parentBlock) {
		if (!utils.isFreeUsingBool(utils.getLeftChildIndex(parentBlock)) || !utils.isFreeUsingBool(utils.getRightChildIndex(parentBlock))) {
			return;
		}

		utils.changeSplitStateUsingBool(parentBlock, false);
		utils.changeFreeStateUsingBool(parentBlock, true);

		if (parentBlock == 0) {
			return;
		} else {
		mergeBlocks(utils.getParentIndex(parentBlock));
		}
	}

	void printAllocatorStateUsingBitSet() {
		for (int i = 0; i < numberOfPossibleBlocks; i++) {
			cout << "Block with index " << i << " is: " << (utils.isFree(i) ? "free" : "busy") << " and "
				<< (utils.isSplit(i) ? "split" : "not split") << endl;
		}
	}

	void printAllocatorStateUsingBoolSet() {
		for (int i = 0; i < numberOfPossibleBlocks; i++) {
			cout << "Block with index " << i << " is: " << (utils.isFreeUsingBool(i) ? "free" : "busy") << " and "
				<< (utils.isSplitUsingBool(i) ? "split" : "not split") << endl;
		}
	}
};