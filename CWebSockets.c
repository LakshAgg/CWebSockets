/**
 * @file CWebSockets.c
 * @author Laksh Aggarwal (aggarwallaksh54@gmail.com)
 *
 * @copyright Copyright (c) 2023
 * @version 1.0
 */
#include "CWebSockets.h"
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>

static nfds_t client_count = 0;
static WSS_Client **clients = NULL;
static struct pollfd *fds = NULL;

// lock for clients, fds, client_count access
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t important = PTHREAD_MUTEX_INITIALIZER;

static unsigned long unique_client_id = 0;

// thread listening for any available data
static pthread_t thread = 0;

static void remove_client_arr(WSS_Client *client, bool dont_check)
{
    if (!dont_check)
        pthread_mutex_lock(&lock);

    for (nfds_t i = 0; i < client_count; i++)
    {
        // if client's id matches
        if (clients[i]->client_id == client->client_id)
        {
            // shift both file descriptor and clients by one
            for (; i < client_count - 1; i++)
            {
                clients[i] = clients[i + 1];
                fds[i].fd = fds[i + 1].fd;
            }

            // reduce client_count
            client_count--;
            break;
        }
    }

    // if no client left: free the pointers
    if (client_count == 0)
    {
        free(clients);
        clients = NULL;
        free(fds);
        fds = NULL;
    }

    if (!dont_check)
        pthread_mutex_unlock(&lock);
}

void close_wss_client(WSS_Client *client)
{
    if (client)
    {
        curl_easy_cleanup(client->curl_handle);
        if (client->on_close)
            client->on_close(client);

        // remove from sockets to poll
        pthread_mutex_lock(&important);
        remove_client_arr(client, false);
        pthread_mutex_unlock(&important);
    }
}

static void *listen_sockets_arr(void *x)
{
    while (1)
    {
        pthread_mutex_lock(&lock);

        if (client_count == 0)
        {
            thread = 0;
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }

        int r;

        r = poll(fds, client_count, 100);

        // check if poll failed
        if (r == -1)
        {
            // close all connections
            for (nfds_t i = 0; i < client_count; i++)
            {
                curl_easy_cleanup(clients[i]->curl_handle);
                if (clients[i]->on_close)
                    clients[i]->on_close(clients[i]);
                remove_client_arr(clients[i], true);
                i--;
            }
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }

        // if any socket is available to read
        if (r > 0)
        {
            for (nfds_t i = 0; i < client_count; i++)
            {
                unsigned long temp_buffer_size = 4096, recieved = 0;
                char *temp_buffer = NULL;
                const struct curl_ws_frame *meta;

                // if can be read
                if (fds[i].revents == POLLIN)
                {
                recieve:

                    // allocate temporary buffer
                    if (temp_buffer == NULL)
                    {
                        temp_buffer = malloc(temp_buffer_size);
                        if (temp_buffer == NULL)
                            continue;
                    }
                    else
                    {
                        temp_buffer_size = temp_buffer_size + recieved;
                        char *temp = realloc(temp_buffer, temp_buffer_size);
                        if (temp == NULL)
                        {
                            free(temp_buffer);
                            continue;
                        }
                        temp_buffer = temp;
                    }

                    // recieve the data
                    CURLcode r = curl_ws_recv(
                        clients[i]->curl_handle,
                        temp_buffer + recieved,
                        temp_buffer_size - recieved,
                        &recieved,
                        &meta);

                    // if not ready: ignore
                    if (r == CURLE_AGAIN)
                    {
                        free(temp_buffer);
                        continue;
                    }

                    // if other error occured
                    else if (r)
                    {
                        // close the connection
                        curl_easy_cleanup(clients[i]->curl_handle);
                        if (clients[i]->on_close)
                            clients[i]->on_close(clients[i]);
                        remove_client_arr(clients[i], true);
                        i--;
                        free(temp_buffer);
                        continue;
                    }

                    // if more data can be read: recieve
                    if (meta && meta->bytesleft)
                        goto recieve;

                    // else just handle the data
                    else
                    {
                        clients[i]->on_message(clients[i], temp_buffer, recieved);
                    }
                }
                if (temp_buffer)
                    free(temp_buffer);
            }
        }
        pthread_mutex_unlock(&lock);

        // wait for any important action: new connection or close connection
        pthread_mutex_lock(&important);
        pthread_mutex_unlock(&important);
    }
}

static int add_socket_arr(int fd, WSS_Client *client)
{
    pthread_mutex_lock(&lock);

    // if not allocated yet
    if (clients == NULL)
    {
        // allocate memory for one client
        clients = malloc(sizeof(struct WSS_Client *));
        if (!clients)
            return 1;
        fds = malloc(sizeof(struct pollfd));
        if (!fds)
        {
            free(clients);
            clients = NULL;
            return 1;
        }

        // create a new thread
        int r = pthread_create(&thread, NULL, listen_sockets_arr, NULL);
        if (r)
        {
            free(clients);
            free(fds);
            clients = NULL;
            fds = NULL;
            return r;
        }

        // assign values
        clients[0] = client;
        client_count = 1;
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
    }
    else
    {
        // realloc both clients and file descriptors
        {
            struct pollfd *temp = realloc(fds, sizeof(struct pollfd) * (client_count + 1));
            if (!temp)
                return 1;
            fds = temp;
        }
        {
            WSS_Client **temp = realloc(clients, sizeof(WSS_Client *) * (client_count + 1));
            if (!temp)
                return 1;
            clients = temp;
        }

        // assign values
        clients[client_count] = client;
        fds[client_count].fd = fd;
        fds[client_count].events = 0;
        fds[client_count].events = POLLIN;
        client_count++;
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int connect_wss_client(WSS_Client *client)
{
    if (client == NULL || client->on_message == NULL || client->url == NULL)
        return 1;

    // initialise curl
    client->curl_handle = curl_easy_init();

    if (client->curl_handle)
    {
        // set options
        curl_easy_setopt(client->curl_handle, CURLOPT_CONNECT_ONLY, 2L);
        curl_easy_setopt(client->curl_handle, CURLOPT_URL, client->url);
        curl_easy_setopt(client->curl_handle, CURLOPT_WRITEDATA, client);

        // try connecting
        CURLcode r = curl_easy_perform(client->curl_handle);

        // if fails
        if (r != CURLE_OK)
        {
            curl_easy_cleanup(client->curl_handle);
            return (int)r;
        }

        client->client_id = unique_client_id++;

        // call on_open if specified
        if (client->on_open)
            client->on_open(client);
        return 0;
    }
    else
        return CURLE_FAILED_INIT;
}

int send_wss_client(WSS_Client *client, void *data, unsigned long size, unsigned int flags)
{
    if (client == NULL || data == NULL || size == 0 || client->curl_handle == NULL)
        return 1;

    unsigned long sent = 0;

    // send whole message
    while (sent < size)
    {
        unsigned long new_sent = 0;
        CURLcode r = curl_ws_send(client->curl_handle, data + sent, size - sent, &new_sent, 0, flags);

        // if error: return it
        if (r)
            return (int)r;
        sent += new_sent;
    }
    return 0;
}

void join_thread_wss_client()
{
    if (thread)
    {
        void *temp;
        pthread_join(thread, &temp);
    }
}

int start_wss_client(WSS_Client *client)
{
    int r;
    if ((r = connect_wss_client(client)))
        return r;
    if ((r = listen_wss_client(client)))
    {
        close_wss_client(client);
        return r;
    }
    return 0;
}

int listen_wss_client(WSS_Client *client)
{
    if (client && client->curl_handle)
    {
        int sockfd;

        // get the socket
        CURLcode r = curl_easy_getinfo(client->curl_handle, CURLINFO_ACTIVESOCKET, &sockfd);

        if (sockfd && !r)
        {
            // add the socket to be polled
            pthread_mutex_lock(&important);
            int r = add_socket_arr(sockfd, client);
            pthread_mutex_unlock(&important);

            // if error; return it
            if (r)
                return r;
            return 0;
        }
        return r;
    }
    return 1;
}