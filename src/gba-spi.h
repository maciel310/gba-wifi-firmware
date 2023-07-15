#ifndef _GBA_SPI_H
#define _GBA_SPI_H

#include <SPI.h>

class GbaSpi {
private:
    bool debugging_enabled = false;

public:
    GbaSpi();
    ~GbaSpi();
    void begin();
    uint32_t WriteSPI32(uint32_t w);
    
    void enableDebugging();
    void disableDebugging();
};

extern GbaSpi GBA_SPI;

#endif