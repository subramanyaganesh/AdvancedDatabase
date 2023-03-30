GROUP 24
========================
1)Vaishnavi Mule
2)Avadhoot Kodag
3)Siddharth Sharma


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
-> A new buffer pool will be created in the memory.
-> page file name is stored in pageFileName.
-> strategy denotes the page replacement technique (FIFO, LRU) that will be utilized by this buffer pool.
-> stratData will pass the parameters to FIFO and LRU. 

shutdownBufferPool():
-> This will destroy the buffer pool.
-> all dirty pages will be written back to the disk by calling forceFlushPool() function.
-> This function will return a returnCode as RC_OK if all the pages has been removed sucessfully.

forceFlushPool():
-> It will write all the modified pages having isDirty value 1 to the disk.
-> isDirty value of every page frame will be checked. If it's value is 1 then the page frame has been modified. if fixcount = 0 then no user is using that page. If both conditions are statisfied then it writes the page frame to the page file.


2. PAGE REPLACEMENT ALGORITHM FUNCTIONS
=========================================

FIFO():
-> First In First Out is a very simple page replacement strategy used.  
-> It is similar to a queue in that the page that appears first in the buffer pool is at the front and is replaced first if the buffer pool is full.
-> When we find the page, we save the content of the page frame to disk and then append the new page at that place.	

LRU():
-> Least Recently Used (LRU) removes the page frame that hasn't been used in a long time from the buffer pool.
-> hitNum will keep a count of the accessed pageframes. 
-> So, while utilizing LRU, we only need to locate the position of the page frame with the lowest hitNum number.
-> The content of the page frame is then written to the page file on disk, and the new page is added at that position.

3. PAGE MANAGEMENT FUNCTIONS
==========================

markDirty():
-> set the isDirty=1 0f specified page.         
-> It finds the page frame via pageNum by repeatedly checking each page in the buffer pool, and when the page is found, isDirty = 1 is set for that page.

unpinPage():
-> This function unpins the page selected. The page to be unpinned is determined by the pageNum.
-> After using a loop to locate the page, it decrements the fixCount of that page by one, indicating that the client is no longer using this page.

forcePage():

-> This page writes the data of the selected page frame to the disk's page file.
-> It finds the requested page using pageNum by looping through all the pages in the buffer loop.
-> When the page is located, the Storage Manager functions are used to write the page frame's content to the page file on disk. It sets dirtyBit = 0 for that page after writing.

pinPage():

-> This function unpins the page selected which is determined by the pageNum.
-> After using a loop to locate the page, it decrements the fixCount of that page by one, indicating that the client is no longer using this page.

4. STATISTICS FUNCTIONS
===========================
getFrameContents():

-> This function will return PageNumbers and that array will be the same size as buffer size
-> We iterate through all of the page frames in the buffer pool to acquire the pageNum value of the page frames in the buffer pool.
-> The "n"th element will be the noof the page stored in the "n"th page frame.

getDirtyFlags(): 

-> This function will return an array of bools which size will be equal to buffer size.
-> We iterate through all of the page frames in the buffer pool to acquire the isDirty value of the page frames in the buffer pool.
-> if the page stored in the "n"th page frame is dirty then the "n"th element will be TRUE .

getFixCounts():

-> This function will return ints and that array will be the same size as buffer size
-> We iterate through all of the page frames in the buffer pool to acquire the FixCounts value of the page frames in the buffer pool.
-> The "n"th element will be the fixCountof the page stored in the "n"th page frame.

getNumReadIO():

-> This function will return the count of no of pages read from the disk.
-> rearIndex variable is used to maintain this data.

getNumWriteIO():

-> This function will return the count of total no of pages written to the disk.
-> writeCount variable is used to maintain this data. when buffer pool is initialized, writeCount value will be set to 0 and it will increment whenever a page frame will be written to the disk.

								