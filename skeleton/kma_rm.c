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
	void* pageHeader;
} pageEntry;

typedef struct
{
	int pageCount;
	int blockCount;
	pageEntry* head;
	void* self;
} pageHeader;

/************Global Variables*********************************************/
kma_page_t* mainPage = NULL;
/************Function Prototypes******************************************/
void initPage(kma_page_t* page);
void addEntry(void* entry, int size);
void findFit(int size);
void deleteEntry(void entry);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	// if the requested size is greater than a page, ignore it
	if ((size + sizeof(void*)) > PAGESIZE)
	{
		//printf("requested size too big");
		return NULL;
	}
	// if there are no allocated page, then initate the page
	if (!mainPage)
	{
		mainPage = get_page();
		initPage(mainPage);
	}
	// find suitable space
	pageEntry* firstFit = FirstFit(size);
	pageHeader* base = BASEADDR(firstFit);
	// increase block count
	base->blockCount++;
	// return block address
	return firstFit;
}

// initialize page
void initPage(kma_page_t* page)
{
	// point page to itself to header can access it
	*((kma_page_t**) page->ptr) = page;
	pageHeader* header = (pageHeader*)(page->ptr);
	// set linked list
	header->head = (pageEntry*)((long int)header + sizeof(pageHeader));
	// add space
	addEntry(page->head, (PAGESIZE - sizeof(pageHeader)));
	// initialize counters
	header->pageCount = 0;
	header->blockCount = 0;
}

//
void addEntry(void* entry, int size)
{
	((pageEntry*)entry)->size = size;
	((pageEntry*)entry)->prev = NULL;

	pageHeader* firstPage = (pageHeader*)(mainPage->ptr);
	void* firstEntry = firstPage->head;
	
	// insert into front of the linked list
	if (entry < firstEntry)
	{
		((pageEntry*)firstEntry)->prev = (pageEntry*)entry;
		((pageEntry*)entry)->next = ((pageEntry*)(firstEntry->head));
		firstPage->head = (pageEntry*)entry;
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
			firstEntry = ((pageEntry*)firstEntry)->next;
		}
		pageEntry* tmp = ((pageEntry*)firstEntry)->next;
		if (tmp != NULL)
		{
			tmp->prev = entry;
		}
		if (tmp == NULL)
		{
			((pageEntry*)firstEntry)->next = entry;
			((pageEntry*)entry)->prev = firstEntry;
			((pageEntry*)entry)->next = tmp;
		}
	}
}

void* firstFit(int size)
{
	int minSize = sizeof(pagEntry*);
	if (size < minSize)
		size = minSize;
	pageHeader* header = (pageHeader*)(mainPage->ptr);
	pageEntry* cursor = (pageEntry*)(header->head);
	while (cursor)
	{
		// no enough size
		if (cursor->size < size)
		{
			cursor = cursor->next;
		}
		// 
		if (cursor->size == size || (cursor->size - size) < minSize)
		{
			deleteEntry(cursor);
			return (void*)cursor;
		}
		else
		{
			addEntry((void*)((long int)cursor + size), (cursor->size - size));
			deleteEntry(cursor);
			return (void*)cursor;
		}
		// no enough space, then add a new page
		kma_page_t* newPage = get_page();
		header->pageCount++;
		initPage(newPage);
		return firstFit(size);
	}
}

void deleteEntry(void* entry)
{
	pageEntry* ptr = (pageEntry*)entry;
	//pageEntry* ptrPrev = ptr->prev;
	//pageEntry* ptrNext = ptr->next;
	
	// only one node
	if (entry->prev == NULL && entry->next == NULL)
	{
		mainPage = NULL;
		return;
	}
	// delete the last node
	else if (entry->next == NULL)
	{
		((pageEntry*)(entry->prev))->next = NULL;
		return;
	}
	// delete the first node
	else if (entry->prev == NULL)
	{
		pageHeader* tmp = (pageHeader*)(mainPage->ptr);
		((pageEntry*)(entry->next))->prev = NULL;
		tmp->head = (void*)(entry->next);
		return;
	}
	else
	{
		((pageEntry*)(entry->prev))->next = entry->next;
		((pageEntry*)(entry->next))->prev = entry->prev;
		return;
	}
}

void
kma_free(void* ptr, kma_size_t size)
{
	pageHeader* page = (pageHeader*)(mainPage->ptr);
	int totalPages = page->pageCount;
	pageHeader* tmp;
	int counter = 0;
	do 
	{
		counter = 0;
		tmp = (((pageHeader*)((long int) mainPage + totalPages * PAGESIZE)));
		pageEntry* tmpEntry = mainPage->head;
		pageEntry* tmp2;
		if (((pageHead*) tmp)->blockCount == 0)
		{
			while (!tmpEntry)
			{
				tmp2 = tmpEntry->next;
				if (tmpEntry->)
			}
		}

	}
	
}

#endif // KMA_RM
