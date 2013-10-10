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

#define DEFAULT_INSULT "you idiot!"

typedef struct index_t {
    int    num_insults;
    size_t len_insults;
    off_t  offs[];
} __attribute__((packed)) index_t;

static char    buffer[4096];
static char    *(*orig_strerror)(int);
static char    *insults;
static index_t *idx;

static int memcnt(char *haystack, size_t len, char needle) {
    char *s = haystack;
    // Inspired by http://stackoverflow.com/a/4235884/1028600 - but without
    // warnings
    int i = 0;
    for (; s < haystack + len; s[i] == needle ? i++ : (intptr_t)(s++));
    return i;
}

static index_t *build_index(char *file, size_t len) {
    int num = memcnt(file, len, '\n');

    index_t *idx = malloc(sizeof(index_t) + (num+1) * sizeof(size_t));
    if (idx == NULL) {
        return NULL;
    }

    idx->len_insults = len;
    idx->num_insults = num;

    int i = 0;
    for (char *s = file; s < file + len; s++) {
        if (*s == '\n') {
            idx->offs[i++] = s - file + 1;
        }
    }
    return idx;
}

static char *get_primary_language(ssize_t *len) {
    char *l;
    #define test_env(str) do { \
        l = getenv(str); \
        if (l == NULL) { \
            break; \
        } \
        *len = strlen(l); \
        char *_ = memchr(l, '_', *len); \
        if (_ != NULL) { \
            *len = _ - l; \
        } \
        if (*len == 0) { \
            break; \
        } \
        return l; \
    } while (0)

    test_env("LC_ALL");
    test_env("LC_MESSAGES");
    test_env("LANG");

    #undef test_env

    *len = 1;
    return "C";
}

static char *get_lang(ssize_t *len) {
    char *p = getenv("LANGUAGE");
    if (p == NULL || strlen(p) == 0) {
        return get_primary_language(len);
    }
    *len = strlen(p);
    return p;
}

static char *memchrnul(char *s, char c, size_t len) {
    char *r = memchr(s, c, len);
    if (r == NULL) {
        return s + len;
    }
    return r;
}

static int open_db() {
    char *insultdir = getenv("INSULTERR_DIR");
    if (insultdir == NULL) {
        insultdir = "/usr/share/insulterr";
    }

    int dl = strlen(insultdir);
    if (insultdir[dl - 1] == '/') {
        dl--;
    }

    ssize_t ll;
    char *l = get_lang(&ll);

    char *buf = alloca(dl + ll + 6);
    memcpy(buf, insultdir, dl);
    buf[dl++] = '/';

    do {
        char *e = memchrnul(l, ':', ll);

        if (memcmp(l, "C", e - l > 2 ? e - l : 2)) {
            memcpy(buf + dl, l, e - l);
            memcpy(buf + dl + (e - l), ".txt", 5);
        } else {
            memcpy(buf + dl, "en", 2);
            memcpy(buf + dl + 2, ".txt", 5);
        }

        int fd = open(buf, O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            return fd;
        }

        ll -= (e - l) + 1;
        l = e + 1;
    } while (ll > 0);
    return -1;
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

    int fd = open_db();
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

    idx = build_index(insults, st.st_size);

    return;

close_file:
    close(fd);
default_insult:
    idx = NULL;
}

static char *get_insult(size_t *len) {
    if (idx == NULL) {
        *len = strlen(DEFAULT_INSULT);
        return DEFAULT_INSULT;
    }

    int max = RAND_MAX - (RAND_MAX % idx->num_insults);
    int n;
    do {
        n = rand();
    } while (n >= max);
    n = n % idx->num_insults;

    char *s = insults + idx->offs[n];
    *len = idx->offs[n+1]-idx->offs[n]-1;

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
