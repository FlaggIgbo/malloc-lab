/*
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
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PG_SIZE 2<<12

#define NUM_CLASSES 10

#define OVERHEAD (ALIGN(sizeof(int) + 2))

//the length of the initial seg-list
#define SEGLIST_LENGTH 10

//array holding the size of usable data in each class (total malloc'd - overhead)
int CLASS_SIZE[NUM_CLASSES];

//array of pointers holding linked lists for all classes
void* CLASSES[NUM_CLASSES];

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{	
	int i, j;
	int alloc_size; //size of memory chunks in any particular size class
	void* mem_chunk;
	
	for(i = 0; i < NUM_CLASSES; i++) { //populate CLASS_SIZE array
		CLASS_SIZE[i] = (ALIGN(ALIGNMENT^(i + 1)) - OVERHEAD);
	}
	
	for(i = 0; i < NUM_CLASSES; i++) {   //mem_sbrk each class
		alloc_size = ALIGN(ALIGNMENT^(i + 1));
		CLASSES[i] = mem_sbrk(alloc_size * SEGLIST_LENGTH);
		
		mem_chunk = CLASSES[i];
		CLASSES[i] = (char*)mem_chunk + 5; //changes pointer to point to THAT chunk's pointer out (making a linked list)
		
		/*	Sets up metadata for each free group as follows
		 *	1 byte = free (0) or allocated (1), 4 bytes = size of chunk as int (including metadata), 
		 *	4 bytes = pointer to next chunk's pointer, ~~~~DATA~~~~~, 4 bytes = size, 1 byte = free or alloc
		 */
		for(j = 0; j < SEGLIST_LENGTH; j++) {
			*mem_chunk = 0; //sets free/allocated byte 
			//*NOTE TO ANDREW - tried doing it in a bit but too confusing/too much hassle - sticking to 1 byte*
			mem_chunk = (char*)mem_chunk + 1;
			*mem_chunk = alloc_size; //puts size data in (in header)
			mem_chunk = (int*)mem_chunk + 1;
			*mem_chunk = (char*)mem_chunk + alloc_size; //puts in pointer to next chunk
			mem_chunk = (char*)mem_chunk + alloc_size - 10;
			*mem_chunk = alloc_size; //puts size data in (in footer)
			mem_chunk = (int*)mem_chunk + 1;
			*mem_chunk = 0; //sets free/allocated byte
			mem_chunk = (char*)mem_chunk + 1; //moves to next in list
		} 
	}
	return 0;
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	int newsize = ALIGN(size + SIZE_T_SIZE);
	

	void *p = mem_sbrk(newsize);
	if (p == (void *)-1)
		return NULL;
	else {
		*(size_t *)p = size;
		return (void *)((char *)p + SIZE_T_SIZE);
	}
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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


