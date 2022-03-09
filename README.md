# Security Analysis of MTE

This repo contains the code and materials from my BluehatIL 2022 talk - "Security Analysis of MTE Through Examples". It includes:

* The [slides](https://github.com/saaramar/security_analysis_mte/blob/main/Security%20Analysis%20of%20MTE%20Through%20Examples.pdf).
* The first basic [example](https://github.com/saaramar/security_analysis_mte/blob/main/first_example/example.c) of MTE.
* The diylist CTF pwn [challenge](https://github.com/saaramar/security_analysis_mte/blob/main/challenge/challenge) with MTE support.
* The three exploits:
  * [base_exploit.py](https://github.com/saaramar/security_analysis_mte/blob/main/challenge/base_exploit.py) - runs the base exploit with 0 awareness for MTE in a loop (works with probabiliy of 1/256)
  * [arb_free_works_exploit.py](https://github.com/saaramar/security_analysis_mte/blob/main/challenge/arb_free_works_exploit.py) - includes the fix of the arbitrary free, runs in a loop (works with probabiliy of 1/16)
  * [deterministic_exploit.py](https://github.com/saaramar/security_analysis_mte/blob/main/challenge/deterministic_exploit.py) - includes the fixes for both the arbitrary free and arbitrary write, runs once (works with probabily 1)

The writeup and exploits for the original challenge without MTE were already published in this [blogpost](https://saaramar.github.io/exploit_pwn_chgs_ubuntu_21.10/). Before reviewing the MTE exploits, it is encouraged to read the blogpost / view the solutions without MTE. 

## General Notes

There are a few important notes mentioned during the talk. Here are some of them:

1. Originally, MTE was designed for bugs detection in production environments at scale. It was not intended to be a memory safety mitigation. In addition, it has a strong restriction: it aims for almost 100% binary compatibility with existing code. Therefore, even though it's not a perfect candidate for memory safety mitigation, it has a very high potential.

2. MTE probabilistically mitigates the most common bug classes (OOBs, UAFs) and deterministically mitigates strictly linear OOBs. See the slides (slide 25) for statistics of vulnerabilities in past years.

3. The demo's goal in this talk was to demonstrate the high impact information disclosures of pointers have on MTE (in any form, memory-based, side channels, etc.). The ability to forge pointers depends on the knowledge of tags. Given this knowledge, probabilistically-mitigated bugs could be deterministically exploited.

4. As explained in the talk - the fact we did not re-tag on free was necessary for a deterministic exploit in this demo because it's a tiny, minimalistic CTF challenge:
   * The only structure we could work with is string (and not even ```std::string```, a simple C ```char*```; just a buffer of characters). Therefore, the ability to leak libc and gain a memory corruption primitive was possible only via dlmalloc's metadata in freed chunks.
   * In real-life, we would have a much wider set of structures, with a much broader set of functionality to use and useful pointers to read (vtables, callbacks, etc.). That's why in real-life, this (exceptionally powerful) 1st order type confusion could be exploited deterministically, even with re-tagging on free.

5. It's clearly unwise to simply take an insecure allocator and enable MTE on top of it. MTE is a very high potential feature, and the properties we can get out of it could be even stronger with a better, more secure allocator.


## Challenge notes

1. The challenge has a (probably unintended) bug, which makes it impossible to use with MTE. The bug is an OOBR in the realloc implementation. Because with MTE we will segfault on that with probabiliy 1/16 (or deterministically if we will implement different tags for adjacent allocations), I've fixed this bug.


   The older code is as follows:

   ```c++
   if (list->size >= list->max) {
       /* Re-allocate a chunk if the list is full */
       Data *old = list->data;
       list->max += CHUNK_SIZE;
       
       list->data = (Data*)malloc(sizeof(Data) * list->max);
       if (list->data == NULL)
         __list_abort("Allocation error");
   
       if (old != NULL) {
         /* Copy and free the old chunk */
         memcpy((char*)list->data, (char*)old, sizeof(Data) * (list->max - 1));
         free(old);
       }
     }
   
   ```

   And keep in mind that:

   ```c++
   #define CHUNK_SIZE 4
   ```

   So, while the newly allocated *list->data* has enough space for the memcpy, the older *list->data* allocation clearly does not, and we end up with an OOBR. I've fixed it to be as follows:

   ```c++
     if (list->size >= list->max) {
       /* Re-allocate a chunk if the list is full */
       Data *old = list->data;
       list->max += CHUNK_SIZE;
       
       list->data = (Data*)my_malloc(sizeof(Data) * list->max);
       if (list->data == NULL)
         __list_abort("Allocation error");
   
       if (old != NULL) {
         /* Copy and free the old chunk */
   
   	    /*This is a bug in the original challenge*/
   	    // my_memcpy((char*)list->data, (char*)old, sizeof(Data) * (list->max - 1));
         my_memcpy((char*)list->data, (char*)old, sizeof(Data) * (list->max - CHUNK_SIZE));
         my_free(old);
       }
     }
   ```


2.  I've built the necessary glibc wrappers (see mte_wrappers.c) for this research ONLY. The implementation is highly unoptimized and should not be used for real-life applications (at all). The goal here was only to gain support for MTE and get it to work 100% correctly, with 0 care for code quality, performance, and maintainability. Also, I've built the bare minimum for the challenge to have 100% support for MTE. Please, under any circumstances, do not use this code for anything besides research.


![image](https://github.com/saaramar/security_analysis_mte/blob/main/demo.png)
