// main.c
// Entry point for the Mongoose-based API server.
// Initializes the server, sets up signal handling, and starts the event loop.

#include "mongoose.h" // Mongoose networking library
#include "router.h"   // Custom routing logic header
#include "handlers.h" // Custom request handlers header (used indirectly via router)
#include <signal.h>   // For signal handling (SIGINT, SIGTERM)
#include <stdio.h>    // For fprintf (standard input/output)
#include <stdlib.h>   // For EXIT_FAILURE, EXIT_SUCCESS

// Global event manager context for Mongoose.
// This structure manages all network connections and events.
static struct mg_mgr mgr;
// Volatile signal atomic variable to safely handle signals across threads/contexts.
// Initialized to 0, changed by signal_handler to indicate a signal was caught.
static volatile sig_atomic_t s_signo;

// Signal handler function.
// Catches termination signals (e.g., Ctrl+C, system shutdown) to allow
// for graceful server shutdown.
static void signal_handler(int signo) {
    s_signo = signo; // Store the signal number
}

// Mongoose event handler function.
// This function is called by Mongoose for various events on connected clients.
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    // Check if the event is an incoming HTTP message.
    if (ev == MG_EV_HTTP_MSG) {
        // Cast event data to mg_http_message structure, which contains
        // details about the HTTP request (method, URI, headers, body).
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        // Dispatch the HTTP request to our custom router.
        router_dispatch(c, hm);
    } else if (ev == MG_EV_ERROR) {
        // Log Mongoose internal errors to standard error.
        fprintf(stderr, "Mongoose error on connection %p: %s\n", (void *) c->fd, (char *) ev_data);
    }
    (void) fn_data; // Explicitly cast to void to suppress unused parameter warning.
}

int main(void) {
    // 1. Register signal handlers for graceful shutdown.
    // SIGINT: Interrupt signal (e.g., Ctrl+C from terminal).
    // SIGTERM: Termination signal (e.g., from `kill` command or system shutdown).
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 2. Initialize Mongoose event manager.
    // This sets up the internal structures needed for network operations.
    mg_mgr_init(&mgr);

    // 3. Add an HTTP listener.
    // This creates a listening socket on the specified address and port (0.0.0.0:8000).
    // `fn` is the callback function that will handle incoming events on this listener.
    // The last argument (NULL) is user data, which is passed to `fn_data` in the callback.
    fprintf(stdout, "Starting API server on http://localhost:8000\n");
    fprintf(stdout, "To exit, press Ctrl+C\n");
    struct mg_connection *c = mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, NULL);
    if (c == NULL) {
        // If listening fails (e.g., port already in use, permissions issue), print error and exit.
        fprintf(stderr, "Error: Cannot start listener. Is port 8000 already in use or do you lack permissions?\n");
        return EXIT_FAILURE;
    }

    // 4. Main event loop.
    // This loop continuously polls Mongoose for network events.
    // It runs as long as no termination signal has been caught (s_signo remains 0).
    while (s_signo == 0) {
        mg_mgr_poll(&mgr, 500); // Poll for events, waiting up to 500ms if no events.
                                // A smaller timeout or 0 would make it more CPU-intensive
                                // but more reactive if a lot of events are expected.
    }

    // 5. Clean up Mongoose resources on exit.
    // This frees memory and closes open sockets managed by Mongoose.
    mg_mgr_free(&mgr);
    fprintf(stdout, "Server gracefully shut down.\n");

    return EXIT_SUCCESS; // Indicate successful program termination.
}

