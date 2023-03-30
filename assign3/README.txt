GROUP 24
========================
1)Vaishnavi Mule
2)Avadhoot Kodag
3)Siddharth Sharma


HOW TO RUN
=========================

1) Go to Project root (assign3) using Terminal.

2) Type ls to list the files and check that we are in the correct directory.

3) Type "make clean" in order to delete old compiled .o files.

4) Type "make" to compile all project files.

5) Type "make run_test" to run "test_assign3_1.c" file.


FUNCTIONS USED
========================
1. TABLE AND RECORD MANAGER FUNCTIONS
=======================================

initRecordManager():
-> This function will initializes the record manager.
-> Storage manager will be initialized by calling initStorageManager().

shutdownRecordManager():
-> all the allocated resources to the record manager will be de-allocated and record manager will be shutdown.
-> every resources/memory will be freed up which are being used by the Record Manager.
-> free() function is used to de-allocate memory space.

createTable():
-> Buffer Pool will be initialized by initBufferPool(). 
-> All the attributes like name, datatype and size will be set and values of the table will initialized.
-> page file will be created and opened.
-> It will write the block which contains the table in the page file and will close the file.

openTable():
-> It will opens the table with the provided name by the parameter.

closeTable():
-> As given by the parameter 'rel' it will close the table.
-> It does it by calling shutdownBufferPool() function of BUffer Manager's function.
-> Buffer manager will write all the previous changes madeBefore closing the buffer pool.

deleteTable()
-> This function will delete the specified table. 
-> destroyPageFile() will be called.
-> page deletion and de-allocation of any memory space will be completed by this function.

getNumTuples()
-> no of tuples present in the table will be returned by getNumTuples function.
-> value of tuplesCount will be return, which is used for storing table's meta-data.


2. RECORD FUNCTIONS
=======================================

insertRecord():
-> This function updates the'record' parameter with the Record ID passed in the insertRecord().
For the record being inserted, we set the Record ID.
-> We bookmark the page with the open slot. Once we have an empty slot, we find the data pointer and add a "+" to indicate that a new record has been added.-> Also we mark the page dirty so that the Buffer Manager writes the content the page back to the disk.
-> In order for the Buffer Manager to write the page's content back to the disk, we also mark the page as dirty.
-> Using the memcpy() C function, we copy the record's data (passed through parameter "record") into the new record before unpinning the page.

deleteRecord():
-> This function deletes a record from the table referred to by the parameter "rel" with the Record ID "id" passed through the parameter.
-> In order to free up space for a new record to be added later, we set our table's freePage meta-data to the Page ID of the page whose record is to be deleted.
-> We pin the page, navigate to the record's data pointer, and change the record's first character to a '-' to indicate that it has been deleted and is no longer required.
-> Finally, we unpin the page after marking it dirty so that the BUffer Manager can save its contents back to disk.

updateRecord():
-> A record in the table referred to by the parameter "rel" that is referenced by the function "record" is updated.
-> Using the meta-data of the table, it locates the page where the record is located and pins that page in the buffer pool.
-> It sets the Record ID and moves on to the record's data storage location.
->  Using the memcpy() C function, we copy the record's data (passed through parameter "record") into the new record, dirty-mark the page, and then unpin the page.

getRecord():
-> This function retrieves a record from the table that is referenced by the parameter "rel" and has the Record ID "id" passed in the parameter. The location referred to by the parameter "record" is where the result record is kept.
-> Using the table's meta-data, it locates the page where the record is stored, and using the record's "id," it pins that page in the buffer pool.
-> It copies the data and sets the Record ID of the "record" parameter to the ID of the record that is present on the page.
-> The page is then unpinned.


3. SCAN FUNCTIONS
=======================================

startScan():
-> The RM ScanHandle data structure, which is supplied as an argument to the startScan() function, is accessed by this function to begin a scan. 
-> We set the scan-related variables in our unique data structure. 
-> If the condition is NULL, an error code is returned.
RC_SCAN_CONDITION_NOT_FOUND

next():
-> This function returns the following tuple that meets the requirement (test expression).
-> If the condition is NULL, an error code is returned RC_SCAN_CONDITION_NOT_FOUND.
-> We return an error code if the table contains no tuples RC_RM_NO_MORE_TUPLES.
-> The table's tuples are iterated through. Then, mark the page containing that tuple, go to the site where the data is stored, transfer the data into a temporary buffer, and then run eval (....)  to evaluate the test expression 
-> The tuple satisfies the criteria if the result (v.boolV) of the test expression is TRUE. The page is subsequently unpinned, and we return RC_OK. 
-> If none of the tuples satisfy the requirement, an error code is returned RC_RM_NO_MORE_TUPLES.

closeScan(): 
-> The scan operation is terminated by this function. 
-> By examining the scanCount value in the table's metadata, we determine whether the scan was complete or not. If it is higher than 0, the scan was not fully completed. 
-> If the scan was insufficient, we unpin the page and reset all variables linked to the scan method in the meta-data of our table (custom data structure). 
-> The space that the metadata had taken up is then freed (de-allocated).


4. SCHEMA FUNCTIONS
=========================================


getRecordSize():
-> The size of a record in the given schema is returned by this function.
-> We loop through the schema's attributes. Each attribute's size (required amount of space in bytes) is iteratively added to the variable "size."
-> The record's size is the value of the variable "size."

freeSchema():
-> This function deletes the memory representation of the schema supplied by the parameter "schema."
-> This is accomplished using the variable (field) refNum in each page frame. refNum keeps track of how many page frames the client is accessing.
-> To remove the schema from memory, we de-allocate the memory space it was using with the C function free().

createSchema():
-> With the given arguments, this function creates a brand-new schema in memory.
-> The number of parameters is indicated by numAttr. The name of the attributes is specified by attrNames. The datatype of the attributes is specified by datatypes. The length of the attribute is specified by typeLength.
-> We establish a schema object and provide it memory space. Finally, we set the parameters of the schema to those specified in the createSchema command.


5. ATTRIBUTE FUNCTIONS
=========================================

createRecord():
-> This function adds a new record to the schema specified by the parameter "schema" and delivers the new record to the createRecord() function's "record" parameter.
-> We give the new record the appropriate amount of memory. Additionally, we allocate RAM for the record's data, which takes up the entire record size.
-> In addition, we append '0', which is NULL in C, to the first slot and add a '-'. This is a fresh blank record, as indicated by the symbol "-." As a last step, we assign this new record to the parameter's "record."

freeRecord():
-> The memory space allotted to the "record" given by the parameter is released by this function. To release the memory space used by the record, we utilize the C function free()

getAttr():
-> The given record in the given schema is used to obtain an attribute using this function.
-> The argument is passed the record, schema, and attribute number whose data is to be obtained. The location indicated by the 'value' parameter provided through is where the attribute details are stored back.
-> Using the attrOffset() function, we navigate to the attribute's location. The attribute's datatype and value are then copied to the '*value' parameter based on the datatype of the attribute.

setAttr():
-> With the help of the supplied schema, this function sets the attribute value in the record. The parameter passes through the record, schema, and attribute number whose data is to be fetched.The "value" option is used to pass the data that will be placed in the attribute.
-> Using the attrOffset() function, we navigate to the attribute's location. The data in the '*value' parameter is then copied to the attribute's datatype and value, depending on the datatype of the attribute.
