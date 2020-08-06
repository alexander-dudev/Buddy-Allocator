#include "Utils.cpp"
class BuddyAllocator {
	
private:
	Utils utils;
	void* pointerToBlockBeginning;
	int sizeInBytes;
	// offset is needed, so that the allocator can know where its metadata begins (for initial blocks
	// which are not a power of two)

public:

	BuddyAllocator(void* pointerToBlockBeginning, int sizeInBytes) : utils(sizeInBytes, pointerToBlockBeginning) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->sizeInBytes = sizeInBytes;
	}

	void* allocateUsingTree(int blockSizeInBytes, int currentBlockIndex, int blockSizeForCurrentLevel) {
		// assuming blockSizeInBytes is a power of two; the caller will find the closestbiggerPowerOfTwo
		// as this is a helper recursive function

		// free
		if (!utils.isSplit(currentBlockIndex) && utils.isFree(currentBlockIndex) && blockSizeInBytes == blockSizeForCurrentLevel) {
			utils.changeFreeState(currentBlockIndex, false);
			return utils.findPointerBy(currentBlockIndex);
		}

		// free but with larger size
		if (!utils.isSplit && utils.isFree(currentBlockIndex) && blockSizeInBytes < blockSizeForCurrentLevel) {
			utils.changeSplitState(currentBlockIndex, true);
			allocateUsingTree(blockSizeInBytes, utils.getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
		}

		// busy
		if (!utils.isSplit && !utils.isFree(currentBlockIndex)) {
			return nullptr;
		}

		// searchig recursively from the left branch
		void* allocatedBlockFromLeftBranch = allocateUsingTree(blockSizeInBytes, utils.getLeftChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);

		return allocatedBlockFromLeftBranch != nullptr ? allocatedBlockFromLeftBranch : allocateUsingTree(blockSizeInBytes, utils.getRightChildIndex(currentBlockIndex), blockSizeForCurrentLevel / 2);
	}

	bool freeUsingTree(void* pointerToBlock) {
		int blockIndex = utils.findBlockIndexFrom(pointerToBlock);

		if (!utils.isFree(blockIndex)) {
			utils.changeFreeState(blockIndex, true);
			mergeBlocks(utils.getParentIndex(blockIndex));
			return true;
		} else {
			return false;
		}
	}

	void mergeBlocks(int parentBlock) {
		if (!utils.isFree(utils.getLeftChildIndex(parentBlock)) || !utils.isFree(utils.getRightChildIndex(parentBlock))) {
			return;
		}

		utils.changeSplitState(parentBlock, false);
		utils.changeFreeState(parentBlock, true);

		mergeBlocks(utils.getParentIndex(parentBlock));
	}




};