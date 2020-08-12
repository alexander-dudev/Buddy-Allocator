#pragma once
#include "Utils.cpp"
#include <iostream>
using namespace std;

class BuddyAllocator {

	// TODO:
	// 1) move all variables inside the block, incl. the forbidden blocks

private:
	void* pointerToBlockBeginning;
	void* pointerToAlignedMemory;
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
		this->pointerToAlignedMemory = pointerToBlockBeginning;
		this->blockSizeInBytes = blockSizeInBytesFromUser;

		if (!Utils::numberIsPowerOfTwo(blockSizeInBytesFromUser)) {
			int closestBiggerBlockSizeWhichIsPowerOfTwo = Utils::findClosestBiggerNumberWhichIsPowerOfTwo(blockSizeInBytesFromUser);
			this->blockSizeInBytes = closestBiggerBlockSizeWhichIsPowerOfTwo;
		}

		int missingBytes = this->blockSizeInBytes - blockSizeInBytesFromUser;
		this->pointerToBlockBeginning = (uint8_t*)pointerToBlockBeginning - missingBytes;

		int bytesInitialAlignement = 0;
		if ((uintptr_t)this->pointerToBlockBeginning % alignof(max_align_t) != 0) {
			bytesInitialAlignement = ((uintptr_t)this->pointerToBlockBeginning % alignof(max_align_t));
			this->pointerToBlockBeginning = (uint8_t*)this->pointerToBlockBeginning - bytesInitialAlignement;
		}

		// free and split tables initialization
		int numberOfBytesUsedByTables = initializeFreeAndSplitTables();
		// at least 2 more integers are required to store the informatio about inaccessible to users blocks
		// and after that the block has to be aligned for the users
		numberOfBytesUsedByTables += bytesInitialAlignement;
		int numberOfBlocksToStoreInnerDataSoFar = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(numberOfBytesUsedByTables + missingBytes);

		// align to store ints !!!
		void* temporaryPointer = this->pointerToAlignedMemory;
		int bytesAfterAddingInaccessibleBlocksData = numberOfBytesUsedByTables;
		bytesAfterAddingInaccessibleBlocksData += missingBytes;
		if ((uintptr_t)temporaryPointer % alignof(int) != 0) {
			int bytesForAlignment = alignof(int)-((uintptr_t)temporaryPointer % alignof(int));
			temporaryPointer = (uint8_t*)temporaryPointer + bytesForAlignment;
			bytesAfterAddingInaccessibleBlocksData += bytesForAlignment;
		}

		void* temp2 = temporaryPointer;

		bytesAfterAddingInaccessibleBlocksData += sizeof(int) + sizeof(int) * numberOfBlocksToStoreInnerDataSoFar;

		
		temporaryPointer = (uint8_t*)temporaryPointer + sizeof(int) + sizeof(int) * numberOfBlocksToStoreInnerDataSoFar;

		int numberOfBlocksAfterAddingInaccessibleBlocksInfo = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesAfterAddingInaccessibleBlocksData);

		while (numberOfBlocksAfterAddingInaccessibleBlocksInfo - numberOfBlocksToStoreInnerDataSoFar > 0) {
			// remove bytes for alignment

			bytesAfterAddingInaccessibleBlocksData += (numberOfBlocksAfterAddingInaccessibleBlocksInfo - numberOfBlocksToStoreInnerDataSoFar) * sizeof(int);

			numberOfBlocksToStoreInnerDataSoFar = numberOfBlocksAfterAddingInaccessibleBlocksInfo;
			numberOfBlocksAfterAddingInaccessibleBlocksInfo = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesAfterAddingInaccessibleBlocksData);
		}




		this->numberOfInaccessibleToUserBlockIndexes = (int*)temp2;
		*(this->numberOfInaccessibleToUserBlockIndexes) = Utils::calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(bytesAfterAddingInaccessibleBlocksData);
		inaccessibleToUserBlockIndexes = this->numberOfInaccessibleToUserBlockIndexes + sizeof(int);
		for (int i = 0; i < *(this->numberOfInaccessibleToUserBlockIndexes); ++i) {
			void* allocatedBlock = allocate(Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES);
			int blockIndex = findBlockIndexFrom(allocatedBlock);
			inaccessibleToUserBlockIndexes[i] = blockIndex;
		}
		
	}

	int initializeFreeAndSplitTables() {

		// in the provided by the user memory the data the allocator needs is ordered as follows:
		// bytes for alignment (for next int) | numberOfPossibleBlocks (int) | requiredBytesForFreeTable (int)
		// numberOfPossibleBlockWithoutLastLevel (int) | requiredBytesForSplitTable (int) (16B)
		// bytes for free table bit field | bytes for split table bit field (18)
		// bytes for alignment (for next int) (20)
		// numberOfInaccessibleToUserBlockIndexes (int) | inaccessibleToUserBlockIndexes (int[])
		// bytes for alignment (for blocks which will be returned to the user)

		int levels = Utils::calculteNumberOfLevelsFor(this->blockSizeInBytes);
		int numberOfPossibleBlocks = Utils::calculateNumberOfPossibleBlocks(levels);
		int numberOfPossibleBlocksWithoutLastLevel = Utils::calculateNumberOfPossibleBlocks(levels - 1);

		int requiredBytesForFreeTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocks);
		int requiredBytesForSplitTable = Utils::calculateNumberOfRequiredBytesFor(numberOfPossibleBlocksWithoutLastLevel);

		int totalBytesUsedForTables = 0;

		if ((uintptr_t)pointerToAlignedMemory % alignof(int) != 0) {
			totalBytesUsedForTables += alignof(int)-((uintptr_t)pointerToAlignedMemory % alignof(int));
			pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + totalBytesUsedForTables;
		}

		// pointerToAlignedMemory is pointing to an aligned address for an int variable

		// number of possible blocks
		this->numberOfPossibleBlocks = (int*)pointerToAlignedMemory;
		*(this->numberOfPossibleBlocks) = numberOfPossibleBlocks;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + sizeof(int);
		totalBytesUsedForTables += sizeof(int);

		// free table size
		this->requiredBytesForFreeTable = (int*)pointerToAlignedMemory;
		*(this->requiredBytesForFreeTable) = requiredBytesForFreeTable;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + sizeof(int);
		totalBytesUsedForTables += sizeof(int);

		// number of possible blocks without last level
		this->numberOfPossibleBlocksWithoutLastLevel = (int*)pointerToAlignedMemory;
		*(this->numberOfPossibleBlocksWithoutLastLevel) = numberOfPossibleBlocksWithoutLastLevel;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + sizeof(int);
		totalBytesUsedForTables += sizeof(int);

		// split table size
		this->requiredBytesForSplitTable = (int*)(pointerToAlignedMemory);
		*(this->requiredBytesForSplitTable) = requiredBytesForSplitTable;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + sizeof(int);
		totalBytesUsedForTables += sizeof(int);

		// free table
		freeTable = (uint8_t*)pointerToAlignedMemory;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + *(this->requiredBytesForFreeTable);
		totalBytesUsedForTables += *(this->requiredBytesForFreeTable);

		// split table
		splitTable = (uint8_t*)pointerToAlignedMemory;
		pointerToAlignedMemory = (uint8_t*)pointerToAlignedMemory + *(this->requiredBytesForSplitTable);
		totalBytesUsedForTables += *(this->requiredBytesForSplitTable);

		for (int i = 0; i < requiredBytesForFreeTable; ++i) {
			this->freeTable[i] = Utils::LARGEST_8_BIT_NUMBER;
		}

		for (int i = 0; i < requiredBytesForSplitTable; ++i) {
			this->splitTable[i] = Utils::SMALLEST_8_BIT_NUMBER;
		}

		return totalBytesUsedForTables;
	}

	int alignBlockIfNecessary(void*& pointerToBlock) {
		int numberOfSkippedBytesForAlignment = 0;

		if ((uintptr_t)pointerToBlock % Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES != 0) {
			numberOfSkippedBytesForAlignment = Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES -((uintptr_t)pointerToBlock % Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES);
			pointerToBlock = (uint8_t*)pointerToBlock + numberOfSkippedBytesForAlignment;
		}

		return numberOfSkippedBytesForAlignment;
	}

	int calculateNumberOfBytesNeededForAlignment(void* pointerToBlock) {
		return alignof(max_align_t)-((uintptr_t)pointerToBlock % alignof(max_align_t));
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
};