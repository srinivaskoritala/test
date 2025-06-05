#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <microhttpd.h>

#define PORT 18080

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
    const char* response = "Hello, World!";
    struct MHD_Response* resp = MHD_create_response_from_buffer(strlen(response),
                                                              (void*)response,
                                                              MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(resp, "Content-Type", "text/plain");
    MHD_queue_response(connection, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return MHD_YES;
}

int main(void) {
    struct MHD_Daemon* daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                                                 PORT,
                                                 NULL,
                                                 NULL,
                                                 &request_handler,
                                                 NULL,
                                                 MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start HTTP daemon: errno=%d (%s)\n", errno, strerror(errno));
        return 1;
    }
    printf("Server started on port %d\n", PORT);
    getchar(); // Wait for user input to stop the server
    MHD_stop_daemon(daemon);
    return 0;
} 