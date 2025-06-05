#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <microhttpd.h>
#include "handlers.h"
#include "../network/vxlan.h"
#include "../storage/memory.h"
#include "../utils/logging.h"

// Initialize API handlers
bool api_init(void) {
    if (!storage_init()) {
        LOG_ERROR_FMT("Failed to initialize storage");
        return false;
    }
    return true;
}

// Clean up API resources
void api_cleanup(void) {
    storage_cleanup();
}

// Helper function to send JSON response
static int send_json_response(struct MHD_Connection* connection, int status_code, const char* json) {
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(json),
        (void*)json,
        MHD_RESPMEM_MUST_COPY
    );

    if (!response) {
        LOG_ERROR_FMT("Failed to create response");
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// Generate error response
char* generate_error_response(const char* code, const char* message) {
    struct json_object* error = json_object_new_object();
    json_object_object_add(error, "code", json_object_new_string(code));
    json_object_object_add(error, "message", json_object_new_string(message));

    const char* json = json_object_to_json_string(error);
    char* response = strdup(json);
    json_object_put(error);
    return response;
}

// Generate success response
char* generate_success_response(const char* data) {
    return strdup(data);
}

// Helper to format time_t to ISO 8601 string
static void format_time_iso8601(const char* iso_str, char* buf, size_t buflen) {
    if (iso_str && buf && buflen > 0) {
        strncpy(buf, iso_str, buflen - 1);
        buf[buflen - 1] = '\0';
    } else if (buf && buflen > 0) {
        buf[0] = '\0';
    }
}

// Handle network creation
int handle_create_network(struct MHD_Connection* connection, const char* upload_data) {
    struct json_object* json = json_tokener_parse(upload_data);
    if (!json) {
        char* error = generate_error_response("INVALID_JSON", "Invalid JSON payload");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        return ret;
    }
    struct json_object* tenant_id, *name, *vni, *description;
    if (!json_object_object_get_ex(json, "tenant_id", &tenant_id) ||
        !json_object_object_get_ex(json, "name", &name) ||
        !json_object_object_get_ex(json, "vni", &vni)) {
        char* error = generate_error_response("INVALID_PARAMS", "Missing required parameters");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        json_object_put(json);
        return ret;
    }
    vxlan_network_t* network = vxlan_create_network(
        json_object_get_string(tenant_id),
        json_object_get_string(name),
        json_object_get_int(vni),
        json_object_object_get_ex(json, "description", &description) ? json_object_get_string(description) : NULL
    );
    if (!network) {
        char* error = generate_error_response("CREATE_FAILED", "Failed to create network");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        json_object_put(json);
        return ret;
    }
    if (!storage_save_network(network)) {
        char* error = generate_error_response("SAVE_FAILED", "Failed to save network");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        vxlan_free_network(network);
        json_object_put(json);
        return ret;
    }
    struct json_object* response = json_object_new_object();
    json_object_object_add(response, "id", json_object_new_string(network->id));
    json_object_object_add(response, "tenant_id", json_object_new_string(network->tenant_id));
    json_object_object_add(response, "name", json_object_new_string(network->name));
    json_object_object_add(response, "vni", json_object_new_int64(network->vni));
    if (network->description) {
        json_object_object_add(response, "description", json_object_new_string(network->description));
    }
    char created_at_str[32], updated_at_str[32];
    format_time_iso8601(network->created_at, created_at_str, sizeof(created_at_str));
    format_time_iso8601(network->updated_at, updated_at_str, sizeof(updated_at_str));
    json_object_object_add(response, "created_at", json_object_new_string(created_at_str));
    json_object_object_add(response, "updated_at", json_object_new_string(updated_at_str));
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_CREATED, response_json);
    json_object_put(response);
    json_object_put(json);
    return ret;
}

// Handle network retrieval
int handle_get_network(struct MHD_Connection* connection, const char* network_id) {
    vxlan_network_t* network = storage_get_network(network_id);
    if (!network) {
        char* error = generate_error_response("NOT_FOUND", "Network not found");
        int ret = send_json_response(connection, MHD_HTTP_NOT_FOUND, error);
        free(error);
        return ret;
    }
    struct json_object* response = json_object_new_object();
    json_object_object_add(response, "id", json_object_new_string(network->id));
    json_object_object_add(response, "tenant_id", json_object_new_string(network->tenant_id));
    json_object_object_add(response, "name", json_object_new_string(network->name));
    json_object_object_add(response, "vni", json_object_new_int64(network->vni));
    if (network->description) {
        json_object_object_add(response, "description", json_object_new_string(network->description));
    }
    char created_at_str[32], updated_at_str[32];
    format_time_iso8601(network->created_at, created_at_str, sizeof(created_at_str));
    format_time_iso8601(network->updated_at, updated_at_str, sizeof(updated_at_str));
    json_object_object_add(response, "created_at", json_object_new_string(created_at_str));
    json_object_object_add(response, "updated_at", json_object_new_string(updated_at_str));
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_OK, response_json);
    json_object_put(response);
    return ret;
}

// Handle network deletion
int handle_delete_network(struct MHD_Connection* connection, const char* network_id) {
    if (!storage_delete_network(network_id)) {
        char* error = generate_error_response("NOT_FOUND", "Network not found");
        int ret = send_json_response(connection, MHD_HTTP_NOT_FOUND, error);
        free(error);
        return ret;
    }
    return send_json_response(connection, MHD_HTTP_NO_CONTENT, "{}");
}

// Handle network listing
int handle_list_networks(struct MHD_Connection* connection) {
    int count;
    vxlan_network_t** networks = storage_list_networks(NULL, &count);
    if (!networks) {
        char* error = generate_error_response("LIST_FAILED", "Failed to list networks");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        return ret;
    }
    struct json_object* response = json_object_new_array();
    for (int i = 0; i < count; i++) {
        struct json_object* network = json_object_new_object();
        json_object_object_add(network, "id", json_object_new_string(networks[i]->id));
        json_object_object_add(network, "tenant_id", json_object_new_string(networks[i]->tenant_id));
        json_object_object_add(network, "name", json_object_new_string(networks[i]->name));
        json_object_object_add(network, "vni", json_object_new_int64(networks[i]->vni));
        if (networks[i]->description) {
            json_object_object_add(network, "description", json_object_new_string(networks[i]->description));
        }
        char created_at_str[32], updated_at_str[32];
        format_time_iso8601(networks[i]->created_at, created_at_str, sizeof(created_at_str));
        format_time_iso8601(networks[i]->updated_at, updated_at_str, sizeof(updated_at_str));
        json_object_object_add(network, "created_at", json_object_new_string(created_at_str));
        json_object_object_add(network, "updated_at", json_object_new_string(updated_at_str));
        json_object_array_add(response, network);
    }
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_OK, response_json);
    json_object_put(response);
    free(networks);
    return ret;
}

// Handle endpoint creation
int handle_create_endpoint(struct MHD_Connection* connection, const char* url, const char* upload_data) {
    // Extract network_id from URL
    const char* network_id = strrchr(url, '/');
    if (!network_id) {
        char* error = generate_error_response("INVALID_URL", "Invalid network ID");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        return ret;
    }
    network_id++; // skip '/'
    struct json_object* json = json_tokener_parse(upload_data);
    if (!json) {
        char* error = generate_error_response("INVALID_JSON", "Invalid JSON payload");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        return ret;
    }
    struct json_object* mac_address, *ip_address, *host_id, *vtep_ip;
    if (!json_object_object_get_ex(json, "mac_address", &mac_address) ||
        !json_object_object_get_ex(json, "ip_address", &ip_address) ||
        !json_object_object_get_ex(json, "host_id", &host_id) ||
        !json_object_object_get_ex(json, "vtep_ip", &vtep_ip)) {
        char* error = generate_error_response("INVALID_PARAMS", "Missing required parameters");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        json_object_put(json);
        return ret;
    }
    vxlan_endpoint_t* endpoint = vxlan_create_endpoint(
        network_id,
        json_object_get_string(mac_address),
        json_object_get_string(ip_address),
        json_object_get_string(host_id),
        json_object_get_string(vtep_ip)
    );
    if (!endpoint) {
        char* error = generate_error_response("CREATE_FAILED", "Failed to create endpoint");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        json_object_put(json);
        return ret;
    }
    if (!storage_save_endpoint(endpoint)) {
        char* error = generate_error_response("SAVE_FAILED", "Failed to save endpoint");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        vxlan_free_endpoint(endpoint);
        json_object_put(json);
        return ret;
    }
    struct json_object* response = json_object_new_object();
    json_object_object_add(response, "id", json_object_new_string(endpoint->id));
    json_object_object_add(response, "network_id", json_object_new_string(endpoint->network_id));
    json_object_object_add(response, "mac_address", json_object_new_string(endpoint->mac_address));
    json_object_object_add(response, "ip_address", json_object_new_string(endpoint->ip_address));
    json_object_object_add(response, "host_id", json_object_new_string(endpoint->host_id));
    json_object_object_add(response, "vtep_ip", json_object_new_string(endpoint->vtep_ip));
    char created_at_str[32], updated_at_str[32];
    format_time_iso8601(endpoint->created_at, created_at_str, sizeof(created_at_str));
    format_time_iso8601(endpoint->updated_at, updated_at_str, sizeof(updated_at_str));
    json_object_object_add(response, "created_at", json_object_new_string(created_at_str));
    json_object_object_add(response, "updated_at", json_object_new_string(updated_at_str));
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_CREATED, response_json);
    json_object_put(response);
    json_object_put(json);
    return ret;
}

// Handle endpoint retrieval
int handle_get_endpoint(struct MHD_Connection* connection, const char* endpoint_id) {
    // Extract network_id from the endpoint_id or URL if needed (assuming endpoint_id is actually a URL)
    const char* network_id = NULL;
    const char* slash = strrchr(endpoint_id, '/');
    if (slash) {
        network_id = endpoint_id;
        endpoint_id = slash + 1;
        // Temporarily null-terminate network_id
        size_t len = slash - network_id;
        char net_id_buf[256];
        if (len < sizeof(net_id_buf)) {
            strncpy(net_id_buf, network_id, len);
            net_id_buf[len] = '\0';
            network_id = net_id_buf;
        } else {
            network_id = NULL;
        }
    }
    vxlan_endpoint_t* endpoint = storage_get_endpoint(network_id, endpoint_id);
    if (!endpoint) {
        char* error = generate_error_response("NOT_FOUND", "Endpoint not found");
        int ret = send_json_response(connection, MHD_HTTP_NOT_FOUND, error);
        free(error);
        return ret;
    }
    struct json_object* response = json_object_new_object();
    json_object_object_add(response, "id", json_object_new_string(endpoint->id));
    json_object_object_add(response, "network_id", json_object_new_string(endpoint->network_id));
    json_object_object_add(response, "mac_address", json_object_new_string(endpoint->mac_address));
    json_object_object_add(response, "ip_address", json_object_new_string(endpoint->ip_address));
    json_object_object_add(response, "host_id", json_object_new_string(endpoint->host_id));
    json_object_object_add(response, "vtep_ip", json_object_new_string(endpoint->vtep_ip));
    char created_at_str[32], updated_at_str[32];
    format_time_iso8601(endpoint->created_at, created_at_str, sizeof(created_at_str));
    format_time_iso8601(endpoint->updated_at, updated_at_str, sizeof(updated_at_str));
    json_object_object_add(response, "created_at", json_object_new_string(created_at_str));
    json_object_object_add(response, "updated_at", json_object_new_string(updated_at_str));
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_OK, response_json);
    json_object_put(response);
    return ret;
}

// Handle endpoint deletion
int handle_delete_endpoint(struct MHD_Connection* connection, const char* url) {
    // Extract endpoint_id from URL
    const char* endpoint_id = strrchr(url, '/');
    if (!endpoint_id) {
        char* error = generate_error_response("INVALID_URL", "Invalid endpoint ID");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        return ret;
    }
    endpoint_id++; // skip '/'
    // Extract network_id from URL (everything before last slash)
    size_t net_id_len = endpoint_id - url - 1;
    char network_id[256] = {0};
    if (net_id_len < sizeof(network_id)) {
        strncpy(network_id, url, net_id_len);
        network_id[net_id_len] = '\0';
    }
    if (!storage_delete_endpoint(network_id, endpoint_id)) {
        char* error = generate_error_response("NOT_FOUND", "Endpoint not found");
        int ret = send_json_response(connection, MHD_HTTP_NOT_FOUND, error);
        free(error);
        return ret;
    }
    return send_json_response(connection, MHD_HTTP_NO_CONTENT, "{}");
}

// Handle endpoint listing
int handle_list_endpoints(struct MHD_Connection* connection, const char* url) {
    // Extract network_id from URL
    const char* network_id = strrchr(url, '/');
    if (!network_id) {
        char* error = generate_error_response("INVALID_URL", "Invalid network ID");
        int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, error);
        free(error);
        return ret;
    }
    network_id++; // skip '/'
    int count;
    vxlan_endpoint_t** endpoints = storage_list_endpoints(network_id, &count);
    if (!endpoints) {
        char* error = generate_error_response("LIST_FAILED", "Failed to list endpoints");
        int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, error);
        free(error);
        return ret;
    }
    struct json_object* response = json_object_new_array();
    for (int i = 0; i < count; i++) {
        struct json_object* endpoint = json_object_new_object();
        json_object_object_add(endpoint, "id", json_object_new_string(endpoints[i]->id));
        json_object_object_add(endpoint, "network_id", json_object_new_string(endpoints[i]->network_id));
        json_object_object_add(endpoint, "mac_address", json_object_new_string(endpoints[i]->mac_address));
        json_object_object_add(endpoint, "ip_address", json_object_new_string(endpoints[i]->ip_address));
        json_object_object_add(endpoint, "host_id", json_object_new_string(endpoints[i]->host_id));
        json_object_object_add(endpoint, "vtep_ip", json_object_new_string(endpoints[i]->vtep_ip));
        char created_at_str[32], updated_at_str[32];
        format_time_iso8601(endpoints[i]->created_at, created_at_str, sizeof(created_at_str));
        format_time_iso8601(endpoints[i]->updated_at, updated_at_str, sizeof(updated_at_str));
        json_object_object_add(endpoint, "created_at", json_object_new_string(created_at_str));
        json_object_object_add(endpoint, "updated_at", json_object_new_string(updated_at_str));
        json_object_array_add(response, endpoint);
    }
    const char* response_json = json_object_to_json_string(response);
    int ret = send_json_response(connection, MHD_HTTP_OK, response_json);
    json_object_put(response);
    free(endpoints);
    return ret;
} 