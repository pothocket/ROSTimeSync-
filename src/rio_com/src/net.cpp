#include "net.h"

int send_data(conn_t & conn, char * buffer, int buffer_len) {
    return sendto(conn.local_socket, buffer, buffer_len, 0, 
            (sockaddr *) &conn.remote_addr, conn.addr_len);
}

int recv_data(conn_t & conn, char * buffer, int buffer_len, int time_out) {
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * time_out;
    fd_set read_set;
    FD_SET(conn.local_socket, &read_set);
    int event = select(conn.local_socket + 1, &read_set, 0, 0, &tv);
    if (event > 0 && FD_ISSET(conn.local_socket, &read_set)) {
        int status = recvfrom(conn.local_socket, buffer, buffer_len, 0, 
            (sockaddr *) &conn.remote_addr, &conn.addr_len);
            // make  sure address length is correct
            conn.addr_len = sizeof(conn.remote_addr);
            return status;
    } else if (event == 0) {
        // timed out
        return -2;
    } else {
        // some error happened
        return -1;
    }
}

int start_server(conn_t & conn, int port) {
    // open socket
    conn.local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (conn.local_socket == -1) return -1;
    // set address size
    conn.addr_len = sizeof(conn.remote_addr);
        // create local address
    sockaddr_in local_addr;
    // clear address with 0's
    memset((char *) &local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr - htonl(INADDR_ANY);
    // bind local address to port
    int status = bind(conn.local_socket, (sockaddr *) &local_addr, 
            sizeof(local_addr));
    if (status == -1) return -1;
}

int start_client(conn_t & conn, char * server_addr, int port) {
    // open socket
    conn.local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (conn.local_socket == -1) return -1;
    // set address size
    conn.addr_len = sizeof(conn.remote_addr);
    // clear remote address with 0's
    memset((char *) &conn.remote_addr, 0, conn.addr_len);
    // setup remote address
    conn.remote_addr.sin_family = AF_INET;
    conn.remote_addr.sin_port = htons(port);
    int status = inet_aton(server_addr, &conn.remote_addr.sin_addr);
    if (status == 0) return -1;
    return 0;
}

void close_conn(conn_t & conn) {
    close(conn.local_socket);
}

