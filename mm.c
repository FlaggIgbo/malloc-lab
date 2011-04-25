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

//array holding the size of data in each class (total malloc'd, this includes metadata/overhead)
int CLASS_SIZE[NUM_CLASSES+1];

//array of pointers holding linked lists for all classes
void* CLASSES[NUM_CLASSES];

/*
 * mm_init - initialize the malloc package by populating arrays.
 */
int mm_init(void)
{	
	int i;
	
	for(i = 0; i < NUM_CLASSES; i++) {					 //populate CLASS_SIZE array
		CLASS_SIZE[i] = (ALIGN(ALIGNMENT^(i + 1)));
		CLASSES[i] = NULL;
	}
	
	CLASS_SIZE[NUM_CLASSES] = NULL; 						 //entry for MISC - bigger than biggest class

	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment
 *
 *	Sets up metadata for each free group as follows
 *	1 byte = free (0) or allocated (1), 4 bytes = size of chunk as int (including metadata), 
 *	~~~~DATA~~~~~, 1 byte = free or alloc
 */
void *mm_malloc(size_t size)
{

		 */
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
	chunk_size = (int)(*ptr);
	ptr--;
	
	//*****TODO - coallescing*****

	*ptr = 0;																		 //sets free/allocated byte to free(in header)
	
	//*NOTE TO ANDREW - tried doing it in 1 bit but too confusing/too much hassle - sticking to 1 byte*
	
	ptr = (int*)(ptr + 1);
	*ptr = chunk_size; 													//puts size data in (in header)
	ptr = (char*)(ptr + 1);
	*ptr = NULL; 																//puts in pointer to next chunk (NULL at the moment, 
																							//since this will be the last entry in the LL)

	ptr = (int*)((char*)ptr + chunk_size - 10);
	*ptr = chunk_size; 													//puts size data in (in footer)
	ptr = (char*)(ptr + 1);
	*ptr = 0; 																	//sets free/allocated byte (in footer)
	ptr = ptr - chunk_size + 6;									//back to pointer to next chunk (NULL in this case) 
	
	
	/*	Checks which size-class this data goes into
	 */
	class_size = CLASS_SIZE[i];
	while( (chunk_size > class_size) && (i < NUM_CLASSES) ) {
		class_size = CLASS_SIZE[++i];
	}
	
	if(class_size == NULL){ 										 //entry is bigger than any class size
		//*****TODO - add to miscellaneous-size linked list in a sorted fashion******
	} else {																		 //entry fits into a predefined class
		LL_ptr = CLASSES[class_size];
	}
	
	/*	Adds to end of linked list
	 */
	while(*LL_ptr != NULL){
		LL_ptr = *LL_ptr
	}
	*LL_ptr = ptr;
	
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


