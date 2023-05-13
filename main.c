#include "ch341a.h"
#include "epaper.h"
#include <unistd.h>
#include <gd.h>

uint8_t gpio_dir_mask = 0b00111111;
uint8_t gpio_data = 0;

#define clrbit(x) { gpio_data = gpio_data & (uint8_t)~x; }
#define setbit(x) { gpio_data = gpio_data | x; }

gdImagePtr loadimage(char * fn)
{
  gdImagePtr res;
  FILE * f;
  f = fopen(fn, "rb");
  if (f == NULL) {
    fprintf(stderr, "ERROR: Failed to open file '%s'\n", fn);
    exit(1);
  }
  res = gdImageCreateFromPng(f);
  fclose(f);
  return res;
}

int main()
{
  int ret;
  
  gdImagePtr im = loadimage("/tmp/komischermuell.png");
#if 0
  for (int y = 0; y < 128; y++) {
    for (int x = 0; x < 296; x++) {
      int c = gdImageGetTrueColorPixel(im, x, y);
      if (c == 0) {
        printf("0");
      } else {
        printf("1");
      }
    }
    printf("\n");
  }
#endif

  ret = ch341a_configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
  if (ret < 0) return -1;

  ret = ch341a_gpio_configure(&gpio_dir_mask);
  if (ret < 0) return -1;

  /* Init the ePaper-display */
  epd_init();
  epd_setframememory(im);
  epd_displayframe();

  ret = ch341a_release();
  printf("\nreturn: %d\n", ret);
  gdImageDestroy(im);
  return ret;
}

