GROUP 10
========================
1)Subramanya Ganesh
2)Kachikwu Nwike
3)Deshon Langdon 


HOW TO RUN
=========================

1) Go to Project root (assign2) using Terminal.

2) Type ls to list the files and check that we are in the correct directory.

3) Type "make clean" in order to delete old compiled .o files.

4) Type "make" to compile all project files.

5) Type "make run_test" to run buff_mgr.exe file.

We have implemented FIFO (First In First Out), LRU (Least Recently Used) page replacement algorithms.

FUNCTIONS USED
========================
1. BUFFER POOL FUNCTIONS
===========================

initBufferPool():
-> A new buffer pool is created with predifined number of pages.
-> Memory is dynamically allocated and stores the pframe value in the buffer pool variable.

shutdownBufferPool():
-> The buffer pool is destroyed and the dirty page is written to disk.
-> Memory is deallocated and mgmtData is reinitialized.

forceFlushPool():
-> The forceFlushPool writes dirty pages to disk.
-> Checks the isData value and fixed count value, then opens available file from disk and writes the contents of the page to the disk.


2. PAGE REPLACEMENT ALGORITHM FUNCTIONS
=========================================

FIFO():
-> Checks if the page is present in the buffer pool and returns the frame is present
-> Otherwise, it checks for a vacant page and saves the page content to disk.
-> It the sets the page frame's content to new page's content

LRU():
-> LRU replaces the page that has not been recently used in the buffer pool
-> The LRU writes old pages to disk before replacing the pages.


3. PAGE MANAGEMENT FUNCTIONS
==========================

markDirty():
-> markDirty marks a page as dirty.
-> It iteratively searches for the frame containing the page and marks it dirty by setting isDirty to 1. 

unpinPage():
-> This method deletes the pin of the provided page.
-> It find the frame corresponding to the page and decrease the fixCount by 1.

forcePage():
-> This method writes the selected page frame data to disk.
-> It iteratively searches for the page using pageNum and writes the page back to file.
-> Finally, it sets the page's dirty state to false.

pinPage():
-> This method pins the specified page using both the FIFO and LRU methods

4. STATISTICS FUNCTIONS
===========================
getFrameContents():
-> This method returns an array of PageNumbers where the ith element is the number of the page stored in the ith page frame.
-> It iteratively goes through the frames in the buffer pool storing the pageNumbers to the appropriate psoition in the array and returning the array.

getDirtyFlags(): 
-> This method returns an array of bools where the ith element is TRUE if the page stored in the ith page frame is dirty.
-> It iteratively goes through the frames in the buffer pool storing the state value to the appropriate position in the array and returning the array.

getFixCounts():
-> This method returns an array of ints where the ith element is the fix count of the page stored in the ith page frame.
-> It iteratively goes through the frames in the buffer pool storing FixCounts value to the appropriate position in the array and returning the array.

getNumReadIO():
-> This method returns the number of pages that have been read from disk since a buffer pool has been initialized by utilizing the rearIndex variable

getNumWriteIO():
-> This method returns the number of pages written to the page file since the buffer pool has been initialized by utilizing the writeCount variable.
