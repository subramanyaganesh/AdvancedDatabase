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
	BM_BufferPool bufferPool;
	RID rID;
	Expr *cond;
	int tuplesCnt;
	int freePage;
	int scanCnt;
} RecMgr;

RecMgr *recMgr;



int findFreeRecSlot(char *data, int recSize)
{
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


extern RC initRecordManager(void* mgmtData) {
    initStorageManager();
	return RC_OK;
}

extern RC shutdownRecordManager() {
    if (recMgr == NULL) {
        return RC_OK;
    }

    free(recMgr);
    recMgr = NULL;

    return RC_OK;
}

extern RC createTable(char *name, Schema *schema)
{
    char tableData[PAGE_SIZE];
    recMgr = (RecMgr*) malloc(sizeof(RecMgr));
    char *pHandle  = tableData;
	SM_FileHandle fileHandle;
    initBufferPool(&recMgr->bufferPool, name, MaxPagesNum, RS_LRU, NULL);
    int result = createPageFile(name);
    result |= openPageFile(name, &fileHandle);

    int intSize = sizeof(int);
int header[] = {0, 1, schema->numAttr, schema->keySize};
int numHeaderValues = sizeof(header) / intSize;

for (int i = 0; i < numHeaderValues; i++) {
    *(int*)pHandle = header[i];
    pHandle += intSize;
}

int attrIdx = 0;
    while (attrIdx < schema->numAttr) {
        strncpy(pHandle , schema->attrNames[attrIdx], recSizeAttr);
    pHandle  += recSizeAttr;
    
    
    switch(schema->dataTypes[attrIdx]) {
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
    
   
    if (schema->dataTypes[attrIdx] == DT_STRING) {
        *(int*)pHandle = schema->typeLength[attrIdx];
    } else {
        *(int*)pHandle = 0;
    }
    pHandle += intSize;
    
    attrIdx++;
    }
    result |= writeBlock(0, &fileHandle, tableData);
    result |= closePageFile(&fileHandle);
    return result == RC_OK ? RC_OK : RC_ERROR;
}



Schema* setSchema(Schema* schema, int attrCnt) {
    schema->numAttr = (attrCnt >= 0) ? attrCnt : schema->numAttr;
    schema->attrNames = (attrCnt >= 0) ? (char**) malloc(sizeof(char*) * attrCnt) : schema->attrNames;
    schema->dataTypes = (attrCnt >= 0) ? (DataType*) malloc(sizeof(DataType) * attrCnt) : schema->dataTypes;
    schema->typeLength = (attrCnt >= 0) ? (int*) malloc(sizeof(int) * attrCnt) : schema->typeLength;
    return schema;
}


extern RC openTable(RM_TableData *rel, char *name) {
    SM_PageHandle pageHandle;    
    int attributeCount;
    int intSize = sizeof(int);
    RC result;

    rel->mgmtData = recMgr;
    rel->name = name;

    result = pinPage(&recMgr->bufferPool, &recMgr->pHandle, 0);
    if (result != RC_OK) return result;

    pageHandle = (char*) recMgr->pHandle.data;
    recMgr->tuplesCnt = *(int*)pageHandle;
    pageHandle += intSize;
    recMgr->freePage = *(int*) pageHandle;
    pageHandle += intSize;
    attributeCount = *(int*)pageHandle;
    pageHandle += intSize;
 
    Schema *schema = (Schema*) malloc(sizeof(Schema));
    setSchema(schema, attributeCount);
 
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

    rel->schema = schema;    
    unpinPage(&recMgr->bufferPool, &recMgr->pHandle);
    forcePage(&recMgr->bufferPool, &recMgr->pHandle);
    return RC_OK;
}



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


extern int getNumTuples (RM_TableData *rel)
{
	RecMgr *recordManager = rel->mgmtData;
return recordManager->tuplesCnt > 0 ? recordManager->tuplesCnt : 0;
}


extern RC insertRecord (RM_TableData *rel, Record *record)
{
	RecMgr *recordManager = rel->mgmtData;
	RID *recordId = &record->id; 
	char *data, *slot;
	recordId->page = recordManager->freePage;
	pinPage(&recMgr->bufferPool, &recMgr->pHandle, recMgr->freePage);
    data = recordManager->pHandle.data;
    recordId->slot = findFreeRecSlot(recordManager->pHandle.data, getRecordSize(rel->schema));

    while (recordId->slot < 0){
        unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
        recordId->page++;
        pinPage(&recordManager->bufferPool, &recordManager->pHandle, recordId->page);
        data = recordManager->pHandle.data;
        recordId->slot = findFreeRecSlot(recordManager->pHandle.data, getRecordSize(rel->schema));
    }
    
    slot = data;
    markDirty(&recordManager->bufferPool, &recordManager->pHandle);

    slot += (recordId->slot * getRecordSize(rel->schema));
    *slot = '+';
    memcpy(++slot, record->data + 1, getRecordSize(rel->schema) - 1);
    unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
    
    recordManager->tuplesCnt++;
    pinPage(&recordManager->bufferPool, &recordManager->pHandle, 0);
       
    return RC_OK;

}

extern RC deleteRecord (RM_TableData *rel, RID id)
{
	RecMgr *recordManager = rel->mgmtData;
	pinPage(&recordManager->bufferPool, &recordManager->pHandle, id.page);
	recordManager->freePage = id.page;
	char *data = recordManager->pHandle.data;
	data += (id.slot * getRecordSize(rel->schema));
	*data = '-';
	markDirty(&recordManager->bufferPool, &recordManager->pHandle);
	unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
	return RC_OK;
}

//Written by Siddharth Sharma
extern RC updateRecord (RM_TableData *rel, Record *record)
{	
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


extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
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


extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    RecMgr *scanMgr;
    RecMgr *tableMgr;
    RC status;

    status = (cond != NULL) ? openTable(rel, "ScanTable") : RC_SCAN_CONDITION_NOT_FOUND;

    if (status == RC_OK) {
scanMgr = (RecMgr*) malloc(sizeof(RecMgr));
if (scanMgr == NULL) {
return RC_SCAN_MGR_CREATE_FAILED;
}
scan->mgmtData = scanMgr;
scanMgr->rID.page = 1;
scanMgr->rID.slot = scanMgr->scanCnt = 0;
scanMgr->cond = cond;
scan->rel = rel;
tableMgr = rel->mgmtData;
if (tableMgr == NULL) {
free(scanMgr);
return RC_RM_UNINITIALIZED;
}
tableMgr->tuplesCnt = recSizeAttr;
}
    return status;
}




extern RC next(RM_ScanHandle *scan, Record *record) {
    RecMgr *scanMgr = scan->mgmtData;
    switch(scanMgr->cond != NULL) {
        case true: {
            RecMgr *tableMgr = scan->rel->mgmtData;
            Schema *schema = scan->rel->schema;
            Value *res = (Value *) malloc(sizeof(Value));
            char *data;

            if (getRecordSize(schema) <= 0) {
                return RC_ERROR;
            }

            int maxSlots = PAGE_SIZE / getRecordSize(schema);
            int scanCount = scanMgr->scanCnt;

            switch (tableMgr->tuplesCnt != 0) {
                case true: {
                    for (; scanCount <= tableMgr->tuplesCnt;) {
                        scanMgr->rID.slot += (scanCount > 0) ? 1 : 0;
                         scanMgr->rID.page += (scanCount > 0 && scanMgr->rID.slot >= maxSlots) ? 1 : 0;
                          scanMgr->rID.slot = (scanCount > 0 && scanMgr->rID.slot >= maxSlots) ? 0 : scanMgr->rID.slot;
                           scanMgr->rID.page = (scanCount == 0) ? 1 : scanMgr->rID.page;

                        RC pinPageStatus = pinPage(&tableMgr->bufferPool, &scanMgr->pHandle, scanMgr->rID.page);
                        if (pinPageStatus == RC_OK) {
                            char *dataPointer = record->data;
            *dataPointer = '-';

            record->id.page = scanMgr->rID.page;
            record->id.slot = scanMgr->rID.slot;

            data = scanMgr->pHandle.data;
            data += scanMgr->rID.slot * getRecordSize(schema);


            for (int i = 1; i < getRecordSize(schema); i++) {
    dataPointer[i] = data[i];
}

scanMgr->scanCnt++;
scanCount++;

            evalExpr(record, schema, scanMgr->cond, &res);
            bool eval = (bool) res->v.boolV;

            if (eval) {
                RC unpinPageStatus = unpinPage(&tableMgr->bufferPool, &scanMgr->pHandle);
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

                    RC unpinPageStatus = unpinPage(&tableMgr->bufferPool, &scanMgr->pHandle);
                    scanMgr->rID.page = 1;
                    scanMgr->rID.slot =scanMgr->scanCnt = 0;
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


extern RC closeScan(RM_ScanHandle *scan) {
    RecMgr *scanManager = (scan != NULL) ? scan->mgmtData : NULL;
    if (scanManager != NULL && scanManager->scanCnt > 0) {
        RecMgr *recordManager = scan->rel->mgmtData;
        RC rc = unpinPage(&recordManager->bufferPool, &scanManager->pHandle);
        if (rc != RC_OK) {
            return RC_ERROR;
        }
        scanManager->scanCnt = 0;
        scanManager->rID.page = 1;
        scanManager->rID.slot = 0;
    }
    if (scan != NULL) {
        scan->mgmtData = NULL;
        free(scanManager);
        return RC_OK;
    }
    return RC_ERROR;
}

extern int getRecordSize (Schema *schema)
{
	int val = 0;
    int i = 0;
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

extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *schema = (Schema *) malloc(sizeof(Schema));
	
	schema->numAttr = numAttr;
	schema->attrNames = attrNames;
	schema->dataTypes = dataTypes;
	schema->typeLength = typeLength;
	schema->keySize = keySize;
	schema->keyAttrs = keys;
	return schema;
}


extern RC freeSchema (Schema *schema)
{
	free(schema);
	return RC_OK;
}


extern RC createRecord (Record **record, Schema *schema)
{
	Record *newRecord = (Record*) malloc(sizeof(Record));
	int recSize = getRecordSize(schema);
    newRecord->data= (char*) malloc(sizeof(char) * recSize);
    newRecord->id.page = -10; //random
    newRecord->id.slot = -10; //random
    char *p = newRecord->data;
    *p = '-';
    p++;
    *(p) = '\0';
    *record = newRecord;
    return RC_OK;

}


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


extern RC freeRecord (Record *record)
{
	if(record != NULL){
		free(record);
		return RC_OK;
	}
	return RC_ERROR;
}

extern RC getAttr(Record *record, Schema *schema, int newAttrIndex  , Value **value) {
      int index  = 0;
    int sizeOfInt   = sizeof(int);
    int sizeOfBool  = sizeof(bool);
    int offSetValue = 1;
    int sizeOfFloat  = sizeof(float);
   

   
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


   
    if (schema->dataTypes[newAttrIndex  ] == DT_STRING) {
        if (sizeOfInt   > 0) {
            newAttribute ->v.stringV = (char*) malloc(schema->typeLength[newAttrIndex  ] + 1);
            if (newAttribute ->v.stringV == NULL) {
                free(newAttribute );
                return RC_OTHER_ERROR; 
            }
            strncpy(newAttribute ->v.stringV, newDataPtr  , schema->typeLength[newAttrIndex  ]);
            newAttribute ->v.stringV[schema->typeLength[newAttrIndex  ]] = '\0';
            newAttribute ->dt = DT_STRING;
        }
    } else if (schema->dataTypes[newAttrIndex  ] == DT_INT) {
        if (sizeOfInt   > 0) {
            int value = 0;
            memcpy(&value, newDataPtr  , sizeOfInt  );
            newAttribute ->v.intV = value;
            newAttribute ->dt = DT_INT;
        }
    } else if (schema->dataTypes[newAttrIndex  ] == DT_FLOAT) {
        if (sizeOfFloat  > 0) {
            float value = 0;
            memcpy(&value, newDataPtr  , sizeOfFloat );
            newAttribute ->v.floatV = value;
            newAttribute ->dt = DT_FLOAT;
        }
    } else if (schema->dataTypes[newAttrIndex  ] == DT_BOOL) {
        if (sizeOfBool  > 0) {
            bool value = 0;
            memcpy(&value, newDataPtr  , sizeOfBool );
            newAttribute ->v.boolV = value;
            newAttribute ->dt = DT_BOOL;
        }
    }

    *value = newAttribute ;
    return RC_OK;
}



