#include "storage_mgr.h"
#include "dberror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define READ "r"
#define WRITE "w+"
RC returnCode;
FILE *filePage;

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

// set and get File Descriptor
void setFD(SM_FileHandle *fHandle, int fd)
{
	fHandle->mgmtInfo = malloc(sizeof(int));
	*((int *)fHandle->mgmtInfo) = fd;
}
int getFd(SM_FileHandle *fHandle)
{
	return *((int *)fHandle->mgmtInfo);
}

// Written by Subramanya Ganesh
extern RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
	returnCode = RC_FILE_NOT_FOUND; // initialize the return code

	if (access(fileName, F_OK) != -1) // checks if the the file can be accessed in a F_OK format
	{
		int fd = open(fileName, O_RDWR); // open the file in readonly format
		if (fd != -1)
		{
			struct stat state;
			stat(fileName, &state);								   // returns the meta data of the file
			long file_size = state.st_size;						   // retrive the info about the size of the file stored by the OS
			fHandle->fileName = fileName;						   // saving the file name to the handler
			fHandle->totalNumPages = (int)(file_size / PAGE_SIZE); // saving the number of pages to the handler
			fHandle->curPagePos = 0;							   // setting the current position to the start of the file
			setFD(fHandle, fd);									   // saving the info about fd to mgmtInfo pointer in  fHandle
			printf("Inside openPageFile and value of fd=%d\n", getFd(fHandle));
			return RC_OK;
		}
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC closePageFile(SM_FileHandle *fHandle)
{
	returnCode = RC_FILE_NOT_FOUND;			   // initialze return code with error value
	filePage = fopen(fHandle->fileName, READ); // open file in read mode
	if (filePage != NULL || filePage != 0)	   // check if file opened successfully or not
	{
		if (fclose(filePage) != 0)
			returnCode = RC_ERROR_WHILE_CLOSE;
		else
			returnCode = RC_OK;
	}
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
	// open file in read mode
	filePage = fopen(fHandle->fileName, READ);
	if (filePage != NULL || filePage != 0)
	{
		if (pageNum < fHandle->totalNumPages || pageNum > 0)
		{
			if (fseek(filePage, (pageNum * PAGE_SIZE), SEEK_SET) == 0) // To align the pointer to the offset of file stream
			{
				fread(memPage, sizeof(char), PAGE_SIZE, filePage); // reads data from the filepage(file stream) to the mempage(pointer)
				if (fclose(filePage) != 0)						   // after reading the page check if the file is able to be closed
					returnCode = RC_ERROR_WHILE_CLOSE;			   // printf("\nFacing issues while closing");
				else
					returnCode = RC_OK; // printf("\nSuccessfully able to read the pager");
			}
			else
				returnCode = RC_READ_NON_EXISTING_PAGE; // printf("\nTrying to read a page which is out of bounds");
		}
		else
			returnCode = RC_READ_NON_EXISTING_PAGE; // printf("\nTrying to read a page which is out of bounds");
	}
	else
		returnCode = RC_FILE_NOT_FOUND; // printf("\nFile not found...");

	return returnCode;
}

// Written by Subramanya Ganesh
extern int getBlockPos(SM_FileHandle *fHandle)
{
	// open file in read mode
	filePage = fopen(fHandle->fileName, READ);
	if (filePage == NULL || filePage == 0)
	{
		printf("\nFile not found...");
		return RC_FILE_NOT_FOUND;
	}
	else
	{
		// updating the current position of page
		int curr = fHandle->curPagePos;
		int close = fclose(filePage);
		if (close != 0)
		{
			return RC_ERROR_WHILE_CLOSE;
		}
		else
		{
			return curr;
		}
	}
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
	int previousPageNumber = (fHandle->curPagePos / PAGE_SIZE) - 1;
	return (previousPageNumber > 0) ? readBlock(previousPageNumber, fHandle, memPage) : RC_ERROR_INVALID_PAGENUM;
}

// Written by Subramanya Ganesh
extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// set the pointer to the current page position in order to read the current block
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	return (currentPageNumber > 0) ? readBlock(currentPageNumber, fHandle, memPage) : RC_ERROR_INVALID_PAGENUM;
}

// Written by Subramanya Ganesh
extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	int nextPageNumber = (fHandle->curPagePos / PAGE_SIZE) + 1;
	return nextPageNumber > 0 ? readBlock(nextPageNumber, fHandle, memPage) : RC_ERROR_INVALID_PAGENUM;
	// increment the current page position by 1 in order to read the next block assign this value to the pointer
}

// Written by Subramanya Ganesh
extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// setting the pointer to the last block by subtracting page number by 1
	return (fHandle->totalNumPages - 1 > 0) ? readBlock(fHandle->totalNumPages - 1, fHandle, memPage) : RC_ERROR_INVALID_PAGENUM;
}
//completed till here
// Written by Subramanya Ganesh
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// open file in read mode
	filePage = fopen(fHandle->fileName, "r+");
	if (filePage == NULL || filePage == 0)
	{
		printf("\nFile not found...");
		returnCode = RC_FILE_NOT_FOUND;
	}
	else
	{
		// To check if the pageNum is less than totalNumPages and greater than 0
		if (pageNum < fHandle->totalNumPages || pageNum > 0)
		{
			// To align the pointer with the file stream. seek will be successful if fseek() function returns 0
			int seekStatus = fseek(filePage, (pageNum * PAGE_SIZE), SEEK_SET);
			if (seekStatus != 0)
			{
				returnCode = RC_WRITE_NON_EXISTING_PAGE;
			}
			else
			{
				// To write the content of memPage to pageF stream
				fwrite(memPage, sizeof(char), strlen(memPage), filePage);
				// To set the current page position to the cursor position of the file stream
				fHandle->curPagePos = ftell(filePage);
				// Close the file stream
				int close = fclose(filePage);
				if (close != 0)
				{
					returnCode = RC_ERROR_WHILE_CLOSE;
				}
				else
				{
					returnCode = RC_OK;
				}
			}
		}
		else
		{
			returnCode = RC_WRITE_NON_EXISTING_PAGE;
		}
	}
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// set the pointer to the current page position in order to write the current block
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	if (currentPageNumber > 0)
	{
		fHandle->totalNumPages++;
		return writeBlock(currentPageNumber, fHandle, memPage);
	}
	else
		return RC_ERROR_INVALID_PAGENUM;
}

// Written by Subramanya Ganesh
extern RC appendEmptyBlock(SM_FileHandle *fHandle)
{
	// create an empty block of same size as PAGE SIZE
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	int seekStatus = fseek(filePage, 0, SEEK_END);
	if (seekStatus != 0)
	{
		free(emptyBlock);
		return RC_WRITE_FAILED;
	}
	else
	{
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, filePage);
	}
	free(emptyBlock);
	// update the total number of pages
	fHandle->totalNumPages++;
	returnCode = RC_OK;
	return returnCode;
}

// Written by Subramanya Ganesh
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
	//// Open file stream in append mode to append the data at the end of file.
	filePage = fopen(fHandle->fileName, "a");
	if (filePage == NULL || filePage == 0)
	{
		printf("\nFile not found...");
		returnCode = RC_FILE_NOT_FOUND;
	}
	else
	{
		// check if number of pages exceeds the total number of pages
		while (numberOfPages > fHandle->totalNumPages)
			appendEmptyBlock(fHandle);
		// closing file stream
		fclose(filePage);
		returnCode = RC_OK;
	}
	return returnCode;
}
