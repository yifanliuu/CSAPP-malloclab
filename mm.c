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
    "nsdd",
    /* First member's full name */
    "Yifan Liu",
    /* First member's email address */
    "2017202090@ruc.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

/* rounds up to the nearest multiple of ALIGNMENT */
// 对齐
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    char * heap_listp = (char *)mem_heap_lo();
    // 建立空heap
    if((heap_listp = mem_sbrk(4 * WSIZE + CHUNKSIZE)) == (void *)-1) 
        return -1;
    // padding
    *(size_t *)heap_listp = 0; 
    // 序言头
    *(size_t *)(heap_listp + WSIZE) = DSIZE | 1;
    // 序言尾
    *(size_t *)(heap_listp + WSIZE*2) = DSIZE | 1;
    // 结尾块
    *(size_t *)(heap_listp + WSIZE*3 + CHUNKSIZE) = 1;
    //空闲块header
    *(size_t *)(heap_listp + WSIZE*3) = CHUNKSIZE;
    // 空闲块footer
    *(size_t *)(heap_listp + WSIZE*3 + CHUNKSIZE - WSIZE) = CHUNKSIZE;

    return 0;
}

/*
 * find_fit - to find the first fit free block 
 * return the free block's header point 
 * if cannot find, return NULL
 */
void *find_fit(size_t newsize){
    char *free_hp = NULL;
    char *heap_listp = (char *)mem_heap_lo();
    //block list header
    heap_listp += 3*WSIZE;
    size_t info = *(size_t *)heap_listp;
    size_t size = info & -2;
    while(info != 1){
        // 判断当前块为空闲块且够用
        //printf("current free block address: %p, info: %d\n", heap_listp, info);

        if( (!(info & 1)) && (newsize <= size) ){
            free_hp = heap_listp;
            break;
        }
        heap_listp += size;
        info = *(size_t *)heap_listp;
        size = info & -2;
    }
    return (void *)free_hp;
}

void *extend_heap(){
    // endblock 变为新的CHUNCK的头 CHUNCKSIZE
    char *end_p = (char *)mem_heap_hi() + 1 - WSIZE;
    *(size_t *)end_p = CHUNKSIZE;
    if(((char *)mem_sbrk(CHUNKSIZE)) == (char *)-1) 
        return -1;
    char *new_end_p = end_p + CHUNKSIZE;
    *(size_t *)(new_end_p - WSIZE) = CHUNKSIZE;
    *(size_t *)new_end_p = 1;
    return (void *)(end_p + WSIZE);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize = ALIGN(size + 2*WSIZE);
    // header pointer 确保返回的空闲块要足够大
    char *hp = (char *)find_fit(newsize);
    while(hp == NULL){
        //sbrk分配并更新hp为新分配的大空闲块的header pointer
        if((hp = (char *)extend_heap()) == (char *)-1) 
            return -1;
        mm_free(hp);
        hp = (char *)find_fit(newsize);
    }
    size_t free_size = *(size_t *)hp;

    *((size_t *)hp) = newsize | 1;
    *((size_t *) (hp + newsize - WSIZE) ) = newsize | 1;
    if(newsize <= free_size-2){
        //更改剩余空闲块header
        *(size_t *)(hp + newsize) = free_size - newsize;
        // 更改剩余空闲块footer
        *(size_t *)(hp + free_size - WSIZE) = free_size - newsize;
    }
    //printf("malloc: size: %d, address: %p, info: %d\n", newsize, (void *)(hp + WSIZE), *(size_t *)(hp));


    return (void *)(hp + WSIZE);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    char *hp = (char *)ptr - WSIZE;
    size_t size = (*(size_t *)hp) & -2;
    char *next_p = (char *)hp + size;
    size_t total_size = size;
    char *prev_p = (char *)hp - WSIZE;
    // 判断上一个是否是空闲块
    if(( (*(size_t *)prev_p) & 1 ) == 0){
        size_t pre_size = *((size_t *)prev_p);
        hp = hp - pre_size;
        total_size += pre_size;  
    }
    //判断下一个是否为空闲块
    if(( (*(size_t *)next_p) & 1 ) == 0 ){
        size_t next_size = *((size_t *)next_p);
        total_size += next_size;
    }
    // 更改空闲块header
    *((size_t *)hp) = total_size;
    // 更改空闲块footer
    *( (size_t *) ((char *)hp + total_size - WSIZE) ) = total_size;

    //printf("free: size: %d, address: %p\n", size, (void *)hp - WSIZE);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */ 
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr = NULL;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    
    size_t copySize = *(size_t *)((char *)oldptr - WSIZE);
    
    copySize = size < copySize ? size : copySize;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
