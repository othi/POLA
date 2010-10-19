/**
 * @file pola-i.c
 * @author Benjamin Loos (loos.benjamin@gmail.com)
 * @version 1.0
 * 
 * pola-i starts a program with an interactive mode for requests
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

