#ifndef __PROGRAM__
#define __PROGRAM__

#include <stddef.h>

// constants
#define max_package_size (512 * 1024) // 512KB
#define children_count 3
#define npipes (children_count * 2)

// argument handling
const char *get_filename_in_args(int argc, const char *argv[]);

// file utils
const char *get_file_content(const char *filename, size_t *filesize);

// child processes
void count_lines_process(int readfd, int writefd);
void count_keywords_process(int readfd, int writefd);
void count_line_comments_process(int readfd, int writefd);

// pipe handling
void long_write(int fd, const void *data, size_t size);
void long_read(int fd, void **dest, size_t *size);
int make_pipe(int *fds);
void close_pipe(int fd);

#endif