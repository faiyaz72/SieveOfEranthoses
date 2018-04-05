#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "filter.h"

int filter(int m, int read_fd, int write_fd) {

	int num;

	while ((read(read_fd, &num, sizeof(int))) > 0) {
		if (num % m != 0) {
			if ((write(write_fd, &num, sizeof(int))) == -1) {
				perror("write");
				exit(1);
			}
		} 
	}

	return 0;
}