#include "Utils.cpp"
#include <iostream>
using namespace std;

class BuddyAllocator {

private:
	void* pointerToBlockBeginning;
	void* pointerForInnerDataInitialization;
	int blockSizeInBytes;

	uint8_t* freeTable;
	int* numberOfPossibleBlocks;
	int* requiredBytesForFreeTable;

	uint8_t* splitTable;
	int* numberOfPossibleBlocksWithoutLastLevel;
	int* requiredBytesForSplitTable;

	int* inaccessibleToUserBlockIndexes;
	int* numberOfInaccessibleToUserBlockIndexes;

public:

	BuddyAllocator(void* pointerToBlockBeginning, int blockSizeInBytesFromUser) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->pointerForInnerDataInitialization = pointerToBlockBeginning;
		this->blockSizeInBytes = blockSizeInBytesFromUser;

		int bytesForInitialization = 0;

		bytesForInitialization += handleTheCaseInWhichBlockSizeIsNotPowerOfTwo(blockSizeInBytesFromUser);
		bytesForInitialization += alignBlockBeginningIfNecessary();
		bytesForInitialization += initializeFreeAndSplitTables();

		int numberOfBlocksToStoreInnerDataSoFar = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesForInitialization);
		int allBytesForAllocatorInitialization = calculateBytesForInitializationAfterAddingInaccessibleBlocksData(bytesForInitialization, numberOfBlocksToStoreInnerDataSoFar);

		initializeInaccessibleBlocksData(allBytesForAllocatorInitialization);
	}

	void* allocate(int sizeInBytes) {
		int sizeToAllocate = Utils::findClosestBiggerNumberWhichIsPowerOfTwo(sizeInBytes);
		return sizeToAllocate <= blockSizeInBytes ? allocateRecursively(sizeToAllocate, 0, blockSizeInBytes) : nullptr;
	}

	bool free(void* pointerToBlock) {
		int blockIndex = findBlockIndexFrom(pointerToBlock);

		if (blockIndex == Utils::INVALID_BLOCK_INDEX) {
			cerr << "Warning! The block index is invalid!" << endl;
		}

		for (int i = 0; i < *(this->numberOfInaccessibleToUserBlockIndexes); ++i) {
			if (blockIndex == inaccessibleToUserBlockIndexes[i]) {
				cout << "Warning! This block cannot be freed!" << endl;
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
		if (isSplit(Utils::getLeftChildIndex(parentBlockIndex)) || !isFree(Utils::getLeftChildIndex(parentBlockIndex)) 
		    || isSplit(Utils::getRightChildIndex(parentBlockIndex)) || !isFree(Utils::getRightChildIndex(parentBlockIndex))) {
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
		return pointer != nullptr ? findBlockIndexRecursively(pointer, pointerToBlockBeginning, 0, blockSizeInBytes)
								  : Utils::INVALID_BLOCK_INDEX;
	}

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

	// block index -> can calculate level -> can calculate block size
	void* findPointerBy(int nodeIndex) {
		int nodeLevel = Utils::getLevelBy(nodeIndex);
		int blockSizeForNodeLevel = Utils::calculateBlockSizePer(nodeLevel, blockSizeInBytes);
		int numberOfBlocksOffsetForNode = nodeIndex - Utils::findFirstNodeIndexFor(nodeLevel);

		return (char*)pointerToBlockBeginning + (numberOfBlocksOffsetForNode * blockSizeForNodeLevel);
	}

	int handleTheCaseInWhichBlockSizeIsNotPowerOfTwo(int blockSizeInBytesFromUser) {
		int missingBytes = 0;
		if (!Utils::numberIsPowerOfTwo(blockSizeInBytesFromUser)) {
			int closestBiggerBlockSizeWhichIsPowerOfTwo = Utils::findClosestBiggerNumberWhichIsPowerOfTwo(blockSizeInBytesFromUser);
			this->blockSizeInBytes = closestBiggerBlockSizeWhichIsPowerOfTwo;
			missingBytes = closestBiggerBlockSizeWhichIsPowerOfTwo - blockSizeInBytesFromUser;
			this->pointerToBlockBeginning = (uint8_t*)pointerToBlockBeginning - missingBytes;
		}

		return missingBytes;
	}

	int alignBlockBeginningIfNecessary() {
		int bytesForInitialAlignment = 0;
		if ((uintptr_t)this->pointerToBlockBeginning % alignof(max_align_t) != 0) {
			bytesForInitialAlignment = ((uintptr_t)this->pointerToBlockBeginning % alignof(max_align_t));
			this->pointerToBlockBeginning = (uint8_t*)this->pointerToBlockBeginning - bytesForInitialAlignment;
		}

		return bytesForInitialAlignment;
	}

	int calculateBytesForInitializationAfterAddingInaccessibleBlocksData(int bytesForAllocatorInitialization, int numberOfBlocksToStoreInnerDataSoFar) {
		int bytesForInitializationAfterAddingInaccessibleBlocksData = bytesForAllocatorInitialization;

		if ((uintptr_t)this->pointerForInnerDataInitialization % alignof(int) != 0) {
			int bytesForAlignment = alignof(int)-((uintptr_t)this->pointerForInnerDataInitialization % alignof(int));
			this->pointerForInnerDataInitialization = (uint8_t*)this->pointerForInnerDataInitialization + bytesForAlignment;
			bytesForInitializationAfterAddingInaccessibleBlocksData += bytesForAlignment;
		}

		bytesForInitializationAfterAddingInaccessibleBlocksData += sizeof(int) + sizeof(int) * numberOfBlocksToStoreInnerDataSoFar;
		int numberOfBlocksAfterAddingInaccessibleBlocksInfo = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesForInitializationAfterAddingInaccessibleBlocksData);

		while (numberOfBlocksAfterAddingInaccessibleBlocksInfo - numberOfBlocksToStoreInnerDataSoFar > 0) {
			bytesForInitializationAfterAddingInaccessibleBlocksData += (numberOfBlocksAfterAddingInaccessibleBlocksInfo - numberOfBlocksToStoreInnerDataSoFar) * sizeof(int);
			numberOfBlocksToStoreInnerDataSoFar = numberOfBlocksAfterAddingInaccessibleBlocksInfo;
			numberOfBlocksAfterAddingInaccessibleBlocksInfo = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesForInitializationAfterAddingInaccessibleBlocksData);
		}

		return bytesForInitializationAfterAddingInaccessibleBlocksData;
	}

	void initializeInaccessibleBlocksData(int allBytesForInitialization) {
		// adds the following data in the provided by the user memory block:
		// bytes for alignment (for next int) | numberOfInaccessibleToUserBlockIndexes (int)
		// inaccessibleToUserBlockIndexes (int[])

		int requiredBlocks = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(allBytesForInitialization);
		initializeInteger(this->pointerForInnerDataInitialization, this->numberOfInaccessibleToUserBlockIndexes, requiredBlocks);
		
		inaccessibleToUserBlockIndexes = (int*)this->pointerForInnerDataInitialization;
		for (int i = 0; i < *(this->numberOfInaccessibleToUserBlockIndexes); ++i) {
			void* allocatedBlock = allocate(Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES);
			int blockIndex = findBlockIndexFrom(allocatedBlock);
			inaccessibleToUserBlockIndexes[i] = blockIndex;
		}
	}

	int initializeFreeAndSplitTables() {
		// adds the following data in the provided by the user memory block:
		// bytes for alignment (for next int) | numberOfPossibleBlocks (int) | requiredBytesForFreeTable (int)
		// numberOfPossibleBlockWithoutLastLevel (int) | requiredBytesForSplitTable (int)
		// bytes for free table bit field (uint8_t[]) | bytes for split table bit field (uint8_t[])

		int levels = Utils::calculateNumberOfLevelsFor(this->blockSizeInBytes);
		int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);
		int numberOfPossibleBlocksWithoutLastLevel = Utils::calculateNumberOfPossibleBlocks(levels - 1);

		int requiredBytesForFreeTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocks);
		int requiredBytesForSplitTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocksWithoutLastLevel);

		int totalBytesUsedForTables = 0;

		if ((uintptr_t)pointerForInnerDataInitialization % alignof(int) != 0) {
			totalBytesUsedForTables += alignof(int)-((uintptr_t)pointerForInnerDataInitialization % alignof(int));
			pointerForInnerDataInitialization = (uint8_t*)pointerForInnerDataInitialization + totalBytesUsedForTables;
		}

		// pointerToAlignedMemory is pointing to an aligned address for an int variable

		// number of possible blocks
		initializeInteger(pointerForInnerDataInitialization, this->numberOfPossibleBlocks, numberOfPossibleBlocks);
		totalBytesUsedForTables += sizeof(int);

		// free table size
		initializeInteger(pointerForInnerDataInitialization, this->requiredBytesForFreeTable, requiredBytesForFreeTable);
		totalBytesUsedForTables += sizeof(int);

		// number of possible blocks without last level
		initializeInteger(pointerForInnerDataInitialization, this->numberOfPossibleBlocksWithoutLastLevel, numberOfPossibleBlocksWithoutLastLevel);
		totalBytesUsedForTables += sizeof(int);

		// split table size
		initializeInteger(pointerForInnerDataInitialization, this->requiredBytesForSplitTable, requiredBytesForSplitTable);
		totalBytesUsedForTables += sizeof(int);

		// free table
		initializeTablePointer(pointerForInnerDataInitialization, freeTable, *(this->requiredBytesForFreeTable));
		totalBytesUsedForTables += *(this->requiredBytesForFreeTable);

		// split table
		initializeTablePointer(pointerForInnerDataInitialization, splitTable, *(this->requiredBytesForSplitTable));
		totalBytesUsedForTables += *(this->requiredBytesForSplitTable);

		for (int i = 0; i < requiredBytesForFreeTable; ++i) {
			this->freeTable[i] = Utils::LARGEST_8_BIT_NUMBER;
		}

		for (int i = 0; i < requiredBytesForSplitTable; ++i) {
			this->splitTable[i] = Utils::SMALLEST_8_BIT_NUMBER;
		}

		return totalBytesUsedForTables;
	}

	void initializeInteger(void*& pointerToMemory, int*& pointerToInitialize, int value) {
		pointerToInitialize = (int*)pointerToMemory;
		*(pointerToInitialize) = value;
		pointerToMemory = (uint8_t*)pointerToMemory + sizeof(int);
	}

	void initializeTablePointer(void*& pointerToMemory, uint8_t*& pointerToInitialize, int bytesForTable) {
		pointerToInitialize = (uint8_t*)pointerToMemory;
		pointerToMemory = (uint8_t*)pointerToMemory + bytesForTable;
	}

	int findByteIndexFor(int blockIndex) {
		return blockIndex / Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	int findBitIndexInsideSingleByteFor(int blockIndex) {
		return blockIndex % Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	bool isFree(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to check whether a block is free or busy!" << endl;
			return false;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		return freeTable[byteIndex] & mask;
	}



	int createMaskFor(int bitIndexInsideSingleByte) {
		return Utils::MASK_FOR_BIT_WITH_INDEX_ZERO << bitIndexInsideSingleByte;
	}

	bool isSplit(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to see whether a block is split or not!" << endl;
			return false;
		}

		if (blockIndex >= *numberOfPossibleBlocksWithoutLastLevel) {
			return false;
		}

		// 0 <= blockIndex < numberOfPossibleBlocksWithoutLastLevel
		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		return splitTable[byteIndex] & mask;
	}

public:

	void markBlockAsFree(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as free!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		freeTable[byteIndex] |= mask;
	}

	void markBlockAsBusy(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as busy!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		freeTable[byteIndex] &= ~mask;
	}

	void markBlockAsSplit(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as split!" << endl;
			return;
		}

		if (blockIndex >= *numberOfPossibleBlocksWithoutLastLevel) {
			cerr << "Warning! A block from the last level of the allocator tree cannot be marked as split!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		splitTable[byteIndex] |= mask;
	}

	void markBlockAsNotSplit(int blockIndex) {
		if (blockIndex >= *numberOfPossibleBlocks) {
			cerr << "Warning! An invalid block index is used to mark a block as not split!" << endl;
			return;
		}

		if (blockIndex >= *numberOfPossibleBlocksWithoutLastLevel) {
			cerr << "Warning! A block from the last level of the allocator tree cannot be marked as not split!" << endl;
			return;
		}

		int byteIndex = findByteIndexFor(blockIndex);
		int bitIndexInsideSingleByte = findBitIndexInsideSingleByteFor(blockIndex);
		uint8_t mask = createMaskFor(bitIndexInsideSingleByte);

		splitTable[byteIndex] &= ~mask;
	}

	void printAllocatorStateUsingBitSet() {
		int levels = Utils::calculateNumberOfLevelsFor(blockSizeInBytes);
		int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);
		for (int i = 0; i < numberOfPossibleBlocks; i++) {
			cout << "Block with index " << i << " is: " << (isFree(i) ? "free" : "busy") << " and "
				<< (isSplit(i) ? "split" : "not split") << endl;
		}
	}
};