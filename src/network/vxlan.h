#ifndef VXLAN_H
#define VXLAN_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

// Constants
#define MAX_VNI 16777215  // 2^24 - 1

// VXLAN network structure
typedef struct {
    char* id;
    char* tenant_id;
    char* name;
    uint32_t vni;
    char* description;
    char* created_at;
    char* updated_at;
} vxlan_network_t;

// VXLAN endpoint structure
typedef struct {
    char* id;
    char* network_id;
    char* mac_address;
    char* ip_address;
    char* host_id;
    char* vtep_ip;
    char* created_at;
    char* updated_at;
} vxlan_endpoint_t;

// Network management functions
vxlan_network_t* vxlan_create_network(const char* tenant_id, const char* name, uint32_t vni, const char* description);
void vxlan_free_network(vxlan_network_t* network);
char* vxlan_generate_network_cmd(const vxlan_network_t* network);
char* vxlan_generate_delete_network_cmd(const char* network_id);

// Endpoint management functions
vxlan_endpoint_t* vxlan_create_endpoint(const char* network_id, const char* mac_address,
                                      const char* ip_address, const char* host_id,
                                      const char* vtep_ip);
void vxlan_free_endpoint(vxlan_endpoint_t* endpoint);
char* vxlan_generate_endpoint_cmd(const vxlan_endpoint_t* endpoint);
char* vxlan_generate_delete_endpoint_cmd(const char* network_id, const char* endpoint_id);

#endif // VXLAN_H 