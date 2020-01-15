/*
 * (C) Radim Kolar 1997-2004
 * This is free software, see GNU Public License version 2 for
 * details.
 *
 * Simple forking WWW Server benchmark:
 *
 * Usage:
 *   webbench --help
 *
 * Return codes:
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad param
 *    3 - internal error, fork failed
 * 
 */ 
#include "socket.c"
#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>

/* values */
volatile int timerexpired=0;//�ж�ѹ��ʱ���Ƿ��Ѿ������趨��ʱ��  ʱ�䵽����̾ͻ��˳�
int speed=0;//��¼���̳ɹ��õ���������Ӧ������  �ɹ��������Ӳ���������(�������-f�򻹱���read��������ݳɹ����ܼ�1)
int failed=0;//��¼ʧ�ܵ�������speed��ʾ�ɹ�����failed��ʾʧ������  һ������ʧ�� ���߶�дʧ�ܶ�+1
int bytes=0; //��¼���̳ɹ���ȡ���ֽ���  ͨ����ȡ����˵Ļ�Ӧ������
/* globals */
int http10=1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */ //http�汾��0��ʾhttp0.9��1��ʾhttp1.0��2��ʾhttp1.1
/* Allow: GET, HEAD, OPTIONS, TRACE */
int keepalive = 1;
#define METHOD_GET 0 
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"
int method=METHOD_GET;//Ĭ������ʽΪGET��Ҳ֧��HEAD��OPTIONS��TRACE
int clients=1;//������Ŀ��Ĭ��ֻ��1�����̷�����ͨ��-c��������
int force=0;//�Ƿ���Ҫ�ȴ���ȡ��server���ص����ݣ�0��ʾҪ�ȴ���ȡ
int force_reload=0;//�Ƿ�ʹ�û��棬1��ʾ�����棬0��ʾ���Ի���ҳ��
int proxyport=80;//����������Ķ˿�
char *proxyhost=NULL;//�����������ip
int benchtime=30;//ѹ��ʱ�䣬Ĭ��30�룬ͨ��-t��������
int max_request = 0;
/* internal */
int mypipe[2];//ʹ�ùܵ����и����̺��ӽ��̵�ͨ��
char host[MAXHOSTNAMELEN];//��������ip
#define REQUEST_SIZE 2048
char request[REQUEST_SIZE];//��Ҫ���͵�http����

static const struct option long_options[]=
{
 {"force",no_argument,&force,1},
 {"reload",no_argument,&force_reload,1},
 {"time",required_argument,NULL,'t'},
 {"max_con",required_argument,NULL,'m'},
 {"keepalive",required_argument,NULL,'k'},
 {"help",no_argument,NULL,'?'},
 {"http09",no_argument,NULL,'9'},
 {"http10",no_argument,NULL,'1'},
 {"http11",no_argument,NULL,'2'},
 {"get",no_argument,&method,METHOD_GET},
 {"head",no_argument,&method,METHOD_HEAD},
 {"options",no_argument,&method,METHOD_OPTIONS},
 {"trace",no_argument,&method,METHOD_TRACE},
 {"version",no_argument,NULL,'V'},
 {"proxy",required_argument,NULL,'p'},
 {"clients",required_argument,NULL,'c'},
 {NULL,0,NULL,0}
};

/* prototypes */
static void benchcore(const char* host,const int port, const char *request);
static int bench(void);
static void build_request(const char *url);

static void alarm_handler(int signal)
{
   timerexpired=1;
}	

static void usage(void)
{
   fprintf(stderr,
	"webbench [option]... URL\n"
	"  -f|--force               Don't wait for reply from server.\n"
	"  -r|--reload              Send reload request - Pragma: no-cache.\n"
	"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	"  -p|--proxy <server:port> Use proxy server for request.\n"
	"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
	"  -9|--http09              Use HTTP/0.9 style requests.\n"
	"  -1|--http10              Use HTTP/1.0 protocol.\n"
	"  -2|--http11              Use HTTP/1.1 protocol.\n"
	"  --get                    Use GET request method.\n"
	"  --head                   Use HEAD request method.\n"
	"  --options                Use OPTIONS request method.\n"
	"  --trace                  Use TRACE request method.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
};
int main(int argc, char *argv[])
{
 int opt=0;
 int options_index=0;
 char *tmp=NULL;

 if(argc==1)
 {
	  usage();
          return 2;
 } 

 while((opt=getopt_long(argc,argv,"912Vfrt:m:k:p:c:?h",long_options,&options_index))!=EOF )
 {
  switch(opt)
  {
   case  0 : break;
   case 'f': force=1;break;
   case 'r': force_reload=1;break; 
   case '9': http10=0;break;
   case '1': http10=1;break;
   case '2': http10=2;break;
   case 'V': printf(PROGRAM_VERSION"\n");exit(0);
   case 't': benchtime=atoi(optarg);break;	     
   case 'm':
        max_request=atoi(optarg);break; 
   case 'k':
        keepalive=atoi(optarg);
        if(keepalive != 0)
            keepalive = 1;
        break;
   case 'p': 
	     /* proxy server parsing server:port */
	     tmp=strrchr(optarg,':');
	     proxyhost=optarg;
	     if(tmp==NULL)
	     {
		     break;
	     }
	     if(tmp==optarg)
	     {
		     fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
		     return 2;
	     }
	     if(tmp==optarg+strlen(optarg)-1)
	     {
		     fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
		     return 2;
	     }
	     *tmp='\0';
	     proxyport=atoi(tmp+1);break;
   case ':':
   case 'h':
   case '?': usage();return 2;break;
   case 'c': clients=atoi(optarg);break;
  }
 }

 if(optind==argc) {
                      fprintf(stderr,"webbench: Missing URL!\n");
		      usage();
		      return 2;
                    }

 if(clients==0) 
    clients=1;
 if(benchtime==0) 
    benchtime=60;
 /* Copyright */
 fprintf(stderr,"Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
	 "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
	 );
 build_request(argv[optind]);
 /* print bench info */
 printf("\nBenchmarking: ");
 switch(method)
 {
	 case METHOD_GET:
	 default:
		 printf("GET");break;
	 case METHOD_OPTIONS:
		 printf("OPTIONS");break;
	 case METHOD_HEAD:
		 printf("HEAD");break;
	 case METHOD_TRACE:
		 printf("TRACE");break;
 }
 printf(" %s",argv[optind]);
 switch(http10)
 {
	 case 0: printf(" (using HTTP/0.9)");break;
	 case 2: printf(" (using HTTP/1.1)");break;
 }
 printf("\n");
 if(clients==1) 
    printf("1 client");
 else
   printf("%d clients",clients);

 printf(", running %d sec", benchtime);
 if(force) 
    printf(", early socket close");
 if(proxyhost!=NULL) 
    printf(", via proxy server %s:%d",proxyhost,proxyport);
 if(force_reload) 
    printf(", forcing reload");
    
 printf(".\n");
 return bench();
}

/*
���������Ҫ����ȫ�ֱ���char request[REQUEST_SIZE]������url��������ݡ�һ�����͵�http GET�������£�

GET /test.jpg HTTP/1.1
User-Agent: WebBench 1.5
Host:192.168.10.1
Pragma: no-cache
Connection: closebuild_request

������Ŀ�ľ���Ҫ��������������һ������Ϣȫ���浽ȫ�ֱ���request[REQUEST_SIZE]�У����л��в���ʹ�õ��ǡ�\r\n������������
һ������Ϣ�ľ���������Ҫ��������������Ĳ������Լ�url��ȷ����
*/
void build_request(const char *url)
{
  char tmp[10];
  int i;

  bzero(host,MAXHOSTNAMELEN);
  bzero(request,REQUEST_SIZE);

  if(force_reload && proxyhost!=NULL && http10<1) 
    http10=1;
  if(method==METHOD_HEAD && http10<1) 
    http10=1;
  if(method==METHOD_OPTIONS && http10<2) 
    http10=2;
  if(method==METHOD_TRACE && http10<2) 
    http10=2;

  switch(method)
  {
	  default:
	  case METHOD_GET: strcpy(request,"GET");break;
	  case METHOD_HEAD: strcpy(request,"HEAD");break;
	  case METHOD_OPTIONS: strcpy(request,"OPTIONS");break;
	  case METHOD_TRACE: strcpy(request,"TRACE");break;
  }
		  
  strcat(request," ");

  if(NULL==strstr(url,"://"))
  {
	  fprintf(stderr, "\n%s: is not a valid URL.\n",url);
	  exit(2);
  }
  if(strlen(url)>1500)
  {
         fprintf(stderr,"URL is too long.\n");
	 exit(2);
  }
  if(proxyhost==NULL)
      if (0!=strncasecmp("http://",url,7)) { 
          fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
          exit(2);
      }
  /* protocol/host delimiter */
  i=strstr(url,"://")-url+3;
  /* printf("%d\n",i); */

  if(strchr(url+i,'/')==NULL) {
      fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
      exit(2);
  }
  
  if(proxyhost==NULL)
  {
       /* get port from hostname */
       if(index(url+i,':')!=NULL &&
           index(url+i,':')<index(url+i,'/')) {
    	   strncpy(host,url+i,strchr(url+i,':')-url-i);
    	   bzero(tmp,10);
    	   strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/')-index(url+i,':')-1);
    	   /* printf("tmp=%s\n",tmp); */
    	   proxyport=atoi(tmp);
    	   if(proxyport==0) 
    	        proxyport=80;
       } else {
         strncpy(host,url+i,strcspn(url+i,"/"));
       }
       // printf("Host=%s\n",host);
       strcat(request+strlen(request),url+i+strcspn(url+i,"/"));
  } else {
       // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
       strcat(request,url);
  }
  if(http10==1)
	  strcat(request," HTTP/1.0");
  else if (http10==2)
	  strcat(request," HTTP/1.1");
  strcat(request,"\r\n");
  
  if(http10>0)
	  strcat(request,"User-Agent: WebBench "PROGRAM_VERSION"\r\n");
	  
  if(proxyhost==NULL && http10>0) {
	  strcat(request,"Host: ");
	  strcat(request,host);
	  strcat(request,"\r\n");
  }

  
  
  if(force_reload && proxyhost!=NULL) {
	  strcat(request,"Pragma: no-cache\r\n");
  }
  if(keepalive == 0) 
	  strcat(request,"Connection: close\r\n");
   else
        strcat(request,"Connection: keep-alive\r\n");
  /* add empty line at end */
  if(http10>0) 
    strcat(request,"\r\n"); 

  printf("Req=%s\n",request);
}

/* vraci system rc error kod */
static int bench(void) //���е�ѹ�ⶼ��bench����ʵ��
{
  int i,j,k;	
  pid_t pid=0;
  FILE *f;

  /* check avaibility of target server */
  i=Socket(proxyhost==NULL?host:proxyhost,proxyport);
  if(i<0) { 
       fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
       return 1;
  }
  
  close(i);
  /* create pipe */
  if(pipe(mypipe)) {//�ܵ������ӽ����򸸽��̻ر�����
	  perror("pipe failed.");
	  return 3;
  }

  /* not needed, since we have alarm() in childrens */
  /* wait 4 next system clock tick */
  /*
  cas=time(NULL);
  while(time(NULL)==cas)
        sched_yield();
  */

  /* fork childs */
  for(i=0;i<clients;i++) //����clients��Сfork�����㹻���ӽ��̽��в���
  {
	   pid=fork();
	   if(pid <= (pid_t) 0)
	   {
		   /* child process or error*/
	           sleep(1); /* make childs faster */
		   break;
	   }
  }

  if( pid< (pid_t) 0)
  {
      fprintf(stderr,"problems forking worker no. %d\n",i);
	  perror("fork failed.");
	  return 3;
  }

  if(pid== (pid_t) 0)//������ӽ��̣�����benchcore���в���
  {
    /* I am a child */
    if(proxyhost==NULL)
        benchcore(host,proxyport,request);
    else
        benchcore(proxyhost,proxyport,request);

         /* write results to pipe */
	 f=fdopen(mypipe[1],"w");  //�ӽ��̽����Խ��������ܵ�
	 if(f==NULL)
	 {
		 perror("open pipe for writing failed.");
		 return 3;
	 }
	 /* fprintf(stderr,"Child - %d %d\n",speed,failed); */
	 fprintf(f,"%d %d %d\n",speed,failed,bytes);
	 fclose(f);
	 return 0;
  } else {  //����Ǹ����̣���ӹܵ���ȡ�ӽ����������������
	  f=fdopen(mypipe[0],"r");
	  if(f==NULL) 
	  {
		  perror("open pipe for reading failed.");
		  return 3;
	  }
	  setvbuf(f,NULL,_IONBF,0);
	  speed=0;
      failed=0;
      bytes=0;

	  while(1)
	  {
		  pid=fscanf(f,"%d %d %d",&i,&j,&k);  //�ӹܵ���ȡ���ݣ�fscanfΪ����ʽ����
		  if(pid<2)
                  {
                       fprintf(stderr,"Some of our childrens died.\n");
                       break;
                  }
		  speed+=i;
		  failed+=j;
		  bytes+=k;
		  /* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
		  if(--clients==0) 
		    break;//������ڼ�¼�Ѿ����˶��ٸ��ӽ��̵����ݣ�������˳�
	  }
	  fclose(f);

      printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
		  (int)((speed+failed)/(benchtime/60.0f)),
		  (int)(bytes/(float)benchtime),
		  speed,
		  failed); //��󽫽����ӡ����Ļ��
  }
  return i;
}

void benchcore(const char *host,const int port,const char *req)
{
 int rlen;
 char buf[1500];
 int s,i;
 int ssock[2000000];
 int socknum = 0;
 int con = 0;
 int max_req_tmp = 0;
 struct sigaction sa;

 /* setup alarm signal handler */
 sa.sa_handler=alarm_handler;
 sa.sa_flags=0;
 if(sigaction(SIGALRM,&sa,NULL)) //��ʱ����ʱ���ã���ʱ�˳�
    exit(3);
 alarm(benchtime); //��ʼ��ʱ

 rlen=strlen(req);
 nexttry:while(1)
 {
    if(timerexpired) //һ����ʱ�򷵻�
    {
       if(failed>0)
       {
          /* fprintf(stderr,"Correcting failed by signal\n"); */
          failed--;
       }

       for(i = 0; i < socknum; i++) {
           if(ssock[i] > 0)
                close(ssock[i]);
       }
       return;
    }

    if(max_request > 0 && con >= max_request) {
        sleep(1);
        continue;
    }

    max_req_tmp++;
    ssock[socknum]=Socket(host,port);  //ע������ʽѭ��ǰ���ù�һ��socket,��bench                         
    if(ssock[socknum]<0) { 
        failed++;
        continue;
    } 

    if(rlen!=write(ssock[socknum],req,rlen)) { //��������
        failed++;
        close(ssock[socknum]);
        continue;
    } 

    if(http10==0) 
	    if(shutdown(ssock[socknum],1)) { 
    	    failed++;
    	    close(ssock[socknum]);
    	    continue;
	    }
	    
    if(force==0) //ȫ�ֱ���force��ʾ�Ƿ�Ҫ�ȴ����������ص�����
    {
            /* read all available data from socket */
	    while(1)
	    {
          if(timerexpired) 
            break; 

	      i=read(ssock[socknum],buf,1500);
              /* fprintf(stderr,"%d\n",i); */
	      if(i<0) 
              { 
                 failed++;
                 close(ssock[socknum]);
                 goto nexttry;
              }
	       else
		       if(i==0) break;
		       else
			       bytes+=i;
	    }
    }

   // if(close(s)) {
    //    failed++;
    //    continue;
    //}
    speed++;
    socknum++;
    con++;
 }
}

