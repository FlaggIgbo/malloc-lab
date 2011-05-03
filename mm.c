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

/* whether or not array free list is implemented*/
#define ARRAY_IMPLEMENTATION 0

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* MALLOC MACROS - TEXT pg 830 */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HEADER(bp)       ((char *)(bp) - WSIZE)
#define FOOTER(bp)       ((char *)(bp) + GET_SIZE(HEADER(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* END MALLOC MACROS */

static char* firstBlock = 0; //points to first block in allocated/free list

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	void* firstBlock;
	void* firstHeapExtention;
	char* ptr;
	size_t size;

	firstBlock = mem_sbrk(2*DSIZE); //sbreaks out for prologue/epilogue nodes
	if (firstBlock == (void*)-1 ) {
		return -1;
	}

	PUT(firstBlock, 0); //add padding byte
	PUT(firstBlock + WSIZE, PACK(DSIZE, 1)); //header for prologue entry/node (16 bytes)
	PUT(firstBlock + (2*WSIZE), PACK(DSIZE, 1)); //footer for prologue entry/node (16 bytes)
	PUT(firstBlock + (3*WSIZE), PACK(0, 1)); //header for epilogue node (only 8 bytes)
	firstBlock = firstBlock + (2*WSIZE); //moves the pointer up to the middle of

	/* Extend heap for first CHUNKSIZE bytes to be malloc'd */

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
	PUT(FDRP(ptr), PACK(size, 0)); //free-block footer
	PUT(HEADER(NEXT_BLKP(ptr)), PACK(0, 1)); //New epilogue header

	//coalesce with previous epilogue header
	size += GET_SIZE(HEADER(PREV_BLKP(ptr)));
	PUT(HEADER(PREV_BLKP(ptr)), PACK(size, 0)); //new header pos, new size
	PUT(FOOTER(ptr), PACK(size, 0)); //same footer pos, new size

	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t newSize; //size including overhead
	size_t heapSize; //size of heap (if heap needs extending)
	size_t slotSize; //size of empty slot found

	char* ptr = NULL;
	void* findPtr;

	if (size == 0)
		return NULL;

	newSize = ALIGN(size + DSIZE); //takes into account overhead and alignment

	/* Search the list for a free block */
	findPtr = firstBlock;
	while (GET_SIZE(HEADER(findPtr)) > 0) {
		if (!GET_ALLOC(HEADER(findPtr)) && (newSize <= GET_SIZE(HEADER(findPtr)))) { //if an empty & big enough block is found
			ptr = findPtr;
			break;
		}
		findPtr = NEXT_BLKP(findPtr);
    }

	/* Convert free block into used */
	if (ptr != NULL) {
		slotSize = GET_SIZE(HEADER(ptr));

		//ensures the remainder of free slot is big enough to be its own free slot
	    if ((slotSize - newSize) >= (2 * DSIZE)) {
			PUT(HEADER(ptr), PACK(newSize, 1));
			PUT(FOOTER(ptr), PACK(newSize, 1));
			ptr = NEXT_BLKP(ptr);
			PUT(HEADER(ptr), PACK(newSize - slotSize, 0));
			PUT(FOOTER(ptr), PACK(newSize - slotSize, 0));
			ptr = PREV_BLKP(ptr);
	    } else {
			PUT(HEADER(ptr), PACK(newSize, 1));
			PUT(FOOTER(ptr), PACK(newSize, 1));
	    }
		return ptr;
	} else { //if ptr IS NULL, meaning no free slot was found
		heapSize = MAX(newSize, CHUNKSIZE); //for the case newSize > CHUNKSIZE, extends heap by newSize

		if ((heapSize/WSIZE)%2) { //ensures we sbrk an even number of words (WSIZEs) to make sure heap is aligned by 8
			size = ((heapSize/WSIZE) + 1) * WSIZE;
		} else {
			size = heapSize;
		}

		ptr = mem_sbrk(size);
		if ((long)ptr == -1) { //if mem_sbrk didn't work
			return -1;
		}

		//mark header/footer/epilogue header for new, gigantic free heap
		PUT(HEADER(ptr), PACK(size, 0)); //free-block header
		PUT(FDRP(ptr), PACK(size, 0)); //free-block footer
		PUT(HEADER(NEXT_BLKP(ptr)), PACK(0, 1)); //New epilogue header

		//coalesce with previous epilogue header
		size += GET_SIZE(HEADER(PREV_BLKP(ptr)));
		PUT(HEADER(PREV_BLKP(ptr)), PACK(size, 0)); //new header pos, new size
		PUT(FOOTER(ptr), PACK(size, 0)); //same footer pos, new size

		slotSize = GET_SIZE(HEADER(ptr));

		//ensures the remainder of free slot is big enough to be its own free slot
	    if ((slotSize - newSize) >= (2 * DSIZE)) {
			PUT(HEADER(ptr), PACK(newSize, 1));
			PUT(FOOTER(ptr), PACK(newSize, 1));
			ptr = NEXT_BLKP(ptr);
			PUT(HEADER(ptr), PACK(newSize - slotSize, 0));
			PUT(FOOTER(ptr), PACK(newSize - slotSize, 0));
			ptr = PREV_BLKP(ptr);
	    } else {
			PUT(HEADER(ptr), PACK(newSize, 1));
			PUT(FOOTER(ptr), PACK(newSize, 1));
	    }

		return ptr;
	}


}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	size_t* size = GET_SIZE(HEADER(ptr));
	size_t previousBlock = GET_ALLOC(FOOTER(PREV_BLKP(ptr))); //1 if prev. block is allocated, 0 if free
	size_t nextBlock = GET_ALLOC(HEADER(PREV_BLKP(ptr))); //1 if next block is allocated, 0 if free

	PUT(HEADER(ptr), PACK(size, 0));
	PUT(FOOTER(ptr), PACK(size, 0));

	/* Start coalescing */
	if (previousBlock && nextBlock) { /* if both prev/next are allocated */
		//Do nothing
	} else if (previousBlock && !nextBlock) { /* if only next block is free */
		size += GET_SIZE(HEADER(NEXT_BLKP(ptr)));
		PUT(HEADER(ptr), PACK(size, 0)); //same header pos, new size
		PUT(FOOTER(ptr), PACK(size, 0)); //new footer pos, new size
	} else if (!previousBlock && nextBlock) { /* if only prev block is free */
		size += GET_SIZE(HEADER(PREV_BLKP(ptr)));
		PUT(HEADER(PREV_BLKP(ptr)), PACK(size, 0)); //new header pos, new size
		PUT(FOOTER(ptr), PACK(size, 0)); //same footer pos, new size
		ptr = PREV_BLKP(ptr);
	} else { /* If both prev/next are free */
		size += GET_SIZE(HEADER(PREV_BLKP(ptr))) + GET_SIZE(FOOTER(NEXT_BLKP(ptr))); //size is sum of all three (prev, current, next)
		PUT(HEADER(PREV_BLKP(ptr))), PACK(size, 0); //new header pos, new size
		PUT(FOOTER(NEXT_BLKP(ptr))), PACK(size, 0); //new footer pos, new size
		ptr = PREV_BLKP(ptr);
	}
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
 * mm_check
 *an extended version of the mm_check suggested in the pdf as well as the example provided on
 *
 *the cs-app website
 */
int check_block(void *bp){
    if ((size_t)bp%8){
        printf("ERROR, %p is not aligned correctly\n", bp);
        return 1;
    }
    if (GET(HDRP(bp)) != GET(FTRP(bp))){
        printf("ERROR, %p has inconsistent header/footer\n", bp);
        return 2;
    }
    return 0;
}

int mm_check(void){

    char *bp;
    int cont = 1;
    int size;
    int found = 0;

    size_t* start_heap =  mem_heap_lo();
    size_t* end_heap =  mem_heap_hi();

    size_t* curr_block = start_heap;

    for(bp = start_heap; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        printf(check_block(bp));
        if (bp > end_heap || bp < start_heap)
            printf("Error: pointer %p out of heap bounds\n", bp);
        if (GET_ALLOC(bp) == 0 && GET_ALLOC(NEXT_BLKP(bp))==0)
            printf("ERROR: contiguous free blocks %p and %p not coalesced\n", bp, NEXT_BLKP(bp));
        if(ARRAY_IMPLEMENTATION){//need to check if it is in the free array

            for(int i = 0; i < sizeof(FREE_ARRAY); i++){
                if(FREE_ARRAY[i] == bp){
                    found = 1;
                    break;
                }
            }
            if(!found)
                printf("ERROR: pointer %p marked as free in heap but not found in free list", bp);
        }
    }

    if(ARRAY_IMPLEMENTATION){
        for(int i = 0; i < sizeof(FREE_ARRAY); i++){
            if( FREE_ARRAY[i] != NULL ){//will only examine true entries in the array
                if(GET_ALLOC(FREE_ARRAY[i] != 0))
                    printf("ERROR: entry %d in free list not marked as free\n", i);
                if(size = GET_SIZE(FREE_ARRAY[i]) == 0)
                    printf("ERROR: size is %d", i);
            }
        }
    }

    return 0;
}











