#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/lmc.h"

int main() {
	struct lmc_conn *conn;
	struct lmc_client_logline **lines;
	char *stats;
	uint64_t logs, i;


    while(1) {
        conn = lmc_connect("B");
        lmc_send_log(conn, "this is a log line");
        lmc_send_log(conn, "this is a log line");
        lmc_send_log(conn, "this is a log line");
        lmc_send_log(conn, "this is a log line");
        lmc_disconnect(conn);
        sleep(1);
    }
	

	// stats = lmc_get_stats(conn);
	// printf("%s\n", stats);
	// lines = lmc_get_logs(conn, 0, 0, &logs);

	// for (i = 0; i < logs; i++) {
	// 	printf("%s: %s\n", lines[i]->time, lines[i]->logline);
	// }
    

	return 0;
}
