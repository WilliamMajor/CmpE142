#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

void ls(char *input);
void cd(char *input);
void redirect(char *position, char* position2, char * input, char directory[]);

int
main(int argc, char **argv)
{
    char *line = NULL;
    size_t linesize = 0;
    size_t linelen;
    int rc;f
    while ((linelen = getline(&line, &linesize, stdin)) != -1) //Want to figure out a way to pring out the shell name before this each time
    {
        if (strncmp("exit", line, 4) == 0) 
        {
            exit(0); // user wants to exit
        }

        else if (strncmp("ls", line, 2) == 0)
        {
            ls(line); // user wants to ls 
        }
	    else if (strncmp("cat", line, 3) == 0)
        {
            cat(line);
        }
        else if (strncmp("$PATH", line, 5) == 0)
        {
            path(line);
        }  
        else if (strncmp("cd", line, 2) == 0)
        {
            cd(line); // user wants to change directory
        }
        else if (strncmp("\n", line, 2) == 0)
        {
            //nothing called
            //do nothing
        }
        else
        {
            line[strcspn(line, "\n")] = 0;
            printf("error command: %s not found\n", line);
        }
        

    }
    free(line);
    if (ferror(stdin))
        err(1, "getline");    
}


/********************************* Command Functions **********************************/
void ls(char *input)
{
    int rc;
    char cwd[256];
    getcwd(cwd,100);
    strcat(cwd,"/");
    rc = fork();
    if (rc < 0)//error
    {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0)//Child
    {
        int pid = (int) getpid();
        if (access("/bin/ls",X_OK) == -1) // make sure the ls command is where we think it is
        {
            if(access("/usr/bin/ls",X_OK) == -1) // if we dont see the ls command here either we have an error
            {
                printf("error no ls command found in /bin/ls or /user/bin/ls");
                exit(-1);
            } 
        } 
        else
        {   
            char * position = strchr(input, '>');
            char * position2 = strchr(input, ' ');
            /* this still needs a lot of work -_- need to send everything between ls and > 
            get into that directory then open and write to a file specified after > 
            in there with the output of ls probably need to parse the file we also need to handle writing multiple files
            like this test ls > output.9 > output.10 */
            if(position)// we found a need for an output redirect
            {

                redirect(position, position2, input, cwd);
                
            }
            else if (strlen(input) > 3)
            {
                memmove(input, input + 3, strlen(input));
                input[strcspn(input, "\n")] = 0;
                char *argm[] = {"ls", input, NULL};
                execvp(argm[0], argm);
                printf("wow errors");
            }
            else
            {
                char *argm[] = {"ls", NULL};
                execvp(argm[0], argm);
                printf("obviously there was an error here, damn");
            }
            kill(pid,SIGTERM);
        }
        
    }
    else //Parent
    {
        wait(NULL);
    }
}

void cd(char *input) //cd command Think this is pretty done...Nope found a bug not adding in initial / to path when the path has other /s...
{
    char s[100];
    char sep[50];
    char newinput[100];
    bool notspacesonly = true;
    
    strcpy(sep,"/");
    memmove(input, input + 3, strlen(input));
    strcpy(newinput,input);

    if(strlen(newinput) > 1)
    {
        if(strncmp("  ", newinput,2) == 0)
        {
            notspacesonly = false;
            chdir(getenv("HOME"));
        }
        else
        {
            notspacesonly = true;
        }
        
    }
    if(strlen(newinput) == 0 || strlen(newinput) == 1)
    {
        chdir(getenv("HOME"));
    }
    else if (notspacesonly)
    {
        getcwd(s, 100); 

        newinput[strcspn(newinput, "\n")] = 0;
        s[strcspn(s, "\n")] = 0;
        sep[strcspn(sep, "\n")] = 0;
        
        strcat(sep,newinput);
        s[strcspn(s, "\n")] = 0;
        strcat(s,sep);
    
        if(chdir(s) == -1)
        {
            printf("bash: cd: %s: No such file or directory\n", newinput);
        }
    }
}

void redirect(char * position, char * position2, char * input, char directory[])
{
    int forkreturn;
    int fd;
    int save_out;
    char * holder;
    char * filelocation;

    if (strlen(position) != strlen(position2)-1) //check if we need to change directory or not
    {  
        memmove(input, input + 3, strlen(input)); //gets rid of ls and the space after it
        strcpy(filelocation, input);
        holder = strchr(input, '>');
        filelocation[strlen(filelocation) - (strlen(holder) + 1)] = 0; // add terminating character after our file location

        if(chdir(filelocation) == -1)
        {
            printf("bash: cd: %s: No such file or directory\n", filelocation);
        }
        //ok we're in the right directory now we need to write the result of ls to it
    }
    forkreturn = fork(); 
    if (forkreturn == 0)//child
    {   
        char * newholder;
        char * filename;

        holder = strchr(input, '>');
        
        memmove(holder, holder + 2, strlen(holder));
        holder[strcspn(holder, "\n")] = 0;
        strcat(directory,holder);

        fd = open(directory, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        
        save_out = dup(fileno(stdout));

        dup2(fd, fileno(stdout));

        char *argm[] = {"ls", NULL};
        execvp(argm[0], argm);
        printf("failed");
    }
    else //parent
    {
        wait(NULL);
        fflush(stdout); close(fd);
        close(save_out);
    }

}

void cat(char *input[])
{
	int rc, fr, t;

	rc = fork();

	if(rc < 0)
	{
		printf("failed");;
		exit(1);
	}
	else if (rc == 0)
	{

		fr = open(input, O_RDONLY);

		if(fc < 0)
		{
			printf("failed");;
			exit(1);
		}

		while(read(fr, &t, 1))
			write(STDOUT_FILENO, &t, 1); //one byte at a time

		close(fc);

	}
	else
	{
		wait(NULL);
	}
    return 0;
}

void path(char * input[])
{
    //still working on a separate file.


}

