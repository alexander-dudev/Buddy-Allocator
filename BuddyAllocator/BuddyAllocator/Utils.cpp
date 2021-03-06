#pragma once
#include <cmath>
#include <cstdint>
class Utils {

public:

	static const int NUMBER_OF_BITS_IN_A_BYTE = 8;
	static const int MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES = 16;
	static const uint8_t MASK_FOR_BIT_WITH_INDEX_ZERO = (uint8_t)1;
	static const int INVALID_BLOCK_INDEX = -1;
	static const uint8_t LARGEST_8_BIT_NUMBER = (uint8_t)255;
	static const uint8_t SMALLEST_8_BIT_NUMBER = (uint8_t)0;

	static int createMaskFor(int bitIndexInsideSingleByte) {
		return Utils::MASK_FOR_BIT_WITH_INDEX_ZERO << bitIndexInsideSingleByte;
	}

	// pow(2,i) == 1 << i
	static int calculateNumberOfBlocksPer(int level) {
		//return pow(2, level);
		return 1 << level;
	}

	static int calculateNumberOfLevelsFor(int blockSizeInBytes) {
		int numberOfLevels = 1;
		while (blockSizeInBytes > MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES) {
			blockSizeInBytes /= 2;
			numberOfLevels++;
		}
		
		return numberOfLevels;

		// or
		//return calculatePowerOfTwo(blockSizeInBytes) - calculatePowerOfTwo(MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES) + 1;
	}

	static int calculateNumberOfPossibleBlocks(int numberOfLevels) {
		int numberOfPossibleBlocks = 0;
		for (int levelIndex = 0; levelIndex < numberOfLevels; ++levelIndex) {
			numberOfPossibleBlocks += calculateNumberOfBlocksPer(levelIndex);
		}

		return numberOfPossibleBlocks;
	}

	static int calculateBlockSizePer(int levelIndex, int initialBlockSizeInBytes) {
		int sizeRepresentedAsPowerOfTwo = calculatePowerOfTwo(initialBlockSizeInBytes);
		//return pow(2, sizeRepresentedAsPowerOfTwo - levelIndex);
		return 1 << (sizeRepresentedAsPowerOfTwo - levelIndex);
	}

	static int calculatePowerOfTwo(int number) {
		if (!numberIsPowerOfTwo(number)) {
			return -1;
		}

		int powerOfTwo = 0;
		//while (pow(2, powerOfTwo) < number) {
		while ((1 << powerOfTwo) < number) {
			powerOfTwo++;
		}

		return powerOfTwo;
	}

	static bool numberIsPowerOfTwo(int number) {
		int power = 0;

		//while (pow(2, power) < number) {
		while ((1 << power) < number) {
			power++;
		}

		//return pow(2, power) == number;
		return (1 << power) == number;
	}

	static int getLeftChildIndex(int parentIndex) {
		return 2 * parentIndex + 1;
	}

	static int getRightChildIndex(int parentIndex) {
		return 2 * parentIndex + 2;
	}

	static int getParentIndex(int childIndex) {
		return (childIndex - 1) / 2;
	}

	static int getBuddyIndex(int nodeIndex) {
		return nodeIndex % 2 == 0 ? nodeIndex - 1 : nodeIndex + 1;
	}

	// knowing the level, we can find the block's size
	static int getLevelBy(int blockIndex) {
		double binaryLogarithm = log2(blockIndex);
		return numberIsPowerOfTwo(blockIndex + 1) ? ceil(binaryLogarithm) : floor(binaryLogarithm);
	}

	// the first block index for a level "i" is: (2^i) - 1
	// on each level there are exactly 2^i blocks

	static int findFirstNodeIndexFor(int level) {
		//return pow(2, level) - 1;
		return (1 << level) - 1;
	}

	static int findClosestBiggerNumberWhichIsPowerOfTwo(int number) {
		return numberIsPowerOfTwo(number) ? number : findClosestBiggerNumberWhichIsPowerOfTwo(number + 1);
	}

	static int calculateNumberOfRequiredBytesFor(int numberOfBits) {
		return numberOfBits % 8 == 0 ? numberOfBits / 8 : numberOfBits / 8 + 1;
	}

	static int calculateNumberOfRequiredBlocksWithTheMinimumSizeToStore(int sizeInBytes) {
		if (sizeInBytes % Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES == 0) {
			return sizeInBytes / Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES;
		}
		else {
			return (sizeInBytes / Utils::MIN_ALLOCATED_BLOCK_SIZE_IN_BYTES) + 1;
		}

	}

	static int findByteIndexFor(int blockIndex) {
		return blockIndex / Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}

	static int findBitIndexInsideSingleByteFor(int blockIndex) {
		return blockIndex % Utils::NUMBER_OF_BITS_IN_A_BYTE;
	}
};