/*
 HA-1 Main.cc
 
 Copyright: Copyright (C) 2013 Mitsuru Nakada All Rights Reserved.
 License: GNU GPL v2
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "HA1Control.h"

HA1Control HA1Control;

void TrapSignal(int no) {
  HA1Control.SetTrapFlag(no);
}

void TrapSignalWatchdog(int no) {
}
  
int WatchDog(pid_t pid) {

  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = TrapSignalWatchdog;
  sa.sa_flags |= SA_RESTART;
  if(sigaction(SIGINT, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }
  if(sigaction(SIGHUP, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }
  if(sigaction(SIGTERM, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }
  if(sigaction(SIGCHLD, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }
  if(sigaction(SIGALRM, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }
  if(sigaction(SIGUSR1, &sa, NULL) != 0) {
    fprintf(stderr, "sigaction error\n");
    return -1;
  }

  sigset_t sigSet;
  sigemptyset(&sigSet);
  sigaddset(&sigSet, SIGINT);
  sigaddset(&sigSet, SIGHUP);
  sigaddset(&sigSet, SIGTERM);
  sigaddset(&sigSet, SIGCHLD);
  sigaddset(&sigSet, SIGALRM);
  sigaddset(&sigSet, SIGUSR1);
  int signal;
  
  struct itimerval tm;
  tm.it_value.tv_sec = WatchDogInterval;
  tm.it_value.tv_usec = 0;
  tm.it_interval.tv_sec = WatchDogInterval;
  tm.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &tm, NULL);
  
  timeval lastKeepAliveTime;
  gettimeofday(&lastKeepAliveTime, NULL);
  while(1) {
    sigwait(&sigSet, &signal);
    if(signal == SIGINT) break;
    if(signal == SIGHUP) break;
    if(signal == SIGTERM) break;
    if(signal == SIGCHLD) break;
    if(signal == SIGINT) break;
    
    if(signal == SIGUSR1) gettimeofday(&lastKeepAliveTime, NULL);
    if(signal == SIGALRM) {
      timeval now;
      gettimeofday(&now, NULL);
      timersub(&now, &lastKeepAliveTime, &now);
      if(now.tv_sec > 2 * WatchDogInterval) kill(pid, SIGTERM);
      if(now.tv_sec > 3 * WatchDogInterval) kill(pid, SIGKILL);
    }
  };
  if(signal == SIGTERM) kill(pid, SIGTERM);
  int stat;
  wait(&stat);
  return signal;
}

int main(int argc, char **argv) {

  int daemonFlag = 0;
  int pidFile = 0;
  int debug = 0;
  int config = 0;
  int error = 0;
  for(int i = 1; i < argc; i++) {
#ifdef ENABLE_DAEMON
    if(!strcmp(argv[i], "-d")) {
      daemonFlag = 1;
      continue;
    }
    if(!strcmp(argv[i], "-p")) {
      pidFile = i + 1;
      i++;
      if(i >= argc) error = 1;
      continue;
    }
    if(!strcmp(argv[i], "-debug")) {
      debug = 1;
      continue;
    }
#endif
    if(!config) {
      config = i;
      continue;
    }
    error = 1;
  }

  if(error || !config) {
#ifdef ENABLE_DAEMON
    fprintf(stderr, "usage : %s [-d] [-p <pidFile>] <config file>\n", argv[0]);
    fprintf(stderr, "       -d : daemon\n");
#else
    fprintf(stderr, "usage : %s <config file>\n", argv[0]);
#endif
    return -1;
  }

  error = HA1Control.ReadConfFile(argv[config]);
  if(error < 0) return error;

#ifdef ENABLE_DAEMON
  if(daemonFlag) {
    if(daemon(0, debug) != 0) {
      fprintf(stderr, "daemon error\n");
      return -1;
    }

    if(pidFile) {
      pid_t pid = getpid();
      FILE *fp = fopen(argv[pidFile], "w+");
      if(fp) {
        fprintf(fp, "%d\n", pid);
        fclose(fp);
      } else {
        fprintf(stderr, "pid file error\n");
        return -1;
      }
    }

    while(1) {
      pid_t pid = fork();
      if(!pid) break;
      if(pid == -1) {
        fprintf(stderr, "fork error\n");
        return -1;
      }
      if(WatchDog(pid) == SIGTERM) return -1;
    }
  }
#endif

  return HA1Control.Connect(daemonFlag);
}
