#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "network.h"
#include "assertions.h"
#include "circuit.h"
#include "serial.h"
#include "edsac_representation.h"

#define CIRCUIT_FILNAME "config/circuit.xml"
#define WIRING_FILNAME "config/wiring.xml"

#define PROGRAM_NAME "edsac_status_tester"
#define TX_ADDRESS "127.0.0.1"
#define TX_PORT 2000
#define RX_ADDRESS "127.0.0.1"
#define RX_PORT 2000
#define SERIAL_DEVICE "/dev/ttyS0"
#define BAUD_RATE 9600
#define CYCLE_DELAY_MS 10

#define ECHO_ONLY 0

#define N_PARAMS 8
#define MAX_ARG_LEN 64

void parseCircuitFile(const char* filename, AssertionsSet** set) {
    xmlDoc *doc = NULL;
    xmlNode *root = NULL;
    doc = xmlReadFile(filename, NULL, 0);
    if(doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
        return;
    }
    root = xmlDocGetRootElement(doc);
    *set = createAssertionSetFromXMLNode(root);
    
    xmlFreeDoc(doc);
}

void parseWiringFile(const char* filename, AssertionsSet* set, Wiring** wiring) {
    xmlDoc *doc = NULL;
    xmlNode *root = NULL;
    doc = xmlReadFile(filename, NULL, 0);
    if(doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
        return;
    }
    root = xmlDocGetRootElement(doc);
    *wiring = createWiringFromXMLNode(set, root);
    
    xmlFreeDoc(doc);
}

void get(AssertionsSet** set, Wiring** wiring) {
    parseCircuitFile(CIRCUIT_FILNAME, set);
    assert(*set != NULL);
    parseWiringFile(WIRING_FILNAME, *set, wiring);
    assert(*wiring != NULL);
}

void listenForErrorsOn(NetworkHandle* net, time_t since, int valveNo) {
    Message* rxMsg;
    int unexpected;
    int nExpected = 0;
    while((rxMsg = readNetworkMessage(net, since)) != NULL) {
        unexpected = false;
        switch(rxMsg->type) {
            case HARD_ERROR_VALVE: {
                if(rxMsg->data.hardware_valve.valve_no != valveNo) {
                    unexpected = true;
                    printf("Unexpected hardware other message received %s\n", rxMsg->data.hardware_other.message);
                } else {
                    nExpected++;
                }
                break;
            }
            case HARD_ERROR_OTHER: {
                unexpected = true;
                printf("Unexpected hardware other message received %s\n", rxMsg->data.hardware_other.message);
                break;
            }
            case SOFT_ERROR: {
                unexpected = true;
                printf("Unexpected software message received %s\n", rxMsg->data.software.message);
                break;
            }
            case KEEP_ALIVE:
            case INVALID:
            default: {
                unexpected = true;
                printf("Unknown, unexpected message received\n");
            }
        }
        if(net->sending && unexpected) {
            resendNetworkMessage(net, rxMsg);
        }
    }
    if(valveNo >= 0 && nExpected <= 0) {
        printf("None of the expected error messages were received\n");
    }
}

void testFaults(AssertionsSet* set, Wiring* wiring, SerialHandle* serial, 
        NetworkHandle* net, int valveNo, CircuitFault fault, int delayMs) {
    
    int nCombs, i;
    time_t timeStarted;
    
    printf("Testing Valve %d simulated with fault=%d\n", valveNo, fault);
    nCombs = (2 << (set->nInputs - 1));
    setValveFault(wiring, valveNo, fault);
    time(&timeStarted);
    for(i = 0; i < nCombs; i++) {
        writeSerial(serial, set, wiring, i);
        delay(delayMs);
    }
    listenForErrorsOn(net, timeStarted, fault == NONE ? -1 : valveNo);
}

typedef struct {
    const char* name;
    const char* format;
    void* dest;
    const char* argsName;
    const char* description;
} CmdLineParam;

/*
 * 
 */
int main(int argc, char** argv) {
    LIBXML_TEST_VERSION

    NetworkHandle* netHndl;
    SerialHandle* serialHndl;
    AssertionsSet* assertions;
    Wiring* wiring;
    char* programName;
    int maxOptionLen;
    int i, j, k, valveNo;
    char* deviceName;
    char* rxAddr;
    int rxPort;
    char* txAddr;
    int txPort;
    int echoOnly, readInOnly, helpMessage;
    int optionsParsingFailed = 0;
    
    programName = PROGRAM_NAME;
    if(argc > 0) {
        programName = argv[0];
    }
    
    rxAddr = malloc(sizeof(char) * (MAX_ARG_LEN + 1));
    assert(strlen(RX_ADDRESS) <= MAX_ARG_LEN);
    strcpy(rxAddr, RX_ADDRESS);
    rxPort = RX_PORT;
    txAddr = malloc(sizeof(char) * (MAX_ARG_LEN + 1));
    assert(strlen(TX_ADDRESS) <= MAX_ARG_LEN);
    strcpy(txAddr, TX_ADDRESS);
    txPort = TX_PORT;
    deviceName = malloc(sizeof(char) * (MAX_ARG_LEN + 1));
    assert(strlen(SERIAL_DEVICE) <= MAX_ARG_LEN);
    strcpy(deviceName, SERIAL_DEVICE);
    echoOnly = ECHO_ONLY;
    readInOnly = 0;
    helpMessage = 0;
    
    CmdLineParam params[N_PARAMS] = {
        { .name="--rx-addr", .format="%s", .dest=rxAddr, .argsName="<address>", .description="The IP address on which to listen for error messages from the node being tested"},
        { .name="--rx-port", .format="%d", .dest=&rxPort, .argsName="<port>", .description="The IP port on which to listen for error messages from the node being tested"},
        { .name="--tx-addr", .format="%s", .dest=txAddr, .argsName="<address>", .description="The IP address from which to send error messages to the mothership"},
        { .name="--tx-port", .format="%d", .dest=&txPort, .argsName="<port>", .description="The IP port from which to send error messages to the mothership"},
        { .name="--serial-device", .format="%s", .dest=deviceName, .argsName="<device>", .description="The serial device acting as the TPG"},
        { .name="--no-up-network", .format=NULL, .dest=&echoOnly, .argsName=NULL, .description="Do not relay any error messages to the mothership and simply echo them"},
        { .name="--help", .format=NULL, .dest=&helpMessage, .argsName=NULL, .description="Display this help message"},
        { .name="--read-config", .format=NULL, .dest=&readInOnly, .argsName=NULL, .description="Echo the parsed contents of the configuration files"}
    };
    
    for(i = 1; i < argc && !optionsParsingFailed; i++) {
        for(j = 0; j < N_PARAMS; j++) {
            if(strcmp(argv[i], params[j].name) == 0) {
                if(params[j].format != NULL) {
                    i++;
                    if(i >= argc) {
                        fprintf(stderr, "No value specified for %s option\n", argv[i]);
                        optionsParsingFailed = 1;
                        break;
                    }
                    if(strlen(argv[i]) > MAX_ARG_LEN) {
                        fprintf(stderr, "Value specified for %s option, \"%s\", is too large. Maximum length is %d\n", params[j].name, argv[i], MAX_ARG_LEN);
                        optionsParsingFailed = 1;
                        break;
                    }
                    k = sscanf(argv[i], params[j].format, params[j].dest);
                    if(k != 1) {
                        fprintf(stderr, "Value specified for %s option, \"%s\", could not be parsed (%d)\n", params[j].name, argv[i], k);
                        optionsParsingFailed = 1;
                        break;
                    }
                } else {
                    *((int*)params[j].dest) = true;
                }
                break;
            }
        }
        if(j >= N_PARAMS) {
            fprintf(stderr, "Unrecognised option, \"%s\"\n", argv[i]);
            optionsParsingFailed = 1;
            break;
        }
    }
    if(optionsParsingFailed) {
        printf("Try \"%s --help\" for help on using this program\n", programName);
        return -1;
    }
    
    if(helpMessage) {
        printf("Usage: %s [options]\nOptions:\n", programName);
        maxOptionLen = 0;
        char** optionStrings;
        assert((optionStrings = malloc(sizeof(char*) * N_PARAMS)) != NULL);
        for(j = 0; j < N_PARAMS; j++) {
            int len = strlen(params[j].name);
            if(params[j].format != NULL) {
                len += 1 + strlen(params[j].argsName);
            }
            assert((optionStrings[j] = malloc(sizeof(char) * (len + 1))) != NULL);
            if(params[j].format != NULL) {
                snprintf(optionStrings[j], len + 1, "%s %s", params[j].name, params[j].argsName);
            } else {
                snprintf(optionStrings[j], len + 1, "%s", params[j].name);
            }
            if(len > maxOptionLen) {
                maxOptionLen = len;
            }
        }
        for(j = 0; j < N_PARAMS; j++) {
            printf("  %-*s %s\n", maxOptionLen + 1, optionStrings[j], params[j].description);
            free(optionStrings[j]);
        }
        free(optionStrings);
    } else {
        printf("RX: %s:%d\nTX: %s:%d\nDevice: %s\nEcho Only: %s\n",
                rxAddr, rxPort, txAddr, txPort, deviceName, echoOnly ? "true" : "false");

        serialHndl = setupSerial(deviceName, BAUD_RATE) ;
        if(serialHndl == NULL) {
            return -1;
        }

        if(echoOnly) {
            netHndl = setupNetwork(rxAddr, rxPort, txAddr, txPort);
        } else {
            netHndl = setupNetwork(rxAddr, rxPort, NULL, 0);
        }
        if(netHndl == NULL) {
            return -1;
        }

        setupWiring();

        get(&assertions, &wiring);

        free(rxAddr);
        free(txAddr);
        free(deviceName);

        printTPs(assertions);
        printTruthTable(assertions);
        printWiring(assertions, wiring);
        printValveWiring(wiring);

        for(j = 0; j < wiring->nValves; j++) {
            valveNo = wiring->valves[j]->number;
            testFaults(assertions, wiring, serialHndl, netHndl, valveNo, NONE, CYCLE_DELAY_MS);
            testFaults(assertions, wiring, serialHndl, netHndl, valveNo, SA0, CYCLE_DELAY_MS);
            testFaults(assertions, wiring, serialHndl, netHndl, valveNo, SA1, CYCLE_DELAY_MS);
        }

        freeAssertionSet(assertions);
        freeWiring(wiring);
        teardownWiring();
        teardownNetwork(netHndl);
        teardownSerial(serialHndl);
    }
    return (EXIT_SUCCESS);
}

