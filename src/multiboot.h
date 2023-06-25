#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <FS.h>

#include "gba-spi.h"

class Multiboot {

private:

public:
    Multiboot();
    ~Multiboot();

    bool isGbaReady();
    void startMultiboot(File romfile);
};

#endif