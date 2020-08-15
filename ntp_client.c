#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define BUF_LEN 1024

struct mstime {
  time_t sec;
  int msec;
};

int get_ntp_time(struct mstime *mtm) {
  int s;                      
  struct hostent *servhost;   
  struct sockaddr_in server;  
  struct servent *service;    

  char send_buf[BUF_LEN];     
  char host[BUF_LEN];
  char path[BUF_LEN];
  unsigned short port = 80;   

  strcpy(host, "ntp-a1.nict.go.jp");
  strcpy(path, "/cgi-bin/ntp");

  servhost = gethostbyname(host);
  if ( servhost == NULL ) return -1;

  memset(&server, 0, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  bcopy(servhost->h_addr, &server.sin_addr, servhost->h_length);
  server.sin_port = htons(port);

  if ( ( s = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) return -1;

  clock_t send_time = clock();

  if ( connect(s, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1 ) return -1;

  sprintf(send_buf, "GET %s HTTP/1.0\r\n", path);
  write(s, send_buf, strlen(send_buf));
  sprintf(send_buf, "Host: %s:%d\r\n", host, port);
  write(s, send_buf, strlen(send_buf));
  sprintf(send_buf, "\r\n");
  write(s, send_buf, strlen(send_buf));

  char buf[BUF_LEN] = {0}, tmbuf[64];
  read(s, buf, BUF_LEN);

  clock_t end_time = clock();

  char* start = strstr(buf, "<BODY>");
  char* end =   strstr(buf, "</BODY>");

  memcpy(tmbuf, start + 6, end - start - 6);

  double t = atof(tmbuf) + ((double)end_time - send_time) / CLOCKS_PER_SEC / 2;
  mtm->sec = t - 2208988800;
  mtm->msec = (t - ((double)2208988800 + mtm->sec)) * 1000;

  close(s);
}

void localtime_ms(struct mstime *mtm) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  mtm->sec = tv.tv_sec;
  mtm->msec = ((double)tv.tv_usec / CLOCKS_PER_SEC) * 1000.0;
}


void mstime_to_string(struct mstime *mtm, char *date_string) {
  struct tm *date;
  date = gmtime(&mtm->sec);
  date->tm_hour += 9;// Japan time
  strftime(date_string, 0xff, "%Y %d %B %p %I:%M:%S", date);
  sprintf(date_string, "%s:%d", date_string, mtm->msec);
}


int main() {
  struct mstime ntptm, loctm;
  int s_diff, ms_diff;
  char date_string[0xff];

  for(size_t i = 0; ; i++) {
    if(i % 100 == 0) {
      get_ntp_time(&ntptm);
      localtime_ms(&loctm);
      loctm.sec += (s_diff = ntptm.sec - loctm.sec);
      loctm.msec += (ms_diff = ntptm.msec - loctm.msec);
      printf("\x1b[31m");
    } else {
      localtime_ms(&loctm);
      loctm.sec  += s_diff;
      loctm.msec += ms_diff;
    }

    mstime_to_string(&loctm, date_string);
    printf("(diff: s: %d ms: %d) %s\r", s_diff, ms_diff, date_string);

    printf("\x1b[0m");

    fflush(stdout);

    usleep(CLOCKS_PER_SEC / 3);
  }
  return 0;
}
