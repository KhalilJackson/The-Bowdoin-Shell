/* 
 * bsh - the Bowdoin Shell
 * 
 * <Khalil Jackson, Diyaa Yaqub>
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>

/* Misc constants */
#define MAXLINE    1024   /* max command line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */

/* Job state constants */
#define UNDEF 0 /* undefined (not an active job) */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* The job struct */
typedef struct job_t {
    pid_t pid;              /* process ID of starting process in the job */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* current job state: UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line that launched the job */
} job_t;

/* Global variables */
job_t jobs[MAXJOBS];        /* The job list */
int nextjid = 1;            /* next job ID to allocate */
int verbose = 0;            /* whether to print verbose output */
char prompt[] = "bsh> ";    /* command line prompt */
extern char** environ;      /* needed for execve */

/* Function prototypes */

/* Core shell functions */
void eval(char* cmdline);
int parseline(const char* cmdline, char** argv); 
int builtin_cmd(char** argv);
void do_bgfg(char** argv);
void waitfg(pid_t pid);

/* Signal handlers */
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sigquit_handler(int sig);

/* Job list helper functions */
void clearjob(job_t* job);
void initjobs(job_t* jobs);
int maxjid(job_t* jobs); 
int addjob(job_t* jobs, pid_t pid, int state, char* cmdline);
int deletejob(job_t* jobs, pid_t pid); 
pid_t fgpid(job_t* jobs);
job_t* getjobpid(job_t* jobs, pid_t pid);
job_t* getjobjid(job_t* jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(job_t* jobs);

/* Other helper functions */
void safe_printf(const char* format, ...);
void error(char* msg);
typedef void handler_t(int);
handler_t* Signal(int signum, handler_t* handler);
void print_usage();

/*
 * main - The shell's main routine 
 */
int main(int argc, char** argv) {
  char cmdline[MAXLINE]; /* buffer to hold a line of input */
  int emit_prompt = 1; /* by default, print shell prompts */

  /* Necessary for the driver to receive all shell output */
  dup2(1, 2);

  /* Parse the command line */
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
      case 'h':             /* print help message */
        print_usage();
        break;
      case 'v':             /* emit additional diagnostic info */
        verbose = 1;
        break;
      case 'p':             /* don't print a prompt */
        emit_prompt = 0;  /* handy for automatic testing */
        break;
      default:
        print_usage();
        break;
    }
  }

  /* Install the signal handlers */

  Signal(SIGINT,  sigint_handler);  /* ctrl-c */
  Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
  Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */
  Signal(SIGQUIT, sigquit_handler); /* kill the shell on SIGQUIT */

  /* Initialize the job list */
  initjobs(jobs);

  /* Execute the shell's read/eval loop */
  while (1) {

    /* print command prompt, if enabled */
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    /* Read command line from stdin (i.e., regular user input) */
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      error("fgets error");
    }

    /* Typing ctrl-d indicates EOF (end-of-file); quit the shell */
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    /* Evaluate the command line */
    eval(cmdline);

    /* Make sure all output has been printed before continuing */
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); /* control should never reach here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg, or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job using the child. If the job is to run in the foreground,
 * wait for it to terminate before returning eval.
*/
void eval(char* cmdline) {
  // TODO - implement me!

char* argv[MAXARGS]; // a local array that holds MAXARGS pointers
int bg = parseline(cmdline, argv);

sigset_t mask; 
sigemptyset(&mask);
sigaddset(&mask, SIGCHLD); 

/* Since you are working in C, you will need to create a separate buffer to hold the (larger, prepended) program name. 
Declare the buffer, copy the "/bin/" string into it using strcpy, 
then append the original program name (e.g., "ls") using strcat
*/
//todo UM DID I DO THIS CORRECTLY...

//check if it is a builtin
//if builtin, runs immeditely
int builtIn = builtin_cmd(argv);
if (builtIn) {
	return;
}

char args = argv[0][0];
int argsSize = 0; 
while (args) {
	argsSize++;
	args++;
}
char buffer[argsSize + 6]; //seperate buffer
if (argv[0][0] != '.' && argv[0][0] != '/') {
	strcpy(buffer, "/bin/"); 
	strcat(buffer, argv[0]);
	argv[0] = buffer; 
}

//if its not builtin
if (builtIn == 0) {
	sigprocmask(SIG_BLOCK, &mask, NULL); //block sigchild
	pid_t pidC = fork();
	
	if (pidC == 0) { // if child process
		sigprocmask(SIG_UNBLOCK, &mask, NULL); //if its a child, unblock 
		setpgid(0, 0);
		execve(argv[0], argv, environ); 
	} else {
		//parent (shell process)
		//sigprocmask(SIG_UNBLOCK, &mask, NULL); //unblcok sigchild           
		if (bg == 0) { //fg
			addjob(jobs, pidC, FG, cmdline);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			waitfg(pidC);
		}  else {
			addjob(jobs, pidC, BG, cmdline);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			//[20] (500) /bin/ls -l
			int jid = pid2jid(pidC); 
			safe_printf("[%d] (%d) %s", jid, pidC, cmdline);
		}
	}	

	//the parent is the shell prcess
	//if its a parent
	//they need to wait for it to be a 
	//parent adds the new job 
	//wait for 

	//sig suspend  puts shell to sleep until a 
	///when you wake up from sig suspend to check if the foreground 
	//use the jobs list to see if its actually done 

	//when the child finishes, a sigchild will be 
	//sig child will update it appropriately
	//u are still in wait fg, watching hte jobs list to wait for the update 
	//IN WAIT FG you are watching the jobs list, you are actually modifying it when teh sigchild handler runs
	//we will see when it is reflected

	//parent continues to run bsh
//child calls execve
//parent adds to addjob
} 
 return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a background
 * job or false if the user has requested a foreground job.  
 */
int parseline(const char* cmdline, char** argv) {
  static char array[MAXLINE]; /* holds local copy of command line */
  char* buf = array;          /* ptr that traverses command line */
  char* delim;                /* points to first space delimiter */
  int argc;                   /* number of args */
  int bg;                     /* background job? */

  strcpy(buf, cmdline);
  buf[strlen(buf) - 1] = ' ';  /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) { /* ignore leading spaces */
    buf++;
  }

  /* Build the argv list */
  argc = 0;
  if (*buf == '\'') {
    buf++;
    delim = strchr(buf, '\'');
  } else {
    delim = strchr(buf, ' ');
  }

  while (delim) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) { /* ignore spaces */
      buf++;
    }

    if (*buf == '\'') {
      buf++;
      delim = strchr(buf, '\'');
    } else {
      delim = strchr(buf, ' ');
    }
  }
  argv[argc] = NULL;

  if (argc == 0) {  /* ignore blank line */
    return 1;
  }

  /* should the job run in the background? */
  if ((bg = (*argv[argc-1] == '&')) != 0) {
    argv[--argc] = NULL;
  }
  return bg;
}

/* 
 * builtin_cmd - If user types a built-in command, execute it immediately.
 *    Returns true if a built-in command was specified or false otherwise.
 */
int builtin_cmd(char** argv) {
  char* cmd = argv[0];
  if (!strcmp(cmd, "quit")) { /* quit command */
    exit(0);  
  }
  if (!strcmp(cmd, "jobs")) { /* jobs command */
    listjobs(jobs);
    return 1;    
  }
  if (!strcmp(cmd, "bg") || !strcmp(cmd, "fg")) { /* bg and fg commands */
    do_bgfg(argv);
    return 1;
  }
  if (!strcmp(cmd, "&")) { /* ignore & by itself */
    return 1;
  }
  return 0;     /* otherwise, not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands.
 */
void do_bgfg(char** argv) {
  // TODO - implement me!

int jid = 0; 

pid_t pid = 0; 

job_t* job;
//if given jid
if (argv[1][0] == '%') {
char* jidS = argv[1] + 1;
jid = atoi(jidS);
job = getjobjid(jobs, jid);
} else {
pid = atoi(argv[1]);
} 


//if given jid is not valid
if (jid != 0) {
	if (job == NULL) {
		safe_printf("%%[%d]: No such job", jid);
	}
}
//} else {
//if given pid not valid



//pid_t pid = job->pid;


//see if pid is null and print message
//if (== NULL) {
//	safe_printf("(500): No such process");
//}


//jid has percentage
//pid has no percentage



if (argv[0][0] == 'b') {
//send sigcont to stopped bg job and continues running in bg

	//checks if running bg without a job
	if (argv[1] == NULL) {
		safe_printf("bg command requires PID or %%jobid argument/n");
	}

	//bg: argument must be a PID or %jobid
	if (jid == 0 && pid == 0) {
		safe_printf("bg: argument must be a PID or %%jobid");
	}

	//get pid of the job number
	job = getjobjid(jobs, jid);
	job->state = BG;
	safe_printf("[%d] (%d) %s", jid, job->pid, job->cmdline);
	kill(job->pid , SIGCONT);
}

else if (argv[0][0] == 'f') {
	//checks if running fg without a job
	if (argv[1] == NULL) {
	 safe_printf("fg command requires PID or %%jobid argument");
	}


	//if can't be converted to an int (not a pid or jid) 
 //      safe_printf("fg: argument must be a PID or %jobid");


//send stopped or running bg a SICONT TO resume (if stopped) and continue running in fg
job = getjobjid(jobs, jid);
if (job->state == ST) {
	kill(job->pid , SIGCONT);
}
job->state = FG;
waitfg(job->pid);
}

//safe_printf("[%d] (%d) %s", jid, job->pid, job->cmdline);
//}


//argv[0] is bg and argv[1] is job


//jobs
//if (argv[0] == "bg") {


//getjobjid()

//getjobpid()


//error: running bg without specifying job

//invalid jid to bg

//invlaid jid to bg

//specifying smth other than pid or jid for bg


//bg job


//} 

//if (argv[0] == "fg") {

//}

//if bg <job>
//bg command: stopped --> bg

//check if argv is jid or pid
//get jobpid method
//get jobjid method

//after you get the job, 

//if fg <job>
//send sigcont
//fg command: 
//stopped --> fg
//bg --> fg



//THESE TWO WILL BE JUST TAKEN FROM COMMAND LINE AND CALL THE HANDLERS THEMSELCES
//(THESE SHOULD BE IN THE METHOD)
//if ctrlc: SIGINT delivered to fg job (all processess) 
//if ctrlz: SIGSTP delivered to fg. makes it into stopped 
  return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process.
 */
void waitfg(pid_t pid) {
// TODO - implement me!
sigset_t set;
sigemptyset(&set);
job_t* curJob = getjobpid(jobs, pid); 

//int success = sigsuspend(&set); 
while (curJob->state ==  FG) {
	sigsuspend(&set);
}
//return when process is undefined in jobs list
//until then, keep sigsuspending
 return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler should reap
 *     all available zombie children, but should not wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) {
  // TODO - implement me!

//SIGTSTP,SIGSTOP  suspend

//if it is terminated, then we want to reap 

pid_t pid; 
int stat; 
int jid; 

while ((pid = waitpid(-1, &stat, WNOHANG|WUNTRACED)) > 0){
jid = pid2jid(pid); 

//if child is stopped
if (WIFSTOPPED(stat)) {
safe_printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, WEXITSTATUS(stat));
getjobjid(jobs,jid)->state = ST; 
} else {
	if (WIFSIGNALED(stat)) {
	//child terminated by signal
	safe_printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, WEXITSTATUS(stat));
	}
	//only do this when terminated
	//while there are children to be reaped
	deletejob(jobs, pid); 
}

}; 

//need to print out a message depending on which sig it was terminated by 



//when the handler is called, this means there sigchild signal sent
//there must be at least a suspended OR terminated process for this to be called

//to detect a suspended process, use waitpid, and update the job list


//confused?? what does it mean to reap?? 
  return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenever user
 *    types ctrl-c at the keyboard. Forward it to the foreground job.
 */
void sigint_handler(int sig) {
  // TODO - implement me!

//finding fg job
int fpid = fgpid(jobs); 
if (fpid != 0) {
	//forwarding to fg job
	kill(-fpid,sig);
}
return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     user types ctrl-z at the keyboard. Forward it to the foreground job.
 */
void sigtstp_handler(int sig) {
  // TODO - implement me!
 
int fpid = fgpid(jobs); //find pid of fg job 
if (fpid != 0) {
        //forwarding to fg job
        kill(-fpid,sig);
	job_t* job = getjobpid(jobs, fpid);	 
	job->state = ST;
}
 return;
}

/*
 * sigquit_handler - Terminate the shell on receipt of a SIGQUIT.
 *    Used by the driver program; do not modify.
 */
void sigquit_handler(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct. */
void clearjob(job_t* job) {
  job->pid = 0;
  job->jid = 0;
  job->state = UNDEF;
  job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list. */
void initjobs(job_t* jobs) {
  for (int i = 0; i < MAXJOBS; i++) {
    clearjob(&jobs[i]);
  }
}

/* maxjid - Returns largest allocated job ID */
int maxjid(job_t* jobs) {
  int max = 0;
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].jid > max) {
      max = jobs[i].jid;
    }
  }
  return max;
}

/* addjob - Add a job to the job list. Return true if
 *    the job was successfully added or false otherwise.
 */
int addjob(job_t* jobs, pid_t pid, int state, char* cmdline) {
  /* pid must be >0 */
  if (pid < 1) {
    return 0;
  }
  /* find an available slot in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextjid++;
      if (nextjid > MAXJOBS) {
        nextjid = 1;
      }
      strcpy(jobs[i].cmdline, cmdline);
      if (verbose) {
        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
      }
      return 1;
    }
  }
  /* no available slots in the job list */
  printf("Tried to create too many jobs\n");
  return 0;
}

/* deletejob - Delete a job with the given pid from the job list. Return true
 *    if the job was deleted or false otherwise.
 */
int deletejob(job_t* jobs, pid_t pid) {
  /* pid must be >0 */
  if (pid < 1) {
    return 0;
  }
  /* find the specified pid in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      nextjid = maxjid(jobs)+1;
      return 1;
    }
  }
  /* no job with the specified pid */
  return 0;
}

/* fgpid - Return PID of current foreground job or 0 if there is no
 *    foreground job.
 */
pid_t fgpid(job_t* jobs) {
  /* look for a foreground job in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].state == FG) {
      return jobs[i].pid;
    }
  }
  /* no foreground job in the job list */
  return 0;
}

/* getjobpid - Find a job (by PID) on the job list. Return the
 *    matching job or NULL if not found.
 */
job_t* getjobpid(job_t* jobs, pid_t pid) {
  /* pid must be >0 */
  if (pid < 1) {
    return NULL;
  }
  /* look for specified pid in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      return &jobs[i];
    }
  }
  /* didn't find the specified pid */
  return NULL;
}

/* getjobjid - Find a job (by JID) on the job list. Return the
 *    matching job or NULL if not found.
 */
job_t* getjobjid(job_t* jobs, int jid) {
  /* jid must be >0 */
  if (jid < 1) {
    return NULL;
  }
  /* look for specified jid in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].jid == jid) {
      return &jobs[i];
    }
  }
  /* didn't find the specified jid */
  return NULL;
}

/* pid2jid - Find the job ID of the job with the specified
 *    process ID. Return 0 if not found.
 */
int pid2jid(pid_t pid) {
  /* pid must be >0 */
  if (pid < 1) {
    return 0;
  }
  /* look for specified pid in the job list */
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  }
  /* didn't find the specified pid */
  return 0;
}

/* listjobs - Print the job list */
void listjobs(job_t* jobs) {
  for (int i = 0; i < MAXJOBS; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
        case BG: 
          printf("Running ");
          break;
        case FG: 
          printf("Foreground ");
          break;
        case ST: 
          printf("Stopped ");
          break;
        default:
          printf("listjobs: Internal error: job[%d].state=%d ", 
              i, jobs[i].state);
          break;
      }
      printf("%s", jobs[i].cmdline);
    }
  }
}

/***********************
 * Other helper routines
 ***********************/

/* safe_printf – version of printf that's safe to use in signal handlers.
 *    Use this rather than printf itself inside signal handlers.
 */
void safe_printf(const char* format, ...) {
  char buf[MAXLINE];
  va_list args;

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  write(1, buf, strlen(buf)); /* write is async-signal-safe */
}

/*
 * error - convenience error routine.
 *    Outputs a message along with the error message indicated by errno
 *    (which is set by most system calls), and then exits the shell.
 *    Uses safe_printf so safe to call from within signal handlers.
 */
void error(char* msg) {
  safe_printf("%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * Signal - wrapper for the sigaction function.
 *    Associates the given signal to the given signal handler function.
 */
handler_t* Signal(int signum, handler_t* handler) {
  struct sigaction action, old_action;

  action.sa_handler = handler;  
  sigemptyset(&action.sa_mask); /* block sigs of type being handled */
  action.sa_flags = SA_RESTART; /* restart syscalls if possible */

  if (sigaction(signum, &action, &old_action) < 0) {
    error("Signal error");
  }
  return (old_action.sa_handler);
}

/*
 * print_usage - print a help message
 */
void print_usage() {
  printf("Usage: shell [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

