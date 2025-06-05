#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdbool.h>
#include <microhttpd.h>

// Initialize API handlers
bool api_init(void);

// Clean up API resources
void api_cleanup(void);

// Network handlers
int handle_create_network(struct MHD_Connection* connection, const char* upload_data);
int handle_get_network(struct MHD_Connection* connection, const char* network_id);
int handle_delete_network(struct MHD_Connection* connection, const char* network_id);
int handle_list_networks(struct MHD_Connection* connection);

// Endpoint handlers
int handle_create_endpoint(struct MHD_Connection* connection, const char* url, const char* upload_data);
int handle_get_endpoint(struct MHD_Connection* connection, const char* endpoint_id);
int handle_delete_endpoint(struct MHD_Connection* connection, const char* url);
int handle_list_endpoints(struct MHD_Connection* connection, const char* url);

#endif // HANDLERS_H 