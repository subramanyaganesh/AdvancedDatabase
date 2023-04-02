GROUP 10
========================
1)Subramanya Ganesh
2)Kachikwu Nwike
3)Deshon Langdon 


Steps to Execute the test cases
=========================

1) Clone the project template from git.
2) Navigate to the project assign3
3) Validate that all the files are present using the ls command
4) Run "make clean" command to clear all the old .o or object files
5) Run "make" command to compile the code and create the corresponding objet file
6) Run "make run" to run "test_assign3_1.c" file.
5) Run "make test_expr" command to compile the code and create the corresponding objet file
6) Run "make run_expr" to run "test_expr.c" file.


FUNCTIONS USED
========================
1. TABLE AND RECORD MANAGER FUNCTIONS
=======================================

initRecordManager():
-> This function initializes the record manager and calls initStorageManager() to initialize the record manager.

shutdownRecordManager():
-> This function shuts down the record manager
-> frees up the memory allocated by the record manager and shuts down the record manager.

createTable():
-> initBufferPool() initializes the buffer pool. 
-> The table's values will be initialized after all of its attributes have been defined.
-> createPageFile() and openPageFile() creates and opens page file.
-> The block containing the table is written to the page file, and closes the file.

openTable():
-> This function creates the table with a name specified by the parameter.

closeTable():
-> This function closes the table specified by the parameter.
-> It stores the table's metadata before clsoing it.

deleteTable()
-> This function deletes the table specified by the parameter by calling destroyPageFile().

getNumTuples()
-> This function returns the number of tuples in the table specified by the parameter.


2. RECORD FUNCTIONS
=======================================

insertRecord():
-> This function inserts the record specified in the parameter with the Record ID passed in the insertRecord().
-> The page is marked as dirty so that the Buffer Manager can write the information of the page back to the disk.
-> Before unpinning the page, memcpy() is called to copy the data from the record specified in the parameter into the new record. 
-> The record is added and its if is set.

deleteRecord():
-> This function deletes the record specified in the parameter with the Record ID passed in the deleteRecord().
-> The page is marked as dirty and unpinned after so that the Buffer Manager can save its content back to disk.

updateRecord():
-> This function updates the record specified in the parameter with the Record ID passed in the updateRecord().
-> It finds the page where the record is kept using the table's meta-data and pins that page in the buffer pool.
-> It assigns the Record ID and navigates to the directory holding the data for the record.

getRecord():
-> This function gets the record specified in the parameter with the Record ID passed in the getRecord().
-> It replicates the data and changes the ID of the "record" argument to the ID of the record on the page.


3. SCAN FUNCTIONS
=======================================

startScan():
-> This function uses the RM ScanHandle data structure, which is passed as an input to startScan(), to start a scan.

next():
-> This function returns the tuple that satisfies the condition (test expression).
-> If none of the tuples meet the requirement, we return the error code RC RM NO MORE TUPLES.

closeScan(): 
-> This function terminates the scan operation. 
-> The scanCount value is used to determine if the scan was completed. 
-> If the scan is insufficient, we unpin the page and reset all variables in our table's meta-data that were linked to the scan method.
-> The metadata-occupied space is then de-allocated.


4. SCHEMA FUNCTIONS
=========================================


getRecordSize():
-> This function returns the size of a record in the given schema.

freeSchema():
-> This function deletes the memory representation of the schema specified by the "schema" parameter.

createSchema():
-> This function creates a new schema in memory using the arguments provided.


5. ATTRIBUTE FUNCTIONS
=========================================

createRecord():
-> This function adds a new record to the schema specified by the "schema" parameter and returns the new record to the "record" parameter of the createRecord() function.
-> This function initializes the page and slot, sets the pointer and allocates memory for the schema/record, returns the size of the current record, allocates space for the record, and sets the pointer and allocates memory for the schema/record.

freeRecord():
-> This function releases the memory allotted to the "record" specified by the parameter. We use the C function free to free up the memory required by the record ()

getAttr():
-> This function retrieves an attribute from the provided record in the supplied schema.
-> The record, schema, and attribute number whose data needs to be fetched are supplied as arguments. The attribute information is stored back at the place denoted by the 'value' parameter that was passed over.
-> We navigate to the attribute's location using the attrOffset() function. Based on the datatype of the attribute, the datatype and value are copied to the '*value' parameter.

setAttr():
-> This function sets the attribute value in the record using the schema provided.
-> With the help of the supplied schema, this function sets the attribute value in the record. The parameter passes through the record, schema, and attribute number whose data is to be fetched.The "value" option is used to pass the data that will be placed in the attribute.
-> We navigate to the attribute's location using the attrOffset() function. Depending on the datatype of the attribute, the data in the '*value' parameter is then copied to the datatype and value of the attribute.
