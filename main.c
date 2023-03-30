#include "ch341a.h"
#include "epaper.h"
#include <unistd.h>

uint8_t gpio_dir_mask = 0b00111111;
uint8_t gpio_data = 0;

#define clrbit(x) { gpio_data = gpio_data & (uint8_t)~x; }
#define setbit(x) { gpio_data = gpio_data | x; }


int main() {
  int ret;

  ret = ch341a_configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
  if (ret < 0) return -1;

  ret = ch341a_gpio_configure(&gpio_dir_mask);
  if (ret < 0) return -1;

  /* Init the ePaper-display */
  epd_init();
  uint8_t dummyimg[EPDSIZEX * EPDSIZEY];
  for (int dip = 0; dip < (EPDSIZEX * EPDSIZEY); dip++) {
    dummyimg[dip] = 0xff; //(dip % 2) ? 0xff : 0x00;
  }
  epd_setframememory(dummyimg, 0, 0, EPDSIZEX, EPDSIZEY);
  epd_displayframe();

  ret = ch341a_release();
  printf("\nreturn: %d\n", ret);
  return ret;
}

