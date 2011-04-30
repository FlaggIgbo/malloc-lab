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

#define OVERHEAD 16//TODO does the 5 need to be 5*8...? Is the ALIGN necessary?

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

	CLASS_SIZE[NUM_CLASSES] = (void*)NULL; 					//entry for MISC - bigger than biggest class

	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment
 *
 *	Sets up metadata for each free group as follows
 *	1 byte = free (0) or allocated (1),
 *	4 bytes = size of chunk as int (including metadata),
 *  3 byte buffer (allowing data to start at a multiple of 8),
 *	~~~~DATA~~~~~,
 *	8 bytes = 00 00 00 00 00 00 00 00/01 = free or alloc
 *
 *	-Searches for appropriate sized block in free list
 *	-If found,
 *	    X marks as used (on both ends!), mark size metadata
 *	    X removes block from free list
 *	    X repairs links of free list
 *	    > takes remaining space and puts it in proper size free list (if possible)
 *	    X return pointer to beginning of usable data
 *	-If unfound
 *	    X mem_sbrk appropriate size
 *	    X mark as used, mark size metadata
 *	    X return pointer to beginning
 */
void *mm_malloc(size_t size)/*{{{*/
{
        printf("mallocing");

	int wasFound = 0; //will be 1 if block is found, 0 if not found
	int newsize = ALIGN(size) + OVERHEAD; //size to be malloc'ed out
	int oldsize; //size of free/sbrk'd block being used
	int i = 0;
	void *returnPointer;
	size_t* LL_ptr;
	size_t* LL_ptr_last;

	while(i < NUM_CLASSES){//find appropriate size class
	    if (size <= CLASS_SIZE[i])
	        break;
	    i++;
	}                      //i now holds index of appropriate size class

	LL_ptr = CLASSES[i];

    while(wasFound == 0) {//search size class for free block, looking for ideal size, this assumes the list is sorted small to large
		if (LL_ptr == NULL) //Linked list did not contain appropriate size
		    break;
		else if (*((int*)(LL_ptr) - 1) >= newsize) {//proper size block found
		    wasFound = 1;
		    *((char*)(LL_ptr) - 5) = 1; //mark as used at front
			*((char*)(LL_ptr) + size + 7) = 1;  //mark as used at back
		    oldsize = *((int*)(LL_ptr) - 1);
		    *((int*)(LL_ptr) - 1) = size; //mark size metadata with size
		    *LL_ptr_last = *LL_ptr; //repaired linked list, even if null.

		    mm_insert((void*)((char*)LL_ptr + size + 8), (oldsize - newsize));

		    returnPointer = ((char*)(LL_ptr) + 3);
		    break;
		} else {
		    LL_ptr_last = LL_ptr;
		    LL_ptr = (size_t*)*LL_ptr;
		}
	}

	if(wasFound == 0) {//need to ask for new memory on the heap
		returnPointer = mem_sbrk(newsize);

		if (returnPointer == (void *)-1) //check for mem_sbrk error
		    return NULL;

		*(char*)returnPointer = 1; //mark as used at front
		returnPointer = ((char*)(returnPointer) + 1);
		*(int*)returnPointer = size; // mark size
		returnPointer = ((char*)(returnPointer) + 7);// now points to start of usable data
		*((char*)(returnPointer) + size) = 1; //mark as used at back
	}
        printf(mm_check());
	return (void*)(returnPointer);

    /* original naive code
	void *p = mem_sbrk(newsize);
	if (p == (void *)-1)
		return NULL;
	else {
		*(size_t *)p = size;
		return (void *)((char *)p + SIZE_T_SIZE);
	} */

}/*}}}*/

/*
 * mm_free - Frees a block by changing metadata to freed and adding to appropriate linked list.
 *
 *	Sets up metadata for each free group as follows
 *	1 byte = free (0) or allocated (1),
 *	4 bytes = size of chunk as int (including metadata),
 *	4/8(size_t) bytes = pointer to next chunk's pointer,
 *	~~~~EMPTY~~~~~,
 *	4 bytes = size,
 *	1 byte = free or alloc
 *
 */
void mm_free(void *argptr)/*{{{*/
{
	int size, csize;
	char* ptr = argptr;

	ptr = (char*)ptr - 3;												//Puts ptr at start of pointer position (no 3byte buffer in free blocks)
	size = *((int*)ptr - 1);

	/*
	 *	Begin Coalescing
	 */
	if(*(ptr - 6) == 0)	{												//the block BEFORE is a free block
		ptr = (char*)ptr - 6;
		csize = *((int*)ptr - 1);										//size of the previous block
		if (csize > 0) {
			ptr = (char*)ptr - 3 - csize;									//sets pointer to pointer portion of previous block
			size = size + csize + 16;
		}
	}

	if(*(ptr + size + 8) == 0)											//the block AFTER is a free block
	{
		csize = *((int*)((char*)ptr + size + 9));						//size of the next block
		if (csize > 0) {
			size = size + csize + 16;
		}
	}

	mm_insert((void*)((char*)argptr - 8), size);
}/*}}}*/

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)/*{{{*/
{
	void *oldptr = ptr;
	void *newptr;
	size_t copySize;

	newptr = mm_malloc(size);
	if (newptr == NULL)
		return NULL;
	copySize = *(int*)((char *)oldptr - 7);
	if (size < copySize)
		copySize = size;
	memcpy(newptr, oldptr, copySize);
	mm_free(oldptr);
	return newptr;
}/*}}}*/

/*
 * mm_check - checks consistency of heap
 *      make a few passes through the data:
 *      for each size free class:
 *          xare all blocks marked as free?
 *          xcheck before and after physically for free blocks (should have been colaesced)
 *
 *      for each block in memory:
 *          as we traverse, any errors in overlap/ underlap will crash the tester
 *          should never get past the max heap byte.
 *          if a block is free:
 *              search for it in the free list
 *          if a block is allocated:
 *           ???
 */
int	mm_check(void)/*{{{*/
{
    size_t* next_ptr;
    size_t* search_ptr;
    int flag = 1;
    int i;
    int size;
    size_t* top_heap = mem_heap_hi();

    printf("checking heap...");

    for(i = 0; i < NUM_CLASSES; i++){//traverse free lists for integrity/*{{{*/
        next_ptr = CLASSES[i];
        if (next_ptr == NULL)
            break;
        while(flag){
            if(*((char*)next_ptr - 5) != 0){//check if block is listed as free
                printf("ERROR: block in free list not marked as free (possibly some other n00b mistake making it look like this)");
                flag = !flag;
                return 1;
            }
            size = *((int*)next_ptr-1);
            if ( (*((char*)next_ptr -6) == 0) || (*((char*)next_ptr + size + 11) == 0) ){//checks if coalesce should have happened but didn't
                printf("ERROR: block immediately before or after free block was free but not coalesced. COALESCEFAIL (possibly some other n00b mistake making it look like this)");
                flag = !flag;
                return 2;
            }
            next_ptr = *(size_t*)next_ptr;
            if (next_ptr == NULL)
                flag = !flag;
        }
    }/*}}}*/

    next_ptr = mem_heap_lo();//start of heap

    while(1){//traverse entire heap for integrity
        if (&next_ptr > *top_heap){
            printf("ERROR: Top of heap exceeded by pointer");
            return 3;
        }

        if (*(char*)next_ptr == 0){//is a free block
            size = *(int*)((char*)next_ptr + 1);

            while(i < NUM_CLASSES){//find appropriate size class
                if (size <= CLASS_SIZE[i])
                    break;
                i++;
            }
            //now need to search for next_ptr in CLASSES[i]
            search_ptr = CLASSES[i];
            while(1){
                if(next_ptr == search_ptr){//has ben found
                    break;
                }
                else if (search_ptr == NULL){//indicates block not found in list
                   printf("ERROR: free block in heap not found in coresponding free list.");
                   return 4;
                }
                search_ptr = *search_ptr;
            }
            next_ptr = ((char*)next_ptr + size);
        }
        else if (*(char*)next_ptr == 1){//is an allocated block
            size = *(char*)((char*)next_ptr + 1);
            next_ptr = ((char*)next_ptr + size);
        }
        else{ //data is corrupted
            printf("free/allocated marker is other than 1/0. DATA CORRUPTED");
            return 5;
        }
        if(*next_ptr == NULL)
            break;
    }
    return 0;
}/*}}}*/


/*
 * mm_insert - given a pointer to the beginning of a free section of data, and a total size,
 *      inserts the data into appropriate list and marks all relevant metadata
 *      xdetermines if size is large enough to be given it's own block (8 + OVERHEAD)
 *      if so:
 *          xdetermines which class to insert k
 *          xupdates metadata (which will include insertion to the appropriate spot on the list
 *              xmarks first byte as free
 *              xnext four hold size (of usable data)
 *              xnext SIZE_T hold pointer
 *              xmarks last byte as free
 *              xprevious four holds size (of usable data)
 *      if not:
 *          xreturn -1
 */

int mm_insert(void* location, int size) {/*{{{*/

    int i = 0;
    int flag = 1;
    size_t* last_ptr = NULL;
    size_t* next_ptr;
    //is size appropriate?
    int minimumsize = 8 + OVERHEAD;
    int usableSize = size - OVERHEAD;

    if (size < minimumsize)
        return -1;
    else{//size is appropriate

	while(i < NUM_CLASSES){//find appropriate size class
	    if (size <= CLASS_SIZE[i])
	        break;
	    i++;
	}   //i now holds index of appropriate size class

        next_ptr = CLASSES[i];
        //find where to insert in list
        //as soon as we find a block with blockSize >= size, insert our block right before that one
        while (flag == 1) {
            if (next_ptr == NULL) {
                flag = !flag;
                if (last_ptr == NULL) {   //list is empty, insert at beginning
                    CLASSES[i] = (char*)location + 5;
					 *(size_t*)((char*)location + 5) = NULL;
                } else {                   //insert at end of list
                    *last_ptr = ((char*)location + 5);
                    *(size_t*)((char*)location + 5) = NULL;
                }
            } else {                                    // list is non-empty, start checking for size
                if (*((int*)next_ptr - 1) >= size) {      //we have found the appropriate spot
                    flag = !flag;
                    if (last_ptr == NULL) {                    //inserting at beginning of list
                        CLASSES[i] =  (char*)location + 5;      //location + 5 bytes is where the pointer to next metadata is stored
                        *(size_t*)((char*)location + 5) = (size_t*)next_ptr;
                    } else {                                  //inserting in middle of list
                        *(size_t*)last_ptr = (char*)location + 5;
                        *((char*)location + 5) = (size_t*)next_ptr;
                    }
                } else {                                   //size is not appropriate, need to try next link
	            last_ptr = next_ptr;
                    next_ptr = *(size_t*)next_ptr;
                }
            }
        }

        *(char*)location = 0; //mark free at front
        location = ((char*)(location) + 1);
        *(int*)location = usableSize; //mark size at front
        location = ((char*)location + (size -2));
        *(char*)location = 0; //mark free at back
        *((int*)location -1) = usableSize; //mark size at back

        return 0;
    }
}/*}}}*/

