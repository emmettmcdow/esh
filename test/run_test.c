#include <stdio.h>
#include <string.h>

#include "../src/common.h"
#include "../src/exec.h"
#include "run_test.h"

// TODO: fix this
int test_full(char *cmdline, char *exp_stdout, char *exp_stderr, int exp_res)
{
    int cmd_res, res;
    char *cmd_stdout, *cmd_stderr;

    cmd_res = 0;
    cmd_stdout = 0;
    cmd_stderr = 0;

    if (cmd_res != exp_res) {
        printf("Result %d =/= %d", cmd_res, exp_res);
        res = 1;
    }
    if (strcmp(cmd_stdout, exp_stdout)) {
        printf("Stdout %s =/= %s", cmd_stdout, exp_stdout);
        res = 1;
    }
    if (strcmp(cmd_stderr, exp_stderr)) {
        printf("Stderr %s =/= %s", cmd_stderr, exp_stderr);
        res = 1;
    }
    return res;
}

int test_tokenize(char *cmdline, char **exp_args, int exp_nargs)
{
    int res;
    int tokens;
    int i;
    char mutable_cmdline[MAX_CMDLINE_LEN + 1];
    char *args[MAX_ARG_N];
    // char *curr_arg;
    res = 0;
    if (cmdline) {
        strncpy(mutable_cmdline, cmdline, MAX_CMDLINE_LEN + 1);
        tokens = tokenize_cmdline(mutable_cmdline, args);
    } else {
        tokens = tokenize_cmdline(0, args);
    }

    if (tokens != exp_nargs) {
        res = 1;
        printf("\n        Expected token_n: %d, got: %d\n", exp_nargs, tokens);
        return res;
    }

    if (cmdline) {
        for (i = 0; i < tokens; i++) {
            if (strcmp(args[i], exp_args[i]))
                res = 1;
        }
    } else {
        res = !(exp_args);
    }
    if (res) {
        if (cmdline) {
            printf("\n        Expected [");
            for (i = 0; i < exp_nargs; i++) {
                printf("\"%s\",", exp_args[i]);
            }
            printf("]\n        Got: [");

            for (i = 0; i < tokens; i++) {
                printf("\"%s\",", args[i]);
            }
            printf("]\n");
        } else {
            printf("        Expected null, got non-null\n");
        }
    }

    return res;
}
