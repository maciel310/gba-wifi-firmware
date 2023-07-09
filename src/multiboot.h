#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <FS.h>

#include "gba-spi.h"

class Multiboot {

private:

public:
    static const uint32_t MULTIBOOT_START_COMMAND = 0x00006202;
    static const uint32_t MULTIBOOT_START_RESPONSE = 0x7202;

    Multiboot();
    ~Multiboot();

    bool isGbaReady();
    void startMultiboot(File romfile);
};

#endif