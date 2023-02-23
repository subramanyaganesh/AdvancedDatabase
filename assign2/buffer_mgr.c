#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

int poolMaxPages; 
int indexNextPage;
int writeNumDisk;
int pagesTtlBuffer;

typedef struct Page
{
	SM_PageHandle data;
	PageNumber pageNum;
	bool pageStatus;
	int pageNumAccess;
	int pageNumAccessPool;
	int pageNumFreqAccess;
} Bm_BufferPoolPage;

//Written by Deshon Langdon
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy, void *stratData){
	poolMaxPages = 0;
	indexNextPage = 0;
	writeNumDisk = 0;
	pagesTtlBuffer = 0;

     
	  bm->mgmtData=  (Bm_BufferPoolPage *)calloc (1, numPages * sizeof(Bm_BufferPoolPage));
      Bm_BufferPoolPage* poolPages = (Bm_BufferPoolPage * )bm->mgmtData;
	  
    

    for (int f = 0; f < numPages; f++) {
        poolPages[f].data = NULL;
        poolPages[f].pageNum = NO_PAGE;
        poolPages[f].pageStatus = false;
        poolPages[f].pageNumAccess = 0;
        poolPages[f].pageNumAccessPool = 0;
        poolPages[f].pageNumFreqAccess = 0;
    }


    bm->numPages = poolMaxPages = numPages;
    bm->pageFile = (char *)pageFileName;
    bm->numPages = poolMaxPages= numPages;
    bm->strategy = strategy;
    

    
	
	return RC_OK;
}

//Written by Deshon Langdon
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
            forceFlushPool(bm); 
	free(bufferPoolFrame);
	bm->mgmtData = NULL;
    bm->pageFile = NULL;
    
	return RC_OK;

}

//Written by Deshon Langdon
extern RC forceFlushPool(BM_BufferPool *const bm)
{
	int k = 0;
	SM_FileHandle bufferHandle;
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
	
	if(bufferPoolFrame!= NULL){
		while(k<poolMaxPages){
			if(bufferPoolFrame[k].pageStatus == true && bufferPoolFrame[k].pageNumAccess == RC_OK ){
					openPageFile(bm->pageFile, &bufferHandle);
					writeBlock(bufferPoolFrame[k].pageNum, &bufferHandle, bufferPoolFrame[k].data);
					writeNumDisk += 1;
					bufferPoolFrame[k].pageStatus = false;
				
			}
			k += 1;
		}
		
		return RC_OK;
		
	}
	else{
		return RC_ERROR_IN_FLUSH_POOL;
	}
}

//Written by Deshon Langdon
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int j;
	int pageNum = page->pageNum;
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
	
	if(bufferPoolFrame != NULL){
		for(j=0; j < poolMaxPages && bufferPoolFrame[j].pageNum == pageNum; j++ )
		{
			if(bufferPoolFrame[j].pageNum == pageNum)
			{
				bufferPoolFrame[j].pageStatus = true;
				return RC_OK;
			}
			j+=1;
		}
		return RC_ERROR;
	}
	else{
		return RC_ERROR;
	}
}

//Written by Deshon Langdon
extern RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{	
	int i = 0;
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
	int pageNumPointer = page->pageNum;
	
	
	while(i < poolMaxPages)
	{
		if(bufferPoolFrame[i].pageNum == pageNumPointer)
		{
			bufferPoolFrame[i].pageNumAccess-=1;
			break;		
		}		
		i+=1;
	}
	return RC_OK;
}

//Written by Deshon Langdon
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int a = 0;
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
	SM_FileHandle bufferHandle;
	int pageNumPointer = page->pageNum;
	while ( a < poolMaxPages)
	{
		if(bufferPoolFrame[a].pageNum == pageNumPointer && openPageFile(bm->pageFile, &bufferHandle) == RC_OK){

				if (writeBlock(bufferPoolFrame[a].pageNum, &bufferHandle, bufferPoolFrame[a].data)==RC_OK){
					bufferPoolFrame[a].pageStatus = false;
					writeNumDisk += 1;
					 break;
				}
			}
			a += 1;
		}
		
	
	return RC_OK;
}

//Written by Siddharth Sharma
extern void FIFO(BM_BufferPool *const bm, Bm_BufferPoolPage *page)
{
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *) bm->mgmtData;
	int i = 0;
	int fIndex = indexNextPage % poolMaxPages;
	int fibm =fIndex % poolMaxPages;
	SM_FileHandle bufferHandle;

	while(i < poolMaxPages)
	{
		if(bufferPoolFrame[fIndex].pageNumAccess != 0)
		{
			fIndex+=1;
			fIndex = (fibm == 0) ? 0 : fIndex;
		}
		else
		{
			if(bufferPoolFrame[fIndex].pageStatus == true)
				{
					if(openPageFile(bm->pageFile, &bufferHandle)== 0)
					{
						if (writeBlock(bufferPoolFrame[fIndex].pageNum, &bufferHandle, bufferPoolFrame[fIndex].data)==0)
						{
							writeNumDisk+=1;
						}
					}
				}
				bufferPoolFrame[fIndex].data = page->data;
				bufferPoolFrame[fIndex].pageNum = page->pageNum;
				bufferPoolFrame[fIndex].pageStatus = page->pageStatus;
				bufferPoolFrame[fIndex].pageNumAccess = page->pageNumAccess;
				break;
		}
		i+=0;
	}
}

//Written by Avadhoot Kodag
extern void LRU(BM_BufferPool *const bm, Bm_BufferPoolPage *page)
{
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *) bm->mgmtData;
	int i = 0;
	int LHIndex, LHNum;

	while(i < poolMaxPages){
		if(bufferPoolFrame[i].pageNumAccess == 0)
		{
			LHIndex = i;
			LHNum = bufferPoolFrame[i].pageNumAccessPool;
			break;
		}
		i+=1;
	}


	i = LHIndex + 1;
	while(i < poolMaxPages){
		if(bufferPoolFrame[i].pageNumAccessPool < LHNum)
		{
			LHIndex = i;
			LHNum = bufferPoolFrame[i].pageNumAccessPool;
		}
		i+=1;
	}

	if(bufferPoolFrame[LHIndex].pageStatus == true)
	{
		SM_FileHandle bufferHandle;
		if(openPageFile(bm->pageFile, &bufferHandle)== 0){
			if(writeBlock(bufferPoolFrame[LHIndex].pageNum, &bufferHandle, bufferPoolFrame[LHIndex].data) == 0) {
				writeNumDisk++;
			}
		}
	}

	bufferPoolFrame[LHIndex].data = page->data;
	bufferPoolFrame[LHIndex].pageStatus = page->pageStatus;
	bufferPoolFrame[LHIndex].pageNumAccess = page->pageNumAccess;
	bufferPoolFrame[LHIndex].pageNum = page->pageNum;
	bufferPoolFrame[LHIndex].pageNumAccessPool = page->pageNumAccessPool;
}

//Written by Avadhoot Kodag
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum)
{
	bool isFull = true;
	Bm_BufferPoolPage *bm_BufferPoolPage = (Bm_BufferPoolPage *)bm->mgmtData;
	SM_FileHandle bufferHandle;
	if(bm_BufferPoolPage[0].pageNum == -1)
	{
		if (openPageFile(bm->pageFile, &bufferHandle) == 0){
			bm_BufferPoolPage[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
			if(ensureCapacity(pageNum,&bufferHandle) == 0){
				if(readBlock(pageNum, &bufferHandle, bm_BufferPoolPage[0].data) == 0){
					page->pageNum = pageNum;
					page->data = bm_BufferPoolPage[0].data;
					bm_BufferPoolPage[0].pageNum = pageNum;
					bm_BufferPoolPage[0].pageNumAccess += 1;
					bm_BufferPoolPage[0].pageNumAccessPool = 0;
					bm_BufferPoolPage[0].pageNumFreqAccess = 0;
					indexNextPage = 0;
					pagesTtlBuffer = 0;
					return RC_OK;
				}
				else{
					return RC_READ_NON_EXISTING_PAGE;
				}
			}
			else{
				return RC_ERROR_IN_ENSURE_CAPACITY;
			}
		}
		else{
			return RC_ERROR_IN_OPEN_PAGEFILE;
		}

	}
	else
	{	
		int m = 0;
		while(m < poolMaxPages){
			if(bm_BufferPoolPage[m].pageNum != -1)
			{	
				if(bm_BufferPoolPage[m].pageNum == pageNum)
				{
					pagesTtlBuffer += 1;
					if(bm->strategy == RS_LRU)
						bm_BufferPoolPage[m].pageNumAccessPool = pagesTtlBuffer;
					bm_BufferPoolPage[m].pageNumAccess += 1;
					isFull = false;
					page->pageNum = pageNum;
					page->data = bm_BufferPoolPage[m].data;
					return RC_OK;
				}
			}
			else {
				if (openPageFile(bm->pageFile, &bufferHandle) == 0){
				bm_BufferPoolPage[m].data = (SM_PageHandle) malloc(PAGE_SIZE);
				if(readBlock(pageNum, &bufferHandle, bm_BufferPoolPage[m].data) == 0){
					page->pageNum = pageNum;
					bm_BufferPoolPage[m].pageNum = pageNum;
					indexNextPage += 1;
					pagesTtlBuffer += 1;

					if(bm->strategy == RS_LRU)
						bm_BufferPoolPage[m].pageNumAccessPool = pagesTtlBuffer;

					page->data = bm_BufferPoolPage[m].data;
					isFull = false;
					bm_BufferPoolPage[m].pageNumAccess = 1;
					bm_BufferPoolPage[m].pageNumFreqAccess = 0;
					return RC_OK;
				}
				else{
					return RC_READ_NON_EXISTING_PAGE;
				}
			}
			else{
				return RC_ERROR_IN_OPEN_PAGEFILE;
			}
		}
		m+=1;
	}
		if(isFull == true)
		{
			Bm_BufferPoolPage *newPage = (Bm_BufferPoolPage *) malloc(sizeof(Bm_BufferPoolPage));
			if(openPageFile(bm->pageFile, &bufferHandle) == 0){
				newPage->data = (SM_PageHandle) malloc(PAGE_SIZE);
				if(readBlock(pageNum, &bufferHandle, newPage->data) == 0){
					newPage->pageNum = pageNum;
					newPage->pageStatus = newPage->pageNumFreqAccess = 0;
					newPage->pageNumAccess = 1;
					indexNextPage += 1;
					pagesTtlBuffer += 1;
					page->pageNum = pageNum;
					page->data = newPage->data;

					if(bm->strategy == RS_LRU)
						newPage->pageNumAccessPool = pagesTtlBuffer;

					switch(bm->strategy)
					{
						case RS_FIFO:
							FIFO(bm, newPage);
							break;

						case RS_LRU:
							LRU(bm, newPage);
							break;

						default:
							printf("\nAlgorithm Not Implemented\n");
							break;
					}
				}
				else{
					return RC_READ_NON_EXISTING_PAGE;
				}
			}
			else{
				return RC_ERROR_IN_OPEN_PAGEFILE;
			}
		}		
		return RC_OK;
	}	
}

//Written by Vaishnavi Mule
extern PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	int i = 0;
	PageNumber *fData = malloc(sizeof(PageNumber) * poolMaxPages);
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *) bm->mgmtData;
	while(i<poolMaxPages)
	{
		if(bufferPoolFrame[i].pageNum != -1){
			fData[i] = bufferPoolFrame[i].pageNum;
		}
		else{
			fData[i] = NO_PAGE;
		}
		i+=1;
	}
	return fData;
}

//Written by Vaishnavi Mule
extern bool *getDirtyFlags (BM_BufferPool *const bm)
{
	int i = 0;
	bool *Flags = malloc(sizeof(bool) * poolMaxPages);
	Bm_BufferPoolPage *bufferPoolFrame = (Bm_BufferPoolPage *)bm->mgmtData;
	while(i < poolMaxPages)
	{
		if(bufferPoolFrame[i].pageStatus == true)
			Flags[i] = true;
		else
			Flags[i] = false;
		i+=1;
	}	
	return Flags;
}

//Written by Vaishnavi Mule
extern int *getFixCounts (BM_BufferPool *const bm)
{
	int i = 0;
	int *fixCounts = malloc(sizeof(int) * poolMaxPages);
	Bm_BufferPoolPage *bufferPoolFrame= (Bm_BufferPoolPage *)bm->mgmtData;
	while(i < poolMaxPages)
	{
		if(bufferPoolFrame[i].pageNumAccess != -1)
			fixCounts[i] = bufferPoolFrame[i].pageNumAccess;
		else
			fixCounts[i] = 0;
		i+=1;
	}	
	return fixCounts;
}

//Written by Vaishnavi Mule
extern int getNumReadIO (BM_BufferPool *const bm)
{
	return (indexNextPage + 1);
}

//Written by Vaishnavi Mule
extern int getNumWriteIO (BM_BufferPool *const bm)
{
	return writeNumDisk;
}
