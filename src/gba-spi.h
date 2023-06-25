#ifndef _GBA_SPI_H
#define _GBA_SPI_H

#include <SPI.h>

class GbaSpi {
private:
    /* data */
public:
    GbaSpi();
    ~GbaSpi();
    void begin();
    uint32_t WriteSPI32(uint32_t w);
};

extern GbaSpi GBA_SPI;

#endif