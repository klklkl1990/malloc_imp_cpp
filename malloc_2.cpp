#include <unistd.h>
#include <string.h>

#define MAX 100000000
#define META_SIZE sizeof(MallocMetadata)

struct MallocMetadata {
    size_t alloc_size;
    size_t total_size;
    bool available;
    MallocMetadata *next;
    MallocMetadata *prev;
};

// Our global pointer to the list that contains all the data sectors
MallocMetadata *list_head = nullptr;

MallocMetadata *GetFirstAvailable(size_t size) {
    MallocMetadata *curr = list_head;
    while (curr) {
        if (curr->alloc_size >= size && curr->available) {
            return curr;
        }
        curr = curr->next;
    }
    return nullptr;
}

MallocMetadata *GetTail() {
    MallocMetadata *curr = list_head;
    MallocMetadata *tail = nullptr;
    while (curr) {
        tail = curr;
        curr = curr->next;
    }
    return tail;
}

void addToList(MallocMetadata *element) {
    if (list_head == NULL) {
        list_head = element;
        return;
    }
    MallocMetadata *tail = GetTail();
    tail->next = element;
    element->prev = tail;
}

void *smalloc(size_t size) {
    if (size == 0 || size > MAX) {
        return NULL;
    }
    void *ptr;
    MallocMetadata *exist = GetFirstAvailable(size);
    if (exist) {
        exist->available = false;
        ptr = exist;
    } else {
        //Didn't find any free block with size bytes
        ptr = sbrk(size + META_SIZE);
        if (ptr == (void *) (-1)) {
            return NULL;
        }
        auto *new_meta = (MallocMetadata *) ptr;
        *new_meta = (MallocMetadata) {size, size + META_SIZE, false, NULL, NULL};
        addToList(new_meta);
        ptr = new_meta;
    }
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

//return new_alloc; //not sure
void sfree(void *p) {
    if (!p)
        return;
    ((MallocMetadata *) p - 1)->available = true;
}


void *srealloc(void *oldp, size_t size) {
    if (size == 0 || size > MAX) {
        return NULL;
    }
    if (!oldp)
        return smalloc(size);
    if (size <= ((MallocMetadata *) oldp - 1)->alloc_size) {
        return oldp;
    }
    size_t requested_size = ((MallocMetadata *) oldp - 1)->alloc_size;
    void *ptr = smalloc(size);
    if (!ptr)
        return NULL;
    memcpy(ptr, oldp, requested_size);
    sfree(oldp);
    return ptr;
}

size_t _num_free_blocks() {
    int count = 0;
    MallocMetadata *temp = list_head;
    while (temp) {
        if (temp->available) count++;
        temp = temp->next;
    }
    return count;
}

size_t _num_free_bytes() {
    int count = 0;
    MallocMetadata *temp = list_head;
    while (temp) {
        if (temp->available)
            count += temp->alloc_size;
        temp = temp->next;
    }
    return count;
}

size_t _num_allocated_blocks() {
    int count = 0;
    MallocMetadata *temp = list_head;
    while (temp) {
        count++;
        temp = temp->next;
    }
    return count;
}

size_t _num_allocated_bytes() {
    int count = 0;
    MallocMetadata *temp = list_head;
    while (temp) {
        count += temp->alloc_size;
        temp = temp->next;
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
