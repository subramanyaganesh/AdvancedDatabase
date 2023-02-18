#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

static void testCustomCases(void);


/* main function running all tests */
int
main (void)
{
  testName = "";
  initStorageManager();
  testCustomCases();
  return 0;
}

void testCustomCases(void){
	  SM_FileHandle fh;
	  SM_PageHandle ph;
	  int i;

	  testName = "test new addition cases";

	  ph = (SM_PageHandle) malloc(PAGE_SIZE);

	  // create a new page file
	  TEST_CHECK(createPageFile (TESTPF));
	  TEST_CHECK(openPageFile (TESTPF, &fh));
	  printf("created and opened file\n");

	  // read first page into handle
	  TEST_CHECK(readFirstBlock (&fh, ph));
	  // the page should be empty (zero bytes)
	  for (i=0; i < PAGE_SIZE; i++)
	    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
	  printf("first block was empty\n");

	  // change ph to be a string and write that one to disk
	  for (i=0; i < PAGE_SIZE; i++)
		  ph[i] = (i % 10) + '0';

	  // Writing string ph to first page of file.
	  TEST_CHECK(writeBlock(0, &fh, ph));
	  printf("writing first block successful \n");


	  // Testing new write function

	  // write string ph to the current file position.
	  TEST_CHECK(writeCurrentBlock(&fh, ph));
	  printf("Writing in current block \n");

	  // Testing new read functions

	  // Reading the page and checking whether it is matching with the string
	  //Reading First Block
	  TEST_CHECK(readFirstBlock (&fh, ph));
	  for (i=0; i < PAGE_SIZE; i++)
		  ASSERT_TRUE((ph[i] == (i % 10) + '0'), "Expected character is present.");
      printf("reading from first block \n");

      // Reading the current block
      TEST_CHECK(readCurrentBlock (&fh, ph));
      for (i=0; i < PAGE_SIZE; i++)
    	  ASSERT_TRUE((ph[i] == (i % 10) + '0'), "Expected character is present.");
      printf("reading from Current block\n");

      // Reading the next block
      TEST_CHECK(readNextBlock (&fh, ph));
      for (i=0; i < PAGE_SIZE; i++)
    	  ASSERT_TRUE((ph[i] == (i % 10) + '0'), "Expected character is present.");
      printf("reading from Next block\n");

      // Reading previous block
      TEST_CHECK(readPreviousBlock (&fh, ph));
      for (i=0; i < PAGE_SIZE; i++)
    	  ASSERT_TRUE((ph[i] == (i % 10) + '0'), "Expected character is present.");
      printf("reading from previous block\n");

      // Reading last block
      TEST_CHECK(readLastBlock (&fh, ph));
      for (i=0; i < PAGE_SIZE; i++)
    	  ASSERT_TRUE((ph[i] == (i % 10) + '0'), "Expected character is present.");
      printf("reading from Last block\n");

      // Test Capacity .
      TEST_CHECK(ensureCapacity(12,&fh));

      TEST_CHECK(destroyPageFile (TESTPF));
      printf(" PageFile destroyed\n");
      TEST_DONE();
}

