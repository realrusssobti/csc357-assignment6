// Description: A simple HTTP server that handles GET, HEAD, and PUT requests.
// Include statements
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/wait.h>

// Constants
#define MAX_REQUEST_SIZE 1024
#define MAX_RESPONSE_SIZE 65536

void get_value(const char *server_fifo, const char *key, char *output_value) {
	int fifo_fd, fifo_read_fd;
	char *other_fifo = "output_fifo";
	char command[100];
	char buffer[BUFSIZ];
	char *token;

	memset(buffer, 0, sizeof(buffer));

	// Create the command string for GET operation
	snprintf(command, sizeof(command), "get %s", key);

	// Open server FIFOs
	fifo_fd = open(server_fifo, O_WRONLY);
	fifo_read_fd = open(other_fifo, O_RDONLY);
	usleep(100);

	if (fifo_read_fd == -1 || fifo_fd == -1) {
		perror("open failure");
		exit(EXIT_FAILURE);
	}

	// Send command to server
	if (write(fifo_fd, command, strlen(command)) == -1) {
		perror("write failed");
		close(fifo_fd);
		exit(1);
	}

	// Read the response from the server
	ssize_t num_bytes = read(fifo_read_fd, buffer, sizeof(buffer));
	if (num_bytes == -1) {
		perror("Error reading from FIFO");
		exit(EXIT_FAILURE);
	}

	// Null-terminate buffer
	buffer[num_bytes] = '\0';

	// Print the value
	printf("%s", buffer);

	// Close server FIFOs
	close(fifo_fd);
	close(fifo_read_fd);
	// set the output value
	strcpy(output_value, buffer);
}

void set_value(const char *server_fifo, const char *key, const char *value) {
	int fifo_fd;
	char command[100];

	// Create the command string for SET operation
	snprintf(command, sizeof(command), "set %s %s", key, value);

	// Open server FIFO
	fifo_fd = open(server_fifo, O_WRONLY);
	usleep(100);

	if (fifo_fd == -1) {
		perror("open failure");
		exit(EXIT_FAILURE);
	}

	// Send command to server
	if (write(fifo_fd, command, strlen(command)) == -1) {
		perror("write failed");
		close(fifo_fd);
		exit(1);
	}

	// Close server FIFO
	close(fifo_fd);
}


// handles the client's requests
void handle_client_request(int client_socket, const char *kvstore_fifo) {
	// Read the client request
	char request[MAX_REQUEST_SIZE];
	ssize_t request_size = read(client_socket, request, sizeof(request) - 1);
	if (request_size == -1) {
		perror("Error reading request");
		exit(EXIT_FAILURE);
	}
	request[request_size] = '\0';

	// Parse the request to extract request type, endpoint, and HTTP version
	char request_type[10];
	char endpoint[100];
	sscanf(request, "%s %s", request_type, endpoint);

	// Handle different types of requests
	if (strcmp(request_type, "GET") == 0) {
		// TODO: Handle GET request
		printf("GET request received for endpoint %s\n", endpoint);

		// Check if the endpoint starts with /kv/
		if (strncmp(endpoint, "/kv/", 4) == 0) {
			// Get the key from the endpoint
			char *key = endpoint + 4;
			char value[1024];
			printf("GET request received for key %s \n", key);
			// handle GET request
			get_value(kvstore_fifo, key, value);
			// return response
			char response[MAX_RESPONSE_SIZE];
			snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n"
			                                     "Content-Type: text/html\r\n"
			                                     "\r\n");
			write(client_socket, response, strlen(response));
			// write the value to the client
			snprintf(response, sizeof(response), "%s", value);
			printf("value: %s\n", value);
			write(client_socket, response, strlen(response));
		} else {


			// Open the requested file
			FILE *file = fopen(endpoint + 1, "r");
			if (file == NULL) {
				// File not found
				char response[MAX_RESPONSE_SIZE];
				snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\n"
				                                     "Content-Type: text/html\r\n"
				                                     "\r\n"
				                                     "<html><body><h1>404 Not Found</h1></body></html>\r\n");
				write(client_socket, response, strlen(response));
			} else {
				// Read the file contents
				char buffer[MAX_RESPONSE_SIZE];
				ssize_t num_bytes_read;
				size_t response_size = 0;
				while ((num_bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
//				write(client_socket, buffer, num_bytes_read);
// print the buffer
//				printf("%s", buffer);
					response_size += num_bytes_read;
				}
				fclose(file);

				// Send the response header
				char response[MAX_RESPONSE_SIZE];
				snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n"
				                                     "Content-Type: text/html\r\n"
				                                     "Content-Length: %zu\r\n"
				                                     "\r\n", response_size);
				write(client_socket, response, strlen(response));
				write(client_socket, buffer, response_size);
			}
		}


	} else if (strcmp(request_type, "HEAD") == 0) {
		printf("HEAD request received for endpoint %s\n", endpoint);
		// Open the requested file
		FILE *file = fopen(endpoint + 1, "r");
		if (file == NULL) {
			// File not found
			char response[MAX_RESPONSE_SIZE];
			snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\n"
			                                     "Content-Type: text/html\r\n"
			                                     "\r\n");
			write(client_socket, response, strlen(response));
		} else {
			// Get the file size
			fseek(file, 0L, SEEK_END);
			size_t file_size = ftell(file);
			fclose(file);

			// Send the response header
			char response[MAX_RESPONSE_SIZE];
			snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n"
			                                     "Content-Type: text/html\r\n"
			                                     "Content-Length: %zu\r\n"
			                                     "\r\n", file_size);
			write(client_socket, response, strlen(response));
		}
	} else if (strcmp(request_type, "PUT") == 0) {
		// Check if the endpoint starts with /kv/
		if (strncmp(endpoint, "/kv/", 4) == 0) {
			// Get the key from the endpoint
			char *key = endpoint + 4;
			// Get the value from the request body
			char *value = strstr(request, "\r\n\r\n") + 4;
			printf("PUT request received for key %s and value %s\n", key, value);
			// handle PUT request
			set_value(kvstore_fifo, key, value);
			// return response
			char response[MAX_RESPONSE_SIZE];
			snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n"
			                                     "Content-Type: text/html\r\n"
			                                     "\r\n");
			write(client_socket, response, strlen(response));

		} else {
			// Invalid endpoint for PUT request
			char response[MAX_RESPONSE_SIZE];
			snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\r\n"
			                                     "Content-Type: text/html\r\n"
			                                     "\r\n"
			                                     "<html><body><h1>400 Bad Request</h1></body></html>\r\n");
			write(client_socket, response, strlen(response));
		}
	} else {
		// Invalid request type
		char response[MAX_RESPONSE_SIZE];
		snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\r\n"
		                                     "Content-Type: text/html\r\n"
		                                     "\r\n"
		                                     "<html><body><h1>400 Bad Request. get good lol</h1></body></html>\r\n");
		write(client_socket, response, strlen(response));
	}

	// Close the client socket
	close(client_socket);
}

void wait_child_processes() {
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0) {
		if (WIFEXITED(status)) {
			int exit_status = WEXITSTATUS(status);
			printf("Child process terminated with status %d\n", exit_status);
		} else if (WIFSIGNALED(status)) {
			int signal_number = WTERMSIG(status);
			printf("Child process terminated due to signal %d\n", signal_number);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <kvstore_fifo> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *kvstore_fifo = argv[1];
	int port = atoi(argv[2]);

	// Create a socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	// Bind the socket to the specified port
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);
	if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
		perror("Error binding socket");
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(server_socket, 10) == -1) {
		perror("Error listening for connections");
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d\n", port);

	// Main server loop
	while (1) {
		// Accept a client connection
		struct sockaddr_in client_address;
		socklen_t client_address_size = sizeof(client_address);
		int client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_size);
		if (client_socket == -1) {
			perror("Error accepting client connection");
			continue;
		}

		// Fork a child process to handle the client request
		pid_t pid = fork();
		if (pid == -1) {
			perror("Error forking child process");
			continue;
		} else if (pid == 0) {
			// Child process
			close(server_socket);  // Close the server socket in the child process
			handle_client_request(client_socket, kvstore_fifo);
			exit(EXIT_SUCCESS);
		} else {
			// Parent process
			close(client_socket);  // Close the client socket in the parent process
			wait_child_processes();  // Check for terminated child processes
		}
	}

	// Close the server socket
	close(server_socket);

	return 0;
}
