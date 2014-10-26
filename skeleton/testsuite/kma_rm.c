/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
#ifdef KMA_RM
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
typedef struct
{
	int size;
	void* prev;
	void* next;
} pageEntry;

typedef struct
{
	void* self;
	int pageCount;
	int blockCount;
	pageEntry* head;
} pageHeader;

/************Global Variables*********************************************/
kma_page_t* mainPage = NULL;
/************Function Prototypes******************************************/
void initPage(kma_page_t* page);
void addEntry(void* entry, int size);
pageEntry* firstFit(int size);
void deleteEntry(void* entry);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
//	printf("kma_malloc\n");
	// if the requested size is greater than a page, ignore it
	if ((size + sizeof(void*)) > PAGESIZE)
	{
		return NULL;
	}
	// if there are no allocated page, then initate the page
	if (mainPage == NULL)
	{
		mainPage = get_page();
		initPage(mainPage);
	}
	// find suitable space
	void* firstFit1 = firstFit(size);
	pageHeader* base = BASEADDR(firstFit1);
	// increase block count
	base->blockCount++;
	// return block address
	return firstFit1;
}

// initialize page
void initPage(kma_page_t* page)
{
//	printf("initPage\n");
	// point page to itself to header can access it
	*((kma_page_t**) page->ptr) = page;
	pageHeader* header = (pageHeader*)(page->ptr);

	// initialize counters
	header->pageCount = 0;
	header->blockCount = 0;
	// set linked list
	header->head = (pageEntry*)((long int)header + sizeof(pageHeader));
	// add space
	addEntry(((void*)(header->head)), (PAGESIZE - sizeof(pageHeader)));
}

//
void addEntry(void* entry, int size)
{
//	printf("addEntry\n");
	// initialize new block
	((pageEntry*)entry)->size = size;
	((pageEntry*)entry)->prev = NULL;

	pageHeader* firstPage = (pageHeader*)(mainPage->ptr);
	void* firstEntry = (void*)(firstPage->head);
	
	// insert into front of the linked list
	if (entry < firstEntry)
	{
		((pageEntry*)(firstPage->head))->prev = (pageEntry*)entry;
		((pageEntry*)entry)->next = ((pageEntry*)(firstPage->head));
		firstPage->head = (pageEntry*)entry;
		return;
	}
	// empty
	else if (entry == firstEntry)
	{
		((pageEntry*)entry)->next = NULL;
		return;
	}
	// find position
	else
	{
		while (((pageEntry*)firstEntry)->next != NULL && entry > firstEntry)
		{
			firstEntry = ((void*)(((pageEntry*)firstEntry)->next));
		}
		pageEntry* tmp = ((pageEntry*)firstEntry)->next;
		if (tmp != NULL)
		{
			tmp->prev = entry;
		}
		((pageEntry*)firstEntry)->next = entry;
		((pageEntry*)entry)->prev = firstEntry;
		((pageEntry*)entry)->next = tmp;
	}
}

pageEntry* firstFit(int size)
{
//	printf("firstFir\n");
	int minSize = sizeof(pageEntry);
	if (size < minSize)
		size = minSize;
	pageHeader* header = (pageHeader*)(mainPage->ptr);
	pageEntry* cursor = (pageEntry*)(header->head);
//printf("%d,      %d,  \n",   cursor->size, size);

	while (cursor)
	{
		// no enough size
		if (cursor->size < size)
		{
			cursor = cursor->next;
			continue;
		}
		// fit
		else if (cursor->size == size || (cursor->size - size) < minSize)
		{
			deleteEntry(cursor);
			return (void*)cursor;
		}
		//
		else
		{
			//printf("%d,      %d,  \n",   cursor->size, size);
			addEntry((void*)((long int)cursor + size), (cursor->size - size));
			deleteEntry(cursor);
			return (void*)cursor;
		}
	}
	// no enough space, then add a new page
	kma_page_t* newPage = get_page();
	initPage(newPage);
	header->pageCount++;
	return firstFit(size);
}

void deleteEntry(void* entry)
{
//	printf("deleteEntry\n");
	pageEntry* ptr = (pageEntry*)entry;
	pageEntry* ptrPrev = ptr->prev;
	pageEntry* ptrNext = ptr->next;
	
	// only one node
	if (ptrPrev == NULL && ptrNext == NULL)
	{
//		printf("only one node");
		pageHeader* tmp = (pageHeader*)(mainPage->ptr);
		tmp->head = NULL;
		mainPage = 0;
		return;
	}
	// delete the last node
	else if (ptrNext == NULL)
	{
//		printf("delete the last node");
		ptrPrev->next = NULL;
		return;
	}
	// delete the first node
	else if (ptrPrev == NULL)
	{
//		printf("the first node");
		pageHeader* tmp = (pageHeader*)(mainPage->ptr);
		ptrNext->prev = NULL;
		tmp->head = ptrNext;
		return;
	}
	else
	{
//		printf("else\n");
		pageEntry* tmp1 = ptr->prev;
		pageEntry* tmp2 = ptr->next;
		tmp1->next = tmp2;
		tmp2->prev = tmp1;
		//((pageEntry*)(ptr->prev))->next = ptr->next;
		//((pageEntry*)(ptr->next))->prev = ptr->prev;
		return;
	}
}

void
kma_free(void* ptr, kma_size_t size)
{
//	printf("kma_free: %d\n", size);
	addEntry(ptr, size);
//	printf("here\n");
	pageHeader* baseAdd = BASEADDR(ptr);
	baseAdd->blockCount--;
	pageHeader* firstPage = (pageHeader*)(mainPage->ptr);
	int totalPages = firstPage->pageCount;
	int count = 1;
	pageHeader* lastPage;
	for (; count; totalPages--)
	{
		lastPage = ((pageHeader*)((long int)firstPage + totalPages * PAGESIZE));
		count = 0;
		if (lastPage->blockCount == 0)
		{
			count = 1;
			pageEntry* tmp;
			for (tmp = firstPage->head; tmp != NULL; tmp = tmp->next)
			{
				if (BASEADDR(tmp) == lastPage)
				{
//					printf("continue delete\n");
					deleteEntry(tmp);
				}
			}
			count = 1;
			if (lastPage == firstPage)
			{
//				printf("last page is first page\n");
				count = 0;
				mainPage = NULL;
			}
//			printf("1\n");
			free_page(lastPage->self);
//			printf("2\n");
			if (mainPage != NULL)
			{
				firstPage->pageCount -= 1;
			}
		}
	}
}

#endif // KMA_RM
