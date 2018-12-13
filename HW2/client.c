#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<signal.h>
#define IP_ADDR "127.0.0.1"
int sockfd,yes=0;
char name[20];
pthread_t th,th1;
FILE*fp,*fd;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
void handler(int signum)
{
	int ret;
	if(signum==SIGINT)
	{
		write(sockfd,"quit",strlen("quit"));
		close(sockfd);
	}
	exit(0);
}
void*snd(void*arg)
{
	char line[1000],str[2048];
	while(1)
	{
		fgets(str,1000,stdin);
		write(sockfd,str,strlen(str)-1);
		if(strcmp(str,"sendfile\n")==0)
		{	
			fgets(str,1000,stdin); //who
			write(sockfd,str,strlen(str)-1);
name:			fgets(str,1000,stdin);//filename
			if(str[strlen(str)-1]=='\n')
				str[strlen(str)-1]='\0';
			fd=fopen(str,"r");
			if(fd==NULL)
			{
				printf("The file isn't exist!\nPlease enter another one.\n");
				goto name;
			}
			write(sockfd,str,strlen(str));
			pthread_cond_wait(&cond,&mutex);
			if(yes==1)
			{
				while((fgets(line,1000,fd)))
				{
					write(sockfd,line,strlen(line));
				}
				sleep(3);
				fclose(fd);
				write(sockfd,"eof",strlen("eof"));
				yes=0;
			}
		}
	}	
	pthread_exit(NULL);
}

int main()
{
	int len,ret;
	struct sockaddr_in client;
	char buff[2048],buff1[2048];

	signal(SIGINT,handler);
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	client.sin_family=AF_INET;
	client.sin_addr.s_addr=inet_addr(IP_ADDR);

	client.sin_port=htons(8080);
	len=sizeof(client);
	ret=connect(sockfd,(struct sockaddr*)&client,len);
	if(ret==-1)
	{
		perror("connect failed");
		return 1;
	}
	len=read(sockfd,buff,sizeof(buff));
	if(len>0)
		buff[len]=0;
	printf("%s",buff);
	memset(buff,0,sizeof(buff));
	pthread_create(&th,NULL,snd,NULL);
	while(1)   //recv
	{
		if((len=read(sockfd,buff,2048))>0)
		{
			buff[len]=0;
			if(strcmp(buff,"Accept\n")==0)
			{
				yes=1;
				pthread_cond_signal(&cond);
				printf("%s",buff);
			}
			else if(strcmp(buff,"Reject\n")==0)
			{
				printf("%s",buff);
				pthread_cond_signal(&cond);
			}
			else if(strcmp(buff,"Sending the file...\n")==0)
			{
				printf("%s",buff);
				if((len=read(sockfd,buff1,2048))>0)
				{
					buff1[len]=0;
					fp=fopen(buff1,"w");
					while(1)
					{
						memset(buff1,0,sizeof(buff1));
						if((len=read(sockfd,buff1,2048))>0)
						{
							buff1[len]=0;
							if(strcmp(buff1,"eof")==0)
								break;
							else if(strncmp(buff1,"[public]",8)==0)
							{
								printf("%s",buff1);
								continue;
							}
							else if(strncmp(buff1,"[whisper]",9)==0)
							{
								printf("%s",buff1);
								continue;
							}	
							fprintf(fp,"%s",buff1);
						}
					}
					fclose(fp);
					memset(buff1,0,sizeof(2048));
				}
			}
			else
				printf("%s",buff);

		}//ifread
		memset(buff,0,sizeof(buff));
	}//while
	return 0;
}
