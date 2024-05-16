#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "esh.h"
#include "exec.h"

static void mainloop()
{
    // room for null-term, and to allow for overflow
    char cmdline[MAX_CMDLINE_LEN + 2];
    char c;
    int i;

    while (1) {
        // Reset
        i = 0;
        c = 0;
        cmdline[0] = 0;

        // Prompt
        putc('$', stderr);
        putc(' ', stderr);
        while ((c = fgetc(stdin))) {
            if (c == EOF) {
                exit(0);
            }
            cmdline[i++] = c;
            if (i > MAX_CMDLINE_LEN) {
                fprintf(stderr, "Too many characters\n");
                i = 0;
                c = 0;
                cmdline[0] = 0;
                break;
            }
            if (c == '\n' || c == EOF)
                break;
        }
        if (i < 2)
            continue;

        // Execute
        exec_cmdline(cmdline);
    }
}

int main(int argc, char **argv)
{
    int pid;
    int res;
    pid = fork();
    if (pid < 0) {
        return pid;
    } else if (pid == 0) {
        mainloop();
    } else {
        waitpid(pid, &res, 0);
    }
    return res;
}
