/**
 * @file example.c
 * @author Laksh Aggarwal (aggarwallaksh54@gmail.com)
 * @brief Example program for CWebSockets
 * @version 1.0
 * @date 2023-08-11
 * 
 * Uses binance market stream to print btcusdt latest trade info
 */
#include "CWebSockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void onopen(WSS_Client *client);
void onclose(WSS_Client *client);
void onmessage(WSS_Client *client, void *data, unsigned long length);

// subscribe message for latest trade of BTC
#define SUBSCRIBE_MESSAGE "{ \"method\": \"SUBSCRIBE\", \"params\": [ \"btcusdt@trade\" ]}"

#define UNSUBSCRIBE_MESSAGE "{ \"method\": \"UNSUBSCRIBE\", \"params\": [ \"btcusdt@trade\" ]}"

int main()
{
    // sample url
    char *url = "wss://fstream.binance.com/ws/";

    // configure client
    WSS_Client client = {.on_close = onclose, .on_message = onmessage, .on_open = onopen, .url = url};

    // connect to the socket
    int ret = connect_wss_client(&client);

    if (ret)
    {
        // handle error
        fprintf(stderr, "Failed to connect.");
        curl_global_cleanup();
        exit(0);
    }

    // listen for incoming messages
    ret = listen_wss_client(&client);

    if (ret)
    {
        // handle error
        close_wss_client(&client);
        curl_global_cleanup();
        fprintf(stderr, "Failed to start listening.");
        exit(0);
    }

    // wait for 10 seconds
    sleep(10);

    ret = send_wss_client(&client, UNSUBSCRIBE_MESSAGE, strlen(UNSUBSCRIBE_MESSAGE), CURLWS_TEXT);
    if (ret)
    {
        // handle failure to send message
        close_wss_client(&client);
        curl_global_cleanup();
        exit(0);
    }

    sleep(2);

    close_wss_client(&client);
    curl_global_cleanup();
}

void onmessage(WSS_Client *client, void *data, unsigned long length)
{
    printf("Recieved data -> %.*s\n", length, data);
}

void onopen(WSS_Client *client)
{
    fprintf(stderr, "Connection successfuly opened %s\n", client->url);

    int ret = send_wss_client(client, SUBSCRIBE_MESSAGE, strlen(SUBSCRIBE_MESSAGE), CURLWS_TEXT);
    if (ret)
    {
        // handle failure to send message
        close_wss_client(client);
        curl_global_cleanup();
        exit(0);
    }
}

void onclose(WSS_Client *client)
{
    fprintf(stderr, "Connection closed %s\n", client->url);
}