#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "edsac_representation.h"
#include "edsac_sending.h"
#include "edsac_server.h"
#include "edsac_arguments.h"

NetworkHandle* setupNetwork(const char* rxAddrStr, int rxPort, 
        const char* txAddrStr, int txPort) {
    
    assert(MAX_MSG_STR_LENGTH <= MAX_MSG_LEN);
    struct sockaddr* adr;
    NetworkHandle* network = malloc(sizeof(NetworkHandle));
    network->sending = false;
    network->server = false;
    if(txAddrStr != NULL) {
        adr = alloc_addr(txAddrStr, txPort);
        assert(NULL != adr);
        network->sending = start_sending(adr, sizeof(*adr));
        free(adr);
        if(!network->sending) {
            fprintf(stderr, "Could not start sending\n");
            teardownNetwork(network);
            return NULL;
        }
    }
    if(rxAddrStr != NULL) {
        adr = alloc_addr(rxAddrStr, rxPort);
        assert(NULL != adr);
        network->server = start_server(adr, sizeof(*adr));
        free(adr);
        if(!network->server) {
            fprintf(stderr, "Could not start server\n");
            teardownNetwork(network);
            return NULL;
        }
    }
    return network;
}

Message* readNetworkMessage(NetworkHandle* network, time_t since) {
    BufferItem* buff;
    Message* msg = NULL;
    assert(network != NULL);
    if(!network->server) {
        fprintf(stderr, "Server has not been started\n");
        return NULL;
    }
    buff = read_message();
    if(buff != NULL) {
        if(difftime(since, buff->recv_time)) {
            msg = &buff->msg;
        }
    }
    return msg;
}

int resendNetworkMessage(NetworkHandle* network, const Message* msg) {
    bool state;
    assert(network != NULL);
    if(!network->sending) {
        fprintf(stderr, "Sending has not been started\n");
        return -1;
    }
    
    state = send_message(msg);
    if(state != true) {
        fprintf(stderr, "Could not send message\n");
        return -1;
    }
    return 1;
}

void teardownNetwork(NetworkHandle* network) {
    if(network->sending) {
        stop_sending();
    }
    if(network->server) {
        stop_server();
    }
    free(network);
}

