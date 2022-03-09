#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include <malloc.h>

/*
 * From arch/arm64/include/uapi/asm/hwcap.h
 */
#define HWCAP2_MTE              (1 << 18)

/*
 * From arch/arm64/include/uapi/asm/mman.h
 */
#define PROT_MTE                 0x20

/*
 * From include/uapi/linux/prctl.h
 */
#define PR_SET_TAGGED_ADDR_CTRL 55
#define PR_GET_TAGGED_ADDR_CTRL 56
# define PR_TAGGED_ADDR_ENABLE  (1UL << 0)
# define PR_MTE_TCF_SHIFT       1
# define PR_MTE_TCF_NONE        (0UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_SYNC        (1UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_ASYNC       (2UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TCF_MASK        (3UL << PR_MTE_TCF_SHIFT)
# define PR_MTE_TAG_SHIFT       3
# define PR_MTE_TAG_MASK        (0xffffUL << PR_MTE_TAG_SHIFT)

/*
 * Insert a random logical tag into the given pointer.
 */
#define insert_random_tag(ptr) ({                       \
        uint64_t __val;					\
	asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})


/*
 * Set the allocation tag on the destination address.
 */
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

#define PAGE_SIZE 0x1000
#define LINE_SIZE 0x10

void my_memcpy(char *dst, char *src, size_t len);
size_t my_strlen(char *s);
char *my_strdup(char *s);
int disable_mte();
int enable_mte();
size_t roundup(size_t size);
void *my_malloc(size_t size);
void my_free(void *p);

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
