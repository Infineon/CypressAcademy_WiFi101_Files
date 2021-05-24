//simple linked list implementation
#include "cyhal.h"
#include "linkedList.h"

#define dbMax (400)
uint32_t dbGetMax()
{
    return dbMax ;
}

// dbFind:
// Search the database for specific deviceId/regId combination
dbEntry_t *dbFind(dbEntry_t *head, dbEntry_t *find){
	dbEntry_t *rval = NULL;
	//loop through linked list and check if any entries match
	while(head != NULL){
		if(head->deviceId == find->deviceId && head->regId == find->regId){
			rval = head;
			return rval;
		}
		head = head->next;
	}
	return rval;

}

// dbSetValue
// searches the database, if newValue is not found then it inserts it or
// overwrite the value if it is found
void dbSetValue(dbEntry_t *head, dbEntry_t *newValue){
    dbEntry_t *found = dbFind(head, newValue);
    if(found) // if it is already in the database
    {
        found->value = newValue->value;
    }
    else // add it to the linked list
    {
        while(head->next != NULL){
        	head = head->next;
        }
        head->next = newValue;
    }
}

//get number of entries in the list
uint32_t dbGetCount(dbEntry_t *head){
    uint32_t count = 1;
    while(head->next != NULL){
    	head = head->next;
    	count++;
    }
    return count;

}
