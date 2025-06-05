#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>
#include "vxlan.h"
#include "../utils/logging.h"

// Generate a UUID string
static char* generate_uuid(void) {
    uuid_t uuid;
    char* uuid_str = malloc(37); // 36 chars + null terminator
    if (!uuid_str) {
        LOG_ERROR_FMT("Failed to allocate memory for UUID");
        return NULL;
    }
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
}

// Get current timestamp in ISO 8601 format
static char* get_timestamp(void) {
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    char* timestamp = malloc(21); // YYYY-MM-DDTHH:MM:SSZ + null terminator
    if (!timestamp) {
        LOG_ERROR_FMT("Failed to allocate memory for timestamp");
        return NULL;
    }
    strftime(timestamp, 21, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return timestamp;
}

// Create a new VXLAN network
vxlan_network_t* vxlan_create_network(const char* tenant_id, const char* name, uint32_t vni, const char* description) {
    if (!tenant_id || !name || vni == 0 || vni > MAX_VNI) {
        LOG_ERROR_FMT("Invalid network parameters");
        return NULL;
    }

    vxlan_network_t* network = malloc(sizeof(vxlan_network_t));
    if (!network) {
        LOG_ERROR_FMT("Failed to allocate memory for network");
        return NULL;
    }

    // Generate UUID
    network->id = generate_uuid();
    if (!network->id) {
        free(network);
        return NULL;
    }

    // Copy tenant ID and name
    network->tenant_id = strdup(tenant_id);
    network->name = strdup(name);
    if (!network->tenant_id || !network->name) {
        LOG_ERROR_FMT("Failed to copy network parameters");
        vxlan_free_network(network);
        return NULL;
    }

    // Set VNI and description
    network->vni = vni;
    network->description = description ? strdup(description) : NULL;

    // Set timestamps
    network->created_at = get_timestamp();
    network->updated_at = strdup(network->created_at);
    if (!network->created_at || !network->updated_at) {
        LOG_ERROR_FMT("Failed to set timestamps");
        vxlan_free_network(network);
        return NULL;
    }

    LOG_INFO_FMT("Created network %s (VNI: %u) for tenant %s", network->id, vni, tenant_id);
    return network;
}

// Free network resources
void vxlan_free_network(vxlan_network_t* network) {
    if (!network) return;

    free(network->id);
    free(network->tenant_id);
    free(network->name);
    free(network->description);
    free(network->created_at);
    free(network->updated_at);
    free(network);
}

// Create a new VXLAN endpoint
vxlan_endpoint_t* vxlan_create_endpoint(const char* network_id, const char* mac_address,
                                       const char* ip_address, const char* host_id, const char* vtep_ip) {
    if (!network_id || !mac_address || !ip_address || !host_id || !vtep_ip) {
        LOG_ERROR_FMT("Invalid endpoint parameters");
        return NULL;
    }

    vxlan_endpoint_t* endpoint = malloc(sizeof(vxlan_endpoint_t));
    if (!endpoint) {
        LOG_ERROR_FMT("Failed to allocate memory for endpoint");
        return NULL;
    }

    // Generate UUID
    endpoint->id = generate_uuid();
    if (!endpoint->id) {
        free(endpoint);
        return NULL;
    }

    // Copy parameters
    endpoint->network_id = strdup(network_id);
    endpoint->mac_address = strdup(mac_address);
    endpoint->ip_address = strdup(ip_address);
    endpoint->host_id = strdup(host_id);
    endpoint->vtep_ip = strdup(vtep_ip);

    if (!endpoint->network_id || !endpoint->mac_address || !endpoint->ip_address ||
        !endpoint->host_id || !endpoint->vtep_ip) {
        LOG_ERROR_FMT("Failed to copy endpoint parameters");
        vxlan_free_endpoint(endpoint);
        return NULL;
    }

    // Set timestamps
    endpoint->created_at = get_timestamp();
    endpoint->updated_at = strdup(endpoint->created_at);
    if (!endpoint->created_at || !endpoint->updated_at) {
        LOG_ERROR_FMT("Failed to set timestamps");
        vxlan_free_endpoint(endpoint);
        return NULL;
    }

    LOG_INFO_FMT("Created endpoint %s for network %s", endpoint->id, network_id);
    return endpoint;
}

// Free endpoint resources
void vxlan_free_endpoint(vxlan_endpoint_t* endpoint) {
    if (!endpoint) return;

    free(endpoint->id);
    free(endpoint->network_id);
    free(endpoint->mac_address);
    free(endpoint->ip_address);
    free(endpoint->host_id);
    free(endpoint->vtep_ip);
    free(endpoint->created_at);
    free(endpoint->updated_at);
    free(endpoint);
}

// Generate iproute2 command for creating a VXLAN network
char* vxlan_generate_network_cmd(const vxlan_network_t* network) {
    if (!network) return NULL;

    // Format: ip link add vxlan<vni> type vxlan id <vni> dstport 4789 dev <interface>
    char* cmd = malloc(256);
    if (!cmd) {
        LOG_ERROR_FMT("Failed to allocate memory for network command");
        return NULL;
    }

    snprintf(cmd, 256, "ip link add vxlan%d type vxlan id %u dstport 4789 dev eth0", 
             network->vni, network->vni);

    LOG_DEBUG_FMT("Generated network command: %s", cmd);
    return cmd;
}

// Generate iproute2 command for adding an endpoint to a VXLAN network
char* vxlan_generate_endpoint_cmd(const vxlan_endpoint_t* endpoint) {
    if (!endpoint) return NULL;

    // Format: bridge fdb append to 00:00:00:00:00:00 dst <vtep_ip> dev vxlan<vni>
    char* cmd = malloc(256);
    if (!cmd) {
        LOG_ERROR_FMT("Failed to allocate memory for endpoint command");
        return NULL;
    }

    // Extract VNI from network_id (in a real implementation, we would look this up)
    uint32_t vni = 1; // Placeholder - should be looked up from network_id

    snprintf(cmd, 256, "bridge fdb append to 00:00:00:00:00:00 dst %s dev vxlan%d", 
             endpoint->vtep_ip, vni);

    LOG_DEBUG_FMT("Generated endpoint command: %s", cmd);
    return cmd;
}

// Generate iproute2 command for deleting a VXLAN network
char* vxlan_generate_delete_network_cmd(const char* network_id) {
    if (!network_id) return NULL;

    // Format: ip link delete vxlan<vni>
    char* cmd = malloc(256);
    if (!cmd) {
        LOG_ERROR_FMT("Failed to allocate memory for delete network command");
        return NULL;
    }

    // Extract VNI from network_id (in a real implementation, we would look this up)
    uint32_t vni = 1; // Placeholder - should be looked up from network_id

    snprintf(cmd, 256, "ip link delete vxlan%d", vni);

    LOG_DEBUG_FMT("Generated delete network command: %s", cmd);
    return cmd;
}

// Generate iproute2 command for removing an endpoint from a VXLAN network
char* vxlan_generate_delete_endpoint_cmd(const char* network_id, const char* endpoint_id) {
    if (!network_id || !endpoint_id) return NULL;

    // Format: bridge fdb del <mac> dst <vtep_ip> dev vxlan<vni>
    char* cmd = malloc(256);
    if (!cmd) {
        LOG_ERROR_FMT("Failed to allocate memory for delete endpoint command");
        return NULL;
    }

    // Extract VNI from network_id (in a real implementation, we would look this up)
    uint32_t vni = 1; // Placeholder - should be looked up from network_id

    snprintf(cmd, 256, "bridge fdb del 00:00:00:00:00:00 dst 0.0.0.0 dev vxlan%d", vni);

    LOG_DEBUG_FMT("Generated delete endpoint command: %s", cmd);
    return cmd;
} 