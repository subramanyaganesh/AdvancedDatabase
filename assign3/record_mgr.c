#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

const int MaxNumPages = 100;
const int AttrSize = 15;
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

//Written by Siddharth Sharma
int findFreeSlot(char *data, int recordSize)
{
    int i=0;
    int totalSlots = PAGE_SIZE / recordSize;

    while(i < totalSlots){
        if (data[i * recordSize] != '+'){
            return i;
        }i++;
    }
    return -1;
}

//Written by Siddharth Sharma
extern RC initRecordManager (void *mgmtData)
{
	initStorageManager();
	return RC_OK;
}

//Written by Siddharth Sharma
extern RC shutdownRecordManager ()
{
	RC returnCode;
	if(recMgr != NULL){
		recMgr = NULL;
		free(recMgr);
		returnCode = RC_OK;
	}
	else{
		free(recMgr);
		returnCode = RC_OK;
	}
	return returnCode;
}

//Written by Avadhoot Kodag
extern RC createTable (char *name, Schema *schema)
{
	char data[PAGE_SIZE];
	if(sizeof(RecMgr) > 0){
		recMgr = (RecMgr*) malloc(sizeof(RecMgr));
		char *pHandle = data;
		if(initBufferPool(&recMgr->bufferPool, name, MaxNumPages, RS_LRU, NULL) == RC_OK){
			int res;
			int intSize = sizeof(int);
			*(int*)pHandle = 0;
			pHandle += intSize;
			*(int*)pHandle = 1;
			pHandle += intSize;
			*(int*)pHandle = schema->numAttr;
			pHandle += intSize;
			*(int*)pHandle = schema->keySize;
			pHandle += intSize;

			int k;
			for(k = 0; k < schema->numAttr; k++)
				{
					strncpy(pHandle, schema->attrNames[k], AttrSize);
					pHandle += AttrSize;
					*(int*)pHandle = (int)schema->dataTypes[k];
					pHandle += intSize;
					*(int*)pHandle = (int) schema->typeLength[k];
					pHandle += intSize;
				}
			SM_FileHandle fileHandle;
			if(((res = createPageFile(name)) != RC_OK) || ((res = openPageFile(name, &fileHandle)) != RC_OK) || ((res = writeBlock(0, &fileHandle, data)) != RC_OK) || ((res = closePageFile(&fileHandle)) != RC_OK))
				return res;

			return RC_OK;
		}
		else{
			return RC_ERROR;
		}
	}
	else{
		return RC_ERROR;
	}
}

//Written by Avadhoot Kodag
Schema* setSchema(Schema* schema, int attrCnt){
	if(attrCnt >= 0){
		schema->numAttr = attrCnt;
		if(attrCnt >= 0)
			schema->attrNames = (char**) malloc(sizeof(char*) *attrCnt);
		if(attrCnt >= 0)
			schema->dataTypes = (DataType*) malloc(sizeof(DataType) *attrCnt);
		if(attrCnt >= 0)
			schema->typeLength = (int*) malloc(sizeof(int) *attrCnt);
		return schema;
	}
	else{
		return schema;
	}
}

//Written by Avadhoot Kodag
extern RC openTable (RM_TableData *rel, char *name)
{
	SM_PageHandle pageHandle;    
	int attributeCount;
	int intSize = sizeof(int);

	rel->mgmtData = recMgr;
	rel->name = name;

	if(pinPage(&recMgr->bufferPool, &recMgr->pHandle, 0)== RC_OK)
	{
	
	pageHandle = (char*) recMgr->pHandle.data;
	
	recMgr->tuplesCnt= *(int*)pageHandle;
	pageHandle = pageHandle + intSize;

	recMgr->freePage= *(int*) pageHandle;
    	pageHandle = pageHandle + intSize;
	
    	attributeCount = *(int*)pageHandle;
	pageHandle = pageHandle + intSize;
 	
	Schema *schema;
	schema = (Schema*) malloc(sizeof(Schema));
	schema = setSchema(schema,attributeCount);

	int k = 0;
	while(k < attributeCount){
		schema->attrNames[k]= (char*) malloc(AttrSize);
		k++;
	}
	int x = 0;
	while(x < schema->numAttr)
    	{
		strncpy(schema->attrNames[x], pageHandle, AttrSize);
		pageHandle += AttrSize;
	   
		schema->dataTypes[x]= *(int*) pageHandle;
		pageHandle += intSize;

		schema->typeLength[x]= *(int*)pageHandle;
		pageHandle += intSize;

		x++;
	}
	
	rel->schema = schema;	
	unpinPage(&recMgr->bufferPool, &recMgr->pHandle);
	forcePage(&recMgr->bufferPool, &recMgr->pHandle);
	return RC_OK;
	}
	else{
		return RC_ERROR;
	}
}

//Written by Vaishnavi Mule
extern RC deleteTable (char *name)
{
	if(destroyPageFile(name) == RC_OK)
		return RC_OK;
	else
		return RC_ERROR;
}

//Written by Vaishnavi Mule
extern RC closeTable (RM_TableData *rel)
{
	RecMgr *recordManager = rel->mgmtData;
	if(shutdownBufferPool(&recordManager->bufferPool) == RC_OK || RC_PINNED_PAGES_IN_BUFFER)
		return RC_OK;
	else
		return RC_ERROR;
}

//Written by Vaishnavi Mule
extern int getNumTuples (RM_TableData *rel)
{
	RecMgr *recordManager = rel->mgmtData;
	int tupleCnt = recordManager->tuplesCnt;
	if(tupleCnt > 0)
		return tupleCnt;
	else
		return 0;
}

//Written by Siddharth Sharma
extern RC insertRecord (RM_TableData *rel, Record *record)
{
	int freeSlot;
	RecMgr *recMgr = rel->mgmtData;
	int rSize;
	RID *recId = &record->id; 
	char *d, *slot;
	recId->page = recMgr->freePage;
	if(pinPage(&recMgr->bufferPool, &recMgr->pHandle, recMgr->freePage) == RC_OK){
		rSize = getRecordSize(rel->schema);
		d = recMgr->pHandle.data;
		freeSlot = findFreeSlot(recMgr->pHandle.data, rSize);
		recId->slot = freeSlot;
		int i = 0;
		for(i = 0; recId->slot == -1; i++){
			if(unpinPage(&recMgr->bufferPool, &recMgr->pHandle) == RC_OK){
				recId->page++;
				if(pinPage(&recMgr->bufferPool, &recMgr->pHandle, recId->page) == RC_OK){
					rSize = getRecordSize(rel->schema);
					d = recMgr->pHandle.data;
					freeSlot = findFreeSlot(recMgr->pHandle.data, rSize);
					recId->slot = freeSlot;
				}
				else{
					return RC_ERROR;
				}
			}
			else{
				return RC_ERROR;
			}
		}
		slot = d;
		if(markDirty(&recMgr->bufferPool, &recMgr->pHandle) == RC_OK){
			slot += (recId->slot * getRecordSize(rel->schema));
			int recSize = getRecordSize(rel->schema) - 1;
			*slot = '+';
			memcpy(++slot, record->data + 1, recSize);
			if(unpinPage(&recMgr->bufferPool, &recMgr->pHandle) == RC_OK){
				recMgr->tuplesCnt++;
				if(pinPage(&recMgr->bufferPool, &recMgr->pHandle, 0) == RC_OK)
					return RC_OK;
				else
					return RC_ERROR;
			}
			else
				return RC_ERROR;
		}
		else
			return RC_ERROR;
	}
	else
		return RC_ERROR;
}

//Written by Siddharth Sharma
extern RC deleteRecord (RM_TableData *rel, RID id)
{

	int pageNum;
	RecMgr *recordManager = rel->mgmtData;
	pageNum = id.page;
	if(pinPage(&recordManager->bufferPool, &recordManager->pHandle, pageNum) == RC_OK){
		if(pageNum > 0)
			recordManager->freePage = pageNum;
		char *data = recordManager->pHandle.data;
		data += (id.slot * getRecordSize(rel->schema));
		*data = '-';
		if(markDirty(&recordManager->bufferPool, &recordManager->pHandle) == RC_OK){
			if(pageNum > 0){
				if(unpinPage(&recordManager->bufferPool, &recordManager->pHandle) == RC_OK){
					return RC_OK;
				}
				else{
					return RC_ERROR;
				}
			}
		}
		else{
			return RC_ERROR;
		}
		return RC_OK;
	}
	else{
		return RC_ERROR;
	}
}

//Written by Siddharth Sharma
extern RC updateRecord (RM_TableData *rel, Record *record)
{	
	RecMgr *recordManager = rel->mgmtData;
	
	if(pinPage(&recordManager->bufferPool, &recordManager->pHandle, record->id.page) == RC_OK){
		char *data;
		RID id = record->id;
		data = recordManager->pHandle.data;
		data += (id.slot * getRecordSize(rel->schema));
		*data = '+';
		memcpy(++data, record->data + 1, getRecordSize(rel->schema) - 1 );
		if(markDirty(&recordManager->bufferPool, &recordManager->pHandle) == RC_OK){
			if(unpinPage(&recordManager->bufferPool, &recordManager->pHandle) == RC_OK){
				return RC_OK;
			}
			else{
				return RC_ERROR;
			}
		}
		else{
			return RC_ERROR;
		}

		return RC_OK;
	}
	else{
		return RC_ERROR;
	}
}

//Written by Vaishnavi Mule
extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
	RecMgr *recordManager = rel->mgmtData;
	if(pinPage(&recordManager->bufferPool, &recordManager->pHandle, id.page)== RC_OK){
	getRecordSize(rel->schema);
	char *dataPointer = recordManager->pHandle.data;
	dataPointer += (id.slot * getRecordSize(rel->schema));
	if(*dataPointer == '+')
	{
		record->id = id;
		char *data = record->data;
		memcpy(++data, dataPointer + 1, getRecordSize(rel->schema) - 1);
	}
	else
	{
		return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
	}
	unpinPage(&recordManager->bufferPool, &recordManager->pHandle);
	return RC_OK;
	}
	else{
		return RC_ERROR;
	}
}

//Written by Siddharth Sharma
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	if (cond != NULL)
	{
	    RecMgr *scanMgr;
		RecMgr *tableMgr;
		if(openTable(rel, "ScanTable") == RC_OK){
			scanMgr = (RecMgr*) malloc(sizeof(RecMgr));
			scan->mgmtData = scanMgr;
			scanMgr->rID.page = 1;
		   	scanMgr->rID.slot = scanMgr->scanCnt = 0;
			scanMgr->cond = cond;
		    scan->rel= rel;
		    tableMgr = rel->mgmtData;
		    tableMgr->tuplesCnt = AttrSize;
			return RC_OK;
			}
			else {
				return RC_ERROR;
			}
	}
	else{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}
}

//Written by Avadhoot Kodag
extern RC next (RM_ScanHandle *scan, Record *record)
{
	RecMgr *scanMgr = scan->mgmtData;
	if (scanMgr->cond != NULL)
	{
		RecMgr *tableMgr = scan->rel->mgmtData;
		Schema *schema = scan->rel->schema;
		Value *res = (Value *) malloc(sizeof(Value));
		char *data;
		if(getRecordSize(schema) > 0){
			int totalSlots = PAGE_SIZE / getRecordSize(schema);
			int scanCount = scanMgr->scanCnt;
			if(tableMgr->tuplesCnt != 0){
				while(scanCount <= tableMgr->tuplesCnt)
				{
					if(scanCount > 0){
						scanMgr->rID.slot++;
						if(scanMgr->rID.slot >= totalSlots)
						{
							scanMgr->rID.slot = 0;
							scanMgr->rID.page++;
						}
					}
					else{
						scanMgr->rID.page = 1;
						scanMgr->rID.slot = 0;
					}

					if(pinPage(&tableMgr->bufferPool, &scanMgr->pHandle, scanMgr->rID.page) == RC_OK){
						record->id.page = scanMgr->rID.page;
						record->id.slot = scanMgr->rID.slot;
						data = scanMgr->pHandle.data;
						data += (scanMgr->rID.slot * getRecordSize(schema));
						char *dataPointer = record->data;
						*dataPointer = '-';
						memcpy(++dataPointer, data + 1, getRecordSize(schema) - 1);
						scanMgr->scanCnt++;
						scanCount++;
						evalExpr(record, schema, scanMgr->cond, &res);
						bool eval = res->v.boolV;
						if(eval)
						{
							unpinPage(&tableMgr->bufferPool, &scanMgr->pHandle);
							return RC_OK;
						}
					}
					else{
						return RC_ERROR;
					}

				}
				if(unpinPage(&tableMgr->bufferPool, &scanMgr->pHandle) == RC_OK){
					scanMgr->rID.page = 1;
					scanMgr->rID.slot =scanMgr->scanCnt = 0;
					return RC_RM_NO_MORE_TUPLES;
				}
				else{
					scanMgr->rID.page = 1;
					scanMgr->rID.slot =scanMgr->scanCnt = 0;
					return RC_RM_NO_MORE_TUPLES;
				}
			}
			else
				return RC_RM_NO_MORE_TUPLES;
		}
		else{
			return RC_ERROR;
		}
	}
	else{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}
}

//Written by Vaishnavi Mule
extern RC closeScan (RM_ScanHandle *scan)
{
	if(scan != NULL){
		RecMgr *scanManager = scan->mgmtData;
		RecMgr *recordManager = scan->rel->mgmtData;
		if(scanManager->scanCnt > 0)
			{
				if(unpinPage(&recordManager->bufferPool, &scanManager->pHandle)==RC_OK){
					scanManager->scanCnt = 0;
					scanManager->rID.page = 1;
					scanManager->rID.slot = 0;

				}
				else{
					return RC_ERROR;
				}
			}
		    	scan->mgmtData = NULL;
		    	free(scan->mgmtData);

			return RC_OK;
	}
	else
		return RC_ERROR;
}

//Written by Vaishnavi Mule
extern int getRecordSize (Schema *schema)
{
    int size = 0;
    int i = 0;
    while( i<schema->numAttr)
    {
        if(schema->dataTypes[i] == DT_STRING)
            size = size + schema->typeLength[i];
        else if (schema->dataTypes[i]==DT_INT)
                size = size + sizeof(int);
        else if(schema->dataTypes[i]==DT_FLOAT)
                size = size + sizeof(float);
        else if(schema->dataTypes[i]==DT_BOOL)
                size = size + sizeof(bool);
    	i+=1;
    }
    return ++size;
}

//Written by Avadhoot Kodag
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	if(typeLength > 0){
		Schema *schema = (Schema *) malloc(sizeof(Schema));
		if(numAttr > 0)
			schema->numAttr = numAttr;
		schema->attrNames = attrNames;
		schema->dataTypes = dataTypes;
		if(typeLength > 0)
			schema->typeLength = typeLength;
		schema->keySize = keySize;
		schema->keyAttrs = keys;
		return schema;
	}
	else
		return (Schema *) malloc(sizeof(Schema));

}

//Written by Vaishnavi Mule
extern RC freeSchema (Schema *schema)
{
	if(schema != NULL)
		free(schema);
	return RC_OK;
}

//Written by Siddharth Sharma
extern RC createRecord (Record **record, Schema *schema)
{
	Record *newRecord = (Record*) malloc(sizeof(Record));
	char test = '-';
	if(getRecordSize(schema) > 0){
		newRecord->data= (char*) malloc(getRecordSize(schema));
		newRecord->id.page = -1;
		newRecord->id.slot = -1;
		char *dataPointer = newRecord->data;
		*dataPointer = test;
		*(++dataPointer) = '\0';
		*record = newRecord;
		return RC_OK;
	}
	else {
		return RC_ERROR;
	}
}

//Written by Vaishnavi Mule
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    int intSize = sizeof(int);
    int floatSize = sizeof(float);
    int boolSize = sizeof(bool);
    int i=0;
    int offSetValue = 1;
    while(i < attrNum)
    {
        while (schema->dataTypes[i]==DT_STRING){
        	offSetValue += schema->typeLength[i];
        	break;
        }

        while(schema->dataTypes[i]==DT_INT){
        	offSetValue += intSize;
        	break;
        }

        while(schema->dataTypes[i]==DT_FLOAT){
        	offSetValue += floatSize;
        	break;
        }
        while(schema->dataTypes[i]==DT_BOOL){
        	offSetValue += boolSize;
        	break;
        }

    	i+=1;
    }
    char *dataPointer = record->data;
    dataPointer += offSetValue;

    while(schema->dataTypes[attrNum]==DT_STRING)
    {
		int length = schema->typeLength[attrNum];
		if(length > 0){
			strncpy(dataPointer, value->v.stringV, length);
			dataPointer += schema->typeLength[attrNum];
		}
		break;
    }
    while(schema->dataTypes[attrNum]==DT_INT)
	{
    	if(intSize > 0){
    		*(int *) dataPointer = value->v.intV;
    		dataPointer += intSize;
    	}
		break;
	}
    while(schema->dataTypes[attrNum]==DT_FLOAT)
	{
    	if(floatSize > 0){
    		*(float *) dataPointer = value->v.floatV;
    		dataPointer += floatSize;
    	}
		break;
	}
    while(schema->dataTypes[attrNum]==DT_BOOL)
	{
    	if(boolSize > 0){
    		*(bool *) dataPointer = value->v.boolV;
    		dataPointer += boolSize;
    	}
		break;
	}
    return RC_OK;
}

//Written by Avadhoot Kodag
extern RC freeRecord (Record *record)
{
	if(record != NULL){
		free(record);
		return RC_OK;
	}
	else
		return RC_ERROR;
}

//Written by Vaishnavi Mule
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    int intSize = sizeof(int);
    int floatSize = sizeof(float);
    int boolSize = sizeof(bool);
    int i=0;
    int offSetValue = 1;
    while(i < attrNum)
    {
        while (schema->dataTypes[i]==DT_STRING){
        	offSetValue += schema->typeLength[i];
        	break;
        }

        while(schema->dataTypes[i]==DT_INT){
        	offSetValue += intSize;
        	break;
        }

        while(schema->dataTypes[i]==DT_FLOAT){
        	offSetValue += floatSize;
        	break;
        }
        while(schema->dataTypes[i]==DT_BOOL){
        	offSetValue += boolSize;
        	break;
        }

    	i+=1;
    }
    Value *attribute = (Value*) malloc(sizeof(Value));
    char *dataPointer = record->data;
    dataPointer += offSetValue;
    while(attrNum == 1){
    	schema->dataTypes[attrNum] = 1;
    	break;
    }

    while(schema->dataTypes[attrNum]==DT_STRING)
    {
    	if(intSize > 0){
    		attribute->v.stringV = (char *) malloc(schema->typeLength[attrNum] + 1);
    		strncpy(attribute->v.stringV, dataPointer, schema->typeLength[attrNum]);
    		if(floatSize > 0){
        		attribute->v.stringV[schema->typeLength[attrNum]] = '\0';
        		attribute->dt = DT_STRING;
    		}
    	}
		break;
    }

    while(schema->dataTypes[attrNum]==DT_INT)
    {
		int value = 0;
		if(intSize > 0){
			memcpy(&value, dataPointer, intSize);
			attribute->v.intV = value;
			attribute->dt = DT_INT;
		}
		break;
    }
    while(schema->dataTypes[attrNum]==DT_FLOAT)
    {
    	float value;
    	if(floatSize > 0){
    		memcpy(&value, dataPointer, floatSize);
    		attribute->v.floatV = value;
    		attribute->dt = DT_FLOAT;
    	}
    	break;
    }
    while(schema->dataTypes[attrNum]==DT_BOOL)
    {
		bool value;
		if(boolSize > 0){
			memcpy(&value,dataPointer, boolSize);
			attribute->v.boolV = value;
			attribute->dt = DT_BOOL;
		}
		break;
    }
    *value = attribute;
    return RC_OK;
}


