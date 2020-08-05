#include <cmath>

class Utils {

private:
	static const int MIN_BLOCK_SIZE_IN_BYTES = 16;

	int blockSizeRepresentedAsPowerOfTwo;
	bool* free;
	bool* split;

public:

	Utils(int sizeInBytes) {
		int numberOfLevels = calculteNumberOfLevels(sizeInBytes);
		int numberOfPossibleBlocks = calculateNumberOfPossibleBlocks(numberOfLevels);
		int numberOfPossibleBlocksWithoutLastLevel = calculateNumberOfPossibleBlocks(numberOfLevels - 1);
		this->blockSizeRepresentedAsPowerOfTwo = calculatePowerOfTwo(sizeInBytes);

		free = new bool[numberOfPossibleBlocks];
		split = new bool[numberOfPossibleBlocksWithoutLastLevel];
		initializeFreeAndSplitArrays();
	}

	~Utils() {
		delete[] free;
		delete[] split;
	}

	void initializeFreeAndSplitArrays(int numberOfPossibleBlocks, int numberOfPossibleBlocksWithoutLastLevel) {
		free[0] = true;
		for (int i = 1; i < numberOfPossibleBlocks; ++i) {
			free[i] = false;
		}

		for (int i = 0; i < numberOfPossibleBlocksWithoutLastLevel; ++i) {
			split[i] = false;
		}
	}

	bool isFree(int index) {
		return free[index];
	}

	bool isSplit(int index) {
		return split[index];
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

	// given pointer -> index
	// we need to be sure that the pointer points inside the block
	// we need either level or block size as well
	// we can find it only using a pointer, but will be more difficult - need to know which blocks are split
	
	int findNodeIndexFrom(void* pointer) {

		return 0;
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

	// given index -> pointer; SHOWN BELOW

	// blokove razstoqnie sprqmo nachaloto
	// po block index -> razstoqnie
	// index -> znaem nivo -> znaem razmer na blok
	// findFirstNodeIndexFor(index) - index -> blokove razstoqnie sprqmo nachaloto
	// ot tam mojem da namerim adresa na bloka i da go vyrnem

	void* findPointerBy(int nodeIndex) {
		int nodeLevel = getLevelBy(nodeIndex);
		int blockSizeForNodeLevel = calculateBlockSizePer(nodeLevel);

		int blockOffsetForNode = nodeIndex - findFirstNodeIndexFor(nodeLevel);
		// calculate pointer
		// pointer = pointerToBeginning + pointerblockOffsetForNode * blockSizeForNodeLevel;

		// return pointer 
		return nullptr;
	}

	int findFirstNodeIndexFor(int level) {
		return pow(2, level) - 1;
	}
	
};