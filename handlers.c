// handlers.c
// Implements the specific logic for handling various API requests.
// This file contains the "business logic" of the API.

#include "handlers.h"    // Header for handler function declarations
#include "mongoose.h"    // Mongoose types and functions
#include "cJSON.h"       // For JSON parsing and generation
#include "utils.h"       // For send_json_response, send_error_response
#include <stdio.h>       // For fprintf
#include <stdlib.h>      // For malloc, free, strtol (safer than atoi)
#include <string.h>      // For strlen, strcmp, strcpy, memcpy, strncmp

// --- In-memory "Database" Simulation ---
// WARNING: This is an in-memory store and is NOT persistent.
// Data will be lost when the server restarts.
// For a production-ready application, this MUST be replaced
// with an actual database (e.g., PostgreSQL, MySQL, SQLite, MongoDB).

#define MAX_ITEMS 100 // Maximum number of items our in-memory array can hold

// Structure to represent an item in our "database".
typedef struct {
    int id;
    char name[64]; // Fixed-size buffer for item name
    int value;
} item_t;

static item_t items[MAX_ITEMS]; // Array to store items
static int next_item_id = 1;    // Counter for assigning unique IDs
static int item_count = 0;      // Current number of items in the array

// Static helper function to initialize some dummy data into the in-memory store.
// This ensures that there's some data available when the server starts.
static void init_dummy_data() {
    // Only initialize if the "database" is empty to avoid re-adding on every call.
    if (item_count == 0) {
        // Add "First Item"
        if (item_count < MAX_ITEMS) {
            items[item_count].id = next_item_id++;
            strcpy(items[item_count].name, "First Item");
            items[item_count].value = 100;
            item_count++;
        }

        // Add "Second Item"
        if (item_count < MAX_ITEMS) {
            items[item_count].id = next_item_id++;
            strcpy(items[item_count].name, "Second Item");
            items[item_count].value = 200;
            item_count++;
        }

        fprintf(stdout, "Dummy data initialized with %d items.\n", item_count);
    }
}

// Static helper function to find an item in the in-memory array by its ID.
// Returns a pointer to the item if found, otherwise returns NULL.
static item_t *find_item_by_id(int id) {
    for (int i = 0; i < item_count; ++i) {
        if (items[i].id == id) {
            return &items[i]; // Item found
        }
    }
    return NULL; // Item not found
}

// Static helper function to extract the integer ID from the URI.
// Expected format: /api/v1/items/{id}
// It safely parses the ID, converting it from string to integer.
// Returns the ID on success, or -1 if the ID is not found or is invalid.
static int get_item_id_from_uri(const struct mg_http_message *hm) {
    const char *prefix = "/api/v1/items/";
    size_t prefix_len = strlen(prefix);

    // Check if the URI starts with the expected prefix and is long enough for an ID.
    // mg_vcmp safely compares mg_str (non-null-terminated) with C strings.
    if (hm->uri.len > prefix_len && mg_vcmp(&hm->uri, prefix) == 0) {
        // The ID part starts immediately after the prefix.
        // We need to extract this part and convert it to an integer.
        // Mongoose mg_str are not null-terminated, so we copy the ID part to a temp buffer.
        char id_str_buf[32]; // Sufficient buffer for typical integer IDs (e.g., up to 2 billion)
        size_t id_len = hm->uri.len - prefix_len;

        // Prevent buffer overflow if the ID string is unexpectedly long.
        if (id_len >= sizeof(id_str_buf)) {
            fprintf(stderr, "Error: Item ID string in URI is too long.\n");
            return -1;
        }

        // Copy the ID part from the URI (which is not null-terminated)
        // into our null-terminated buffer.
        memcpy(id_str_buf, hm->uri.p + prefix_len, id_len);
        id_str_buf[id_len] = '\0'; // Null-terminate the string for strtol

        // Use strtol for safer string-to-long conversion with error checking.
        char *endptr;
        long id_long = strtol(id_str_buf, &endptr, 10); // Base 10 conversion

        // Check for conversion errors:
        // 1. If no digits were found (endptr points to the start of the string).
        // 2. If there are extra characters after the number (endptr is not at the end of the string).
        // 3. Check for overflow/underflow if the ID can exceed `int` range (though `long` is usually larger).
        if (endptr == id_str_buf || *endptr != '\0' || id_long < INT_MIN || id_long > INT_MAX) {
            fprintf(stderr, "Error: Invalid integer ID format in URI: '%s'\n", id_str_buf);
            return -1; // Not a valid integer ID
        }

        return (int)id_long; // Cast to int (assuming ID fits within int range)
    }
    fprintf(stderr, "Error: URI does not match expected ID format or is too short.\n");
    return -1; // URI does not match expected format for an ID
}

// --- Handler Implementations ---

// Handles GET requests to the root path "/".
void handle_root(struct mg_connection *c, struct mg_http_message *hm) {
    (void) hm; // Cast to void to suppress unused parameter warning.
    // Send a simple JSON response.
    send_json_response(c, 200, "{ \"message\": \"Welcome to the C API Backend! Navigate to /api/v1/items for data.\" }");
}

// Handles GET requests to "/api/v1/items" (to get all items).
void handle_get_all_items(struct mg_connection *c, struct mg_http_message *hm) {
    (void) hm; // Unused
    init_dummy_data(); // Ensure dummy data exists before retrieving.

    cJSON *root = cJSON_CreateObject(); // Create the root JSON object
    if (!root) {
        send_error_response(c, 500, "Internal Server Error", "Failed to allocate memory for JSON root object.");
        return;
    }

    cJSON *items_array = cJSON_CreateArray(); // Create a JSON array for items
    if (!items_array) {
        cJSON_Delete(root); // Clean up root object if array creation fails
        send_error_response(c, 500, "Internal Server Error", "Failed to allocate memory for JSON items array.");
        return;
    }

    cJSON_AddItemToObject(root, "items", items_array); // Add the array to the root object

    // Iterate through our in-memory items and add them to the JSON array.
    for (int i = 0; i < item_count; ++i) {
        cJSON *item_obj = cJSON_CreateObject(); // Create a JSON object for each item
        if (item_obj) {
            cJSON_AddNumberToObject(item_obj, "id", items[i].id);
            cJSON_AddStringToObject(item_obj, "name", items[i].name);
            cJSON_AddNumberToObject(item_obj, "value", items[i].value);
            cJSON_AddItemToArray(items_array, item_obj); // Add item object to the array
        } else {
            fprintf(stderr, "Warning: Failed to create JSON object for item ID %d. Skipping this item.\n", items[i].id);
            // Even if an item fails, we try to send what we have, but log the error.
        }
    }

    char *json_str = cJSON_PrintUnformatted(root); // Convert cJSON object to a compact JSON string
    if (json_str) {
        send_json_response(c, 200, json_str); // Send the JSON response with 200 OK
        free(json_str); // IMPORTANT: Free the string allocated by cJSON_PrintUnformatted
    } else {
        send_error_response(c, 500, "Internal Server Error", "Failed to stringify JSON response for all items.");
    }
    cJSON_Delete(root); // IMPORTANT: Always free the cJSON object to prevent memory leaks
}

// Handles GET requests to "/api/v1/items/{id}" (to get a single item by ID).
void handle_get_item_by_id(struct mg_connection *c, struct mg_http_message *hm) {
    init_dummy_data(); // Ensure dummy data exists.

    // Extract item ID from the URI.
    int item_id = get_item_id_from_uri(hm);
    if (item_id == -1) {
        send_error_response(c, 400, "Bad Request", "Invalid or missing item ID in URI. Expected format: /api/v1/items/{id}");
        return;
    }

    // Find the item in our in-memory store.
    item_t *item = find_item_by_id(item_id);
    if (item == NULL) {
        send_error_response(c, 404, "Not Found", "Item with specified ID not found.");
        return;
    }

    cJSON *item_obj = cJSON_CreateObject(); // Create JSON object for the found item
    if (!item_obj) {
        send_error_response(c, 500, "Internal Server Error", "Failed to allocate memory for item JSON object.");
        return;
    }
    // Populate JSON object with item data.
    cJSON_AddNumberToObject(item_obj, "id", item->id);
    cJSON_AddStringToObject(item_obj, "name", item->name);
    cJSON_AddNumberToObject(item_obj, "value", item->value);

    char *json_str = cJSON_PrintUnformatted(item_obj);
    if (json_str) {
        send_json_response(c, 200, json_str); // Send JSON response
        free(json_str); // Free allocated string
    } else {
        send_error_response(c, 500, "Internal Server Error", "Failed to stringify item JSON.");
    }
    cJSON_Delete(item_obj); // Free cJSON object
}

// Handles POST requests to "/api/v1/items" (to create a new item).
void handle_create_item(struct mg_connection *c, struct mg_http_message *hm) {
    init_dummy_data(); // Ensure dummy data is initialized if first create.

    // Check if we have space for a new item.
    if (item_count >= MAX_ITEMS) {
        send_error_response(c, 507, "Insufficient Storage", "Cannot create more items, in-memory storage limit reached.");
        return;
    }

    // Parse the request body as JSON.
    // Use ParseWithLength for safety, as hm->body.p might not be null-terminated.
    cJSON *json_body = cJSON_ParseWithLength(hm->body.p, hm->body.len);
    if (!json_body) {
        send_error_response(c, 400, "Bad Request", "Invalid JSON format in request body.");
        return;
    }

    // Get "name" and "value" fields from the parsed JSON.
    cJSON *name_obj = cJSON_GetObjectItemCaseSensitive(json_body, "name");
    cJSON *value_obj = cJSON_GetObjectItemCaseSensitive(json_body, "value");

    // Validate if fields exist and are of the correct type.
    if (!cJSON_IsString(name_obj) || (name_obj->valuestring == NULL) ||
        !cJSON_IsNumber(value_obj)) {
        cJSON_Delete(json_body); // Free parsed JSON
        send_error_response(c, 400, "Bad Request", "Missing or invalid 'name' (string) or 'value' (number) in JSON body.");
        return;
    }

    // Basic validation for name length to prevent buffer overflow.
    if (strlen(name_obj->valuestring) >= sizeof(items[0].name)) {
        cJSON_Delete(json_body);
        send_error_response(c, 400, "Bad Request", "Item name provided is too long (max 63 characters).");
        return;
    }

    // Create a new item structure.
    item_t new_item;
    new_item.id = next_item_id++; // Assign a new unique ID.
    strcpy(new_item.name, name_obj->valuestring); // Copy name (safe due to length check above).
    new_item.value = (int)cJSON_GetNumberValue(value_obj); // Get integer value.

    // Add the new item to our in-memory array.
    items[item_count] = new_item;
    item_count++;

    // Prepare the JSON response for the created item.
    cJSON *response_obj = cJSON_CreateObject();
    if (response_obj) {
        cJSON_AddNumberToObject(response_obj, "id", new_item.id);
        cJSON_AddStringToObject(response_obj, "name", new_item.name);
        cJSON_AddNumberToObject(response_obj, "value", new_item.value);

        char *json_str = cJSON_PrintUnformatted(response_obj);
        if (json_str) {
            send_json_response(c, 201, json_str); // 201 Created status code.
            free(json_str);
        } else {
            send_error_response(c, 500, "Internal Server Error", "Failed to stringify response JSON for created item.");
        }
        cJSON_Delete(response_obj);
    } else {
        send_error_response(c, 500, "Internal Server Error", "Failed to allocate memory for response JSON object.");
    }
    cJSON_Delete(json_body); // IMPORTANT: Always free parsed JSON.
}

// Handles PUT requests to "/api/v1/items/{id}" (to update an existing item).
void handle_update_item(struct mg_connection *c, struct mg_http_message *hm) {
    init_dummy_data(); // Ensure dummy data exists.

    // Extract item ID from URI.
    int item_id = get_item_id_from_uri(hm);
    if (item_id == -1) {
        send_error_response(c, 400, "Bad Request", "Invalid or missing item ID in URI. Expected format: /api/v1/items/{id}");
        return;
    }

    // Find the item to be updated.
    item_t *item = find_item_by_id(item_id);
    if (item == NULL) {
        send_error_response(c, 404, "Not Found", "Item with specified ID not found for update.");
        return;
    }

    // Parse the request body as JSON.
    cJSON *json_body = cJSON_ParseWithLength(hm->body.p, hm->body.len);
    if (!json_body) {
        send_error_response(c, 400, "Bad Request", "Invalid JSON format in request body for update.");
        return;
    }

    // Get "name" and "value" fields (they are optional for update).
    cJSON *name_obj = cJSON_GetObjectItemCaseSensitive(json_body, "name");
    cJSON *value_obj = cJSON_GetObjectItemCaseSensitive(json_body, "value");

    // Update item name if provided and valid.
    if (name_obj && cJSON_IsString(name_obj) && name_obj->valuestring != NULL) {
        if (strlen(name_obj->valuestring) >= sizeof(item->name)) {
            cJSON_Delete(json_body);
            send_error_response(c, 400, "Bad Request", "Updated item name too long (max 63 characters).");
            return;
        }
        strcpy(item->name, name_obj->valuestring);
    }

    // Update item value if provided and valid.
    if (value_obj && cJSON_IsNumber(value_obj)) {
        item->value = (int)cJSON_GetNumberValue(value_obj);
    }

    cJSON_Delete(json_body); // Free parsed JSON.

    // Respond with the updated item's data.
    cJSON *response_obj = cJSON_CreateObject();
    if (response_obj) {
        cJSON_AddNumberToObject(response_obj, "id", item->id);
        cJSON_AddStringToObject(response_obj, "name", item->name);
        cJSON_AddNumberToObject(response_obj, "value", item->value);

        char *json_str = cJSON_PrintUnformatted(response_obj);
        if (json_str) {
            send_json_response(c, 200, json_str); // 200 OK status code.
            free(json_str);
        } else {
            send_error_response(c, 500, "Internal Server Error", "Failed to stringify response JSON for updated item.");
        }
        cJSON_Delete(response_obj);
    } else {
        send_error_response(c, 500, "Internal Server Error", "Failed to allocate memory for response JSON object.");
    }
}

// Handles DELETE requests to "/api/v1/items/{id}" (to delete an item).
void handle_delete_item(struct mg_connection *c, struct mg_http_message *hm) {
    init_dummy_data(); // Ensure dummy data exists.

    // Extract item ID from URI.
    int item_id = get_item_id_from_uri(hm);
    if (item_id == -1) {
        send_error_response(c, 400, "Bad Request", "Invalid or missing item ID in URI. Expected format: /api/v1/items/{id}");
        return;
    }

    int found_index = -1;
    // Find the index of the item to be deleted.
    for (int i = 0; i < item_count; ++i) {
        if (items[i].id == item_id) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        send_error_response(c, 404, "Not Found", "Item with specified ID not found for deletion.");
        return;
    }

    // "Delete" the item by shifting subsequent items back in the array.
    // This is a simple O(N) deletion for a small array. For large arrays/production,
    // a linked list or more sophisticated data structure or a database is needed.
    for (int i = found_index; i < item_count - 1; ++i) {
        items[i] = items[i + 1];
    }
    item_count--; // Decrease the total item count.

    // Send a success message.
    send_json_response(c, 200, "{ \"message\": \"Item deleted successfully.\" }");
}

