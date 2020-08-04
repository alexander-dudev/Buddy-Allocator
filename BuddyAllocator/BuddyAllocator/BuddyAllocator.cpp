
class BuddyAllocator {
	
private:
	void* pointerToBlockBeginning;
	int sizeInBytes;

public:

	BuddyAllocator(void* pointerToBlockBeginning, int sizeInBytes) {
		this->pointerToBlockBeginning = pointerToBlockBeginning;
		this->sizeInBytes = sizeInBytes;
	}
};