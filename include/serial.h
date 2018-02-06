#ifndef SERIAL_H
#define SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "assertions.h"
#include "circuit.h"
    
typedef struct {
    int fd;
} SerialHandle;

SerialHandle* setupSerial(const char* device, int baud);
void teardownSerial(SerialHandle* serial);
void writeSerialStr(SerialHandle* serial);
void writeSerial(SerialHandle* serial, AssertionsSet* set, Wiring* wiring, int n);
    
#ifdef __cplusplus
}
#endif

#endif /* SERIAL_H */

