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

// buffer
typedef struct
{
	void* head;
} buffer;

// linked list 
typedef struct
{
	int size;
	int occupy;
	pages* pageList;
	buffer* bufferList;
} linkedList;

typedef struct
{
	kma_page_t* page;
	struct pages* next;
} pages;

/************Global Variables*********************************************/
kma_page_t* mainPage = NULL;
/************Function Prototypes******************************************/
void initPage(kma_page_t* page);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if (!mainPage)
	{
		mainPage = get_page();
		initPage(mainPage);
	}

	mainList* main_list = mainPage->ptr;
	linkedList* freeList = NULL;
	int totalSize = size + sizeof(buffer);
	switch(totalSize)
	{
		case 32:
			freeList = &main_list->ll32;
			break;
		case 64:
			freeList = &main_list->ll64;
			break;
		case 128:
			freeList = &main_list->ll128;
			break;
		case 256:
			freeList = &main_list->ll256;
			break;
		case 512:
			freeList = &main_list->ll512;
			break;
		case 1024:
			freeList = &main_list->ll1024;
			break;
		case 2048:
			freeList = &main_list->ll2048;
			break;
		case 4096:
			freeList = &main_list->ll4096;
			break;
		case 8192:
			freeList = &main_list->ll8192;
			break;
		default:
			break;
	}
	if (freeList)
		return getBufferFromFreeList(freeList);
}

// todo: create a init function to init all structure
void initPage(kma_page_t* page)
{
	mainList* main_list = mainPage->ptr;
	main_list->ll32.size = 32;
	main_list->ll64.size = 64;
	main_list->ll128.size = 128;
	main_list->ll256.size = 256;
	main_list->ll512.size = 512;
	main_list->ll1024.size = 1024;
	main_list->ll2048.size = 2048;
	main_list->ll4096.size = 4096;
	main_list->ll8192.size = 8192;
	main_list->ll.size = sizeof(pages) + sizeof(buffer);

	main_list->ll32.occupy = 0;
	main_list->ll64.occupy = 0;
	main_list->ll128.occupy = 0;
	main_list->ll256.occupy = 0;
	main_list->ll512.occupy = 0;
	main_list->ll1024.occupy = 0;
	main_list->ll2048.occupy = 0;
	main_list->ll4096.occupy = 0;
	main_list->ll8192.occupy = 0;
	main_list->ll.occupy = 0;
	
	main_list->ll32.pageList = NULL;
	main_list->ll64.pageList = NULL;
	main_list->ll128.pageList = NULL;
	main_list->ll256.pageList = NULL;
	main_list->ll512.pageList = NULL;
	main_list->ll1024.pageList = NULL;
	main_list->ll2048.pageList = NULL;
	main_list->ll4096.pageList = NULL;
	main_list->ll8192.pageList = NULL;
	main_list->ll.pageList = NULL;

	main_list->ll32.bufferList = NULL;
	main_list->ll64.bufferList = NULL;
	main_list->ll128.bufferList = NULL;
	main_list->ll256.bufferList = NULL;
	main_list->ll512.bufferList = NULL;
	main_list->ll1024.bufferList = NULL;
	main_list->ll2048.bufferList = NULL;
	main_list->ll4096.bufferList = NULL;
	main_list->ll8192.bufferList = NULL;
	main_list->ll.bufferList = NULL;

	main_list->occupy = 0;
	// splite pages into buffer
	int remainedSize = PAGESIZE - sizeof(mainList);
	int bufferCounts = remainedSize / ();
	void* startPoint = page->ptr + sizeof(mainList);
	buffer* bufferHead;
	for (int i = 0; i < bufferCounts; i++)
	{
		bufferHead = (buffer*)(startPoint + i * (sizeof(pages) + sizeof(buffer)));
		bufferHead->head = main_list->ll.bufferList;
		main_list->ll.bufferList = bufferHead;
	}

	addPageToFreeList(page, &main_list->ll);
}

void addPageToFreeList(kma_page_t* page, linkedList* list)
{
	mainList* main = (mainList*)buffer->ptr;
	page* pagel = (page*)getBufferFromFreeList(&main->ll);
	pagel->page = page;

	pagel->next = list->pageList;
	list->pageList = pagel;
}

void* getBufferFromFreeList(linkedlist* list)
{
	if (!list->bufferList)
	{
		addBufferTo(list);
	}
	buffer* buf = list->bufferList;
	list->bufferList = (buffer*)buf->head;
	buf->head = (void*)list;
	list->occupy++;
	mainList* main = bufferList->ptr;
	if (list != &main->ll)
		main->occupy++;
	return (void*)buf + sizeof(buffer);
}

void addBuffersTo(linkedlist* list)
{
	kma_page_t* page = get_page();
	int bufferCount = PAGESIZE / list->size;
	void* pagePoint = page->ptr;
	for (int i = 0; i < bufferCount; i++)
	{
		buffer* buf = (buffer*)(pagePoints + i * list->size);
		buf->head = list->bufferList;
		list->bufferList = buf;
	}
	addPageToFreeList(page, list);
}

void
kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_P2FL
