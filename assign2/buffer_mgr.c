#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

int maximumBufferPool;
int rightIndex;
int writeCounter;
int totalNumberOfHits;

typedef struct Page
{
	SM_PageHandle data;
	PageNumber pageNum;
	int setDirtyBit;
	int currentCounter;
	int hitRate;
	int frequencyCounter;
} PageF;

// Written by Deshon Langdon-kk
void initialisePage(PageF *poolPages, int numPages)
{
	// iterate to each page and initialise with 0
	for (int i = 0; i < numPages; i++)
	{
		poolPages[i].hitRate = poolPages[i].setDirtyBit = poolPages[i].currentCounter = poolPages[i].frequencyCounter = 0;
		poolPages[i].data = NULL;
		poolPages[i].pageNum = NO_PAGE;
	}
}

// Written by Deshon Langdon-kk
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

// Written by Deshon Langdon-kk
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	forceFlushPool(bm);
	free((PageF *)bm->mgmtData);
	bm->pageFile =bm->mgmtData= NULL;

	return RC_OK;
}

// Written by Deshon Langdon-kk
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

// Written by Deshon Langdon-kk
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

// Written by Deshon Langdon-kk
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

// Written by Subramanya Ganesh-kk
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
extern void FIFO(BM_BufferPool *const bm, PageF *page)
{
	PageF *pFrame = (PageF *)bm->mgmtData;

	int fIndex = rightIndex % maximumBufferPool;
	int fibm = fIndex % maximumBufferPool;
	SM_FileHandle fh;

	while (true)
	{
		if (pFrame[fIndex].currentCounter != 0)
		{
			++fIndex;
			fIndex = (fibm == 0) ? 0 : fIndex;
		}
		else
		{
			if (pFrame[fIndex].setDirtyBit == 1)
			{
				if (openPageFile(bm->pageFile, &fh) == 0)
				{
					if (writeBlock(pFrame[fIndex].pageNum, &fh, pFrame[fIndex].data) == 0)
					{
						++writeCounter;
					}
				}
			}
			pFrame[fIndex].data = page->data;
			pFrame[fIndex].pageNum = page->pageNum;
			pFrame[fIndex].setDirtyBit = page->setDirtyBit;
			pFrame[fIndex].currentCounter = page->currentCounter;
			break;
		}
	}
}

// Written by Subramanya Ganesh
extern void LRU(BM_BufferPool *const bm, PageF *page)
{
	int i = 0;
	PageF *pFrame = (PageF *)bm->mgmtData;
	int LHIndex, LHNum;

	while (i < maximumBufferPool && pFrame[i].currentCounter != 0)
		i++;
	LHIndex = i;
	LHNum = pFrame[i].hitRate;
	++i;
	for (; i < maximumBufferPool; i++)
	{
		if (pFrame[i].hitRate < LHNum)
		{
			LHIndex = i;
			LHNum = pFrame[i].hitRate;
		}
	}

	if (pFrame[LHIndex].setDirtyBit == 1)
	{
		SM_FileHandle fh;
		if (openPageFile(bm->pageFile, &fh) == 0 && writeBlock(pFrame[LHIndex].pageNum, &fh, pFrame[LHIndex].data) == 0)
			writeCounter++;
	}

	pFrame[LHIndex].data = page->data;
	pFrame[LHIndex].setDirtyBit = page->setDirtyBit;
	pFrame[LHIndex].currentCounter = page->currentCounter;
	pFrame[LHIndex].pageNum = page->pageNum;
	pFrame[LHIndex].hitRate = page->hitRate;
}

// Written by Subramanya Ganesh
extern RC assignValues(BM_PageHandle *const page, const PageNumber pageNum, PageF *pageF)
{
	page->pageNum = pageNum;
	page->data = pageF[0].data;
	pageF[0].pageNum = pageNum;
	pageF[0].currentCounter += 1;
	pageF[0].hitRate = 0;
	pageF[0].frequencyCounter = 0;
	rightIndex = 0;
	totalNumberOfHits = 0;
	return RC_OK;
}

// Written by Subramanya Ganesh
extern RC setIfBlockReads(BM_BufferPool *const bm,
						  BM_PageHandle *const page,
						  const PageNumber pageNum,
						  PageF *pageF, int m,
						  bool *ptrIsFull)
{
	page->pageNum = pageNum;
	pageF[m].pageNum = pageNum;
	rightIndex += 1;
	totalNumberOfHits += 1;

	if (bm->strategy == RS_LRU)
		pageF[m].hitRate = totalNumberOfHits;

	page->data = pageF[m].data;
	*ptrIsFull = false;
	pageF[m].currentCounter = 1;
	pageF[m].frequencyCounter = 0;
	return RC_OK;
}

// Written by Subramanya Ganesh
extern RC equalityCondition(BM_BufferPool *const bm,
							BM_PageHandle *const page,
							const PageNumber pageNum,
							PageF *pageF, int m,
							bool *ptrIsFull)
{
	totalNumberOfHits += 1;
	if (bm->strategy == RS_LRU)
		pageF[m].hitRate = totalNumberOfHits;
	pageF[m].currentCounter += 1;
	*ptrIsFull = false;
	page->pageNum = pageNum;
	page->data = pageF[m].data;
	return RC_OK;
}

// Written by Subramanya Ganesh
void readBlockCondition(BM_BufferPool *const bm,
						BM_PageHandle *const page,
						const PageNumber pageNum,
						PageF *pageF, PageF *newPage)
{
	newPage->pageNum = pageNum;
	newPage->setDirtyBit = newPage->frequencyCounter = 0;
	newPage->currentCounter = 1;
	rightIndex += 1;
	totalNumberOfHits += 1;
	page->pageNum = pageNum;
	page->data = newPage->data;

	if (bm->strategy == RS_LRU)
		newPage->hitRate = totalNumberOfHits;

	bm->strategy == RS_FIFO ? FIFO(bm, newPage) : LRU(bm, newPage);
}

// Written by Subramanya Ganesh
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
				  const PageNumber pageNum)
{
	bool isFull = true;
	bool *ptrIsFull = &isFull;
	PageF *pageF = (PageF *)bm->mgmtData;
	SM_FileHandle fh;
	if (pageF[0].pageNum == -1)
	{
		if (openPageFile(bm->pageFile, &fh) != 0)
			return RC_ERROR_IN_OPEN_PAGEFILE;
		pageF[0].data = (SM_PageHandle)malloc(PAGE_SIZE);
		return ensureCapacity(pageNum, &fh) == 0 ? readBlock(pageNum, &fh, pageF[0].data) == 0 ? assignValues(page, pageNum, pageF)
																							   : RC_READ_NON_EXISTING_PAGE
												 : RC_ERROR_WHILE_ENSURE_CAPACITY;
	}
	else
	{
		for (int m = 0; m < maximumBufferPool; m++)
		{
			if (pageF[m].pageNum != -1)
			{
				if (pageF[m].pageNum == pageNum)
					return equalityCondition(bm, page, pageNum, pageF, m, ptrIsFull);
			}

			else
			{
				if (openPageFile(bm->pageFile, &fh) != 0)
					return RC_ERROR_IN_OPEN_PAGEFILE;
				pageF[m].data = (SM_PageHandle)malloc(PAGE_SIZE);
				return readBlock(pageNum, &fh, pageF[m].data) == 0 ? setIfBlockReads(bm, page, pageNum, pageF, m, ptrIsFull) : RC_READ_NON_EXISTING_PAGE;
			}
		}
		if (isFull == true)
		{
			PageF *newPage = (PageF *)malloc(sizeof(PageF));
			if (openPageFile(bm->pageFile, &fh) != 0)
				return RC_ERROR_IN_OPEN_PAGEFILE;
			newPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, newPage->data) == 0 ? readBlockCondition(bm, page, pageNum, pageF, newPage) : RC_READ_NON_EXISTING_PAGE;
		}
		return RC_OK;
	}
}

// Written by Subramanya Ganesh-kk
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	PageNumber *pageData = malloc(sizeof(PageNumber) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		pageData[i] = ((PageF *)bm->mgmtData)[i].pageNum != -1 ? ((PageF *)bm->mgmtData)[i].pageNum : NO_PAGE;
	return pageData;
}

// Written by Subramanya Ganesh-kk
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
	bool *decider = malloc(sizeof(bool) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		decider[i] = ((PageF *)bm->mgmtData)[i].setDirtyBit == 1 ? true : false;
	return decider;
}

// Written by Subramanya Ganesh-kk
extern int *getFixCounts(BM_BufferPool *const bm)
{
	int *defaultCount = malloc(sizeof(int) * maximumBufferPool);
	for (int i = 0; i < maximumBufferPool; i++)
		defaultCount[i] = ((PageF *)bm->mgmtData)[i].currentCounter != -1 ? ((PageF *)bm->mgmtData)[i].currentCounter : 0;
	return defaultCount;
}

// Written by Subramanya Ganesh-kk
extern int getNumReadIO(BM_BufferPool *const bm)
{
	return ++rightIndex;
}

// Written by Subramanya Ganesh-kk
extern int getNumWriteIO(BM_BufferPool *const bm)
{
	return writeCounter;
}
