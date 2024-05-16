#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "exec.h"

int exec_cmdline(char *cmdline)
{
    int cmdline_n;
    int tokens;
    char *args[MAX_ARG_N];
    int pid;
    int res;

    if (!cmdline)
        return -EINVAL;

    cmdline_n = strlen(cmdline);
    if (cmdline_n > MAX_CMDLINE_LEN)
        return -EINVAL;

    // Do nothing if newline only
    if (cmdline_n == 1)
        return 0;

    tokens = tokenize_cmdline(cmdline, args);
    if (tokens < 0) {
        return tokens;
    }
    // Zero out the last one, in case there is leftover data
    if (tokens != MAX_ARG_N)
        args[tokens] = 0;

    if (!strcmp("cd", args[0])) {
        res = esh_change_directory(args, tokens);
        if (res) {
            fprintf(stderr, "Command returned %d\n", res);
        }
    } else if (!strcmp("exit", args[0]) || (args[0][0] == EOF)) {
        esh_exit(args, tokens);
    } else {
        pid = fork();
        if (pid < 0) {
            // fork failed
            return pid;
        } else if (pid == 0) {
            // child
            if (tokens > 1)
                execvp(args[0], args);
            else
                execvp(args[0], 0);
            // Something went wrong
            fprintf(stderr, "Exec failed\n");
            exit(errno);
        } else {
            // parent
            waitpid(pid, &res, 0);
            if (res) {
                fprintf(stderr, "Command returned %d\n", WEXITSTATUS(res));
            }
        }
    }
    return res;
}

int esh_change_directory(char **args, int nargs)
{
    int res;
    if (nargs != 2) {
        // Normally cd without an argument goes to $HOME. But we are not
        // handling that in this case.
        printf("ERROR: cd requires 1 argument, %d was given\n", nargs);
        return EINVAL;
    }
    res = chdir(args[1]);

    return res;
}

void esh_exit(char **args, int nargs)
{
    int res;

    if (nargs == 1)
        res = 0;
    else
        res = atoi(args[1]);

    exit(res);
}

static void _remove_quotes_and_terminate(char *cmdline, int cmdline_i,
                                         int token_section_i, int removed_chars)
{
    char *to, *from;
    int len;

    // Move section of token over IF
    //   - Section isn't first section
    //   - and characters were actually removed
    if (token_section_i > -1 && removed_chars) {
        from = cmdline + token_section_i;
        to = from - removed_chars;
        len = cmdline_i - token_section_i;
        memmove(to, from, len);
    }
    cmdline[cmdline_i - removed_chars] = 0;
}

int tokenize_cmdline(char *cmdline, char **args)
{
    int cmdline_i;
    int arg_i;
    int removed_chars;
    int recent_valid_character_i;
    int first_valid_character_i;
    char c;
    char token_sep;

    if (!cmdline || !args) {
        return -EINVAL;
    }

    arg_i = 0;
    token_sep = ' ';
    removed_chars = 0;
    first_valid_character_i = -1;
    recent_valid_character_i = -1;

    for (cmdline_i = 0; cmdline_i < MAX_CMDLINE_LEN; cmdline_i++) {
        c = cmdline[cmdline_i];
        switch (c) {
        case '\n':
            if (token_sep != ' ') {
                fprintf(stderr, "Unmatched quotes\n");
                return -EINVAL;
            }

            // There's no last token
            if (first_valid_character_i == -1) {
                return arg_i;
            }
            _remove_quotes_and_terminate(
                cmdline, cmdline_i, recent_valid_character_i, removed_chars);
            args[arg_i] = cmdline + first_valid_character_i;
            first_valid_character_i = -1;
            arg_i++;
            if (arg_i > MAX_ARG_N) {
                fprintf(stderr, "Too many tokens\n");
                return -E2BIG;
            }
            return arg_i;
        case '\'':
        case '"':
            /*
             * Cases:
             * 1. Opening Quote
             * 2. Closing Quote
             * 3. Ignored quote / Regular Character
             */
            if (token_sep == ' ') { // Case 1
                token_sep = c;
                // Remove quote if not the first char in the token
                if (first_valid_character_i > -1 &&
                    cmdline_i != first_valid_character_i) {
                    // Clear out any partial token
                    if (recent_valid_character_i > -1 && removed_chars) {
                        _remove_quotes_and_terminate(cmdline, cmdline_i,
                                                     recent_valid_character_i,
                                                     removed_chars);
                        removed_chars = 0;
                    }
                    removed_chars++;
                }
                recent_valid_character_i = -1;
                break;
            } else if (c == token_sep) { // Case 2
                _remove_quotes_and_terminate(cmdline, cmdline_i,
                                             recent_valid_character_i,
                                             removed_chars);

                // Adjust valid character indices
                recent_valid_character_i = -1;
                token_sep = ' ';
                // If we haven't seen a valid character in the token yet,
                // then we know that we don't need to shift at all, because
                // our anchor has not yet been set.
                if (first_valid_character_i > -1)
                    removed_chars++;
                break;
            }
            // Fall through, treat it as a regular character         // Case 3
        case ' ':
            // Move along, spaces don't count in quotes
            if (token_sep == ' ') {
                // We haven't yet seen a non-separator
                if (first_valid_character_i == -1) {
                    continue;
                }

                _remove_quotes_and_terminate(cmdline, cmdline_i,
                                             recent_valid_character_i,
                                             removed_chars);
                // Format argument
                args[arg_i] = cmdline + first_valid_character_i;

                // Reset
                removed_chars = 0;
                first_valid_character_i = -1;
                recent_valid_character_i = -1;
                arg_i++;
                if (arg_i > MAX_ARG_N) {
                    return -E2BIG;
                }
                break;
            }
        default:
            if (first_valid_character_i == -1)
                first_valid_character_i = cmdline_i;
            if (recent_valid_character_i == -1)
                recent_valid_character_i = cmdline_i;
            break;
        }
    }
    // If the last character we saw was not a newline, the data was invalid
    fprintf(stderr, "Too many characters\n");
    return -E2BIG;
}

// "Commented out" - leaving this here so you can see the thought process.
#if 0
// This covers "reasonable" uses of the commandline.
// This doesn't use an additonal buffer. Iterates over the list once.
// The next iteration will handle the entirety of the tokenizer's abilities.
// And will be cleaned up.
static int tokenize_cmdline_2(char *cmdline, char **args) {
    int cmdline_i;
    int arg_i;
    int first_valid_character_i;
    char c;
    char token_sep;

    if (!cmdline || !args) {
        return -EINVAL;
    }

    arg_i = 0;
    token_sep = ' ';
    first_valid_character_i = -1;
    
    // <= because we are not counting the newline at the end
    for (cmdline_i = 0; cmdline_i < MAX_CMDLINE_LEN; cmdline_i++) {
        c = cmdline[cmdline_i];
        if (c == '\n') {
            // This means there is an unmatched quote somewhere
            if (token_sep != ' ')
                return -EINVAL;

            // There's no last token
            if (first_valid_character_i == -1) {
                return arg_i;
            }
            cmdline[cmdline_i] = 0;
            args[arg_i] = cmdline + first_valid_character_i;
            first_valid_character_i = -1;
            arg_i++;
            if (arg_i > MAX_ARG_N) {
                return -E2BIG;
            }
            return arg_i;
        } else if (c == token_sep) {
            // Reset the current separator
            if (token_sep != ' ') {
                token_sep = ' ';
                // If the next character is not a space, the quotes had no meaning here
                // therefore, do not write another arg
                if (cmdline_i != MAX_CMDLINE_LEN - 1 && cmdline[cmdline_i + 1] != ' ') {
                    // This is the ugly case, we need to move our string forward to delete this useless char
                    memmove(cmdline + first_valid_character_i + 1,
                            cmdline + first_valid_character_i,
                            cmdline_i - first_valid_character_i);
                    // Zero out the char we don't care about anymore, move the head
                    cmdline[first_valid_character_i++] = 0;
                    continue;
                }
            }
            // We haven't yet seen a non-separator
            if (first_valid_character_i == -1) {
                continue;
            }
            cmdline[cmdline_i] = 0;
            args[arg_i] = cmdline + first_valid_character_i;
            first_valid_character_i = -1;
            arg_i++;
            if (arg_i > MAX_ARG_N) {
                return -E2BIG;
            }
        } else {
            // We haven't yet read any valid token chars
            if (first_valid_character_i == -1) {
                // Beginning of quoted token
                // TODO: what if trailing quote followed by a char?
                if (c == '"' || c == '\'') {
                    token_sep = c;
                    continue;
                }

                first_valid_character_i = cmdline_i;
            }
        }
    }
    // If the last character we saw was not a newline, the data was invalid
    return -E2BIG;
}

// First try. Gets the job done, but does not handle quotes. And has memory
// leaks. 
static int tokenize_cmdline_1(char *cmdline, char **args) {
    char c;
    // char curr_delimiter;  // Can be space, single or double quotes
    int cmdline_i;
    int arg_off;
    int arg_i;
    int res;
    int i;
    int n;
    char temp_arg_buf[MAX_CMDLINE_LEN+1];

    if (!cmdline || !args) {
        return EINVAL;
    }

    cmdline_i = 0;
    arg_off = 0;
    arg_i = 0;
    res = 0;
    args[0] = 0;  // 0 initialize
    
    while (cmdline_i <= MAX_CMDLINE_LEN &&
           (c = cmdline[cmdline_i++])) {
        // TODO: handle escapes
        // TODO: change this to pick up non-space
        if ((c == ' '  || c == '\n') && arg_off != 0) {    // Throw out leading spaces
            if (arg_off > MAX_CMDLINE_LEN) {
                res = E2BIG;  // TODO: pick right errno for this
                goto exit;
            }
            /*
             * 1. Allocate for the current arg to write
             * 2. Copy over the temporary buffer to the current arg
             * 3. Check for arg_n overflow
             * 4. Reset arg_offset
             * 5. Initialize current arg
             */
            args[arg_i] = malloc(arg_off + 1);  // Offset + 1 = size, size + null-term
            if (!args[arg_i]) {
                res = ENOMEM;
                goto exit;
            }

            temp_arg_buf[arg_off + 1] = 0;
            printf("No. %d, ", arg_i);
            n = strlcpy(args[arg_i], temp_arg_buf, arg_off + 1);
            if(n != arg_off) {
                    printf("Failing on copy with err %d\n", n);
                    res = EINVAL;
                    goto exit;
            }
            
            if(++arg_i > MAX_ARG_N) {
                res = E2BIG;
                goto exit;
            }
            arg_off = 0;
            args[arg_i] = 0;  // 0 initialize
        } else if (c != ' ' && c != '\n') {
            // Only write non-spaces
            temp_arg_buf[arg_off++] = c;
        }
    }

    if (cmdline_i > MAX_CMDLINE_LEN) {
        res = E2BIG;  // TODO: pick right errno for this
        goto exit;
    }
    // TODO: handle end
exit:
    if (res) {
        for (i = 0; i <= arg_i; i++) {
            if (args[i])
                free(args[i]);
        }
        return -res;
    }

    return arg_i;
    
}
#endif
