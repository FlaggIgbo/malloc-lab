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

#define WSIZE       4       //header/footer size
#define DSIZE       8       //total overhead size
#define CHUNKSIZE  (1<<12)  //amnt to extend heap by 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

#define PACK(size, alloc)  ((size) | (alloc)) //puts size and allocated byte into 4 bytes

#define GET(p)       (*(unsigned int *)(p)) //read word at address p
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) //write word at address p

#define GET_SIZE(p)  (GET(p) & ~0x7) //extracts size from 4 byte header/footer
#define GET_ALLOC(p) (GET(p) & 0x1) //extracts allocated byte from 4 byte header/footer

#define HEADER(ptr)       ((char *)(ptr) - WSIZE) //get ptr's header address                   
#define FOOTER(ptr)       ((char *)(ptr) + GET_SIZE(HEADER(ptr)) - DSIZE) //get ptr's footer address

#define NEXT(ptr)  ((char *)(ptr) + GET_SIZE(((char *)(ptr) - WSIZE))) //next block
#define PREVIOUS(ptr)  ((char *)(ptr) - GET_SIZE(((char *)(ptr) - DSIZE))) //prev block

static char *firstBlock = 0;  //ptr to first block in list 
int test = 0;

/* Function prototypes for internal helper routines */
static void *enlarge(size_t size);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) 
{
	void* ptr;
	size_t size;
	
	firstBlock = mem_sbrk(4*WSIZE); //sbreaks out the first few bytes - to create prologue/epilogue
	
    if (firstBlock == (void *)-1) {
		return -1;
	}
	
    PUT(firstBlock, 0); //for alignment
    PUT(firstBlock + (1*WSIZE), PACK(DSIZE, 1)); //header for prologue entry/node (8 bytes)
    PUT(firstBlock + (2*WSIZE), PACK(DSIZE, 1)); //footer for prologue entry/node (8 bytes)
    PUT(firstBlock + (3*WSIZE), PACK(0, 1)); //header for epilogue node (only 4 bytes)
    firstBlock += (2*WSIZE); //moves the pointer up to between prologue/epilogue



	/* sbreak out a heap for free space */	
	if ((CHUNKSIZE/WSIZE)%2) { //ensures we sbrk an even number of words (WSIZEs) to make sure heap is aligned by 8
		size = ((CHUNKSIZE/WSIZE) + 1) * WSIZE;
	} else {
		size = CHUNKSIZE;
	}
	
	ptr = mem_sbrk(size);
	if ((long)ptr == -1) { //if mem_sbrk didn't work
		return -1;
	}

	//mark header/footer/epilogue header for new, gigantic free heap
    PUT(HEADER(ptr), PACK(size, 0)); //free-block header
    PUT(FOOTER(ptr), PACK(size, 0)); //free-block footer
    PUT(HEADER(NEXT(ptr)), PACK(0, 1)); //New epilogue header

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) 
{
    size_t asize; //size including overhead
	size_t slotSize; //size of empty slot found
    char* ptr;
	void* findPtr; //pointer for finding the next empty slot

    if (firstBlock == 0){
		mm_init();
    }

    if (size == 0) { //check for ignoring if no size given
		return NULL;
	}

	asize = ALIGN(size) + DSIZE; //aligns and adds overhead
	
    /* searches the free list for a free block */
	ptr = NULL;
    for (findPtr = firstBlock; GET_SIZE(HEADER(findPtr)) > 0; findPtr = NEXT(findPtr)) {
		if (!GET_ALLOC(HEADER(findPtr)) && (asize <= GET_SIZE(HEADER(findPtr)))) { //if an empty/big enough block is found
			ptr = findPtr;
			break;
		}
    }
	
	/* Convert free block to used */
    if (ptr != NULL) {  
		slotSize = GET_SIZE(HEADER(ptr));   

		//ensures the remainder of free slot is big enough to be its own free slot
	    if ((slotSize - asize) >= (2*DSIZE)) { 
			PUT(HEADER(ptr), PACK(asize, 1));
			PUT(FOOTER(ptr), PACK(asize, 1));
			ptr = NEXT(ptr);
			PUT(HEADER(ptr), PACK(slotSize-asize, 0));
			PUT(FOOTER(ptr), PACK(slotSize-asize, 0));
			ptr = PREVIOUS(ptr);
	    }
	    else { 
			PUT(HEADER(ptr), PACK(slotSize, 1));
			PUT(FOOTER(ptr), PACK(slotSize, 1));
	    }

		return ptr;
    }

	ptr = enlarge(MAX(asize,CHUNKSIZE));
	
    if (ptr == NULL)  
		return NULL;
		
    slotSize = GET_SIZE(HEADER(ptr));   

    if ((slotSize - asize) >= (2*DSIZE)) { 
		PUT(HEADER(ptr), PACK(asize, 1));
		PUT(FOOTER(ptr), PACK(asize, 1));
		ptr = NEXT(ptr);
		PUT(HEADER(ptr), PACK(slotSize-asize, 0));
		PUT(FOOTER(ptr), PACK(slotSize-asize, 0));
		ptr = PREVIOUS(ptr);
    }
    else { 
		PUT(HEADER(ptr), PACK(slotSize, 1));
		PUT(FOOTER(ptr), PACK(slotSize, 1));
    }

    return ptr;
} 

/* 
 * mm_free - frees blocks 
 */
void mm_free(void *ptr)
{
	size_t prevBlock;
	size_t nextBlock;
	
/* $end mmfree */
    if(ptr == 0) 
	return;

/* $begin mmfree */
    size_t size = GET_SIZE(HEADER(ptr));
/* $end mmfree */
    if (firstBlock == 0){
	mm_init();
    }
/* $begin mmfree */

    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));

    /* Coalesce if the previous block was free */
    prevBlock = GET_ALLOC(FOOTER(PREVIOUS(ptr)));
    nextBlock = GET_ALLOC(HEADER(NEXT(ptr)));
    size = GET_SIZE(HEADER(ptr));

    if (prevBlock && nextBlock) {            /* Case 1 */
		//do nothing
    } else if (prevBlock && !nextBlock) {      /* Case 2 */
		size += GET_SIZE(HEADER(NEXT(ptr)));
		PUT(HEADER(ptr), PACK(size, 0));
		PUT(FOOTER(ptr), PACK(size,0));
    } else if (!prevBlock && nextBlock) {      /* Case 3 */
		size += GET_SIZE(HEADER(PREVIOUS(ptr)));
		PUT(FOOTER(ptr), PACK(size, 0));
		PUT(HEADER(PREVIOUS(ptr)), PACK(size, 0));
		ptr = PREVIOUS(ptr);
    } else {                                     /* Case 4 */
		size += GET_SIZE(HEADER(PREVIOUS(ptr))) + 
	    GET_SIZE(FOOTER(NEXT(ptr)));
		PUT(HEADER(PREVIOUS(ptr)), PACK(size, 0));
		PUT(FOOTER(NEXT(ptr)), PACK(size, 0));
		ptr = PREVIOUS(ptr);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldSize;
	void* newptr;
	
	newptr = mm_malloc(size);
	
	if (!newptr && size != 0) //if newPtr is null, mm_malloc failed
		return NULL;
		
	if (ptr == NULL) //if ptr is null, no need to free
		return newptr;

	if (size != 0) { //if size == 0, simply free
		oldSize = GET_SIZE(HEADER(ptr));
		if (size < oldSize)
			oldSize = size;
		memcpy(newptr, ptr, oldSize);
	} else {
		newptr = 0;
	}

	mm_free(ptr);
	
	return newptr;
}


/* 
 * enlarge - sbreaks more free space to be malloc'd
 */
static void *enlarge(size_t size) 
{
    char *ptr;
    size_t adjustedSize;
	size_t prevBlock;
	size_t nextBlock;

    /* Allocate an even number of words to maintain alignment */
    adjustedSize = ((size/WSIZE) % 2) ? ((size/WSIZE)+1) * WSIZE : size; 
    
	ptr = mem_sbrk(adjustedSize);
	if ((long)ptr == -1)  
		return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HEADER(ptr), PACK(adjustedSize, 0));         /* Free block header */   
    PUT(FOOTER(ptr), PACK(adjustedSize, 0));         /* Free block footer */   
    PUT(HEADER(NEXT(ptr)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    prevBlock = GET_ALLOC(FOOTER(PREVIOUS(ptr)));
    nextBlock = GET_ALLOC(HEADER(NEXT(ptr)));
    adjustedSize = GET_SIZE(HEADER(ptr));

    if (prevBlock && nextBlock) {            /* Case 1 */
		return ptr;
    } else if (prevBlock && !nextBlock) {      /* Case 2 */
		adjustedSize += GET_SIZE(HEADER(NEXT(ptr)));
		PUT(HEADER(ptr), PACK(adjustedSize, 0));
		PUT(FOOTER(ptr), PACK(adjustedSize,0));
    } else if (!prevBlock && nextBlock) {      /* Case 3 */
		adjustedSize += GET_SIZE(HEADER(PREVIOUS(ptr)));
		PUT(FOOTER(ptr), PACK(adjustedSize, 0));
		PUT(HEADER(PREVIOUS(ptr)), PACK(adjustedSize, 0));
		ptr = PREVIOUS(ptr);
    } else {                                     /* Case 4 */
		adjustedSize += GET_SIZE(HEADER(PREVIOUS(ptr))) + 
	    GET_SIZE(FOOTER(NEXT(ptr)));
		PUT(HEADER(PREVIOUS(ptr)), PACK(adjustedSize, 0));
		PUT(FOOTER(NEXT(ptr)), PACK(adjustedSize, 0));
		ptr = PREVIOUS(ptr);
    }

    return ptr;                                        
}