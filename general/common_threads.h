
void* common_send_thread(void* raw_node_ptr);
void* common_recv_thread(void* raw_node_ptr);
int readN(socket_conn conn, char* read_buf, int message_len);
