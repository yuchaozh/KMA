/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
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
 *        suppfor evaluating efficiency of algorithm (wasted memory),
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
#ifdef KMA_P2FL
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

// buffer
typedef struct
{
	void* head;
} buffer;

typedef struct pages_struct
{
	kma_page_t* page;
	struct pages_struct* next;
} pages;


// free list 
typedef struct
{
	int size;
	int occupy;
	pages* pageList;
	buffer* bufferList;
} linkedList;

// main list to track sets of free lists
typedef struct
{
	linkedList ll32;
	linkedList ll64;
	linkedList ll128;
	linkedList ll256;
	linkedList ll512;
	linkedList ll1024;
	linkedList ll2048;
	linkedList ll4096;
	linkedList ll8192;
	linkedList ll;
	int occupy;
} mainList;

/************Global Variables*********************************************/
kma_page_t* mainPage = NULL;
/************Function Prototypes******************************************/
// initialize the main page 
void initPage(kma_page_t* page);
// add buffer to the freelist
void addBuffer(linkedList* list);
// get buffer from the freelist
void* getBuffer(linkedList* list);
// add one page to the freelist
void addPage(kma_page_t* page, linkedList* list);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	// if mainPage not existed, call initPage to initialize it
	if (!mainPage)
	{
		mainPage = get_page();
		initPage(mainPage);
	}
	
	// mainPage can be use
	mainList* main_list = mainPage->ptr;
	linkedList* freeList = NULL;
	void* point = NULL;
	int totalSize = size + sizeof(buffer);

	// choose corresponding free list according to the size requested
	if (totalSize <= 32)
		freeList = &main_list->ll32;
	else if (totalSize <= 64)
		freeList = &main_list->ll64;
	else if (totalSize <= 128)
		freeList = &main_list->ll128;
	else if (totalSize <= 256)
		freeList = &main_list->ll256;
	else if (totalSize <= 512)
		freeList = &main_list->ll512;
	else if (totalSize <= 1024)
		freeList = &main_list->ll1024;
	else if (totalSize <= 2048)
		freeList = &main_list->ll2048;
	else if (totalSize <= 4096)
		freeList = &main_list->ll4096;
	else if (totalSize <= 8192)
		freeList = &main_list->ll8192;
	
	if (freeList)
		point = getBuffer(freeList);
	return point;
}

// initialize the main page
void initPage(kma_page_t* page)
{
	mainList* main_list = mainPage->ptr;
	
	// initialize the mainList object
	main_list->ll32 = (linkedList){32, 0, NULL, NULL};
	main_list->ll64 = (linkedList){64, 0, NULL, NULL};
	main_list->ll128 = (linkedList){128, 0, NULL, NULL};
	main_list->ll256 = (linkedList){256, 0, NULL, NULL};
	main_list->ll512 = (linkedList){512, 0, NULL, NULL};
	main_list->ll1024 = (linkedList){1024, 0, NULL, NULL};
	main_list->ll2048 = (linkedList){2048, 0, NULL, NULL};
	main_list->ll4096 = (linkedList){4096, 0, NULL, NULL};
	main_list->ll8192 = (linkedList){8192, 0, NULL, NULL};
	main_list->ll = (linkedList){sizeof(pages) + sizeof(buffer), 0, NULL, NULL};
	
	// splite pages into buffer
	int remainedSize = PAGESIZE - sizeof(mainList);
	int bufferCounts = remainedSize / (sizeof(pages) + sizeof(buffer));
	void* startPoint = page->ptr + sizeof(mainList);
	int i;
	buffer* bufferHead;
	for (i = 0; i < bufferCounts; i++)
	{
		bufferHead = (buffer*)(startPoint + i * (sizeof(pages) + sizeof(buffer)));
		bufferHead->head = main_list->ll.bufferList;
		main_list->ll.bufferList = bufferHead;
	}
	addPage(page, &main_list->ll);
}

/*
 * get buffer from the free list
 */
void* getBuffer(linkedList* list)
{
	// if no buffer at all, add one and return
	if (!list->bufferList)
	{
		addBuffer(list);
	}
	// choose one buffer to assign
	list->occupy++;
	buffer* buf = list->bufferList;
	list->bufferList = (buffer*)buf->head;
	buf->head = (void*)list;
	mainList* mainlist = mainPage->ptr;
	if (list != &mainlist->ll)
		mainlist->occupy++;
	void* bufferPoint = (void*)buf + sizeof(buffer);
	return bufferPoint;
}

/*
 * add buffer to the free list
 */
void addBuffer(linkedList* list)
{
	kma_page_t* page = get_page();
	void* pagePoint = page->ptr;
	int i;
	int bufferCount = PAGESIZE / list->size;
	// add buffer in the linked list
	for (i = 0; i < bufferCount; i++)
	{
		buffer* buf = (buffer*)(pagePoint + i * list->size);
		buf->head = list->bufferList;
		list->bufferList = buf;
	}
	addPage(page, list);
}

/*
 * add page to the free list
 */
void addPage(kma_page_t* page, linkedList* list)
{
	mainList* mainlist = (mainList*)mainPage->ptr;
	pages* pagelist = (pages*)getBuffer(&mainlist->ll);
	pagelist->page = page;
	pagelist->next = list->pageList;
	list->pageList = pagelist;
}

/*
 * free memory
 */
void
kma_free(void* ptr, kma_size_t size)
{
	buffer* buf = (buffer*)((void*)ptr - sizeof(buffer));
	linkedList* list = (linkedList*)buf->head;
	buf->head = list->bufferList;
	list->bufferList = buf;
	list->occupy--;
	// if there is no memory allocated in the linked list, then free that page
	if (list->occupy == 0)
	{
		list->bufferList = NULL;
		pages* page = list->pageList;
		while (page)
		{
			free_page(page->page);
			page = page->next;
		}
		list->pageList = NULL;
	}

	mainList* mainlist = (mainList*) mainPage->ptr;
	mainlist->occupy--;
	// if there is no memory allocated in the mainPage, then free the mainPage
	if (mainlist->occupy == 0)
	{
		pages* page = mainlist->ll.pageList;
		while (page->next)
		{
			free_page(page->page);
			page = page->next;
		}
		free_page(page->page);
		mainPage = NULL;
	}
}

#endif // KMA_P2FL
