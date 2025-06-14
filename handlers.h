// handlers.h
// Header for API request handler functions.
// Declares the functions that process specific HTTP requests.

#ifndef HANDLERS_H
#define HANDLERS_H

#include "mongoose.h" // Required for struct mg_connection and mg_http_message

// Declare handler functions for various API endpoints.
// Each function takes a Mongoose connection and an HTTP message as arguments.

// Handles GET requests to the root path "/"
void handle_root(struct mg_connection *c, struct mg_http_message *hm);

// Handles GET requests to "/api/v1/items" (to retrieve all items)
void handle_get_all_items(struct mg_connection *c, struct mg_http_message *hm);

// Handles GET requests to "/api/v1/items/{id}" (to retrieve a single item by ID)
void handle_get_item_by_id(struct mg_connection *c, struct mg_http_message *hm);

// Handles POST requests to "/api/v1/items" (to create a new item)
void handle_create_item(struct mg_connection *c, struct mg_http_message *hm);

// Handles PUT requests to "/api/v1/items/{id}" (to update an existing item by ID)
void handle_update_item(struct mg_connection *c, struct mg_http_message *hm);

// Handles DELETE requests to "/api/v1/items/{id}" (to delete an item by ID)
void handle_delete_item(struct mg_connection *c, struct mg_http_message *hm);

#endif // HANDLERS_H

