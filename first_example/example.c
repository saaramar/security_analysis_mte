/*
 * To be compiled with -march=armv8.5-a+memtag
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>

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
        uint64_t __val;                                 \
        asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
        __val;                                          \
})

/*
 * Set the allocation tag on the destination address.
 */
#define set_tag(tagged_addr) do {                                      \
        asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
} while (0)

int main(void) {
        unsigned char *ptr;
        unsigned long page_sz = sysconf(_SC_PAGESIZE);
        unsigned long hwcap2 = getauxval(AT_HWCAP2);

        /* check if MTE is present */
        if (!(hwcap2 & HWCAP2_MTE)) {
            printf("MTE not supported\n");
            return EXIT_FAILURE;
        }
                
        /*
         * Enable the tagged address ABI, synchronous or asynchronous MTE
         * tag check faults (based on per-CPU preference) and allow all
         * non-zero tags in the randomly generated set.
         */
        if (prctl(PR_SET_TAGGED_ADDR_CTRL,
                  PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC |
                  (0xfffe << PR_MTE_TAG_SHIFT),
                  0, 0, 0)) {
                perror("prctl() failed");
                return EXIT_FAILURE;
        }

        ptr = mmap(NULL, page_sz, PROT_READ | PROT_WRITE | PROT_MTE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
                perror("mmap() failed");
                return EXIT_FAILURE;
        }

        /* access with the default tag (0) */
        ptr[0] = 0x41;
        ptr[1] = 0x42;

        printf("ptr[0] = 0x%hhx ptr[1] = 0x%hhx\n", ptr[0], ptr[1]);

        /* set the logical and allocation tags */
        ptr = (unsigned char *)insert_random_tag(ptr);
        set_tag(ptr);

        printf("ptr == %p\n", ptr);

        /* non-zero tag access */
        ptr[0] = 0x43;
        printf("ptr[0] = 0x%hhx ptr[1] = 0x%hhx\n", ptr[0], ptr[1]);

        /*
         * If MTE is enabled correctly the next instruction will generate an
         * exception.
         */
        printf("Expecting SIGSEGV...\n");
        ptr[0x10] = 0x44;

        /* this should not be printed in the PR_MTE_TCF_SYNC mode */
        printf("...haven't got one\n");

        return EXIT_FAILURE;
}
