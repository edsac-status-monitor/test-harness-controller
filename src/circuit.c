#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "circuit.h"
#include "xmlutil.h"
#include "tables.h"
    
#define NODE_NAME_TP "tp"
#define NODE_NAME_VALVE "valve"
#define ATTR_NAME_ID "id"
#define ATTR_NAME_PIN "pin"
#define ATTR_NAME_NUMBER "number"
#define ATTR_NAME_HIGH_PIN "high-pin"
#define ATTR_NAME_LOW_PIN "low-pin"

#define TABLE_TITLE "Wiring"
#define TABLE_TP_HEADING "TP"
#define TABLE_PIN_HEADING "Arduino Pin"
#define TABLE_VALVES_TITLE "Valve Wiring"
#define TABLE_VALVE_NO_HEADING "Valve No"
#define TABLE_LOW_PIN_HEADING "Low Pin"
#define TABLE_HIGH_PIN_HEADING "High Pin"

void setupWiring() {
    wiringPiSetupGpio();
}

void teardownWiring() {
    
}

Wire* createWire(int tpIndex, int pin) {
    Wire* wire = malloc(sizeof(Wire));
    wire->tpIndex = tpIndex;
    wire->writePin = pin;
    return wire;
}

void freeWire(Wire* wire) {
    free(wire);
}

Valve* createValve(int number, int lowPin, int highPin) {
    Valve* valve = malloc(sizeof(Valve));
    valve->number = number;
    valve->lowGPIOPin = lowPin;
    valve->highGPIOPin = highPin;
    return valve;
}

void freeValve(Valve* valve) {
    free(valve);
}

int getIndexOfTPIndexInWiring(Wiring* wiring, int tpIndex) {
    int i;
    for(i = 0; i < wiring->nWires; i++) {
        if(wiring->wires[i]->tpIndex == tpIndex) {
            return i;
        }
    }
    return -1;
}

Wiring* createWiringFromXMLNode(AssertionsSet* set, xmlNode* wiringNode) {
    assert(set != NULL);
    assert(wiringNode != NULL);
    int j, tpIndex, pin, number, highPin, lowPin;
    Wiring* wiring = malloc(sizeof(Wiring));
    xmlNode* child;
    
    child = wiringNode->children;
    wiring->wires = NULL;
    wiring->nWires = 0;
    wiring->maxPins = 0;
    wiring->valves = NULL;
    wiring->nValves = 0;
    while(child != NULL) {
        if(child->type == XML_ELEMENT_NODE) {
            if(strEqual(child->name, NODE_NAME_TP)) {
                if(!xmlHasProp(child, ATTR_NAME_ID)) {
                    freeWiring(wiring);
                    fprintf(stderr, "TP node has no id\n");
                    return NULL;
                }
                if(!xmlHasProp(child, ATTR_NAME_PIN)) {
                    freeWiring(wiring);
                    fprintf(stderr, "TP node has no pin\n");
                    return NULL;
                }
                tpIndex = getIndexOfTPNodeInSet(set, child);
                if(tpIndex < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "TP node refers to a tp not in the assertions set\n");
                    return NULL;
                }
                for(j = 0; j < wiring->nWires; j++) {
                    if(wiring->wires[j]->tpIndex == tpIndex) {
                        freeWiring(wiring);
                        fprintf(stderr, "TP node is referenced twice in wiring file\n");
                        return NULL;
                    }
                }
                pin = nodePropAsInteger(child, ATTR_NAME_PIN);
                if(pin < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "TP node has an invalid pin attribute\n");
                    return NULL;
                }
                
                wiring->wires = realloc(wiring->wires, sizeof(Wire*) * (wiring->nWires + 1));
                wiring->wires[wiring->nWires] = createWire(tpIndex, pin);
                wiring->nWires++;
                if(pin + 1 > wiring->maxPins) {
                    wiring->maxPins = pin + 1;
                }
            } else if(strEqual(child->name, NODE_NAME_VALVE)) {
                if(!xmlHasProp(child, ATTR_NAME_NUMBER)) {
                    fprintf(stderr, "valve node has no number\n");
                    freeWiring(wiring);
                    return NULL;
                }
                if(!xmlHasProp(child, ATTR_NAME_HIGH_PIN)) {
                    fprintf(stderr, "valve node has no high pin\n");
                    freeWiring(wiring);
                    return NULL;
                }
                if(!xmlHasProp(child, ATTR_NAME_HIGH_PIN)) {
                    fprintf(stderr, "valve node has no low pin\n");
                    freeWiring(wiring);
                    return NULL;
                }
                number = nodePropAsInteger(child, ATTR_NAME_NUMBER);
                for(j = 0; j < wiring->nValves; j++) {
                    if(wiring->valves[j]->number == number) {
                        freeWiring(wiring);
                        fprintf(stderr, "valve number is referenced twice in wiring file\n");
                        return NULL;
                    }
                }
                highPin = nodePropAsInteger(child, ATTR_NAME_HIGH_PIN);
                if(highPin < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "valve node has an invalid high pin attribute\n");
                    return NULL;
                }
                highPin = physPinToGpio(highPin);
                if(highPin < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "valve node has an invalid high pin attribute as a GPIO\n");
                    return NULL;
                }
                lowPin = nodePropAsInteger(child, ATTR_NAME_LOW_PIN);
                if(lowPin < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "valve node has an invalid low pin attribute\n");
                    return NULL;
                }
                lowPin = physPinToGpio(lowPin);
                if(lowPin < 0) {
                    freeWiring(wiring);
                    fprintf(stderr, "valve node has an invalid low pin attribute as a GPIO\n");
                    return NULL;
                }
                
                wiring->valves = realloc(wiring->valves, sizeof(Valve*) * (wiring->nValves + 1));
                wiring->valves[wiring->nValves] = createValve(number, lowPin, highPin);
                wiring->nValves++;
            } else {
                freeWiring(wiring);
                fprintf(stderr, "Unknown node name: \"%s\"\n", child->name);
                return NULL;
            }
        }
        child = child->next;
    }
    
    if(wiring->nWires != set->nInputs) {
        freeWiring(wiring);
        fprintf(stderr, "Number of inputs circuit inputs does not match number of wires\n", child->name);
        return NULL;
    }
    
    for(j = 0; j < wiring->nValves; j++) {
        pinMode(wiring->valves[j]->lowGPIOPin, OUTPUT);
        pinMode(wiring->valves[j]->highGPIOPin, OUTPUT);
    }
    
    return wiring;
}

void freeWiring(Wiring* wiring) {
    int i;
    if(wiring != NULL) {
        for(i = 0; i < wiring->nWires; i++) {
            freeWire(wiring->wires[i]);
        }
        for(i = 0; i < wiring->nValves; i++) {
            freeValve(wiring->valves[i]);
        }
        free(wiring);
    }
}

void setValveFault(Wiring* wiring, int valveNo, CircuitFault fault) {
    int i, lowVal, highVal;
    for(i = 0; i < wiring->nValves; i++) {
        lowVal = LOW;
        highVal = LOW;
        if(wiring->valves[i]->number == valveNo) {
            if(fault == SA0) {
                lowVal = HIGH;
            } else if(fault == SA1) {
                highVal = HIGH;
            }
        }
        digitalWrite(wiring->valves[i]->lowGPIOPin, lowVal);
        digitalWrite(wiring->valves[i]->highGPIOPin, highVal);
    }
}

/*void readInTPValues(Wiring* wiring, int* dest) {
    int i;
    for(i = 0; i < wiring->nWires; i++) {
        dest[wiring->wires[i]->tpIndex] = digitalRead(wiring->wires[i]->gpioPin);
    }
}*/

void printWiring(AssertionsSet* set, Wiring* wiring) {
    int i, j, maxCellStringLen, nColumns, nRows;
    char** columns;
    char*** rows;
    assert(set != NULL);
    assert(wiring != NULL);
    
    maxCellStringLen = 255;
    nColumns = 2;
    assert((columns = malloc(sizeof(char*) * nColumns)) != NULL);
    columns[0] = TABLE_TP_HEADING;
    columns[1] = TABLE_PIN_HEADING;
    nRows = wiring->nWires;
    assert((rows = malloc(sizeof(char**) * nRows)) != NULL);
    for(i = 0; i < nRows; i++) {
        rows[i] = malloc(sizeof(char*) * nColumns);
        rows[i][0] = malloc(sizeof(char) * maxCellStringLen);
        snprintf(rows[i][0], maxCellStringLen, "%s", set->tps[wiring->wires[i]->tpIndex]->tpName);
        rows[i][1] = malloc(sizeof(char) * maxCellStringLen);
        snprintf(rows[i][1], maxCellStringLen, "%d", wiring->wires[i]->writePin);
    }
    printTable(stdout, TABLE_TITLE, columns, nColumns, rows, nRows);
    for(i = 0; i < nRows; i++) {
        for(j = 0; j < nColumns; j++) {
            free(rows[i][j]);
        }
        free(rows[i]);
    }
    free(rows);
    free(columns);
}

void printValveWiring(Wiring* wiring) {
    int i, j, maxCellStringLen, nColumns, nRows;
    char** columns;
    char*** rows;
    assert(wiring != NULL);
    
    maxCellStringLen = 32;
    nColumns = 3;
    assert((columns = malloc(sizeof(char*) * nColumns)) != NULL);
    columns[0] = TABLE_VALVE_NO_HEADING;
    columns[1] = TABLE_LOW_PIN_HEADING;
    columns[2] = TABLE_HIGH_PIN_HEADING;
    nRows = wiring->nValves;
    assert((rows = malloc(sizeof(char**) * nRows)) != NULL);
    for(i = 0; i < nRows; i++) {
        rows[i] = malloc(sizeof(char*) * nColumns);
        rows[i][0] = malloc(sizeof(char) * maxCellStringLen);
        snprintf(rows[i][0], maxCellStringLen, "%d", wiring->valves[i]->number);
        rows[i][1] = malloc(sizeof(char) * maxCellStringLen);
        snprintf(rows[i][1], maxCellStringLen, "%d", wiring->valves[i]->lowGPIOPin);
        rows[i][2] = malloc(sizeof(char) * maxCellStringLen);
        snprintf(rows[i][2], maxCellStringLen, "%d", wiring->valves[i]->highGPIOPin);
    }
    printTable(stdout, TABLE_VALVES_TITLE, columns, nColumns, rows, nRows);
    for(i = 0; i < nRows; i++) {
        for(j = 0; j < nColumns; j++) {
            free(rows[i][j]);
        }
        free(rows[i]);
    }
    free(rows);
    free(columns);
}