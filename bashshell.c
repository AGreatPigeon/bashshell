/***************************************************************************
 *
 * Filename: Simple_Shell_V1_0
 *
 * Synopsis:
 * Simple_Shell <User input>
 *
 * This code program is designed to take in Command from the user and carry them out
 * using the fork command in Linux or using built in Function to perform some functions.
 * Also maintains a history of the user's last 20 commands they can later invoke and a small set of
 * aliased commands may also be set by the user and later invoked to perform their aliased function.
 *
 * Author:
 *      Fraser George, Paul Burns, Kristine Semjonova, Andi Anderson
 *
 * Version: See VERSION below
 *
 **************************************************************************/

/********************* R E V I S I O N   H I S T O R Y ********************
 *
 * v0.1 20/02/2014  Added User input function which Prints a prompt then takes user input using an fgets
 *		    then tokenises the Input using strtok these tokens are then printed with " at either.
 *		    Also Flushes the stdin before taking input and resets the Cmdstr at the start of each loop.
 *		    Also checks for a new line and the exit command.
 *
 * v0.2 27/02/2014  Implemented Task 2 by utilising the fork() command in a function createProcess. 
 *		    If the fork() command has succeeded, use execvp to execute the command using the pointer to 
 *		    the first element in the input, e.g. ls and the remainder (if applicable) e.g. -lR.
 *
 * v0.3 04/03/2014  Implemented task 3 using chdir(getenv("HOME")) to change directory to the 
 *          home directory. Before calling the user input function. Also implemented the internal 
 *          command pwd through using the getcwd() function to get the current working directory and 
 *          put in the char *cwd which is then printed using fprintf.
 *
 * v0.4 04/03/2014  Implemented task 4 getting and setting the path. Getting the path simply uses
 *          getenv("PATH")
 *          Also checking to assure the correct number of parameters have been provided, Set paths
 *		    is achieved by using setenv("PATH",path,1) where path is taken as the second
 *		    element of the cmdStr array. The original path is also saved at start-up of the shell
 *		    and restored before exiting the shell and printed on both occasions to ensure it is the same.
 *
 * v0.5 11/03/2014  Implemented Task 5 added a check for the cd command within the command function
 *		    Implemented changing directories within the internal function cd() and used chdir
 *		    to change directories. perror used to handle errors when changing directory.
 *
 * v0.6 17/03/2014  Implemented task 6 History array used to store Commands entered by user checks if first
 *		    Char is not a ! in which case the command is to be stored in history,
 *		    after the array is full, we just shift all array elements by one position, i.e.
 *		    element 1 becomes 0, 2 becomes 1, etc. In the last position the most recent
 *		    command is stored. To invoke a command we just tokenise the command in history then
 *		    pass its cmdStr to the command() function as normal remembering not to add this command to 
 *          the history.
 *
 * v0.7 25/03/2014  Implemented task 7 saved and load the history using the fopen funtion to open the file,
 *		    then use fgets to read the next history command to be added to history and the fprintf function
 *          to write commands from the History array to the opened file then fclose(FILE) the file in both
 *          cases.
 *
 * v0.8 13/04/2014  Implemented Task 8 aliases and improved code readability by including the tokenise code
 *          in its own function.
 *		    In tokenise, check to see if the command being entered is an alias. If so, read the command 
 *          which is aliased and call tokenise once again with this aliased command.
 */

#define VERSION "Simple_Shell_V1_0, Last Update 13/04/2014\n"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

/* Maximum number of arguments per line */
#define MAX_ARGS 50
/* Maximum length of an input */
#define MAX_LEN  514
/* Set of delimiters used to tokenize string */
#define DELIMS " <>|\t\r\n"
/* Shell prompt */
#define prompt "> "

/* The following macro is used to purge unwanted input */
#define FLUSH_STDIN(x) while((x=fgetc(stdin)) != EOF && x!='\n');
/* Size macro: Used to determine the size of an array */
#define SIZE(x) (sizeof(x)/sizeof(x[0]))

/* Structure which contains:
 * - alias name;
 * - corresponding command. */
typedef struct{
	char * alias_name;
	char * alias_cmd;
} Alias;

/* Array to store commands history. */
char *history[MAX_LEN];
/* Array to store parsed command line */
char *cmdStr[MAX_ARGS];

/* Array of aliases, which size is 10 */
Alias alias_array[10];

/* Counter of total number of aliases */
int alias_counter;
/* Counter of total number of commands stored in history */
int history_counter;

void commands();

/* Synopsis:
 * void reset_cmdstr()
 *
 * Description:
 * The function deletes all content of cmdStr[]
 * by setting each element to NULL.
 *
 * Returns:
 * The function has return type void.
 */
void reset_cmdstr() {

	int i = 0;
	while(cmdStr[i] != NULL) {
		cmdStr[i] = NULL;
		i++;
	}
}

/* Alias Commands */

/* Synopsis:
 * int alias_check(char *alias)
 *
 * Description:
 * The function checks whether the alias user wants to use
 * has already been used by comparing desired alias
 * to each alias in alias_array[].
 *
 * Returns:
 * Index of this alias in alias_array[] if alias has already been used.
 * -1 if alias has not been used.
 */
int alias_check(char *alias){
    int i = 0;
	while(i < alias_counter){
		char *check = alias_array[i].alias_name;
		if(strcmp(check, alias) == 0)
			return i;
        i++;
	}
	return -1;
}

/* Synopsis:
 * void print_alias()
 *
 * Description:
 * The function prints out all aliases and their corresponding
 * commands stored in alias_array[]. If there are no aliases
 * then error message is printed out.
 *
 * Returns:
 * The function has return type void.
 */
void print_alias(){
	int i = 0;
	if(alias_counter == 0)
		puts("Error: There are no aliases recognized.");
	while(i < alias_counter){
		printf("Alias: %s\tCommand: %s\n", alias_array[i].alias_name, alias_array[i].alias_cmd);
		i++;
	}
}

/* Synopsis:
 * void add_alias()
 *
 * Description:
 * The function adds new alias with its corresponding command
 * to alias_array[].Several checks are done before adding:
 * - Whether there is a valid number of parameters
 * - Whether user is not trying to create alias equal
 *   to the command name;
 * - Whether user is not trying to create an alias for alias;
 * - Whether user is not trying to use already used alias;
 * - Whether the maximum number of aliases is reached.
 * If successful, new alias is created and counter is incremented.
 *
 * Returns:
 * The function has return type void.
 */
void add_alias(){

    int i, index;
	/* Check to prevent invalid number of arguments and 
	 * prevent having alias same as command name. */
	if(cmdStr[2] == NULL || strcmp(cmdStr[1], cmdStr[2]) == 0){
		puts("Error: Invalid alias");
		return;
	}

	/* Check whether aliased command is not an alias itself. */
	index = alias_check(cmdStr[2]);

   	if(index >= 0) /* If the command is an alias itself .. */
		puts("Error: Cannot give an alias to an existing alias."); /* .. display error message. */

	else{
		/* Check whether desired alias has been already used. */
		index = alias_check(cmdStr[1]);

		if(index >= 0){ /* The alias has been used, so ..
				 *display appropriate message .. */
			puts("Warning: An alias with this name already exists. Proceeding to overwrite."); 

			/* .. and override the previous one. */
			alias_array[index].alias_name = cmdStr[1];
			i = 2;
			/* Delete old command. */
			strcpy(alias_array[index].alias_cmd, "");
           		/* Add new command with all its parameters. */
			while(cmdStr[i] != NULL){
				strcat(alias_array[index].alias_cmd, cmdStr[i]);
				strcat(alias_array[index].alias_cmd, " ");
				i++;
			}
		}

		else{ /* Alias can be used. */

			/* Check whether the maximum number of aliases is reached. */
			if(alias_counter >= SIZE(alias_array)){
				puts("Error: Alias list is full.");
				return;
			}
			/* Add new alias and corresponding command with its parameters. */
			alias_array[alias_counter].alias_name = cmdStr[1];
            		i = 2;
			strcpy(alias_array[alias_counter].alias_cmd, "");

			while(cmdStr[i] != NULL){
				strcat(alias_array[alias_counter].alias_cmd, cmdStr[i]);
				strcat(alias_array[alias_counter].alias_cmd, " ");
				i++;
			}
			alias_counter++; /* Increment total number of aliases. */
		}
	}
}

/* Synopsis:
 * void alias()
 *
 * Description:
 * The function checks what user intended to do,
 * add new alias or print all aliases.
 * This is done by checking whether 'aliases' is followed 
 * by any parameters and then appropriate function is called.
 *
 * Returns:
 * The function has return type void.
 */
void alias(){

	if(cmdStr[1] == NULL)
		print_alias();

	else
		add_alias();

}

/* Synopsis:
 * void remove_alias()
 *
 * Description:
 * The function removes alias from alias_array[] if it is present
 * or displays error message if it is not.
 *
 * Returns:
 * The function has return type void.
 */
void remove_alias(){
    int i, index;
	if(cmdStr[1] == NULL) /* Check if valid number of parameters. */
		puts("Error: No alias selected");
	else{
		/* Get index of alias in alias_check[] if it is there. */
		index = alias_check(cmdStr[1]);
		/* If alias is in array remove it and shift all subsequent 
		 * aliases by 1 to the beginning of array. */
		if(index >= 0){
            		i=index+1;
			while(i<alias_counter){
	 			strcpy(alias_array[i-1].alias_name, alias_array[i].alias_name);
				strcpy(alias_array[i-1].alias_cmd,alias_array[i].alias_cmd);
				i++;
 			}
			alias_counter--;
		}
		else
			puts("Error: Alias does not exist.");
	}
}

/* Synopsis:
 * int tokenise(char *input)
 *
 * Description:
 * The function takes user input and parses into tokens.
 *
 * Returns:
 * 0 
 * 1
 * 2
 */
int tokenise(char *input){

 	char *temp, *token, *inputCopy = malloc(sizeof(input));
    	int index, i;
    	strcpy(inputCopy, input);

    	cmdStr[0] = malloc(sizeof(cmdStr[0]));
 	cmdStr[0] = strtok(input, DELIMS);

 	if(cmdStr[0] == NULL)
 		return 0;

 	/* If the input includes an alias invoke, then it must be processed. */
 	index = alias_check(cmdStr[0]);
    
 	if(index >= 0){
 		temp = malloc(sizeof(temp));
 		strcpy(temp, alias_array[index].alias_cmd);
 		while ( (token = strtok(NULL, DELIMS) ) != NULL) {
 			strcat(temp, " ");
 			strcat(temp, token);
 		}
 		return tokenise(temp);
 	}

    	i = 1;
 	while ( (token = strtok(NULL, DELIMS) ) != NULL) {

 		cmdStr[i] = malloc(sizeof(cmdStr[i]));
 		strcpy(cmdStr[i], token);
 		i++;

 		if (i >= 50) {
 			puts("Error: Too many parameters\n");
 			return 0;
 		}
 	}
 	return 1;
}

/* Command Functions */

/* Synopsis:
 * void pwd()
 *
 * Description:
 * The function prints out current working directory.
 *
 * Returns:
 * The function has return type void
 */
void pwd(){
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(stdout, "%s\n", cwd);
    else
        perror("getcwd() error");
}

/* Synopsis:
 * void cd()
 *
 * Description:
 * The function changes working directory to either HOME directory if
 * no extra parameters provided or to working directory provided
 * by the user.
 *
 * Returns:
 * The function has return type void
 */
void cd(){
    errno = 0;
    if (!cmdStr[2]){ /* Check if not too many arguments provided. */
        if (!cmdStr[1]) { /* Check if new directory chosen. */
            chdir(getenv("HOME")); /* If no, change to HOME directory. */
            if (errno != 0 ){
                perror("Error changing dir");
            } 
	    else{
                pwd(); /* Print new working directory. */
            }
        }
   	else {
            chdir(cmdStr[1]); /* Change to working directory provided by user. */
            if (errno != 0 ){
                perror("Error changing dir");
            }
	    else{
                pwd();
            }
        }
    }
    else{
        printf("chdir: Error too many arguments\n");
    }
    return;
}

/* Synopsis:
 * void add_history(char *input)
 *
 * Description:
 * The function adds new command to the history. If history is full,
 * then first command in history is removed, the rest of the commands are shifted
 * to the beginning of the array and new command is added to the end of the array.
 *
 * Returns:
 * The function has return type void.
 */
void add_history(char *input){
    /* If history is full .. */
    if (history_counter == 20){
	/* .. then shift all commands except first one
	 * to the beginning of the array .. */
        int i = 0;
        while (i < 19){
            history[i] = history[i+1];
            i++;
        }
		/* .. and add new command to the end of the array. */
        history[19] = NULL;
        history[19] = strdup(input);
    }
    /* If history is not full then add command. */
    history[history_counter] = strdup(input);
    /* If history is not full, increment counter. */
    if (history_counter < 20)
        history_counter++;
}

/* Synopsis:
 * void invoke_history()
 *
 * Description:
 * The function invokes and runs a command from history.
 * '!!' runs last command
 * '!<no.>' runs command specified by <no.>.
 *
 * Returns:
 * The function has return type void.
 */
void invoke_history(){
    if(!cmdStr[1]){ /* Check if valid number of arguments. */
        char *input = NULL, *copy = NULL;

        if(history_counter == 0){
            puts("Error: No commands in history.");
            return;
        }
		/* Check if run last command. */
        if(strncmp("!!",cmdStr[0], 2) == 0){
            if (history_counter == 0){
                input = strdup(history[20]);
            } else {
                input = strdup(history[history_counter - 1]);
            }
        }
        else { /* Run command specified by user. */
            char *num = strtok(cmdStr[0], "!\n ");
            int index = atoi(num);
            if (index > 0){
                if (index < 0 || index > 20){
                    puts("Error: History number out of bounds.");
                    return;
                } else if (index <= history_counter) {
                    input = strdup(history[index-1]);
                } else if (index > history_counter) {
                    puts("Error: History number out of bounds.");
                    return;
                }
            }else {
                puts("Error: Invalid history invocation.");
                return;
            }
        }
        copy = strdup(input);
        if (tokenise(copy) == 1)
            commands();
        else
            puts("Error: Invalid history invocation.");
    }
    else{
        puts("HISTORY: Error too many arguments");
    }
    return;
}

/* Synopsis:
 * void open_history()
 *
 * Description:
 * The function creates new file at HOME directory to store history
 * if the file has not been created. 
 * Otherwise, reads lines from file and adds commands to history.
 *
 * Returns:
 * The function has return type void
 */
void open_history(){
	FILE *history_file;
	char load_file[513];
    char *letter;

	/* Create a new file. */
	if((history_file = fopen(".hist_list", "r")) == NULL){
		puts("Creating new history file at HOME directory.");
	}

	/* File exists, read it. */
	else {
		while (1) {
			if((fgets(load_file, 513, history_file)) == NULL){ /* End of file check. */
				break;
			}

           		 /* Check to see if there is a newline file in the history.
            		  * If so, replace with a null terminate. */
			if ((letter = strchr(load_file, '\n')) != NULL)
                		*letter = '\0';
			add_history(load_file);
	 	}
	 	fclose(history_file);
	}

}

/* Synopsis:
 * void save_history()
 *
 * Description:
 * The function opens file where history is stored and prints
 * history to that file.
 *
 * Returns:
 * The function has return type void.
 */
void save_history(){
	FILE *history_file;
    	int i;
	history_file = fopen(".hist_list", "w+");
 	i = 0;

	while(i < history_counter){ /* Ensures empty history is not printed. */
		fprintf(history_file, "%s\n", history[i]);
		i++;
	}
 	fclose(history_file);
}

/* Synopsis:
 * void createProcess()
 *
 * Description:
 * The function forks a child process from parent
 * process, executes it, goes back to parent process.
 *
 * Returns:
 * The function has return type void.
 */
void createProcess(){
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {  /* Fork a child process. */
        perror("Error");
        exit(1);
    }
    else if (pid == 0) {  /* For the child process.. */
        if (execvp(cmdStr[0], cmdStr) < 0) { /* .. execute the command. */
            perror("Error");
            exit(1);
        }
    }
    else {   /* For the parent.. */
        while (wait(&status) != pid) /* .. wait for completion. */
            ;
    }
}

/* Synopsis:
 * void commands()
 *
 * Description:
 * The function compares user input to different command names,
 * checks if enough and not too many arguments provided for each
 * command and then invokes appropriate function which
 * executes command from input line.
 *
 * Returns:
 * The function has return type void.
 */
void commands(){
    int count = 0; /* Helper variable for printing out history. */
    while (1){
        /* Check if user invoked a command from history. */
        if (cmdStr[0][0] == '!')
        {
            invoke_history();
            return;
        }
        else if( strcmp(cmdStr[0], "history") == 0)
        {
            if(!cmdStr[1]){ /* Check if valid number of arguments. */
                while(count < history_counter){
                    printf("[%d]> %s\n", count + 1, history[count]);
                    count++;
                }
            } else {
                puts("HISTORY: Error too many arguments.");
            }
        }
        else if (strcmp(cmdStr[0], "getpath") == 0) {
            if(!cmdStr[1]){ /* Check if valid number of arguments. */
                printf("%s\n",getenv("PATH"));
            }
            else{
                puts("GETPATH: Error too many arguments.");
            }
        }
        else if (strcmp(cmdStr[0], "setpath") == 0) {
            if(!cmdStr[1]){ /* Check if enough arguments. */
                puts("SETPATH: Not enough arguments.");
            }
            else if(!cmdStr[2]){ /* Check if valid number of arguments. */
                const char *path=cmdStr[1];
                if(setenv("PATH",path,1)<0)
                    perror("Error setting path: ");
            }
            else{
                puts("SETPATH: Error too many arguments");
            }
        }
        else if (strcmp(cmdStr[0], "pwd") == 0) {
            if(!cmdStr[1]){ /* Check if valid number of arguments. */
                pwd();
            }
            else{
                puts("PWD: Error too many arguments.");
            }
        }
        else if (strcmp(cmdStr[0], "cd") == 0) {
            cd();
        }
        else if(strcmp(cmdStr[0], "alias") == 0){
            alias();

        } 
	else if(strcmp(cmdStr[0], "unalias") == 0){
            remove_alias();
        }
        else{
            createProcess();
        }
        return;
    }
}

/* Synopsis:
 * void user_input()
 *
 * Description:
 * The function works with user input, checks it has valid length,
 * whether or not exit command received, invokes input string tokenising
 * and checks whether to add command to commands history or if it is
 * history invocation.
 *
 * Returns:
 * The function has return type void.
 */
void user_input(){
    /* Declare local variables and initialise. */
    char input[MAX_LEN], *cmd = NULL, copy[MAX_LEN];
    int i, return_val;
    while(1){
        reset_cmdstr();
        i=0;
        printf(prompt);

        if (fgets(input, MAX_LEN, stdin) == NULL){
            puts("Exit code received");
            return;
        }

        if (strlen(input)>(MAX_LEN-2)){
            puts(": file name too big\n");
            FLUSH_STDIN(i)
            continue;
        }

        /* Checking user has not just hit enter. */
        if (input[0] == '\n')
            continue;

        if ((cmd = strchr(input, '\n')) != NULL)
            *cmd = '\0';

        /* Get copy of the string before tokenising. */
        strcpy(copy, input);
        
        return_val = tokenise(input);

        if (return_val == 0)
            continue;
        else if (return_val == 2)
            puts("Error: Tokenise Fault.");

        /* Add the input to the history if it is not history invocation. */
        if(copy[0] != '!')
            add_history(copy);

		/* Exit command received. */
        if (strcmp(cmdStr[0],"exit") == 0){
            puts("Exiting the Shell");
            return;
        }
        else {
            commands();
        }
    }
}


int main()
{
    char cwd[1024]; /* Variable to hold the path name of working directory. */
    const char *path; /* Variable to store the environment string. */
    int i = 0;

    path = getenv("PATH"); /* Get system path. */
    printf("%s\n",path); /* Print system path. */

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(stdout, "Current working dir: %s\n", cwd);
    else
        perror("getcwd() error");

    chdir(getenv("HOME")); /* Change to HOME directory. */

    history_counter = 0;
    alias_counter = 0;

	while(i < SIZE(alias_array)){
		alias_array[i].alias_name = malloc(sizeof(alias_array[i].alias_name));
		alias_array[i].alias_cmd = malloc(sizeof(alias_array[i].alias_cmd));
		i++;
	}

    open_history();
    pwd();
    user_input();
    chdir(getenv("HOME")); /* Change to HOME directory. */
    save_history();
    setenv("PATH",path,1); /* Restore original value of PATH environment parameter. */
    printf("%s\n",getenv("PATH")); /* Print system path. */
    chdir(cwd); /* Change back to original working directory. */
    pwd();

    return 0;
}
