#include "program.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>


const char* get_filename_in_args(int argc, const char* argv[]) {
    if (argc == 1) {
        printf("You need to pass a file name as an argument\n");
        exit(1);
    }
    return argv[1];
}

const char* get_file_content(const char* filename, size_t* filesize) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("[error, no such file]: %s\n", filename);
        exit(1);
    }
    struct stat st;
    if (stat(filename, &st) == -1) {
        printf("[error while reading file info]\n");
        exit(1);
    }

    *filesize = st.st_size;

    const char* content = malloc(*filesize);
    fread((void*)content, *filesize, 1, file);
    fclose(file);

    return content;
}

const char* extract_file(int fd, size_t* size) {
    char *content;
    long_read(fd, (void**) &content, size);
    return content;
}

void count_lines_process(int readfd, int writefd) {
    size_t filesize;
    const char* file_content = extract_file(readfd, &filesize);

    int line_count = 0;
    int is_empty = 1;
    for (const char* c = file_content; 1; c++) {
        if ((*c == '\n' || *c == 0) && !is_empty) {
            line_count++;
            is_empty = 1;
        }
        if (*c != '\n') is_empty = 0;
        if (*c == 0) break;
    }

    write(writefd, &line_count, sizeof(line_count));
    free((void*)file_content);
}

int is_space(char c) {
    const char spaces[] = { ' ', '\t', '\n' };
    int const spclen = sizeof(spaces) / sizeof(spaces[0]);
    for (int i = 0; i < spclen; i++) {
        if (spaces[i] == c) {
            return 1;
        }
    }
    return 0;
}

int is_symbol(char c) {
    const char symbols[] = { '(', ')', '{', '}', '.', ',', ';', ':', '+', '-', '=', '!', '*', '&', '|', '/', '<', '>', '"', '\'' };
    int const symblen = sizeof(symbols) / sizeof(symbols[0]);
    for (int i = 0; i < symblen; i++) {
        if (symbols[i] == c) {
            return 1;
        }
    }
    return 0;
}

int is_keyword(char const* word) {
    const char* keywords[] = { "int", "float", "char", "const", "void", "double", "printf", "scanf", "for", "while", "return" };
    int const keywlen = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < keywlen; i++) {
        char tmp[64];
        int len = strlen(keywords[i]);
        memcpy(tmp, word, len);
        tmp[len] = 0;
        if (strcmp(keywords[i], tmp) == 0) {
            return len;
        }
    }
    return 0;
}

void count_keywords_process(int readfd, int writefd) {
    size_t filesize;
    const char* file_content = extract_file(readfd, &filesize);

    int keywordcount = 0;

    for (const char* c = file_content; *c != 0; c++) {
        int len;
        if (is_space(*c)) continue;
        if (is_symbol(*c)) continue;
        if ((len = is_keyword(c)) > 0) {
            keywordcount++;
            c += len - 1;
        }
    }

    write(writefd, &keywordcount, sizeof(keywordcount));
    free((void*)file_content);
}

void count_line_comments_process(int readfd, int writefd) {
    size_t filesize;
    const char* file_content = extract_file(readfd, &filesize);

    int linecommentcount = 0;

    for (const char* c = file_content; c[1] != 0; c++) {
        if (c[0] == '/' && c[1] == '/') {
            linecommentcount++;
        }
    }

    write(writefd, &linecommentcount, sizeof(linecommentcount));
    free((void*)file_content);
}

void long_write(int fd, const void* data, size_t size) {
    int remaining_content = size;
    while (remaining_content > 0) {
        int tosend = remaining_content > max_package_size ? max_package_size : remaining_content;
        remaining_content -= tosend;

        if (write(fd, &tosend, sizeof(tosend)) == -1)
            printf("[error while writing in pipe]\n");
        if (write(fd, data, tosend) == -1)
            printf("[error while writing in pipe]\n");
        int eof = remaining_content == 0;
        if (write(fd, &eof, sizeof(eof)) == -1)
            printf("[error while writing in pipe]\n");
        
        data += tosend;
    }
}

void long_read(int fd, void** dest, size_t* size) {
    int content_size = 0;
    char* content = NULL;

    int eof;
    do {
        char* package;
        int size;
        if (read(fd, &size, sizeof(size)) == -1) {
            printf("[error while reading pipe] at process %d\n", getpid());
        }

        package = malloc(size);

        if (read(fd, package, size) == -1) {
            printf("[error while reading pipe] at process %d\n", getpid());
        }
        if (read(fd, &eof, sizeof(eof)) == -1) {
            printf("[error while reading pipe] at process %d\n", getpid());
        }

        char* new_content = malloc(content_size + size);
        memcpy(new_content, content, content_size);
        memcpy(new_content + content_size, package, size);

        if (content)
            free(content);
        free(package);

        content_size += size;
        content = new_content;
    } while (!eof);

    if (size) *size = content_size;
    *dest = content;
}

int make_pipe(int* fds) {
    int st = pipe(fds);
    printf("opening pipes (%d, %d)\n", fds[0], fds[1]);
    return st;
}

void close_pipe(int fd) {
    close(fd);
    printf("closing pipe (%d)\n", fd);
}
