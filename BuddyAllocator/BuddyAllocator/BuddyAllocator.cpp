
class BuddyAllocator {
	
private:
	void* pointerToBlockBeginning;
	int sizeInBytes;

	// offset is needed, so that the allocator can know where its metadata begins (for initial blocks
	// which are not a power of two)

public:

	BuddyAllocator(void* pointerToBlockBeginning, int sizeInBytes) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->sizeInBytes = sizeInBytes;
	}

	void allocate(int blockSizeInBytes) {

	}

	void free(void* pointerToBlock) {

	}


};