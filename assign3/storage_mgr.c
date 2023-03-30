//Assignment-1 Storage manager
//Group Members
//Avadhoot Kodag
//Siddharth Sharma
//Vaishnavi Mule


#include "storage_mgr.h"
#include "dberror.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>


#define FILE_READ_PERMISSION "r"
#define FILE_WRITE_PERMISSION "w+"
#define FILE_READ_WRITE_PERMISSION "r+"
RC returnCode;
FILE *pageF;

extern void initStorageManager(void){
	pageF = NULL; //set file pointer to NULL
}

//Written by Avadhoot Kodag
extern RC createPageFile(char *fileName){
	//open the file stream in write and read mode
    pageF = fopen(fileName, FILE_WRITE_PERMISSION);
    if(pageF == NULL){
    	returnCode = RC_FILE_NOT_FOUND;
    }
    else{
    	//create empty page
		SM_PageHandle emptyPage = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
		//write empty page
		fwrite(emptyPage,sizeof(char),PAGE_SIZE,pageF);
		//close page file
		fclose(pageF);
		//free allocated space
		free(emptyPage);
		returnCode = RC_OK;
    	}
    return returnCode;
}


extern RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	pageF = fopen(fileName, FILE_READ_PERMISSION);
	if(pageF == NULL) {
		returnCode = RC_FILE_HANDLE_NOT_INIT;
	} else {
		fHandle->fileName = fileName;
		struct stat fileInfo;
		fHandle->curPagePos = 0;
		if(fstat(fileno(pageF), &fileInfo) >= 0){
			fHandle->totalNumPages = fileInfo.st_size/ PAGE_SIZE;
			if(fclose(pageF) != 0)
				returnCode = RC_ERROR_ON_CLOSE;
			else
				returnCode = RC_OK;
		}
		else
			returnCode = RC_ERROR;
	}
	return returnCode;
}

//Written by Avadhoot Kodag
extern RC closePageFile (SM_FileHandle *fHandle) {
	//open file in read mode
	pageF = fopen(fHandle->fileName, FILE_READ_PERMISSION);
	//check if file opened successfully or not
	if(pageF == NULL || pageF == 0)
    {
		printf("\nFile not found...");
		returnCode = RC_FILE_NOT_FOUND;
	}
	else{
		//close the page file
		int close = fclose(pageF);
		if(close != 0){
			returnCode = RC_ERROR_ON_CLOSE;
		}
		else{
			//printf("\nFile Close operation Successful...");
			returnCode = RC_OK;
		}
	}
    return returnCode;
}

//Written by Avadhoot Kodag
extern RC destroyPageFile (char *fileName) {
	//printf("Inside destroy");
	//open file in read mode
	pageF = fopen(fileName, FILE_READ_PERMISSION);
	//check if file opened successfully or not
	if(pageF == NULL || pageF == 0)
	{
		printf("\nFile not found...");
		returnCode = RC_FILE_NOT_FOUND;
	}
	else{
		//close the page file
		int close = fclose(pageF);
		if(close != 0){
			returnCode = RC_ERROR_ON_CLOSE;
		}
		else{
			//remove the file
			int rem = remove(fileName);
			if(rem != 0){
				returnCode = RC_ERROR_ON_DESTROY;
			}
			else{
				returnCode = RC_OK;
			}
		}
	}
	return returnCode;
}


//Written by Siddharth Sharma
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//open file in read mode
	pageF = fopen(fHandle->fileName, FILE_READ_PERMISSION);
	if(pageF == NULL || pageF == 0)
	{
			printf("\nFile not found...");
			returnCode = RC_FILE_NOT_FOUND;
	}
	else{
		if (pageNum < fHandle->totalNumPages || pageNum > 0)
		{
			//To align the pointer with the file stream
			int seekStatus = fseek(pageF, (pageNum * PAGE_SIZE), SEEK_SET);
			if(seekStatus != 0) {
				returnCode = RC_READ_NON_EXISTING_PAGE;
			}
			else {
				fread(memPage, sizeof(char), PAGE_SIZE, pageF);
				int close = fclose(pageF);
				if(close != 0){
					returnCode = RC_ERROR_ON_CLOSE;
				}
				else{
					returnCode = RC_OK;
				}
			}
		}
		else{
			returnCode = RC_READ_NON_EXISTING_PAGE;
		}
	}
	return returnCode;
}

//Written by Siddharth Sharma
extern int getBlockPos (SM_FileHandle *fHandle) {
	//open file in read mode
	pageF = fopen(fHandle->fileName, FILE_READ_PERMISSION);
	if(pageF == NULL || pageF == 0)
	{
		printf("\nFile not found...");
		return RC_FILE_NOT_FOUND;
	}
	else{
		//updating the current position of page
		int curr = fHandle->curPagePos;
		int close = fclose(pageF);
			if(close != 0){
				return RC_ERROR_ON_CLOSE;
			}
			else{
				return curr;
			}
	}
}

//Written by Siddharth Sharma
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	int first = 0;
	//assigning the values of the parameters
	returnCode = readBlock(first, fHandle, memPage);
	return returnCode;
}

//Written by Siddharth Sharma
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//set the pointer to the current page position and subtract by 1 in order to read the previous block
	int previousPageNumber = (fHandle->curPagePos / PAGE_SIZE) - 1;
	if(previousPageNumber > 0)
		return readBlock(previousPageNumber, fHandle, memPage);
	else
		return RC_ERROR_INVALID_PAGENUM;
}

//Written by Siddharth Sharma
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//set the pointer to the current page position in order to read the current block
	int currentPageNumber = fHandle->curPagePos / PAGE_SIZE;
	if(currentPageNumber > 0)
		return readBlock(currentPageNumber, fHandle, memPage);
	else
		return RC_ERROR_INVALID_PAGENUM;
}

//Written by Siddharth Sharma
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//set the pointer to the current page position and add the value by 1 in order to read the next block
	int nextPageNumber = (fHandle->curPagePos / PAGE_SIZE) + 1;
	if(nextPageNumber > 0)
		return readBlock(nextPageNumber, fHandle, memPage);
	else
		return RC_ERROR_INVALID_PAGENUM;
}

//Written by Siddharth Sharma
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	//setting the pointer to the last page number and subtract it by 1
	int lastPageNumber = fHandle->totalNumPages - 1;
	if(lastPageNumber > 0)
		return readBlock(lastPageNumber, fHandle, memPage);
	else
		return RC_ERROR_INVALID_PAGENUM;
}

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
		returnCode = RC_WRITE_FAILED;
	else{
		pageF = fopen(fHandle->fileName, FILE_READ_WRITE_PERMISSION);
		if(pageF == NULL)
			returnCode = RC_FILE_NOT_FOUND;
		else{
			int startPosition = pageNum * PAGE_SIZE;
			if(pageNum == 0) {
				fseek(pageF, startPosition, SEEK_SET);
				int i = 0;
				int isEOF;

				while(i < PAGE_SIZE){
					isEOF = feof(pageF);
					if(isEOF > 0)
						 appendEmptyBlock(fHandle);
					fputc(memPage[i], pageF);
					i+=1;
				}
				fHandle->curPagePos = ftell(pageF);
				int close = fclose(pageF);
				if(close != 0){
					returnCode = RC_ERROR_ON_CLOSE;
				}
				else{
					returnCode = RC_OK;
				}

			} else {
				fHandle->curPagePos = startPosition;
				int close = fclose(pageF);
				if(close != 0){
					returnCode = RC_ERROR_ON_CLOSE;
				}
				else{
					returnCode = RC_OK;
				}
				writeCurrentBlock(fHandle, memPage);
			}
			returnCode = RC_OK;
		}
	}
	return returnCode;
}

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	pageF = fopen(fHandle->fileName, FILE_READ_WRITE_PERMISSION);
	if(pageF == NULL)
		returnCode = RC_FILE_NOT_FOUND;
	else{
		appendEmptyBlock(fHandle);
		fseek(pageF, fHandle->curPagePos, SEEK_SET);
		fwrite(memPage, sizeof(char), strlen(memPage), pageF);
		fHandle->curPagePos = ftell(pageF);
		int close = fclose(pageF);
		if(close != 0){
			returnCode = RC_ERROR_ON_CLOSE;
		}
		else{
			returnCode = RC_OK;
		}
	}
	return returnCode;
}


//Written by Vaishnavi Mule
extern RC appendEmptyBlock (SM_FileHandle *fHandle) {
	SM_PageHandle emptyBlock = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
	if(fseek(pageF, 0, SEEK_END) != 0 ) {
		free(emptyBlock);
		return RC_WRITE_FAILED;

	} else {
		fwrite(emptyBlock, sizeof(char), PAGE_SIZE, pageF);
	}
	free(emptyBlock);
	//update the total number of pages
	fHandle->totalNumPages++;
	returnCode = RC_OK;
	return returnCode;
}

//Written by Vaishnavi Mule
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
	//// Open file stream in append mode to append the data at the end of file.
	pageF = fopen(fHandle->fileName, "a");
	if(pageF == NULL || pageF == 0)
	{
			printf("\nFile not found...");
			returnCode = RC_FILE_NOT_FOUND;
	}
	else{
		//check if number of pages exceeds the total number of pages
		while(numberOfPages > fHandle->totalNumPages)
			appendEmptyBlock(fHandle);
		// closing file stream
		fclose(pageF);
		returnCode = RC_OK;
	}
	return returnCode;
}

