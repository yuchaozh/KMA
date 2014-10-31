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
} block;

typedef struct
{
	// must be the first one, point to self, used in free_page()
	void* self;
	int pageCount;
	int blockCount;
	block* head;
} pageHeader;

/************Global Variables*********************************************/
kma_page_t* mainPage = NULL;
/************Function Prototypes******************************************/
void initPage(kma_page_t* page);
void addEntry(void* entry, int size);
block* firstFit(int size);
void deleteEntry(block* entry);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

/**
 * allocate memory
 **/
void*
kma_malloc(kma_size_t size)
{
	// if the requested size is greater than a page, ignore it
	if ((size + sizeof(void*)) > PAGESIZE)
	{
		return NULL;
	}
	// if there are no allocated page, then initate the page
	if (!mainPage)
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

/**
 * initialize page
 **/
void initPage(kma_page_t* page)
{
	// point page to itself to header can access it
	*((kma_page_t**) page->ptr) = page;
	pageHeader* header = (pageHeader*)(page->ptr);
	// initialize counters
	header->pageCount = 0;
	header->blockCount = 0;
	// set linked list
	header->head = (block*)((long int)header + sizeof(pageHeader));
	// add space
	addEntry(((void*)(header->head)), (PAGESIZE - sizeof(pageHeader)));
}

/**
 * Add blocks on the page
 **/ 
void addEntry(void* entry, int size)
{
	// initialize new block
	((block*)entry)->size = size;
	((block*)entry)->prev = NULL;
	pageHeader* firstPage = (pageHeader*)(mainPage->ptr);
	void* firstEntry = (void*)(firstPage->head);
	
	// insert into front of the linked list
	if (entry < firstEntry)
	{
		((block*)(firstPage->head))->prev = (block*)entry;
		((block*)entry)->next = ((block*)(firstPage->head));
		firstPage->head = (block*)entry;
		return;
	}
	// empty
	else if (entry == firstEntry)
	{
		((block*)entry)->next = NULL;
		return;
	}
	// find position
	else
	{
		while (((block*)firstEntry)->next != NULL && entry > firstEntry)
		{
			firstEntry = ((void*)(((block*)firstEntry)->next));
		}
		block* tmp = ((block*)firstEntry)->next;
		if (tmp)
		{
			tmp->prev = entry;
		}
		((block*)firstEntry)->next = entry;
		((block*)entry)->prev = firstEntry;
		((block*)entry)->next = tmp;
	}
}

/**
 * Find first fit block
 **/
block* firstFit(int size)
{
	int minSize = sizeof(block);
	if (size < minSize)
		size = minSize;
	pageHeader* header = (pageHeader*)(mainPage->ptr);
	block* cursor = (block*)(header->head);

	while (cursor)
	{
		// no enough size
		if (cursor->size < size)
		{
			cursor = cursor->next;
			continue;
		}
		// perfect fit
		else if (cursor->size == size || (cursor->size - size) < minSize)
		{
			deleteEntry(cursor);
			return (void*)cursor;
		}
		// fit, add fragement to the linked list
		else
		{
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

/**
 * Delete block
 **/
void deleteEntry(block* entry)
{
	block* ptr = entry;
	block* ptrPrev = ptr->prev;
	block* ptrNext = ptr->next;
	
	// only one node
	if ((!ptrPrev) && (!ptrNext))
	{
		pageHeader* tmp = (pageHeader*)(mainPage->ptr);
		tmp->head = NULL;
		mainPage = NULL;
		return;
	}
	// delete the last node
	else if (!ptrNext)
	{
		ptrPrev->next = NULL;
		return;
	}
	// delete the first node
	else if (!ptrPrev)
	{
		pageHeader* tmp = (pageHeader*)(mainPage->ptr);
		ptrNext->prev = NULL;
		tmp->head = ptrNext;
		return;
	}
	else
	{
		((block*)(ptr->prev))->next = ptr->next;
		((block*)(ptr->next))->prev = ptr->prev;
		return;
	}
}

/**
 * Free memory
 **/
void
kma_free(void* ptr, kma_size_t size)
{
	addEntry(ptr, size);
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
			block* tmp;
			for (tmp = firstPage->head; tmp != NULL; tmp = tmp->next)
			{
				if (BASEADDR(tmp) == lastPage)
				{
					deleteEntry(tmp);
				}
			}
			count = 1;
			if (lastPage == firstPage)
			{
				count = 0;
				mainPage = NULL;
			}
			free_page(lastPage->self);
			if (mainPage)
			{
				firstPage->pageCount--;
			}
		}
	}
}

#endif // KMA_RM
