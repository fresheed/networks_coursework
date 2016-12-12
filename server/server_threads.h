void* server_proc_thread(void* raw_node_ptr);
int serverProcMessage(message* msg, messages_set* st, primes_pool* pool);

int processServerRequest(message* msg, messages_set* set, primes_pool* pool);
int processServerResponse(message* msg, messages_set* set, primes_pool* pool);
