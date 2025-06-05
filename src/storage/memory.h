#ifndef MEMORY_STORAGE_H
#define MEMORY_STORAGE_H

#include <stdbool.h>
#include "../network/vxlan.h"

// Initialize storage system
bool storage_init(void);

// Clean up storage resources
void storage_cleanup(void);

// Network storage functions
bool storage_save_network(vxlan_network_t* network);
vxlan_network_t* storage_get_network(const char* network_id);
bool storage_delete_network(const char* network_id);
vxlan_network_t** storage_list_networks(const char* tenant_id, int* count);

// Endpoint storage functions
bool storage_save_endpoint(vxlan_endpoint_t* endpoint);
vxlan_endpoint_t* storage_get_endpoint(const char* network_id, const char* endpoint_id);
bool storage_delete_endpoint(const char* network_id, const char* endpoint_id);
vxlan_endpoint_t** storage_list_endpoints(const char* network_id, int* count);

// Helper functions
void storage_free_network_array(vxlan_network_t** networks, int count);
void storage_free_endpoint_array(vxlan_endpoint_t** endpoints, int count);

#endif // MEMORY_STORAGE_H 