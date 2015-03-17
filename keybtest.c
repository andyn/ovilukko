#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdint.h>

int main(void)
{
//	const char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
	const char *dev = "/dev/input/by-id/usb-GASIA_PS2toUSB_Adapter-event-kbd";
	int fd = open(dev, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	uint64_t rfid = 0;
	while (1) {
		struct input_event ev;
		ssize_t n = read(fd, &ev, sizeof ev);
		if (n == (ssize_t)-1) {
			if (errno == EINTR)
				continue;
			else
				break;
		} else
		if (n != sizeof ev) {
			errno = EIO;
			break;
		}
		if (ev.type == EV_KEY && ev.value == 0) {
			switch (ev.code) {
			case KEY_1: rfid = rfid*10 + 1; break;
			case KEY_2: rfid = rfid*10 + 2; break;
			case KEY_3: rfid = rfid*10 + 3; break;
			case KEY_4: rfid = rfid*10 + 4; break;
			case KEY_5: rfid = rfid*10 + 5; break;
			case KEY_6: rfid = rfid*10 + 6; break;
			case KEY_7: rfid = rfid*10 + 7; break;
			case KEY_8: rfid = rfid*10 + 8; break;
			case KEY_9: rfid = rfid*10 + 9; break;
			case KEY_0: rfid = rfid*10 + 0; break;
			case KEY_ENTER: {
				printf("%llu\n",rfid);
				rfid = 0;
				break;
			}
			default: break;
			}
		}

	}
	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}
