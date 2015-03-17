#ifndef LOG_H_
#define LOG_H_

/*
 * Like printf, but prints out a timestamp and a space before the actual string
 */

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define ENOUGH 128
#define LOGGER_TIMESTAMP_FORMAT "(%F %T %z) "

int logger(char const *format, ...) {
	va_list args;
	va_start(args, format);
	int ret = vflogger(stdout, format, args);
	va_end(args);
	return ret;
}

int flogger(FILE *stream, char const *format, ...) {
	va_list args;
	va_start(args, format);
	int ret = vflogger(stream, format, args);
	va_end(args);
	return ret;
}

int vflogger(FILE *stream, char const *format, va_list args) {
	// Print the timestamp
	time_t now = time(NULL);
	struct tm *now_tm = localtime(&now);
	char time_buffer[ENOUGH];
	strftime(time_buffer, sizeof time_buffer, LOGGER_TIMESTAMP_FORMAT, now_tm);
	// The man page of strftime(3) is rather vague when it comes to
	// buffer maxlen and the terminating null, so just be sure.
	time_buffer[sizeof time_buffer - 1] = '\0';
	fprintf(stream, "%s", time_buffer);

	// Print the actual line
	int ret = vfprintf(stream, format, args);
	// Ensure that the logged line gets written to disk
	fflush(stream);

	return ret;
}

#endif // LOG_H_

