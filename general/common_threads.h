
void* common_send_thread(void* raw_node_ptr);
void* common_recv_thread(void* raw_node_ptr);
int readN(int socket_fd, char* read_buf);
void endCommunication(node_data* node);
