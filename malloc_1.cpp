#include <unistd.h>

#define MAX 100000000
//MAX=10^8

void* smalloc(size_t size){
    if(size == 0 || size > MAX){
        return NULL;
    }
    void* prev_break=sbrk(size);
    if(prev_break==(void*)(-1)){
        return NULL;
    }
    return prev_break;
}