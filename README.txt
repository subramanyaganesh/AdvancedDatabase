GROUP 10
========================
1)Subramanya Ganesh
2) Kachikwu Nwike



HOW TO RUN
=========================

1) Go to Project root (assign1) using Terminal.

2) Type ls to list the files and check that we are in the correct directory.

3) Type "make clean" in order to delete old compiled .o files.

4) Type "make" to compile all project files.

5) Type "make run" to run "test_assign1_1.c" file.

6) Type "make run" to run "test_assign1_2.c" file.



FUNCTIONS USED
========================

initStorageManager()
--> This function will set the file stream object to NULL to initialize.

createPageFile():
              
-> This function has a paramter as filename and will create a file with that name.
-> To create a new file we use the fopen() in the 'w+' mode, which opens a new file for reading and writing.
-> this function will return a returnCode as RC_OK if file created successfully otherwise it will return RC_FILE_NOT_FOUND if file could not be created.

openPageFile():

-> To open the file created in the preceding function and update all meta data parameters of the stuct fHandle such as Total number of pages, Current page position, File name.
-> To open file we use the fopen() in the 'r' mode, which opens a new file for reading.
-> If the file could not be opened, we return RC_FILE_NOT_FOUND, otherwise we return RC_OK .

closePageFile():
                
-> we use the fopen() in the 'r' mode, which opens a new file for reading and check if page file is not NULL.
-> To close file we use the fclose() , which closes the file and will return 0 if successful.
		
destroyPageFile():

-> we use the fopen() in the 'r' mode, which opens a new file for reading and check if page file is not NULL.
-> To close file we use the fclose() , which closes the file and will return 0 if successful.
-> To delete the existing file, we use the remove() C function to remove the file presented in memory.

readBlock():
           
-> We verify the validity of the page number. The page number should not be less than 0 and more than total number of pages.
-> Using the valid file pointer, we use fseek() to navigate to the specified location.
-> If fseek() succeeds, we read the data from the specified page number and store it in the memPage.

getBlockPos():
                
-> This function returns the current page position as determined by FileHandle's curPagePos.

readFirstBlock():

-> To read the first block, we call the readBlock with the pageNum parameter as 0

readCurrentBlock():

-> To read the current block, we call the readBlock with the pageNum parameter as currentPageNumber.

readNextBlock():

-> To read the next block, we call the readBlock with the pageNum parameter as currentPageNumber + 1.   

readPreviousBlock():

-> To read the previous block, we call the readBlock with the pageNum parameter as currentPageNumber - 1.           

readLastBlock():
                
->To read the last block, we call the readBlock with the pageNum parameter as totalNumPages - 1.

writeBlock():
           
-> We verify the validity of the page number. The page number should not be less than 0 and more than number of pages overall.
-> Using the valid file pointer, we use fseek() to navigate to the specified location.
-> If fseek() succeeds, we write the data from the specified page number and store it in the memPage.

writeCurrentBlock():

-> To write the current block, we call the writeBlock with the pageNum parameter as currentPageNumber.

appendEmptyBlock():

-> To append existing file with new page We create an empty block of same size as PAGE SIZE and move the file cursor to the last page.
-> Write the newly created page and update the total number of pages.                  

ensureCapacity():

-> check if number of pages exceeds the total number of pages and then add that much number of empty blocks using appendEmptyBlock function.

