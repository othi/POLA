#include "pola-i.h"

int main (int argc, char** argv)
{
	// set LD_PRELOAD to replace open() and opendir() calls
    setenv("LD_PRELOAD", "./libpola.so", 1);

	// tell library to activate interactive mode
    setenv("INTERACTIVE", "1", 1);

    // Usage
    if (argc < 2)
    {
        printf("Usage: %s <command> [arg1] [arg2] ..", argv[0]);
        exit(1);
    }

    int i;
    // remove pola-i programm name from argv to prepare for execv
    for (i=1; i<argc; i++)
        argv[i-1] = argv[i];
    argv[i-1] = NULL;

	// execute command
    execvp(argv[0], argv);

    // execvp has failed for some reason
    perror("Error executing file: ");

    return EXIT_FAILURE;
}