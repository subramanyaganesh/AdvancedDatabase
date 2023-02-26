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

// Written by Deshon Langdon
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	maximumBufferPool = 0;
	rightIndex = 0;
	writeCounter = 0;
	totalNumberOfHits = 0;

	bm->mgmtData = (PageF *)calloc(1, numPages * sizeof(PageF));
	PageF *poolPages = (PageF *)bm->mgmtData;

	for (int i = 0; i < numPages; i++)
	{
		poolPages[i].hitRate = 0;
		poolPages[i].data = NULL;
		poolPages[i].pageNum = NO_PAGE;
		poolPages[i].setDirtyBit = 0;
		poolPages[i].currentCounter = 0;
		poolPages[i].frequencyCounter = 0;
	}

	bm->numPages = maximumBufferPool = numPages;
	bm->pageFile = (char *)pageFileName;
	bm->numPages = maximumBufferPool = numPages;
	bm->strategy = strategy;

	return RC_OK;
}

// Written by Deshon Langdon
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	PageF *pFrame = (PageF *)bm->mgmtData;
	forceFlushPool(bm);
	free(pFrame);
	bm->mgmtData = NULL;
	bm->pageFile = NULL;

	return RC_OK;
}

// Written by Deshon Langdon
extern RC forceFlushPool(BM_BufferPool *const bm)
{
	int k = 0;
	SM_FileHandle fh;
	PageF *pFrame = (PageF *)bm->mgmtData;

	if (pFrame != NULL)
	{
		while (k < maximumBufferPool)
		{
			if (pFrame[k].setDirtyBit == 1 && pFrame[k].currentCounter == RC_OK)
			{
				openPageFile(bm->pageFile, &fh);
				writeBlock(pFrame[k].pageNum, &fh, pFrame[k].data);
				writeCounter += 1;
				pFrame[k].setDirtyBit = 0;
			}
			k += 1;
		}
		return RC_OK;
	}
	else
	{
		return RC_ERROR_WHILE_FLUSHING_TO_POOL;
	}
}

// Written by Deshon Langdon
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int j;
	int pageNum = page->pageNum;
	PageF *pFrame = (PageF *)bm->mgmtData;

	if (pFrame != NULL)
	{
		for (j = 0; j < maximumBufferPool; j++)
		{
			if (pFrame[j].pageNum == pageNum)
			{
				if (pFrame[j].pageNum == pageNum)
				{
					pFrame[j].setDirtyBit = 1;
					return RC_OK;
				}

				j += 1;
			}
		}
		return RC_ERROR;
	}
	else
	{
		return RC_ERROR;
	}
}

// Written by Deshon Langdon
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int i = 0;
	PageF *pFrame = (PageF *)bm->mgmtData;
	SM_FileHandle fh;
	int pageNumPointer = page->pageNum;
	for (; i < maximumBufferPool; i++)
	{
		if (pFrame[i].pageNum == pageNumPointer && openPageFile(bm->pageFile, &fh) == RC_OK)
		{
			if (writeBlock(pFrame[i].pageNum, &fh, pFrame[i].data) == RC_OK)
			{
				pFrame[i].setDirtyBit = 0;
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
	PageF *pFrame = (PageF *)bm->mgmtData;
	int pageNumPointer = page->pageNum;
	while (i < maximumBufferPool && pFrame[i].pageNum != pageNumPointer)
		i++;
	pFrame[i].currentCounter -= 1;
	return RC_OK;
}

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
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

// Written by Subramanya Ganesh-done
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	int i = 0;
	PageNumber *fData = malloc(sizeof(PageNumber) * maximumBufferPool);
	PageF *pFrame = (PageF *)bm->mgmtData;
	for (; i < maximumBufferPool; i++)
		fData[i] = pFrame[i].pageNum != -1 ? pFrame[i].pageNum : NO_PAGE;
	return fData;
}

// Written by Subramanya Ganesh-done
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
	int i = 0;
	bool *Flags = malloc(sizeof(bool) * maximumBufferPool);
	PageF *pFrame = (PageF *)bm->mgmtData;
	for (; i < maximumBufferPool; i++)
		Flags[i] = pFrame[i].setDirtyBit == 1 ? true : false;
	return Flags;
}

// Written by Subramanya Ganesh-done
extern int *getFixCounts(BM_BufferPool *const bm)
{
	int i = 0;
	int *fixCounts = malloc(sizeof(int) * maximumBufferPool);
	PageF *pFrame = (PageF *)bm->mgmtData;
	for (; i < maximumBufferPool; i++)
		fixCounts[i] = pFrame[i].currentCounter != -1 ? pFrame[i].currentCounter : 0;
	return fixCounts;
}

// Written by Subramanya Ganesh-done
extern int getNumReadIO(BM_BufferPool *const bm)
{
	return ++rightIndex;
}

// Written by Subramanya Ganesh-done
extern int getNumWriteIO(BM_BufferPool *const bm)
{
	return writeCounter;
}
