#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <signal.h>
#include "filter.h"


/* Wrapper function to close pipe file descriptor
 * and error checking
*/
void Close(int fd) {

	if (close(fd) != 0) {
		perror("close");
		exit(1);
	}
}

/* Wrapper function to error check pipe
*/
void Pipe(int* toPipe) {
	
	if (pipe(toPipe) == -1) {
		perror("pipe");
		exit(1);
	}
}

/* Simple helper function to determine if a given number is a Prime number
 * or not.
 * Returns 0 if it isn't a Prime or Returns 1 if it is a Prime
 */
int isPrime(int n) {

	int result = 1;

	for (int i = 2; i<n; i++) {
		if (n % i == 0) {
			result = 0;
			return result;
		}
	}

	return result;
}

/* Helper function to determine whether given two factors, whether data is a product
 * of primes. 
*/
int noProduct(int factorsList1, int factorsList2, int data) {

	double limit = sqrt(data);

	if (factorsList2 < limit) {
		printf("%d is not the product of two primes\n", data);
		return 0;
	}

	return 1;
}

/* Helper funtion to determine given a single factor, whether data is not the 
 * product of two primes or finds the second number where the product is equal 
 * to data
*/
int oneFactor(int factorsList1, int data) {

	int temp = data / factorsList1;

	int check = isPrime(temp);

	if (check == 1) { 		//check is prime;
		printf("%d %d %d\n", data, factorsList1, temp);
		return 0;

	} else {
		printf("%d is not the product of two primes\n", data);
		return 1;
	}
}

/* Helper function to determine the approriate statement to print
*/
void determinePrint(int* factorsList, int data) {
	// When the two factors found are equal to data
	if (factorsList[0] * factorsList[1] == data) {
		printf("%d %d %d\n", data, factorsList[0], factorsList[1]);

	// When no factors are found
	} else if (factorsList[0] == 0 && factorsList[1] == 0) {
		printf("%d is prime\n", data);

	// When the product of factors are not equal to data
	} else if(factorsList[0] != 0 && factorsList[1] != 0) {
		noProduct(factorsList[0], factorsList[1], data);

	// When only 1 factor is found
	} else if(factorsList[0] != 0 && factorsList[1] == 0) {
		oneFactor(factorsList[0], data);
	}

}

int main(int argc, char** argv) {


    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
	    perror("signal");
	    exit(1);
	}
	//checking arguments and returns 1 if incorrent 
	if (argc != 2){
		fprintf(stderr, "Usage:\n\tpfact n\n");
		return 1;
	}

	int data = strtol(argv[1], NULL, 10);

	if (data < 1) {
		fprintf(stderr, "Usage:\n\tpfact n\n");
		return 1;
	}

	// declaring a 2D array of pipes, as the total number of filters are unknown
	// an upper limit of data (number inputted) is set
	int cur[data][2];

	// Another piper to hold factors found during filter process
	int resultPipe[2];

	Pipe(resultPipe);

	//First pipe will hold number 2 - data
	Pipe(cur[0]);
	int k = 2;

	while (k != data + 1) {
		write(cur[0][1], &k, sizeof(int));
		k++;
	}


	int m = 2;

	// Variable that will hold the number of factors found
	int factorCount = 0;

	double limit = sqrt(data);

	int mast = fork();
	if (mast == -1) {
		perror("fork");
		exit(1);
	}

	int count;

	int value = 0;

	if(mast == 0) {
		for(count = 1; count <data; count++) {
			//This pipe will have the filtered values from the process
			Pipe(cur[count]);

			int child = fork();
			if (child == -1) {
				perror("fork");
				exit(1);
			}

			if (child == 0) {

				read(cur[count-1][0], &m, sizeof(int));

				if (data % m == 0) {

					factorCount++;

					if (write(resultPipe[1], &m, sizeof(int)) == -1) {
						perror("count not write to resultPipe\n");
					}
				}

				if (m >= limit || factorCount == 2) {
					exit(1);
				}

				Close(cur[count-1][1]);

				filter(m, cur[count-1][0], cur[count][1]);
			}

			if (child > 0) {

				Close(cur[count][1]);

				Close(cur[count-1][1]);

				int status_2;

				if ((wait(&status_2)) == -1) {
				  perror("wait");
				  exit(1);
				}

				//Process terminates with 1 + number of filters that follow 
				if (WIFEXITED(status_2)) {
					value = 1 + WEXITSTATUS(status_2);
					exit(value); 
				}
			}
		}

	} if (mast > 0) {

		//Closes the first pipe that was made in the beginning.
		Close(cur[0][1]);

	    int status;

	    if ((wait(&status)) == -1) {
		  perror("wait");
		  exit(1);
		}

		// Collects the total number of filters from first child
		int returnVal;
		if (WIFEXITED(status)) {
			returnVal = WEXITSTATUS(status) - 2;
		}

		Close(resultPipe[1]);

		// Setting up an array to store my factors, initially setting values
		// to 0
		int factorsList[2];
		factorsList[0] = 0;
		factorsList[1] = 0;

		int factorIndex = 0;

		// Put all the factors found in an array that will hold atmost 2 ints
		while ((read(resultPipe[0], &factorsList[factorIndex], sizeof(int))) > 0) {
			factorIndex++;
		}

		// If checks to determine prime, factors etc
		// Special case for then data = 2, return prime

		if (data == 2) {
			printf("%d is prime\n", data);
			printf("Number of filters = %d\n", returnVal);
			return 0;
		}

		// Calls helper method to print appropriate message
		determinePrint(factorsList, data);

		printf("Number of filters = %d\n", returnVal);
		return 0;

	}
}