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

#define OVERHEAD 16
#define HEADER_SIZE 8
#define FOOTER_SIZE 8

size_t* freeList = (size_t*)0xFFFFFFFF;
int freeListSize = 0;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
        printf("malloc...\n");
	int newSize = ALIGN(size) + OVERHEAD;
	void *p;
	size_t* nextPtr;
	size_t* lastPtr;
	int j = 0; //LL node counter - for debugging

	nextPtr = (size_t*)freeList;
	lastPtr = (size_t*)&freeList;

	//
	// while(1) {
	// 	if (nextPtr == (size_t*)0xFFFFFFFF) {
			p = mem_sbrk(newSize);
	// 		break;
	// 	} else if (*(int*)((char*)nextPtr - 4) >= size) {
	// 		p = (void*)((char*)nextPtr - 5);
	// 		*lastPtr = *nextPtr;
	// 		break;
	// 	} else {
	// 		lastPtr = nextPtr;
	// 		nextPtr = (size_t)*nextPtr;
	// 		j++;
	// 	}
	// }

	if (p == (void *)-1)
		return NULL;
	else {
		*(char*)p = 1; //set header ALLOCATED bit
		*(size_t *)((char*)p + 1) = size; //set header size
		*(size_t *)((char*)p + size + OVERHEAD - 5) = size; //set footer size
		*((char*)p + size + OVERHEAD - 1) = 1; //set footer ALLOCATED bit
		return (void *)((char *)p + HEADER_SIZE);
	}
}

/*
 * mm_free
 */
void mm_free(void *pointer)
{
        
	printf("free...\n");
	
	if (pointer < mem_heap_lo() || pointer > mem_heap_hi())
		return;
		
	int size;
	int found = 0;
	char* ptr = (char*)pointer - HEADER_SIZE; //points to the start of the free block
	size_t* nextPtr;
	size_t* lastPtr;
	
	/* ---Vars for loop debugging--- */
	size_t* storedPtr;
	int stored = 0;
	int loopSize = 0;
	int loopEnd = 0;
	int j = 0; //LL node counter - for debugging
	 /* ------ */
	
	size = *(int*)((char*)ptr + 1); //extracts size from malloc'ed metadata

	nextPtr = (size_t*)freeList;
	lastPtr = (size_t*)&freeList;
	
	while(found == 0) {
		if ((nextPtr == (size_t*)0xFFFFFFFF) || (*(int*)((char*)nextPtr - 4) >= size)) { //if found or at end
			*lastPtr = (size_t*)((char*)ptr + 5);
			*(size_t*)((char*)ptr + 5) = (size_t*)nextPtr;
			freeListSize++;
			found = 1;
		} else { //if not, keep looking through list
                        //printf("search fail...%d\n",j );
			lastPtr = nextPtr;
			nextPtr = (size_t*)*nextPtr;
			j++;
			if (j > freeListSize) { //loop occuring - fix loop
				if (stored == 0) {
					stored = 1;
					storedPtr = lastPtr;
				}
				if (storedPtr == nextPtr && loopSize > 2) {
					*nextPtr = (size_t*)0xFFFFFFFF;
					loopEnd = 1;
				}
				if (loopEnd == 0) {
					loopSize++;
				}
			}
		}		

	}

	// *(char*)p = 1; //set header UNALLOCATED bit
	// *(size_t *)((char*)p + 1) = size; //set header size
	// *(size_t *)((char*)p + size + OVERHEAD - 5) = size; //set footer size
	// *((char*)p + size + OVERHEAD - 1) = 1; //set footer UNALLOCATED bit
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldPtr = ptr;
	void *newPtr;
	size_t copySize;

	newPtr = mm_malloc(size);
	if (newPtr == NULL)
		return NULL;
	copySize = *(size_t *)((char *)oldPtr - HEADER_SIZE + 1);
	if (size < copySize)
		copySize = size;
	memcpy(newPtr, oldPtr, copySize);
	mm_free(oldPtr);
	return newPtr;
}
