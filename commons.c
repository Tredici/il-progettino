#include "commons.h"

void errExit(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf("\n");
    vfprintf(stderr, format, args);
    fprintf("\n");
    va_end(args);

    exit(EXIT_FAILURE);
}

