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


// Our global pointer to the list that contains all the data sectors
MallocMetadata *regular_list_head = nullptr;
MallocMetadata *mmap_list_head = nullptr;
MallocMetadata *hist[HIST_SIZE] = {nullptr};


void *smalloc(size_t size);

void *scalloc(size_t num, size_t size);

void sfree(void *p);

void *srealloc(void *oldp, size_t size);

////updated
MallocMetadata *GetFirstAvailable(size_t size) {
    MallocMetadata *curr = nullptr;
    int listindex = size / KILO_BYTE;
    for (int i = listindex; i < HIST_SIZE; i++) {
        curr = hist[i];
        while (curr) {
            if (curr->alloc_size >= size && curr->available) {
                return curr;
            }
            curr = curr->hist_next;
        }
    }
    return curr;
}

MallocMetadata *GetTailInHist() {
    MallocMetadata *tail = nullptr;
    int populated_index;
    for (int i = HIST_SIZE - 1; i > -1; i--) {
        if (hist[i]) {
            populated_index = i;
            break;
        }
    }
    MallocMetadata *curr = hist[populated_index];
    while (curr) {
        tail = curr;
        curr = curr->hist_next;
    }
    return tail;
}

MallocMetadata *GetTail() {
    MallocMetadata *curr = regular_list_head;
    MallocMetadata *tail = nullptr;
    while (curr) {
        tail = curr;
        curr = curr->next;
    }
    return tail;
}

void addToList(MallocMetadata *element) {
    if (regular_list_head == NULL) {
        regular_list_head = element;
        return;
    }
    MallocMetadata *tail = GetTail();
    tail->next = element;
    element->prev = tail;
}

int findIndexInHist(MallocMetadata *block) {
    MallocMetadata *it = block;
    MallocMetadata *head = nullptr;
    while (it) {
        head = it;
        it = it->hist_prev;
    }
    for (int i = 0; i < HIST_SIZE; i++) {
        if (hist[i] == head) {
            return i;
        }
    }
    return -1;
}

void removeFromHist(MallocMetadata *element) {
    if (!element)
        return;
    MallocMetadata *prev, *next;
    prev = element->hist_prev;
    next = element->hist_next;
    if (!prev) {
        int index = findIndexInHist(prev);
        hist[index] = next;
    } else {
        prev->hist_next = next;
    }
    if (next)
        next->hist_prev = prev;
    element->available = false;
    element->hist_prev = nullptr;
    element->hist_next = nullptr;
    return;
}

/*
MallocMetadata *GetFirstAvailableMmap(size_t size) {
    MallocMetadata *curr = mmap_list_head;
    while (curr) {
        if (curr->alloc_size >= size && curr->available) {
            return curr;
        }
        curr = curr->next;
    }
    return nullptr;
}
 */

MallocMetadata *GetTailMmap() {
    MallocMetadata *curr = mmap_list_head;
    MallocMetadata *tail = nullptr;
    while (curr) {
        tail = curr;
        curr = curr->next;
    }
    return tail;
}

void addToListMmap(MallocMetadata *element) {
    if (mmap_list_head == NULL) {
        mmap_list_head = element;
        return;
    }
    MallocMetadata *tail = GetTailMmap();
    tail->next = element;
    element->prev = tail;
}

void nodeSwap(MallocMetadata *left, MallocMetadata *right) {
    MallocMetadata *a = left->hist_prev;
    MallocMetadata *b = right->hist_next;
    a->hist_next = right;
    right->hist_prev = a;
    left->hist_prev = right;
    left->hist_next = b;
    right->hist_next = left;
    b->hist_prev = left;
}

void sortList(MallocMetadata *start, int index) {
    int swapped, i;
    MallocMetadata *ptr1;
    MallocMetadata *lptr = NULL;
    /* Checking for empty list */
    if (start == NULL)
        return;
    do {
        swapped = 0;
        ptr1 = start;
        while (ptr1->hist_next != lptr) {
            //asuming this is sorted by total_size not aloc_size
            if ((ptr1->alloc_size) > ((ptr1->hist_next)->alloc_size)) {
                nodeSwap(ptr1, ptr1->hist_next);
                swapped = 1;
            }
            ptr1 = ptr1->hist_next;
        }
        lptr = ptr1;
    } while (swapped);


    MallocMetadata *temp=start;
    MallocMetadata *head= nullptr;
    while (temp) {
        head = temp;
        temp = temp->hist_prev;
    }
    hist[index]=head;
}

MallocMetadata *GetLocalTailByHistIndex(int index) {
    MallocMetadata *curr = hist[index];
    MallocMetadata *tail = nullptr;
    while (curr) {
        tail = curr;
        curr = curr->hist_next;
    }
    return tail;
}

////sort the list after adding
void addToListInHist(MallocMetadata *element, int index) {
    if (hist[index] == nullptr) {
        hist[index] = element;
        return;
    }
    MallocMetadata *tail = GetLocalTailByHistIndex(index);
    tail->hist_next = element;
    element->hist_prev = tail;
    ////is the head updated here?
    sortList(hist[index],index);
}

/*
 * splite the blcok
 */
MallocMetadata *advanced_malloc_cutter(size_t size, MallocMetadata *dest) {
    long remain_space_check = dest->alloc_size - size - META_SIZE;
    if (remain_space_check < BYTES_TO_SPLIT) {
        return dest;
    }
    // we have to split
    size_t remain_space = remain_space_check;
    void *new_free_block = (void *) (((char *) (dest + 1)) + size);
    MallocMetadata *new_element = (MallocMetadata *) new_free_block;
    *new_element = (MallocMetadata) {remain_space,
                                     remain_space + META_SIZE,
                                     true, dest->next, dest, nullptr, nullptr};
    dest->next = new_element;
    dest->alloc_size = size;
    dest->total_size = size + META_SIZE;
    if (new_element->next) {
        (new_element->next)->prev = new_element;
    }
    sfree(new_element + 1);
    return dest;
}

//return true if enlarged and if true the tail is  enlarged
////might fixed the mmap prob because we might forgot this one, but not sure,
////because if unmmaped its not nessacery
void advanced_malloc_enlarge_last_block(MallocMetadata *tail, size_t size) {
    if (!tail->available)
        return;
    if (size < tail->alloc_size)
        return;
    size_t add_space = size - tail->alloc_size;
    void *ptr;
    ptr = sbrk(add_space);
    if (ptr == (void *) (-1)) {
        return;
    }
    tail->alloc_size += add_space;
    tail->total_size += add_space;
    return;
}

void *smalloc(size_t size) {
    if (size == 0 || size > MAX) {
        return NULL;
    }
    void *ptr;
    MallocMetadata *exist = nullptr;
    ////if we are not mmap
    if (size < MMAP_LIM) {
        exist = GetFirstAvailable(size);
        if (exist) {
            exist = advanced_malloc_cutter(size, exist);
            exist->available = false;
            removeFromHist(exist);
            ptr = exist;
        } else {
            //Didn't find any free block with size bytes, first check if we
            // can enlarge the tail, if not, make new
            MallocMetadata *tail = GetTail();
            MallocMetadata *lastfreeblock = GetTailInHist();
            if (lastfreeblock == tail) {
                ////may solve the mmap probs, need to check if it works fine
                advanced_malloc_enlarge_last_block(tail, size);
                removeFromHist(tail);
                tail->available = false;
                ptr = tail;
            } else {
                ptr = sbrk(size + META_SIZE);
                if (ptr == (void *) (-1)) {
                    return NULL;
                }
            }
        }
    } else { // Creating new block
        ptr = mmap(NULL, size + META_SIZE, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (ptr == (void *) (-1)) {
            return NULL;
        }
    }
    auto *new_meta = (MallocMetadata *) ptr;
    *new_meta = (MallocMetadata) {size, size + META_SIZE, false, NULL,
                                  NULL, NULL, NULL};
    if (size >= MMAP_LIM) {
        addToListMmap(new_meta);
    } else {
        addToList(new_meta);
    }
    ptr = new_meta;
    ptr = (MallocMetadata *) ptr + 1;
    return ptr;
}

void *scalloc(size_t num, size_t size) {
    if (size == 0 || size * num > MAX) {
        return NULL;
    }
    void *ptr = smalloc(num * size);
    if (!ptr)
        return NULL;
    memset(ptr, 0, num * size);//not sure if size or num*size
    return ptr;
}

/*
 * only adds the size and update the regular list and not hist
 */
MallocMetadata *advanced_malloc_merge(MallocMetadata *bottom, MallocMetadata
*upper) {
    bottom->alloc_size = bottom->alloc_size + upper->total_size;
    bottom->total_size += upper->total_size;
    bottom->next = upper->next;
    if (upper->next) {
        upper->next->prev = bottom;
    }
    if (bottom->prev == NULL) {
        regular_list_head = bottom;
    }
    return bottom;
}


void sfree(void *p) {
    if (!p)
        return;
    MallocMetadata *curr = (MallocMetadata *) p - 1;
    if (curr->available)
        return;
    if (curr->alloc_size < MMAP_LIM) {
        if (curr->prev && curr->prev->available) {
            removeFromHist(curr->prev);
            removeFromHist(curr);
            curr = advanced_malloc_merge(curr->prev, curr);
        }
        if (curr->next && curr->next->available) {
            removeFromHist(curr);
            removeFromHist(curr->next);
            curr = advanced_malloc_merge(curr, curr->next);
        }
        curr->available = true;
        ////not sure about the total size ot total size-meta size
        addToListInHist(curr, (curr->total_size) / (KILO_BYTE));
        return;
    } else {
        if (curr->prev) {
            (curr->prev)->next = curr->next;
        } else { // curr->prev=nullptr When we delete the head of the list
            mmap_list_head = curr->next;
        }
        if (curr->next) {
            (curr->next)->prev = curr->prev;
        }
        munmap(curr, curr->total_size);
        return;
    }
}


MallocMetadata *adjacentBlocksCheck(MallocMetadata *curr, size_t size,
                                    MergeType *type) {
    size_t with_lower, with_upper, with_both;
    if (curr->prev && curr->prev->available) {
        *type = BUTTOM;
        with_lower = curr->prev->alloc_size + curr->total_size;
        if (with_lower >= size) {
            // Merge with lower
            curr = advanced_malloc_merge(curr->prev, curr);
            return curr;
        } else if (curr == GetTail()) {//You are tail and your prev is available
            //*type=TAIL;
            size_t prev_size = curr->alloc_size;
            advanced_malloc_enlarge_last_block(curr,
                                               size - curr->prev->alloc_size);
            if (prev_size != curr->alloc_size) {
                curr = advanced_malloc_merge(curr->prev, curr);
                return curr;
            }
        }
    }
    if (curr->next && curr->next->available) {
        *type = TOP;
        with_upper = curr->alloc_size + curr->next->total_size;
        if (with_upper >= size) {
            //Merge with upper
            curr = advanced_malloc_merge(curr, curr->next);
            return curr;
        }
    }
    if (curr->prev && curr->prev->available &&
        curr->next && curr->next->available) {
        *type = BOTH;
        with_both = curr->prev->alloc_size + curr->total_size +
                    curr->next->total_size;
        if (with_both >= size) {
            //Merge with both
            curr = advanced_malloc_merge(curr->prev, curr);
            curr = advanced_malloc_merge(curr, curr->next);
            return curr;
        }
    }
    return nullptr;
}

//return true if enlarged and if true the tail is  enlarged, works only with
// sbark
bool advanced_malloc_enlarge_last_block_from_realloc(MallocMetadata *tail,
                                                     size_t size) {
    size_t add_space = size - tail->alloc_size;
    void *ptr = sbrk(add_space);
    if (ptr == (void *) (-1)) {
        return false;
    }
    tail->alloc_size += add_space;
    tail->total_size += add_space;
    return true;
}

void *srealloc(void *oldp, size_t size) {
    if (size == 0 || size > MAX) {
        return NULL;
    }
    if (!oldp)
        return smalloc(size);
    MallocMetadata *curr = ((MallocMetadata *) oldp - 1);
    size_t requested_size = curr->alloc_size;
    if (size < MMAP_LIM && requested_size < MMAP_LIM) {
        if (size <= curr->alloc_size) {
            if (advanced_malloc_cutter(size, curr) == curr)
                return oldp; //Using our current block: oldp
        }
        MallocMetadata *old_prev = curr->prev;
        MallocMetadata *old_next = curr->next;
        MallocMetadata *old_curr = curr;
        MergeType type = NONE;
        curr = adjacentBlocksCheck(curr, size, &type);
        if (curr) { //Trying to merge with my neighbors
            ////might be problems with curr after merge
            curr->available = false;
            if (type == BUTTOM) {
                removeFromHist(old_prev);
                removeFromHist(curr);
            }
            if (type == TOP) {
                removeFromHist(curr);
                removeFromHist(old_next);
            }
            if (type == BOTH) {
                removeFromHist(old_prev);
                removeFromHist(curr);
                removeFromHist(old_next);
            }
            memcpy(curr + 1, oldp, requested_size);
            curr = advanced_malloc_cutter(size,
                                          curr); //Checking if split is possible
            return curr + 1;
        }
        curr = ((MallocMetadata *) oldp - 1);
        if (curr != GetTail()) {
            void *ptr = smalloc(
                    size); // find a big block or allocate a new block
            if (!ptr)
                return NULL;
            memcpy(ptr, oldp, requested_size);
            sfree(oldp);
            return ptr;
        } else { // curr==GetTail()
            bool check = advanced_malloc_enlarge_last_block_from_realloc(curr,
                                                                         size);
            if (check) {
                curr->available = false;
                removeFromHist(curr);
                memcpy(curr + 1, oldp, requested_size);
                return curr + 1;
            } else {
                return NULL;
            }
        }
    } else {
        void *ptr = smalloc(size); // find a big block or allocate a new block
        if (!ptr)
            return NULL;
        memcpy(ptr, oldp, requested_size);
        sfree(oldp);
        return ptr;
    }
}

size_t _num_free_blocks() {
    int count = 0;
    MallocMetadata *temp = nullptr;
    for (int i = 0; i < HIST_SIZE; i++) {
        temp = hist[i];
        while (temp) {
            if (temp->available) count++;
            temp = temp->hist_next;
        }
    }
    return count;
}

size_t _num_free_bytes() {
    int count = 0;
    MallocMetadata *temp = nullptr;
    for (int i = 0; i < HIST_SIZE; i++) {
        temp = hist[i];
        while (temp) {
            if (temp->available)
                count += temp->alloc_size;
            temp = temp->hist_next;
        }
    }
    return count;
}

size_t _num_allocated_blocks() {
    int count = 0;
    MallocMetadata *temp = regular_list_head;
    MallocMetadata *temp_map = mmap_list_head;
    while (temp) {
        count++;
        temp = temp->next;
    }
    while (temp_map) {
        count++;
        temp_map = temp_map->next;
    }
    return count;
}

size_t _num_allocated_bytes() {
    int count = 0;
    MallocMetadata *temp = regular_list_head;
    MallocMetadata *temp_map = mmap_list_head;
    while (temp) {
        count += temp->alloc_size;
        temp = temp->next;
    }
    while (temp_map) {
        count += temp_map->alloc_size;
        temp_map = temp_map->next;
    }
    return count;
}

size_t _num_meta_data_bytes() {
    int counter = _num_allocated_blocks();
    return counter * META_SIZE;
}

size_t _size_meta_data() {
    return META_SIZE;
}
