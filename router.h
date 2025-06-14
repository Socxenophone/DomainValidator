// router.h
// Header for the API routing logic.
// Defines data structures and functions for dispatching HTTP requests to handlers.

#ifndef ROUTER_H
#define ROUTER_H

#include "mongoose.h" // Required for struct mg_connection and mg_http_message

// Define a function pointer type for request handlers.
// All API handler functions must conform to this signature.
typedef void (*request_handler_fn)(struct mg_connection *c, struct mg_http_message *hm);

// Structure to define a single API route.
typedef struct {
    const char *method;      // HTTP method (e.g., "GET", "POST", "PUT", "DELETE")
    const char *uri_prefix;  // URI prefix for matching (e.g., "/api/items", "/api/items/")
                             // Use an exact string for non-ID routes, or a prefix ending with '/' for ID-based routes.
    int exact_match;         // 1 if the URI must match exactly, 0 if it's a prefix match.
    request_handler_fn handler; // Pointer to the handler function that will process this route.
} route_t;

// Function to dispatch an incoming HTTP request to the appropriate handler.
// This function iterates through the defined routes and calls the matching handler.
void router_dispatch(struct mg_connection *c, struct mg_http_message *hm);

#endif // ROUTER_H

