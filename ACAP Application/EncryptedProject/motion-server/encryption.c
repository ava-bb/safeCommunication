#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<netinet/in.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<math.h>
#include<syslog.h>
#include<time.h>
#include "encryption.h"

static unsigned char* readBytes(int* len,int client_sockfd);
static void writeBytes(unsigned char* dataBytes, int len, int client_sockfd);
unsigned char* readBytesEnc(int *len, int client_sockfd, int secret);
int readIntEnc(int client_sockfd, int secret);

static int buffToInt(unsigned char *buffer)
{
	int num = (int)((unsigned char)(buffer[0]) << 24 | (unsigned char)(buffer[1]) << 16 | (unsigned char)(buffer[2]) << 8 | (unsigned char)(buffer[3]));
	return num;
}

static void intToBuff(int n,unsigned char* buffer) {
	buffer[0] = (n >> 24) & 0xFF;
	buffer[1] = (n >> 16) & 0xFF;
	buffer[2] = (n >> 8) & 0xFF;
	buffer[3] = n & 0xFF;
}

static int readInt(int client_sockfd){
	int len;
	unsigned char *dataBytes = readBytes(&len, client_sockfd);
	int n = buffToInt(dataBytes);
	free(dataBytes);
	return n;
}

static void writeInt(int n, int client_sockfd){
	unsigned char bytes[4];
	intToBuff(n,bytes);
	writeBytes(bytes,4,client_sockfd);
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

static unsigned char* readBytes(int* len,int client_sockfd){
	//read 4 bytes for int
	unsigned char lenBytes[4];
	read(client_sockfd, &lenBytes,4);
	*len = buffToInt(lenBytes);
	syslog(LOG_INFO,"server read 4 len bytes\n");

	//read data bytes
	unsigned char *dataBytes = malloc(*len);
	if(*len > 0)
		read(client_sockfd, dataBytes,*len);
	syslog(LOG_INFO,"server read data len bytes\n");
	return dataBytes;
}

unsigned char* encryptXOR(unsigned char* clear, int *len, int secret){
	*len = *len*2;
	unsigned char* cipher = malloc(*len);
	unsigned char kPrev = secret & 0xFF, k;
	int i;
	for(i=0; i < *len; i=i+2){
		k = (rand() & 0xFF); //only last byte
		cipher[i]=kPrev ^ k;
		cipher[i+1]=clear[i/2] ^ k;
		kPrev = k;		
	}
	return cipher;	
}

unsigned char* decryptXOR(unsigned char* cipher, int* len, int secret){
	unsigned char* clear = malloc(*len/2);
	unsigned char kPrev = secret & 0xFF, k;
	int i;
	for(i=0; i < *len; i=i+2){
		k = kPrev ^ cipher[i];
		clear[i/2] = cipher[i+1] ^ k;
		kPrev = k;		
	}
	*len /= 2;
	return clear;		
}


unsigned char* readBytesEnc(int *len, int client_sockfd, int secret){
	unsigned char* cipher = readBytes(len, client_sockfd);
 	return decryptXOR(cipher, len, secret);
}

void writeBytesEnc(unsigned char* clear, int len, int client_sockfd, int secret){
	unsigned char* cipher = encryptXOR(clear, &len, secret);
	writeBytes(cipher,len,client_sockfd);
}

int readIntEnc(int client_sockfd, int secret){
	int len;
	unsigned char *intBytes = readBytesEnc(&len, client_sockfd, secret);
	int n = buffToInt(intBytes);
	free(intBytes);
	return n;
}

void writeIntEnc(int n, int client_sockfd, int secret){
	unsigned char bytes[4];
	intToBuff(n,bytes);
	writeBytesEnc(bytes,4,client_sockfd, secret);
}

static int getPrime(int min, int max){
	srand(time(NULL));
	int prime;
	int notPrime = 1;
	while(notPrime){
		prime = min + (rand() % (max-min));
		notPrime = 0;
		int trashold = prime/2; //(int)sqrt((double) prime)
		//syslog(LOG_INFO,"Number %d, trashold %d\n",prime,trashold);
		int i;
		for(i=2; i<trashold; i++){
			if(prime%i==0){
				notPrime=1;
				break;
			}
		}
	}
	return prime;
}


static int sqrAndMul(int x, int y, int n){
	if(y == 0)
		return  1;
	if(y == 1)
		return  x % n;
	
	if(y%2 == 0)
		return sqrAndMul( (x*x)%n,  y/2, n) % n;
	else
		return (x * (sqrAndMul( (x*x)%n, (y-1)/2, n)%n)) % n;
}


int keyExchange(int client_sockfd){

	int maxHalf = 216;//preventing INT overflow
 	int minHalf = maxHalf/2; //go down for one bit
	int sKey = 0;

	int g = getPrime(3,minHalf); //generator
	int p = getPrime(minHalf,maxHalf); //modulos
	do{
		sKey = getPrime(minHalf,p-1); //secret part of key
	}while(p == sKey); //key must be different than modulos
	
	int SKey = sqrAndMul(g, sKey, p);
        writeInt(g,client_sockfd);
        writeInt(p,client_sockfd);
	writeInt(SKey,client_sockfd);

	int CKey = readInt(client_sockfd);

	int sharedSecret = sqrAndMul(CKey, sKey, p);
	syslog(LOG_INFO,"DH DATA: g %d, p %d, sKey %d, SKey %d, CKey %d, shareSec %d\n", g, p, sKey, SKey, CKey, sharedSecret);

	//read client conformation
	int conf = readIntEnc(client_sockfd, sharedSecret);
	syslog(LOG_INFO,"KeyExchange conformation %d\n", conf);
	//0 means everything ok
	if(conf == 0)
		writeIntEnc(0, client_sockfd, sharedSecret);
	else
		writeIntEnc(-1, client_sockfd, sharedSecret);
	return sharedSecret;
}
