

#ifndef HW4_MALLOC_3_H
#define HW4_MALLOC_3_H

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define MAX 100000000
#define META_SIZE sizeof(MallocMetadata)
#define BYTES_TO_SPLIT (128)
#define MMAP_LIM 1024000 // 128kb
#define HIST_SIZE (128)
#define KILO_BYTE 8000 // kilo byte


struct MallocMetadata {
    size_t alloc_size;
    size_t total_size;
    bool available;
    MallocMetadata *next;
    MallocMetadata *prev;
    MallocMetadata *hist_next;
    MallocMetadata *hist_prev;
};
enum MergeType {
    NONE, TOP, BUTTOM, BOTH, TAIL
};

void *smalloc(size_t size);

void *scalloc(size_t num, size_t size);

void sfree(void *p);

void *srealloc(void *oldp, size_t size);

MallocMetadata *GetFirstAvailable(size_t size);

MallocMetadata *GetTailInHist();

MallocMetadata *GetTail();

void addToList(MallocMetadata *element);

int findIndexInHist(MallocMetadata *block);

void removeFromHist(MallocMetadata *element);

MallocMetadata *GetTailMmap();

void addToListMmap(MallocMetadata *element);

void nodeSwap(MallocMetadata *left, MallocMetadata *right);

void sortList(MallocMetadata *start, int index);

MallocMetadata *GetLocalTailByHistIndex(int index);

////sort the list after adding
void addToListInHist(MallocMetadata *element, int index);

/*
 * splite the blcok
 */
MallocMetadata *advanced_malloc_cutter(size_t size, MallocMetadata *dest);

//return true if enlarged and if true the tail is  enlarged
////might fixed the mmap prob because we might forgot this one, but not sure,
////because if unmmaped its not nessacery
void advanced_malloc_enlarge_last_block(MallocMetadata *tail, size_t size);

/*
 * only adds the size and update the regular list and not hist
 */
MallocMetadata *advanced_malloc_merge(MallocMetadata *bottom, MallocMetadata
*upper);


MallocMetadata *adjacentBlocksCheck(MallocMetadata *curr, size_t size,
                                    MergeType *type);

//return true if enlarged and if true the tail is  enlarged, works only with
// sbark
bool advanced_malloc_enlarge_last_block_from_realloc(MallocMetadata *tail,
                                                     size_t size);

size_t _num_free_blocks();

size_t _num_free_bytes();

size_t _num_allocated_blocks();

size_t _num_allocated_bytes();

size_t _num_meta_data_bytes();

size_t _size_meta_data();

#endif //HW4_MALLOC_3_H
