#include "ch341a.h"
#include "epaper.h"
#include <unistd.h>
#include <gd.h>

/* global variables that keep the state of the USB-GPIO-Adapter. */
uint8_t gpio_dir_mask = 0b00111111;
uint8_t gpio_data = 0;

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

void showhelpandexit(char * pn)
{
  printf("epaper-nametag-programmer compiled on %s %s\n",
          __DATE__, __TIME__);
  printf("Syntax: %s COMMAND parametersforcommand\n", pn);
  printf("where COMMAND can be one of:\n");
  printf(" loadpng    loads an image from a .png file.\n");
  printf("            takes exactly one parameter: the filename.\n");
  exit(1);
}

int main(int argc, char ** argv)
{
  int ret;
  gdImagePtr im;

  if (argc < 2) {
    showhelpandexit(argv[0]);
  }

  if (strcmp(argv[1], "loadpng") == 0) {
    if (argc != 3) {
      fprintf(stderr, "ERROR: loadpng needs exactly one parameter: a filename to load.\n");
      exit(1);
    }
    im = loadimage(argv[2]);
    if ((gdImageSX(im) != EPDSIZEX) || (gdImageSY(im) != EPDSIZEY)) {
      printf("Warning: The image does not have the correct size for the epaper-display\n");
      printf("         (which would be %u x %u pixels).\n", EPDSIZEX, EPDSIZEY);
    }
#if 0
    /* Dump the image to get an impression of how it looks */
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
  } else if (strcmp(argv[1], "foo") == 0) {
    /* Create a new image in memory */
    im = gdImageCreateTrueColor(EPDSIZEX, EPDSIZEY);
    /* Get color values */
    int colorwhite = gdImageColorClosest(im, 255, 255, 255);
    int colorblack = gdImageColorClosest(im,   0,   0,   0);
    /* Fill the whole image with white */
    gdImageFilledRectangle(im, 0, 0, (EPDSIZEX - 1), (EPDSIZEY - 1), colorwhite);
    /* Draw around in it */
    gdImageSetPixel(im, 10, 10, colorblack);
    gdImageLine(im, 0, 0, (EPDSIZEX - 1), (EPDSIZEY - 1), colorblack);
    /* FIXME implement me */
  } else {
    fprintf(stderr, "ERROR: %s is not a valid command.\n", argv[1]);
    exit(1);
  }

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

