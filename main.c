/**
*For original source code 
*pls look http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client 
*
*note: This programme works well. It uses send/recv instead of write/read.
*      Another programme which uses write/read cannot get correct response(?). --Saintre 2016.9.26
***/
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host, char *page);
void usage();
     
#define HOST "auctions.search.yahoo.co.jp"
#define PAGE "search?fr=auc_top&p=LEGO+パーツ&oq=lego+pa&auccat=0&tab_ex=commerce&sc_i=auc_sug&ei=UTF-8&b=41" 
#define PORT 80
#define USERAGENT "HTMLGET 1.0"

#define BUFFERSIZE 12288
     
int main(int argc, char **argv)
{
    struct sockaddr_in *remote;
    int sock;
    int tmpres;
    char *ip;
    char *get;
    char buf[BUFFERSIZE];
    char *host;
    char *page;
    

    host = HOST;
    page = PAGE;
    sock = create_tcp_socket();
    ip = get_ip(host);
    //fprintf(stderr, "IP is %s\n", ip);
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

    while((tmpres = recv(sock, buf, sizeof(buf), 0)) > 0){
      	fprintf(stdout, buf);
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
      return 0;
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
