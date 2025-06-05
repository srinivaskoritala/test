#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "../src/network/vxlan.h"
#include "../src/storage/memory.h"
#include "../src/utils/logging.h"

// Callback function for CURL
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((char*)userp)[0] = '\0';
    strncat((char*)userp, (char*)contents, size * nmemb);
    return size * nmemb;
}

// Test network creation
static bool test_create_network(void) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize CURL\n");
        return false;
    }

    char response[4096] = {0};
    const char* json = "{\"tenant_id\":\"tenant1\",\"name\":\"test-network\",\"vni\":1000,\"description\":\"Test network\"}";

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:18080/api/v1/networks");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Failed to create network: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 201) {
        printf("Failed to create network: HTTP %ld\n", http_code);
        return false;
    }

    printf("Created network: %s\n", response);
    return true;
}

// Test endpoint creation
static bool test_create_endpoint(const char* network_id) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize CURL\n");
        return false;
    }

    char response[4096] = {0};
    char url[256];
    snprintf(url, sizeof(url), "http://localhost:18080/api/v1/networks/%s/endpoints", network_id);

    const char* json = "{\"mac_address\":\"00:11:22:33:44:55\",\"ip_address\":\"192.168.1.100\","
                      "\"host_id\":\"host1\",\"vtep_ip\":\"10.0.0.1\"}";

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Failed to create endpoint: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 201) {
        printf("Failed to create endpoint: HTTP %ld\n", http_code);
        return false;
    }

    printf("Created endpoint: %s\n", response);
    return true;
}

// Test network retrieval
static bool test_get_network(const char* network_id) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize CURL\n");
        return false;
    }

    char response[4096] = {0};
    char url[256];
    snprintf(url, sizeof(url), "http://localhost:18080/api/v1/networks/%s", network_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Failed to get network: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        printf("Failed to get network: HTTP %ld\n", http_code);
        return false;
    }

    printf("Got network: %s\n", response);
    return true;
}

// Test endpoint listing
static bool test_list_endpoints(const char* network_id) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize CURL\n");
        return false;
    }

    char response[4096] = {0};
    char url[256];
    snprintf(url, sizeof(url), "http://localhost:18080/api/v1/networks/%s/endpoints", network_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("Failed to list endpoints: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        printf("Failed to list endpoints: HTTP %ld\n", http_code);
        return false;
    }

    printf("Listed endpoints: %s\n", response);
    return true;
}

// Main test function
int main(void) {
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);

    // Run tests
    printf("Running network service tests...\n\n");

    // Test network creation
    printf("Testing network creation...\n");
    if (!test_create_network()) {
        printf("Network creation test failed\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Network creation test passed\n\n");

    // Test endpoint creation
    printf("Testing endpoint creation...\n");
    if (!test_create_endpoint("test-network-id")) {
        printf("Endpoint creation test failed\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Endpoint creation test passed\n\n");

    // Test network retrieval
    printf("Testing network retrieval...\n");
    if (!test_get_network("test-network-id")) {
        printf("Network retrieval test failed\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Network retrieval test passed\n\n");

    // Test endpoint listing
    printf("Testing endpoint listing...\n");
    if (!test_list_endpoints("test-network-id")) {
        printf("Endpoint listing test failed\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Endpoint listing test passed\n\n");

    // Clean up
    curl_global_cleanup();
    printf("All tests passed!\n");
    return 0;
} 