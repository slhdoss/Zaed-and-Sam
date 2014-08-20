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
#include <string.h>
#include <assert.h>

#define HEADER_SIZE    sizeof(struct free_list_header)  
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

typedef unsigned char byte;
typedef u_int32_t vlink_t;
typedef u_int32_t vsize_t;
typedef u_int32_t vaddr_t;

typedef struct free_list_header {
   u_int32_t magic;           // ought to contain MAGIC_FREE
   vsize_t size;              // # bytes in this block (including header)
   vlink_t next;              // memory[] index of next free block
   vlink_t prev;              // memory[] index of previous free block
} free_header_t;

// Global dataa

static byte *memory = NULL;   // pointer to start of suballocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]

static u_int32_t smallestPowerOfTwo (u_int32_t size);

void sal_init(u_int32_t size)
{
    if (memory == NULL) {
        // check that size consists of sensible values
        if(size < 0){ // another test to exclude characters???
            fprintf(stderr, "sal_init: memory request consists incorrect Values");
            abort();
        }
        //check that size is a power of 2, if not increment it untill it is 
        u_int32_t correct_size = smallestPowerOfTwo (size + HEADER_SIZE); 

        // malloc new memory block
        memory = malloc(correct_size);

        // if there is insuficient memory for memory bloc or failure in malloc abort 
        if(memory == NULL){
            fprintf(stderr, "sal_init: insufficient memory");
            abort();  
        } else { 
            //set global variables
            free_list_ptr = 0xCCCCCCCC;          // memory[0] is the first segment global variable
            memory_size = correct_size; //size of memory allocated global variable 
               
            // create a new header for the new bloc of memory
            free_header_t firstMemBlock =  {MAGIC_FREE, memory_size, free_list_ptr, free_list_ptr};
            /*
            // intialize the first header stuct with stats
            firstMemBlock.magic = MAGIC_FREE; // Magic free is an identifier which indicates the memory bloc has no content 
            firstMemBlock.size = HEADER_SIZE;  // No memory has yet been allocated to the header
            firstMemBlock.next = free_list_ptr;  // index, loops to the first head untill more headers are added 
            firstMemBlock.prev = free_list_ptr; //  index, loops to the first head untill more headers are added
            */

            //write first header struct to the start of the malloced memory array
            memcpy(memory, &firstMemBlock, HEADER_SIZE);
        }
    }
}

void *sal_malloc(u_int32_t n)
{
   // Find a region that fits.
   free_header_t * currFreeRegion = &(memory[free_list_ptr]);
   
   while (currFreeRegion->size < n + HEADER_SIZE) {
       currFreeRegion = &(memory[currFreeRegion->next])
       
       if (currFreeRegion == &(memory[free_list_ptr])) {
           return NULL;
       }
   }
   
   // See if we can fit it within half the space
   while ((currFreeRegion->size) / 2 >= (n + HEADER_SIZE)) {
       free_header_t newHeader = {MAGIC_FREE, currFreeRegion->size / 2, free_list_ptr, free_list_ptr};
       // Divide it in half
       // Put the header half way
       // Adjust the sizes
   }
   
   return NULL; // temporarily
}




void sal_free(void *object)
{
   // TODO
}

void sal_end(void)
{
    free(memory);
    memory = NULL;
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




////// Our things

static u_int32_t smallestPowerOfTwo (u_int32_t size) {

    u_int32_t smallestPower = 2;
    while (smallestPower < size) {
        smallestPower *= 2;
    }

    return smallestPower;
}
