/*
prics.c
A part of prics, a system for controlling CounterStrike-servers from the web
Written by Kung den Knege (Eli Kaufman) for Joel Nygårds
Contact the author att kungdenknege@gmail.com

Compile by (for example): cc -o prics prics.c

Usage: prics <outputfile> <IP to allow connections from> [port to listen on=1025] [IP top listen on=all]
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// !!! THE COMMAND TO RUN
#define COMMAND "/usr/games/bin/hlds-something"

#define STATE_NOCONTROL  0
#define STATE_TIMECONTROL 1
#define STATE_CONTROL 2

#define DEFAULT_PORT 1025
#define ACCEPT_SLEEP 1
#define RECV_TIMEOUT 10

#define PROTOCOL_ARGDELIMITER " "
#define DATA_MAXLEN 100

void error(char* msg);
void killinstance();
void emptyfile(char* file);
void fillfile(char* file, char* rcon, char* pass);
void startinstance();
int getdata(int sock, char* data, int len);

FILE* instance = NULL;

int main(int argc, char** argv) {
  int state = STATE_NOCONTROL;
  int listensocket, consocket;
  int count, timeout, size = sizeof(struct sockaddr_in);
  struct sockaddr_in me, you;

  // Check commandline arguments
  if (argc < 3 || argc > 5)
    error("Usage: prics <outputfile> <ip to accept connections from> [port to listen on] [ip to listen on]");

  // Create, bind and listen
  me.sin_family = AF_INET;
  me.sin_port = (argc > 3) ? (htons(atoi(argv[3]))) : (htons(DEFAULT_PORT));
  me.sin_addr.s_addr = (argc > 4) ? (inet_addr(argv[4])) : INADDR_ANY;
  memset(&(me.sin_addr), '\0', 8);

  if ((listensocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    error("Couldn't create socket");
  fcntl(listensocket, F_SETFL, O_NONBLOCK);
  if (bind(listensocket, (struct sockaddr*)&me, sizeof(struct sockaddr)) < 0)
    error("Couldn't bind to port");
  if (listen(listensocket, 1) < 0)
    error("Couldn't listen on socket");

  // Loop for incoming connections
  for (count = 0;; count++) {
    // Check timeout
    if ((state == STATE_TIMECONTROL) && (count * ACCEPT_SLEEP) > (timeout * 60)) {
      killinstance();
      emptyfile(argv[1]);
      startinstance();
      state = STATE_NOCONTROL;
    }

    // Check for incoming connection
    if ((consocket = accept(listensocket, (struct sockaddr*)&you, &size)) >= 0) {
      // Set recv timeout
      struct timeval tmo;
      tmo.tv_sec = RECV_TIMEOUT;
      tmo.tv_usec = 0;
      if (setsockopt(consocket, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo)) < 0)
	error("Could not set recv timeout");
      
      if (you.sin_addr.s_addr != inet_addr(argv[2])) {
	close(consocket);
	continue; // Client is a cheating SOAB!
      }

      char data[DATA_MAXLEN];
      char* rcon;
      char* pass;
      char* time;

      // Get line of data
      if (getdata(consocket, data, DATA_MAXLEN) < 0)
	continue; // C'mon client, get a grip!

      // Parse data
      rcon = strtok(data, PROTOCOL_ARGDELIMITER);
      pass = strtok(NULL, PROTOCOL_ARGDELIMITER);
      time = strtok(NULL, PROTOCOL_ARGDELIMITER);

      if (rcon == NULL || pass == NULL || time == NULL)
	continue; // Whoops, client doesn't know what he's doing!

      // Fill file, restart instance, go into state
      killinstance();
      emptyfile(argv[1]);
      fillfile(argv[1], rcon, pass);
      startinstance();
      state = (atoi(time) > 0) ? (timeout = atoi(time), count = 0, STATE_TIMECONTROL) : STATE_CONTROL;
    }

    // Sleep
    sleep(ACCEPT_SLEEP);
  }
}

void error(char* msg) {
  fprintf(stderr, "%s%s", msg, "\n");
  exit(1);
}

void killinstance() {
  FILE* stream;
  char c, state, count = 0;
  char pid[10];
  char command[] = COMMAND;
  char cmd[strlen(strtok(command, " ")) + 15];

  // Using ps to grab pid, very ugly but very effective :)
  sprintf(cmd, "ps -e | grep %s", strtok(command, " "));
  if ((stream = popen(cmd, "r")) == NULL)
    error("Couldn't run ps or grep");

  state = 0;
  while ((c = fgetc(stream)) != EOF) {
    if (c == ' ' && state == 0) // Strip leading slashes
      continue;
    if ((c == ' ' && state != 0) || count > 8) // Finished reading pid
      break;
  
    state = 1;
    pid[count++] = c;
  }
  pclose(stream);

  if (count == 0) // Nothing read, no instance running
    return;

  *(pid+count) = '\0';

  if (kill(atoi(pid), SIGTERM) < 0)
    error("Couldn't stop process, probably haven't got the privileges");

  if (instance != NULL) {
    pclose(instance);
    instance = NULL;
  }
}

void emptyfile(char* file) {
  FILE* fil;
  remove(file);

  if ((fil = fopen(file, "w")) == NULL)
    error("Couldn't recreate file");

  fclose(fil);
}

void fillfile(char* file, char* rcon, char* pass) {
  FILE* fil;
  if ((fil = fopen(file, "w")) == NULL)
    error("Couldn't open or create file");

  fprintf(fil, "rcon_password %s\n", rcon);
  fprintf(fil, "sv_pasword %s\n", pass);

  fclose(fil);
}

void startinstance() {
  if ((instance = popen(COMMAND, "r")) == NULL)
    error("Shell wouldn't run specified command");
}

int getdata(int sock, char* data, int len) {
  memset(data, '\0', len);
  recv(sock, data, len-1, 0);
  close(sock);

  if (data[strlen(data) - 1] != '\n')
    return -1;

  data[strlen(data) - 1] = '\0';

  return 0;
}
