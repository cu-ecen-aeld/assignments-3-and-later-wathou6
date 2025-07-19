#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

int main( int argc, char *argv[] ) {
	if( argc != 3 ) {
		fprintf(stderr, "Error: parameters were not specified\n");
		return EXIT_FAILURE;
	}

	const char *writefile = argv[1];
	const char *writestr = argv[2];

	openlog( argv[0], LOG_PID | LOG_CONS, LOG_USER );

	FILE *fptr;
	fptr = fopen(writefile, "w");
	if( fptr == NULL ) {
		syslog( LOG_ERR, "Error open '%s': %s", writefile, strerror(errno)) ;
		closelog();
		return EXIT_FAILURE;
	}

	uint32_t search_len = strlen(writestr);
	if( fwrite( writestr, 1, search_len, fptr ) != search_len ) {
		syslog( LOG_ERR, "Error writing to '%s': %s", writefile, strerror(errno) );
		fclose(fptr);
		closelog();
		return EXIT_FAILURE;
	}

	if( fclose(fptr) == EOF ) {
		syslog( LOG_ERR, "Error closing '%s': %s", writefile, strerror(errno) );
		closelog();
		return EXIT_FAILURE;
	}

	syslog( LOG_DEBUG, "Writing '%s' to '%s'", writestr, writefile );
	closelog();
	return EXIT_SUCCESS;
}
