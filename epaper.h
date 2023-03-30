
#ifndef _EPAPER_H_
#define _EPAPER_H_

/* global variables declared in main.c */
extern uint8_t gpio_dir_mask;
extern uint8_t gpio_data;

/* resolution of our epaper display */
#define EPDSIZEX 128
#define EPDSIZEY 296

/* Initializes the ePaper display (including initializing the GPIO-pins) */
void epd_init(void);
/* put an image buffer to the frame memory.
 * this won't update the display (yet). */
void epd_setframememory(const unsigned char * image_buffer,
                        int x, int y,
                        int image_width, int image_height);
/* This displays the frame that you sent to the display before
 * (via epd_setframememory). */
void epd_displayframe(void);

#endif /* _EPAPER_H_ */

