#ifndef __SMALLSH_H
#define __SMALLSH_H

struct elements;

void command_prompt(char* command, pid_t pid, struct sigaction SIGINT_action);
void expand_variable(char* input,  pid_t pid);
int pid_digits (int pid);
struct list* parse_command(char* command);
void exit_status(int *e_status);
int check_grounding(struct list* l);
int find_num_spaces(char* command);
void cd_command(struct list* l, char* command);
void other_commands(struct list* l, char* command, char* input, 
char* output, int *e_status, struct sigaction SIGINT_action);

void check_file_io(struct list* l, char* input, char* output);
void catchSIGINT(int signo);
void catchSIGTSTP(int signo);
void exit_operations();

#endif
