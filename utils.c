// utils.c
// Implements utility functions for sending HTTP responses.

#include "utils.h"     // Header for utility function declarations
#include "mongoose.h"  // Mongoose types and functions
#include "cJSON.h"     // Required for creating JSON error responses
#include <stdio.h>     // For snprintf (used implicitly by cJSON_PrintUnformatted)
#include <stdlib.h>    // For free (used for cJSON_PrintUnformatted result)
#include <string.h>    // For strlen

// Helper function to send a JSON response to the client.
void send_json_response(struct mg_connection *c, int status_code, const char *json_data) {
    // mg_http_reply constructs and sends an HTTP response.
    // Parameters:
    // c: The connection.
    // status_code: HTTP status code (e.g., 200 OK, 201 Created).
    // fmt: Format string for HTTP headers.
    // ...: Arguments for the format string (the response body).

    // Content-Type: application/json tells the client the response is JSON.
    // Access-Control-Allow-Origin: * enables Cross-Origin Resource Sharing (CORS)
    // for all domains. This is convenient for development (e.g., if your frontend
    // is on a different port), but for production, you should restrict this
    // to specific trusted domains (e.g., "http://your-frontend-domain.com").
    mg_http_reply(c, status_code, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n", "%s", json_data);
}

// Helper function to send an error response in JSON format.
// This creates a structured JSON error message for consistency.
void send_error_response(struct mg_connection *c, int status_code, const char *status_text, const char *message) {
    cJSON *error_obj = cJSON_CreateObject(); // Create a new JSON object for the error.
    if (!error_obj) {
        // Fallback: If cJSON object creation fails, send a basic plain text error.
        fprintf(stderr, "Critical Error: Failed to create cJSON error object. Falling back to plain text error.\n");
        mg_http_reply(c, 500, "Content-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\n", "Internal Server Error: Failed to generate structured error response.");
        return;
    }

    // Populate the error JSON object with relevant details.
    cJSON_AddNumberToObject(error_obj, "status_code", status_code);
    cJSON_AddStringToObject(error_obj, "error", status_text);
    cJSON_AddStringToObject(error_obj, "message", message); // Fixed the typo here: 'mess age' -> 'message'

    char *json_str = cJSON_PrintUnformatted(error_obj); // Convert the cJSON object to a compact string.
    if (json_str) {
        // Send the formatted JSON error response.
        send_json_response(c, status_code, json_str);
        free(json_str); // IMPORTANT: Free the memory allocated by cJSON_PrintUnformatted.
    } else {
        // Fallback: If cJSON stringification fails, send a basic plain text error.
        fprintf(stderr, "Critical Error: Failed to stringify cJSON error object. Falling back to plain text error.\n");
        mg_http_reply(c, 500, "Content-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\n", "Internal Server Error: Failed to stringify error JSON response.");
    }
    cJSON_Delete(error_obj); // IMPORTANT: Always free the cJSON object to prevent memory leaks.
}

