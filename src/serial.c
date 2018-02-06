#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "assertions.h"
#include "circuit.h"
#include "serial.h"

SerialHandle* setupSerial(const char* device, int baud) {
    SerialHandle* handle = NULL;
    int fd;
    fd = serialOpen(device, baud) ;
    if(fd >= 0) {
        handle = malloc(sizeof(SerialHandle));
        handle->fd = fd;
    } else {
        fprintf(stderr, "Could not open serial device \"%s\"\n", device);
    }
    return handle;
}

void writeSerialStr(SerialHandle* serial) {
    serialPuts(serial->fd, "Hello World\n");
}

void writeSerial(SerialHandle* serial, AssertionsSet* set, Wiring* wiring, int n) {
    int i, wiringIndex, pinIndex;
    char* send;
    send = malloc(sizeof(char) * (wiring->maxPins + 2));
    for(int i = 0; i < wiring->maxPins; i++) {
        send[i] = '0';
    }
    send[wiring->maxPins] = '\n';
    send[wiring->maxPins + 1] = '\0';
    for(i = 0; i < set->nInputs; i++) {
        wiringIndex = getIndexOfTPIndexInWiring(wiring, i);
        if(wiringIndex >= 0 && wiringIndex < wiring->nWires) {
            pinIndex = wiring->wires[wiringIndex]->writePin;
            if(pinIndex >= 0 && pinIndex < wiring->maxPins) {
                send[pinIndex] = (n & (1 << i)) == 0 ? '0' : '1';
            } else {
                fprintf(stderr, "Invalid pin index: %d\n", pinIndex);
            }
        } else {
            fprintf(stderr, "Invalid wiring index: %d\n", wiringIndex);
        }
    }
    //printf("%d => \"%s\"", n, send);
    serialPuts(serial->fd, send); 
    free(send);
}

void teardownSerial(SerialHandle* serial) {
    serialClose(serial->fd);
    free(serial);
}