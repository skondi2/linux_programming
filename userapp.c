#include "userapp.h"
#include <stdio.h>
#include <unistd.h>

int fibonacci(int n) {
	if (n == 0 || n == 1) return n;
	return fibonacci(n - 2)  + fibonacci(n - 1);
}

int main(int argc, char* argv[])
{
	// write to /proc/mp1/status
	FILE* ptr = fopen("/proc/mp1/status", "w");
	if (ptr == NULL) {
		printf("Trying to write, failed to open proc file\n");
		return 1;
	}
	int pid = getpid();
	fprintf(ptr, "%d", pid);
	fclose(ptr);

	// fibonacci sequence for large number
	fibonacci(100);

	// read from /proc/mp1/status
	ptr = fopen("/proc/mp1/status", "r");
	if (ptr == NULL) {
		printf("Trying to read, failed to open proc file\n");
		return 1;
	}
	char ch;
	while (!feof(ptr)) {
		ch = fgetc(ptr);
		printf("%c", ch);
	}
	fclose(ptr);
	
	return 0;
}
