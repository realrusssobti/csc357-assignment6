#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#define TABLE_SIZE 100
#include "hashtable.h"

int input_fifo, output_fifo;
char *server_fifo;
char* out_fifo_name = "output_fifo";
HashTable* hashtable;

void handle_signal(){
	/* Destroy table, close file, unlink */
	destroyHashTable(hashtable);
	close(input_fifo);
	close(output_fifo);
	unlink(server_fifo);
	unlink(out_fifo_name);
	exit(EXIT_FAILURE);
}



int main (int argc, char* argv[]) {

	char buf[BUFSIZ];
	int  n;
	memset(buf, 0, sizeof(buf));
	char *database;
	char token[32];
	char key[32];
	char value[1024];
	FILE* file;
	char line[1024];
	int numTokens;
	char* k;
	char* v;
	char* answer;
	char error[100];

	if (argc < 2) {
		fprintf(stderr, "usage: ./server <db file name> <input fifo name>\n");
		exit(EXIT_FAILURE);
	}

	/* Create Hashtable and fill with database */
	hashtable = createHashTable();
	database = argv[1];
	server_fifo = argv[2];

	/* Open file with key's and values */
	file = fopen(database, "r+");

	/* Register signal handler for SIGINT (Ctrl+C) */
	signal(SIGINT, handle_signal);

	if (file == NULL){
		printf("Failed to open file \n");
		exit(EXIT_FAILURE);
	}

	/* Insert all values into hashtable */
	fseek(file, 0, SEEK_END);
	long size = ftell(file);

	/* Check if file is empty and add values if not */
	if (size != 0){
		fseek(file, 0, SEEK_SET);

		while (fgets(line, sizeof(line), file) != NULL) {
			k = strtok(line, ",");
			v = strtok(NULL, ",");
			insert(hashtable, k, v);
		}

	}
	fclose(file);
	// check if server_fifo exists
	if (access(server_fifo, F_OK) == -1){
		/* Create server_fifo */
		printf("Creating fifo at %s\n", server_fifo);
		if(mkfifo(server_fifo, 0666) == -1){
			printf("creating input fifo failed");
			exit(EXIT_FAILURE);
		}
	}

	// check if output_fifo exists
	if (access(out_fifo_name, F_OK) == -1) {
		/* Create output fifo */

		if (mkfifo(out_fifo_name, 0666) == -1){
			printf("creating output fifo failed");
			exit(EXIT_FAILURE);
		}
	}




	//	Create a server to read and another to write
	input_fifo = open(server_fifo, O_RDONLY);
	output_fifo = open(out_fifo_name, O_WRONLY);

//	main loop to process requests
	while(1) {

		/* Check if value sent */
		n = read(input_fifo, buf, BUFSIZ);

		if (n == EOF || n <= 0){
			continue;
		} else {

			/* Save the input, save key */
			buf[n] = '\0';

			/* If 3 then SET, else GET */
			numTokens = sscanf(buf, "%s %s %[^\n]", token, key, value);

			/* Null terminate */
			buf[0] = '\0';


			if (strcmp(token, "set") == 0 && numTokens == 3){
				/* Set */
				/* Try to update existing key's value */
				value[strlen(value) - 1] = '\n';

				if (!update(hashtable, key, value)){
					/* If the key doesn't exist, insert the kv pair */
					insert(hashtable, key, value);
				}

				/* Update the database file */
				file = fopen(database, "r+");
				if (file == NULL){
					/* Check if the file opened properly */
					fprintf(stderr, "Failed to open file \n");
					exit(EXIT_FAILURE);
				}

				/* Remove all contents from file */
				if (ftruncate(fileno(file), 0) != 0){
					fprintf(stderr, "Failed to truncate file \n");
					exit(EXIT_FAILURE);
				}

				/* write all values in hashtable to file */
				for (int i = 0; i < TABLE_SIZE; i++){
					if (hashtable->data[i] != NULL) {
						fprintf(file, "%s,%s", hashtable->data[i]->key,
						        hashtable->data[i]->value);
					}
				}

				fclose(file);


			} else if (strcmp(token, "get") == 0 && numTokens == 2) {
				/* Get */
				answer = get(hashtable, key);
				/* Check if key exists" */

				if (answer == NULL){
					sprintf(error, "Key %s does not exist.\n", key);
					usleep(1000);

					/* Write, if failed shut down */
					if (write(output_fifo, error, strlen(error)) == -1){
						perror("write failed");
						exit(EXIT_FAILURE);
					}

					continue;

				} else {
					usleep(1000);
					memset(buf, 0, sizeof(buf));

					if (write(output_fifo, answer, strlen(answer)) == -1) {
						perror("write failed");
						exit(EXIT_FAILURE);
					}


				}
				/* Free the answer */
				free(answer);
			}
			/* Clean */
			memset(buf, 0, sizeof(buf));
			fflush(stdout);
		}
	}

	/* Exit */
	destroyHashTable(hashtable);
	return 0;

}

