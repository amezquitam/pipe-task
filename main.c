#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/wait.h>
#include "program.h"

int main(int argc, char const* argv[]) {

    /**
     * 0: root to child 1       1: child 1 to root
     * 2: root to child 2       3: child 2 to root
     * 4: root to child 3       5: child 3 to root
     */
    int pipes[npipes][2];
    for (int i = 0; i < npipes; i++)
        if (make_pipe(pipes[i]) == -1) {
            printf("[error while creating pipe[%d]]\n", i);
            return 1;
        }

    int i;
    for (i = 0; i < children_count; i++) {
        int childpid = fork();
        if (childpid == 0) break;
        if (childpid == -1) {
            printf("[error while fork\n]");
            return 1;
        }
    }

    if (i == children_count) {
        // close pipes
        for (int j = 0; j < npipes; j++) {
            int index = j % 2 == 0 ? 0 : 1;
            close_pipe(pipes[j][index]);
        }

        // reading file content
        size_t filesize;
        const char* filename = get_filename_in_args(argc, argv);
        const char* content = get_file_content(filename, &filesize);

        // sending file content to children 
        for (int j = 0; j < 3; j++) {
            long_write(pipes[j * 2][1], content, filesize);
        }

        // receiving results
        int line_count, keyword_count, comment_count;
        read(pipes[1][0], &line_count, sizeof(line_count));
        read(pipes[3][0], &keyword_count, sizeof(keyword_count));
        read(pipes[5][0], &comment_count, sizeof(comment_count));
        printf("line count: %d\n", line_count);
        printf("keyword count: %d\n", keyword_count);
        printf("comment count: %d\n", comment_count);

        // close pipes
        for (int j = 0; j < npipes; j++) {
            int index = j % 2 == 0 ? 1 : 0;
            close_pipe(pipes[j][index]);
        }

        for (int i = children_count; i--;)
            wait(NULL);
    }

    // close pipes unused by children
    if (i < 3) {
        for (int j = 0; j < npipes; j++) {
            if (j != 2 * i && j != 2 * i + 1) {
                close_pipe(pipes[j][1]);
                close_pipe(pipes[j][0]);
            }
        }
    }

    typedef void(*child_func_t)(int, int);

    // assosiate a function to each children by index
    child_func_t child_func[children_count] = {
        count_lines_process,
        count_keywords_process,
        count_line_comments_process
    };

    for (int j = 0; j < children_count; j++) {
        if (i != j) continue;
        int* read = pipes[j * 2];
        int* write = pipes[j * 2 + 1];
        close_pipe(read[1]);
        close_pipe(write[0]);
        child_func[j](read[0], write[1]);
        close_pipe(read[0]);
        close_pipe(write[1]);
    }

    return 0;
}
