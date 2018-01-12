#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<netinet/in.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<glib.h>
//capture part
#include <capture.h>
#include <syslog.h>
#include "capture_image.h"

#define CAPTURE_PROP_FMT "fps=1"
media_frame  *frame;
media_stream *stream;

void write_jpeg(void *data, char* path,int size)
{
	syslog(LOG_INFO,"STEP IN WRITING :%d", size);
	FILE *fp;
	fp = fopen(path, "w");
	int i;
	for(i=0; i<size; i++){
		fputc(((unsigned char *)data)[i], fp);
	}
	syslog(LOG_INFO,"STEP OUT WRITING");
	fclose(fp);
}
unsigned char *capture_jpeg(unsigned char *data, int *size)
{
	//JPEG
	syslog(LOG_INFO,"STEP STREAM ");
	
        if (NULL == stream)
	{
          syslog(LOG_INFO,"NULL Stream");
	}

	frame  = capture_get_frame(stream);
	syslog(LOG_INFO,"STEP FRAME");
	data   = capture_frame_data(frame);//our picture
	syslog(LOG_INFO,"STEP DATA");
	*size = (int)capture_frame_size(frame);
	syslog(LOG_INFO,"STEP SIZE");

	write_jpeg(data, "tmp/test.jpeg",(int)*size);
	return data;

}

void open_stream()
{
	stream = capture_open_stream(IMAGE_JPEG, CAPTURE_PROP_FMT);
}
void free_frames()
{
	capture_frame_free(frame);
	capture_close_stream(stream);
}
