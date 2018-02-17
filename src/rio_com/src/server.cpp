#include "net.h"
#include <iostream>
#include <cstdlib>

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
    
    printf("Starting server...\n");
    status = start_server(conn, port);
    if (status == -1) error("Problem starting server.");
    printf("Receiving data...\n");
    status = recv_data(conn, buffer, size, 1000);
    if (status == -1) error("Problem receiving data.");
    if (status == -2) error("Receiver timed out.");
    printf("Data: %s\n", buffer); 
    /* send response back */
    status = send_data(conn, buffer, size);
    if (status == -1) error("Problem sending data.");
    close_conn(conn);   
}
