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

#define NOT_FOUND -1

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

static int num_free_blocks = 0; // how many free blocks we have

static u_int32_t smallestPowerOfTwo (u_int32_t size);
static u_int32_t getBestFreeRegionIndex (u_int32_t desired_size);
static void splitFreeRegion (u_int32_t region_index, u_int32_t desired_size);
static void merge (u_int32_t region_index);

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
            num_free_blocks = 1;
               
            // create a new header for the new bloc of memory
            free_header_t firstMemBlock =  {MAGIC_FREE, memory_size, free_list_ptr, free_list_ptr};

            //write first header struct to the start of the malloced memory array
            memcpy(memory, &firstMemBlock, HEADER_SIZE);
        }
    }
}


void *sal_malloc(u_int32_t n) {
    // if we're all out of space, then fuck it.
    if (num_free_blocks == 0) {
        return NULL;
    }

    
    // desired size is just given plus the size of the header. 
    u_int32_t desired_size = n + HEADER_SIZE;
    
    // determine the 'best' region to allocate based on some policy
    // at the moment its just first-fit.
    u_int32_t chosen_region_index = getBestFreeRegionIndex (desired_size);

    // if there's no space, then return null
    if (chosen_region_index == NOT_FOUND) { 
        return NULL; 
    }

    // split the region, if possible.
    splitFreeRegion(chosen_region_index, desired_size);

    // set the magic variable to 'allocated'
    free_header_t * chosen_region_header = (free_header_t *) memory + chosen_region_index;
    chosen_region_header->magic = MAGIC_ALLOC;

    // remove the region from the free list by adjusting it's neighbours.
    if (num_free_blocks > 1) {
        free_header_t * next_neighbour_header = (free_header_t *) memory + chosen_region_header->next;
        free_header_t * prev_neighbour_header = (free_header_t *) memory + chosen_region_header->prev;
        next_neighbour_header->prev = chosen_region_header->prev;
        prev_neighbour_header->next = chosen_region_header->next;
    }

    // we're assuming that free_list_ptr already pointed to the smallest index.
    // so if the chosen region = free list ptr, then shifting it to next will maintain that
    // if it isnt, then we dont need to change anything
    // fuck yeah its an invariant.
    if (num_free_blocks > 1 && chosen_region_index == free_list_ptr) {
        free_list_ptr = chosen_region_header->next;
    }
   
    num_free_blocks--;
    
    // finally, return the address of the allocated region (plus header size)
    return (void *) memory + chosen_region_index + HEADER_SIZE; 
}

void sal_free(void *object) {
    
    // establish a header point of reference
    free_header_t * objectMemBlock = (free_header_t *) object - HEADER_SIZE;
    
    // find the header index of the object
    u_int32_t object_Index = object - (void *) memory - HEADER_SIZE;
    

    // Check requested header is valid
    if( objectMemBlock->magic != MAGIC_ALLOC) {
        fprintf(stderr, "Attempt to free non-allocated memory");
        abort();
    }
    
    // check if the entire block is allocated
    if(num_free_blocks == 0) { 
        objectMemBlock->next = object_Index;
        objectMemBlock->prev = object_Index;
        free_list_ptr = object_Index;
    // check if allocated bloc is the header with the smalled index
    } else if (object_Index < free_list_ptr) { 
        free_header_t * flpMemBlock = (free_header_t *) &(memory[free_list_ptr]);
        
        // Change index pointers
        objectMemBlock->next = free_list_ptr;
        objectMemBlock->prev = flpMemBlock->prev;
        flpMemBlock->prev = object_Index;
                    
        free_list_ptr = object_Index;
    // All remaining cases
    } else {
        free_header_t * neighbourFreeMemblock=(free_header_t *)  &(memory[free_list_ptr]);
        u_int32_t neighbourFreeMemblock_index = free_list_ptr; 
        
        // travers through free list untill an adjacent free memory block is found
        while(object_Index < neighbourFreeMemblock_index) {
            neighbourFreeMemblock_index = neighbourFreeMemblock-> next;
            neighbourFreeMemblock = (free_header_t *) &(memory[neighbourFreeMemblock_index]);
            
            if (neighbourFreeMemblock_index == free_list_ptr){
                break;    
            }
        }
        // Change index pointers
        objectMemBlock -> next = neighbourFreeMemblock-> prev;      
        neighbourFreeMemblock-> prev = object_Index;
        objectMemBlock -> next = neighbourFreeMemblock_index;
    } 

    objectMemBlock->magic = MAGIC_FREE;
    num_free_blocks++;

    merge (object_Index);
}

void sal_end(void) {
    free(memory);
    memory = NULL;
    memory_size = 0; // just in case the old value resurfaces
    num_free_blocks = 0;
}

void sal_stats(void) {
   // Optional, but useful
   printf("sal_stats\n");
    // we "use" the global variables here
    // just to keep the compiler quiet
   memory = memory;
   free_list_ptr = free_list_ptr;
   memory_size = memory_size;
}




////// Our things

static void merge (u_int32_t region_index) {
    // If theres only 1 free block then nothing should happen
    if (num_free_blocks < 2) {
        return;
    }

    free_header_t * region_header = (free_header_t *) memory + region_index;
    u_int32_t dest_region_index;
    free_header_t * dest_region_header;

    // If index is an even (inc. 0) multiple of size, then merge forward, otherwise merge backward.
    if ((region_index / region_header->size) % 2 == 0) {
        // we're going to try and merge it with the free region directly in front.
        dest_region_index = region_header->next;
        dest_region_header = (free_header_t *) memory + dest_region_index;

        // If it's located directly in front, AND is the right size, then they can
        // merge! (otherwise do nothing)
        if ( (dest_region_index == region_index + region_header->size) 
             && (dest_region_header->size == region_header->size) ) {

            // so weve agreed that free_list_ptr always contains the smallest possible free index.
            // this means that it never corresponds to the region directly in front (dest_index)
            // so dont have to do anything about it. Fuck yeah

            // adjust the size of the header;
            region_header->size *= 2;

            // adjust the free pointer list so everyone points to the right place
            // ie, the region in front is removed
            region_header->next = dest_region_header->next;
            free_header_t * new_neighbour = (free_header_t *) memory + region_header->next;
            new_neighbour->prev = region_index;

            // number of free blocks has decreased
            num_free_blocks--;

            // there is a possibility that we can merge again! So let's just call
            // merge again with region_index;
            merge (region_index);
        }

    } else {
        // we're going to try and merge it with the free region directly behind.
        dest_region_index = region_header->prev;
        dest_region_header =(free_header_t *)  memory + dest_region_index;

        // If it's located directly in front of the previous region, 
        // AND is the right size, then they can merge! (Otherwise do nothing)
        if ( (region_index == dest_region_index + dest_region_header->size) 
             && (dest_region_header->size == region_header->size) ) {

            // so weve agreed that free_list_ptr always contains the smallest possible free index.
            // this means that it never points to what we're on, since obviously theres a free 
            // region behind us in memory.

            // adjust the size of the previous header;
            dest_region_header->size *= 2;

            // adjust the free pointer list so everyone points to the right place
            // ie, the current region is removed
            dest_region_header->next = region_header->next;
            free_header_t * new_neighbour =(free_header_t *)  memory + dest_region_header->next;
            new_neighbour->prev = dest_region_index;

            // number of free blocks has decreased
            num_free_blocks--;

            // there is a possibility that we can merge again! So let's just call
            // merge again with dest_region_index;
            merge (dest_region_index);
        }
    }
}

static void splitFreeRegion (u_int32_t region_index, u_int32_t desired_size) {
    free_header_t * region_header = (free_header_t *) memory + region_index;

    // while we can fit it within half the space of the chosen free region, 
    // divide the region in half.
    while (region_header->size / 2 >= desired_size) {
        // determine where the split will occur.
        u_int32_t destination_index = region_index + (region_header->size / 2);

        // create a new header to insert at destination_index.
        free_header_t new_header = { MAGIC_FREE, 
                                     region_header->size / 2, 
                                     region_header->next,
                                     region_index };
      
        // insert the header
        memcpy(memory + destination_index, &new_header, HEADER_SIZE);
       
        // adjust the prev index of the header 'in front of' (this_free_region->next) the free region we're splitting.
        free_header_t * neighbour = (free_header_t *) memory + region_header->next;
        neighbour->prev = destination_index;
       
        // adjust the header of the region we're splitting to account for the changes.
        region_header->size /= 2;
        region_header->next = destination_index;
       
        num_free_blocks++;
   }
}

// At the moment our policy is to find the first place that fits,
// but that could change.
static u_int32_t getBestFreeRegionIndex (u_int32_t desired_size) {
    u_int32_t curr_free_region_index        = free_list_ptr;
    free_header_t * curr_free_region_header = (free_header_t *) memory + curr_free_region_index;
   
    // loop through all free regions until we find one that fits.
    while (curr_free_region_header->size < desired_size) {
        // get the index of the next free region.
        curr_free_region_index = curr_free_region_header->next;
       
        // if we're back to free_list_ptr, there's no space availeble
        // so just return NULL.
        if (curr_free_region_index == free_list_ptr) {
            return NOT_FOUND;
        } 
       
        // otherwise set the header pointer to the header of the next region.
        curr_free_region_header = (free_header_t *) memory + curr_free_region_index;
   }

   return curr_free_region_index;       
}

static u_int32_t smallestPowerOfTwo (u_int32_t size) {

    u_int32_t smallestPower = 2;
    while (smallestPower < size) {
        smallestPower *= 2;
    }

    return smallestPower;
}
