/
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
#include <math.h>

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

    // check that size consists of sensible values
    if(size < 0){ // another test to exclude characters???
        fprintf(stderr, "sal_init: memory request consists incorrect Values");
        abort();  
    //check that size is a power of 2, if not increment it untill it is 
    if(sqrt((float)size) % 1 != 0){
        while((sqrt((float)size) % 1 != 0)) {  // convert to float so that sqrt function can work
            size++;    
        }
        (u_int32_t)size;  // convert the varible back into u_int32_1 
    }    

    // malloc new memory block
    byte *memory = malloc(size);

    // if there is insuficient memory for memory bloc or failure in malloc abort 
    if(memory == NULL){
        fprintf(stderr, "sal_init: insufficient memory");
        abort();  
    } else {
        
        //set global variables
        free_list_ptr = 0;  // memory[0] is the first segment global variable
        memory_size = sizeof(memory); //size of memory allocated global variable 
        
        
        // create a new header for the new bloc of memory
        free_header_t firstMemBlock;

        // intialize the first header stuct with stats
        firstMemBlock.magic = MAGIC_FREE; // Magic free is an identifier which indicates the memory bloc has no content 
        firstMemBlock.size = HEADER_SIZE;  // No memory has yet been allocated to the header
        firstMemBlock.next = free_list_ptr;  // index, loops to the first head untill more headers are added 
        firstMemBlock.prev = free_list_ptr; //  index, loops to the first head untill more headers are added

        //write first header struct to the start of the malloced memory array
        memcpy(memory, &firstMemBlock, HEADER_SIZE);
        
    }

}

void *sal_malloc(u_int32_t n)
{
   // TODO
   return NULL; // temporarily
}

void sal_free(void *object) {
    // TODO
}

void sal_end(void) {
    free(memory);  
    memory_size = 0; // just in case the old value resurfaces
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



/*
Notes


