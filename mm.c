/*/*{{{*/
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	/* Team name */
	"Prostatus",
	/* First member's full name */
	"Andrew Flockhart",
	/* First member's NYU NetID*/
	"abf277@nyu.edu",
	/* Second member's full name (leave blank if none) */
	"Hursh Agrawal",
	/* Second member's email address (leave blank if none) */
	"ha470@nyu.edu"
};/*}}}*/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PG_SIZE 2<<12

#define NUM_CLASSES 10

#define OVERHEAD (ALIGN(sizeof(int) + 2))

//array holding the size of data in each class (total malloc'd, this includes metadata/overhead)
//class sizes are 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096
int CLASS_SIZE[NUM_CLASSES+1];

//array of pointers holding linked lists for all classes
void* CLASSES[NUM_CLASSES+1];

/*
 * mm_init - initialize the malloc package by populating arrays.
 */
int mm_init(void)
{
	int i;
	int size_value = ALIGNMENT;

	for(i = 0; i < NUM_CLASSES; i++) {					 //populate CLASS_SIZE array

		CLASS_SIZE[i] = (ALIGN(size_value));
		CLASSES[i] = NULL;

		size_value = size_value*2;
	}

	CLASS_SIZE[NUM_CLASSES] = NULL; 					//entry for MISC - bigger than biggest class

	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment
 *
 *	Sets up metadata for each free group as follows
 *	1 byte = free (0) or allocated (1), 4 bytes = size of chunk as int (including metadata),
 *	~~~~DATA~~~~~,
 *	1 byte = free or alloc
 *
 *	-Searches for appropriate sized block in free list
 *	-If found,
 *	    >marks as used, mark size metadata
 *	    >removes block from free list
 *	    >repairs links of free list
 *	    >return pointer to beginning
 *	        TODO: is the pointer to the beginning of raw data or beginning of metadata?
 *	        >think i figured it out- we want to point to the beginning of the raw data.
 *	    >takes remaining space and puts it in proper size free list (if possible)
 *	-If unfound
 *	    >mem_sbrk appropriate size
 *	    >mark as used, mark size metadata
 *	    >return pointer to beginning
 */
void *mm_malloc(size_t size)
{

        int wasFound; //will be 1 if block is found, 0 if not found
	int newsize = ALIGN(size + OVERHEAD); //TODO is this correct? why are we adding SIZE_T_SIZE?? have changed to OVERHEAD
        int i = 0;

        while(i < NUM_CLASSES){//find appropriate size class
            if newsize <= CLASS_SIZE[i]
                break;
            i++;
        }                      //i now holds index of appropriate size class

        void* LL_ptr = CLASSES[i];

        while(wasFound ==0){//search size class for free block, looking for ideal size, this assumes the list is sorted small to large
           if LL_ptr == NULL //Linked list did not contain appropriate size
                break;
        }



	void *p = mem_sbrk(newsize);
	if (p == (void *)-1)
		return NULL;
	else {
		*(size_t *)p = size;
		return (void *)((char *)p + SIZE_T_SIZE);
	}
}

/*
 * mm_free - Frees a block by changing metadata to freed and adding to appropriate linked list.
 *
 *	Sets up metadata for each free group as follows
 *	1 byte = free (0) or allocated (1), 4 bytes = size of chunk as int (including metadata),
 *	4/8 bytes = pointer to next chunk's pointer, ~~~~EMPTY~~~~~, 4 bytes = size, 1 byte = free or alloc
 *
 */
void mm_free(void *ptr)
{
	int chunk_size, class_size;
	int i = 0;
	void* LL_ptr;

	ptr = (char*)ptr - 4;
	chunk_size = *(int*)ptr;
	ptr = (char*)ptr - 1;

	//*****TODO - coallescing*****

	*ptr = 0;															//sets free/allocated byte to free(in header)

	ptr = (int*)(ptr + 1);
	*ptr = chunk_size; 													//puts size data in (in header) - redo post coallescing

	ptr = (int*)((char*)ptr + chunk_size - 6);
	*ptr = chunk_size; 													//puts size data in (in footer)
	ptr = (char*)(ptr + 1);
	*ptr = 0; 															//sets free/allocated byte (in footer)

	ptr = ptr - chunk_size + 6;											//back to pointer to next chunk (NULL in this case)

	class_size = CLASS_SIZE[i];
	while( (chunk_size > class_size) && (i < NUM_CLASSES) ) {
		class_size = CLASS_SIZE[++i];						    	    //Checks which size class the data goes into
	}

	if(class_size == NULL){										 	    //entry is bigger than any class size

		//*****TODO - add to miscellaneous-size linked list in a sorted fashion******

	} else {															//entry fits into a predefined class
		LL_ptr = CLASSES[class_size];
	}

/*	while(*LL_ptr != NULL){										   		//Adds to end of linked list
		LL_ptr = *LL_ptr

		//******TODO - add to linked list in a SORTED fashion - to make a sorted LL, NOT at end*******
	}
	*LL_ptr = ptr;
*/

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t copySize;

	newptr = mm_malloc(size);
	if (newptr == NULL)
		return NULL;
	copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
	if (size < copySize)
		copySize = size;
	memcpy(newptr, oldptr, copySize);
	mm_free(oldptr);
	return newptr;
}

/*
 * mm_check - checks consistency of heap
 */
int	mm_check(void)
{

}


