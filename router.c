// router.c
// Implements the API routing logic, dispatching incoming HTTP requests
// to the appropriate handler functions based on method and URI.

#include "router.h"    // Header for route_t and router_dispatch declaration
#include "mongoose.h"  // Mongoose library functions (for mg_vcmp)
#include "handlers.h"  // Include specific handlers to register them in the routes array
#include "utils.h"     // For send_error_response
#include <string.h>    // For strcmp, strncmp, strlen

// Array of registered routes.
// This defines all the API endpoints and their corresponding handlers.
// The order matters: more specific routes (e.g., exact matches) should
// generally come before less specific ones (e.g., prefix matches),
// especially if prefixes overlap.
static const route_t routes[] = {
    // GET all items: Matches exactly "/api/v1/items"
    {"GET", "/api/v1/items", 1, handle_get_all_items},
    // POST create item: Matches exactly "/api/v1/items"
    {"POST", "/api/v1/items", 1, handle_create_item},

    // Routes requiring an item ID (e.g., /api/v1/items/123)
    // These use uri_prefix ending with '/' and exact_match=0 (prefix match).
    // The handler function is responsible for parsing the ID from the URI.
    {"GET", "/api/v1/items/", 0, handle_get_item_by_id},
    {"PUT", "/api/v1/items/", 0, handle_update_item},
    {"DELETE", "/api/v1/items/", 0, handle_delete_item},

    // Root endpoint: Matches exactly "/"
    {"GET", "/", 1, handle_root},

    // Add more routes here as your API grows.
    // Example for a different endpoint:
    // {"GET", "/api/v1/users", 1, handle_get_all_users},
};

// Calculate the total number of routes in the array.
static const size_t num_routes = sizeof(routes) / sizeof(routes[0]);

// Function to dispatch an incoming HTTP request to the appropriate handler.
void router_dispatch(struct mg_connection *c, struct mg_http_message *hm) {
    // Log the incoming request for debugging purposes.
    fprintf(stdout, "Incoming request: %s %.*s\n", hm->method.p, (int)hm->uri.len, hm->uri.p);

    for (size_t i = 0; i < num_routes; ++i) {
        const route_t *route = &routes[i];

        // 1. Check if the HTTP method matches.
        if (strcmp(hm->method.p, route->method) != 0) {
            continue; // Method does not match, try the next route.
        }

        // 2. Check if the URI matches the route's specification.
        if (route->exact_match) {
            // For exact matches, compare the full URI string.
            if (mg_vcmp(&hm->uri, route->uri_prefix) == 0) {
                // Found an exact matching route, call its handler.
                route->handler(c, hm);
                return; // Request handled, exit dispatch.
            }
        } else {
            // For prefix matches, check if the URI starts with the defined prefix.
            // This is typically used for routes like /api/v1/items/{id}
            if (hm->uri.len >= strlen(route->uri_prefix) &&
                mg_vcmp(&hm->uri, route->uri_prefix) == 0) {
                // Found a matching route prefix, call its handler.
                // The handler will then be responsible for parsing any ID from the URI.
                route->handler(c, hm);
                return; // Request handled, exit dispatch.
            }
        }
    }

    // If the loop completes, no matching route was found.
    // Send a 404 Not Found error response.
    send_error_response(c, 404, "Not Found", "The requested resource or endpoint was not found on this server.");
}

