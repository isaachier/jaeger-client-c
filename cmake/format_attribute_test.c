#include <stdarg.h>
#include <stdio.h>

static void my_printf(const void* arg, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

static void my_printf(const void* arg, const char* format, ...)
{
    (void) arg;
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

int main(void)
{
    my_printf(NULL, "%s\n", "hello");
}
