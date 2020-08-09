#pragma once
#include <cmath>
#include <cstdint>
class Utils {

private:
	static const int NUMBER_OF_BITS_IN_A_BYTE = 8;
	static const int MIN_BLOCK_SIZE_IN_BYTES = 16;
	const uint8_t maskForBitWithIndexZero = (uint8_t)1;

	int sizeInBytes;
	void* pointerToBlockBeginning;
	int blockSizeRepresentedAsPowerOfTwo;

	bool* free;
	bool* split;
	int numberOfPossibleBlocks;
	int numberOfPossibleBlocksWithoutLastLevel;

	void** arrayContaningListsWithEmptyBlocksForEachLevel;

	uint8_t* freeTable;
	uint8_t* splitTable;
	int requiredBytesForFreeTable;
	int requiredBytesForSplitTable;

public:

	Utils(int sizeInBytes, void* pointerToBlockBeginning) {
		this->sizeInBytes = sizeInBytes;
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		int numberOfLevels = calculteNumberOfLevels(sizeInBytes);
		this->numberOfPossibleBlocks = calculateNumberOfPossibleBlocks(numberOfLevels);
		//int numberOfPossibleBlocksWithoutLastLevel = calculateNumberOfPossibleBlocks(numberOfLevels - 1);
		this->blockSizeRepresentedAsPowerOfTwo = calculatePowerOfTwo(sizeInBytes);

		free = new bool[numberOfPossibleBlocks];
		// numberOfPossibleBlocksWithoutLastLevel can be used in the line below
		split = new bool[numberOfPossibleBlocks];
		initializeFreeAndSplitArrays();

		arrayContaningListsWithEmptyBlocksForEachLevel = new void*[numberOfLevels];
		arrayContaningListsWithEmptyBlocksForEachLevel[0] = pointerToBlockBeginning;
		for (int i = 1; i < numberOfLevels; i++) {
			arrayContaningListsWithEmptyBlocksForEachLevel[i] = nullptr;
		}


		// free and split bit manipulaion
		this->requiredBytesForFreeTable = calculateNumberOfRequiredBytesForTables(numberOfPossibleBlocks);
		// later could be optimized to use less memory
		// int requiredBytesForSplitTable = calculateNumberOfRequiredBytesForTables(numberOfPossibleBlocksWithoutLastLevel);
		this->requiredBytesForSplitTable = calculateNumberOfRequiredBytesForTables(numberOfPossibleBlocks);

		freeTable = new uint8_t[requiredBytesForFreeTable];
		splitTable = new uint8_t[requiredBytesForSplitTable];
		initializeFreeAndSplitTables();
	}

	~Utils() {
		delete[] free;
		delete[] split;
	}

	// big 4 needed

	void initializeFreeAndSplitArrays() {
		for (int i = 0; i < numberOfPossibleBlocks; ++i) {
			free[i] = true;
		}

		// should be changed to numberOfPossibleBlocksWithoutLastLevel later on
		for (int i = 0; i < numberOfPossibleBlocks; ++i) {
			split[i] = false;
		}
	}

	bool isFreeUsingBool(int index) {
		return free[index];
	}

	bool isSplitUsingBool(int index) {
		return split[index];
	}

	void changeFreeStateUsingBool(int index, bool state) {
		free[index] = state;
	}

	void changeSplitStateUsingBool(int index, bool state) {
		split[index] = state;
	}

	void initializeFreeAndSplitTables() {
		for (int i = 0; i < requiredBytesForFreeTable; ++i) {
			this->freeTable[i] = 255;
		}

		for (int i = 0; i < requiredBytesForSplitTable; ++i) {
			this->splitTable[i] = 0;
		}
	}

	int findByteIndexFor(int blockIndex) {
		return blockIndex / NUMBER_OF_BITS_IN_A_BYTE;
	}

	int createMaskFor(int blockIndex) {
		int bitIndexInsideSingleByte = blockIndex % NUMBER_OF_BITS_IN_A_BYTE;
		return maskForBitWithIndexZero << bitIndexInsideSingleByte;
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

	int getNumberOfBlocksPer(int level) {
		return pow(2, level);
	}

	int calculteNumberOfLevels(int sizeInBytes) {
		int numberOfLevels = 1;
		while (sizeInBytes > MIN_BLOCK_SIZE_IN_BYTES) {
			sizeInBytes /= 2;
			numberOfLevels++;
		}
		
		return numberOfLevels;

		//return blockSizeRepresentedAsPowerOfTwo - calculatePowerOfTwo(MIN_BLOCK_SIZE_IN_BYTES) + 1;
	}

	int calculateNumberOfPossibleBlocks(int numberOfLevels) {
		int numberOfPossibleBlocks = 0;
		for (int levelIndex = 0; levelIndex < numberOfLevels; ++levelIndex) {
			numberOfPossibleBlocks += getNumberOfBlocksPer(levelIndex);
		}

		return numberOfPossibleBlocks;
	}

	int calculateBlockSizePer(int level) {
		return pow(2, blockSizeRepresentedAsPowerOfTwo - level);
	}

	int calculatePowerOfTwo(int number) {
		int powerOfTwo = 0;

		while (pow(2, powerOfTwo) < number) {
			powerOfTwo++;
		}
		return powerOfTwo;
	}

	int getLeftChildIndex(int parentIndex) {
		return 2 * parentIndex + 1;
	}

	int getRightChildIndex(int parentIndex) {
		return 2 * parentIndex + 2;
	}

	int getParentIndex(int childIndex) {
		return (childIndex - 1) / 2;
	}

	int getBuddyIndex(int nodeIndex) {
		return nodeIndex % 2 == 0 ? nodeIndex - 1 : nodeIndex + 1;
	}

	// knowing the level, we can find the block's size
	int getLevelBy(int blockIndex) {
		double binaryLogarithm = log2(blockIndex);
		return numberIsPowerOfTwo(blockIndex + 1) ? ceil(binaryLogarithm) : floor(binaryLogarithm);
	}

	// list of node indexes by levelIndex

	// tochna stepen na 2-kata -> 2^i
	// vlqvo ot neq ima tochno 1 element -> index (2^i) - 1
	// na samoto nivo ima tochno 2^i elementa

	bool numberIsPowerOfTwo(int number) {
		int power = 0;

		while (pow(2, power) < number) {
			power++;
		}

		return pow(2, power) == number;
	}



	int findFirstNodeIndexFor(int level) {
		return pow(2, level) - 1;
	}

	int findBlockIndexFrom(void* pointer) {
		return findBlockIndexFrom(pointer, 0, sizeInBytes);
	}

	// given pointer -> index
	// we need to be sure that the pointer points inside the block
	// we need either level or block size as well
	// we can find it only using a pointer, but will be more difficult - need to know which blocks are split
	// initial value for blockIndexForCurrentLevel : 0
	//                   blockSize = whole initial block size
	int findBlockIndexFrom(void* pointer, int blockIndexForCurrentLevel, int blockSizeForCurrentLevel) {
		if (pointer == pointerToBlockBeginning && !isSplitUsingBool(blockIndexForCurrentLevel)) {
			return blockIndexForCurrentLevel;
		}

		if (pointer < (char*)pointerToBlockBeginning + (blockSizeForCurrentLevel / 2)) {
			// moving to the left child
			return findBlockIndexFrom(pointer, getLeftChildIndex(blockIndexForCurrentLevel), blockSizeForCurrentLevel / 2);
		}
		else {
			// moving to the right child
			return findBlockIndexFrom(pointer, getRightChildIndex(blockIndexForCurrentLevel), blockSizeForCurrentLevel / 2);
		}


	}


	// given a nodeIndex -> return a pointer to the node

	// blokove razstoqnie sprqmo nachaloto
	// 1) blockIndex -> distance
	// index -> znaem nivo -> znaem razmer na blok
	// findFirstNodeIndexFor(index) - index -> blokove razstoqnie sprqmo nachaloto
	// ot tam mojem da namerim adresa na bloka i da go vyrnem

	void* findPointerBy(int nodeIndex) {
		int nodeLevel = getLevelBy(nodeIndex);
		int blockSizeForNodeLevel = calculateBlockSizePer(nodeLevel);
		int numberOfBlocksOffsetForNode = nodeIndex - findFirstNodeIndexFor(nodeLevel);

		return (char*)pointerToBlockBeginning + (numberOfBlocksOffsetForNode * blockSizeForNodeLevel);
	}

	int findClosestBiggerNumberWhichIsPowerOfTwo(int number) {
		return numberIsPowerOfTwo(number) ? number : findClosestBiggerNumberWhichIsPowerOfTwo(number + 1);
	}

	int calculateNumberOfRequiredBytesForTables(int numberOfPossibleBlocks) {
		return numberOfPossibleBlocks % 8 == 0 ? numberOfPossibleBlocks / 8 : numberOfPossibleBlocks / 8 + 1;
	}
	
};