
void* common_send_thread(void* raw_node_ptr);
void* common_recv_thread(void* raw_node_ptr);
int readN(int socket_fd, char* read_buf, int message_len);
void endCommunication(node_data* node);
int recvMessageContent(message* msg, int socket_fd);
int sendMessageContent(message* msg, int socket_fd);
