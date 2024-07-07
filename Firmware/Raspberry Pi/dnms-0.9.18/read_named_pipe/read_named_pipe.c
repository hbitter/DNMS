
/*   *****  read named pipe example ***** 
 *
 *   source of original program is: stackoverflow: Linux timeout for read() on named pipe
 *   https://stackoverflow.com/questions/60850286/linux-timeout-for-read-on-named-pipe
 * 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define name_of_pipe "/home/helmut/dnms/dnms_pipe"
#define timeout_duration 10  // in seconds

int fd_named_pipe;
char buffer[512];



int main(void) {
  while (1) {
    fd_named_pipe = open (name_of_pipe, O_RDONLY | O_NONBLOCK);
    if (fd_named_pipe < 0) {
      perror("open");
      return EXIT_FAILURE;
    }

    struct pollfd waiter = {.fd = fd_named_pipe, .events = POLLIN};

    while (1) {
      //  timeout after timeout_duration in seconds
      switch (poll(&waiter, 1, timeout_duration * 1000)) {
      case 0:
        puts("The named pipe (fifo) timed out ");
        break;
      case 1:
        if (waiter.revents & POLLIN) {

          ssize_t len = read(fd_named_pipe, buffer, sizeof buffer - 1);
          if (len < 0) {
            perror("read");
            return EXIT_FAILURE;
          }
          buffer[len] = '\0';
          printf("%s", buffer);
        } else if (waiter.revents & POLLERR) {
          puts("Got a POLLERR");
          return EXIT_FAILURE;
        } else if (waiter.revents & POLLHUP) {
          // Writer closed its end
          goto closed;
        }
        break;
      default:
        perror("poll");
        return EXIT_FAILURE;
      }
    }
  closed:
    if (close(fd_named_pipe) < 0) {
      perror("close");
      return EXIT_FAILURE;
    }
  }
}
