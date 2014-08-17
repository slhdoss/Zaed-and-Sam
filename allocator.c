//
//  COMP1927 Assignment 1 - Memory Suballocator
//  allocator.c ... implementation
//
//  Created by Liam O'Connor on 18/07/12.
//  Modified by John Shepherd in August 2014
//  Copyright (c) 2012-2014 UNSW. All rights reserved.
//

#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define HEADER_SIZE    sizeof(struct free_list_header)  
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

typedef unsigned char byte;
typedef u_int32_t vlink_t;          // Memory index
typedef u_int32_t vsize_t;          // Memory Size + Header 
typedef u_int32_t vaddr_t;          // index of the first memeory bloc

typedef struct free_list_header {
   u_int32_t magic;           // ought to contain MAGIC_FREE
   vsize_t size;              // # bytes in this block (including header)
   vlink_t next;              // memory[] index of next free block
   vlink_t prev;              // memory[] index of previous free block
} free_header_t;

// Global data

static byte *memory = NULL;   // pointer to start of suballocator memory  ???after the header
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]


void sal_init(u_int32_t size) {
    

    //check that size is a power of 2, if not increment untill it is
    if(sqrt((float)size) % 1 != 0){
        while((sqrt((float)size) % 1 != 0)) {
            size++;    
        }
        (u_int32_t) size 
    }

    // malloc new memory block
    byte *memory = malloc(size);

    // if there is insuficient memory for newMemBlock abort 
    if(memory == NULL){
        fprintf(stderr, "sal_init: insufficient memory");
        abort();  
    } else {
        free_list_ptr = memory[1];  //traverse through the header to first bloc of memory
        memory_size = sizeof(memory); //size of memory allocated
                
        Location -> magic = MAGIC_FREE; //?? don't know the purpose of this yet, general stats?? 
        Location -> size = sizeof(newMemBlock);  // general stats ot be stored in the header??
        Location -> next = free_list_ptr;   // stored index, initialized at the membloc post header start 
        Location -> prev = free_list_ptr; // stoted index, initialized at the membloc post header start
    }

}

void *sal_malloc(u_int32_t n)
{
   // TODO
   return NULL; // temporarily
}

void sal_free(void *object)
{
   // TODO
}

void sal_end(void)
{
   // TODO
}

void sal_stats(void)
{
   // Optional, but useful
   printf("sal_stats\n");
    // we "use" the global variables here
    // just to keep the compiler quiet
   memory = memory;
   free_list_ptr = free_list_ptr;
   memory_size = memory_size;
}
