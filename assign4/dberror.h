#ifndef DBERROR_H
#define DBERROR_H

#include "stdio.h"

/* module wide constants */
#define PAGE_SIZE 4096

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_FILE_HANDLE_NOT_INIT 2
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_ERROR_WHILE_CLOSE 5
#define RC_ERROR_ON_DESTROY 6
#define RC_ERROR_INVALID_PAGENUM 7
#define RC_WRITE_NON_EXISTING_PAGE 8
#define RC_ERROR 400 // Added a new definiton for ERROR
#define RC_PINNED_PAGES_IN_BUFFER 500 // Added a new definition for Buffer Manager
#define RC_SCAN_MGR_CREATE_FAILED 902
#define RC_RM_UNINITIALIZED 903
#define NO_FREE_SLOTS 904

#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_EXPR_RESULT_IS_NOT_BOOLEAN 201
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_RM_NO_MORE_TUPLES 203
#define RC_RM_NO_PRINT_FOR_DATATYPE 204
#define RC_RM_UNKOWN_DATATYPE 205
#define RC_TABLE_TOO_LARGE 206
#define RC_RECORD_TOO_LARGE 207
#define RC_OTHER_ERROR 304
#define RC_MALLOC_FAILED 305

#define RC_IM_KEY_NOT_FOUND 300
#define RC_IM_KEY_ALREADY_EXISTS 301
#define RC_IM_N_TO_LAGE 302
#define RC_IM_NO_MORE_ENTRIES 303

#define RC_MELLOC_MEM_ALLOC_FAILED 400
#define RC_SCHEMA_NOT_INIT 401
#define RC_PIN_PAGE_FAILED 402
#define RC_UNPIN_PAGE_FAILED 403
#define RC_INVLD_PAGE_NUM 404
#define RC_IVALID_PAGE_SLOT_NUM 405 
#define RC_MARK_DIRTY_FAILED 406
#define RC_BUFFER_SHUTDOWN_FAILED 407 
#define RC_NULL_IP_PARAM 408
#define RC_FILE_DESTROY_FAILED 409
#define RC_OPEN_TABLE_FAILED 410

#define RC_ERROR_WHILE_FLUSHING_TO_POOL 602
#define RC_ERROR_IN_OPEN_PAGEFILE 700
#define RC_ERROR_WHILE_ENSURE_CAPACITY 701
#define RC_RM_NO_TUPLE_WITH_GIVEN_RID 600
#define RC_SCAN_CONDITION_NOT_FOUND 601

/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError (RC error);
extern char *errorMessage (RC error);

#define THROW(rc,message) \
  do {			  \
    RC_message=message;	  \
    return rc;		  \
  } while (0)		  \

// check the return code and exit if it is an error
#define CHECK(code)							\
  do {									\
    int rc_internal = (code);						\
    if (rc_internal != RC_OK)						\
      {									\
	char *message = errorMessage(rc_internal);			\
	printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
	free(message);							\
	exit(1);							\
      }									\
  } while(0);


#endif
