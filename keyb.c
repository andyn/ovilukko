#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdint.h>

#include <glib.h>

#include "color.h"
#include "db.h"
#include "gpio.h"
#include "logger.h"


// Log file
#define LOG_FILE_PATH "/var/log/ovi.log"

// Debian's constants for nobody:nogroup
#define GID_NOGROUP 65534
#define UID_NOBODY 65534


/**
 * Read RFID number from input device at FD.
 * \param fd File descriptor of an opened input device.
 * \return RFID number as 64-bit unsigned integer, -1 on error or
 * 0 when no code is not yet available. */
gint64 read_rfid(int fd) {
	// Static variable to store received RFID value.
	static guint64 rfid = 0;

	while (1) {
		struct input_event ev;
		ssize_t n = read(fd, &ev, sizeof ev);
		if (n == (ssize_t)-1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EWOULDBLOCK) {
				return 0;
			} else { // I/O Error occurred
				return -1;
			}
		} else if (n != sizeof ev) {
			errno = EIO;
			return -1;
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
				guint64 retval = rfid;
				rfid = 0;
				return retval;
			}
			default:
				// Unknown code, abort reading
				rfid = 0;
				return 0;
			}
		}
	}
	return 0;
}



/** GPIO pin to use : pin18 = GPIO23 */
#define RELAY_PIN 18

/** Device name to use. */
//const char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
const char *dev = "/dev/input/by-id/usb-GASIA_PS2toUSB_Adapter-event-kbd";

/** RFID database file. */
const char *dbfile = "rfid.ini";


/**
 * Loop to read from keypad and test detected RFID validity.
 * \param db Authentication database. */
void rfid_loop(GHashTable *db, FILE *log_file) {

	// Open the input device device.
	int rfid_fd = open(dev, O_RDONLY|O_NONBLOCK);
	if (rfid_fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return;
	}

	// Get exclusive access.
	if (ioctl(rfid_fd,EVIOCGRAB,1) != 0) {
		fprintf(stderr,"Failed to get exclusive access to device\n");
	}

	while (1) {
		fd_set fdread;
		fd_set fderr;
		FD_ZERO(&fdread);
		FD_ZERO(&fderr);
		FD_SET(rfid_fd,&fdread);
		FD_SET(rfid_fd,&fderr);
//		struct timeval tv = { 5, 0 };
//		int rc = select(rfid_fd+1, &fdread, NULL, &fderr, &tv);
		int rc = select(rfid_fd+1, &fdread, NULL, &fderr, NULL);

		if (rc < 0) {
			fprintf(stderr,"Select() failed (%d)\n",rc);
			break;
		}
		if (rc == 0) {
			continue;
		}
		if (FD_ISSET(rfid_fd,&fderr)) {
			fprintf(stderr,"I/O error encountered\n");
			break;
		}
		if (FD_ISSET(rfid_fd,&fdread)) {
			gint64 rfid = read_rfid(rfid_fd);
			// If error encountered, break out.
			if (rfid < 0) {
				fprintf(stderr,"RFID read resulted I/O error\n");
				break;
			}
			// If code not yet ready, continue.
			if (rfid == 0) continue;
			if (db_check_rfid(db,rfid) == RES_PASS) {
				struct dbentry *entry = db_get_entry(db, rfid);
				assert(entry);
				flogger(log_file, "Accepted ID %llu (%s, %s)\n", rfid, entry->full_name, entry->nickname);
				logger(COLOR_GREEN "Accepted ID %llu (%s, %s)\n" COLOR_OFF, rfid, entry->full_name, entry->nickname);
				GPIOWrite(RELAY_PIN,HIGH);
				sleep(5);
				GPIOWrite(RELAY_PIN,LOW);
			} else {
				flogger(log_file, "Unknown ID %llu\n", rfid);
				logger(COLOR_RED "Unknown ID %llu\n" COLOR_OFF, rfid);
			}
		}
	}

	close(rfid_fd);
}


int main(void)
{
	fprintf(stderr, COLOR_UNDERLINE "Elepaja door lock (built on %s %s)\n" COLOR_OFF, __DATE__, __TIME__);
	// Load database.
	GHashTable *db = db_init();
	if (db_read(db,dbfile)) {
		fprintf(stderr,"Failed to read db\n");
		return EXIT_FAILURE;
	}

	// Setup GPIO pins.
	GPIOExport(RELAY_PIN);
	GPIODirection(RELAY_PIN,OUT);
	GPIOWrite(RELAY_PIN,LOW);

	// Open log file for writing
	FILE *log_file = fopen(LOG_FILE_PATH, "a+");
	if (!log_file) {
		fprintf(stderr, "Failed to open log file '%s' for appending\n", LOG_FILE_PATH);
		return EXIT_FAILURE;
	}
	flogger(log_file, "Started (build %s %s)\n", __DATE__, __TIME__);
	
	/*
	// Drop root privileges
	if (getuid() == 0) {
		if (setgid(GID_NOGROUP) != 0) {
			return EXIT_FAILURE;
			fprintf(stderr, "ERROR: Could not drop root group privileges\n");
		}
		if (setuid(UID_NOBODY) != 0) {
			fprintf(stderr, "ERROR: Could not drop root user privileges\n");
			return EXIT_FAILURE;
		}
	}
	*/
	
	while (1) {
		printf("Starting RFID monitoring loop\n");
		rfid_loop(db, log_file);
		sleep(1);
	}

	GPIOWrite(RELAY_PIN,LOW);
	g_hash_table_unref(db);
	return EXIT_FAILURE;
}
