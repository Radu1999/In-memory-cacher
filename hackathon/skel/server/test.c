#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	LMC_LINE_SIZE		256
#define	LMC_COMMAND_SIZE	1024
#define	LMC_SERVER_IP		"127.0.0.1"
#define	LMC_SERVER_PORT		38379
#define	LMC_CLIENT_MAX_NAME	16
#define	LMC_STATUS_MAX_SIZE	512
#define	LMC_TIME_FORMAT		"%Y/%m/%d-%H:%M:%S"
#define	LMC_FTIME_FORMAT	"%Y.%m.%d-%H.%M.%S"
#define	LMC_TIME_SIZE		20  /* strlen("YYYY/mm/dd-HH:MM:SS") + 1 */
#define	LMC_LOGLINE_SIZE	(LMC_LINE_SIZE - LMC_TIME_SIZE)
#define	LMC_STATS_FORMAT	"Status at %s\nMemory: %ldKB\nLoglines: %lu\n"

#include "../include/lmc.h"

static int
print_stats(struct lmc_conn *conn)
{
	char *stats;

	stats = lmc_get_stats(conn);
	if (stats == NULL) {
		printf("Failed to get stats\n");
		return -1;
	}

	printf("%s\n", stats);

	lmc_free_buf(stats);

	return 0;
}

static uint64_t
count_log_lines(struct lmc_conn *conn)
{
	uint64_t count;
	lmc_get_logs(conn, 0, 0, &count);
	return count;
}


int main() {
	
	struct lmc_conn *conn;
	uint64_t count, count_0, count_1;
	int ret;

	conn = lmc_connect("aaaab");
	if (!conn) {
		printf("Client not connected\n");
		return 0;
	}

	count = 20;
	count_0 = count_log_lines(conn);
	for(int i = 0; i < count; i++) {
		lmc_send_log(conn, "BROSKY");
	}

	count_1 = count_log_lines(conn);

	if (count_1 - count_0 != count) {
		ret = -1;
		printf("Incorrect number of new logs\n");
	}

	ret = print_stats(conn);

	ret = lmc_disconnect(conn);
	if (ret != 0)
		printf("Disconnect error\n");

	lmc_free(conn);

	return 0;
}
