#ifndef NETWORK_H
#define NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "edsac_representation.h"

#define MAX_MSG_STR_LENGTH 200

typedef struct {
    int server;
    int sending;
} NetworkHandle;

NetworkHandle* setupNetwork(const char* rxAddrStr, int rxPort, 
        const char* txAddrStr, int txPort);
Message* readNetworkMessage(NetworkHandle* network, time_t since);
int resendNetworkMessage(NetworkHandle* network, const Message* msg);
void teardownNetwork(NetworkHandle* network);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H */

