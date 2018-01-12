#ifndef __ENCRYPTION_H__
#define __ENCRYPTION_H__

unsigned char* encryptXOR(unsigned char* clear, int *len, int secret);
unsigned char* decryptXOR(unsigned char* cipher, int* len, int secret);
void writeIntEnc(int n, int client_sockfd, int secret);
int keyExchange(int client_sockfd);
unsigned char* readBytesEnc(int *len, int client_sockfd, int secret);
void writeBytesEnc(unsigned char* clear, int len, int client_sockfd, int secret);
#endif
