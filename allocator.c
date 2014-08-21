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

void sal_init(u_int32_t size) {
    
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
            free_list_ptr = 0;          // memory[0] is the first segment global variable
            memory_size = correct_size; //size of memory allocated global variable 
               
            // create a new header for the new bloc of memory
            free_header_t firstMemBlock =  {MAGIC_FREE, memory_size, free_list_ptr, free_list_ptr};

            //write first header struct to the start of the malloced memory array
            memcpy(memory, &firstMemBlock, HEADER_SIZE);
        }
    }
}

void *sal_malloc(u_int32_t n) {
    
    // establish a point of reference called currMemBlock
    free_header_t * currMemBlock = &(memory[free_list_ptr]);
   
    //confirms if input request fits a memory segment available 
    //traverses through mememory untill it finds a potential fit
    while (currMemBlock->size < n + HEADER_SIZE) {  
        currMemBlock = &(memory[currMemBlock->next])
        
        /*
        // check for memory coruption durring header traverse (potential issue here)
        if (currMemBlock->magic != MAGIC_FREE) {
            fprintf(stderr, "Memory corruption");
            abort();
        }
        */


        //If it traverse through the list and reloops to the first header returns NULL    
        if (currMemBlock == &(memory[free_list_ptr])) {
            return NULL;
       }
    }
   
    // See if request can be fited within half the space of identified block otherwise continue with current block identified
    if((currMemBlock->size) / 2 >= (n + HEADER_SIZE)) {

        // segment the bloc untill a close fit is found
        while ((currMemBlock->size) / 2 >= (n + HEADER_SIZE)) {
        
            // make a new header
            free_header_t newHeader = {MAGIC_FREE, currMemBlock->size / 2, currMemBlock->next, (currMemBlock-memory)};
            memcpy(&(memory[((currMemBlock/2)-memory)], &newHeader, HEADER_SIZE); // recheck this!!!!!
            // change the currMemBlock stats
            currMemBlock -> next = (currMemBlock/2) - memory;
            currMemBlock -> size = currMemBlock -> size / 2;

        }
    } 
    // assign the new bloc as allocated
        currMemBlock -> magic = MAGIC_ALLOC;
    // adjust the free_lis_ptr to next free header after the newly allocated memory block
        free_list_ptr = currMemBlock->next;
    
   return (currMemBlock + HEADER_SIZE); //return pointer to the first byte of newly allocated memory after the header
}


void sal_free(void *object) {
    
    // establish a point of reference called currMemBlock
    free_header_t * currMemBlock = &(memory[free_list_ptr]);
    
    //
    if(*(object-HEADER_SIZE) != MAGIC_ALLOC){
        fprintf(stderr, "Attempt to free non-allocated memory");
        abort();
    } else {

    

    }

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
