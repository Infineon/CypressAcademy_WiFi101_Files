#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include "cyhal.h"

// the dbEntry is the structure that is stored in the linked list.
typedef struct dbEntry {
    uint32_t deviceId;
    uint32_t regId;
    uint32_t value;
    struct dbEntry *next;
} dbEntry_t;

//Find Function
struct dbEntry *dbFind(struct dbEntry *head, struct dbEntry *find);
//setvalue function
void dbSetValue(struct dbEntry *head, struct dbEntry *newValue);
//getmax function
uint32_t dbGetMax();
//getcount function
uint32_t dbGetCount(dbEntry_t *head);

#endif
