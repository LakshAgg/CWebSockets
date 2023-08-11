/**
 * @file CWebSockets.h
 * @author Laksh Aggarwal (aggarwallaksh54@gmail.com)
 * A minimal web socket client to quickly set up ws connections.
 * contains the implementation of WebSocket client operations,
 * including connecting to a WebSocket server, sending and receiving data,
 * managing client connections, and handling callbacks for open, close, and message events.
 *
 * @copyright Copyright (c) 2023
 * @version 1.0
 */

// Header include guards
#ifndef CWEBSOCKETS_H
#define CWEBSOCKETS_H
#include <curl/curl.h>
#include <pthread.h>

typedef struct WSS_Client
{
    /// @brief Called on successfully connecting to the socket (optional)
    void (*on_open)(struct WSS_Client *client);

    /// @brief Called after socket gets closed (optional)
    void (*on_close)(struct WSS_Client *client);

    /// @brief Called on recieving a message
    void (*on_message)(struct WSS_Client *client, void *data, unsigned long len);

    /// @brief The web socket stream url: begins with ws or wss
    char *url;

    CURL *curl_handle;
    unsigned long client_id;
} WSS_Client;

/**
 * @brief Just connects to the web socket.
 * @param client
 * @return int - 0 if success
 */
int connect_wss_client(WSS_Client *client);

/**
 * @brief Sends the data through the socket.
 * @param client
 * @param data
 * @param size
 * @param flags - for curl_ws_send, eg: CURLWS_TEXT
 * @return int - 0 if success
 */
int send_wss_client(WSS_Client *client, void *data, unsigned long size, unsigned int flags);

/**
 * @brief Connects to the socket and starts listening
 * @param client
 * @return int
 */
int start_wss_client(WSS_Client *client);

/**
 * @brief Create a new thread if not done already to read from sockets.
 * @param client
 * @return int - 0 if success
 */
int listen_wss_client(WSS_Client *client);

/**
 * @brief Closes the connection
 * @param client
 */
void close_wss_client(WSS_Client *client);

void join_thread_wss_client();

#endif // Header include guard