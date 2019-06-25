/*
 *  author: Yue Luo
 *  userName: lawy623
 *  UID: yl4003
 *  email: yl4003@columbia.edu
 *  data: 18 Sep 2018
 */

/* Function Declaratins */
void tokenize(char *input, char *arglist[]);
int execute(char *arglist[]);
int get_pipes(char *input);
void do_2pipes(char *input);
void do_pipes(char *input, int pipes_num);
int splits_pipes(char *inputs[], char *input, int sec);
int replace_sign(char *new_input, char *input);
