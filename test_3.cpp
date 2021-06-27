#include "malloc_3.h"
#include <assert.h>
#include <exception>
#include <stdio.h>
#include <iostream>
#include <bits/stdc++.h>

#define TESTS_MAX_SIZE 100000000
#define TESTS_MIN_SPLIT 128
#define TESTS_BYTES_PER_KB 1024
#define TESTS_MMAP_MIN_SIZE 128*TESTS_BYTES_PER_KB


void smalloc_tests() {
    //Basics
    int *p1, *p2, *p3, *p4, *p5, *p6;
    int size1 = 256;
    int size2 = 4096;
    int size3 = 2 * TESTS_MMAP_MIN_SIZE;
    p1 = (int *) smalloc(-1);
    assert(p1 == NULL);
    p1 = (int *) smalloc(TESTS_MAX_SIZE + 1);
    assert(p1 == NULL);
    p1 = (int *) smalloc(size1 * sizeof(int));
    p2 = (int *) smalloc(size1 * sizeof(int));
    assert(p1 && p2);
    assert(p2 >= p1 + size1);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 2 * (size1 * sizeof(int)));
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);


    for (int i = 0; i < size1; i++)
        p1[i] = i;

    for (int i = 0; i < size1; i++)
        assert(p1[i] == i);

    //Challenge 1 split blocks
    std::cout << "challenge 1 accepted!" << std::endl;
    sfree(p1);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == size1 * sizeof(int));
    //shouldn't split
    std::cout << "not yet" << std::endl;
    p1 = (int *) smalloc(size1 * sizeof(int) - TESTS_MIN_SPLIT - _size_meta_data() + 1);
    assert(p1 < p2);
    assert(_num_allocated_blocks() == 2);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    std::cout << "not yet" << std::endl;
    sfree(p1);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == size1 * sizeof(int));
    //should split
    p1 = (int *) smalloc(size1 * sizeof(int) - TESTS_MIN_SPLIT - _size_meta_data());
    assert(p1 < p2);
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == TESTS_MIN_SPLIT);
    std::cout << "challenge 1 completed!" << std::endl;

    //Challenge 2 Merge blocks
    std::cout << "challenge 2 accepted!" << std::endl;
    sfree(p1);
    sfree(p2);
    assert(_num_free_blocks() <= 1);

    assert(_num_free_bytes() == 2 * (size1 * sizeof(int)) + _size_meta_data() ||
           _num_free_bytes() == 0);    //Allowing reducing the heap.


    p1 = (int *) smalloc(size2);
    p2 = (int *) smalloc(size2);
    p3 = (int *) smalloc(size2);
    p4 = (int *) smalloc(size2);
    p5 = (int *) smalloc(size2);

    assert(p1 && p2 && p3 && p4 && p5);

    assert(_num_allocated_blocks() == 5);
    assert(_num_allocated_bytes() == _num_allocated_blocks() * size2);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);

    sfree(p2);
    sfree(p4);
    assert(_num_free_blocks() == 2);
    assert(_num_free_bytes() == 2 * size2);

    sfree(p3);
    assert(_num_free_blocks() == 1);
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_bytes() == 3 * size2 + 2 * _size_meta_data());


    sfree(p1);
    assert(_num_allocated_blocks() == 2);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 4 * size2 + 3 * _size_meta_data());

    sfree(p5);
    assert(_num_free_blocks() <= 1);

    assert(_num_free_bytes() == 5 * (size2) + 4 * _size_meta_data() ||
           _num_free_bytes() == 0);
    std::cout << "challenge 2 completed!" << std::endl;

    //Challenge 3 Wilderness
    std::cout << "challenge 3 accepted!" << std::endl;

    p1 = (int *) smalloc(size2);
    p2 = (int *) smalloc(size2);
    p3 = (int *) smalloc(size2);
    p4 = (int *) smalloc(size2);
    p5 = (int *) smalloc(size2);

    sfree(p5);
    p5 = (int *) smalloc(3 * size2);

    p6 = (int *) smalloc(size2);

    assert(p6 > p5);

    assert(_num_allocated_bytes() == 8 * size2);

    assert(_num_allocated_blocks() == 6);
    std::cout << "challenge 3 completed!" << std::endl;
    //Challenge 4 Large allocations
    std::cout << "challenge 4 accepted!" << std::endl;

    sfree(p1);
    assert(_num_allocated_blocks() == 6);
    assert(_num_free_blocks() == 1);
    p1 = (int *) smalloc(size3);
    assert(p1 != nullptr);
    assert(_num_allocated_blocks() == 7);
    assert(_num_free_blocks() == 1);

    for (int i = 0; i < size3 / 4; i++)
        p1[i] = i;

    for (int i = 0; i < size3 / 4; i++)
        assert(p1[i] == i);


    sfree(p1);
    sfree(p2);
    sfree(p3);
    sfree(p4);
    sfree(p5);
    sfree(p6);

    assert(_num_free_blocks() == 1);

    std::cout << "challenge 4 completed!" << std::endl;

}

void srealloc_tests() {
    size_t size = _num_free_bytes();
    unsigned int num = size / sizeof(int);
    int *left, *middle, *right;
    int *temp;
    int *p1, *p2, *p3;

    left = (int *) scalloc(num, sizeof(int));
    middle = (int *) scalloc(num, sizeof(int));
    right = (int *) scalloc(num, sizeof(int));

    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 0);
    for (int i = 0; i < num; i++) {
        assert(left[i] == 0);
        assert(middle[i] == 0);
        assert(right[i] == 0);
    }

    sfree(left);
    sfree(right);
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 2);

    for (unsigned int i = 0; i < num; i++) {
        middle[i] = i;
    }

    //1.a
    temp = (int *) srealloc(middle, size);
    assert(temp == middle);

    //1.b
    temp = (int *) srealloc(middle, 2 * size + _size_meta_data());
    assert(_num_allocated_blocks() == 2);
    assert(_num_free_blocks() == 1);
    assert(temp == left);
    for (unsigned int i = 0; i < num; i++) {
        assert(temp[i] == i);
    }

    //1.c

    temp = (int *) srealloc(temp, 3 * size + 2 * _size_meta_data());
    assert(temp == left);
    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 0);

    for (unsigned int i = 0; i < num; i++) {
        assert(temp[i] == i);
    }

    sfree(temp);
    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 1);

    left = (int *) scalloc(num, sizeof(int));
    middle = (int *) scalloc(num, sizeof(int));
    right = (int *) scalloc(num, sizeof(int));

    for (unsigned int i = 0; i < num; i++) {
        middle[i] = i;
    }
    sfree(left);
    sfree(right);

    //1.d
    temp = (int *) srealloc(middle, 3 * size + 2 * _size_meta_data());

    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 0);
    assert(temp == left);
    for (unsigned int i = 0; i < num; i++) {
        assert(temp[i] == i);
    }

    sfree(temp);
    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 1);


    //1.e
    left = (int *) scalloc(num, sizeof(int));
    middle = (int *) scalloc(num, sizeof(int));
    right = (int *) scalloc(num, sizeof(int));

    for (unsigned int i = 0; i < num; i++) {
        middle[i] = i;
    }

    temp = (int *) srealloc(middle, 3 * size + 2 * _size_meta_data());
    assert(right < temp);
    for (unsigned int i = 0; i < num; i++) {
        assert(temp[i] == i);
    }
    assert(_num_allocated_blocks() == 4);
    assert(_num_free_blocks() == 1);


    sfree(left);
    sfree(temp);
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 2);

    //1.b edgy
    for (unsigned int i = 0; i < num; i++) {
        right[i] = i;
    }
    temp = (int *) srealloc(right, 2 * size + _size_meta_data());
    assert(temp == left);
    for (unsigned int i = 0; i < num; i++) {
        assert(temp[i] == i);
    }
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 2);

    sfree(temp);
    assert(_num_allocated_blocks() == 2);
    assert(_num_free_blocks() == 2);

    //1.f
    p1 = (int *) smalloc(3 * size + 2 * _size_meta_data());
    p2 = (int *) smalloc(size);
    p3 = (int *) smalloc(2 * size + _size_meta_data());
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 0);

    temp = (int *) srealloc(p2, 2 * size);

    assert(_num_allocated_blocks() == 4);
    assert(_num_free_blocks() == 1);


    //check wilderness
    sfree(p1);
    int freeBytesBefore = _num_free_bytes();
    int allocatedBytesBefore = _num_allocated_bytes();
    p2 =(int*) srealloc(temp, 3 * size);
    assert(_num_allocated_bytes() == allocatedBytesBefore + size);
    assert(_num_free_bytes() == freeBytesBefore);
    assert(p2 == temp);

    sfree(p2);
    sfree(p3);

    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 1);

    // Verify not using metadata after copy
    p1 = (int *) scalloc(num / 2, sizeof(int));
    p2 = (int *) scalloc(num, sizeof(int));
    for (unsigned int i = 0; i < num; i++) {
        p2[i] = i;
    }

    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 1);

    sfree(p1);

    p2 = (int *)srealloc(p2, size + 1);
    assert(p2 == p1);
    for (unsigned int i = 0; i < num; i++) {
        assert(p2[i] == i);
    }
}

void runTests() {
    std::cout << "It's gonna be LEGEN... " << std::endl;
    smalloc_tests();
    std::cout << "wait for it..." << std::endl;
    srealloc_tests();
    std::cout << "DARY! LEGENDARY!" << std::endl;
}

int main() {
    runTests();
    return 0;
}
