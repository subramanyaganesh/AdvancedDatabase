#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"


const int recSizeAttr = 15;
const int MaxPagesNum = 100;
typedef struct RecMgr
{
	BM_PageHandle pHandle;
    int recMgrFreePage;
    RID recID;
	Expr *recCond;
	int scanRecordCnt;
    int tuplesRecordCnt;
    BM_BufferPool bufferPool;
	
} RecMgr;

RecMgr *recMgr;


//Written by Deshon Langdon
int findFreeRecSlot(char *data, int recSize)
{   // This function finds the first available slot in a page of records
    int index = 0;
int maxSlots = PAGE_SIZE / recSize;

for (; index < maxSlots; ++index) {
    char* slot = data + index * recSize;
    if (*slot != '+') {
        return index;
    }
}

return -1;
}

//Written by Deshon Langdon
extern RC initRecordManager(void* mgmtData) {
    // This function initializes the record manager

    initStorageManager();
	return RC_OK;
}

extern RC shutdownRecordManager() {

// This function shuts down the record manager
    if (recMgr == NULL) {
        return RC_OK;
    }

    free(recMgr);
    recMgr = NULL;

    return RC_OK;
}

//Written by Deshon Langdon
extern RC createTable(char *name, Schema *schema)
{
    char tableData[PAGE_SIZE]; // Define a buffer to hold page data for the table
	 int maxNumAttr= (PAGE_SIZE - 16)/(64+4+4+4);
  	 int overMax = maxNumAttr + 1;
	 int overMaxForRecord = PAGE_SIZE - 3 * 4 - 1 + 1;
	if(schema->numAttr==overMax){ // Check if the schema exceeds the maximum number of attributes or the maximum size of a record
		return RC_TABLE_TOO_LARGE;
	}
	if(*(schema->typeLength)==overMaxForRecord){
		return RC_TABLE_TOO_LARGE;
	}
    recMgr = (RecMgr*) malloc(sizeof(RecMgr));
	
    char *pHandle  = tableData;
	SM_FileHandle fileHandle;
    initBufferPool(&recMgr->bufferPool, name, MaxPagesNum, RS_LRU, NULL); // Initialize the buffer pool for the record manager
    int result = createPageFile(name);
    result |= openPageFile(name, &fileHandle);

    int intSize = sizeof(int);
int header[] = {0, 1, schema->numAttr, schema->keySize};
int numHeaderValues = sizeof(header) / intSize;

for (int i = 0; i < numHeaderValues; i++) {
    *(int*)pHandle = header[i];
    pHandle += intSize;
}

int attrIdx = 0; // Iterate over each attribute in the schema and write the attribute data to the buffer
    while (attrIdx < schema->numAttr) {
        strncpy(pHandle , schema->attrNames[attrIdx], recSizeAttr);
    pHandle  += recSizeAttr;
    
    
    switch(schema->dataTypes[attrIdx]) { // Write the attribute data type to the buffer
        case DT_INT:
            *(int*)pHandle = 1;
            break;
        case DT_FLOAT:
            *(int*)pHandle = 2;
            break;
        case DT_BOOL:
            *(int*)pHandle = 3;
            break;
        case DT_STRING:
            *(int*)pHandle = 4;
            break;
        default:
            *(int*)pHandle = 0;
            break;
    }
    pHandle  += intSize;
    
   
    if (schema->dataTypes[attrIdx] == DT_STRING) { // Write the attribute length to the buffer
        *(int*)pHandle = schema->typeLength[attrIdx];
    } else {
        *(int*)pHandle = 0;
    }
    pHandle += intSize;
    
    attrIdx++;
    }
    // Write the buffer to the first page of the page file and close the file
    result |= writeBlock(0, &fileHandle, tableData); 
    result |= closePageFile(&fileHandle);
    return result == RC_OK ? RC_OK : RC_ERROR;
}


//Written by Deshon Langdon
Schema* setSchema(Schema* schema, int attrCnt) {
    schema->numAttr = (attrCnt >= 0) ? attrCnt : schema->numAttr;
    schema->attrNames = (attrCnt >= 0) ? (char**) malloc(sizeof(char*) * attrCnt) : schema->attrNames;
    schema->dataTypes = (attrCnt >= 0) ? (DataType*) malloc(sizeof(DataType) * attrCnt) : schema->dataTypes;
    schema->typeLength = (attrCnt >= 0) ? (int*) malloc(sizeof(int) * attrCnt) : schema->typeLength;
    return schema;
}

//Written by Deshon Langdon
extern RC openTable(RM_TableData *rel, char *name) {
    SM_PageHandle pageHandle;    
    int attributeCount;
    int intSize = sizeof(int);
    RC result;

    rel->mgmtData = recMgr;
    rel->name = name;

// Pin the first page of the buffer pool and check for errors
    result = pinPage(&recMgr->bufferPool, &recMgr->pHandle, 0);
    if (result != RC_OK) return result;

    pageHandle = (char*) recMgr->pHandle.data;
    recMgr->tuplesRecordCnt = *(int*)pageHandle;
    pageHandle += intSize;
    recMgr->recMgrFreePage = *(int*) pageHandle;
    pageHandle += intSize;
    attributeCount = *(int*)pageHandle;
    pageHandle += intSize;
 
 
// Create a new schema and set its attributes
    Schema *schema = (Schema*) malloc(sizeof(Schema));
    setSchema(schema, attributeCount);
 
 // Iterate over the attributes in the schema
    for (int i = 0; i < attributeCount; i++) {
       char* attributeName = (char*) malloc(recSizeAttr);
    if(attributeName == NULL) {
     
    }
    strncpy(attributeName, pageHandle, recSizeAttr);
    schema->attrNames[i] = attributeName;
    pageHandle += recSizeAttr;
    
    int* dataType = (int*) malloc(intSize);
    if(dataType == NULL) {
       
    }
    *dataType = *(int*) pageHandle;
    schema->dataTypes[i] = *dataType;
    pageHandle += intSize;
    free(dataType);
    
    int* typeLength = (int*) malloc(intSize);
    if(typeLength == NULL) {
       
    }
    *typeLength = *(int*) pageHandle;
    schema->typeLength[i] = *typeLength;
    pageHandle += intSize;
    free(typeLength);
    }

// Assign the schema to the RM_TableData struct and unpin the first page
    rel->schema = schema;    
    unpinPage(&recMgr->bufferPool, &recMgr->pHandle);
    forcePage(&recMgr->bufferPool, &recMgr->pHandle);
    return RC_OK;
}


//Written by Deshon Langdon
extern RC deleteTable (char *name)
{
	return destroyPageFile(name) == RC_OK ? RC_OK : RC_ERROR;
}


extern RC closeTable (RM_TableData *rel)
{
	RecMgr *recordManager = rel->mgmtData;
return shutdownBufferPool(&recordManager->bufferPool) == RC_OK || 
RC_PINNED_PAGES_IN_BUFFER ? RC_OK : RC_ERROR;
}

//Written by Deshon Langdon
extern int getNumTuples (RM_TableData *rel)
{
	RecMgr *recordManager = rel->mgmtData;
return recordManager->tuplesRecordCnt > 0 ? recordManager->tuplesRecordCnt : 0;
}

//Written by Kachikwu Nwike
extern RC insertRecord (RM_TableData *rel, Record *record)
{
	RecMgr *recordManager = rel->mgmtData;
	RID *recordId = &record->id; 
	char *data, *slot;
	recordId->page = recordManager->recMgrFreePage;
	pinPage(&recMgr->bufferPool, &recMgr->pHandle, recMgr->recMgrFreePage);
    data = recordManager->pHandle.data;
    recordId->slot = findFreeRecSlot(recordManager->pHandle.data, getRecordSize(rel->schema));

    while (recordId->slot < 0){// While there are no free slots in the page
        unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
        recordId->page++;
        pinPage(&recordManager->bufferPool, &recordManager->pHandle, recordId->page);
        data = recordManager->pHandle.data;
        recordId->slot = findFreeRecSlot(recordManager->pHandle.data, getRecordSize(rel->schema));
    }
    
    slot = data;
    markDirty(&recordManager->bufferPool, &recordManager->pHandle);

    slot += (recordId->slot * getRecordSize(rel->schema)); // Move the slot pointer to the slot where the record will be inserted
    *slot = '+';// Set the marker for the slot to indicate that it is occupied
    memcpy(++slot, record->data + 1, getRecordSize(rel->schema) - 1);
    unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
    
    recordManager->tuplesRecordCnt++;
    pinPage(&recordManager->bufferPool, &recordManager->pHandle, 0);
       
    return RC_OK;

}
//Written by Kachikwu Nwike
extern RC deleteRecord (RM_TableData *rel, RID id)
{
    //this function deletes rel
	RecMgr *recordManager = rel->mgmtData;
	pinPage(&recordManager->bufferPool, &recordManager->pHandle, id.page);
	recordManager->recMgrFreePage = id.page;
	char *data = recordManager->pHandle.data;
	data += (id.slot * getRecordSize(rel->schema));
	*data = '-';
	markDirty(&recordManager->bufferPool, &recordManager->pHandle);
	unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
	return RC_OK;
}

//Written by Kachikwu Nwike
extern RC updateRecord (RM_TableData *rel, Record *record)
{   // this function updates rel with record	
	RecMgr *recordManager = rel->mgmtData;
	
	pinPage(&recordManager->bufferPool, &recordManager->pHandle, record->id.page);
	char *data = recordManager->pHandle.data;
	data += (record->id.slot * getRecordSize(rel->schema));
	*data = '+';
	memcpy(++data, record->data + 1, getRecordSize(rel->schema) - 1 );
	markDirty(&recordManager->bufferPool, &recordManager->pHandle);
	unpinPage(&recordManager->bufferPool, &recordManager->pHandle);	
	return RC_OK;
}

//Written by Subramanya Ganesh
extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{   // getRecord() gets record from rel
    char *data, *dataPointer;
	RecMgr *mgr = rel->mgmtData;
	pinPage(&mgr->bufferPool, &mgr->pHandle, id.page);
	getRecordSize(rel->schema);
	dataPointer = mgr->pHandle.data;
    int flag = (id.slot * getRecordSize(rel->schema));
	dataPointer += flag;
	if(*dataPointer == '+')
	{
		record->id = id;
		data = record->data;
		memcpy(++data, ++dataPointer, getRecordSize(rel->schema) - 1);
	}
	else
	{
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	unpinPage(&mgr->bufferPool, &mgr->pHandle);
	return RC_OK;
}

//Written by Subramanya Ganesh
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *recCond)
{
    RecMgr *reordMgrScan;
    RecMgr *tableMgr;
    RC status;

// Check if a record condition is provided and open a scan table 
    status = (recCond != NULL) ? openTable(rel, "ScanTable") : RC_SCAN_CONDITION_NOT_FOUND;

    if (status == RC_OK) {
reordMgrScan = (RecMgr*) malloc(sizeof(RecMgr));
if (reordMgrScan == NULL) {
return RC_SCAN_MGR_CREATE_FAILED;
}
scan->mgmtData = reordMgrScan;
reordMgrScan->recID.slot = reordMgrScan->scanRecordCnt = 0;
scan->mgmtData = reordMgrScan;
reordMgrScan->recCond = recCond;
reordMgrScan->recID.page = 1;
scan->rel = rel;

// Get the table manager and initialize the tuples record count
tableMgr = rel->mgmtData;
if (tableMgr == NULL) {
free(reordMgrScan);
return RC_RM_UNINITIALIZED;
}
tableMgr->tuplesRecordCnt = recSizeAttr;
}
    return status;
}



//Written by Subramanya Ganesh
extern RC next(RM_ScanHandle *scan, Record *record) {
    RecMgr *reordMgrScan = scan->mgmtData;
    switch(reordMgrScan->recCond != NULL) {
        case true: {
            RecMgr *tableMgr = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;
    Value *res = malloc(sizeof(Value));
    char *data;

    if (getRecordSize(schema) <= 0) {
        free(res);
        return RC_ERROR;
    }

            int maxSlots = PAGE_SIZE / getRecordSize(schema);
            int scanCount = reordMgrScan->scanRecordCnt;

            switch (tableMgr->tuplesRecordCnt != 0) {
                case true: {
                    for (; scanCount <= tableMgr->tuplesRecordCnt;) {
                        reordMgrScan->recID.slot += (scanCount > 0) ? 1 : 0;
                         reordMgrScan->recID.page += (scanCount > 0 && reordMgrScan->recID.slot >= maxSlots) ? 1 : 0;
                          reordMgrScan->recID.slot = (scanCount > 0 && reordMgrScan->recID.slot >= maxSlots) ? 0 : reordMgrScan->recID.slot;
                           reordMgrScan->recID.page = (scanCount == 0) ? 1 : reordMgrScan->recID.page;

                        RC pinPageStatus = pinPage(&tableMgr->bufferPool, &reordMgrScan->pHandle, reordMgrScan->recID.page);
                        if (pinPageStatus == RC_OK) {
                            char *dataPointer = record->data;
            *dataPointer = '-';

                        // Set record ID
                         record->id = reordMgrScan->recID;

                  // Compute offset into page data
                 int recSize = getRecordSize(schema);
                  char *pageData = reordMgrScan->pHandle.data;
              int offset = record->id.slot * recSize;

              // Advance to start of record data
                   pageData += offset;
                    data = pageData;

            for (int i = 1; i < getRecordSize(schema); i++) {
    dataPointer[i] = data[i];
}

reordMgrScan->scanRecordCnt++;
scanCount++;

            evalExpr(record, schema, reordMgrScan->recCond, &res);
            bool eval = (bool) res->v.boolV;

            if (eval) {
                RC unpinPageStatus = unpinPage(&tableMgr->bufferPool, &reordMgrScan->pHandle);
                if (unpinPageStatus == RC_OK) {
                    return RC_OK;
                } else {
                    return RC_RM_NO_MORE_TUPLES;
                }
            }
        } else if (pinPageStatus == RC_ERROR) {
            return RC_ERROR;
        }

        // Default case for switch statement is handled by else statement
        else {
            // Do nothing
        }
    
                    }

                    RC unpinPageStatus = unpinPage(&tableMgr->bufferPool, &reordMgrScan->pHandle);
                    reordMgrScan->recID.page = 1;
                    reordMgrScan->recID.slot =reordMgrScan->scanRecordCnt = 0;
                    return unpinPageStatus == RC_OK ? RC_RM_NO_MORE_TUPLES : RC_RM_NO_MORE_TUPLES;
                }
                default: {
                    return RC_RM_NO_MORE_TUPLES;
                }
            }
        }
        default: {
            return RC_SCAN_CONDITION_NOT_FOUND;
        }
    }
}

//Written by Subramanya Ganesh
extern RC closeScan(RM_ScanHandle *scan) 
{   //terminates the scan operation
    RecMgr *scanManager = (scan != NULL) ? scan->mgmtData : NULL;
    if (scanManager != NULL && scanManager->scanRecordCnt > 0) {
        RecMgr *recordManager = scan->rel->mgmtData;
        RC rc = unpinPage(&recordManager->bufferPool, &scanManager->pHandle);
        if (rc != RC_OK) {
            return RC_ERROR;
        }
        scanManager->scanRecordCnt = 0;
        scanManager->recID.page = 1;
        scanManager->recID.slot = 0;
    }
    if (scan != NULL) {
        scan->mgmtData = NULL;
        free(scanManager);
        return RC_OK;
    }
    return RC_ERROR;
}

//Written by Subramanya Ganesh
extern int getRecordSize (Schema *schema)
{
	int val = 0;
    int i = 0;
    // Loop through all of the attributes in the schema.
    for(i=0; i<schema->numAttr; i++)
    {
       switch (schema->dataTypes[i]) {
            case DT_FLOAT:
                val += sizeof(float);
                break;
            case DT_BOOL:
                val += sizeof(bool);
                break;
            case DT_STRING:
                val += schema->typeLength[i];
                break;
            case DT_INT:
                val += sizeof(int);
                break;
        }
    }
    return ++val;
}
//Written by Subramanya Ganesh
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *schema = (Schema *) malloc(sizeof(Schema)); // allocate memory for schema
	// get schema values
	schema->numAttr = numAttr;
	schema->attrNames = attrNames;
	schema->dataTypes = dataTypes;
	schema->typeLength = typeLength;
	schema->keySize = keySize;
	schema->keyAttrs = keys;
	return schema;
}

//Written by Kachikwu Nwike
extern RC freeSchema (Schema *schema)
{
	free(schema);
	return RC_OK;
}

//Written by Kachikwu Nwike
extern RC createRecord (Record **record, Schema *schema)
{
	Record *newRecord = (Record*) malloc(sizeof(Record)); 
	int recSize = getRecordSize(schema);
    newRecord->data= (char*) malloc(sizeof(char) * recSize); //allocate memory for record
    //initialize
    newRecord->id.page = -10; //random
    newRecord->id.slot = -10; //random
    char *p = newRecord->data;
    *p = '-';
    p++;
    *(p) = '\0';
    *record = newRecord;
    return RC_OK;

}

//Written by Kachikwu Nwike
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    int i=0;
    int offSetValue = 1;
    
    while (i < attrNum) {
        if (schema->dataTypes[i] == DT_STRING) {
            offSetValue += schema->typeLength[i];
        } else if (schema->dataTypes[i] == DT_INT) {
            offSetValue += sizeof(int);
        } else if (schema->dataTypes[i] == DT_FLOAT) {
            offSetValue += sizeof(float);
        } else if (schema->dataTypes[i] == DT_BOOL) {
            offSetValue += sizeof(bool);
        }
        i++;
    }
    char *dataPointer = record->data;
    dataPointer += offSetValue;

// Set the value of the attribute based on its data type
    if (value->dt == DT_STRING) {
		strncpy(dataPointer, value->v.stringV, schema->typeLength[attrNum]);
		dataPointer += schema->typeLength[attrNum];
	}

    else if (value->dt == DT_INT) {
		*(int *) dataPointer = value->v.intV;
    	dataPointer += sizeof(int);
	}

    else if (value->dt == DT_FLOAT) {
		*(float *) dataPointer = value->v.floatV;
    	dataPointer += sizeof(float);
	}

    else if (value->dt == DT_BOOL) {
		*(bool *) dataPointer = value->v.boolV;
    	dataPointer += sizeof(bool);
	}

    return RC_OK;
}

//Written by Kachikwu Nwike
extern RC freeRecord (Record *record)
{
	if(record != NULL){
		free(record);
		return RC_OK;
	}
	return RC_ERROR;
}
//Written by Deshon Langdon
extern RC getAttr(Record *record, Schema *schema, int newAttrIndex  , Value **value) {
      int index  = 0;
    int sizeOfInt   = sizeof(int);
    int sizeOfBool  = sizeof(bool);
    int offSetValue = 1;
    int sizeOfFloat  = sizeof(float);
   

   // Calculate the offset value based on the attribute's data type and its index in the schema
    while (index  < newAttrIndex  ) {
        if (schema->dataTypes[index] == DT_STRING) {
            offSetValue += schema->typeLength[index ];
        } else if (schema->dataTypes[index ] == DT_INT) {
            offSetValue += sizeOfInt  ;
        } else if (schema->dataTypes[index ] == DT_FLOAT) {
            offSetValue += sizeOfFloat ;
        } else if (schema->dataTypes[index ] == DT_BOOL) {
            offSetValue += sizeOfBool ;
        }
        index++;
    }
while(newAttrIndex   == 1){
    	schema->dataTypes[newAttrIndex  ] = 1;
    	break;
    }
   char *newDataPtr   = record->data;
newDataPtr   += offSetValue;

Value *newAttribute  = (Value*) malloc(sizeof(Value));


   // Retrieve the value based on the attribute's data type
    if (schema->dataTypes[newAttrIndex] == DT_STRING && sizeOfInt > 0) {
    char* stringData = (char*) malloc(schema->typeLength[newAttrIndex] + 1);
    if (stringData == NULL) {
        free(newAttribute);
        return RC_OTHER_ERROR;
    }
    strncpy(stringData, newDataPtr, schema->typeLength[newAttrIndex]);
    stringData[schema->typeLength[newAttrIndex]] = '\0';
    newAttribute->v.stringV = stringData;
    newAttribute->dt = DT_STRING;

} else if (schema->dataTypes[newAttrIndex  ] == DT_INT && sizeOfInt   > 0) {
            int value = 0;
            memcpy(&value, newDataPtr  , sizeOfInt  );
            newAttribute ->v.intV = value;
            newAttribute ->dt = DT_INT;
        
    } else if (schema->dataTypes[newAttrIndex  ] == DT_FLOAT && sizeOfFloat  > 0 ) {
        
            float value = 0;
            memcpy(&value, newDataPtr  , sizeOfFloat  );
            newAttribute ->v.floatV = value;
            newAttribute ->dt = DT_FLOAT;
        
    } else if (schema->dataTypes[newAttrIndex  ] == DT_BOOL && sizeOfBool  > 0 ) {
       
            bool value = 0;
            memcpy(&value, newDataPtr  , sizeOfBool );
            newAttribute ->v.boolV = value;
            newAttribute ->dt = DT_BOOL;
        
    }

    *value = newAttribute ;
    return RC_OK;
}



