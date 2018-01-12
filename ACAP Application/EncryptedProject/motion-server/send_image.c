#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/time.h>
#include <fcntl.h>
#include "capture_image.h"
#include "send_image.h"
#include "encryption.h"
#define SERVER_ADDR "192.168.0.7"
#define FPS 30


#if 0
static void intToBuff(int n,unsigned char* buffer) {
	buffer[0] = (n >> 24) & 0xFF;
	buffer[1] = (n >> 16) & 0xFF;
	buffer[2] = (n >> 8) & 0xFF;
	buffer[3] = n & 0xFF;
}
static void writeBytes(unsigned char* dataBytes, int len, int client_sockfd){
	//write 4 bytes for len
	//len =  (int)htonl((unsigned int)len);
	unsigned char lenBytes[4];
	intToBuff(len,lenBytes);
	//write(client_sockfd,&len,sizeof(len));
	write(client_sockfd,lenBytes,4);
	syslog(LOG_INFO,"server write 4 len bytes\n");

	//write data bytes
	if(len > 0)
		write(client_sockfd,dataBytes,len);
	syslog(LOG_INFO,"server write %d data len bytes\n", len);
}
#endif

int send_image(int *fd)
{
  unsigned char *data = NULL;
  int size = 0;
  int sockfd = 0;
  int fps_msec = 1000/FPS;
  struct timeval     tv_start, tv_end;
  int msec1, msec2;
//  struct timeval sleep_dur;

  sockfd = create_socket();

  syslog(LOG_INFO,"connecting to server\n");

//KEY exchange
	int secret = keyExchange(sockfd);
	if(secret <= 0){
		close(sockfd);
		exit(0);
	}
//KEY exchange
  open_stream();
   // Capture image 
  while (1)
  {
    gettimeofday(&tv_start, NULL);
    msec1  = tv_start.tv_usec/1000;

    data = capture_jpeg(data, &size);
    syslog(LOG_INFO,"writing jpeg\n");
    writeBytesEnc(data, size, sockfd, secret);

    char c = 0;
    syslog(LOG_INFO,"wrote jpeg :%d\n", *fd);
    if(read(*fd, &c, sizeof(c))) {
         if (c == 1) {
           syslog(LOG_INFO,"Break pipe");
           break;
         }
    } else {
      syslog(LOG_INFO,"read nothing :%d",c); 
    }
    gettimeofday(&tv_end, NULL);
    msec2  = tv_end.tv_usec/1000;
    syslog(LOG_INFO,"wrote jpeg :%d :%d :%d\n",fps_msec, msec2,msec1);
    if (fps_msec > (msec2-msec1)) {
      usleep(fps_msec-(msec2-msec1));
    }
  }
  syslog(LOG_INFO,"size to send:%d\n", size);
  //writeBytes(data, size, sockfd);


  free_frames();
  close (*fd);
  close(sockfd);
  exit(0);
}

int create_socket()
{
  int sockfd;
  int len;
  int result;
  struct sockaddr_in address;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(SERVER_ADDR);
  address.sin_port = htons(9735);
  len = sizeof(address);

  result = connect(sockfd, (struct sockaddr *)&address, len);
  syslog(LOG_ERR, "connect after");
  if (result == -1) {
    perror ("oops: client1");
    exit (1);
  } 

  return sockfd;
}
