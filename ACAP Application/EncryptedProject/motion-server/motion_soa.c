/**
 * @file ax_event_subscription_example.c
 *
 * @brief This example illustrates how to setup an subscription to the
 * manual trigger event.
 *
 * Error handling has been omitted for the sake of brevity.
 */

#include <glib.h>
#include <glib-object.h>
#include <axsdk/axevent.h>
#include <syslog.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
//capture part
#include <syslog.h>
#include "send_image.h"
#include "encryption.h"
static int motion;
int fd[2];

static void
subscription_callback(guint subscription,
    AXEvent *event, guint *token);

static guint
subscribe_to_vmd3(AXEventHandler *event_handler, guint *token);

#if 0
static void appendToFile(char* path, void *data, int size);
static void logDateTime(char* path, int action);
#endif

static void
subscription_callback(guint subscription,
    AXEvent *event, guint *token)
{
  const AXEventKeyValueSet *key_value_set;
  gboolean state;
  GError *error;
  GTimeVal timeval;

  /* The subscription id is not used in this example. */
  (void)subscription;

  /* Extract the AXEventKeyValueSet from the event. */
  key_value_set = ax_event_get_key_value_set(event);


  /* Get the state of the motion detection . */
  if (ax_event_key_value_set_get_boolean(key_value_set,
        "active", NULL, &state, &error)) {

    /* Print a helpfull message. */
//  char path[] = "/var/log/motion.log";

    if (state) {
      syslog(LOG_ERR, "PIPE CREATION IN PROGRESS....");
      if (-1 == pipe(fd)) {
        syslog(LOG_INFO,"Failed Pipe Creation");
      }
      fcntl(fd[0], F_SETFL, O_NONBLOCK);
      fcntl(fd[1], F_SETFL, O_NONBLOCK);
      g_get_current_time(&timeval);
      syslog(LOG_INFO,"Camera has detected motion!!!!");
//      logDateTime(path, 1);
      if (fork() == 0) {
        close (fd[1]);
	send_image(&fd[0]);
      } else {
        motion = 1;
      }
    } else {
      if (motion) {
         syslog(LOG_INFO,"Detected motion stopped !!!!");
//	 logDateTime(path, 0);
         close (fd[0]);
         int c = 1;
	 write(fd[1], &c, sizeof(c));
         motion = 0;
         close(fd[1]);
      }
    }
  } else {
     syslog(LOG_ERR,"Error:%s ", error->message);
     g_error_free(error);
  }

  g_message("Here is th token:%d\n", *token);
}

static guint
subscribe_to_vmd3(AXEventHandler *event_handler, guint *token)
{
  AXEventKeyValueSet *key_value_set;
  guint subscription;
//  int sockfd;

//  sockfd = create_socket();
  
  //close(sockfd);

  key_value_set = ax_event_key_value_set_new();

  ax_event_key_value_set_add_key_values(key_value_set,
	NULL,
	"topic0", "tns1", "RuleEngine", AX_VALUE_TYPE_STRING,
	"topic1", "tnsaxis", "VMD3", AX_VALUE_TYPE_STRING,
	"active", NULL, NULL, AX_VALUE_TYPE_BOOL, NULL);

  /* Time to setup the subscription. Use the "token" input argument as
   * input data to the callback function "subscription callback"
   */
  ax_event_handler_subscribe(event_handler, key_value_set,
        &subscription, (AXSubscriptionCallback)subscription_callback, token,
        NULL);

  /* The key/value set is no longer needed */
  ax_event_key_value_set_free(key_value_set);

  return subscription;
}

#if 0
static void appendToFile(char* path, void *data, int size){
	FILE *fp;
	fp = fopen(path, "a");
	int i;
	for(i=0; i<size; i++){
		fputc(((unsigned char *)data)[i], fp);
	}
	fclose(fp);
}


static void logDateTime(char* path, int action){
	time_t timer;
	char buffer[26];
	struct tm* tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%Y-%m-%dT%H:%M:%S;", tm_info);
	appendToFile(path, buffer, 20);
	if(action)
		appendToFile(path, "1\n", 2);
	else
		appendToFile(path, "0\n", 2);
}
#endif

int main(void)
{
  GMainLoop *main_loop;
  AXEventHandler *event_handler;
  guint token = 1234;
  guint subscription;
  openlog(NULL, LOG_PID, LOG_DAEMON);

  main_loop = g_main_loop_new(NULL, FALSE);

  event_handler = ax_event_handler_new();

  subscription = subscribe_to_vmd3(event_handler, &token);
  if(!subscription)
  {
    syslog(LOG_ERR, "Motion Dection Subscription Failed");
  }

  g_main_loop_run(main_loop);

  ax_event_handler_unsubscribe(event_handler, subscription, NULL);

  ax_event_handler_free(event_handler);

  closelog();

  return 0;
}

