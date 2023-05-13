CFLAGS=-std=gnu99 -Wall
epntpr: main.c ch341a.c ch341a.h epaper.c epaper.h
	gcc $(CFLAGS) ch341a.c epaper.c main.c -o epntpr -lusb-1.0 -lgd
clean:
	rm *.o ch341 epntpr -f
install-udev-rule:
	cp 99-ch341a.rules /etc/udev/rules.d/
	udevadm control --reload-rules
.PHONY: clean install-udev-rule
