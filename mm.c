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
    // free_list head node
    *(size_t *)(heap_listp + WSIZE) = heap_listp + WSIZE*3;
    // padding
    *(size_t *)(heap_listp + WSIZE*2) = DSIZE | 1;
    // 结尾块
    *(size_t *)(heap_listp + WSIZE*3 + CHUNKSIZE) = 1;
    //空闲块header
    *(size_t *)(heap_listp + WSIZE*3) = CHUNKSIZE;
    //空闲块link_prev_p 
    *(size_t *)(heap_listp + WSIZE*4) = NULL;
    // 空闲块link_next_p
    *(size_t *)(heap_listp + WSIZE*5) = NULL;
    // 空闲块footer

    *(size_t *)(heap_listp + WSIZE*3 + CHUNKSIZE - WSIZE) = CHUNKSIZE;
    return 0;
}

void show_free_list(){
    char *link_list = *(size_t *)((char *)mem_heap_lo() + WSIZE);
    int count = 0;
    while(link_list!=NULL){
        printf("%p->", link_list);

        char *link_prev_hp = *(size_t *)(link_list + WSIZE);
        char *link_next_hp = *(size_t *)(link_list + 2*WSIZE);

        /*----test----*/
        //printf("link_prev: %p, link_next: %p\n",link_prev_hp, link_next_hp);

        link_list = *(size_t *)(link_list+2*WSIZE);
        count++;
        if(count >= 10){
            break;
        }
    }
    printf("NULL\n");
}

/*
 * find_fit - to find the first fit free block 
 * return the free block's header point 
 * if cannot find, return NULL
 */
void *find_fit(size_t newsize){
    char *free_hp = NULL;
    char *heap_listp = *(size_t *)(mem_heap_lo() + WSIZE);
    if(heap_listp == NULL){
        return (void *)free_hp;
    }
    size_t info = *(size_t *)heap_listp;
    size_t size = info & -2;

    //从空闲列表的第一个空闲块开始往下找
    while(heap_listp != NULL){
        // 判断当前块为空闲块且够用
        // printf("current free block address: %p, info: %x\n", heap_listp, info);
        if( (!(info & 1)) && (newsize <= size) ){
            free_hp = heap_listp;
            break;
        }
        heap_listp = *(size_t *)(heap_listp + 2*WSIZE);
        if(heap_listp == NULL){
            break;
        }
        info = *(size_t *)heap_listp;
        size = info & -2;
    }

    return (void *)free_hp;
}

void *coalesce(void *ptr)
{
    char *hp = (char *)ptr - WSIZE;
    size_t size = (*(size_t *)hp) & -2;
    char *next_hp = (char *)hp + size;
    size_t total_size = size;
    char *prev_hp = (char *)hp - WSIZE;
    // 判断上一个是否是空闲块
    if(( (*(size_t *)prev_hp) & 1 ) == 0){

        size_t pre_size = *((size_t *)prev_hp);
        hp = hp - pre_size;
        total_size += pre_size;
        // 将prev_hp的 link_prev_hp 和 link_next_hp 连上
        char *link_prev_hp = *(size_t *)(hp + WSIZE);
        char *link_next_hp = *(size_t *)(hp + 2*WSIZE);

        if(link_prev_hp != NULL){
            // 更改link_prev_hp 的 link_next_hp 为link_next_hp
            *(size_t *)(link_prev_hp + 2*WSIZE) = link_next_hp;
        }
        else
        {
            change_free_list_head(link_next_hp);
        }
        
        if(link_next_hp != NULL){
            // 更改link_next_hp 的 link_prev_hp 为link_prev_hp
            *(size_t *)(link_next_hp + WSIZE) = link_prev_hp;
        }
    }
    
    //判断下一个是否为空闲块
    if(( (*(size_t *)next_hp) & 1 ) == 0 ){
        size_t next_size = *((size_t *)next_hp);
        total_size += next_size;
        // 将next_hp的 link_prev_hp 和 link_next_hp 连上
        char *link_prev_hp = *(size_t *)(next_hp + WSIZE);
        char *link_next_hp = *(size_t *)(next_hp + 2*WSIZE);

        if(link_prev_hp != NULL){
            // 更改link_prev_hp 的 link_next_hp 为link_next_hp
            *(size_t *)(link_prev_hp + 2*WSIZE) = link_next_hp;
        }
        else
        {
            change_free_list_head(link_next_hp);
        }
        
        if(link_next_hp != NULL){
            // 更改link_next_hp 的 link_prev_hp 为link_prev_hp
            *(size_t *)(link_next_hp + WSIZE) = link_prev_hp;
        }
    }

    // 更改空闲块header
    *(size_t *)hp = total_size;
    // 更改空闲块footer
    *(size_t *)(hp + total_size - WSIZE) = total_size;

    // 将hp加入link_list的头部
    char *link_list_head = *(size_t *)((char *)mem_heap_lo() + WSIZE);

    //如果链表头是上一个空闲块，那么合并后的空闲块设置为表头
    if(link_list_head == hp){
        return (void *)hp;
    }
    //如果链表头是下一个空闲块，那么
    if(link_list_head == next_hp){
        *(size_t *)(hp + WSIZE) = NULL;
        *(size_t *)(hp + WSIZE*2) = *(size_t *)(link_list_head + 2*WSIZE);
        change_free_list_head(hp);
        return (void *)hp;
    }

    // 更改hp的两个指针
    *(size_t *)(hp + WSIZE) = NULL;
    *(size_t *)(hp + WSIZE*2) = link_list_head;

    // 更改link_list的link_prev_hp指针
    if(link_list_head != NULL){
        *(size_t *)(link_list_head + WSIZE) = hp;
    }
    change_free_list_head(hp);
    
    return (void *)hp;
}

void change_free_list_head(new_head){
    *(size_t *)((char *)mem_heap_lo() + WSIZE) = new_head;
}

void *extend_heap(){
    //printf("---- start extend heap ----\n");
    // endblock 变为新的CHUNCK的头 CHUNCKSIZE
    char *end_p = (char *)mem_heap_hi() + 1 - WSIZE;
    *(size_t *)end_p = CHUNKSIZE;
    if(((char *)mem_sbrk(CHUNKSIZE)) == (char *)-1) 
        return -1;
    char *new_end_p = end_p + CHUNKSIZE;
    *(size_t *)(new_end_p - WSIZE) = CHUNKSIZE;
    *(size_t *)new_end_p = 1;

    char *free_list_head = *(size_t *)((char *)mem_heap_lo() + WSIZE);
    char *hp = end_p;

    if (free_list_head != NULL)
    {
         hp = (char *)coalesce(end_p + WSIZE);
    }
    else
    {
        *(size_t *)(hp + WSIZE) = NULL;
        *(size_t *)(hp + 2*WSIZE) = NULL;
        change_free_list_head(hp);
    }
      

    //show_free_list();
    
    return (void *)hp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    //printf("---- start malloc ----\n");
    size_t newsize = ALIGN(size + 2*WSIZE);
    // header pointer 确保返回的空闲块要足够大
    char *hp = (char *)find_fit(newsize);
    while(hp == NULL){
        //sbrk分配并更新hp为新分配的大空闲块的header pointer
        if((hp = (char *)extend_heap()) == (char *)-1) 
            return -1;
        if(*(size_t *)hp > newsize){
            break;
        }
        hp = NULL;
    }
    //printf("hp: %p\n", hp);
    size_t free_size = *(size_t *)hp;

    if(newsize <= free_size-4*WSIZE){
        //更改剩余空闲块header
        char *rest_free_block = hp + newsize;
        *(size_t *)(rest_free_block) = free_size - newsize;
        // 更改剩余空闲块footer
        *(size_t *)(hp + free_size - WSIZE) = free_size - newsize;
        // 更改空闲块俩指针
        char *link_prev_hp = *(size_t *)(hp + WSIZE);
        char *link_next_hp = *(size_t *)(hp + 2*WSIZE);

        *(size_t *)(rest_free_block + WSIZE) = link_prev_hp;
        *(size_t *)(rest_free_block + 2*WSIZE) = link_next_hp;

        if(link_prev_hp != NULL){
            // 更改link_prev_hp 的 link_next_hp 为rest_free_block
            *(size_t *)(link_prev_hp + 2*WSIZE) = rest_free_block;
        }
        else
        {
            change_free_list_head(rest_free_block);
        }
        
        if(link_next_hp != NULL){
            // 更改link_next_hp 的 link_prev_hp 为rest_free_block
            *(size_t *)(link_next_hp + WSIZE) = rest_free_block;
        }

        *(size_t *)hp = newsize | 1;
        // 更改分配块footer
        *(size_t *) (hp + newsize - WSIZE) = newsize | 1;
    }
    // 否则该空闲块全部用于分配
    else {
        // 将hp的 link_prev_hp 和 link_next_hp 连上
        char *link_prev_hp = *(size_t *)(hp + WSIZE);
        char *link_next_hp = *(size_t *)(hp + 2*WSIZE);

        if(link_prev_hp != NULL){
            // 更改link_prev_hp 的 link_next_hp 为link_next_hp
            *(size_t *)(link_prev_hp + 2*WSIZE) = link_next_hp;
        }
        else
        {
            change_free_list_head(link_next_hp);
        }
        if(link_next_hp != NULL){
            // 更改link_next_hp 的 link_prev_hp 为link_prev_hp
            *(size_t *)(link_next_hp + WSIZE) = link_prev_hp;
        }

        // 更改分配快footer
        *(size_t *)hp = free_size | 1;
        *(size_t *) (hp + free_size - WSIZE) = free_size | 1;
    }
    
    //printf("malloc: size: %x, address: %p, info: %x\n", newsize, (void *)(hp + WSIZE), *(size_t *)(hp));

    //show_free_list();
    return (void *)(hp + WSIZE);
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //printf("---- start free ----\n");
    coalesce(ptr);

    size_t info = *(size_t *)(ptr - WSIZE);
    //printf("free: size: %x, address: %p\n", info, (void *)(ptr - WSIZE));

    //show_free_list();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */ 
void *mm_realloc(void *ptr, size_t size)
{
    //printf("---- start realloc ----\n");
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
