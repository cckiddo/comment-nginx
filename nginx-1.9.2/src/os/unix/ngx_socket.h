
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SOCKET_H_INCLUDED_
#define _NGX_SOCKET_H_INCLUDED_


#include <ngx_config.h>


#define NGX_WRITE_SHUTDOWN SHUT_WR

typedef int  ngx_socket_t;

#define ngx_socket          socket
#define ngx_socket_n        "socket()"


#if (NGX_HAVE_FIONBIO)

int ngx_nonblocking(ngx_socket_t s);
int ngx_blocking(ngx_socket_t s);

#define ngx_nonblocking_n   "ioctl(FIONBIO)"
#define ngx_blocking_n      "ioctl(!FIONBIO)"

#else

#define ngx_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define ngx_nonblocking_n   "fcntl(O_NONBLOCK)"

#define ngx_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define ngx_blocking_n      "fcntl(!O_NONBLOCK)"

#endif

int ngx_tcp_nopush(ngx_socket_t s);
int ngx_tcp_push(ngx_socket_t s);

#if (NGX_LINUX)

#define ngx_tcp_nopush_n   "setsockopt(TCP_CORK)"
#define ngx_tcp_push_n     "setsockopt(!TCP_CORK)"

#else

#define ngx_tcp_nopush_n   "setsockopt(TCP_NOPUSH)"
#define ngx_tcp_push_n     "setsockopt(!TCP_NOPUSH)"

#endif

/*
1.close()����

[cpp] view plaincopyprint?
01.<SPAN style="FONT-SIZE: 13px">#include<unistd.h>  
02.int close(int sockfd);     //���سɹ�Ϊ0������Ϊ-1.</SPAN>  
#include<unistd.h>
int close(int sockfd);     //���سɹ�Ϊ0������Ϊ-1.    close һ���׽��ֵ�Ĭ����Ϊ�ǰ��׽��ֱ��Ϊ�ѹرգ�Ȼ���������ص����ý��̣�
���׽����������������ɵ��ý���ʹ�ã�Ҳ����˵����������Ϊread��write�ĵ�һ��������Ȼ��TCP�����Է������Ŷӵȴ����͵��Զ˵��κ����ݣ�
������Ϻ�������������TCP������ֹ���С�

    �ڶ���̲����������У����ӽ��̹������׽��֣��׽������������ü�����¼�Ź����ŵĽ��̸������������̻�ĳһ�ӽ���close���׽���ʱ��
���������ü�������Ӧ�ļ�һ�������ü����Դ�����ʱ�����close���þͲ�������TCP����·���ֶ������̡�

2.shutdown()����

[cpp] view plaincopyprint?
01.<SPAN style="FONT-SIZE: 13px">#include<sys/socket.h>  
02.int shutdown(int sockfd,int howto);  //���سɹ�Ϊ0������Ϊ-1.</SPAN>  
#include<sys/socket.h>
int shutdown(int sockfd,int howto);  //���سɹ�Ϊ0������Ϊ-1.    �ú�������Ϊ������howto��ֵ

    1.SHUT_RD��ֵΪ0���ر����ӵĶ���һ�롣

    2.SHUT_WR��ֵΪ1���ر����ӵ�д��һ�롣

    3.SHUT_RDWR��ֵΪ2�����ӵĶ���д���رա�

    ��ֹ�������ӵ�ͨ�÷����ǵ���close��������ʹ��shutdown�ܸ��õĿ��ƶ������̣�ʹ�õڶ�����������

3.������������
    close��shutdown��������Ҫ�����ڣ�
    close������ر��׽���ID������������Ľ��̹���������׽��֣���ô����Ȼ�Ǵ򿪵ģ����������Ȼ������������д��������ʱ�����Ƿǳ���Ҫ�� ���ر��Ƕ��ڶ���̲�����������˵��

    ��shutdown���жϽ��̹�����׽��ֵ��������ӣ���������׽��ֵ����ü����Ƿ�Ϊ�㣬��Щ��ͼ���ý��̽�����յ�EOF��ʶ����Щ��ͼд�Ľ��̽����⵽SIGPIPE�źţ�ͬʱ������shutdown�ĵڶ�������ѡ������ķ�ʽ��
*/
#define ngx_shutdown_socket    shutdown
#define ngx_shutdown_socket_n  "shutdown()"

#define ngx_close_socket    close
#define ngx_close_socket_n  "close() socket"


#endif /* _NGX_SOCKET_H_INCLUDED_ */
