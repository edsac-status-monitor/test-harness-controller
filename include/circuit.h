#ifndef CIRCUIT_H
#define CIRCUIT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libxml/tree.h>
#include "assertions.h"
    
typedef struct {
    int tpIndex;
    int writePin;
} Wire;
typedef struct {
    int number;
    int highGPIOPin;
    int lowGPIOPin;
} Valve;
typedef struct {
    Wire** wires;
    int nWires;
    int maxPins;
    Valve** valves;
    int nValves;
} Wiring;
typedef enum {
    NONE, SA0, SA1
} CircuitFault;    

void setupWiring();
void teardownWiring();
int getIndexOfTPIndexInWiring(Wiring* wiring, int tpIndex);
Wiring* createWiringFromXMLNode(AssertionsSet* assertionsSet, xmlNode* wiringNode);
void freeWiring(Wiring* wiring);
void setValveFault(Wiring* wiring, int valveNo, CircuitFault fault);
void printWiring(AssertionsSet* assertionsSet, Wiring* wiring);
void printValveWiring(Wiring* wiring);

#ifdef __cplusplus
}
#endif

#endif /* CIRCUIT_H */

