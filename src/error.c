// error.c

#include <stdio.h>
#include <stdarg.h>
#include "error.h"

#define COLOR_RED "\x1b[31m"
#define RESET_COLOR "\x1b[0m"

void error_report(int line, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, COLOR_RED "Error [line %d]: ", line);
    vfprintf(stderr, format, args);
    fprintf(stderr, RESET_COLOR);

    va_end(args);
}
