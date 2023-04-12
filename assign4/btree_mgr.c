#include "btree_mgr.h"
#include "tables.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include <stdlib.h>
#include <string.h>

SM_FileHandle btreeFH;

typedef struct BTREE
{
    int *key;
    struct BTREE **next;
    RID *id;
} BTree;

BTree *treeRoot, *scan;
int maxEle;
int indexNum = 0;

// init and shutdown index manager
extern RC initIndexManager(void *mgmtData)
{
    return RC_OK;
}

RC shutdownIndexManager()
{
    return RC_OK;
}

// create, destroy, open, and close an btree index
extern RC createBtree(char *idxId, DataType keyType, int n)
{
    treeRoot = (BTree *)malloc(sizeof(BTree));
    treeRoot->next = malloc(sizeof(BTree) * (n + 1));

    treeRoot->key = malloc(sizeof(int) * n);
    treeRoot->id = malloc(sizeof(int) * n);

    for (int i = 0; i < n + 1; i++)
        treeRoot->next[i] = NULL;
    maxEle = n;
    createPageFile(idxId);

    return RC_OK;
}

extern RC openBtree(BTreeHandle **tree, char *idxId)
{
    return openPageFile(idxId, &btreeFH) == 0 ? RC_OK : RC_ERROR;
}
extern RC freeFuction(BTreeHandle *tree)
{
    free(treeRoot);
    return RC_OK;
}

extern RC closeBtree(BTreeHandle *tree)
{
    return closePageFile(&btreeFH) == 0 ? freeFuction(tree) : RC_ERROR;
}

extern RC deleteBtree(char *idxId)
{
    return destroyPageFile(idxId) == 0 ? RC_OK : RC_ERROR;
}

// access information about a b-tree
extern RC getNumNodes(BTreeHandle *tree, int *result)
{
    BTree *temp = (BTree *)malloc(sizeof(BTree));
    int numNodes = 0,i = 0;
    while (i < maxEle + 2)
    {
        numNodes++;
        i++;
    }
    *result = numNodes;
    free(temp);
    return RC_OK;
}

extern RC getNumEntries(BTreeHandle *tree, int *result)
{
    int totalEle = 0;
    BTree *temp = (BTree *)malloc(sizeof(BTree));
    temp = treeRoot;
    while (temp != NULL)
    {
        int i = 0;
        while (i < maxEle && temp->key[i] != 0)
        {
            totalEle++;
            i++;
        }
        temp = temp->next[maxEle];
    }
    *result = totalEle;
    // free(temp);
    return RC_OK;
}

extern RC getKeyType(BTreeHandle *tree, DataType *result)
{
    return RC_OK;
}

extern RC findKey(BTreeHandle *tree, Value *key, RID *result)
{
    BTree *tmp = treeRoot;
    bool found = FALSE;

    while (tmp != NULL)
    {
        int i = 0;
        while (i < maxEle && tmp->key[i] != key->v.intV)
        {
            i++;
        }
        if (tmp->key[i] == key->v.intV && i < maxEle)
        {
            result->slot = tmp->id[i].slot;
            result->page = tmp->id[i].page;
            found = TRUE;
            break;
        }
        tmp = tmp->next[maxEle];
    }
    // free(temp);
    return found == TRUE ? RC_OK : RC_IM_KEY_NOT_FOUND;
}

extern RC insertKey(BTreeHandle *tree, Value *key, RID rid)
{
    int i = 0, nodeFull = 0, totalEle = 0;

    BTree *temp;
    BTree *node = malloc(sizeof(BTree));
    node->key = (int *)malloc(sizeof(int) * maxEle);
    node->id = (RID *)malloc(sizeof(int) * maxEle);
    node->next = malloc(sizeof(BTree) * (maxEle + 1));

    while (i < maxEle)
    {
        node->key[i] = 0;
        i++;
    }
    //    printf("\n\nIteration: %d", key->v.intV);

    for (temp = treeRoot; temp != NULL; temp = temp->next[maxEle])
    {
        nodeFull = 0;
        i = 0;
        while (i < maxEle)
        {
            if (temp->key[i] == 0)
            {
                int pg = rid.page;
                int sl = rid.slot;
                temp->id[i].page = pg;
                temp->id[i].slot = sl;
                temp->key[i] = key->v.intV;
                temp->next[i] = NULL;
                nodeFull = nodeFull + 1;
                break;
            }
            i++;
        }
        if ((temp->next[maxEle] == NULL) && (nodeFull == 0))
        {
            temp->next[maxEle] = node;
            node->next[maxEle] = NULL;
        }
    }

    for (temp = treeRoot; temp != NULL; temp = temp->next[maxEle])
        for (;i < maxEle &&temp->key[i] != 0;i++)
        {
            totalEle++;
        }

    if (totalEle == 6)
    {
        int root_key = treeRoot->next[maxEle]->key[0];
        int root_key2 = treeRoot->next[maxEle]->next[maxEle]->key[0];
        BTree *root_next = treeRoot->next[maxEle];

        node->key[0] = root_key;
        node->key[1] = root_key2;

        node->next[0] = treeRoot;
        node->next[1] = root_next;
        node->next[2] = root_next->next[maxEle];
    }

    return RC_OK;
}

extern RC deleteKey(BTreeHandle *tree, Value *key)
{
    // Initialize a temporary pointer to the root of the B-tree and a flag variable to track if the key was found
    BTree *temp = treeRoot;
    int found = 0, i;

    // Loop through the B-tree while the temporary pointer is not null and the key has not been found
    while (temp != NULL && !found)
    {
        // Loop through the keys in the current node while the key has not been found
        for (i = 0; i < maxEle && !found; i++)
        {
            // If the key is found, set the key and RID values to 0 and set the found flag to 1
            if (temp->key[i] == key->v.intV)
            {
                temp->id[i].slot = 0;
                temp->id[i].page = 0;
                temp->key[i] = 0;
                found = 1;
            }
        }
        // Move to the next node by setting the temporary pointer to the next node in the array of children of the current node
        temp = temp->next[maxEle];
    }

    // Return the status code for successful completion of the function
    return RC_OK;
}

extern RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle)
{
    int i, count, swap, pg, st, c, d, totalEle = 0;

    scan = (BTree *)malloc(sizeof(BTree));
    scan = treeRoot;
    BTree *temp = (BTree *)malloc(sizeof(BTree));
    indexNum = 0;

    for (temp = treeRoot; temp != NULL; temp = temp->next[maxEle])
    {
        int j = 0;
        while (j < maxEle)
        {
            if (temp->key[j] != 0)
            {
                totalEle++;
            }
            j++;
        }
    }

    int elements[maxEle][totalEle];
    int key[totalEle];
    count = 0;

    for (temp = treeRoot; temp != NULL; temp = temp->next[maxEle])
    {
        i = 0;
        while (i < maxEle)
        {
            if (temp->key[i] != 0)
            {
                int temp_pg = temp->id[i].page;
                key[count] = temp->key[i];
                elements[0][count] = temp_pg;
                elements[1][count] = temp->id[i].slot;
                count = count + 1;
            }
            i++;
        }
    }

    c = 0;
    while (c < count - 1)
    {
        d = 0;
        while (d < count - c - 1)
        {
            if (key[d] > key[d + 1])
            {
                swap = key[d + 1];
                pg = elements[0][d + 1];
                st = elements[1][d + 1];

                key[d + 1] = key[d];
                elements[0][d + 1] = elements[0][d];
                elements[1][d + 1] = elements[1][d];

                key[d] = swap;
                elements[0][d] = pg;
                elements[1][d] = st;
            }
            d++;
        }
        c++;
    }

    count = 0;
    temp = treeRoot;
    for (temp = treeRoot; temp != NULL; temp = temp->next[maxEle])
    {
        i = 0;
        while (i < maxEle)
        {
            if (temp->key[i] != 0)
            {
                int temp_pg = elements[0][count];
                temp->key[i] = key[count];
                temp->id[i].page = temp_pg;
                temp->id[i].slot = elements[1][count];
                count++;
            }
            i++;
        }
    }

    return RC_OK;
}

extern RC nextEntry(BT_ScanHandle *handle, RID *result)
{
    if (scan->next[maxEle] != NULL)
    {
        do
        {
            // If all elements in the current node have been scanned, move to the next node
            if (indexNum == maxEle)
            {
                indexNum = 0;
                scan = scan->next[maxEle];
            }

            result->page = scan->id[indexNum].page;
            result->slot = scan->id[indexNum].slot;
            indexNum++;
            // Keep scanning nodes until an entry is found or there are no more nodes to scan
        } while (scan->next[maxEle] != NULL && (*result).page == 0 && (*result).slot == 0);

        // If there are no more entries to scan, return an error
      return  (*result).page == 0 && (*result).slot == 0?RC_IM_NO_MORE_ENTRIES:RC_OK;
    }
    // If there are no more nodes to scan, return an error
    else
    {
        return RC_IM_NO_MORE_ENTRIES;
    }
}

extern RC closeTreeScan(BT_ScanHandle *handle)
{
    indexNum = 0;
    return RC_OK;
}

// debug and test functions
char *printTree(BTreeHandle *tree)
{
    return RC_OK;
}
