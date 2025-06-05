#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <microhttpd.h>
#include "api/handlers.h"
#include "utils/logging.h"
#include <errno.h>

#define PORT 18080
#define MAX_CONNECTIONS 1024

static struct MHD_Daemon* mhd_daemon = NULL;

// Signal handler for graceful shutdown
static void signal_handler(int signum) {
    if (mhd_daemon) {
        MHD_stop_daemon(mhd_daemon);
        mhd_daemon = NULL;
    }
    api_cleanup();
    logging_cleanup();
    exit(0);
}

// Main request handler
static enum MHD_Result request_handler(void* cls,
                         struct MHD_Connection* connection,
                         const char* url,
                         const char* method,
                         const char* version,
                         const char* upload_data,
                         size_t* upload_data_size,
                         void** ptr) {
    static int dummy;
    if (&dummy != *ptr) {
        *ptr = &dummy;
        return MHD_YES;
    }

    if (0 != *upload_data_size) {
        *upload_data_size = 0;
        return MHD_YES;
    }

    // Parse URL and method to determine handler
    if (strcmp(method, "POST") == 0) {
        if (strncmp(url, "/api/v1/networks", 15) == 0) {
            return handle_create_network(connection, upload_data);
        } else if (strstr(url, "/endpoints") != NULL) {
            return handle_create_endpoint(connection, url, upload_data);
        }
    } else if (strcmp(method, "GET") == 0) {
        if (strncmp(url, "/api/v1/networks", 15) == 0) {
            if (strlen(url) > 16) {
                return handle_get_network(connection, url + 16);
            } else {
                return handle_list_networks(connection);
            }
        } else if (strstr(url, "/endpoints") != NULL) {
            return handle_list_endpoints(connection, url);
        }
    } else if (strcmp(method, "DELETE") == 0) {
        if (strncmp(url, "/api/v1/networks", 15) == 0) {
            if (strstr(url, "/endpoints") != NULL) {
                return handle_delete_endpoint(connection, url);
            } else {
                return handle_delete_network(connection, url + 16);
            }
        }
    }

    // Return 404 for unhandled routes
    const char* response = "{\"error\":\"Not Found\"}";
    struct MHD_Response* resp = MHD_create_response_from_buffer(strlen(response),
                                                              (void*)response,
                                                              MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(resp, "Content-Type", "application/json");
    MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
    MHD_destroy_response(resp);
    return MHD_YES;
}

int main(void) {
    // Initialize logging
    if (!logging_init("network_service.log")) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }
    logging_set_level(LOG_LEVEL_DEBUG);

    // Initialize API
    if (!api_init()) {
        fprintf(stderr, "Failed to initialize API\n");
        logging_cleanup();
        return 1;
    }

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Start HTTP daemon
    mhd_daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                             PORT,
                             NULL,
                             NULL,
                             &request_handler,
                             NULL,
                             MHD_OPTION_THREAD_POOL_SIZE,
                             MAX_CONNECTIONS,
                             MHD_OPTION_END);

    if (mhd_daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP daemon: errno=%d (%s)\n", errno, strerror(errno));
        api_cleanup();
        logging_cleanup();
        return 1;
    }

    printf("Network service started on port %d\n", PORT);

    // Wait for signals
    while (1) {
        sleep(1);
    }

    return 0;
} 