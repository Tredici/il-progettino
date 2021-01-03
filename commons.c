#include "commons.h"
#include <sys/uio.h>
#include <stdio.h>
#include <stdarg.h> /* per un numero variabile di argomenti */
#include <stdlib.h>
#include <unistd.h>

#define MAX_ERR_L 256

void errExit(const char *format, ...)
{
    va_list args;

    fflush(stdout);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

