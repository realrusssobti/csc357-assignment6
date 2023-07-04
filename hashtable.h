#ifndef HASHTABLE_H
#define HASHTABLE_H

#define TABLE_SIZE 100 // Update the table size as needed

typedef struct {
    char key[33];   /* 32 char + null */
    char value[1025];   /* 1024 char */
} KeyValuePair;

typedef struct {
    KeyValuePair** data;
} HashTable;

int hash(const char* key);

int update(HashTable* hashtable, char* key, char* value);

void destroyHashTable(HashTable* hashtable);

HashTable* createHashTable(void);

void insert(HashTable* hashtable, const char* key, const char* value);

char* get(HashTable* hashtable, const char* key);

#endif  /* HASHTABLE_H */

