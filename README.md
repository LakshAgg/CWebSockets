# CWebSockets
A simple implementation of web socket client in c using libcurl. *Note: requires libcurl version >= 7.86.0*

This library contains the implementation of WebSocket client operations, including connecting to a WebSocket server, sending and receiving data, managing client connections, and handling callbacks for open, close, and message events.

## Installation
*Make sure libcurl version >= 7.86.0 is installed.*

To install this library execute the following commands:
```sh
git clone https://github.com/LakshAgg/CWebSockets.git
cd CWebSockets

make
sudo make install
make clean
```

To uninstall execute `make uninstall`.

Compile with `-lcwebsockets -lcurl `.

## Example
Check out the [example](example.c) program which uses the Binance market stream and prints the latest trades for 10 seconds.

## WSS_Client
```c
struct WSS_Client
{
    /// @brief Called on successfully connecting to the socket (optional)
    void (*on_open)(struct WSS_Client *client);

    /// @brief Called after socket gets closed (optional)
    void (*on_close)(struct WSS_Client *client);

    /// @brief Called on recieving a message
    void (*on_message)(struct WSS_Client *client, void *data, unsigned long len);

    /// @brief The web socket stream url: begins with ws or wss
    char *url;
}
```
#### url
The URL to connect to. eg: `wss://fstream.binance.com/ws/`.

#### on_open(WSS_Client *client)
This function will be called *after* the client has successfully connected by calling `start_wss_client` or `connect_wss_client`.

#### on_message(WSS_Client *client, void *data, unsigned long len)
This function is called when a message is received from the socket.

The data received is not null-terminated.
`len` is the length of data received.

#### on_close(WSS_Client *client)
This function will be *after* the client has disconnected from the socket.

## Functions 
### connect_wss_client
```c
/**
 * @brief Just connects to the web socket.
 * @param client
 * @return int - 0 if success
 */
int connect_wss_client(WSS_Client *client);
```
This function will open a new connection and call client->on_open if successful. After you are done with the connection, call `close_wss_client`. At the end of the program call `curl_global_cleanup` to avoid any memory leaks.

### listen_wss_client
```c
/**
 * @brief Create a new thread if not done already to read from sockets.
 * @param client
 * @return int - 0 if success
 */
int listen_wss_client(WSS_Client *client);
```
A list of all open connections is maintained by CWebSockets, this is used with `poll` to wait for any new data in any connection.

If `listen_wss_client` is called for the first time, it will create a new thread using `pthread_create` which will use `poll` to wait for any data. On subsequent calls, only the list is updated to include the new client's socket.

### send_wss_client
```c
/**
 * @brief Sends the data through the socket.
 * @param client
 * @param data
 * @param size
 * @param flags - for curl_ws_send
 * @return int - 0 if success
 */
int send_wss_client(WSS_Client *client, void *data, unsigned long size, unsigned int flags);
```
This function sends the data and returns 0 on success. For more info on flags, refer to [curl_ws_send](https://curl.se/libcurl/c/curl_ws_send.html).

### close_wss_client
```c
/**
 * @brief Closes the connection
 * @param client
 */
void close_wss_client(WSS_Client *client);
```
This closes the connection and frees the memory allocated for it.

### start_wss_client
```c
/**
 * @brief Connects to the socket and starts listening
 * @param client
 * @return int
 */
int start_wss_client(WSS_Client *client);
```
This function just calls connect and listen together and returns 0 if no error occurs.

### join_thread_wss_client
This function joins the thread created for reading new data. This will block further execution.