#include "mte_wrappers.h"

int disable_mte() {
	/*
	* Disable MTE entirely. Keep tagged address ABI on.
	*/
	if (prctl(PR_SET_TAGGED_ADDR_CTRL,
			PR_TAGGED_ADDR_ENABLE  | PR_MTE_TCF_NONE |
			(0xfffe << PR_MTE_TAG_SHIFT),
			0, 0, 0)) {
		perror("prctl() failed");
		return 1;
	}

	return 0;
}

int enable_mte() {
	/*
	* Enable the tagged address ABI, synchronous MTE
	* tag check faults (based on per-CPU preference) and allow all
	* non-zero tags in the randomly generated set.
	*/
	if (prctl(PR_SET_TAGGED_ADDR_CTRL,
			PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC |
			(0xfffe << PR_MTE_TAG_SHIFT),
			0, 0, 0)) {
		perror("prctl() failed");
		return 1;
	}

	return 0;

}

size_t my_strlen(char *s) {
	size_t len = 0;
	while (*s != '\x00') {
		++s;
		++len;
	}
	return len;
}

void my_memcpy(char *dst, char *src, size_t len) {
	for (size_t i = 0; i < len; ++i) {
		dst[i] = src[i];
	}
}

char *my_strdup(char *s) {
	size_t len = my_strlen(s);
	char *str = my_malloc(len + 1);
	my_memcpy(str, s, len);
	str[len] = '\x00';
	return str;
}

size_t roundup(size_t size) {
	if (size % LINE_SIZE) {
		size = size + LINE_SIZE - (size % LINE_SIZE);
	}

	if (size % LINE_SIZE != 0x0) {
		printf("ERROR alignment");
		return 0;
	}

	return size;
}

void *my_malloc(size_t size) {
	char *p = NULL;

	disable_mte();

	/*
       * That's ugly, but it's fine for this research.
       *
       * The problem is that with dlmalloc, the malloc_usable_sizes are 0x18, 0x28, 0x38, etc.
       * And, the size in the header has an alignment of +0x8. That means, that we have two possible cases:
       * 	- we DO NOT tag the last 8 bytes of the allocation
       *	- we DO tag the last 8 bytes, but we ALSO tag the next allocation's size
       *
       * Both cases are fine for this POC's correctness because we ALWAYS have the property of:
	   * 	"the user asked for N bytes of memory, and these N bytes are tagged".
       * 
       * And it works, because we run the allocator functionality with MTE disabled.
       *
       * In other words: for dlmalloc specifically, with the header's alignment of +0x8, we're guaranteed
       * that any 0x10-byte granule we touch is either all ours or shared just with a header (either before
       * our allocation or after us with padding). 
       *
	 */
	p = (char*)malloc(size);
	size = roundup(size);

	/* 
	*  This is highly unoptimized.
	*  Always set the underlying memory as tagged memory.
	*  Take care of allocations that cross page boundary.
	*/
	char *base = (char*)((unsigned long long)p & ~0xfff);

	/*
	 * get number of pages the new allocation covers
	 */
	int num_pages = max(1, size / PAGE_SIZE);
	if ( ((unsigned long long)(p + size) & ~0xfff) > (unsigned long long)base) {
		num_pages++;
	}
	if (mprotect(base , PAGE_SIZE * num_pages, PROT_READ | PROT_WRITE | PROT_MTE)) {
		perror("mprotect");
		exit(1);
	}

	enable_mte();

	p = (char*)insert_random_tag(p);
	char *pChunk = p;
	for (size_t cl = 0; cl < size / LINE_SIZE; ++cl) {
		set_tag(pChunk);
		pChunk += LINE_SIZE;
	}

	return (void*)p;
}

void my_free(void *p) {
	if(p) {
		/*
		 * do a load, in sync-mode this will trigger a memory tagging violation
		 * if p has an incorrect tag
		 */
		int val = *(int*)p;
		disable_mte();

		p = (void*)((unsigned long long)p & 0x00ffffffffffffff);
		free(p);

		enable_mte();
	}
}
