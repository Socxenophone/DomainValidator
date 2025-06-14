// utils.h
// Header for utility functions used across the API server.

#ifndef UTILS_H
#define UTILS_H

#include "mongoose.h" // Required for struct mg_connection

// Helper function to send a JSON response to the client.
// c: The Mongoose connection object.
// status_code: The HTTP status code (e.g., 200, 201).
// json_data: A null-terminated C string containing the JSON payload.
void send_json_response(struct mg_connection *c, int status_code, const char *json_data);

// Helper function to send an error response in JSON format.
// This formats common error details (status code, error text, message) into JSON.
// c: The Mongoose connection object.
// status_code: The HTTP status code for the error (e.g., 400, 404, 500).
// status_text: A brief, human-readable status text (e.g., "Bad Request", "Not Found").
// message: A more detailed error message for the client.
void send_error_response(struct mg_connection *c, int status_code, const char *status_text, const char *message);

#endif // UTILS_H

