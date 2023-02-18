GROUP 10
========================
1)Subramanya Ganesh
2)Kachikwu Nwike
3)Deshon Langdon 


Steps to Execute the test cases
=========================

1) Clone the project template from git.
2) Navigate to the project assign1
3) Validate that all the files are present using the ls command
4) Run "make clean" command to clear all the old .o or object files
5) Run "make" command to compile the code and create the corresponding objet file
6) Run "make run" to run "test_assign1_1.c" file.
7) Run "make run" to run "test_assign1_2.c" file.



Function Description
========================

initStorageManager()
This methord is used to instantiate the file stream

createPageFile():
              
1. The primary objective of this method is to create a file as specified by the parameter
2. In order to perform this we have to use the fopen() function in w+ mode which is read and write
3. The output of this function will be a return code RC_OK if the file is created else will return a RC_FILE_NOT_FOUND return code

openPageFile():

1. This method is used to open the file using the os supported method open in readonly mode and then store the files metadat into a structure called fhandle

2. If the file is opened successfully then the fuction returns RS_OK else it returns RC_FILE_NOT_FOUND returncode

closePageFile():
                
1. Here as well we use the fopen() method which opens the file in r mode and then we are checking if the page is null or not
2. In order to close the file we use the fclose function and this method returns 0 if it is successful
		
destroyPageFile():

1. Similar to the close function, we first open the file in read mode using the fopen function and then check if the page is not NULL
2. We perform the same function as that of the closePageFile and call the fclose and check if the output is 0 which means successful or else unsuccessful
3. Finally we use the remove function to remove the file and deallocate all its memory

readBlock():
           
1. We verify the validity of the page number. The page number should not be less than 0 and more than total number of pages.
2. Using the valid file pointer, we use fseek() to navigate to the specified location.
3. If fseek() succeeds, we read the data from the specified page number and store it in the memPage.

getBlockPos():
                
This function is used to return the page pointed by the FileHandele's curPagePos

readFirstBlock():

1. To read the first block, we call the readBlock with the pageNum parameter as 0
2. We verify the validity of the page number. The page number should not be less than 0.
3. Using the valid file pointer, we use fseek() to navigate to the first Block.
4. Check if the first Block is within the page limits. If it is, we read the data from the first page number and set the currentPageNumber to the first block.

readCurrentBlock():

1. To read the current block, we call the readBlock with the pageNum parameter as currentPageNumber.
2. Using the valid file pointer, we use fseek() and fread() to navigate and read the current Block.
3. Set the currentPageNumber to the current block.
4. Check if the current Block is within the page limits. 

readNextBlock():

1. To read the next block, we call the readBlock with the pageNum parameter as currentPageNumber + 1.   
2. Check validity of next block (Not greater that the total number of blocks)
3. Using the valid file pointer, we use fseek() and fread() to navigate and read the next Block.
4. Set the currentPageNumber to the next block.
5. Check if the next Block is within the page limits. 

readPreviousBlock():

1. To read the previous block, we call the readBlock with the pageNum parameter as currentPageNumber - 1.      
2. Check validity of previous block 
3. Using the valid file pointer, we use fseek() and fread() to navigate and read the previous Block.
4. Set the currentPageNumber to the previous block.
5. Check if the previous Block is within the page limits.      

readLastBlock():
                
1. To read the last block, we call the readBlock with the pageNum parameter as totalNumPages - 1.
2. Using the valid file pointer, we use fseek() and fread() to navigate and read the last Block.
3. Set the currentPageNumber to the last block.
4. Check if the last Block is within the page limits.

writeBlock():
           
1. First check if the page passed as the parameter is valid this means that the page number should not be less than 0 and more than the total pages
2. Use the fseek method to get to the specified location
3. if the fseek operation is successful then we transfer data from the disk to the memory page

writeCurrentBlock():

1. We use the writeBlock method to write the data from disk to the mempage but in this case we compute the currentpage and write data in this page

appendEmptyBlock():

1. We create an empty block of same size as PAGE SIZE and move the file cursor to the last page and now append existing file
2. After writing update the page count

ensureCapacity():

1. Check if the page limit is not exceeded and then add that many number of empty blocks using the appendEmptyBlock method
