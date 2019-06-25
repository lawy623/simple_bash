# Simple Bash in C.
Author: Yue Luo

## Programming part:
The source code is in '`shell.c`' with a header file named '`shell.h`'. Please download the repo, and run '`make w4118_sh`'' to make the targeted executable. You can run '`make clean`'' to clean it up.

### Design Logic:
- First the shell prints prompt and it is waiting for the stdin and it will receive it as a line. Then, it will go to replace all the found valid !! and !str in this input. If the !'s are not valid(i.e. can not find in past logs, or just a single !), the shell will not save this invalid command in log and will not execute it.
- After that, it will find the number of valid '|' in it. If the '|' is not valid(i.e. '|' appears at the beginning or at last, or the string between two '|' are all white spaces), shell will save it in the log, but do not execute it. We also count the numbers of '|' if this input is valid.
- The log table is only 100 logs maximum. When clear history, I do not clean it up, but I use a start_count to set the new start point. When logs are needed to be replaced, just clean the table entry found by [pos%100] and point it to a new log.
- Input without using pipe: Just tokenize the valid string input, and execute by built-in command or system call via execvp().
- Input with pipes: Before that we keep the number of valid '|' in this input, and here we using dup2() iteratively to pass inputs and results through pipes.

### What does the shell support?
1. Multiple pipes can be used. The number is not limited. But invalid pipe string will not be execute.
2. Basic signal handle. (Ctrl-c, ctrl-z will not exit the shell, but it is also not like the real bash, which will start a new line; ctrl-d will make EOF and terminate the shell, but also sending error message).
3. Pressing Enter only (empty line) will just continue with a new line. Only Space and Enter(Lines with all ' 's) will also continue. These two inputs will not be saved in history log.
4. Built-in command 'cd', 'history', and 'exit'. They will return error if the arguments are not valid ('cd': only $cd is valid; 'history': only $history / $history -c / $history [num] are valid; 'exit': all can exit, but will print error with any other arguments.
5. Shell will decide whether the input with !str and !! is valid. If ! appears along, or !str can not be found, the shell will continue without saving it. No matter where is the position of !str and !!, they will be replaced(which is like a real bash).

### What does the shell not support?
1. Special characters like '\t', '[', '^' are not specifically implemented and tested in this shell. Please better not include them in the input.
2. Not all the signal handlers are implemented same as real bash (Like direction arrows, tab with auto complete, etc.)
3. We do not support '||' and '&&'. Also not redirection '>', '<'.
4. The length of  total valid input (i.e. after replacing the !str and !!) may be limited to 300 char.
5. Single ! With not str following will be treated as invalid and it is not saved in history.

### Sample run
A sample run of this shell is included in the run_sample.txt.
