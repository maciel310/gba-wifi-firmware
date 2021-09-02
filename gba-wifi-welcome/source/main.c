#include <tonc.h>
#include <string.h>

void handle_serial() {
  u32 data = REG_SIODATA32;
  if (data == 0xAABBCCDD) {
    // Trigger a HardReset.
    asm("swi 0x26");
  }

  REG_SIOCNT |= SION_ENABLE;
}

void serial_init() {
  REG_RCNT = 0;
  REG_SIODATA32 = 0;
  REG_SIOCNT = SION_CLK_EXT | SIO_MODE_32BIT | SIO_IRQ;

  irq_add(II_SERIAL, handle_serial);

  REG_SIOCNT |= SION_ENABLE;
}

int main() {
  irq_init(NULL);
  irq_add(II_VBLANK, NULL);
  REG_DISPCNT= DCNT_MODE0 | DCNT_BG0;

  // --- (1) Base TTE init for tilemaps ---
  tte_init_se(
      /*bgNumber=*/ 0,
      /*bgControl=*/ BG_CBB(0)|BG_SBB(31),
      /*tileOffset=*/ 0,
      /*inkColor=*/ CLR_YELLOW,
      /*bitUnpackOffset=*/ 14,
      /*defaultFont=*/ NULL,
      /*defaultRenderer=*/ NULL);

  // Init alternative ink colors
  pal_bg_bank[1][15]= CLR_BLUE;

  tte_erase_screen();
  tte_write("\n\nThanks for buying my GBA WiFi Adapter!\n\n");
  tte_write("First, connect to the GbaWiFi network to configure.\n\n");
  tte_write("Then, to load a game visit\nhttp://gba.local/\nin your browser.\n\n");
  tte_write("If you have questions or\nissues contact me at\nmaciel310@gmail.com");

  serial_init();

  while (true) {
    VBlankIntrWait();
  }
}
