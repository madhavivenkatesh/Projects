#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* a help function that 
 *  * open the given device and use dup2 to redirect the fno to the device */
int redirect(int fno, char dev[], mode_t mode);

/*
 *  * a helper function that
 *   * find the full path for the executable file in the PATH environment variables
 *    * */
int resolve(char name[], char fullPath[], int capacity);

/* a helper function to fork a process an execute the command */
void process(char command[]);

/* return the address in the command after trimming empty spaces in the command*/
char *trim(char command[]);

int main() {

  /* redirect the standand IO to /dev/tty */
    if (redirect(STDOUT_FILENO, "/dev/tty", O_WRONLY | O_CREAT) == -1) {
        return -1;
	  }

if (redirect(STDIN_FILENO, "/dev/tty", O_RDONLY) == -1) {
return -1;
}

while (1) {
char line[256]; /* assume the longest command will have less than 256 characters */
char *command;

printf ("utdash$ ");
fgets(line, sizeof(line), stdin);
command = trim(line);
process(command);
}

return 0;
}

/* a helper function to fork a process an execute the command */
void process(char command[]) 
{

char fullPath[PATH_MAX];
char *params[64]; /* assume the maximum number of params is 64 */
char *lasts = NULL;
char *path = strtok_r(command, " ", &lasts);
int child;
int i;

	if (path == NULL) { /* an empty line */
	return;
	}

	if (strstr(path, "/") == NULL) {
	/* just the name, so resolve the path from the PATH environment variables */
		if (resolve(path, fullPath, sizeof(fullPath)) == 0) {
		printf ("-utdash: %s: command not found\n", path);
		return;
		}
		
path = fullPath;
	}

/* fork a child process to execute the command */
child = fork();
	if (child == -1) {
	printf ("Error: cannot create a process to execute the command.\n");
	return;
	}

	if (child != 0) { /* parent process returns when the child is done */
	int stat = 0;
	waitpid(child, &stat, 0);
	return;
	}

/* child process */
/* check redirection commands, which could be either one of these
*    *  > filename
*       *  >> filename
*          *  < filename
*             * */
params[0] = fullPath;
i = 1;

while ((params[i] = strtok_r(NULL, " ", &lasts)) != NULL) {
i ++;
}

/* check the last two */
	if (i > 2) {
		if (strcmp(params[i - 2], ">") == 0) {
		/* redirect the output to the file */
		if (redirect(STDOUT_FILENO, params[i - 1], O_WRONLY | O_CREAT) == -1) {
		printf ("Error: could not redirect to the file.\n");
		exit(0);
		}
	}

	if (strcmp(params[i - 2], ">>") == 0) {
	/* redirect the output to the file */
		if (redirect(STDOUT_FILENO, params[i - 1], O_WRONLY | O_CREAT | O_APPEND) == -1) {
		printf ("Error: could not redirect to the file.\n");
		exit(0);
		}
	}

	if (strcmp(params[i - 2], "<") == 0) {
	/* redirect the output to the file */
		if (redirect(STDIN_FILENO, params[i - 1], O_RDONLY) == -1) {
		printf ("Error: could not redirect tfrom the file.\n");
		exit(0);
		}
	}
params[i - 2] = NULL;
i = i - 2;
}

/* now call execl to execute it */
	if (execv(path, params) == -1) {
	printf ("Error: could not execute the command. (%d)\n", errno);
	exit(0);
	}
}
int resolve(char name[], char fullPath[], int capacity) {
char *paths = getenv("PATH");
char *lasts = NULL;
char *path;
if (paths == NULL) {
return 0;
}
paths = strdup(paths); /* create a copy of the PATH so we can change it using strtok_r */
/* enumerate all the paths */
for (path = strtok_r(paths, ":", &lasts); path; path = strtok_r(NULL, ":", &lasts)) {
struct stat st;
snprintf (fullPath, capacity, "%s/%s", path, name);
fullPath[capacity] = '\0';
/* check whether this full path exist */
	if (stat(fullPath, &st) == -1) { /* not exist */
	continue;
	}
	/* check whether it is an executable file */
	if (st.st_mode & S_IXUSR) { /* found */
	free(paths);
	return 1;
	}
}

free(paths);
return 0; /* not found */
}


int redirect(int fno, char file[], mode_t mode) {
/* open the device */
int nfno = open (file, mode, 0666);

	if (nfno == -1) {
	fprintf (stderr, "Error: could not open device %s (%d).\n", file, errno);
	return -1;
	}

	if (dup2(nfno, fno) == -1) {
	fprintf (stderr, "Error: could not redirect to the devie %s (%d).\n", file, errno);
	close(nfno);
	return -1;
	}

return nfno;
}


/* return the address in the command after trimming empty spaces in the command*/
char *trim(char command[]) {
int len;
char *ret = command;
/* remove space at beginning*/
while (*ret == ' ') ret++;

/* remove space at the end */
len = strlen(ret);
while (len > 0 && (ret[len - 1] == ' ' || ret[len - 1] == '\n')) {
ret[len - 1] = '\0';
len --;
}

return ret;
}





