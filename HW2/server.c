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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t u = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t t = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mxx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
int listenfd,connfd[10],filein[10],yes[10]={0};
char namelist[10][20];
void rcv_snd(int n);
int main()
{
	int i=0;
	pthread_t thread;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t len;
	char buff[2048];

	if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("Socket created failed.\n");
		return -1;
	}
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(8080);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
	{
		printf("Bind failed.\n");
		return -1;
	}

	listen(listenfd,10);
	for(i=0;i<10;i++)
	{
		connfd[i]=-1;
		filein[i]=-1;
		yes[i]=-1;
	}
	while(1)
	{
		len=sizeof(cliaddr);
		for(i=0;i<10;i++)
		{
			if(connfd[i]==-1)
				break;
		}

		pthread_mutex_lock(&mutex);
		connfd[i]=accept(listenfd,(struct sockaddr*)&cliaddr,&len);
		pthread_mutex_unlock(&mutex);

		pthread_create(malloc(sizeof(pthread_t)),NULL,(void*)(&rcv_snd),(void*)i);
	}
	close(listenfd);
	return 0;
}
void rcv_snd(int n)
{
	char*ask="Enter your name: ";
	char buff[2048],buff1[2048],buff2[2048],filename[50];
	char name[20];
	int i=0,len;
	char line[]="----------\n";
	char m[]="Sorry the person isn't exist!\n";
	char me[]="The name is exist!\n";
	write(connfd[n],ask,strlen(ask));
	while(1)
	{
		memset(name,0,sizeof(name));
		if((len=read(connfd[n],name,20))>0)
			name[len]=0;
		for(i=0;i<10;i++)
		{ 
			if(connfd[i]!=-1)
			{
				if(strcmp(namelist[i],name)==0)
				{
					write(connfd[n],me,strlen(me));
					break;
				}
			}

		}
		if(i==10)
			break;
	}
	strncpy(buff,name,strlen(name));
	strcat(buff,"\tjoin in\n");
	memset(namelist[n],0,sizeof(namelist[n]));
	strncpy(namelist[n],name,strlen(name));
	for(i=0;i<10;i++)
	{
		if(connfd[i]!=-1)
			write(connfd[i],buff,strlen(buff));
	}
	while(1)
	{
		if((len=read(connfd[n],buff1,2048))>0)
		{
			buff1[len]=0;
			if(strcmp("quit",buff1)==0)
				break;
			else if(strcmp("online",buff1)==0)
			{
				write(connfd[n],line,strlen(line));
				for(i=0;i<10;i++)
				{	
					if(connfd[i]!=-1)
					{
						write(connfd[n],namelist[i],strlen(namelist[i]));
						write(connfd[n],"\n",strlen("\n"));
					}

				}
				write(connfd[n],line,strlen(line));
				continue;
			}
			else if(strcmp("sendfile",buff1)==0)
			{
				char ms[]="Who you want to send the file?\n";
				write(connfd[n],ms,strlen(ms));
				write(connfd[n],line,strlen(line));
				for(i=0;i<10;i++)
				{
					if((connfd[i]!=-1)&&i!=n)
					{
						write(connfd[n],namelist[i],strlen(namelist[i]));
						write(connfd[n],"\n",strlen("\n"));
					}
				}
				write(connfd[n],line,strlen(line));
				if((len=read(connfd[n],buff2,2048))>0)
				{
					buff2[len]=0;
					for(i=0;i<10;i++)
					{
						if(connfd[i]!=-1)
						{
							if(strcmp(buff2,namelist[i])==0)
								break;
						}
					}
					if(i==10)
					{
						write(connfd[n],m,strlen(m));
						continue;
					}
					memset(buff2,0,sizeof(buff2));
					pthread_mutex_lock(&u);
					filein[i]=n;
					pthread_mutex_unlock(&u);
					write(connfd[n],"Enter the file name\n",strlen("Enter the file name\n"));
					memset(filename,0,sizeof(filename));
					if((len=read(connfd[n],filename,50))>0)
						filename[len]=0;
					sprintf(buff2,"Do you want to accept the file '%s' from %s ?(Y/N)\n",filename,namelist[n]);
					write(connfd[i],buff2,strlen(buff2));
					pthread_cond_wait(&cond,&mxx);
					if(yes[i]==n)
					{
						write(connfd[i],"Sending the file...\n",strlen("Sending the file...\n"));
						sleep(1);
						write(connfd[i],filename,strlen(filename));
						sleep(1);
						while(1)
						{
							memset(buff2,0,sizeof(buff2));
							if((len=read(connfd[n],buff2,2048))>0)
								buff2[len]=0;
							if(strcmp(buff2,"eof")==0)
							{
								write(connfd[i],buff2,strlen(buff2));
								break;
							}
							else
								write(connfd[i],buff2,strlen(buff2));
						}
						sleep(1);
						write(connfd[i],"Success!\n",strlen("Success!\n"));
						yes[i]=-1;
					}//if can write
				}//send to whom

			}//sendfile
			else if(strcmp("whisper",buff1)==0)
			{
				char msg[]="Who you want to whisper?\n";
				write(connfd[n],msg,strlen(msg));
				write(connfd[n],line,strlen(line));
				for(i=0;i<10;i++)
				{
					if((connfd[i]!=-1)&&i!=n)
					{
						write(connfd[n],namelist[i],strlen(namelist[i]));
						write(connfd[n],"\n",strlen("\n"));
					}
				}
				write(connfd[n],line,strlen(line));
				if((len=read(connfd[n],buff2,2048))>0)
				{
					buff2[len]=0;
					for(i=0;i<10;i++)
					{
						if(connfd[i]!=-1)
						{
							if(strcmp(buff2,namelist[i])==0)
								break;
						}
					}
					if(i==10)
					{
						write(connfd[n],m,strlen(m));
						continue;
					}
					while(1)
					{
						memset(buff2,0,sizeof(buff2));
						memset(buff,0,sizeof(buff));
						if((len=read(connfd[n],buff2,2048))>0)
						{
							buff2[len]=0;
							if(strcmp(buff2,"quit")==0)
								goto cancel;
							else if(strcmp(buff2,"public")==0)
								break;	
							sprintf(buff,"[whisper] %s\t:%s\n",namelist[n],buff2);
							write(connfd[i],buff,strlen(buff));
						}
					}
				}//enter name
			}//whisper
			else
			{
				memset(buff,0,sizeof(buff));
				if(filein[n]==-1)
				{
					sprintf(buff,"[public] %s\t:%s\n",namelist[n],buff1);
					for(i=0;i<10;i++)
					{
						if(connfd[i]!=-1)
							write(connfd[i],buff,sizeof(buff));
					}
				}
				else if(filein[n]>=0)
				{
					if(strcmp(buff1,"Y")==0)
					{
						yes[n]=filein[n];
						write(connfd[filein[n]],"Accept\n",strlen("Accept\n"));
						pthread_cond_signal(&cond);
					}
					else if(strcmp(buff1,"N")==0)
					{
						write(connfd[filein[n]],"Reject\n",strlen("Reject\n"));
						pthread_cond_signal(&cond);
					}
					pthread_mutex_lock(&t);
					filein[n]=-1;
					pthread_mutex_unlock(&t);
				}//filein>=0
			}//else
		}//if(read)
		memset(buff1,0,sizeof(buff1));
		memset(buff,0,sizeof(buff));
		memset(buff2,0,sizeof(buff2));
	}//while
cancel:	close(connfd[n]);
	pthread_mutex_lock(&t);
	connfd[n]=-1;
	pthread_mutex_unlock(&t);
	pthread_exit(NULL);
}//rcv_snd

