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

// global data
static byte *memory = NULL;   // pointer to start of suballocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]

// function prototypes
static u_int32_t smallestPowerOfTwo (u_int32_t size);
static u_int32_t getBestFreeRegionIndex (u_int32_t desired_size);
static void splitFreeRegion (u_int32_t region_index, u_int32_t desired_size);
static void merge (u_int32_t region_index);

void sal_init(u_int32_t size) {  
    // only allocate memory if we haven't already done so. 
    if (memory == NULL) {    
        // check that size consists of sensible values
        if (size < HEADER_SIZE + 1) { 
            fprintf(stderr, "sal_init: memory request too low");
            abort();
        }
        
        // find the smallest power of two >= the requested size. 
        u_int32_t correct_size = smallestPowerOfTwo(size); 

        // initialise memory
        memory = malloc(correct_size);

        // if malloc fails (due to insufficient available memory) then abort
        if(memory == NULL){
            fprintf(stderr, "sal_init: insufficient memory");
            abort();  
        } else { 
            // set global variables
            free_list_ptr = 0;              
            memory_size = correct_size;    
               
            // create the header to place at the start of the memory block
            free_header_t firstMemBlock = { MAGIC_FREE, 
                                            memory_size, 
                                            free_list_ptr, 
                                            free_list_ptr };

            // copy header to start of memory block
            memcpy(memory, &firstMemBlock, HEADER_SIZE);
        }
    }
}


void *sal_malloc(u_int32_t n) {
    // if user has requsted 0 bytes, then return NULL.
    if (n == 0) {
        return NULL;
    }

    u_int32_t desired_size = n + HEADER_SIZE;
    
    // find the region to allocate to based on some policy (best-fit)
    u_int32_t free_index = getBestFreeRegionIndex (desired_size);

    // if there's no space, then return null
    if (free_index == NOT_FOUND) { 
        return NULL; 
    }
    
    // split the region, if possible.
    splitFreeRegion (free_index, desired_size);

    // set the magic variable to 'allocated'
    free_header_t * free_header = (free_header_t *) (memory + free_index);
    
    // if there's only one free region then return NULL since this violates 
    // the invariant.
    if (free_header->next == free_index) {
        return NULL;
    }
    
    // otherwise set the header's magic value to allocated
    free_header->magic = MAGIC_ALLOC;
    
    // adjust the prev/next values of next/prev free regions in the list.
    free_header_t * next_free_header = (free_header_t *) (memory + free_header->next);
    free_header_t * prev_free_header = (free_header_t *) (memory + free_header->prev);    
    next_free_header->prev = free_header->prev;
    prev_free_header->next = free_header->next;

    // we're maintaining the invariant that free_list_ptr contains the smallest index
    // of any free region indexes. if we've chosen the region pointed at by free_list_ptr
    // shifting it to next will maintain this!
    if (free_index == free_list_ptr) {
        free_list_ptr = free_header->next;
    }

    // return the address of the allocated region (plus header size)
    return (void *) (memory + free_index + HEADER_SIZE); 
}

void sal_free(void *object) {  
    // find the index and create a header reference
    u_int32_t object_index = object - (void *) memory - HEADER_SIZE;
    free_header_t * object_header = (free_header_t *) (memory + object_index);
    
    // check if the object's header is valid.
    if (object_header->magic != MAGIC_ALLOC) {
        if (object_header->magic == MAGIC_FREE) {
            fprintf(stderr, 
                "memory corruption: attempting to free memory doesn't appear to be allocated");
        } else {
            fprintf(stderr, 
                "memory corruption: given location is not a valid memory region (perhaps you've changed it's value, or it's been overwritten)");
        }
        
        abort();
    }
    
    u_int32_t curr_free_index = free_list_ptr;
    free_header_t * curr_free_header = (free_header_t *) (memory + free_list_ptr);
    
    // free_list_ptr points to free region in the list with smallest index, so if the index of
    // the object is less than this we know free_header is directly in front. otherwise loop
    // through the list until we reach the free_header directly in front of it, or we've
    // wrapped around.
    while(curr_free_index < object_index) {
        curr_free_index = curr_free_header->next;
        curr_free_header = (free_header_t *) (memory + curr_free_index);
        
        if (curr_free_index == free_list_ptr){
            break;    
        }
    }
    
    // let the header behind the free'd region point to the free region header
    free_header_t * prev_header = (free_header_t *) (memory + curr_free_header->prev);
    prev_header->next = object_index;
    
    // set the new objects and curr region's next and prev values. curr_free_region is the one directly
    // in front of the list.
    object_header->next = curr_free_index;
    object_header->prev = curr_free_header->prev;
    curr_free_header->prev = object_index;
    
    // finally set the objects magic value to free 
    object_header->magic = MAGIC_FREE;
    
    if (object_index < free_list_ptr) {
        free_list_ptr = object_index;
    }
    
    // merge if possible.
    merge (object_index);
}

void sal_end(void) {
    if(memory != NULL){
        free(memory);
        memory = NULL;
        memory_size = 0; // just in case the old value resurfaces
    }
}

void sal_stats(void) {
}

/* ********************** static functions ********************** */

static void merge (u_int32_t region_index) {
    free_header_t * region_header = (free_header_t *) (memory + region_index);
    
    // if there's only one block left, then there's nothing to split!
    if (region_header->next == region_index) {
        return;
    }
    
    u_int32_t dest_region_index;
    free_header_t * dest_region_header;

    // If index is an even (inc. 0) multiple of size, then merge forward, otherwise merge backward.
    if ((region_index / region_header->size) % 2 == 0) {
        // we're going to try and merge it with the free region directly in front.
        dest_region_index = region_header->next;
        dest_region_header = (free_header_t *) (memory + dest_region_index);

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
            free_header_t * new_neighbour = (free_header_t *) (memory + region_header->next);
            new_neighbour->prev = region_index;


            // there is a possibility that we can merge again! So let's just call
            // merge again with region_index;
            merge (region_index);
        }

    } else {
        // we're going to try and merge it with the free region directly behind.
        dest_region_index = region_header->prev;
        dest_region_header =(free_header_t *)  (memory + dest_region_index);

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
            
            free_header_t * next_neighbour =(free_header_t *) (memory + region_header->next);
            next_neighbour->prev = dest_region_index;
            

            // there is a possibility that we can merge again! So let's just call
            // merge again with dest_region_index;
            merge (dest_region_index);
        }
    }
}

static void splitFreeRegion (u_int32_t region_index, u_int32_t desired_size) {
    free_header_t * region_header = (free_header_t *) (memory + region_index);

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
        free_header_t * neighbour = (free_header_t *) (memory + region_header->next);
        neighbour->prev = destination_index;
       
        // adjust the header of the region we're splitting to account for the changes.
        region_header->size /= 2;
        region_header->next = destination_index;
   }
}

// At the moment our policy is to find the first place that fits,
// but that could change.
static u_int32_t getBestFreeRegionIndex (u_int32_t desired_size) {
    u_int32_t curr_free_region_index        = free_list_ptr;
    free_header_t * curr_free_region_header = (free_header_t *) (memory + curr_free_region_index);
   
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
        curr_free_region_header = (free_header_t *) (memory + curr_free_region_index);
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
