#include<stdio.h>
#include<string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>

#define SIZE 1
#define NUMELEM 5
#define BUFFERSIZE 4096
#define JUMP_MULT 59
#define MULTIPLE 17
#define LOOPPERPAGE 20
#define URLLEN 60
#define ALIGNMENT 28
     
#define PORT 80
#define USERAGENT "HTMLGET 1.0"

int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host, char *page);
void usage();

void copywith(char* dest,char* src,int len);
int len_from_endchar(char* begin,char endchar);/*Why a function which takes char types as params should have a prototype declare?*/
int judge_page(char* url,char* filwords);/*Get the html page and judge whether it is useful*/

int main(void)
{
    FILE* fd = NULL;
    char buff[BUFFERSIZE*MULTIPLE];
    char tmpbuff[BUFFERSIZE*MULTIPLE];
    char linklist[LOOPPERPAGE][URLLEN];
    char* cursor;

    memset(buff,0,sizeof(buff));
    memset(tmpbuff,0,sizeof(tmpbuff));

    fd = fopen("tmp","rw+");
    fseek(fd,BUFFERSIZE*JUMP_MULT-5000,SEEK_CUR); /*get over the garbages at the beginning of a html file*/

    if(NULL == fd)
    {
        printf("\n fopen() Error!!!\n");
        return 1;
    } else ;//printf("\n File opened successfully through fopen()\n");

    /*Read a crawled html*/
    if(SIZE*BUFFERSIZE*MULTIPLE != fread(buff,SIZE,BUFFERSIZE*MULTIPLE,fd))
    {
        printf("\n fread() failed\n");
        return 1;
    }
    
    //printf("%s\n",buff);
    //return 0;
    /*Pick up the useful urls in turns with buff and tmpbuff*/
    for(int turns=0;turns<LOOPPERPAGE;turns+=2)
    {
	cursor = strstr(buff, "a1wrp");
    	int len = len_from_endchar(cursor+ALIGNMENT,'"');
    	//fprintf(stdout,"len %d\n",len);
    	copywith(linklist[turns],cursor+ALIGNMENT,len);
    	fprintf(stdout,"url %d: %s\n",turns,linklist[turns]);
  
    	int offset = (int)(cursor-buff)+ALIGNMENT+len;
    	copywith(tmpbuff,cursor+ALIGNMENT+len,BUFFERSIZE*MULTIPLE-offset);/*Get rid of the already processed texts*/
    	//fprintf(stdout,"%s\n",tmpbuff);

    	cursor = strstr(tmpbuff, "a1wrp");
    	len = len_from_endchar(cursor+ALIGNMENT,'"');
    	copywith(linklist[turns+1],cursor+ALIGNMENT,len);
    	fprintf(stdout,"url %d: %s\n",turns+1,linklist[turns+1]);

    	offset = (int)(cursor-tmpbuff)+ALIGNMENT+len;
    	copywith(buff,cursor+ALIGNMENT+len,BUFFERSIZE*MULTIPLE-offset);
    }
  
	int res;
	for(int turns=0;turns<LOOPPERPAGE;turns++)
	{
	    if(!judge_page(linklist[turns],"入札者評価制限")) fprintf(stdout,"%s, Not match\n",linklist[turns]); 
            else
		fprintf(stdout,"%s, OK\n",linklist[turns]); 
	}
	
    
    fclose(fd);
    return 0;
}

int len_from_endchar(char* begin,char endchar)
{
   int len = 0;
   char* iter = begin;
   while(NULL!=iter&&*iter!=endchar)
   {
	iter++;
	len++;
   }
   return len;
}

void copywith(char* dest,char* src,int len)
{
  for(int i=0;i<len;i++) dest[i] = src[i];
}

int judge_page(char* url,char* filwords)
{
    struct sockaddr_in *remote;
    int sock;
    int tmpres;
    char *ip;
    char *get;
    char *cursor;
    char buf[BUFFERSIZE+1];
    char host[30];
    char page[30];
    char tmp[100];
    
    /*Separate the url*/
    char* divider;
    divider = strstr(url,"/jp");
    int offset = (int)(divider-url);
    copywith(host,url,offset);
    copywith(page,divider,URLLEN-offset);
    if(NULL!=(divider=strstr(host,"pp"))) divider[1]='\0';
    if(NULL!=(divider=strstr(host,"jpjp"))) {divider[2]='\0';divider[3]='\0';}
    //fprintf(stdout,"host: %s  page: %s\n",host,page);

    sock = create_tcp_socket();
    ip = get_ip(host);
    remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
    remote->sin_family = AF_INET;
    tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
    if( tmpres < 0)  
    {
      perror("Can't set remote->sin_addr.s_addr");
      exit(1);
    }else if(tmpres == 0)
    {
      fprintf(stderr, "%s is not a valid IP address\n", ip);
      exit(1);
    }
    remote->sin_port = htons(PORT);
     
    if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
      perror("Could not connect");
      exit(1);
    }
    get = build_get_query(host, page);
    //fprintf(stderr, "Query is:\n<<START>>\n%s<<END>>\n", get);
     
    //Send the query to the server
    int sent = 0;
    while(sent < strlen(get))
    {
      tmpres = send(sock, get+sent, strlen(get)-sent, 0);
      if(tmpres == -1)
      {
       perror("Can't send query");
       exit(1);
      }
       sent += tmpres;
    }
    //now it is time to receive the page
    memset(buf, 0, sizeof(buf));
    int htmlstart = 0;
    char * htmlcontent;
    while((tmpres = recv(sock, buf, BUFFERSIZE, 0)) > 0){
      if(htmlstart == 0)
      {
          htmlcontent = strstr(buf, "\r\n\r\n");
          if(htmlcontent != NULL){
            htmlstart = 1;
            htmlcontent += 4;
          }
      }else{	htmlcontent = buf;	}
      if(htmlstart)
      {
        if(NULL!=(cursor=strstr(buf, filwords)))
	{
	   copywith(tmp,cursor,140);
	   //fprintf(stdout, "%s\n", tmp);	
	   if(NULL!=strstr(tmp, "あり"))
	   {
	      free(get);
      	      free(remote);
      	      free(ip);
      	      close(sock);
	      //fprintf(stdout, "no matching,return.\n");
      	      return 0;
	   }
	}
      }
     
        memset(buf, 0, tmpres);
      }//end while
      if(tmpres < 0)
      {
        perror("Error receiving data");
      }
      free(get);
      free(remote);
      free(ip);
      close(sock);
      return 1;
}

    void usage()
    {
      fprintf(stderr, "USAGE: htmlget host [page]\n\
    \thost: the website hostname. ex: coding.debuntu.org\n\
    \tpage: the page to retrieve. ex: index.html, default: /\n");
    }
     
     
    int create_tcp_socket()
    {
      int sock;
      if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Can't create TCP socket");
        exit(1);
      }
      return sock;
    }
     
     
char *get_ip(char *host)
{
      struct hostent *hent;
      int iplen = 15; //XXX.XXX.XXX.XXX
      char *ip = (char *)malloc(iplen+1);
      memset(ip, 0, iplen+1);
      if((hent = gethostbyname(host)) == NULL)
      {
	fprintf(stdout,"Unknown host: %s\n",host);
        herror("Can't get IP");
        exit(1);
      }
      if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
      {
        perror("Can't resolve host");
        exit(1);
      }
      return ip;
    }
     
    char *build_get_query(char *host, char *page)
    {
      char *query;
      char *getpage = page;
      char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
      if(getpage[0] == '/'){
        getpage = getpage + 1;
        //fprintf(stderr,"Removing leading \"/\", converting %s to %s\n", page, getpage);
      }
      // -5 is to consider the %s %s %s in tpl and the ending \0
      query = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)-5);
      sprintf(query, tpl, getpage, host, USERAGENT);
      return query;
    }
