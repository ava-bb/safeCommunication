#ifndef __CAPTURE_IMAGE_H__
#define __CAPTURE_IMAGE_H__

void write_jpeg(void *data, char* path,int size);
unsigned char *capture_jpeg(unsigned char* data,int *size);
void open_stream(void);
void free_frames(void);

#endif /* __CAPTURE_IMAGE__H__ */
