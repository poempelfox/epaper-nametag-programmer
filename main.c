#include "ch341a.h"
#include <unistd.h>

/* We use the following cabling:
 * VCC  == VCC
 * GND  == GND
 * DIN  == MOSI
 * CLK  == SCK
 * CS   == CS0
 * DC   == CS1  (data/command control pin)
 * RST  == CS2
 * BUSY == MISO
 */

/* CS is wired to CS0, gpio-data bit10 */
#define CSBIT 0x01
/* DC is wired to CS1, gpio-data bit 1 */
#define DCBIT 0x02
/* RST is wired to CS2, gpio-data bit 2 */
#define RSTBIT 0x04
/* CLK is wired to SCK, gpio-data bit 3 */
#define CLKBIT 0x08
/* DIN is wired to MOSI, gpio-data bit 5 */
#define DATABIT 0x20
/* BUSY is wired to MISO, gpio-data bit 7 (Read-only!) */
#define BUSYBIT 0x80

/* extra sleep between bitbanging, in case the I/O over USB is not slow enough... */
#define SLPDELAY 100

/* resolution of our epaper display */
#define EPDSIZEX 128
#define EPDSIZEY 296

uint8_t gpio_dir_mask = 0b00111111;
uint8_t gpio_data = 0;

#define clrbit(x) { gpio_data = gpio_data & (uint8_t)~x; }
#define setbit(x) { gpio_data = gpio_data | x; }

/* This is some init stuff for the epaper display.
 * I have no idea what it does. This is C&P from the manufacturer. */
unsigned char WS_20_30[159] =
{											
  0x80,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x40,	0x0,	0x0,	0x0,
  0x10,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x20,	0x0,	0x0,	0x0,
  0x80,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x40,	0x0,	0x0,	0x0,
  0x10,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x20,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x14,	0x8,	0x0,	0x0,	0x0,	0x0,	0x1,
  0xA,	0xA,	0x0,	0xA,	0xA,	0x0,	0x1,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x14,	0x8,	0x0,	0x1,	0x0,	0x0,	0x1,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x1,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
  0x44,	0x44,	0x44,	0x44,	0x44,	0x44,	0x0,	0x0,	0x0,
  0x22,	0x17,	0x41,	0x0,	0x32,	0x36
};	


void wrapgpioinst() {
  int ret = ch341a_gpio_instruct(&gpio_dir_mask, &gpio_data);
  if (ret < 0) {
    fprintf(stderr, "There was an error changing the I/O pins. Aborting.\n");
    ch341a_release();
    exit(1);
  }
  //printf("DEBUG: gpio state 0x%02x\n", gpio_data);
}

void epd_waitfornotbusy(void) {
  long repctr = 0;
  while (gpio_data & BUSYBIT) {
    usleep(10*1000);
    wrapgpioinst();
    repctr++;
    if (repctr > 1000000) {
      fprintf(stderr, "Timed out waiting for BUSY bit to clear.\n");
      ch341a_release();
      exit(2);
    }
  }
  /* Wait a short while longer (example code does that, probably for a reason) */
  usleep(10*1000);
}

void spisendbyte(uint8_t b) {
  for (int i = 0; i < 8; i++) {
    usleep(SLPDELAY);
    if (b & 0x80) {
      setbit(DATABIT);
    } else {
      clrbit(DATABIT);
    }
    wrapgpioinst();
    setbit(CLKBIT);
    wrapgpioinst();
    clrbit(CLKBIT);
    wrapgpioinst();
    usleep(SLPDELAY);
    b <<= 1;
  }
}

void epd_sendcmd(uint8_t cmd) {
  /* The DC pin needs to be low for commands. */
  clrbit(DCBIT);
  wrapgpioinst();
  /* Pull chip select low. */
  clrbit(CSBIT);
  wrapgpioinst();
  spisendbyte(cmd);
  /* Pull chip select high again. */
  setbit(CSBIT);
  wrapgpioinst();
}

void epd_senddata(uint8_t data) {
  /* The DC pin needs to be high for data. */
  setbit(DCBIT);
  wrapgpioinst();
  /* Pull chip select low. */
  clrbit(CSBIT);
  wrapgpioinst();
  spisendbyte(data);
  /* Pull chip select high again. */
  setbit(CSBIT);
  wrapgpioinst();
}

void epd_reset() {
  setbit(RSTBIT)
  wrapgpioinst();
  usleep(20 * 1000);
  clrbit(RSTBIT);
  wrapgpioinst();
  usleep(20 * 1000);
  setbit(RSTBIT)
  wrapgpioinst();
  usleep(20 * 1000);
}

/* "Set memory area" */
void epd_setmemoryarea(int x_start, int y_start, int x_end, int y_end) {
  epd_sendcmd(0x44);
  /* x point must be the multiple of 8 or the last 3 bits will be ignored */
  epd_senddata((x_start >> 3) & 0xFF);
  epd_senddata((x_end >> 3) & 0xFF);
  epd_sendcmd(0x45);
  epd_senddata(y_start & 0xFF);
  epd_senddata((y_start >> 8) & 0xFF);
  epd_senddata(y_end & 0xFF);
  epd_senddata((y_end >> 8) & 0xFF);
}

void epd_setmemorypointer(int x, int y) {
  epd_sendcmd(0x4E);
  /* x point must be the multiple of 8 or the last 3 bits will be ignored */
  epd_senddata((x >> 3) & 0xFF);
  epd_sendcmd(0x4F);
  epd_senddata(y & 0xFF);
  epd_senddata((y >> 8) & 0xFF);
  epd_waitfornotbusy();
}

/* put an image buffer to the frame memory.
 * this won't update the display (yet). */
void epd_setframememory(const unsigned char * image_buffer,
                        int x, int y,
                        int image_width, int image_height) {
  int x_end;
  int y_end;

  if ((image_buffer == NULL)
   || (x < 0) || (image_width < 0)
   || (y < 0) || (image_height < 0)) {
    return;
  }
  /* x point must be the multiple of 8 or the last 3 bits will be ignored */
  x &= 0xF8;
  image_width &= 0xFFF8;
  if (x + image_width >= EPDSIZEX) {
    x_end = EPDSIZEX - 1;
  } else {
    x_end = x + image_width - 1;
  }
  if (y + image_height >= EPDSIZEY) {
    y_end = EPDSIZEY - 1;
  } else {
    y_end = y + image_height - 1;
  }
  epd_setmemoryarea(x, y, x_end, y_end);
  epd_setmemorypointer(x, y);
  epd_sendcmd(0x24);
  /* send the image data */
  for (int j = 0; j < y_end - y + 1; j++) {
    for (int i = 0; i < (x_end - x + 1) / 8; i++) {
      epd_senddata(image_buffer[i + j * (image_width / 8)]);
    }
  }
}

/* This displays the frame that you sent before. */
void epd_displayframe(void) {
  epd_sendcmd(0x22);
  epd_senddata(0xc7);
  epd_sendcmd(0x20);
  epd_waitfornotbusy();
}

void epd_init(void) {
  /* This is basically C&P from the manufacturers example code.
   * The comments are C&P from there too, which is why they make little sense. */
  epd_reset();
  epd_waitfornotbusy();
  epd_sendcmd(0x12); // SWRESET
  epd_waitfornotbusy();
  epd_sendcmd(0x01); //Driver output control
  epd_senddata(0x27);
  epd_senddata(0x01);
  epd_senddata(0x00);

  epd_sendcmd(0x11); //data entry mode
  epd_senddata(0x03);

  epd_setmemoryarea(0, 0, (EPDSIZEX - 1), (EPDSIZEY - 1));

  epd_sendcmd(0x21); //  Display update control
  epd_senddata(0x00);
  epd_senddata(0x80);	

  epd_setmemorypointer(0, 0);
  epd_waitfornotbusy();

  /* "SetLut_by_host" */
  epd_sendcmd(0x32);
  for (int i = 0; i < 153; i++) {
    epd_senddata(WS_20_30[i]);
  }
  epd_waitfornotbusy();
  epd_sendcmd(0x3f);
  epd_senddata(WS_20_30[153]);
  epd_sendcmd(0x03);  // gate voltage
  epd_senddata(WS_20_30[154]);
  epd_sendcmd(0x04);  // source voltage
  epd_senddata(WS_20_30[155]); // VSH
  epd_senddata(WS_20_30[156]); // VSH2
  epd_senddata(WS_20_30[157]); // VSL
  epd_sendcmd(0x2c);  // VCOM
  epd_senddata(WS_20_30[158]);
  epd_waitfornotbusy();
}

int main() {
  int ret;

  ret = ch341a_configure(CH341A_USB_VENDOR, CH341A_USB_PRODUCT);
  if (ret < 0) return -1;

  ret = ch341a_gpio_configure(&gpio_dir_mask);
  if (ret < 0) return -1;

  /* Default state of pins:
   * bits for CS0, RESET, CLK, DATA, DC all set. */
  gpio_data = CSBIT | RSTBIT | CLKBIT | DATABIT | DCBIT;
  wrapgpioinst();

  /* Init the ePaper-display */
  epd_init();
  uint8_t dummyimg[EPDSIZEX * EPDSIZEY];
  for (int dip = 0; dip < (EPDSIZEX * EPDSIZEY); dip++) {
    dummyimg[dip] = 0xaa; //(dip % 2) ? 0xff : 0x00;
  }
  epd_setframememory(dummyimg, 0, 0, EPDSIZEX, EPDSIZEY);
  epd_displayframe();

  ret = ch341a_release();
  printf("\nreturn: %d\n", ret);
  return ret;
}

