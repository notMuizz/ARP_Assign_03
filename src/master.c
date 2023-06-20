#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

FILE* logfile;
time_t curtime;

char *master_a="/tmp/master_a";
char *master_b="/tmp/master_b";
int fd_ma,fd_mb;

int other_states;

pid_t a,b;

void controller(int function,int line){

  ctime(&curtime);
  if(function==-1){
    fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),line);
    fflush(stderr);
    fprintf(logfile,"Error: %s Line: %d\n",strerror(errno),line);
    fflush(logfile);
    exit(EXIT_FAILURE);
  }
}

void sa_function(int sig){

  ctime(&curtime);

  if(sig==SIGINT){

    unlink(master_a);
    unlink(master_b);


    controller(other_states=kill(a,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole process A terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    controller(other_states=kill(b,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole process B terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    fprintf(logfile,"TIME: %s Master terminated\n",ctime(&curtime));
    fflush(logfile);

    fclose(logfile);

    printf("\nMaster terminated\n");
    fflush(stdout);

    exit(EXIT_SUCCESS);


  }
}

int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid==-1)
    exit(EXIT_FAILURE);

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    time(&curtime);
    if(execvp (program, arg_list) == 0);
      fprintf(stderr,"ERROR: %s in line %d\n",strerror(errno),__LINE__);
      fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Check in line %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
      fflush(logfile);
      fclose(logfile);
      exit(EXIT_FAILURE);
  }
}

int main() {

  time(&curtime);
  pid_t process;
  logfile=fopen("./logfiles/master_logfile","w"); 


  struct sigaction sa;
    
  memset(&sa,0,sizeof(sa));
  //sigemptyset(&stop_reset.sa_mask);
  sa.sa_flags=SA_RESTART;
  sa.sa_handler=&sa_function;
  controller(sigaction(SIGINT,&sa,NULL),__LINE__);


  //pipes for pids processes
  controller(mkfifo(master_a,0666),__LINE__);
  controller(mkfifo(master_b,0666),__LINE__);

  //opening pipes

  controller(fd_ma=open(master_a,O_RDONLY | O_NONBLOCK),__LINE__);
  controller(fd_mb=open(master_b,O_RDONLY | O_NONBLOCK),__LINE__);

  //ending opening pipes
  char * arg_list_A[] = { "/usr/bin/konsole","--hold", "-e", "./bin/processA", NULL };
  char * arg_list_B[] = { "/usr/bin/konsole", "--hold","-e", "./bin/processB", NULL };

  pid_t pid_procA = spawn("/usr/bin/konsole", arg_list_A);
  sleep(1);
  pid_t pid_procB = spawn("/usr/bin/konsole", arg_list_B);

  fprintf(logfile,"TIME: %s Processes spawned\n",ctime(&curtime));
  fflush(logfile);

  sleep(3);
  //reading pipes

  controller(read(fd_ma,&a,sizeof(pid_t)),__LINE__);
  controller(read(fd_mb,&b,sizeof(pid_t)),__LINE__);

  //ending reading pipes

  //starting unlinking pipes

  controller(unlink(master_a),__LINE__);
  controller(unlink(master_b),__LINE__);

  //ending unlinking pipes


  //closing file descriptors
  close(fd_ma);
  close(fd_mb);

  int status;

  controller(process=wait(&status),__LINE__);
  
  time(&curtime);
  if(WIFEXITED(status)==0){
    printf("First process has terminated with PID: %d and status: %d\n",process,WEXITSTATUS(status));
    fflush(stdout);
    fprintf(logfile,"TIME: %s First process has terminated with PID: %d and status: %d\n",ctime(&curtime),process,WEXITSTATUS(status));
    fflush(logfile);   
  }
  else{
    fprintf(stderr,"First process has terminated with PID: %d and wrong status: %d\n",process,status);
    fflush(stderr);
    fprintf(logfile,"TIME: %s First process has terminated with PID: %d and status: %d\n",ctime(&curtime),process,status);
    fflush(logfile);
  }

  time(&curtime);
  if(process==pid_procA){
    controller(other_states=kill(b,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Process B terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    controller(other_states=kill(pid_procB,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole process B terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);
  }
  else{
    controller(other_states=kill(a,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Process A terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    controller(other_states=kill(pid_procA,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole process A terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);
  }


  //closing all resources
  time(&curtime);
  fprintf(logfile,"TIME: %s Master terminated\n",ctime(&curtime));
  fflush(logfile);

  fclose(logfile);

  printf("\nMaster terminated\n");
  fflush(stdout);

  return 0;
}