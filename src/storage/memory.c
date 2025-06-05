#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "memory.h"
#include "../utils/logging.h"

// Hash table size (prime number for better distribution)
#define HASH_SIZE 10007

// Hash table entry
typedef struct hash_entry {
    char* key;
    void* value;
    struct hash_entry* next;
} hash_entry_t;

// Hash table
typedef struct {
    hash_entry_t* entries[HASH_SIZE];
    pthread_mutex_t mutex;
} hash_table_t;

// Global hash tables
static hash_table_t networks_table;
static hash_table_t endpoints_table;

// Hash function (djb2)
static unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_SIZE;
}

// Initialize storage system
bool storage_init(void) {
    // Initialize hash tables
    memset(&networks_table, 0, sizeof(hash_table_t));
    memset(&endpoints_table, 0, sizeof(hash_table_t));

    // Initialize mutexes
    if (pthread_mutex_init(&networks_table.mutex, NULL) != 0 ||
        pthread_mutex_init(&endpoints_table.mutex, NULL) != 0) {
        LOG_ERROR_FMT("Failed to initialize mutexes");
        return false;
    }

    LOG_INFO_FMT("Storage system initialized");
    return true;
}

// Clean up storage resources
void storage_cleanup(void) {
    // Free network entries
    pthread_mutex_lock(&networks_table.mutex);
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_entry_t* entry = networks_table.entries[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            vxlan_network_t* network = (vxlan_network_t*)entry->value;
            vxlan_free_network(network);
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    pthread_mutex_unlock(&networks_table.mutex);

    // Free endpoint entries
    pthread_mutex_lock(&endpoints_table.mutex);
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_entry_t* entry = endpoints_table.entries[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            vxlan_endpoint_t* endpoint = (vxlan_endpoint_t*)entry->value;
            vxlan_free_endpoint(endpoint);
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    pthread_mutex_unlock(&endpoints_table.mutex);

    // Destroy mutexes
    pthread_mutex_destroy(&networks_table.mutex);
    pthread_mutex_destroy(&endpoints_table.mutex);

    LOG_INFO_FMT("Storage system cleaned up");
}

// Save network to storage
bool storage_save_network(vxlan_network_t* network) {
    if (!network || !network->id) return false;

    unsigned int h = hash(network->id);
    hash_entry_t* entry = malloc(sizeof(hash_entry_t));
    if (!entry) {
        LOG_ERROR_FMT("Failed to allocate memory for network entry");
        return false;
    }

    entry->key = strdup(network->id);
    entry->value = network;
    entry->next = NULL;

    pthread_mutex_lock(&networks_table.mutex);
    entry->next = networks_table.entries[h];
    networks_table.entries[h] = entry;
    pthread_mutex_unlock(&networks_table.mutex);

    LOG_DEBUG_FMT("Saved network %s", network->id);
    return true;
}

// Get network from storage
vxlan_network_t* storage_get_network(const char* network_id) {
    if (!network_id) return NULL;

    unsigned int h = hash(network_id);
    vxlan_network_t* network = NULL;

    pthread_mutex_lock(&networks_table.mutex);
    hash_entry_t* entry = networks_table.entries[h];
    while (entry) {
        if (strcmp(entry->key, network_id) == 0) {
            network = (vxlan_network_t*)entry->value;
            break;
        }
        entry = entry->next;
    }
    pthread_mutex_unlock(&networks_table.mutex);

    return network;
}

// Delete network from storage
bool storage_delete_network(const char* network_id) {
    if (!network_id) return false;

    unsigned int h = hash(network_id);
    bool found = false;

    pthread_mutex_lock(&networks_table.mutex);
    hash_entry_t* entry = networks_table.entries[h];
    hash_entry_t* prev = NULL;

    while (entry) {
        if (strcmp(entry->key, network_id) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                networks_table.entries[h] = entry->next;
            }
            vxlan_network_t* network = (vxlan_network_t*)entry->value;
            vxlan_free_network(network);
            free(entry->key);
            free(entry);
            found = true;
            break;
        }
        prev = entry;
        entry = entry->next;
    }
    pthread_mutex_unlock(&networks_table.mutex);

    if (found) {
        LOG_DEBUG_FMT("Deleted network %s", network_id);
    }
    return found;
}

// List networks
vxlan_network_t** storage_list_networks(const char* tenant_id, int* count) {
    *count = 0;
    vxlan_network_t** networks = NULL;
    int capacity = 0;

    pthread_mutex_lock(&networks_table.mutex);
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_entry_t* entry = networks_table.entries[i];
        while (entry) {
            vxlan_network_t* network = (vxlan_network_t*)entry->value;
            if (!tenant_id || strcmp(network->tenant_id, tenant_id) == 0) {
                if (*count >= capacity) {
                    capacity = capacity == 0 ? 16 : capacity * 2;
                    networks = realloc(networks, capacity * sizeof(vxlan_network_t*));
                    if (!networks) {
                        pthread_mutex_unlock(&networks_table.mutex);
                        return NULL;
                    }
                }
                networks[(*count)++] = network;
            }
            entry = entry->next;
        }
    }
    pthread_mutex_unlock(&networks_table.mutex);

    return networks;
}

// Save endpoint to storage
bool storage_save_endpoint(vxlan_endpoint_t* endpoint) {
    if (!endpoint || !endpoint->id) return false;

    unsigned int h = hash(endpoint->id);
    hash_entry_t* entry = malloc(sizeof(hash_entry_t));
    if (!entry) {
        LOG_ERROR_FMT("Failed to allocate memory for endpoint entry");
        return false;
    }

    entry->key = strdup(endpoint->id);
    entry->value = endpoint;
    entry->next = NULL;

    pthread_mutex_lock(&endpoints_table.mutex);
    entry->next = endpoints_table.entries[h];
    endpoints_table.entries[h] = entry;
    pthread_mutex_unlock(&endpoints_table.mutex);

    LOG_DEBUG_FMT("Saved endpoint %s", endpoint->id);
    return true;
}

// Get endpoint from storage
vxlan_endpoint_t* storage_get_endpoint(const char* network_id, const char* endpoint_id) {
    if (!endpoint_id) return NULL;

    unsigned int h = hash(endpoint_id);
    vxlan_endpoint_t* endpoint = NULL;

    pthread_mutex_lock(&endpoints_table.mutex);
    hash_entry_t* entry = endpoints_table.entries[h];
    while (entry) {
        endpoint = (vxlan_endpoint_t*)entry->value;
        if (strcmp(entry->key, endpoint_id) == 0 &&
            (!network_id || strcmp(endpoint->network_id, network_id) == 0)) {
            break;
        }
        endpoint = NULL;
        entry = entry->next;
    }
    pthread_mutex_unlock(&endpoints_table.mutex);

    return endpoint;
}

// Delete endpoint from storage
bool storage_delete_endpoint(const char* network_id, const char* endpoint_id) {
    if (!endpoint_id) return false;

    unsigned int h = hash(endpoint_id);
    bool found = false;

    pthread_mutex_lock(&endpoints_table.mutex);
    hash_entry_t* entry = endpoints_table.entries[h];
    hash_entry_t* prev = NULL;

    while (entry) {
        vxlan_endpoint_t* endpoint = (vxlan_endpoint_t*)entry->value;
        if (strcmp(entry->key, endpoint_id) == 0 &&
            (!network_id || strcmp(endpoint->network_id, network_id) == 0)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                endpoints_table.entries[h] = entry->next;
            }
            vxlan_free_endpoint(endpoint);
            free(entry->key);
            free(entry);
            found = true;
            break;
        }
        prev = entry;
        entry = entry->next;
    }
    pthread_mutex_unlock(&endpoints_table.mutex);

    if (found) {
        LOG_DEBUG_FMT("Deleted endpoint %s", endpoint_id);
    }
    return found;
}

// List endpoints
vxlan_endpoint_t** storage_list_endpoints(const char* network_id, int* count) {
    if (!network_id) return NULL;

    *count = 0;
    vxlan_endpoint_t** endpoints = NULL;
    int capacity = 0;

    pthread_mutex_lock(&endpoints_table.mutex);
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_entry_t* entry = endpoints_table.entries[i];
        while (entry) {
            vxlan_endpoint_t* endpoint = (vxlan_endpoint_t*)entry->value;
            if (strcmp(endpoint->network_id, network_id) == 0) {
                if (*count >= capacity) {
                    capacity = capacity == 0 ? 16 : capacity * 2;
                    endpoints = realloc(endpoints, capacity * sizeof(vxlan_endpoint_t*));
                    if (!endpoints) {
                        pthread_mutex_unlock(&endpoints_table.mutex);
                        return NULL;
                    }
                }
                endpoints[(*count)++] = endpoint;
            }
            entry = entry->next;
        }
    }
    pthread_mutex_unlock(&endpoints_table.mutex);

    return endpoints;
}

// Free network array
void storage_free_network_array(vxlan_network_t** networks, int count) {
    if (!networks) return;
    free(networks);
}

// Free endpoint array
void storage_free_endpoint_array(vxlan_endpoint_t** endpoints, int count) {
    if (!endpoints) return;
    free(endpoints);
} 