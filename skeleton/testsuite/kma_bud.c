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
  freelist_t buffer_32;
  freelist_t buffer_64;
  freelist_t buffer_128;
  freelist_t buffer_256;
  freelist_t buffer_512;
  freelist_t buffer_1024;
  freelist_t buffer_2048;
  freelist_t buffer_4096;
  freelist_t buffer_8192;
  int used;
} mainlist_t;

/************Global Variables*********************************************/
kma_page_t * start = NULL;


/************Function Prototypes******************************************/
//set up admin struct
void initPage(void);
//returns a buffer for the user
void* getBuffer(freelist_t*);
//returns the buddy of a buffer
void* getBuddy(void*,int);
//removes a free buffer from said list
void removeFromFreelist(buffer_t*,freelist_t*);
//free a buffer
void freeAndMerge(void*);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{
  // printf("start kma allocate\n");
  if(start == NULL){
    //set up admin data
    initPage();
  }
  void* result = NULL;
  mainlist_t* mainlist = (mainlist_t*)((void*)start->ptr + sizeof(buffer_t));
  freelist_t* request = NULL;
  int adjusted = size + sizeof(buffer_t);
  if (adjusted <= 32)
    request = &mainlist->buffer_32;
  else if (adjusted <= 64)
    request = &mainlist->buffer_64;
  else if (adjusted <= 128)
    request = &mainlist->buffer_128;
  else if (adjusted <= 256)
    request = &mainlist->buffer_256;
  else if (adjusted <= 512)
    request = &mainlist->buffer_512;
  else if (adjusted <= 1024)
    request = &mainlist->buffer_1024;
  else if (adjusted <= 2048)
    request = &mainlist->buffer_2048;
  else if (adjusted <= 4096)
    request = &mainlist->buffer_4096;
  else if (adjusted <= 8192)
    request = &mainlist->buffer_8192;

  if (request != NULL)
    result = getBuffer(request);

  return result;
}

void kma_free(void* ptr, kma_size_t size)
{
  freeAndMerge(ptr);
  mainlist_t* mainlist = (mainlist_t*)((void*)start->ptr + sizeof(buffer_t));
  if (mainlist->used == 0) {
    free_page(start);
    start = NULL;
  }
}

void initPage()
{
  /* set everythin up, initialize values */
  start = get_page();
  mainlist_t * mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));

  mainlist->buffer_32.size = 5;
  mainlist->buffer_64.size = 6;
  mainlist->buffer_128.size = 7;
  mainlist->buffer_256.size = 8;
  mainlist->buffer_512.size = 9;
  mainlist->buffer_1024.size = 10;
  mainlist->buffer_2048.size = 11;
  mainlist->buffer_4096.size = 12;
  mainlist->buffer_8192.size = 13;

  mainlist->buffer_32.up = &mainlist->buffer_64;
  mainlist->buffer_64.up = &mainlist->buffer_128;
  mainlist->buffer_128.up = &mainlist->buffer_256;
  mainlist->buffer_256.up = &mainlist->buffer_512;
  mainlist->buffer_512.up = &mainlist->buffer_1024;
  mainlist->buffer_1024.up = &mainlist->buffer_2048;
  mainlist->buffer_2048.up = &mainlist->buffer_4096;
  mainlist->buffer_4096.up = &mainlist->buffer_8192;
  mainlist->buffer_8192.up = NULL;

  mainlist->buffer_32.start = NULL;
  mainlist->buffer_64.start = NULL;
  mainlist->buffer_128.start = NULL;
  mainlist->buffer_256.start = NULL;
  mainlist->buffer_512.start = NULL;
  mainlist->buffer_1024.start = NULL;
  mainlist->buffer_2048.start = NULL;
  mainlist->buffer_4096.start = NULL;
  mainlist->buffer_8192.start = NULL;
  mainlist->used = 0;

  buffer_t* buf = start->ptr;
  buf->head = NULL;
  mainlist->buffer_8192.start = buf;
  buf->prev = NULL;
  kma_malloc(sizeof(mainlist_t));
  mainlist->used = 0;
}

void *getBuffer(freelist_t* list)
{
  mainlist_t* mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));
  mainlist->used++;
  if (list->start == NULL){
    if (list->up == NULL){
      // get a new page
      kma_page_t* page = get_page();
      buffer_t* buf = (buffer_t*)page->ptr;
      buf->page = page;
      buf->head = (void*)list;
      buf->size = list->size;
      return ((void*)buf + sizeof(buffer_t));
    }
    else {
      buffer_t* bufb = (buffer_t*)((void*)getBuffer(list->up)-sizeof(buffer_t));
      uintptr_t offset = 0x1 << list->size;
      buffer_t* bufa = (buffer_t*)((void*)bufb + offset);
      if(list->start != NULL)
        list->start->prev = bufa;
      bufa->head = (void*)list->start;
      list->start = bufa;
      bufa->prev = NULL;
      bufa->size = list->size;
      bufb->head = (void*)list;
      bufb->size = list->size;
      return ((void*)bufb + sizeof(buffer_t));
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

void freeAndMerge(void* ptr)
{
  mainlist_t*mainlist = (mainlist_t*)((void*)start->ptr+sizeof(buffer_t));
  mainlist->used--;
  buffer_t* buf = (buffer_t*)((void*)ptr-sizeof(buffer_t));
  freelist_t* list = (freelist_t*)buf->head;

  buffer_t* bud = (buffer_t*) getBuddy((void*)buf,list->size);

  if ((bud->head == buf->head) || (list->up == NULL) || (buf->size != bud->size))  //bud is still used so just return buf to list
  {
    if (list->up == NULL){
      free_page(buf->page);
    }
    else{
      if(list->start != NULL)
        list->start->prev = buf;
      buf->head = (void*)list->start;
      list->start = buf;
      buf->prev = NULL;
    }
  }
  else  //same size
  {
    removeFromFreelist(bud,list);
    if (bud < buf) {
      buffer_t* temp = buf;
      buf = bud;
      bud = temp;
    }
    buf->head = list->up;
    buf->size++;
    freeAndMerge((void*)buf+sizeof(buffer_t));
  }
}

void* getBuddy(void*buf,int size)
{
  // returns the address of the buffer's buddy
  uintptr_t flipper = 1<<size;
  uintptr_t address = (uintptr_t)buf;
  return (void*)(address^flipper);
}

void removeFromFreelist(buffer_t* buf, freelist_t* list)
{
  // removes a buffer from its free list
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

#endif // KMA_BUD
