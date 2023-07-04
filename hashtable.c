#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#define TABLE_SIZE 100


typedef struct{
	char key[33]; /* 32 char + null */
	char value[1025]; /* 1024 char */
} KeyValuePair;

typedef struct{
	KeyValuePair** data;
} HashTable;


int hash(const char* key){
	/* Hash based on string */
	int sum = 0;
	int i;
	for (i = 0; key[i] != '\0'; i++){
		sum += key[i];
	}
	return sum % TABLE_SIZE;
}


int update(HashTable *hashtable, char *key, char *value) {
	/* Check if key already exists */
	int index = hash(key);
	if (hashtable->data[index] != NULL && strcmp(hashtable->data[index]->key,
	                                             key) == 0) {
		/* Copy value into table */
		strncpy(hashtable->data[index]->value, value,
		        sizeof(hashtable->data[index]->value) - 1);
		/* Null Termination */
		hashtable->data[index]->value[sizeof(hashtable->data[index]->value)
		                              - 1] = '\0';
		/* Return that value updated */
		return 1;
	} else {
		/* Key not found */
		return 0;
	}

}



void destroyHashTable(HashTable* hashtable){
	int i;
	/* Remove all elements */
	for (i = 0; i < TABLE_SIZE; i++){
		if (hashtable->data[i] != NULL){
			free(hashtable->data[i]);
		}
	}
	/* Free hashtable */
	free(hashtable->data);
	free(hashtable);
}


HashTable* createHashTable() {
	/* Create hashtable */
	HashTable* hashtable = (HashTable*)malloc(sizeof(HashTable));
	hashtable->data = (KeyValuePair**)calloc(TABLE_SIZE,
	                                         sizeof(KeyValuePair*));
	if (hashtable->data == NULL) {
		/* Error handling is allocation fails */
		free(hashtable);
		return NULL;
	}
	return hashtable;
}

void insert(HashTable* hashtable, const char* key, const char* value){
	/* Get index */
	int index = hash(key);
	/* Check if already a value */
	if (hashtable->data[index] != NULL) {
		free(hashtable->data[index]);
	}
	/* Create kvp */
	KeyValuePair* kvp = (KeyValuePair*)malloc(sizeof(KeyValuePair));
	if (kvp == NULL) {
		/* Error handling if memory allocation fails */
		return;
	}
	/* Copy key and value */
	strncpy(kvp->key, key, sizeof(kvp->key) - 1);;
	kvp->key[sizeof(kvp->key) - 1] = '\0'; /* Ensure null termination */


	strncpy(kvp->value, value, sizeof(kvp->value) - 1);
	kvp->value[sizeof(kvp->value) - 1] = '\0'; /* Ensure null termination */

	hashtable->data[index] = kvp;
}

char* get(HashTable* hashtable, const char* key) {
	/* Get index and check if it exists */
	int index = hash(key);

	if (hashtable->data[index] != NULL && strcmp(hashtable->data[index]->key,
	                                             key) == 0) {

		/* Allocate memory for the returned value */
		char* result = malloc(sizeof(hashtable->data[index]->value));
		if (result != NULL) {
			strncpy(result, hashtable->data[index]->value,
			        sizeof(hashtable->data[index]->value) - 1);
			result[sizeof(hashtable->data[index]->value) - 1] = '\0';
			/* Ensure null termination */

			return result;
		}
	}

	return NULL; /* Key not found */
}

