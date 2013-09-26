#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <error.h>
#include <stdint.h>
#include <alloca.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

static char   buffer[4096];
static char   *(*orig_strerror)(int);
static char   *insults;
static int    num_insults;
static size_t len_insults;

int memcnt(char *haystack, size_t len, char needle) {
    char *s = haystack;
    // Inspired by http://stackoverflow.com/a/4235884/1028600 - but without
    // warnings
    int i = 0;
    for (; s < haystack + len; s[i] == needle ? i++ : (intptr_t)(s++));
    return i;
}

void init() {
    orig_strerror = dlsym(RTLD_NEXT, "strerror");

    srand(time(NULL));

    char *insultdir = getenv("INSULTERR_DIR");
    if (insultdir == NULL) {
        insultdir = "/usr/share/insulterr";
    }

    int len = strlen(insultdir);
    char *buf = alloca(len + 8);
    strncpy(buf, insultdir, len + 8);
    strncpy(buf + len, "/en.txt", 8);
    buf[len + 7] = '\0';

    int fd = open(buf, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        error(0, errno, "Could not open insults from %s", buf);
        goto default_insult;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        error(0, errno, "Could not stat insults file");
        goto close_file;
    }

    insults = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (insults == MAP_FAILED) {
        error(0, errno, "Could not mmap insults file");
        goto close_file;
    }

    num_insults = memcnt(insults, st.st_size, '\n');
    len_insults = st.st_size;

    return;

close_file:
    close(fd);
default_insult:
    insults = "you idiot!\n";
    num_insults = 1;
    len_insults = strlen(insults);
}

char *get_insult(size_t *len) {
    int max = RAND_MAX - (RAND_MAX % num_insults);
    int n;
    do {
        n = rand();
    } while (n >= max);
    n = n % num_insults;

    char *s = insults;
    for (int i = 0; i < n; i++) {
        while (*s != '\n') s++;
        s++;
    }

    char *e = strchr(s, '\n');
    *len = e-s;
    return s;
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

    size_t len;
    char *insult = get_insult(&len);
    fprintf(stderr, ", %.*s\n", (int)len, insult);

    if (status != 0) {
        exit(status);
    }
}

void perror(const char *s) {
    size_t len;
    char *insult = get_insult(&len);
    fprintf(stderr, "%s: %s, %.*s\n", s, orig_strerror(errno), (int)len, insult);
}

char *strerror(int errnum) {
    char *emsg = orig_strerror(errnum);
    size_t len;
    char *insult = get_insult(&len);
    snprintf(buffer, 4096, "%s, %.*s", emsg, (int)len, insult);
    return buffer;
}
