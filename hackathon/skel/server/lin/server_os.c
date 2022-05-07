/**
 * Hackathon SO: LogMemCacher
 * (c) 2020-2021, Operating Systems
 */
#define _GNU_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "../../include/server.h"
#define THREAD_NUM 20

char *lmc_logfile_path;

static struct LinkedList *queue;
static pthread_mutex_t write_lock;

void queue_add(struct LinkedList *q, struct lmc_client *value)
{
    pthread_mutex_lock(&q->lock);

	add_nth_node(q, q->count, value);

    pthread_mutex_unlock(&q->lock);

    pthread_cond_signal(&q->is_available);
}


void *queue_get(void *args)
{
	struct LinkedList *q = (struct LinkedList *) args;

	while(1) {
 		pthread_mutex_lock(&q->lock);

		while (q->count == 0)
			pthread_cond_wait(&q->is_available, &q->lock);

		struct lmc_client * client = remove_nth_node(q, 0)->data;
		
		pthread_mutex_unlock(&q->lock);
		int disc = 0;
		lmc_get_command(client, &disc);
		if(disc == 0) {
			queue_add(queue, client);
		} else {
			close(client->client_sock);
			free(client);
		}
	}
	return NULL;
}

/**
 * Client connection loop function. Creates the appropriate client connection
 * socket and receives commands from the client in a loop.
 *
 * @param client_sock: Socket to communicate with the client.
 *
 * @return: This function always returns 0.
 *
 * TODO: The lmc_get_command function executes blocking operations. The server
 * is unable to handle multiple connections simultaneously.
 */
static int lmc_client_function(SOCKET client_sock)
{
	struct lmc_client *client;

	client = lmc_create_client(client_sock);
	queue_add(queue, client);

	return 0;
}

/**
 * Server main loop function. Opens a socket in listening mode and waits for
 * connections.
 */
void
lmc_init_server_os(void)
{
	int sock, client_size, client_sock;
	struct sockaddr_in server, client;
	int opten;

	queue = malloc(sizeof(*queue));

	pthread_mutex_init(&write_lock, NULL);

	memset(&server, 0, sizeof(struct sockaddr_in));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return;

	opten = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opten, sizeof(opten));

	server.sin_family = AF_INET;
	server.sin_port = htons(LMC_SERVER_PORT);
	server.sin_addr.s_addr = inet_addr(LMC_SERVER_IP);

	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Could not bind");
		exit(1);
	}

	if (listen(sock, LMC_DEFAULT_CLIENTS_NO) < 0) {
		perror("Error while listening");
		exit(1);
	}

	pthread_t th[THREAD_NUM];
	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_create(&th[i], NULL, queue_get, queue);
	}

	while (1) {
		memset(&client, 0, sizeof(struct sockaddr_in));
		client_size = sizeof(struct sockaddr_in);
		client_sock = accept(sock, (struct sockaddr *)&client,
				(socklen_t *)&client_size);

		if (client_sock < 0) {
			perror("Error while accepting clients");
		}

		lmc_client_function(client_sock);
	}

	free(queue);
}

/**
 * OS-specific client cache initialization function.
 *
 * @param cache: Cache structure to initialize.
 *
 * @return: 0 in case of success, or -1 otherwise.
 *
 * TODO: Implement proper handling logic.
 */
int
lmc_init_client_cache(struct lmc_cache *cache)
{
	lmc_logfile_path = malloc(100);
	memcpy(lmc_logfile_path, "logs_lmc/", 9);
	memcpy(lmc_logfile_path + 9, cache->service_name, strlen(cache->service_name));
	memcpy(lmc_logfile_path + 9 + strlen(cache->service_name), ".log", 5);

	cache->log_fd = open(lmc_logfile_path, O_CREAT | O_RDWR, 0666);
	free(lmc_logfile_path);

	cache->dim = 0;
	cache->log_line_num = 0;
	if(cache->log_fd == -1) {
		return -1;
	}
	cache->pages = 1;
	cache->ptr = mmap(NULL, getpagesize(), PROT_WRITE, MAP_SHARED, cache->log_fd, 0);
	if(cache->ptr == MAP_FAILED) {
		return -1;
	}
	return ftruncate(cache->log_fd, getpagesize());
}

/**
 * OS-specific function that handles adding a log line to the cache.
 *
 * @param client: Client connection;
 * @param log: Log line to add to the cache.
 *
 * @return: 0 in case of success, or -1 otherwise.
 *
 * TODO: Implement proper handling logic. Must be able to dynamically resize the
 * cache if it is full.
 */
int
lmc_add_log_os(struct lmc_client *client, struct lmc_client_logline *log)
{
	pthread_mutex_lock(&write_lock);
	if(client->cache->dim + LMC_TIME_SIZE + strlen(log->logline) + 1 > client->cache->pages * getpagesize()) {
		int old_size = client->cache->pages * getpagesize();
		client->cache->pages *= 2;
		int new_size = client->cache->pages * getpagesize();
		client->cache->ptr = mremap(client->cache->ptr, old_size, new_size, MREMAP_MAYMOVE);
		if(client->cache->ptr == MAP_FAILED) {
			return -1;
		}
		if(ftruncate(client->cache->log_fd, new_size) == -1) {
			return -1;
		}
	}
	client->cache->log_line_num++;
	memcpy(client->cache->ptr + client->cache->dim, log->time, LMC_TIME_SIZE);
	client->cache->dim += LMC_TIME_SIZE;
	memcpy(client->cache->ptr + client->cache->dim, log->logline, strlen(log->logline));
	client->cache->dim += strlen(log->logline);
	memcpy(client->cache->ptr + client->cache->dim, "\n", 1);
	client->cache->dim++;
	pthread_mutex_unlock(&write_lock);
	return 0;
}

/**
 * OS-specific function that handles flushing the cache to disk,
 *
 * @param client: Client connection.
 *
 * @return: 0 in case of success, or -1 otherwise.
 *
 * TODO: Implement proper handling logic.
 */
int
lmc_flush_os(struct lmc_client *client)
{
	if(msync(client->cache->ptr, client->cache->dim, MS_SYNC) == -1) {
		return -1;
	}
	return ftruncate(client->cache->log_fd, client->cache->dim);
}

/**
 * OS-specific function that handles client unsubscribe requests.
 *
 * @param client: Client connection.
 *
 * @return: 0 in case of success, or -1 otherwise.
 *
 * TODO: Implement proper handling logic. Must flush the cache to disk and
 * deallocate any structures associated with the client.
 */
int
lmc_unsubscribe_os(struct lmc_client *client)
{
	if(lmc_flush_os(client) == -1) {
		return -1;
	}
	if(close(client->cache->log_fd) == -1) {
		return -1;
	}
	free(client->cache->service_name);
	if(munmap(client->cache->ptr, client->cache->pages * getpagesize()) == -1) {
		return -1;
	}
	free(client->cache);
	return 0;
}

int truncate_file_os(struct lmc_client *client) {
	return ftruncate(client->cache->log_fd, client->cache->pages * getpagesize());
}
