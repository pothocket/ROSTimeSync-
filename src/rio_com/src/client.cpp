#include <iostream>
#include <cstring>
#include <cstdlib>
#include "net.h"


static conn_t conn;

void error(char * msg) {
	std::cout << msg << std::endl;
	close_conn(conn);
	exit(1);
}

int main(int arg_count, char ** args) {
    int port = 22222;
    if (arg_count >= 2) {
        port = atoi(args[1]);
    }
    int status;

	const int size = 128;
	char buffer[size];	

	strcpy(buffer, "hello there, sir!");
	
	printf("Starting client...\n");
	status = start_client(conn, "127.0.0.1", port);
	if (status == -1) error("Problem starting client.");

	printf("Printing data: %s ...\n", buffer);
	status = send_data(conn, buffer, size);
	if (status == -1) error("Problem sending data.");
    status = recv_data(conn, buffer, size, 1000);
	if (status == -1) error("Problem sending data.");
    printf("Got data: %s \n", buffer);    
    close_conn(conn); 
}
