/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
typedef struct
{
  void* head;
  void* prev;
  kma_page_t* page;
  unsigned char size;
} buffer_t;

typedef struct free_list_t
{
  unsigned char size;
  struct free_list_t * up;
  buffer_t * start;
} freelist_t;

typedef struct
{
  freelist_t buf32;
  freelist_t buf64;
  freelist_t buf128;
  freelist_t buf256;
  freelist_t buf512;
  freelist_t buf1024;
  freelist_t buf2048;
  freelist_t buf4096;
  freelist_t buf8192;
  int used;
} mainlist_t;

/************Global Variables*********************************************/
kma_page_t * start = NULL;

//#define DEBUG
#ifdef DEBUG
#define DPRINT(x) printf x
#else
#define DPRINT(x)
#endif

/************Function Prototypes******************************************/
//set up admin struct
void
setupMainPage(void);
//returns a buffer for the user
void*
getBufferFromFreelist(freelist_t*);
//returns the buddy of a buffer
void*
getBuddy(void*,int);
//removes a free buffer from said list
void
removeFromFreelist(buffer_t*,freelist_t*);
//free a buffer
void
freeAndMerge(void*);
//returns free buffer count from given freelist
int
freeCount(freelist_t*);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
  /* finds the smalled free list that will satisfy the request size and 
    attempts to grab a buffer from it
  */
  DPRINT(("Entered kma_malloc with request of %i\n",size));
  if(start == NULL){
    //set up admin data
    setupMainPage();    
  }
  void* result = NULL;
  mainlist_t* mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));
  freelist_t* request = NULL;
  int adjusted = size+sizeof(buffer_t);
  if (adjusted <= 32) {
    request = &mainlist->buf32;
  } else if (adjusted <= 64) {
    request = &mainlist->buf64;
  } else if (adjusted <= 128) {
    request = &mainlist->buf128;
  } else if (adjusted <= 256) {
    request = &mainlist->buf256;
  } else if (adjusted <= 512) {
    request = &mainlist->buf512;
  } else if (adjusted <= 1024) {
    request = &mainlist->buf1024;
  } else if (adjusted <= 2048) {
    request = &mainlist->buf2048;
  } else if (adjusted <= 4096) {
    request = &mainlist->buf4096;
  } else if (adjusted <= 8192) {
    request = &mainlist->buf8192;
  }
  if (request != NULL)
    result = getBufferFromFreelist(request);
  
  DPRINT(("Requesting %d bytes\n", rounded_up));

  #ifdef LOGGING
    numrequests++;
    totalRequests += size;
    totalUtil += (float)totalRequests/(float)(page_stats()->num_in_use*PAGESIZE);
    //printf("%f\n",totalUtil);
  #endif

  return result;
}

void
setupMainPage()
{
  /* set everythin up, initialize values */
  DPRINT(("setting up main page\n"));
  start = get_page();
  mainlist_t * mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));

  mainlist->buf32.size = 5;
  mainlist->buf64.size = 6;
  mainlist->buf128.size = 7;
  mainlist->buf256.size = 8;
  mainlist->buf512.size = 9;
  mainlist->buf1024.size = 10;
  mainlist->buf2048.size = 11;
  mainlist->buf4096.size = 12;
  mainlist->buf8192.size = 13;

  mainlist->buf32.up = &mainlist->buf64;
  mainlist->buf64.up = &mainlist->buf128;
  mainlist->buf128.up = &mainlist->buf256;
  mainlist->buf256.up = &mainlist->buf512;
  mainlist->buf512.up = &mainlist->buf1024;
  mainlist->buf1024.up = &mainlist->buf2048;
  mainlist->buf2048.up = &mainlist->buf4096;
  mainlist->buf4096.up = &mainlist->buf8192;
  mainlist->buf8192.up = NULL;

  mainlist->buf32.start = NULL;
  mainlist->buf64.start = NULL;
  mainlist->buf128.start = NULL;
  mainlist->buf256.start = NULL;
  mainlist->buf512.start = NULL;
  mainlist->buf1024.start = NULL;
  mainlist->buf2048.start = NULL;
  mainlist->buf4096.start = NULL;
  mainlist->buf8192.start = NULL;
  mainlist->used = 0;

  buffer_t* buf = start->ptr;
  buf->head = NULL;
  mainlist->buf8192.start = buf;
  buf->prev = NULL;
  kma_malloc(sizeof(mainlist_t));
  mainlist->used = 0;
}

void *
getBufferFromFreelist(freelist_t* list)
{
  /* attempts to return a free buffer from the input list
  if none are free, either requests a new page or makes 
  a recursive call to the next largest size and breaks that 
  into two buffers
  */
  DPRINT(("get buffer of size %i\n",list->size));
  mainlist_t*mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));
  mainlist->used++;
  if (list->start==NULL) {
    if (list->up==NULL){
      kma_page_t* page = get_page();
      //addPage(page);
      buffer_t* buf = (buffer_t*)page->ptr;
      buf->page = page;
      buf->head = (void*)list;
      buf->size = list->size;
      DPRINT(("using list at %p\n",list));
      DPRINT(("head at %p returning %p\n",buf,(void*)buf+sizeof(buffer_t)));
      return ((void*)buf+sizeof(buffer_t));
    }
    else {
      buffer_t* bufb = (buffer_t*)((void*)getBufferFromFreelist(list->up)-sizeof(buffer_t));
      uintptr_t offset = 0x1<<list->size;
      buffer_t* bufa = (buffer_t*)((void*)bufb + offset);
      if(list->start!=NULL)
        list->start->prev = bufa;
      bufa->head = (void*)list->start;
      list->start = bufa;
      bufa->prev = NULL;
      bufa->size = list->size;
      bufb->head = (void*)list;
      bufb->size = list->size;
      return ((void*)bufb+sizeof(buffer_t));
    }
  }
  buffer_t* buf = list->start;
  list->start = buf->head;
  buf->head = (void*)list;
  buf->size = list->size;
  if(list->start!=NULL) {
    list->start->prev = NULL;
  }
  return ((void*)buf+sizeof(buffer_t));
}

void
kma_free(void* ptr, kma_size_t size)
{
  /*
    frees a buffer. free everything and set start to NULL if nothing is being used
  */
  freeAndMerge(ptr);
  mainlist_t* mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t)); 
  // printf("used=%i, 64=%i, 128=%i, 256=%i, ",mainlist->used,freeCount(&mainlist->buf64),freeCount(&mainlist->buf128),freeCount(&mainlist->buf256));
  // printf("512=%i, 1024=%i, 2048=%i, 4096=%i, 8192=%i\n",freeCount(&mainlist->buf512),freeCount(&mainlist->buf1024),freeCount(&mainlist->buf2048),freeCount(&mainlist->buf4096),freeCount(&mainlist->buf8192));
  // printf("%i\n",mainlist->used);
  if (mainlist->used==0) {
    DPRINT(("clear everything"));
    free_page(start);
    start=NULL;
    #ifdef LOGGING
      printf("util ratio = %f\n",totalUtil/(float)numrequests);
    #endif
  }
  #ifdef LOGGING
    totalRequests-=size;
  #endif
}

void
freeAndMerge(void* ptr)
{
  /*
    returns a buffer to its respective free list. if its buddy is free then
    merges the two and makes a recursive call to free the merged buffer 
  */
  DPRINT(("freeing buffer\n"));
  mainlist_t*mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));
  mainlist->used--;
  DPRINT(("head at %p buffer at %p\n",ptr-sizeof(buffer_t),ptr));
  buffer_t* buf = (buffer_t*)((void*)ptr-sizeof(buffer_t));
  freelist_t* list = (freelist_t*)buf->head;
  DPRINT(("buf->head=%p and list=%p\n",buf->head,list));

  DPRINT(("merge or return to list\n"));
  buffer_t* bud = (buffer_t*) getBuddy((void*)buf,list->size);
  //printf("i am %p and my buddy is %p\n",buf,bud);

  if ((bud->head==buf->head)|(list->up==NULL)|(buf->size!=bud->size))  //bud is still used so just return buf to list
  {
    //printf("return to list of size %i\n",buf->size);
    if (list->up==NULL){
      free_page(buf->page);
      //removePage((void*)buf);
    }
    else{
      if(list->start!=NULL)
        list->start->prev = buf;
      buf->head = (void*)list->start;
      list->start = buf;
      buf->prev = NULL;
    }
  }
  else  //they have to be the same size
  {
    //printf("merging two of size %i\n",buf->size);
    removeFromFreelist(bud,list);
    if (bud<buf) {
      buffer_t* temp = buf;
      buf = bud;
      bud = temp;
    }
    buf->head = list->up;
    buf->size++;
    freeAndMerge((void*)buf+sizeof(buffer_t));
  }
}

void*
getBuddy(void*buf,int size)
{
  /*
    returns the address of the buffer's buddy
  */
  uintptr_t flipper = 1<<size;
  uintptr_t address = (uintptr_t)buf;
  return (void*)(address^flipper);
}
void
removeFromFreelist(buffer_t* buf, freelist_t* list)
{
  /*
    removes a buffer from its free list
  */
  buffer_t* next = (buffer_t*)buf->head;
  buffer_t* prev = (buffer_t*)buf->prev;
  if (list->start==buf) {
    list->start = next;
  }
  if (next!=NULL){
    next->prev = prev;
  }
  if (prev!=NULL){
    prev->head = next;
  }
}

int
freeCount(freelist_t* list)
{
  /*
    counts the number of free buffers in a free list
  */
  int count = 0;
  buffer_t* buf = list->start;
  while (buf!=NULL) {
    count++;
    buf = (buffer_t*)buf->head;
  }
  return count;
}

#endif // KMA_BUD
