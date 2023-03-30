#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ "r"
#define WRITE "w+"
RC returnCode;
FILE *filePage;
static int i=0;

extern void initStorageManager(void)
{
	printf("Initializing Storage Manager\nBy\n ");
	printf("Subramanya Ganesh (A20516250) \n");
	filePage = NULL; // set file pointer to NULL
}

// Written by Subramanya Ganesh
extern RC createPageFile(char *fileName)
{
	returnCode = RC_FILE_NOT_FOUND;	   // initialize the returnCode
	filePage = fopen(fileName, WRITE); // open the file in write mode
	if (filePage != NULL)
	{
		SM_PageHandle pageBlock = (SM_PageHandle)malloc(PAGE_SIZE * sizeof(char)); // assign a blocks of memory blocks
		memset(pageBlock, '\0', PAGE_SIZE);										   // initialize the memory block with null character
		fwrite(pageBlock, sizeof(char), PAGE_SIZE, filePage);					   // insert empty data into the pageBlock
		fclose(filePage);														   // deallocating the space in the file
		free(pageBlock);														   // freeing the allocated space to avoid memory leak
		returnCode = RC_OK;
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC openPageFile(char *fileName,SM_FileHandle*fHandle){
	//open file in read mode
	filePage = fopen(fileName,READ);
	//check if file opened successfully or not
    if(filePage==NULL){
    	return RC_FILE_HANDLE_NOT_INIT;
    }
    //set the pointer to the starting position
    int start = fseek(filePage,0,SEEK_SET);
    //set filehandle's file name
    fHandle->fileName = fileName;
    //set filehandle's current page position
    fHandle->curPagePos = start;
    //set filehandle's total number of pages
    fHandle->totalNumPages = ftell(filePage)+1;
	printf("Inside openPageFile and value of fd=%d\n", ++i);
    int close = fclose(filePage);
	if(close != 0){
		returnCode = RC_ERROR_WHILE_CLOSE;
	}
	else{
		returnCode = RC_OK;
	}
    return returnCode;
}

// Written by Subramanya Ganesh
extern RC closePageFile(SM_FileHandle *fHandle)
{
	returnCode = RC_FILE_NOT_FOUND;									   // initialze return code with error value
	filePage = fopen(fHandle->fileName, READ);						   // open file in read mode
	if (filePage != NULL || filePage != 0)							   // check if file opened successfully or not
		return (fclose(filePage) != 0) ? RC_ERROR_WHILE_CLOSE : RC_OK; // validate if the file is able to be closed and return the error code accordingly
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC destroyPageFile(char *fileName)
{
	returnCode = RC_FILE_NOT_FOUND;
	filePage = fopen(fileName, READ);	   // open file in read mode
	if (filePage != NULL || filePage != 0) // check if file opened successfully or not
	{
		if (fclose(filePage) == 0)	   // close the file and check if was successful
			if (remove(fileName) == 0) // remove the file and check if it was successful
				returnCode = RC_OK;
			else
				returnCode = RC_ERROR_ON_DESTROY;
		else
			returnCode = RC_ERROR_WHILE_CLOSE;
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	returnCode = RC_READ_NON_EXISTING_PAGE; // printf("Initializing Trying to read a page which is out of bounds");
	// open file in read mode
	filePage = fopen(fHandle->fileName, READ);
	if (filePage != NULL || filePage != 0)
	{
		if (pageNum < fHandle->totalNumPages || pageNum > 0)
		{
			if (fseek(filePage, (pageNum * PAGE_SIZE), SEEK_SET) == 0) // To align the pointer to the offset of file stream
			{
				fread(memPage, sizeof(char), PAGE_SIZE, filePage); // reads data from the filepage(file stream) to the mempage(pointer)
				returnCode = (fclose(filePage) != 0) ? RC_ERROR_WHILE_CLOSE : RC_OK;
				// after reading the page check if the file is able to be closed ? printf("\nFacing issues while closing"):printf("\nSuccessfully able to read the pager");
			}
		}
	}
	else
		returnCode = RC_FILE_NOT_FOUND; // printf("\nFile not found...");

	return returnCode;
}

// Written by Subramanya Ganesh
extern int getBlockPos(SM_FileHandle *fHandle)
{
	returnCode = RC_FILE_NOT_FOUND;			   // initialising the return code with File not present
	filePage = fopen(fHandle->fileName, READ); // open file in read mode
	if (filePage != NULL || filePage != 0)
	{
		int curr = fHandle->curPagePos;								// updating the current position of page
		return fclose(filePage) != 0 ? RC_ERROR_WHILE_CLOSE : curr; // check if the file has errors while closing then return error else return block position
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	return readBlock(0, fHandle, memPage); // return the first block '0' from the mempage
}

// Written by Subramanya Ganesh
extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// set the pointer to the current page position and subtract by 1 in order to read the previous block
	int previousBlock = (fHandle->curPagePos / PAGE_SIZE) - 1;
	return (previousBlock > 0) ? readBlock(previousBlock, fHandle, memPage) : RC_READ_NON_EXISTING_PAGE;
}

// Written by Subramanya Ganesh
extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// assign the pointer to the current location
	int currentPageBlock = fHandle->curPagePos / PAGE_SIZE;
	return (currentPageBlock > 0) ? readBlock(currentPageBlock, fHandle, memPage) : RC_READ_NON_EXISTING_PAGE;
}

// Written by Subramanya Ganesh
extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int nextBlock = (fHandle->curPagePos / PAGE_SIZE) + 1;
	return nextBlock > 0 ? readBlock(nextBlock, fHandle, memPage) : RC_READ_NON_EXISTING_PAGE;
	// increment the current page position by 1 in order to read the next block assign this value to the pointer
}

// Written by Subramanya Ganesh
extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// assign the pointer to the previous block read by subtracting the totalnumber of pages by 1
	return (fHandle->totalNumPages - 1 > 0) ? readBlock(fHandle->totalNumPages - 1, fHandle, memPage) : RC_ERROR_INVALID_PAGENUM;
}
//   Written by Subramanya Ganesh
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	returnCode = RC_WRITE_NON_EXISTING_PAGE;   // initialize return code
	filePage = fopen(fHandle->fileName, "r+"); // using r+ since we dont want to null the existing file wich is done in
	if (filePage != NULL || filePage != 0)
	{
		// To check if the pageNum is less than totalNumPages and greater than 0
		if (pageNum < fHandle->totalNumPages || pageNum > 0)
		{
			// To align the pointer with the file stream seek will be successful if fseek() function returns 0
			int seekStatus = fseek(filePage, (pageNum * PAGE_SIZE), SEEK_SET);
			if (seekStatus == 0)
			{
				printf("------------------------INSIDE SEEKSTATUS----------------------------\n");
				fwrite(memPage, sizeof(char), strlen(memPage), filePage);			 // To write the content of memPage to pageF stream
				fHandle->curPagePos = ftell(filePage);								 // To set the current page position to the cursor position of the file stream
				returnCode = (fclose(filePage) != 0) ? RC_ERROR_WHILE_CLOSE : RC_OK; // check and Close the file stream
			}
		}
	}
	else
	{
		returnCode = RC_FILE_NOT_FOUND; // printf("\nFile not found...");
	}
	return returnCode;
}

//   Written by Subramanya Ganesh
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	returnCode = RC_ERROR_INVALID_PAGENUM;
	int currentBlockNumber = fHandle->curPagePos / PAGE_SIZE; // assign the pointer to current location
	if (currentBlockNumber > 0)
	{
		++fHandle->totalNumPages;
		returnCode = writeBlock(currentBlockNumber, fHandle, memPage);
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC appendEmptyBlock(SM_FileHandle *fHandle)
{
	returnCode = RC_WRITE_FAILED;											   // initialize returncode
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char)); // create an empty block of same size as PAGE SIZE
	int seekStatus = fseek(filePage, 0, SEEK_END);
	if (seekStatus == 0)
	{
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, filePage);
		fHandle->totalNumPages++; // update the total number of pages
		free(emptyBlock);
		returnCode = RC_OK;
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
	returnCode = RC_FILE_NOT_FOUND;
	filePage = fopen(fHandle->fileName, "a"); // Open file stream in append mode to append the data at the end of file.
	if (filePage != NULL || filePage != 0)
	{
		while (numberOfPages > fHandle->totalNumPages) // check if number of pages exceeds the total number of pages
			appendEmptyBlock(fHandle);
		returnCode = fclose(filePage) != 0 ? RC_ERROR_WHILE_CLOSE : RC_OK; // check if the file has errors while closing then return error else return block position
	}
	return returnCode;
}
