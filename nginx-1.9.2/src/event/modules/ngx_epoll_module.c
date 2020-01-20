
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

/* http://blog.csdn.net/zhang_shuai_2011/article/details/7678990
�ļ��첽I/O���:
֪���첽IO�Ѿ��ܾ��ˣ�����ֱ����������������������һ��ʵ�����⣨��һ��CPU�ܼ��͵�Ӧ���У���һЩ��Ҫ��������ݿ��ܷ��ڴ����ϡ�Ԥ��
֪����Щ���ݵ�λ�ã�����Ԥ�ȷ����첽IO�����󡣵ȵ�������Ҫ�õ���Щ���ݵ�ʱ���ٵȴ��첽IO��ɡ�ʹ�����첽IO���ڷ���IO����ʵ��ʹ��
�������ʱ���ڣ����򻹿��Լ������������飩��
�ٴ˻��ᣬҲ˳���о���һ��linux�µ��첽IO��ʵ�֡�
linux����Ҫ�������첽IO��һ������glibcʵ�ֵģ����³�֮Ϊglibc�汾����һ������linux�ں�ʵ�֣�����libaio����װ���ýӿڣ����³�֮Ϊlinux�汾����

����������̿��Կ�����linux�汾���첽IOʵ����ֻ��������CPU��IO�豸�����첽���������ԣ�IO�����ύ�Ĺ�����Ҫ�����ڵ������߳���ͬ����ɵģ�����
�ύ������CPU��IO�豸���Բ��й��������Ե������̿��Է��أ������߿��Լ������������飩�����ͬ��IO��������ռ�ö����CPU��Դ��
��glibc�汾���첽IO�����������߳����߳�֮������첽���������ԣ�ʹ�����µ��߳������IO�����������������ռ��CPU��Դ�����̵߳Ĵ��������١�����
������CPU���������ҵ������̺߳��첽�����߳�֮�仹�����̼߳�ͨ�ŵĿ�������������IO�����ύ�Ĺ��̶����첽�����߳�������ˣ���linux�汾�ǵ���
������ɵ������ύ�����������߳̿��Ը������Ӧ�������顣���CPU��Դ�ܸ��㣬����ʵ�ֵ�Ҳ������

����һ�㣬�����������������첽IO�ӿڣ��ύ����첽IO����ʱ����glibc�汾���첽IO�У�ͬһ��fd�Ķ�д������ͬһ���첽�����߳�����ɡ����첽�����߳�
����ͬ���ء�һ��һ����ȥ������Щ�������ԣ����ڵײ��IO��������˵����һ��ֻ�ܿ���һ�����󡣴�������������첽�����̲߳Ż��ύ��һ��������
��ʵ�ֵ��첽IO������ֱ�ӽ����������ύ����IO��������IO�������ܿ������е�����������ˣ�IO������ʹ�õ�������㷨���ܷ��Ӹ���Ĺ�Ч��������
�ˣ���������£�����ϵͳ�е�IO���󶼼�����ͬһ��fd�ϣ����Ҳ�ʹ��Ԥ������IO����������ֻ�ܿ���һ��������ô�����㷨���˻��������ȷ����㷨����
�ܻἫ���������ͷ�ƶ��Ŀ�����
















    ��֮ǰ�ᵽ���¼�����ģ�鶼���ڴ��������¼�����û���漰�������ļ��Ĳ��������ڽ�����Linux�ں�2.6.2x֮��汾��֧�ֵ��ļ��첽I/O����
��ngx_epoll_moduleģ����������ļ��첽I/O����ṩ����ġ������ᵽ���ļ��첽I/O������glibc���ṩ���ļ��첽I/O��glibc���ṩ���첽I/O��
���ڶ��߳�ʵ�ֵģ����������������ϵ��첽I/O��������˵�����첽I/O����Linux�ں�ʵ�֣�ֻ�����ں��гɹ�������˴��̲������ں˲Ż�֪ͨ
���̣�����ʹ�ô����ļ��Ĵ����������¼��Ĵ���ͬ����Ч��

    ʹ�����ַ�ʽ��ǰ����Linux�ں˰汾�б���֧���ļ��첽I/O����Ȼ���������ĺô�Ҳ�ǳ����ԣ�Nginx�Ѷ�ȡ�ļ��Ĳ����첽���ύ���ں˺���
�˻�֪ͨI/O�豸������ִ�в�����������Nginx���̿��Լ�����ֵ�ռ��CPU�����ң����������¼��ѻ���I/O�豸�Ķ�����ʱ�����ᷢ�ӳ��ں��С�����
�㷨�������ƣ��Ӷ����������ȡ���������ĳɱ���

    ע��Linux�ں˼�����ļ��첽I/O�ǲ�֧�ֻ�������ģ�Ҳ����˵����ʹ��Ҫ�������ļ�����Linux�ļ������д��ڣ�Ҳ����ͨ����ȡ�����Ļ���
�е��ļ���������ʵ�ʶԴ��̵Ĳ�������Ȼ������worker���̵ĽǶ�����˵���˺ܴ��ת�����ǶԵ���������˵�������п��ܽ���ʵ�ʴ�����ٶȣ���
Ϊԭ�ȿ��Դ��ڴ��п��ٻ�ȡ���ļ�����ʹ�����첽I/O����һ����Ӵ����϶�ȡ���첽�ļ�I/O�ǰѡ�˫�н������ؼ�Ҫ��ʹ�ó���������󲿷��û�
������ļ��Ĳ��������䵽�ļ������У���ô��Ҫʹ���첽I/O����֮���������ʹ���ļ��첽I/O����һ���Ƿ��Ϊ����������������ϵ����������
���Կ�����Ȼ�첽�ļ�I/O���Ա�������worker���̣����Ƕ�ȡ�ļ����ݵ�ʱ���䳤��

    Ŀǰ��Nginx��֧���ڶ�ȡ�ļ�ʱʹ���첽I/O����Ϊ����д���ļ�ʱ������д���ڴ�
�о����̷��أ�Ч�ʺܸߣ���ʹ���첽I/Oд��ʱ�ٶȻ������½���
*/

/*
Epoll��LT��ETģʽ�µĶ�д��ʽ
����ʱ�䣺July 10, 2012 ���ࣺLinux 

��VPS��CentOSװ����¼��

��MooC��һЩ���˼·��

��һ����������socket�ϵ���read/write����, ����EAGAIN����EWOULDBLOCK(ע: EAGAIN����EWOULDBLOCK)
�������Ͽ�, ��˼��:EAGAIN: ����һ�Σ�EWOULDBLOCK: �������һ������socket, ��������block��perror���: Resource temporarily unavailable

�ܽ�:
��������ʾ��Դ��ʱ��������readʱ����������û�����ݣ�����writeʱ��д���������ˡ�����������������������socket��
read/write��Ҫ��������������Ƿ�����socket��read/write��������-1�� ͬʱerrno����ΪEAGAIN��
���ԣ���������socket��read/write����-1������������ˡ������ڷ�����socket��read/write����-1��һ��������ĳ����ˡ�
������Resource temporarily unavailable����ʱ��Ӧ�����ԣ�ֱ��Resource available��

���ϣ�����non-blocking��socket����ȷ�Ķ�д����Ϊ:
�������Ե�errno = EAGAIN�Ĵ����´μ�����
д�����Ե�errno = EAGAIN�Ĵ����´μ���д

����select��epoll��LTģʽ�����ֶ�д��ʽ��û������ġ�������epoll��ETģʽ�����ַ�ʽ����©����


epoll������ģʽLT��ET
���ߵĲ�������level-triggerģʽ��ֻҪĳ��socket����readable/writable״̬������ʲôʱ�����epoll_wait���᷵�ظ�socket��
��edge-triggerģʽ��ֻ��ĳ��socket��unreadable��Ϊreadable���unwritable��Ϊwritableʱ��epoll_wait�Ż᷵�ظ�socket��

���ԣ���epoll��ETģʽ�£���ȷ�Ķ�д��ʽΪ:
����ֻҪ�ɶ�����һֱ����ֱ������0������ errno = EAGAIN
д:ֻҪ��д����һֱд��ֱ�����ݷ����꣬���� errno = EAGAIN

��ȷ�Ķ�


n = 0;
 while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {
     n += nread;
 }
 if (nread == -1 && errno != EAGAIN) {
     perror("read error");
 } ��ȷ��д


int nwrite, data_size = strlen(buf);
 n = data_size;
 while (n > 0) {
     nwrite = write(fd, buf + data_size - n, n);
     if (nwrite < n) {
         if (nwrite == -1 && errno != EAGAIN) {
             perror("write error");
         }
         break;
     }
     n -= nwrite;
 } ��ȷ��accept��accept Ҫ���� 2 ������
(1) ����ģʽ accept ���ڵ�����
�������������TCP���ӱ��ͻ���ز�ۣ����ڷ���������accept֮ǰ���ͻ�����������RST��ֹ���ӣ����¸ոս��������ӴӾ����������Ƴ���
����׽ӿڱ����ó�����ģʽ���������ͻ�һֱ������accept�����ϣ�ֱ������ĳ���ͻ�����һ���µ�����Ϊֹ�������ڴ��ڼ䣬��������
����������accept�����ϣ����������е��������������ò�������

����취�ǰѼ����׽ӿ�����Ϊ�����������ͻ��ڷ���������accept֮ǰ��ֹĳ������ʱ��accept���ÿ�����������-1����ʱԴ��Berkeley��
ʵ�ֻ����ں��д�����¼��������Ὣ���¼�֪ͨ��epool��������ʵ�ְ�errno����ΪECONNABORTED����EPROTO��������Ӧ�ú�������������

(2)ETģʽ��accept���ڵ�����
��������������������ͬʱ�����������TCP��������˲����۶���������ӣ������Ǳ�Ե����ģʽ��epollֻ��֪ͨһ�Σ�acceptֻ����
һ�����ӣ�����TCP����������ʣ�µ����Ӷ��ò�������

����취����whileѭ����סaccept���ã�������TCP���������е��������Ӻ����˳�ѭ�������֪���Ƿ�������������е����������أ�accept
����-1����errno����ΪEAGAIN�ͱ�ʾ�������Ӷ������ꡣ

�ۺ��������������������Ӧ��ʹ�÷�������accept��accept��ETģʽ�µ���ȷʹ�÷�ʽΪ��


while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote, (size_t *)&addrlen)) > 0) {
     handle_client(conn_sock);
 }
 if (conn_sock == -1) {
     if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
     perror("accept");
 } һ����Ѷ��̨������������
ʹ��Linuxepollģ�ͣ�ˮƽ����ģʽ����socket��дʱ���᲻ͣ�Ĵ���socket��д���¼�����δ���

��һ�����ձ�ķ�ʽ��
��Ҫ��socketд���ݵ�ʱ��Ű�socket����epoll���ȴ���д�¼���
���ܵ���д�¼��󣬵���write����send�������ݡ�
���������ݶ�д��󣬰�socket�Ƴ�epoll��

���ַ�ʽ��ȱ���ǣ���ʹ���ͺ��ٵ����ݣ�ҲҪ��socket����epoll��д������Ƴ�epoll����һ���������ۡ�

һ�ָĽ��ķ�ʽ��
��ʼ����socket����epoll����Ҫ��socketд���ݵ�ʱ��ֱ�ӵ���write����send�������ݡ��������EAGAIN����socket����epoll����epoll��
������д���ݣ�ȫ�����ݷ�����Ϻ����Ƴ�epoll��

���ַ�ʽ���ŵ��ǣ����ݲ����ʱ����Ա���epoll���¼��������Ч�ʡ�

�����һ��ʹ��epoll,ETģʽ�ļ�HTTP����������:


#include <sys/socket.h>
 #include <sys/wait.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <sys/epoll.h>
 #include <sys/sendfile.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <strings.h>
 #include <fcntl.h>
 #include <errno.h> 
 
 #define MAX_EVENTS 10
 #define PORT 8080
 
 //����socket����Ϊ������ģʽ
 void setnonblocking(int sockfd) {
     int opts;
 
     opts = fcntl(sockfd, F_GETFL);
     if(opts < 0) {
         perror("fcntl(F_GETFL)\n");
         exit(1);
     }
     opts = (opts | O_NONBLOCK);
     if(fcntl(sockfd, F_SETFL, opts) < 0) {
         perror("fcntl(F_SETFL)\n");
         exit(1);
     }
 }
 
 int main(){
     struct epoll_event ev, events[MAX_EVENTS];
     int addrlen, listenfd, conn_sock, nfds, epfd, fd, i, nread, n;
     struct sockaddr_in local, remote;
     char buf[BUFSIZ];
 
     //����listen socket
     if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         perror("sockfd\n");
         exit(1);
     }
     setnonblocking(listenfd);
     bzero(&local, sizeof(local));
     local.sin_family = AF_INET;
     local.sin_addr.s_addr = htonl(INADDR_ANY);;
     local.sin_port = htons(PORT);
     if( bind(listenfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
         perror("bind\n");
         exit(1);
     }
     listen(listenfd, 20);
 
     epfd = epoll_create(MAX_EVENTS);
     if (epfd == -1) {
         perror("epoll_create");
         exit(EXIT_FAILURE);
     }
 
     ev.events = EPOLLIN;
     ev.data.fd = listenfd;
     if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
         perror("epoll_ctl: listen_sock");
         exit(EXIT_FAILURE);
     }
 
     for (;;) {
         nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
         if (nfds == -1) {
             perror("epoll_pwait");
             exit(EXIT_FAILURE);
         }
 
         for (i = 0; i < nfds; ++i) {
             fd = events[i].data.fd;
             if (fd == listenfd) {
                 while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote, 
                                 (size_t *)&addrlen)) > 0) {
                     setnonblocking(conn_sock);
                     ev.events = EPOLLIN | EPOLLET;
                     ev.data.fd = conn_sock;
                     if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock,
                                 &ev) == -1) {
                         perror("epoll_ctl: add");
                         exit(EXIT_FAILURE);
                     }
                 }
                 if (conn_sock == -1) {
                     if (errno != EAGAIN && errno != ECONNABORTED 
                             && errno != EPROTO && errno != EINTR) 
                         perror("accept");
                 }
                 continue;
             }  
             if (events[i].events & EPOLLIN) {
                 n = 0;
                 while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {
                     n += nread;
                 }
                 if (nread == -1 && errno != EAGAIN) {
                     perror("read error");
                 }
                 ev.data.fd = fd;
                 ev.events = events[i].events | EPOLLOUT;
                 if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                     perror("epoll_ctl: mod");
                 }
             }
             if (events[i].events & EPOLLOUT) {
                 sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nHello World", 11);
                 int nwrite, data_size = strlen(buf);
                 n = data_size;
                 while (n > 0) {
                     nwrite = write(fd, buf + data_size - n, n);
                     if (nwrite < n) {
                         if (nwrite == -1 && errno != EAGAIN) {
                             perror("write error");
                         }
                         break;
                     }
                     n -= nwrite;
                 }
                 close(fd);
             }
         }
     }
 
     return 0;
 } 







�����и��ǳ����Ե����⣬����ĳһʱ�̣������ռ����¼�������ʱ����ʵ��100����
���еĴ󲿷ֶ���û���¼������ġ���ˣ����ÿ���ռ��¼�ʱ��������100�����ӵ��׽�
�ִ�������ϵͳ�������Ⱦ����û�̬�ڴ浽�ں�̬�ڴ�Ĵ������ƣ������ɲ���ϵͳ�ں�Ѱ
����Щ��������û��δ������¼��������Ǿ޴����Դ�˷ѣ�Ȼ��select��poll����������
�ģ�����������ֻ�ܴ���ǧ���������ӡ���epoll��������������Linux�ں���������
һ�����׵��ļ�ϵͳ����ԭ�ȵ�һ��select����poll���÷ֳɶ�3�����֣�����epoll_create
����1��epoll������epoll�ļ�ϵͳ�и�������������Դ��������epoll_ctl��epoll����
�������100������ӵ��׽��֡�����epoll_wait�ռ������¼������ӡ�������ֻ��Ҫ�ڽ�
������ʱ����1��epoll���󣬲�����Ҫ��ʱ��������ӻ�ɾ�����ӾͿ����ˣ���ˣ���ʵ
���ռ��¼�ʱ��epoll_wait��Ч�ʾͻ�ǳ��ߣ���Ϊ����epoll_waitʱ��û������������100
������ӣ��ں�Ҳ����Ҫȥ����ȫ�������ӡ�
    ��ô��Linux�ں˽����ʵ�����ϵ��뷨�أ�������Linux�ں�2.6.35�汾Ϊ������
��˵��һ��epoll����θ�Ч�����¼��ġ�ͼ9-5չʾ��epoll���ڲ���Ҫ���ݽṹ�����
���ŵġ�
    ��ĳһ�����̵���epoll_create����ʱ��Linux�ں˻ᴴ��һ��eventpoll�ṹ�壬���
�ṹ������������Ա��epoll��ʹ�÷�ʽ������أ�������ʾ��
struct eventpoll  (
    �����޺����ĸ��ڵ㣬������д洢��������ӵ�epoll�е��¼���Ҳ�������epoll��ص��¼�����
    struct rb_root rbr;
    ����˫������rdllist�����Ž�Ҫͨ��epoll_wait���ظ��û��ġ������������¼�
    struct list__ head rdllist;
)��
    ÿһ��epoll������һ��������eventpoll�ṹ�壬����ṹ������ں˿ռ��д����
�����ڴ棬���ڴ洢ʹ��epoll_ctl������epoll��������ӽ������¼�����Щ�¼�����ҵ�
rbr������У��������ظ���ӵ��¼��Ϳ���ͨ�����������Ч��ʶ�������epoll_ctl������
�ܿ죩��Linux�ں��е���ú�������7���н��ܵ�Nginx������Ƿǳ����Ƶģ����Բ���
ngx_rbtree_t����������⡣
    ͼ9-5 epollԭ��ʾ��ͼ
    ������ӵ�epoll�е��¼��������豸���������������������ص���ϵ��Ҳ����˵��
��Ӧ���¼�����ʱ���������Ļص�����������ص��������ں��н���ep_poll_callback����
����������¼��ŵ������rdllist˫�������С�����ں��е�˫��������ngx_queue_t����
��������ȫ��ͬ�ģ�Nginx������Linux�ں˴�������ƣ������ǿ��Բ�������⡣��epoll
�У�����ÿһ���¼����Ὠ��һ��epitem�ṹ�壬������ʾ��
struct epitem{
    ����������ڵ㣬��ngx_rbtree_node�ߺ�����ڵ�����
    struct rb node rbn;
����˫������ڵ㣬��ngx_queue_t˫������ڵ�����
struct list head rdllink;
�����¼��������Ϣ
struct  epoll_filefd  f fd;
����ָ����������eventpoll����
struct eventpoll *ep;
�����ڴ����¼�����
    struct  epoll_event  event;
    );
    �������ÿһ���¼���Ӧ�ŵ���Ϣ��
    ������epoU��wait����Ƿ��з����¼�������ʱ��ֻ�Ǽ��eventpon�����е�rdllist˫
�������Ƿ�����itemԪ�ض��ѣ����rd11ist����Ϊ�գ����������¼����Ƶ��û�̬�ڴ�
�У�ͬʱ���¼��������ظ��û�����ˣ�ep01l__wait��Ч�ʷǳ��ߡ�epoU��ctl����epol1����
����ӡ��޸�iɾ���¼�ʱ����rbr������в����¼�Ҳ�ǳ��죬Ҳ����˵��ep011�Ƿǳ���
Ч�ģ����������׵ش�����򼶱�Ĳ������ӡ�


����select poll��ʱ��ÿ�λ�ȡ�����ݺ󻹵�����FD_ZERO FD_SET������������Ӧ�ò���ں�̬�����ݿ�����Ӱ�����ܡ�

Epoll�ĸ�Ч�������ݽṹ��������ܲ��ɷֵģ��������ͻ��ᵽ��
���Ȼ���һ��selectģ�ͣ�����I/O�¼�����ʱ��select֪ͨӦ�ó������¼����˿�ȥ������Ӧ�ó��������ѯ���е�FD���ϣ�����ÿ��FD�Ƿ����¼��������������¼�������������������
int res = select(maxfd+1, &readfds, NULL, NULL, 120);

if(res > 0)

{

    for(int i = 0; i < MAX_CONNECTION; i++)

    {

        if(FD_ISSET(allConnection[i],&readfds))

        {

            handleEvent(allConnection[i]);

        }

    }

}
// if(res == 0) handle timeout, res < 0 handle error

 
Epoll���������Ӧ�ó�����I/0�¼��������������Ӧ�ó�����ص���Ϣ����Щ��Ϣ��Ӧ�ó������ģ���˸�����Щ��ϢӦ�ó������ֱ�Ӷ�
λ���¼��������ر�������FD���ϡ�
intres = epoll_wait(epfd, events, 20, 120);

for(int i = 0; i < res;i++)

{
    handleEvent(events[n]);

}


    ��̽��Nginx���������¼�������ܡ���ι���ͬ���¼���
��ģ��ģ��������н���epollΪ��������Linux����ϵͳ�ں������ʵ��epoll�¼�������
�Ƶģ��ڼ��˽������÷��󣬻��һ��˵��ngx_epoll_moduleģ������λ���epollʵ��
Nginx���¼������ġ��������߾ͻ��Nginx�������¼�������Ʒ�����ȫ����˽⣬ͬʱ
����Ū���Nginx�ڼ�ʮ�򲢷������������������Ч���÷�������Դ�ġ�




    ����һ����������100���û�ͬʱ��һ�����̱�����TCP���ӣ���ÿһʱ��ֻ�м�ʮ��
�򼸰ٸ�TCP�����ǻ�Ծ�ģ����յ�TCP������Ҳ����˵����ÿһʱ�̣�����ֻ��Ҫ������
100�������е�һС�������ӡ���ô����β��ܸ�Ч�ش������ֳ����أ������Ƿ���ÿ��ѯ
�ʲ���ϵͳ�ռ����¼�������TCP����ʱ������100������Ӹ��߲���ϵͳ��Ȼ���ɲ���ϵ
ͳ�ҳ��������¼������ļ��ٸ������أ�ʵ���ϣ���Linux�ں�2.4�汾��ǰ����ʱ��select
����poll�¼�������ʽ�����������ġ�
    �����и��ǳ����Ե����⣬����ĳһʱ�̣������ռ����¼�������ʱ����ʵ��100����
���еĴ󲿷ֶ���û���¼������ġ���ˣ����ÿ���ռ��¼�ʱ��������100�����ӵ��׽�
�ִ�������ϵͳ�������Ⱦ����û�̬�ڴ浽�ں�̬�ڴ�Ĵ������ƣ������ɲ���ϵͳ�ں�Ѱ
����Щ��������û��δ������¼��������Ǿ޴����Դ�˷ѣ�Ȼ��select��poll����������
�ģ�����������ֻ�ܴ���ǧ���������ӡ���epoll��������������Linux�ں���������
һ�����׵��ļ�ϵͳ����ԭ�ȵ�һ��select����poll���÷ֳɶ�3�����֣�����epoll_create
����1��epoll������epoll�ļ�ϵͳ�и�������������Դ��������epoll_ctl��epoll����
�������100������ӵ��׽��֡�����epoll_wait�ռ������¼������ӡ�������ֻ��Ҫ�ڽ�
������ʱ����1��epoll���󣬲�����Ҫ��ʱ��������ӻ�ɾ�����ӾͿ����ˣ���ˣ���ʵ
���ռ��¼�ʱ��epoll_wait��Ч�ʾͻ�ǳ��ߣ���Ϊ����epoll_waitʱ��û������������100
������ӣ��ں�Ҳ����Ҫȥ����ȫ�������ӡ�
    ��ô��Linux�ں˽����ʵ�����ϵ��뷨�أ�������Linux�ں�2.6.35�汾Ϊ������
��˵��һ��epoll����θ�Ч�����¼��ġ�ͼ9-5չʾ��epoll���ڲ���Ҫ���ݽṹ�����
���ŵġ�
    ��ĳһ�����̵���epoll_create����ʱ��Linux�ں˻ᴴ��һ��eventpoll�ṹ�壬���
�ṹ������������Ա��epoll��ʹ�÷�ʽ������أ�������ʾ��
struct eventpoll  (
    �����޺����ĸ��ڵ㣬������д洢��������ӵ�epoll�е��¼���Ҳ�������epoll��ص��¼�����
    struct rb_root rbr;
    ����˫������rdllist�����Ž�Ҫͨ��epoll_wait���ظ��û��ġ������������¼�
    struct list__ head rdllist;
)��
    ÿһ��epoll������һ��������eventpoll�ṹ�壬����ṹ������ں˿ռ��д����
�����ڴ棬���ڴ洢ʹ��epoll_ctl������epoll��������ӽ������¼�����Щ�¼�����ҵ�
rbr������У��������ظ���ӵ��¼��Ϳ���ͨ�����������Ч��ʶ�������epoll_ctl������





312�������������Nginx
�ܿ죩��Linux�ں��е���ú�������7���н��ܵ�Nginx������Ƿǳ����Ƶģ����Բ���
ngx_rbtree_t����������⡣
    ͼ9-5 epollԭ��ʾ��ͼ
    ������ӵ�epoll�е��¼��������豸���������������������ص���ϵ��Ҳ����˵��
��Ӧ���¼�����ʱ���������Ļص�����������ص��������ں��н���ep_poll_callback����
����������¼��ŵ������rdllist˫�������С�����ں��е�˫��������ngx_queue_t����
��������ȫ��ͬ�ģ�Nginx������Linux�ں˴�������ƣ������ǿ��Բ�������⡣��epoll
�У�����ÿһ���¼����Ὠ��һ��epitem�ṹ�壬������ʾ��
struct epitem{
    ����������ڵ㣬��ngx_rbtree_node�ߺ�����ڵ�����
    struct rb node rbn;
����˫������ڵ㣬��ngx_queue_t˫������ڵ�����
struct list head rdllink;
�����¼��������Ϣ
struct  epoll_filefd  f fd;
����ָ����������eventpoll����
struct eventpoll *ep;
�����ڴ����¼�����

    struct  epoll_event  event;
    )j
    �������ÿһ���¼���Ӧ�ŵ���Ϣ��
    ������epoU��wait����Ƿ��з����¼�������ʱ��ֻ�Ǽ��eventpon�����е�rdllist˫
�������Ƿ�����itemԪ�ض��ѣ����rd11ist����Ϊ�գ����������¼����Ƶ��û�̬�ڴ�
�У�ͬʱ���¼��������ظ��û�����ˣ�ep01l__wait��Ч�ʷǳ��ߡ�epoU��ctl����epol1����
����ӡ��޸�iɾ���¼�ʱ����rbr������в����¼�Ҳ�ǳ��죬Ҳ����˵��ep011�Ƿǳ���
Ч�ģ����������׵ش�����򼶱�Ĳ������ӡ�
9.6.2���ʹ��epoll
    epollͨ������3��epollϵͳ����Ϊ�û��ṩ����
    (1) epoll_, createϵͳ����
    epoll_create��c���е�ԭ�����¡�
    epoll_create����һ�������֮��epoll��ʹ�ö�����������������ʶ������size�Ǹ�
��epoll��Ҫ����Ĵ����¼���Ŀ������ʹ��epollʱ���������close�ر���������
    ע��size����ֻ�Ǹ����ں����epoll����ᴦ����¼�������Ŀ���������ܹ�����
���¼�������������Linuxީ�µ�һЩ�ں˰汾��ʵ���У����size����û���κ����塣
    (2) epoll_ctlϵͳ����
    epoll_ctl��C���е�ԭ�����¡�
    int epoll_ctl(int  epfd, int  op, int  fd, struct  epoll_event*  event)j
    epoll_ctl��epoll��������ӡ��޸Ļ���ɾ������Ȥ���¼�������0��ʾ�ɹ�������
��һ1����ʱ��Ҫ����errno�������жϴ������͡�epoll_wait�������ص��¼���Ȼ��ͨ��
epoll_ctl��ӹ�epoll�еġ�����epfd��epoll_create���صľ������op�������������
9-2��
�����������������������ש���������������������������
��    ��    op��ȡֵ   ��    ����                  ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_ADD     ��    ����µ��¼���epoll�� ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL MOD     ��    �޸�epoll�е��¼�     ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_DEL     ��    ɾ��epoll�е��¼�     ��
�����������������������ߩ���������������������������
    ��3������fd�Ǵ����������׽��֣���4���������ڸ���epoll��ʲô�����¼���
��Ȥ����ʹ����epoll_event�ṹ�壬�����Ľ��ܹ���epollʵ�ֻ����л�Ϊÿһ���¼���
��epitem�ṹ�壬����epitem����һ��epoll_event���͵�event��Ա�����濴һ��epoll_
event�Ķ��塣
struct epoll_event{
        _uint32 t events;
        epoll data_t data;
};
events��ȡֵ����9-3��
��9-3 epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)                   ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������
��data��Ա��һ��epoll_data���ϣ��䶨�����¡�
typedef union epoll_data
       void        *ptr;
      int           fd;
     uint32_t            u32 ;
     uint64_t            u64 ;
} epoll_data_t;
    �ɼ������data��Ա��������ʹ�÷�ʽ��ء����磬ngx_epoll_moduleģ��ֻʹ����
�����е�ptr��Ա����Ϊָ��ngx_connection��t���ӵ�ָ�롣
    (3) epoll_waitϵͳ����
    epoll_wait��C���е�ԭ�����¡�
int  epoll_wait (int  epfd, struct  epoll_event*  events, int  maxevents, int  timeout) ;
    �ռ���epoll��ص��¼����Ѿ��������¼������epoll��û���κ�һ���¼�����������
��ȴ�timeout����󷵻ء�epoll_wait�ķ���ֵ��ʾ��ǰ�������¼��������������0����
��ʾ���ε�����û���¼��������������һ1�����ʾ���ִ�����Ҫ���errno�������ж�



�������͡���1������epfd��epoll������������2������events���Ƿ���õ�epoll_event
�ṹ�����飬��ou����ѷ������¼����Ƶ�eVents�����У�eVents�������ǿ�ָ�룬�ں�ֻ
��������ݸ��Ƶ����events�����У�����ȥ�����������û�̬�з����ڴ档�ں���������
Ч�ʺܸߣ�����3������maxevents��ʾ���ο��Է��ص�����¼���Ŀ��ͨ��maxevents����
��Ԥ�����eV|ents����Ĵ�С����ȵġ���4������timeout��ʾ��û�м�⵽�¼�����ʱ
���ȴ���ʱ�䣨��λΪ���룩�����timeoutΪ0�����ʾep01l��wait��rdllist������Ϊ�գ�
���̷��أ�����ȴ���
    ep011�����ֹ���ģʽ��LT��ˮƽ������ģʽ��ET����Ե������ģʽ��Ĭ������£�
ep011����LTģʽ��������ʱ���Դ��������ͷ������׽��֣�����9��3�е�EPOLLET��ʾ
���Խ�һ���¼���ΪETģʽ��ETģʽ��Ч��Ҫ��LTģʽ�ߣ���ֻ֧�ַ������׽��֡�ET
ģʽ��LTģʽ���������ڣ���һ���µ��¼�����ʱ��ETģʽ�µ�Ȼ���Դ�epoU��wait����
�л�ȡ������¼������Ǽӹ����û�а�����¼���Ӧ���׽��ֻ����������꣬������׽�
��û���µ��¼��ٴε���ʱ����ETģʽ�����޷��ٴδ�epoll__Wait�����л�ȡ����¼��ģ�
��LTģʽ���෴��ֻҪһ���¼���Ӧ���׽��ֻ������������ݣ������ܴ�epoll-wait�л�
ȡ����¼�����ˣ���LTģʽ�¿�������epoll��Ӧ��Ҫ��һЩ����̫���׳�������
ETģʽ���¼�����ʱ�����û�г��׵ؽ����������ݴ����꣬��ᵼ�»������е��û���
��ò�����Ӧ��Ĭ������£�Nginx��ͨ��ETģʽʹ��epoU�ģ��������оͿ��Կ������
���ݡ�


1)��epoll_create����
    ����������int epoll_create(int size)
   �ú�������һ��epollר�õ��ļ������������еĲ�����ָ�����������������Χ��

2)��epoll_ctl����
   ����������int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
   �ú������ڿ���ĳ���ļ��������ϵ��¼�������ע���¼����޸��¼���ɾ���¼���
������
   epfd���� epoll_create ���ɵ�epollר�õ��ļ���������
   op��Ҫ���еĲ��������ܵ�ȡֵEPOLL_CTL_ADD ע�ᡢEPOLL_CTL_MOD �޸ġ�   EPOLL_CTL_DEL ɾ����
   fd���������ļ���������
   event��ָ��epoll_event��ָ�룻
   ������óɹ��򷵻�0�����ɹ��򷵻�-1��

3)��epoll_wait����
   ����������int epoll_wait(int epfd,struct epoll_event * events,int maxevents,int timeout)
   �ú���������ѯI/O�¼��ķ�����
   ������
   epfd����epoll_create ���ɵ�epollר�õ��ļ���������
   epoll_event�����ڻش��������¼������飻
   maxevents��ÿ���ܴ�����¼�����
   timeout���ȴ�I/O�¼������ĳ�ʱֵ��
*/

#if (NGX_TEST_BUILD_EPOLL)

/* epoll declarations */
/*
epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)                  ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������

����Linux Epoll ETģʽEPOLLOUT��EPOLLIN����ʱ�� ETģʽ��Ϊ��Ե����ģʽ������˼�壬������Ե��������������ᴥ���ġ�

EPOLLOUT�¼���
EPOLLOUT�¼�ֻ��������ʱ����һ�Σ���ʾ��д������ʱ����Ҫ����������Ҫ��׼��������������
1.ĳ��write��д���˷��ͻ����������ش�����ΪEAGAIN��
2.�Զ˶�ȡ��һЩ���ݣ������¿�д�ˣ���ʱ�ᴥ��EPOLLOUT��
�򵥵�˵��EPOLLOUT�¼�ֻ���ڲ���д����д��ת��ʱ�̣��Żᴥ��һ�Σ����Խб�Ե��������з�û��ģ�

��ʵ������������ǿ�ƴ���һ�Σ�Ҳ���а취�ģ�ֱ�ӵ���epoll_ctl��������һ��event�Ϳ����ˣ�event��ԭ��������һģһ�����У����������EPOLLOUT�����ؼ����������ã��ͻ����ϴ���һ��EPOLLOUT�¼���

EPOLLIN�¼���
EPOLLIN�¼���ֻ�е��Զ�������д��ʱ�Żᴥ�������Դ���һ�κ���Ҫ���϶�ȡ��������ֱ������EAGAINΪֹ������ʣ�µ�����ֻ�����´ζԶ���д��ʱ����һ��ȡ�����ˡ�


*/
#define EPOLLIN        0x001
#define EPOLLPRI       0x002
#define EPOLLOUT       0x004
#define EPOLLRDNORM    0x040
#define EPOLLRDBAND    0x080
#define EPOLLWRNORM    0x100
#define EPOLLWRBAND    0x200
#define EPOLLMSG       0x400
#define EPOLLERR       0x008 //EPOLLERR|EPOLLHUP����ʾ�����쳣���  fd�������������  //epoll EPOLLERR|EPOLLHUPʵ������ͨ��������д�¼����ж�д����recv write����������쳣
#define EPOLLHUP       0x010


#define EPOLLRDHUP     0x2000 //���Զ��Ѿ��رգ�����д���ݣ���������¼�

#define EPOLLET        0x80000000 //��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)  
//���øñ�Ǻ��ȡ�����ݺ�������ڴ������ݹ����������µ����ݵ��������ᴥ��epoll_wait���أ��������ݴ�����Ϻ�����add epoll_ctl�������ο�<linux�����ܷ���������>9.3.4��
#define EPOLLONESHOT   0x40000000

#define EPOLL_CTL_ADD  1
#define EPOLL_CTL_DEL  2
#define EPOLL_CTL_MOD  3

typedef union epoll_data {
    void         *ptr; //��ngx_epoll_add_event�и�ֵΪngx_connection_t����
    int           fd;
    uint32_t      u32;
    uint64_t      u64;
} epoll_data_t;

struct epoll_event {
/*
��9-3 epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(L1��)                  ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������
*/
    uint32_t      events;
    epoll_data_t  data;
};


int epoll_create(int size);

int epoll_create(int size)
{
    return -1;
}


int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//EPOLL_CTL_ADDһ�κ󣬾Ϳ���һֱͨ��epoll_wait����ȡ���¼������ǵ���EPOLL_CTL_DEL������ÿ�ζ��¼�����epoll_wait���غ�Ҫ�������EPOLL_CTL_ADD��
//֮ǰ�������еĵط�����ע���ˣ���עΪÿ�ζ��¼�������Ҫ����addһ�Σ�epoll_ctr addд�¼�һ��
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    return -1;
}


int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout);

/*
//ngx_notify->ngx_epoll_notifyֻ�ᴥ��epoll_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
*/
int epoll_wait(int epfd, struct epoll_event *events, int nevents, int timeout) //timeoutΪ-1��ʾ���޵ȴ�
{
    return -1;
}

#if (NGX_HAVE_EVENTFD)
#define SYS_eventfd       323
#endif

#if (NGX_HAVE_FILE_AIO)

#define SYS_io_setup      245
#define SYS_io_destroy    246
#define SYS_io_getevents  247

typedef u_int  aio_context_t;

struct io_event {
    uint64_t  data;  /* the data field from the iocb */ //���ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_data��һ�µ�
    uint64_t  obj;   /* what iocb this event came from */ //ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    int64_t   res;   /* result code for this event */
    int64_t   res2;  /* secondary result */
};


#endif
#endif /* NGX_TEST_BUILD_EPOLL */

//��Ա��Ϣ��ngx_epoll_commands
typedef struct {
    /*
    events�ǵ���epoll_wait����ʱ���˵ĵ�3������maxevents������2������events����Ĵ�СҲ�����������ģ����潫��ngx_epoll_init�����г�ʼ���������
     */
    ngx_uint_t  events; // "epoll_events"��������  Ĭ��512 ��ngx_epoll_init_conf
    ngx_uint_t  aio_requests; // "worker_aio_requests"��������  Ĭ��32 ��ngx_epoll_init_conf
} ngx_epoll_conf_t;


static ngx_int_t ngx_epoll_init(ngx_cycle_t *cycle, ngx_msec_t timer);
#if (NGX_HAVE_EVENTFD)
static ngx_int_t ngx_epoll_notify_init(ngx_log_t *log);
static void ngx_epoll_notify_handler(ngx_event_t *ev);
#endif
static void ngx_epoll_done(ngx_cycle_t *cycle);
static ngx_int_t ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event,
    ngx_uint_t flags);
static ngx_int_t ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event,
    ngx_uint_t flags);
static ngx_int_t ngx_epoll_add_connection(ngx_connection_t *c);
static ngx_int_t ngx_epoll_del_connection(ngx_connection_t *c,
    ngx_uint_t flags);
#if (NGX_HAVE_EVENTFD)
static ngx_int_t ngx_epoll_notify(ngx_event_handler_pt handler);
#endif
static ngx_int_t ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer,
    ngx_uint_t flags);

#if (NGX_HAVE_FILE_AIO)
static void ngx_epoll_eventfd_handler(ngx_event_t *ev);
#endif

static void *ngx_epoll_create_conf(ngx_cycle_t *cycle);
static char *ngx_epoll_init_conf(ngx_cycle_t *cycle, void *conf);

static int                  ep = -1; //ngx_epoll_init -> epoll_create�ķ���ֵ
static struct epoll_event  *event_list; //epoll_events��sizeof(struct epoll_event) * nevents, ��ngx_epoll_init
static ngx_uint_t           nevents; //nerentsҲ��������epoll_events�Ĳ���

#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
static int                  notify_fd = -1; //��ʼ����ngx_epoll_notify_init
static ngx_event_t          notify_event;
static ngx_connection_t     notify_conn;
#endif

#if (NGX_HAVE_FILE_AIO)
//����֪ͨ�첽I/O�¼���������������iocb�ṹ���е�aio_resfd��Ա��һ�µ�  ͨ����fd��ӵ�epoll�¼��У��Ӷ����Լ���첽io�¼�
int                         ngx_eventfd = -1;
//�첽I/O�������ģ�ȫ��Ψһ�����뾭��io_setup��ʼ������ʹ��
aio_context_t               ngx_aio_ctx = 0; //ngx_epoll_aio_init->io_setup����
//�첽I/O�¼���ɺ����֪ͨ����������Ҳ����ngx_eventfd����Ӧ��ngx_event_t�¼�
static ngx_event_t          ngx_eventfd_event; //���¼�
//�첽I/O�¼���ɺ����֪ͨ��������ngx_eventfd����Ӧ��ngx_connectiont����
static ngx_connection_t     ngx_eventfd_conn;

#endif

static ngx_str_t      epoll_name = ngx_string("epoll");

static ngx_command_t  ngx_epoll_commands[] = {
    /*
    �ڵ���epoll_waitʱ�����ɵ�2�͵�3����������Linux�ں�һ�����ɷ��ض��ٸ��¼�������������ʾ����һ��epoll_waitʱ���ɷ���
    ���¼�������Ȼ����Ҳ��Ԥ������ô��epoll_event�ṹ�����ڴ洢�¼�
     */
    { ngx_string("epoll_events"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      0,
      offsetof(ngx_epoll_conf_t, events),
      NULL },

    /*
    �ڿ����첽I/O��ʹ��io_setupϵͳ���ó�ʼ���첽I/O�����Ļ���ʱ����ʼ������첽I/O�¼�����
     */
    { ngx_string("worker_aio_requests"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      0,
      offsetof(ngx_epoll_conf_t, aio_requests),
      NULL },

      ngx_null_command
};


ngx_event_module_t  ngx_epoll_module_ctx = {
    &epoll_name,
    ngx_epoll_create_conf,               /* create configuration */
    ngx_epoll_init_conf,                 /* init configuration */

    {
        ngx_epoll_add_event,             /* add an event */  //ngx_add_event
        ngx_epoll_del_event,             /* delete an event */
        ngx_epoll_add_event,             /* enable an event */ //ngx_add_conn
        ngx_epoll_del_event,             /* disable an event */
        ngx_epoll_add_connection,        /* add an connection */
        ngx_epoll_del_connection,        /* delete an connection */
#if (NGX_HAVE_EVENTFD)
        ngx_epoll_notify,                /* trigger a notify */
#else
        NULL,                            /* trigger a notify */
#endif
        ngx_epoll_process_events,        /* process the events */
        ngx_epoll_init,                  /* init the events */ //�ڴ������ӽ�����ִ��
        ngx_epoll_done,                  /* done the events */
    }
};

ngx_module_t  ngx_epoll_module = {
    NGX_MODULE_V1,
    &ngx_epoll_module_ctx,               /* module context */
    ngx_epoll_commands,                  /* module directives */
    NGX_EVENT_MODULE,                    /* module type */
    NULL,                                /* init master */
    NULL,                                /* init module */
    NULL,                                /* init process */
    NULL,                                /* init thread */
    NULL,                                /* exit thread */
    NULL,                                /* exit process */
    NULL,                                /* exit master */
    NGX_MODULE_V1_PADDING
};


#if (NGX_HAVE_FILE_AIO)

/*
 * We call io_setup(), io_destroy() io_submit(), and io_getevents() directly
 * as syscalls instead of libaio usage, because the library header file
 * supports eventfd() since 0.3.107 version only.
 */
/*
 Linux�ں��ṩ��5��ϵͳ��������ļ��������첽I/O���ܣ�����9-7��
��9-7  Linux�ں��ṩ���ļ��첽1/0��������
�������������������������������������������ש������������������������������������ש�������������������������������
��    ������                              ��    ��������                        ��    ִ������                  ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��  nr events��ʾ��Ҫ��ʼ�����첽     ��  ��ʼ���ļ��첽I/O�������ģ� ��
��int io_setup(unsigned nr_events,        ��1/0�����Ŀ��Դ�����¼�����С��     ��ִ�гɹ���ctxp���Ƿ��������  ��
��aio context_t *ctxp)                    ������ctxp���ļ��첽1/0������������   ����������������첽1/0������   ��
��                                        ��                                    �������ٿ��Դ���nr- events����  ��
��                                        ����ָ��                              ��                              ��
��                                        ��                                    ����������0��ʾ�ɹ�             ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��                                    ��  �����ļ��첽1/0�������ġ�   ��
��int io_destroy (aio_context_t ctx)      ��  ctx���ļ��첽1/0��������������    ��                              ��
��                                        ��                                    ������0��ʾ�ɹ�                 ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_submit(aiio_context_t ctx,       ��  ctx���ļ��첽1/0����������������  ��  �ύ�ļ��첽1/0����������   ��
��                                        ��nr��һ���ύ���¼�������cbp����Ҫ   ��                              ��
��long nr, struct iocb *cbp[])            ��                                    ��ֵ��ʾ�ɹ��ύ���¼�����      ��
��                                        ���ύ���¼������е�����Ԫ�ص�ַ      ��                              ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_cancel(aio_context_t ctx, struct ��  ctx���ļ��첽1/0������������      ��  ȡ��֮ǰʹ��io��sumbit�ύ  ��
��                                        ������iocb��Ҫȡ�����첽1/0��������   ����һ���ļ��첽I/O����������   ��
��iocb *iocb, struct io_event *result)    ��                                    ��                              ��
��                                        ��result��ʾ���������ִ�н��        ��O��ʾ�ɹ�                     ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                I                       ��  ctx���ļ��첽1/0������������      ��                              ��
��int io_getevents(aio_context_t ctx,     ������min_nr��ʾ����Ҫ��ȡmln_ nr��   ��                              ��
��long min_nr, lon, g nr,                 ���¼�����nr��ʾ�����ȡnr���¼���    ��  ���Ѿ���ɵ��ļ��첽I/O��   ��
��struct io  event "*events, struct       ������events����ĸ���һ������ͬ�ģ�  ���������ж�ȡ����              ��
��timespec *timeout)                      ��events��ִ����ɵ��¼����飻tlmeout ��                              ��
��         I "                            ���ǳ�ʱʱ�䣬Ҳ�����ڻ�ȡmm- nr��    ��                              ��
��                                        ���¼�ǰ�ĵȴ�ʱ��                    ��                              ��
�������������������������������������������ߩ������������������������������������ߩ�������������������������������
    ��9-7���оٵ���5�ַ����ṩ���ں˼�����ļ��첽I/O���ƣ�ʹ��ǰ��Ҫ�ȵ���io_setup������ʼ���첽I/O�����ġ���Ȼһ�����̿���ӵ��
����첽I/O�����ģ���ͨ����һ�����㹻�ˡ�����io_setup�������������첽I/O�����ĵ���������aio��context_t��
�ͣ��������������epoll_create���ص�������һ�����ǹᴩʼ�յġ�ע�⣬nr_ eventsֻ��ָ�����첽I/O���ٳ�ʼ��������������������û��
���������Դ�����첽I/O�¼���Ŀ��Ϊ�˱�����⣬������io_setup��epoll_create���жԱȣ����ǻ��Ǻ����Ƶġ�
    ��Ȼ��epoll���첽I/O���жԱȣ���ô��Щ�����൱��epoll_ctrl�أ�����io_submit��io_cancel������io_submit�൱�����첽I/O������¼���
��io_cancel���൱�ڴ��첽I/O���Ƴ��¼���io_submit���õ���һ���ṹ��iocb������򵥵ؿ�һ�����Ķ��塣
    struct iocb  f
    ��t�洢��ҵ����Ҫ��ָ�롣���磬��Nginx�У�����ֶ�ͨ���洢�Ŷ�Ӧ��ngx_event_tͤ����ָ
�롣��ʵ������io_getevents�����з��ص�io event�ṹ���data��Ա����ȫһ�µ�+��
    u int64 t aio data;
7 7����Ҫ����
u  int32��PADDED (aio_key,  aio_raservedl)j
���������룬��ȡֵ��Χ��io_iocb_cmd_t�е�ö������
u int16 t aio lio_opcode;
������������ȼ�
int16 t aio_reqprio,
����i41-������
u int32 t aio fildes;
��������д������Ӧ���û�̬������
u int64 t aio buf;
��������д�������ֽڳ���
u int64 t aio_nbytes;
��������д������Ӧ���ļ��е�ƫ����
int64 t aio offset;
7 7�����ֶ�
u int64��aio reserved2;
    ��+��ʾ��������ΪIOCB FLAG RESFD����������ں˵����첽I/O���������ʱʹ��eventfd��
��֪ͨ������epoll���ʹ��+��
    u int32��aio_flags��
������ʾ��ʹ��IOCB FLAG RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
U int32 t aio resfd;
    ��ˣ������ú�iocb�ṹ��󣬾Ϳ������첽I/O�ύ�¼��ˡ�aio_lio_opcode������
ָ��������¼��Ĳ������ͣ�����ȡֵ��Χ���¡�
    typedef enum io_iocb_cmd_t{
    �����첽������
    IO_CMD_PREAD=O��
    �����첽д����
    IO_CMD_PWRITE=1��
    ����ǿ��ͬ��
    IO_CMD_FSYNC=2��
    ����Ŀǰ��ʹ��
    IO_CMD_FDSYNC=3��
    ����Ŀǰδʹ��
    IO_CMD_POLL=5��
    ���������κ�����
    IO_CMD_NOOP=6��
    )  io_iocb_cmd_t
    ��Nginx�У���ʹ����IO_CMD_PREAD���������ΪĿǰ��֧���ļ����첽I/O��ȡ����֧���첽I/O��д�롣������һ����Ҫ��ԭ�����ļ���
�첽I/O�޷����û��棬��д�ļ�����ͨ�����䵽�����еģ�Linux����ͳһ�������С��ࡱ����ˢ�µ����̵Ļ��ơ�
    ������ʹ��io submit���ں��ύ���ļ��첽I/O�������¼�����ʹ��io_cancel����Խ��Ѿ��ύ���¼�ȡ����
    ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�epoll_wait�����������õ���io_event�ṹ�壬���濴һ�����Ķ��塣
struct io event  {
    �������ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_data��һ�µ�
    uint64 t  data;
    ����ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    uint64_t   obj��
    �����첽I/O����Ľṹ��res���ڻ����0ʱ��ʾ�ɹ���С��0ʱ��ʾʧ��
    int64һt    res��
    ��7��������
    int64һ��    res2 j
)��
    ���������ݻ�ȡ��io��event�ṹ�����飬�Ϳ��Ի���Ѿ���ɵ��첽I/O�����ˣ��ر���iocb�ṹ���е�aio data��Ա��io_event�е�data����
���ڴ���ָ�룬Ҳ����˵��ҵ���е����ݽṹ���¼���ɺ�Ļص������������
    �����˳�ʱ��Ҫ����io_destroy���������첽I/O�����ģ����൱�ڵ���close�ر�epoll����������
    Linux�ں��ṩ���ļ��첽I/O�����÷��ǳ��򵥣���������������ں���CPU��I/O�豸�Ǹ��Զ�����������һ���ԣ����ύ���첽I/O������
������ȫ����������������ֱ�����������鿴�첽I/O�����Ƿ���ɡ�
*/
//��ʼ���ļ��첽I/O��������
static int
io_setup(u_int nr_reqs, aio_context_t *ctx) //�൱��epoll�е�epoll_create
{
    return syscall(SYS_io_setup, nr_reqs, ctx);
}


static int
io_destroy(aio_context_t ctx)
{
    return syscall(SYS_io_destroy, ctx);
}

// ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�epoll_wait����
static int
io_getevents(aio_context_t ctx, long min_nr, long nr, struct io_event *events,
    struct timespec *tmo) //io_submitΪ����¼�
{
    return syscall(SYS_io_getevents, ctx, min_nr, nr, events, tmo);
}

/*
ngx_epoll_aio_init��ʼ��aio�¼��б� ngx_file_aio_read��Ӷ��ļ��¼�������ȡ��Ϻ�epoll�ᴥ��
ngx_epoll_eventfd_handler->ngx_file_aio_event_handler 
nginx file aioֻ�ṩ��read�ӿڣ����ṩwrite�ӿڣ���Ϊ�첽aioֻ�Ӵ��̶���д������aio��ʽһ��д���䵽
���̻��棬���Բ��ṩ�ýӿڣ�����첽ioд���ܻ����
*/

//aioʹ�ÿ��Բο�ngx_http_file_cache_aio_read  ngx_output_chain_copy_buf ����Ϊ��ȡ�ļ�׼����
static void
ngx_epoll_aio_init(ngx_cycle_t *cycle, ngx_epoll_conf_t *epcf)
{
    int                 n;
    struct epoll_event  ee;

/*
����eventfdϵͳ�����½�һ��eventfd���󣬵ڶ��������е�0��ʾeventfd�ļ�������ʼֵΪ0�� ϵͳ���óɹ����ص�����eventfd���������������
*/
#if (NGX_HAVE_SYS_EVENTFD_H)
    ngx_eventfd = eventfd(0, 0); //  
#else
    ngx_eventfd = syscall(SYS_eventfd, 0); //ʹ��Linux�е�323��ϵͳ���û�ȡһ�����������
#endif

    if (ngx_eventfd == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "eventfd() failed");
        ngx_file_aio = 0;
        return;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "aio eventfd: %d", ngx_eventfd);

    n = 1;

    if (ioctl(ngx_eventfd, FIONBIO, &n) == -1) { //����ngx_eventfdΪ������
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "ioctl(eventfd, FIONBIO) failed");
        goto failed;
    }

    if (io_setup(epcf->aio_requests, &ngx_aio_ctx) == -1) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                      "io_setup() failed");
        goto failed;
    }

    ngx_eventfd_event.data = &ngx_eventfd_conn;//���������첽I/O���֪ͨ��ngx_eventfd_event�¼�������ngx_connection_t�����Ƕ�Ӧ��

    //�ú�����ngx_epoll_process_events��ִ��
    ngx_eventfd_event.handler = ngx_epoll_eventfd_handler; //���첽I/O�¼���ɺ�ʹ��ngx_epoll_eventfd handler��������
    ngx_eventfd_event.log = cycle->log;
    ngx_eventfd_event.active = 1;
    ngx_eventfd_conn.fd = ngx_eventfd;
    ngx_eventfd_conn.read = &ngx_eventfd_event; //ngx_eventfd_conn���ӵĶ��¼����������ngx_eventfd_event
    ngx_eventfd_conn.log = cycle->log;

    ee.events = EPOLLIN|EPOLLET;
    ee.data.ptr = &ngx_eventfd_conn;

    //��epoll����ӵ��첽I/O��֪ͨ������ngx_eventfd
    /*
    ������ngx_epoll_aio init��������첽I/O��epoll�����������ĳһ���첽I/O�¼���ɺ�ngx_eventfdѮ���ʹ��ڿ���״̬������epoll_wait��
    ����ngx_eventfd event�¼���ͻ�������Ļص�����ngx_epoll_eventfd handler�����Ѿ���ɵ��첽I/O�¼�
     */
    if (epoll_ctl(ep, EPOLL_CTL_ADD, ngx_eventfd, &ee) != -1) { //�ɹ���ֱ�ӷ���
        return;
    }

    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                  "epoll_ctl(EPOLL_CTL_ADD, eventfd) failed");

    //epoll_ctlʧ��ʱ�����aio��context   

    if (io_destroy(ngx_aio_ctx) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "io_destroy() failed");
    }

failed:

    if (close(ngx_eventfd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "eventfd close() failed");
    }

    ngx_eventfd = -1;
    ngx_aio_ctx = 0;
    ngx_file_aio = 0;
}

#endif

static ngx_int_t
ngx_epoll_init(ngx_cycle_t *cycle, ngx_msec_t timer)
{
    ngx_epoll_conf_t  *epcf;

    epcf = ngx_event_get_conf(cycle->conf_ctx, ngx_epoll_module);

    if (ep == -1) {
       /*
        epoll_create����һ�������֮��epoll��ʹ�ö�����������������ʶ������size�Ǹ���epoll��Ҫ����Ĵ����¼���Ŀ������ʹ��epollʱ��
        �������close�ر���������ע��size����ֻ�Ǹ����ں����epoll����ᴦ����¼�������Ŀ���������ܹ�������¼�������������Linuxީ
        �µ�һЩ�ں˰汾��ʵ���У����size����û���κ����塣

        ����epoll_create���ں��д���epoll���������Ѿ�����������size��������ָ��epoll�ܹ����������¼���������Ϊ�����Linux�ں�
        �汾�У�epoll�ǲ�������������ģ�������Ϊcycle->connectionn/2��������cycle->connection_n��Ҳ��Ҫ��
        */
        ep = epoll_create(cycle->connection_n / 2);

        if (ep == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "epoll_create() failed");
            return NGX_ERROR;
        }

#if (NGX_HAVE_EVENTFD)
        if (ngx_epoll_notify_init(cycle->log) != NGX_OK) {
            ngx_epoll_module_ctx.actions.notify = NULL;
        }
#endif

#if (NGX_HAVE_FILE_AIO)

        ngx_epoll_aio_init(cycle, epcf);

#endif
    }

    if (nevents < epcf->events) {
        if (event_list) {
            ngx_free(event_list);
        }

        event_list = ngx_alloc(sizeof(struct epoll_event) * epcf->events,
                               cycle->log);
        if (event_list == NULL) {
            return NGX_ERROR;
        }
    }

    nevents = epcf->events;//nerentsҲ��������epoll_events�Ĳ���

    ngx_io = ngx_os_io;

    ngx_event_actions = ngx_epoll_module_ctx.actions;

#if (NGX_HAVE_CLEAR_EVENT)
    //Ĭ���ǲ���LTģʽ��ʹ��epoll�ģ�NGX USE CLEAR EVENT��ʵ���Ͼ����ڸ���Nginxʹ��ETģʽ
    ngx_event_flags = NGX_USE_CLEAR_EVENT
#else
    ngx_event_flags = NGX_USE_LEVEL_EVENT
#endif
                      |NGX_USE_GREEDY_EVENT
                      |NGX_USE_EPOLL_EVENT;

    return NGX_OK;
}


#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
static ngx_int_t
ngx_epoll_notify_init(ngx_log_t *log)
{
    struct epoll_event  ee;

#if (NGX_HAVE_SYS_EVENTFD_H)
    notify_fd = eventfd(0, 0);
#else
    notify_fd = syscall(SYS_eventfd, 0);
#endif

    if (notify_fd == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "eventfd() failed");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, log, 0,
                   "notify eventfd: %d", notify_fd);

    notify_event.handler = ngx_epoll_notify_handler;
    notify_event.log = log;
    notify_event.active = 1;

    notify_conn.fd = notify_fd;
    notify_conn.read = &notify_event;
    notify_conn.log = log;

    ee.events = EPOLLIN|EPOLLET;
    ee.data.ptr = &notify_conn;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, notify_fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                      "epoll_ctl(EPOLL_CTL_ADD, eventfd) failed");

        if (close(notify_fd) == -1) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                            "eventfd close() failed");
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}

//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�

static void //ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�
ngx_epoll_notify_handler(ngx_event_t *ev)
{
    ssize_t               n;
    uint64_t              count;
    ngx_err_t             err;
    ngx_event_handler_pt  handler;

    if (++ev->index == NGX_MAX_UINT32_VALUE) {
        ev->index = 0;

        n = read(notify_fd, &count, sizeof(uint64_t));

        err = ngx_errno;

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "read() eventfd %d: %z count:%uL", notify_fd, n, count);

        if ((size_t) n != sizeof(uint64_t)) {
            ngx_log_error(NGX_LOG_ALERT, ev->log, err,
                          "read() eventfd %d failed", notify_fd);
        }
    }

    handler = ev->data;
    handler(ev);
}

#endif

/*
ngx_event_actions_t done�ӿ�����ngx_epoll_done����ʵ�ֵģ���Nginx�˳�����ʱ����õ����á�ngx_epoll_done��Ҫ�ǹر�epoll������ep��
ͬʱ�ͷ�event_1ist���顣
*/
static void
ngx_epoll_done(ngx_cycle_t *cycle)
{
    if (close(ep) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "epoll close() failed");
    }

    ep = -1;

#if (NGX_HAVE_EVENTFD)

    if (close(notify_fd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "eventfd close() failed");
    }

    notify_fd = -1;

#endif

#if (NGX_HAVE_FILE_AIO)

    if (ngx_eventfd != -1) {

        if (io_destroy(ngx_aio_ctx) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "io_destroy() failed");
        }

        if (close(ngx_eventfd) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "eventfd close() failed");
        }

        ngx_eventfd = -1;
    }

    ngx_aio_ctx = 0;

#endif

    ngx_free(event_list);

    event_list = NULL;
    nevents = 0;
}

/*
epoll_ctlϵͳ����
    epoll_ctl��C���е�ԭ�����¡�
    int epoll_ctl(int  epfd, int  op, int  fd, struct  epoll_event*  event)j
    epoll_ctl��epoll��������ӡ��޸Ļ���ɾ������Ȥ���¼�������0��ʾ�ɹ�������
��һ1����ʱ��Ҫ����errno�������жϴ������͡�epoll_wait�������ص��¼���Ȼ��ͨ��
epoll_ctl��ӹ�epoll�еġ�����epfd��epoll_create���صľ������op�������������
9-2��
�����������������������ש���������������������������
��    ��    op��ȡֵ  ��    ����                  ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_ADD     ��    ����µ��¼���epoll�� ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL MOD     ��    �޸�epoll�е��¼�     ��
�ǩ��������������������贈��������������������������
��I EPOLL_CTL_DEL     ��    ɾ��epoll�е��¼�     ��
�����������������������ߩ���������������������������
    ��3������fd�Ǵ����������׽��֣���4���������ڸ���epoll��ʲô�����¼���
��Ȥ����ʹ����epoll_event�ṹ�壬�����Ľ��ܹ���epollʵ�ֻ����л�Ϊÿһ���¼���
��epitem�ṹ�壬����epitem����һ��epoll_event���͵�event��Ա�����濴һ��epoll_
event�Ķ��塣
struct epoll_event{
        _uint32 t events;
        epoll data_t data;
};
events��ȡֵ����9-3��
��9-3 epoll_event��events��ȡֵ����
�������������������ש�������������������������������������������������������������������������������
��    eventsȡֵ  ��    ����                                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLIN         ��                                                                              ��
��                ��  ��ʾ��Ӧ�������������ݿ��Զ�����TCP���ӵ�Զ�������ر����ӣ�Ҳ�൱�ڿɶ���   ��
��                ��������Ϊ��Ҫ����������FIN����                                               ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLOUT        ��  ��ʾ��Ӧ�������Ͽ���д�����ݷ��ͣ����������η����������������TCP���ӣ����� ��
��                �������ɹ����¼��൱�ڿ�д�¼���                                                ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLRDHUP      ��  ��ʾTCP���ӵ�Զ�˹رջ��ر�����                                           ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLPRI        ��  ��ʾ��Ӧ���������н���������Ҫ��                                            ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLERR        ��  ��ʾ��Ӧ�����ӷ�������                                                      ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLHUP        ��  ��ʾ��Ӧ�����ӱ�����                                                        ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLET         ��  ��ʾ��������ʽ��������Ե����(ET)��ϵͳĬ��Ϊˮƽ����(LT��)                  ��
�ǩ����������������贈������������������������������������������������������������������������������
��EPOLLONESHOT    ��  ��ʾ������¼�ֻ����һ�Σ��´���Ҫ����ʱ�����¼���epoll                     ��
�������������������ߩ�������������������������������������������������������������������������������
��data��Ա��һ��epoll_data���ϣ��䶨�����¡�
typedef union epoll_data
       void        *ptr;
      int           fd;
     uint32_t            u32 ;
     uint64_t            u64 ;
} epoll_data_t;
    �ɼ������data��Ա��������ʹ�÷�ʽ��ء����磬ngx_epoll_moduleģ��ֻʹ����
�����е�ptr��Ա����Ϊָ��ngx_connection_t���ӵ�ָ�롣
*/ 
//ngx_epoll_add_event��ʾ���ĳ�����͵�(������д��ͨ��flagָ���ٷ���ʽ��NGX_CLEAR_EVENTΪET��ʽ��NGX_LEVEL_EVENTΪLT��ʽ)�¼���
//ngx_epoll_add_connection(��дһ�������ȥ, ʹ��EPOLLET���ش�����ʽ)
static ngx_int_t      //ͨ��flagָ���ٷ���ʽ��NGX_CLEAR_EVENTΪET��ʽ��NGX_LEVEL_EVENTΪLT��ʽ
ngx_epoll_add_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags) //�ú�����װΪngx_add_event�ģ�ʹ�õ�ʱ��Ϊngx_add_event
{ //һ�������¼��еı��Ķ�дͨ��ngx_handle_read_event  ngx_handle_write_event����¼�
    int                  op;
    uint32_t             events, prev;
    ngx_event_t         *e;
    ngx_connection_t    *c;
    struct epoll_event   ee;

    c = ev->data; //ÿ���¼���data��Ա����������Ӧ��ngx_connection_t����

    /*
    ��������event����ȷ����ǰ�¼��Ƕ��¼�����д�¼���������eventg�Ǽ���EPOLLIN��־λ����EPOLLOUT��־λ
     */
    events = (uint32_t) event;

    if (event == NGX_READ_EVENT) {
        e = c->write;
        prev = EPOLLOUT;
#if (NGX_READ_EVENT != EPOLLIN|EPOLLRDHUP)
        events = EPOLLIN|EPOLLRDHUP;
#endif

    } else {
        e = c->read;
        prev = EPOLLIN|EPOLLRDHUP;
#if (NGX_WRITE_EVENT != EPOLLOUT)
        events = EPOLLOUT;
#endif
    }

    //��һ�����epoll_ctlΪEPOLL_CTL_ADD,����ٴ���ӷ���activeΪ1,��epoll_ctlΪEPOLL_CTL_MOD
    if (e->active) { //����active��־λȷ���Ƿ�Ϊ��Ծ�¼����Ծ����������޸Ļ�������¼�
        op = EPOLL_CTL_MOD; 
        events |= prev; //�����active�ģ���events= EPOLLIN|EPOLLRDHUP|EPOLLOUT;

    } else {
        op = EPOLL_CTL_ADD;
    }

    ee.events = events | (uint32_t) flags; //����flags������events��־λ��
    /* ptr��Ա�洢����ngx_connection_t���ӣ��ɲμ�epoll��ʹ�÷�ʽ��*/
    ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

    if (e->active) {//modify
        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "epoll modify read and write event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
    } else {//add
        if (event == NGX_READ_EVENT) {
            ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "epoll add read event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
        } else
            ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "epoll add write event: fd:%d op:%d ev:%08XD", c->fd, op, ee.events);
    }
  
    //EPOLL_CTL_ADDһ�κ󣬾Ϳ���һֱͨ��epoll_wait����ȡ���¼������ǵ���EPOLL_CTL_DEL������ÿ�ζ��¼�����epoll_wait���غ�Ҫ�������EPOLL_CTL_ADD��
    //֮ǰ�������еĵط�����ע���ˣ���עΪÿ�ζ��¼�������Ҫ����addһ��
    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {//epoll_wait() ϵͳ���õȴ����ļ������� c->fd ���õ� epoll ʵ���ϵ��¼�
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }
    //�����ngx_add_event->ngx_epoll_add_event�а�listening�е�c->read->active��1�� ngx_epoll_del_event�а�listening����read->active��0
    //��һ�����epoll_ctlΪEPOLL_CTL_ADD,����ٴ���ӷ���activeΪ1,��epoll_ctlΪEPOLL_CTL_MOD
    ev->active = 1; //���¼���active��־λ��Ϊ1����ʾ��ǰ�¼��ǻ�Ծ��   ngx_epoll_del_event����0
#if 0
    ev->oneshot = (flags & NGX_ONESHOT_EVENT) ? 1 : 0;
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_epoll_del_event(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags)
{
    int                  op;
    uint32_t             prev;
    ngx_event_t         *e;
    ngx_connection_t    *c;
    struct epoll_event   ee;

    /*
     * when the file descriptor is closed, the epoll automatically deletes
     * it from its queue, so we do not need to delete explicitly the event
     * before the closing the file descriptor
     */

    if (flags & NGX_CLOSE_EVENT) {
        ev->active = 0;
        return NGX_OK;
    }

    c = ev->data;

    if (event == NGX_READ_EVENT) {
        e = c->write;
        prev = EPOLLOUT;

    } else {
        e = c->read;
        prev = EPOLLIN|EPOLLRDHUP;
    }

    if (e->active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev | (uint32_t) flags;
        ee.data.ptr = (void *) ((uintptr_t) c | ev->instance);

    } else {
        op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "epoll del event: fd:%d op:%d ev:%08XD",
                   c->fd, op, ee.events);

    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }
    //�����ngx_add_event->ngx_epoll_add_event�а�listening�е�c->read->active��1�� ngx_epoll_del_event�а�listening����read->active��0
    ev->active = 0;

    return NGX_OK;
}

//ngx_epoll_add_event��ʾ���ĳ�����͵�(������д��ʹ��LT�ٷ���ʽ)�¼���ngx_epoll_add_connection (��дһ�������ȥ, ʹ��EPOLLET���ش�����ʽ)
static ngx_int_t
ngx_epoll_add_connection(ngx_connection_t *c) //�ú�����װΪngx_add_conn�ģ�ʹ�õ�ʱ��Ϊngx_add_conn
{
    struct epoll_event  ee;

    ee.events = EPOLLIN|EPOLLOUT|EPOLLET|EPOLLRDHUP; //ע��������ˮƽ���� 
    ee.data.ptr = (void *) ((uintptr_t) c | c->read->instance);

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "epoll add connection(read and write): fd:%d ev:%08XD", c->fd, ee.events);

    if (epoll_ctl(ep, EPOLL_CTL_ADD, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      "epoll_ctl(EPOLL_CTL_ADD, %d) failed", c->fd);
        return NGX_ERROR;
    }

    c->read->active = 1;
    c->write->active = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_epoll_del_connection(ngx_connection_t *c, ngx_uint_t flags)
{
    int                 op;
    struct epoll_event  ee;

    /*
     * when the file descriptor is closed the epoll automatically deletes
     * it from its queue so we do not need to delete explicitly the event
     * before the closing the file descriptor
     */

    if (flags & NGX_CLOSE_EVENT) { //����ú��������Ż����close(fd)����ô�Ͳ���epoll del�ˣ�ϵͳ���Զ�del
        c->read->active = 0;
        c->write->active = 0;
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "epoll del connection: fd:%d", c->fd);

    op = EPOLL_CTL_DEL;
    ee.events = 0;
    ee.data.ptr = NULL;

    if (epoll_ctl(ep, op, c->fd, &ee) == -1) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      "epoll_ctl(%d, %d) failed", op, c->fd);
        return NGX_ERROR;
    }

    c->read->active = 0;
    c->write->active = 0;

    return NGX_OK;
}


#if (NGX_HAVE_EVENTFD)
//ִ��ngx_epoll_notify���ͨ��epoll_wait����ִ�иú���ngx_epoll_notify_handler
//ngx_epoll_notify_handler  ngx_epoll_notify_init  ngx_epoll_notify(ngx_notify)����Ķ�

static ngx_int_t
ngx_epoll_notify(ngx_event_handler_pt handler)
{
    static uint64_t inc = 1;

    notify_event.data = handler;

    //ngx_notify->ngx_epoll_notifyֻ�ᴥ��epool_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
    if ((size_t) write(notify_fd, &inc, sizeof(uint64_t)) != sizeof(uint64_t)) {
        ngx_log_error(NGX_LOG_ALERT, notify_event.log, ngx_errno,
                      "write() to eventfd %d failed", notify_fd);
        return NGX_ERROR;
    }

    return NGX_OK;
}

#endif

void ngx_epoll_event_2str(uint32_t event, char* buf)
{
    if(event & EPOLLIN)
        strcpy(buf, "EPOLLIN ");

    if(event & EPOLLPRI) 
        strcat(buf, "EPOLLPRI ");
  
    if(event & EPOLLOUT)
        strcat(buf, "EPOLLOUT ");

    if(event & EPOLLRDNORM)
        strcat(buf, "EPOLLRDNORM ");

    if(event & EPOLLRDBAND)
        strcat(buf, "EPOLLRDBAND ");

    if(event & EPOLLWRNORM)
        strcat(buf, "EPOLLWRNORM ");

    if(event & EPOLLWRBAND)
        strcat(buf, "EPOLLWRBAND ");

    if(event & EPOLLMSG) 
        strcat(buf, "EPOLLMSG ");

    if(event & EPOLLERR)
        strcat(buf, "EPOLLERR ");
        
    if(event & EPOLLHUP)
        strcat(buf, "EPOLLHUP ");
        
    strcat(buf, " ");
}

//ngx_epoll_process_eventsע�ᵽngx_process_events��  
//��ngx_epoll_add_event���ʹ��
//�ú�����ngx_process_events_and_timers�е���
static ngx_int_t
ngx_epoll_process_events(ngx_cycle_t *cycle, ngx_msec_t timer, ngx_uint_t flags)//flags�����к���NGX_POST_EVENTS��ʾ�����¼�Ҫ�Ӻ���
{
    int                events;
    uint32_t           revents;
    ngx_int_t          instance, i;
    ngx_uint_t         level;
    ngx_err_t          err;
    ngx_event_t       *rev, *wev;
    ngx_queue_t       *queue;
    ngx_connection_t  *c;
    char epollbuf[256];
    
    /* NGX_TIMER_INFINITE == INFTIM */

    //ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "begin to epoll_wait, epoll timer: %M ", timer);

    /*
     ����epoll_wait��ȡ�¼���ע�⣬timer��������process_events����ʱ����ģ���9.7��9.8���л��ᵽ�������
     */
    //The call was interrupted by a signal handler before any of the requested events occurred or the timeout expired;
    //������źŷ���(������ngx_timer_signal_handler)���綨ʱ������᷵��-1
    //��Ҫ��ngx_add_event��ngx_add_conn���ʹ��        
    //event_list�洢���Ǿ����õ��¼��������select���Ǵ����û�ע����¼�����Ҫ������飬����ÿ��select���غ���Ҫ���������¼�����epoll����
    /*
    ������ȴ����¼������ͻ��������¼�(����ǴӸ����̼̳й�����ep��Ȼ�����ӽ���whileǰ��ngx_event_process_init->ngx_add_event���)��
    ���Ѿ��������ӵ�fd��д�¼��������ngx_event_accept->ngx_http_init_connection->ngx_handle_read_event
    */

/*
ngx_notify->ngx_epoll_notifyֻ�ᴥ��epoll_in������ͬʱ����epoll_out�������������¼�epoll_in,���ͬʱ����epoll_out
*/
    events = epoll_wait(ep, event_list, (int) nevents, timer);  //timerΪ-1��ʾ���޵ȴ�   nevents��ʾ���������ٸ��¼����������0
    //EPOLL_WAIT���û�ж�д�¼����߶�ʱ����ʱ�¼�������������˯�ߣ�������̻��ó�CPU

    err = (events == -1) ? ngx_errno : 0;

    //��flags��־λָʾҪ����ʱ��ʱ��������������µ�
    //Ҫ��ngx_timer_resolution���볬ʱ�����ʱ�䣬Ҫ��epoll��д�¼���ʱ�����ʱ��
    if (flags & NGX_UPDATE_TIME || ngx_event_timer_alarm) {
        ngx_time_update();
    }

    if (err) {
        if (err == NGX_EINTR) {

            if (ngx_event_timer_alarm) { //��ʱ����ʱ�����epoll_wait����
                ngx_event_timer_alarm = 0;
                return NGX_OK;
            }

            level = NGX_LOG_INFO;

        } else {
            level = NGX_LOG_ALERT;
        }

        ngx_log_error(level, cycle->log, err, "epoll_wait() failed");
        return NGX_ERROR;
    }

    if (events == 0) {
        if (timer != NGX_TIMER_INFINITE) {
            return NGX_OK;
        }

        ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                      "epoll_wait() returned no events without timeout");
        return NGX_ERROR;
    }

    //��������epoll_wait���ص������¼�
    for (i = 0; i < events; i++) { //��ngx_epoll_add_event���ʹ��
        /*
        �����������ᵽ��ngx_epoll_add_event���������Կ���ptr��Ա����ngx_connection_t���ӵĵ�ַ�������1λ�����⺬�壬��Ҫ�������ε�
          */
        c = event_list[i].data.ptr; //ͨ�����ȷ�����Ǹ�����

        instance = (uintptr_t) c & 1; //����ַ�����һλȡ��������instance������ʶ, ��ngx_epoll_add_event

        /*
          ������32λ����64λ���������ַ�����1λ�϶���0��������������������ngx_connection_t�ĵ�ַ��ԭ�������ĵ�ֵַ
          */ //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����
        c = (ngx_connection_t *) ((uintptr_t) c & (uintptr_t) ~1);

        rev = c->read; //ȡ�����¼� //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����

        if (c->fd == -1 || rev->instance != instance) { //�ж�������¼��Ƿ�Ϊ�����¼�
          //��fd�׽���������Ϊ-l����instance��־λ�����ʱ����ʾ����¼��Ѿ������ˣ����ô���
            /*
             * the stale event from a file descriptor
             * that was just closed in this iteration
             */

            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll: stale event %p", c);
            continue;
        }

        revents = event_list[i].events; //ȡ���¼�����
        ngx_epoll_event_2str(revents, epollbuf);

        memset(epollbuf, 0, sizeof(epollbuf));
        ngx_epoll_event_2str(revents, epollbuf);
        ngx_log_debug4(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "epoll: fd:%d %s(ev:%04XD) d:%p",
                       c->fd, epollbuf, revents, event_list[i].data.ptr);

        if (revents & (EPOLLERR|EPOLLHUP)) { //����Է�close���׽��֣�������Ӧ��
            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "epoll_wait() error on fd:%d ev:%04XD",
                           c->fd, revents);
        }

#if 0
        if (revents & ~(EPOLLIN|EPOLLOUT|EPOLLERR|EPOLLHUP)) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                          "strange epoll_wait() events fd:%d ev:%04XD",
                          c->fd, revents);
        }
#endif

        if ((revents & (EPOLLERR|EPOLLHUP)) 
             && (revents & (EPOLLIN|EPOLLOUT)) == 0)
        {
            /*
             * if the error events were returned without EPOLLIN or EPOLLOUT,
             * then add these flags to handle the events at least in one
             * active handler
             */

            revents |= EPOLLIN|EPOLLOUT; //epoll EPOLLERR|EPOLLHUPʵ������ͨ��������д�¼����ж�д����recv write����������쳣
        }

        if ((revents & EPOLLIN) && rev->active) { //����Ƕ��¼��Ҹ��¼��ǻ�Ծ��

#if (NGX_HAVE_EPOLLRDHUP)
            if (revents & EPOLLRDHUP) {
                rev->pending_eof = 1;
            }
#endif
            //ע�������c�п�����acceptǰ��c�����ڼ���Ƿ�ͻ��˷���tcp�����¼�,accept���سɹ�������´���һ��ngx_connection_t��������д�ͻ��˵�����
            rev->ready = 1; //��ʾ�Ѿ������ݵ�������ֻ�ǰ�accept�ɹ�ǰ�� ngx_connection_t->read->ready��1��accept���غ�����´����ӳ��л�ȡһ��ngx_connection_t
            //flags�����к���NGX_POST_EVENTS��ʾ�����¼�Ҫ�Ӻ���
            if (flags & NGX_POST_EVENTS) {
                /*
                ���Ҫ��post�������Ӻ�����¼�������Ҫ�ж������������¼�������ͨ�¼����Ծ�����������
                ��ngx_posted_accept_events���л���ngx_postedL events�����С�����post�����е��¼���ʱִ��
                */
                queue = rev->accept ? &ngx_posted_accept_events
                                    : &ngx_posted_events;

                ngx_post_event(rev, queue); 

            } else {
                //������յ��ͻ������ݣ�����Ϊngx_http_wait_request_handler  
                rev->handler(rev); //���Ϊ��ûaccept����Ϊngx_event_process_init������Ϊngx_event_accept������Ѿ��������ӣ��������Ϊngx_http_process_request_line
            }
        }

        wev = c->write;

        if ((revents & EPOLLOUT) && wev->active) {

            if (c->fd == -1 || wev->instance != instance) { //�ж�������¼��Ƿ�Ϊ�����¼�
                //��fd�׽���������Ϊ-1����instance��־λ�����ʱ����ʾ����¼��Ѿ����ڣ����ô���
                /*
                 *  the stale event from a file descriptor
                 * that was just closed in this iteration
                 */

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                               "epoll: stale event %p", c);
                continue;
            }

            wev->ready = 1;

            if (flags & NGX_POST_EVENTS) { 
                ngx_post_event(wev, &ngx_posted_events); //������¼���ӵ�post�������Ӻ���

            } else { //�����������д�¼��Ļص���������������¼�
                wev->handler(wev);
            }
        }
    }

    return NGX_OK;
}


#if (NGX_HAVE_FILE_AIO)
/*
�첽�ļ�i/o�����¼��Ļص�����Ϊngx_file_aio_event_handler�����ĵ��ù�ϵ����������epoll_wait(ngx_process_events_and_timers)�е���
ngx_epoll_eventfd_handler��������ǰ�¼����뵽ngx_posted_events�����У����Ӻ�ִ�еĶ����е���ngx_file_aio_event_handler����
*/
/*
 ���������¼����������ƾ�������ͨ��ngx_eventfd֪ͨ��������ngx_epoll_eventfd
handler�ص������������ļ��첽I/O�¼���������ġ�
    ��ô���������첽I/O���������ύ�첽I/O�����أ�����ngx_linux_aio read.c�ļ���
��ngx_file_aio_read�������ڴ��ļ��첽I/O������������Ḻ������ļ��Ķ�ȡ
*/
/*
ngx_epoll_aio_init��ʼ��aio�¼��б� ngx_file_aio_read��Ӷ��ļ��¼�������ȡ��Ϻ�epoll�ᴥ��
ngx_epoll_eventfd_handler->ngx_file_aio_event_handler 
nginx file aioֻ�ṩ��read�ӿڣ����ṩwrite�ӿڣ���Ϊ�첽aioֻ�Ӵ��̶���д������aio��ʽһ��д���䵽
���̻��棬���Բ��ṩ�ýӿڣ�����첽ioд���ܻ����
*/

//�ú�����ngx_process_events_and_timers��ִ��
static void
ngx_epoll_eventfd_handler(ngx_event_t *ev) //��epoll_wait�м�⵽aio���ɹ��¼������ߵ�����
{
    int               n, events;
    long              i;
    uint64_t          ready;
    ngx_err_t         err;
    ngx_event_t      *e;
    ngx_event_aio_t  *aio;
    struct io_event   event[64]; //һ������ദ��64���¼�
    struct timespec   ts;

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0, "eventfd handler");
    //��ȡ�Ѿ���ɵ��¼���Ŀ�������õ�ready�У�ע�⣬���ready�ǿ��Դ���64��
    n = read(ngx_eventfd, &ready, 8);//����read������ȡ����ɵ�I/O�ĸ���   

    err = ngx_errno;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, ev->log, 0, "aio epoll handler eventfd: %d", n);

    if (n != 8) {
        if (n == -1) {
            if (err == NGX_EAGAIN) {
                return;
            }

            ngx_log_error(NGX_LOG_ALERT, ev->log, err, "read(eventfd) failed");
            return;
        }

        ngx_log_error(NGX_LOG_ALERT, ev->log, 0,
                      "read(eventfd) returned only %d bytes", n);
        return;
    }

    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    //ready��ʾ��δ������¼�����ready����0ʱ��������
    while (ready) {
        //����io_getevents��ȡ�Ѿ���ɵ��첽I/O�¼�
        events = io_getevents(ngx_aio_ctx, 1, 64, event, &ts);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "io_getevents: %l", events);

        if (events > 0) {
            ready -= events; //��ready��ȥ�Ѿ�ȡ�����¼�

            for (i = 0; i < events; i++) { //����event��������¼�

                ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                               "io_event: %uXL %uXL %L %L",
                                event[i].data, event[i].obj,
                                event[i].res, event[i].res2);
                //data��Աָ������첽I/O�¼���Ӧ�ŵ�ʵ���¼�,�����ngx_event_tΪngx_event_aio_t->event
               /*��ngx_epoll_eventfd_handler��Ӧ,��ִ��ngx_file_aio_read���첽I/O����Ӷ��¼���Ȼ��ͨ��epoll�������ض�ȡ
               ���ݳɹ�����ִ��ngx_epoll_eventfd_handler*/
               //�����e����ngx_file_aio_read����ӽ����ģ��ں����ngx_post_event�л�ִ��ngx_file_aio_event_handler
                e = (ngx_event_t *) (uintptr_t) event[i].data; //��ngx_file_aio_read��io_submit��Ӧ

                e->complete = 1;
                e->active = 0;
                e->ready = 1;

                aio = e->data;
                aio->res = event[i].res;

                ngx_post_event(e, &ngx_posted_events); 
                //�����¼��ŵ�ngx_posted_events�������Ӻ�ִ��,ִ�иö��е�handle�ط���ngx_process_events_and_timers
            }

            continue;
        }

        if (events == 0) {
            return;
        }

        /* events == -1 */
        ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                      "io_getevents() failed");
        return;
    }
}

#endif


static void *
ngx_epoll_create_conf(ngx_cycle_t *cycle)
{
    ngx_epoll_conf_t  *epcf;

    epcf = ngx_palloc(cycle->pool, sizeof(ngx_epoll_conf_t));
    if (epcf == NULL) {
        return NULL;
    }

    epcf->events = NGX_CONF_UNSET;
    epcf->aio_requests = NGX_CONF_UNSET;

    return epcf;
}


static char *
ngx_epoll_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_epoll_conf_t *epcf = conf;

    ngx_conf_init_uint_value(epcf->events, 512);
    ngx_conf_init_uint_value(epcf->aio_requests, 32);

    return NGX_CONF_OK;
}
