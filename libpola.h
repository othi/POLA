#define _GNU_SOURCE

#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <dlfcn.h>
#include <sys/stat.h>

#define FLAGS (O_RDONLY | O_WRONLY | O_RDWR)

/* accept all read requests for this session */
unsigned int accept_reads = 0;

/* accept all write requests for this session */
unsigned int accept_writes = 0;


unsigned int compare_string (char* string1, char* string2);

const char* strip_brackets (const char* string);

void record_error (const char* pathname);

int my_open_interactive (const char* pathname, int flags,
                         int (*libc_open)(const char *name, int flags));

int my_open (const char* pathname, int flags,
             int (*libc_open)(const char *name, int flags));

DIR *my_opendir_interactive (const char* dirname,
                             DIR *(*libc_opendir)(const char *name));

DIR *my_opendir (const char* dirname, DIR *(*libc_opendir)(const char *name));

int open (const char* pathname, int flags, ...);

DIR* opendir (const char* dirname);
