/**
 * @file libpola.c
 * @author Benjamin Loos (loos.benjamin@gmail.com)
 * @version 1.0
 * 
 * This library replaces open and opendir functions to implement the POLA
 * principles
 * Copyright (C) 2010 Benjamin Loos
 * 
 * @section LICENSE
 * This file is part of the POLA library.

 * The POLA library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "libpola.h"

extern int errno;

/**
 * Compares two strings
 *
 * @param string1 First string
 * @param string2 Second string
 * @return Boolean value equaling true if strings are the same
 */
unsigned int compare_string (char* string1, char* string2)
{
    if (strlen(string1) == strlen(string2)
        && strstr(string1, string2) == string1)
        return 1;
    return 0;
}

/**
 * Strips < and > from beginning and end of string
 *
 * Arguments given to applications loading this library need to be
 * encapsuled in < and > if write access is allowed.
 * Those brackets need to be removed in order to reference real files.
 *
 * @param string String to strip from
 * @return String without < and >
 */
const char* strip_brackets (const char* string)
{
    // only if string is encapsuled by brackets
    if (string[0] != '<' || string[strlen(string)-1] != '>')
        return string;

    // allocate space
    char* res = malloc((strlen(string)-1)*sizeof(char));
    memset(res, '\0', strlen(string)-1);
    // copy string
    strncpy(res, string+1, strlen(string)-2);
    return res;
}


/**
 * Records a security breach in the auth.log system log.
 *
 * @param pathname Path of the requested file/directory
 */
void record_error (const char* pathname)
{
    syslog(LOG_NOTICE | LOG_AUTHPRIV,
           "User %s requested non-authorized access to %s\n",
           getenv("USERNAME"), pathname);
}


/**
 * Interactive replacement for open
 *
 * This function replaces the open() function from glibc. It asks before
 * giving access to the file, first.
 * Possible answers to the access requests are
 *    - Y/y to accept a request
 *    - N/n to deny a request
 *    - R/r to always accept a request
 *
 * @param pathname Path to the file to open
 * @param flags Flags to open the file with
 * @param libc_open Original open() from glibc
 * @return File descriptor of the file to open
 */
int my_open_interactive (const char* pathname, int flags,
                         int (*libc_open)(const char *name, int flags))
{
    unsigned int ask = 0; // boolean
    char a[16];
    char b;
    char mode;
    char* realp = realpath(pathname, NULL);

    // read only
    if ((flags & FLAGS) == O_RDONLY)
    {
		if (!accept_reads)
		{
			// ask for permission
			printf("> pola request: read access to %s? [N/y/r] ",
			       (realp == NULL ? pathname : realp));
			ask = 1;
			mode = 'r';
		}
    }

    // write only
    else if ((flags & FLAGS) == O_WRONLY)
    {
		if (!accept_writes)
		{
			// ask for permission
        	printf("> pola request: write access to %s? [N/y/r] ",
        	       (realp == NULL ? pathname : realp));
	        ask = 1;
	        mode = 'w';
		}
    }

    // read and write
    else if ((flags & FLAGS) == O_RDWR)
    {
		if (!accept_reads && !accept_writes)
		{
			// ask for permission
			printf("> pola request: read and write access to %s? [N/y/r] ",
			       (realp == NULL ? pathname : realp));
			ask = 1;
			mode = 'a';
		}
    }
    else
        printf("Trying to open %s with flags %d!\n",
               (realp == NULL ? pathname : realp), flags);

    free(realp);

    // if we have asked a question
    if (ask)
    {
        memset(a, '\0', 16);
        // get line from stdin
        fgets(a, 16, stdin);
        // get first character
        sscanf(a, "%c", &b);
		memset(a, '\0', 16);

        // accept all reads or writes
        if (b == 'r' || b == 'R')
        {
            switch (mode)
            {
                case 'r':
                    accept_reads = 1;
                    break;
                case 'w':
                    accept_writes = 1;
                    break;
                case 'a':
                    accept_writes = 1;
                    accept_reads = 1;
            }
        }

        // access denied
        if (b != 'y' && b != 'Y' && b != 'r' && b != 'R')
        {
	    // write to syslog
            record_error(pathname);
            // set errno so the calling application knows what happened
            errno = EACCES;
            // return with error
            return -1;
        }
    }

	// everything passed, now call the original open
    return libc_open(pathname, flags);
}

/**
 * Replacement for open implementing the POLA principle
 *
 * This function replaces the open() function from glibc.
 * It implements the following features:
 *    - Read requests are accepted if the file is given in the arguments
 *      (CMD_LINE environment var)
 *    - Write requests are accepted if the file is between brackets (< and >)
 * Refused access rights are logged in the system log.
 *
 * @param pathname Path to the file to open
 * @param flags Flags to open the file with
 * @param libc_open Original open() from glibc
 * @return File descriptor of the file to open
 */
int my_open (const char* pathname, int flags,
             int (*libc_open)(const char *name, int flags))
{
	// get arguments of calling application
    char* cmd = getenv("CMD_LINE");
    // get specifically allowed writes
    char* allowed_writes = getenv("ALLOWED_WRITES");

	// read only
    if ((flags & FLAGS) == O_RDONLY)
    {
        // check if filename is found in arguments
        if (strstr(cmd, pathname) != NULL)
        // call original open()
        return libc_open(pathname, flags);
    }
    // write access
    else if ((flags & FLAGS) == O_RDWR || (flags & FLAGS) == O_WRONLY)
    {
        // check for brackets
        if ((pathname[0] == '<' && pathname[strlen(pathname)-1] == '>')
            // or filename in arguments
            || (strstr(allowed_writes, pathname) != NULL))
        {
            if (pathname[0] == '<')
            {
                // strip brackets
                const char* file = strip_brackets(pathname);
                // call original open()
                return libc_open(file, flags);
            }
            // call original open()
            return libc_open(pathname, flags);
        }
    }

    // checks did not pass, record error to syslog, set errno accordingly
    // and return an error
    record_error(pathname);
    errno = EACCES;
    return -1;
}

/**
 * Replacement for glibc open() calling interactive or non-interactive variants
 *
 * @param pathname Path to the file to open
 * @param flags Flags to open the file with
 * @return File descriptor of the file to open
 */
int open (const char* pathname, int flags, ...)
{
    // back up original open()
    int (*libc_open)(const char *name, int flags);
    *(void **)(&libc_open) = dlsym(RTLD_NEXT, "open");

    // check if interactive mode was requested
    char* interactive = getenv("INTERACTIVE");
    if (*interactive == '1')
    {
        return my_open_interactive(pathname, flags, libc_open);
    }
    else
    {
        return my_open(pathname, flags, libc_open);
    }
}

/**
 * Interactive glibc opendir replacement
 *
 * This function replaces opendir from glibc, implementing a Y/n request
 * before opening the directory
 *
 * @param dirname Name of the directory
 * @param libc_opendir Original opendir from glibc
 * @return Pointer to struct dirent of directory
 */
DIR *my_opendir_interactive (const char* dirname,
                             DIR *(*libc_opendir)(const char *name))
{
    char a;
    char line[1024];
    printf("Trying to open directory %s! Allow? (Y/n) ", dirname);
    fgets(line, 1024, stdin);
    sscanf(line, "%c", &a);
    if (a == 'Y' || a == 'y')
        return libc_opendir(dirname);

    record_error(dirname);
    errno = EACCES;
    return NULL;
}

/**
 * glibc opendir replacement implementing the POLA principle
 *
 * This function only allows access to the directory if it is explicitly
 * mentioned in the arguments (CMD_LINE environment var)
 *
 * @param dirname Name of the directory
 * @param libc_opendir Original opendir from glibc
 * @return Pointer to struct dirent of directory
 */
DIR *my_opendir (const char* dirname, DIR *(*libc_opendir)(const char *name))
{
    char* cmd = getenv("CMD_LINE");

    if (strstr(cmd, dirname) != NULL)
    {
        return libc_opendir(dirname);
    }

    record_error(dirname);
    errno = EACCES;
    return NULL;
}

/**
 * glibc opendir replacement calling the interactive / non-interactive variants
 *
 * @param dirname Name of the directory
 * @return Pointer to struct dirent of directory
 */
DIR *opendir(const char *dirname)
{
	// back up original opendir()
    DIR *(*libc_opendir)(const char *name);
    *(void **)(&libc_opendir) = dlsym(RTLD_NEXT, "opendir");

    char* interactive = getenv("INTERACTIVE");
    if (*interactive == '1')
    {
        return my_opendir_interactive (dirname, libc_opendir);
    }
    else
    {
        return my_opendir (dirname, libc_opendir);
    }

}
