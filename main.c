#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <error.h>
#include <stdint.h>

static char buffer[4096];
static char *(*orig_strerror)(int);

void init() {
    orig_strerror = dlsym(RTLD_NEXT, "strerror");
}

void error(int status, int errnum, const char *format, ...) {
    static void (*orig_error)(int, int, const char *);
    if (orig_error == NULL) {
        orig_error = dlsym(RTLD_NEXT, "strerror");
    }

    fflush(stdout);

    fprintf(stderr, "%s: ", program_invocation_name);
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    if (errnum != 0) {
        fprintf(stderr, ": %s", orig_strerror(errnum));
    }

    fprintf(stderr, ", you idiot!\n");

    if (status != 0) {
        exit(status);
    }
}

void perror(const char *s) {
    fprintf(stderr, "%s: %s, you idiot!\n", s, orig_strerror(errno));
}

char *strerror(int errnum) {
    char *emsg = orig_strerror(errnum);
    snprintf(buffer, 4096, "%s, you idiot!", emsg);
    return buffer;
}
