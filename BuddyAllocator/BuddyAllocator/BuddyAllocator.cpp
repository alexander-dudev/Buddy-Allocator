#pragma once
#include "Utils.cpp"
#include <iostream>
using namespace std;

class BuddyAllocator {

// TODO:
// 1) initial block size to be any number
//    an offset is needed, so that the allocator can know where its metadata begins 
//    and where its real data begins (for initial blockswhich are not a power of two)
// 2) free and split tables should be moved inside the block, at the beginning, aligned
// 3) the information about "forbidden" blocks should be saved;
//    i.e. blocks for our inner structures that cannot be freed by the user!
//
//  4) deal with a not aligned by the user pointer to the block beginnings

private:
	void* pointerToUsablePartFromBlock;
	void* pointerToBlockBeginning;
	int blockSizeInBytes;

	uint8_t* freeTable;
	uint8_t* splitTable;
	int numberOfPossibleBlocks;
	int numberOfPossibleBlocksWithoutLastLevel;
	int requiredBytesForFreeTable;
	int requiredBytesForSplitTable;

	int* forbiddenBlocks;
	int forbiddenBlocksSize;

public:

	BuddyAllocator(void* pointerToBlockBeginning, int blockSizeInBytes) {
		this->pointerToUsablePartFromBlock = pointerToBlockBeginning;
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->blockSizeInBytes = blockSizeInBytes;

		// free and split tables initialization
		int levels = Utils::calculteNumberOfLevelsFor(this->blockSizeInBytes);
		this->numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);
		this->numberOfPossibleBlocksWithoutLastLevel = Utils::calculateNumberOfPossibleBlocks(levels - 1);

		this->requiredBytesForFreeTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocks);
		this->requiredBytesForSplitTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocksWithoutLastLevel);

		initializeFreeAndSplitTables();

		if (!Utils::numberIsPowerOfTwo(blockSizeInBytes)) {
			int closestBiggerBlockSizeWhichIsPowerOfTwo = Utils::findClosestBiggerNumberWhichIsPowerOfTwo(blockSizeInBytes);
			int missingBytes = closestBiggerBlockSizeWhichIsPowerOfTwo - blockSizeInBytes;
			this->pointerToBlockBeginning = (char*)pointerToUsablePartFromBlock - missingBytes;
			this->blockSizeInBytes = closestBiggerBlockSizeWhichIsPowerOfTwo;

			this->forbiddenBlocksSize = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(missingBytes);
			forbiddenBlocks = new int[forbiddenBlocksSize];
			for (int i = 0; i < forbiddenBlocksSize; ++i) {
				void* allocatedBlock = allocate(Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES);
				int blockIndex = findBlockIndexFrom(allocatedBlock);
				forbiddenBlocks[i] = blockIndex;
			}
		}


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
		for (int i = 0; i < forbiddenBlocksSize; ++i) {
			if (blockIndex == forbiddenBlocks[i]) {
				cout << "Warning! This block cannot be freed!";
				return false;
			}
		}

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
			this->freeTable[i] = (uint8_t)255;
		}

		for (int i = 0; i < requiredBytesForSplitTable; ++i) {
			this->splitTable[i] = (uint8_t)0;
		}
	}

	int findByteIndexFor(int blockIndex) {
		return blockIndex / Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	int findBitIndexInsideSingleByteFor(int blockIndex) {
		return blockIndex % Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	bool isFree(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to check whether a block is free or busy!" << endl;
			return false;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		return freeTable[byteIndex] & mask;
	}

	void markBlockAsFree(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as free!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		freeTable[byteIndex] |= mask;
	}

	void markBlockAsBusy(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as busy!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		freeTable[byteIndex] &= ~mask;
	}

	int createMaskFor(int bitIndexInsideSingleByte) {
		return Utils::MASK_FOR_BIT_WITH_INDEX_ZERO << bitIndexInsideSingleByte;
	}

	bool isSplit(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to see whether a block is split or not!" << endl;
			return false;
		}

		if (blockIndex >= numberOfPossibleBlocksWithoutLastLevel) {
			return false;
		}

		// 0 <= blockIndex < numberOfPossibleBlocksWithoutLastLevel
		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		return splitTable[byteIndex] & mask;
	}

	void markBlockAsSplit(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as split!" << endl;
			return;
		}

		if (blockIndex >= numberOfPossibleBlocksWithoutLastLevel) {
			cerr << "Warning! A block from the last level of the allocator tree cannot be marked as split!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		splitTable[byteIndex] |= mask;
	}

	void markBlockAsNotSplit(int blockIndex) {
		if (blockIndex >= numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as not split!" << endl;
			return;
		}

		if (blockIndex >= numberOfPossibleBlocksWithoutLastLevel) {
			cerr << "Warning! A block from the last level of the allocator tree cannot be marked as not split!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		splitTable[byteIndex] &= ~mask;
	}
};