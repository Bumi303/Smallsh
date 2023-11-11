#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#include "smallsh.h"

//background == 1 --- foreground == 0
int grounding = 0;
int background_mode = true;

int flag = 0;


pid_t pid_tracker[512];
int count;

int status;

//name: list
//description: linked list that hold all value for commands
//return: 
struct list {
    char arg[2048];
    struct list* next;
};



//name:  pid_digits
//description: finds number of digitsi in pid for memory things
//return: 
int pid_digits (int pid) {
    int i = 1;
    
    // divids by 10 until it doesn't need to any more
    for (int i = 1; pid > 9; i++) {
        pid /= 10;
    }
    return i;
}

//name: expand_variable
//description: expands $$ and puts so it is ready for linked list
//return: 
void expand_variable(char* input, pid_t pid){
    //unsigned int
    size_t length = strlen(input);
    int j = 0;
    //current position from string
    int curr = 0;
    pid = getpid();
    //converts pid to a string
    char str_pid[10];
    //removes garbage
    strcpy(str_pid, "");
    sprintf(str_pid, "%d", pid);

    //used to track characters as they traverse the char pointer
    char first;
    char second;

    //new string we are making (//10 because max size is 2^22)
    char new_str[length * 10];
    char temp[length * 10];
    strcpy(temp, input);

    //removes garbage value from string
    strcpy(new_str, "");

    //iterates through char pointer
    for (int i = 0; i < length; i++){
        //first aka the most left value
        first = temp[i];
        if (i != length - 1){
            //the value after first aka second
            second = temp[i + 1];
        }

        //checks if $$ is found and replaces it with pid also updates new string
        if (first == '$' && second == '$' && first == second){
            //copies input into new_str up until $ is found
            strncpy(new_str, temp, i);

            //needed to remove garbage value
            new_str[i] = '\0';

            //replaces $$ with pid
            strcat(new_str, str_pid);

            //adds the rest of the string onto new str
            strcat(new_str, strncpy(temp, temp + i + 2, length));

            //replaces string with updated version and updates length
            strcpy(temp, new_str);
            length = strlen(temp);
            //reset new_str
            strcpy(new_str, "");
            
            //needed because pid can be different lengths
            curr = i + pid_digits(pid);
        }
    }
    strcpy(input, temp);
}

//name: find_num_spaces
//description: finds the number of spaces in the command
//return: 
int find_num_spaces(char* command){
    int num_spaces = 0;
    //counts every occurance of a space
    for (int i = 0; i < strlen(command); i++){
        if (command[i] == ' '){
            num_spaces++;
        }
    }
    //we dont want the final space
    return num_spaces - 1;
}

//name: parse_command
//description: parses through input and saves it in linked list
//return: 
struct list* parse_command(char* command){
    //used to parse through user entered commands
    
    //needed to keep track of beginning
    struct list* head = NULL;
    struct list* tail = NULL;

    int num_spaces = find_num_spaces(command);
   

    //iterates through all positions (needed in case each value is one letter)
    for (int i = 0; i < num_spaces + 1; i++){
         
        //creates new node
        struct list* new_node = malloc(sizeof(struct list));

        // takes token at space and puts it into linked list
        char* saveptr;
        char *token = strtok_r(command, " ", &saveptr);
        strcpy(new_node->arg, token); 
        strcpy(command, saveptr);

        // printf("index ---> %d | command ---> %25s | arg ---> %15s | token ---> %25s \n", i, command, new_node->arg, token);
        // fflush(stdout);
        //keeps track of head and tail
        if (head == NULL){
            head = new_node;
            tail = new_node;
        }
        else{
            tail->next = new_node;
            tail = new_node;
        }

        //routine stuff
        new_node->next = NULL;
        new_node = new_node->next;

    }
    return head;
}

//name: check_grounding
//description: checks what the ground of the command needs to be
//return: 
int check_grounding(struct list* l){
    struct list* temp = l;

    while (temp->next != NULL){
        temp = temp->next;
    }
    // we iterate to the last value to find if the 
    //the command is background or foreground

    if (strcmp(temp->arg, "&") == 0 && background_mode == true){
        grounding = 1;
    }
    else if (strcmp(temp->arg, "&") == 0 && background_mode == false){

    }
}

//name: exit_operations
//description: kill sthe program
//return: 
void exit_operations(){
    int index;
    if (count > 0){
        //clears all pids 
        while (index < count ){
            kill(pid_tracker[index], SIGTERM);
            exit(1);
        }
    }
    exit(0);
}

//name: cd_command
//description:
//return: 
void cd_command(struct list* l, char* command){
    struct list* temp = l;
    int index = 0;
    char s[2048];
    /*
        you probably watn tto check grounding here
        i do not need to
    */
   
    //tests if there is only one value
    if (index == 0 && temp->next == NULL){
        chdir(getenv("HOME"));
        // printf("%s\n", getcwd(s, 2048));
        // fflush(stdout);
    }
    else{
        //cd can only have two values
        if (chdir(temp->next->arg) != -1){
            chdir(temp->next->arg);
            // printf("%s\n", getcwd(s, 2048));
            // fflush(stdout);
        }
        else{
            printf("The Directory your looking for is can not be found\n");
            fflush(stdout);
        }
        
    }
    
}

//name: check_file_io
//description: checks if the command has input output functionality
//return: 
void check_file_io(struct list* l,char* input, char* output){
    struct list* temp = l;
    //iterates through list
    //pretty self explanitory
    while (temp != NULL){
        if (strcmp(temp->arg, "<") == 0){
            strcpy(input, temp->next->arg);
        }
        if (strcmp(temp->arg, ">") == 0){
            strcpy(output, temp->next->arg);
        }
        temp = temp->next;
    }
}

//name: exit_status
//description: shows exit status and previous signals
//return: 
void exit_status(int *exit_s){
    int e = 0;
    int s = 0;
    int v;

    /*
        first we need to get the status of the last process
        this returns that value using the pid
    */
    waitpid(getpid(), &status, 0);


    //checks the condition of status
    int t = WIFEXITED(status);

    /*
        Wacky way to check the status
        gaureented a different way 
        i am very tired
    */

    if (t == 1){
        e = WEXITSTATUS(status);
    }
    else {
        s = WTERMSIG(status);
    }

    //compares value to determine what type of status to show
    if (e >= 1 || s >= 1){
        v = 1;
    }
    else{
        v = 0;
    }

    //prints out status messages
    if (s == 0){
        printf("exit value %d\n", v);
        fflush(stdout);
    }
    else{
        *exit_s = 1;
        printf("terminated by signal %d\n", s);
        fflush(stdout);
    }
}

//name: other_commands
//description: handles all other commands needed for secure shell
//return: 
void other_commands(struct list* l, char* command, char* input, char* output, int *e_status, struct sigaction SIGINT_action){
    //513 because last value is null;
    char *pathway[513];
    struct list* temp = l;
    struct list* t = l;
    struct list* tester = l; //for testing purposes
    int i = 0;
    int file_in;
    int file_out;
    int re;
    pid_t s_pid;

    // checks if any special conditions apply and removes them from pathway
    // the function we use does not handle these operations
    while (t != NULL){
        if (strcmp(t->arg, ">") != 0 && strcmp(t->arg, "<") != 0 && strcmp(t->arg, "&") != 0){
            pathway[i] = t->arg; 
        }
        t = t->next;
        i++;
    }
    //last value needs to be null
    pathway[i] = NULL;

    //not used but scared to delete
    //you know how c is
    int *child_exit = 0;

    //forks
    s_pid = fork();

    //we need to keep track of pids for exit operations
    pid_tracker[count] = s_pid;
    //counts number of pids
    count++;
    
    //a lot from below if from the lecture and explorations
    switch(s_pid){
        case -1: //redundancy
            perror("fork() failed\n");
            exit(1);
            break;
        case 0:
            // input  == "<"
            if (strcmp(input, "") != 0){
                //opens file and chekc is it is valid
                file_in = open(input, O_RDONLY);
                if (file_in < 0){
                    perror("Input file has not been found or does not exist: ");
                    fflush(stdout);
                    exit(1);
                }
                
                /*
                    might not be needed but at this point
                    I am afriad to delete
                */
                re = dup2(file_in, 0);
                if (re < 0){ //checks if dup2 is working 
                    perror("input dup2()");
                    fflush(stdout);
                    exit(2);
                }

                //closes the function
                //exploration said so 
                //used for exec
                fcntl(file_in, F_SETFD, FD_CLOEXEC);
            }   
            
            fflush(stdout);
            // output == ">"
            if (strcmp(output, "") != 0){
                //opens file and chekc is it is valid
                file_out = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (file_out < 0){
                    perror("Output file has not been found or does not exist: ");
                    fflush(stdout);
                    exit(1);
                }


                /*
                    might not be needed but at this point
                    I am afriad to delete
                */
                re = dup2(file_out, 1);
                if (re < 0) {
                    perror("output dup2()");
                    exit(2);
                }

                //closes the function
                //exploration said so 
                //used for exec
                fcntl(file_out, F_SETFD, FD_CLOEXEC);
            }


            // depending on ground we want to know if we can use
            //^C. some cases this is used
            if (grounding == 0){ 
                SIGINT_action.sa_handler = SIG_DFL;
            }
            sigaction(SIGINT, &SIGINT_action, NULL);


            // executes and checks if command is real
            int t;
            t = execvp(pathway[0], (char* const*)pathway);
            if (t == -1){
                perror("Command not found: ");
				exit(2);
            }
            break;

        default:
            // checks for both conditions to be true
            // i was getting errors for hours until i found this
            // might be another way to do this but whatever
            if (grounding == 1 && background_mode == true){
                waitpid(s_pid, &status, WNOHANG);
                printf("background pid is %d\n", s_pid);
                fflush(stdout);
            }
            else{
                waitpid(s_pid, &status, 0);
            }
    }

    //need to wait for all operationt to be done
    while ((s_pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("\nBackground pid %d has terminated: ", s_pid);
        fflush(stdout);
        exit_status(e_status);
    }

    //reset flags
    grounding = 0;
    flag = 0;
}

//name: command_prompt
//description: this is the command prompt and handle built in commands
//return: 
void command_prompt(char* command, pid_t pid, struct sigaction SIGINT_action){
    printf(": ");
    fflush(stdout);
    fgets(command, 2048, stdin);
    char input[2048];
    char output[2048];
    strcpy(input, "");
    strcpy(output, "");
    int e_status = 0;


    // if (strcmp(command, "kill -SIGTSTP $$") == 0){
    //     flag = 1;
    // }
    
    
    //removes all new lines
    for (int i = 0; i < strlen(command); i++){
        if (command[i] == '\n') { command[i] = '\0'; }
    }

    
    //checks for blank line
    if (command[0] != '\0'){
        //checks if exit was entered
        if (strcmp(command, "exit") != 0){
            
            
            //converts all $$ to pid number
            expand_variable(command, pid);

            //needed to parse data correctly, does not interfere with ANY inputs
            strcat(command, " *");

            
            //creates a linked list of all arguments seperated into nodes
            struct list* arguments = parse_command(command);

            // checks if status was entered before foreground commands
            if (strcmp(arguments->arg, "status") == 0) { 
                exit_status(&e_status); 
                
            }
            else if (strcmp(arguments->arg, "#") != 0){
                //checks if command has file input output
                check_file_io(arguments, input, output);

                //checks if foreground or background
                check_grounding(arguments);

                //resets commands to avoid problems
                
                fflush(stdout);

                // if user inputs cd this handles that
                if (strcmp(arguments->arg, "cd") == 0) {
                    cd_command(arguments, command);
                }
                else{
                    //all other commands not required
                    other_commands(arguments, command, input, output, &e_status, SIGINT_action);
                }

                // need to check if status needs to be called
                if (WIFSIGNALED(status) && e_status == 0){ exit_status(&e_status); }
            }

            //resets the file to avoid errors
            //i think this is redundant
            for (int i = 0; i < 2048; i++){
                input[i] = '\0';
                output[i] = '\0';
            }
        }
        else {
            //needed for exiting correctly
            exit_operations();
        }
    }
}   
 
//name: catchSIGTSTP
//description: used to catch ^K and switch in/out foreground mode
//return: 
void catchSIGTSTP(int signo){
    /*
    
        since it flip flops back and forth this is needed to prevent
        it from switching the wrong way
    
    */
    if (background_mode == true){
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        //needed for reentrant reasons
        write(STDOUT_FILENO, message, 50);
        // fflush(stdout);
        background_mode = false;
    }
    else if (background_mode == false){
        char* message = "\nExiting foreground-only mode\n";
        //needed for reentrant reasons
        write(STDOUT_FILENO, message, 30);
        // fflush(stdout);
        background_mode = true;
    }
    else {
        char* message = "\nSomething bad has happened\n";
        //needed for reentrant reasons
        write(STDOUT_FILENO, message, 30);
        fflush(stdout);
        background_mode = true;
    }
    //function exits on a off line
    char* continue_commands = ": ";
	write(STDOUT_FILENO, continue_commands, 2);
}

//name: catchSIGINT
//description: catches ^C
//return: 
void catchSIGINT(int signo){
    //not used
    char* message = "\nCaught SIGINT, sleeping for 10 seconds\n";
    write(STDOUT_FILENO, message, 40);
    sleep(10);
}

//name: main
//description: main hub for program
//return: 
int main(){
	pid_t pid = getpid();
    int ppid = getppid();
    bool stop_smallsh = false;
    char command[2048];

    /*
        takes care of ^K and ^C
        needs to be in the order to prevent errors
    */
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    // SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    /*
    loops until user inputs exit
    */
    while (stop_smallsh == false){
        command_prompt(command, pid, SIGINT_action);
        strcpy(command, "");
        if (strcmp(command, "exit") == 0){
            stop_smallsh = true;
        }
    }
    
	return 0;
}