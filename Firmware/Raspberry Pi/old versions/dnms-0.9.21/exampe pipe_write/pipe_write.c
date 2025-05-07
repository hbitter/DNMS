// test für fifo - etwas senden

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define buffer_size 256

int main (void) {

	char buffer[buffer_size];
	int pipe_fd;
	int loop_number = 0;
	int i;

	umask(0);
	if (mkfifo ("/tmp/start_stop_dnms", O_RDWR | 0666)  < 0) {
		/* pipe bereits vorhanden - ist ok */
		if (errno == EEXIST) {
			printf("pipe vorhanden ok\n");
		} else {
			perror("mkfifo()");
			exit (EXIT_FAILURE);
		}
	}
	pipe_fd = open ("/tmp/start_stop_dnms", O_RDWR);
	if (pipe_fd == -1) {
		perror ("open()");
		exit (EXIT_FAILURE);		
	}
	
	while(1) {  // wait for message via named pipe in endless loop
		loop_number++;
		write (pipe_fd, "1", 1);		
		printf("write  #: %d,   1  in pipe geschrieben\n", loop_number);
		sleep(20);
		loop_number++;	
		write (pipe_fd, "0", 1);		
		printf("write  #: %d,   0  in pipe geschrieben\n", loop_number);
		sleep(20);		
	}
	
}
