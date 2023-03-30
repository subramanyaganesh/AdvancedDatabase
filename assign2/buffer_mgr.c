#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

int maximumBufferPool=0;
int rightIndex=0;
int writeCounter=0;
int totalNumberOfHits=0;

typedef struct Page
{
	SM_PageHandle data;
	PageNumber pageNum;
	int setDirtyBit;
	int currentCounter;
	int hitRate;
	int frequencyCounter;
} PageF;

// Written by Deshon Langdon
void initialisePage(PageF *poolPages, int numPages)
{
	// iterate to each page and initialise with 0
	for (int i = 0; i < numPages; i++)
	{
		poolPages[i].setDirtyBit = poolPages[i].hitRate = 0;
		poolPages[i].pageNum = NO_PAGE;
		poolPages[i].data = NULL;
		poolPages[i].frequencyCounter = poolPages[i].currentCounter = 0;
	}
}

// Written by Deshon Langdon
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	// allocate memory
	bm->mgmtData = (PageF *)calloc(1, numPages * sizeof(PageF));
	// initial condition
	maximumBufferPool = rightIndex = writeCounter = totalNumberOfHits = 0;
	initialisePage((PageF *)bm->mgmtData, numPages);
	// assign values
	bm->pageFile = (char *)pageFileName;
	maximumBufferPool = numPages;
	bm->numPages = maximumBufferPool;
	bm->strategy = strategy;
	return RC_OK;
}

// Written by Deshon Langdon
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	forceFlushPool(bm);
	PageF *pg=(PageF *)bm->mgmtData;
	int i= 0;
	while(i < maximumBufferPool)
	{
		if(pg[i].currentCounter != 0)
		{
			return RC_PINNED_PAGES_IN_BUFFER;
		} i++;
	}
	free(pg);
	bm->pageFile = bm->mgmtData = NULL;

	return RC_OK;
}

// Written by Deshon Langdon
extern RC forceFlushPool(BM_BufferPool *const bm)
{
	int k = 0;
	SM_FileHandle fh;
	PageF *pagetFrame = (PageF *)bm->mgmtData;

	if (pagetFrame == NULL)
		return RC_ERROR_WHILE_FLUSHING_TO_POOL;

	while (k < maximumBufferPool)
	{
		if (pagetFrame[k].setDirtyBit == 1 && pagetFrame[k].currentCounter == RC_OK)
		{
			openPageFile(bm->pageFile, &fh);
			writeBlock(pagetFrame[k].pageNum, &fh, pagetFrame[k].data);
			++writeCounter;
			pagetFrame[k].setDirtyBit = 0;
		}
		++k;
	}
	return RC_OK;
}

// Written by Deshon Langdon
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int j;
	int rc = RC_ERROR;
	int pageNum = page->pageNum;
	PageF *pagetFrame = (PageF *)bm->mgmtData;

	if (pagetFrame == NULL)
		return rc;

	for (j = 0; j < maximumBufferPool; j++)
	{
		if (pagetFrame[j].pageNum == pageNum)
		{
			pagetFrame[j].setDirtyBit = 1;
			rc = RC_OK;
			return rc;
		}
	}
	return rc;
}

// Written by Deshon Langdon
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int i = 0;
	PageF *pagetFrame = (PageF *)bm->mgmtData;
	SM_FileHandle fh;
	int pageNumPointer = page->pageNum;
	for (; i < maximumBufferPool; i++)
	{
		if (pagetFrame[i].pageNum == pageNumPointer && openPageFile(bm->pageFile, &fh) == RC_OK)
		{
			if (writeBlock(pagetFrame[i].pageNum, &fh, pagetFrame[i].data) == RC_OK)
			{
				pagetFrame[i].setDirtyBit = 0;
				++writeCounter;
				break;
			}
		}
	}
	return RC_OK;
}

// Written by Subramanya Ganesh
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int i = 0;
	// reach to the point where pagenumber is pointing to the desired page Number
	while (i < maximumBufferPool && ((PageF *)bm->mgmtData)[i].pageNum != page->pageNum)
		i++;
	// decrement the current counter
	--((PageF *)bm->mgmtData)[i].currentCounter;
	return RC_OK;
}

// Written by Subramanya Ganesh
void putPageVal(int i, PageF *pagetFrame, PageF *page, int p)
{
	if (p == 0 || p == 1)
		pagetFrame[i].data = page->data;
	if (p == 0)
		pagetFrame[i].hitRate = page->hitRate;
	if (p == 0 || p == 1)
		pagetFrame[i].pageNum = page->pageNum;
	if (p == 0 || p == 1)
		pagetFrame[i].setDirtyBit = page->setDirtyBit;
	if (p == 0 || p == 1)
		pagetFrame[i].currentCounter = page->currentCounter;
}

// Written by Subramanya Ganesh
extern void FIFO(BM_BufferPool *const bm, PageF *page)
{
	PageF *pageFrame = (PageF *)bm->mgmtData;
	int indexValue = rightIndex % maximumBufferPool;
	int bmfindex = indexValue % maximumBufferPool;
	SM_FileHandle fh;

	while (true)
	{
		if (pageFrame[indexValue].currentCounter != 0)
		{
			++indexValue;
			indexValue = bmfindex == 0 ? 0 : indexValue;
		}
		else
		{
			if (pageFrame[indexValue].setDirtyBit == 1 && openPageFile(bm->pageFile, &fh) == 0 && writeBlock(pageFrame[indexValue].pageNum, &fh, pageFrame[indexValue].data) == 0)
				++writeCounter;

			putPageVal(indexValue, pageFrame, page, 1);
			break;
		}
	}
}

// Written by Subramanya Ganesh
extern void LRU(BM_BufferPool *const bm, PageF *page)
{
	int i = 0;
	PageF *pagetFrame = (PageF *)bm->mgmtData;
	int LHIndex, LHNum;

	while (i < maximumBufferPool && pagetFrame[i].currentCounter != 0)
		i++;
	LHIndex = i;
	LHNum = pagetFrame[i].hitRate;
	++i;
	for (; i < maximumBufferPool; i++)
	{
		if (pagetFrame[i].hitRate < LHNum)
		{
			LHIndex = i;
			LHNum = pagetFrame[i].hitRate;
		}
	}

	if (pagetFrame[LHIndex].setDirtyBit == 1)
	{
		SM_FileHandle fh;
		if (openPageFile(bm->pageFile, &fh) == 0 && writeBlock(pagetFrame[LHIndex].pageNum, &fh, pagetFrame[LHIndex].data) == 0)
			writeCounter++;
	}
	putPageVal(LHIndex, pagetFrame, page, 0);
}

// Written by Subramanya Ganesh
extern RC assignValues(BM_PageHandle *const page, const PageNumber pageNum, PageF *pageFrame)
{

	rightIndex = pageFrame[0].hitRate = pageFrame[0].frequencyCounter = totalNumberOfHits = 0;
	// set the index, number of hits and frequency counter as 0
	page->data = pageFrame[0].data;
	// assign the frames page and the page structure value to pageNumber
	pageFrame[0].pageNum = page->pageNum = pageNum;
	// increment the counter
	++pageFrame[0].currentCounter;
	return RC_OK;
}

// Written by Subramanya Ganesh
extern RC setIfBlockReads(BM_BufferPool *const bm,
						  BM_PageHandle *const page,
						  const PageNumber pageNum,
						  PageF *pageFrame, int m,
						  bool *ptrIsFull)
{

	++rightIndex;
	++totalNumberOfHits;

	if (bm->strategy == RS_LRU)
		pageFrame[m].hitRate = totalNumberOfHits;

	page->pageNum = pageNum;
	*ptrIsFull = false;
	page->data = pageFrame[m].data;
	*ptrIsFull = false;
	pageFrame[m].currentCounter = 1;
	pageFrame[m].frequencyCounter = pageFrame[m].currentCounter - 1;
	pageFrame[m].pageNum = pageNum;

	return RC_OK;
}

// Written by Subramanya Ganesh
extern RC equalityCondition(BM_BufferPool *const bm,
							BM_PageHandle *const page,
							const PageNumber pageNum,
							PageF *pageFrame, int m,
							bool *ptrIsFull)
{
	++totalNumberOfHits;
	if (bm->strategy == RS_LRU)
		pageFrame[m].hitRate = totalNumberOfHits;
	// assign the is full variable to false
	*ptrIsFull = false;
	// set page number to the page structure
	page->pageNum = pageNum;
	// assign data to the page structure
	page->data = pageFrame[m].data;
	*ptrIsFull = false;
	++pageFrame[m].currentCounter;
	return RC_OK;
}

// Written by Subramanya Ganesh
void readBlockCondition(BM_BufferPool *const bm,
						BM_PageHandle *const page,
						const PageNumber pageNum,
						PageF *pageFrame, PageF *newPage)
{
	newPage->pageNum = page->pageNum = pageNum;
	newPage->currentCounter = 1;
	++rightIndex;
	++totalNumberOfHits;
	page->data = newPage->data;
	newPage->setDirtyBit = newPage->frequencyCounter = 0;
	if (bm->strategy == RS_LRU)
		newPage->hitRate = totalNumberOfHits;

	bm->strategy == RS_FIFO ? FIFO(bm, newPage) : LRU(bm, newPage);
}

// Written by Subramanya Ganesh
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
				  const PageNumber pageNum)
{
	PageF *pageFrame = (PageF *)bm->mgmtData;
	bool isFull = true;
	SM_FileHandle fh;
	bool *ptrIsFull = &isFull;
	if (pageFrame[0].pageNum == -1)
	{
		if (openPageFile(bm->pageFile, &fh) != 0)
			return RC_ERROR_IN_OPEN_PAGEFILE;
		pageFrame[0].data = (SM_PageHandle)malloc(PAGE_SIZE);
		return ensureCapacity(pageNum, &fh) == 0 ? readBlock(pageNum, &fh, pageFrame[0].data) == 0 ? assignValues(page, pageNum, pageFrame)
																								   : RC_READ_NON_EXISTING_PAGE
												 : RC_ERROR_WHILE_ENSURE_CAPACITY;
	}
	else
	{
		for (int m = 0; m < maximumBufferPool; m++)
		{
			if (pageFrame[m].pageNum != -1)
			{
				if (pageFrame[m].pageNum == pageNum)
					return equalityCondition(bm, page, pageNum, pageFrame, m, ptrIsFull);
			}

			else
			{
				if (openPageFile(bm->pageFile, &fh) != 0)
					return RC_ERROR_IN_OPEN_PAGEFILE;
				pageFrame[m].data = (SM_PageHandle)malloc(PAGE_SIZE);
				return readBlock(pageNum, &fh, pageFrame[m].data) == 0 ? setIfBlockReads(bm, page, pageNum, pageFrame, m, ptrIsFull) : RC_READ_NON_EXISTING_PAGE;
			}
		}
		if (isFull == true)
		{
			PageF *newPage = (PageF *)malloc(sizeof(PageF));
			if (openPageFile(bm->pageFile, &fh) != 0)
				return RC_ERROR_IN_OPEN_PAGEFILE;
			newPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, newPage->data) == 0 ? readBlockCondition(bm, page, pageNum, pageFrame, newPage) : RC_READ_NON_EXISTING_PAGE;
		}
		return RC_OK;
	}
}

// Written by Subramanya Ganesh
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	PageNumber *pageData = malloc(sizeof(PageNumber) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		pageData[i] = ((PageF *)bm->mgmtData)[i].pageNum != -1 ? ((PageF *)bm->mgmtData)[i].pageNum : NO_PAGE;
	return pageData;
}

// Written by Subramanya Ganesh
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
	bool *decider = malloc(sizeof(bool) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		decider[i] = ((PageF *)bm->mgmtData)[i].setDirtyBit == 1 ? true : false;
	return decider;
}

// Written by Subramanya Ganesh
extern int *getFixCounts(BM_BufferPool *const bm)
{
	int *defaultCount = malloc(sizeof(int) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		defaultCount[i] = ((PageF *)bm->mgmtData)[i].currentCounter != -1 ? ((PageF *)bm->mgmtData)[i].currentCounter : 0;
	return defaultCount;
}

// Written by Subramanya Ganesh
extern int getNumReadIO(BM_BufferPool *const bm)
{
	return ++rightIndex;
}

// Written by Subramanya Ganesh
extern int getNumWriteIO(BM_BufferPool *const bm)
{
	return writeCounter;
}
