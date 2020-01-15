
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_ERRNO_H_INCLUDED_
#define _NGX_ERRNO_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef int               ngx_err_t;

#define NGX_EPERM         EPERM
#define NGX_ENOENT        ENOENT //����file_nameָ�����ļ�������
#define NGX_ENOPATH       ENOENT
#define NGX_ESRCH         ESRCH
#define NGX_EINTR         EINTR
#define NGX_ECHILD        ECHILD
#define NGX_ENOMEM        ENOMEM
#define NGX_EACCES        EACCES
#define NGX_EBUSY         EBUSY
#define NGX_EEXIST        EEXIST
#define NGX_EXDEV         EXDEV
#define NGX_ENOTDIR       ENOTDIR
#define NGX_EISDIR        EISDIR
#define NGX_EINVAL        EINVAL
#define NGX_ENFILE        ENFILE
#define NGX_EMFILE        EMFILE
#define NGX_ENOSPC        ENOSPC
#define NGX_EPIPE         EPIPE
#define NGX_EINPROGRESS   EINPROGRESS
#define NGX_ENOPROTOOPT   ENOPROTOOPT
#define NGX_EOPNOTSUPP    EOPNOTSUPP
#define NGX_EADDRINUSE    EADDRINUSE
#define NGX_ECONNABORTED  ECONNABORTED
#define NGX_ECONNRESET    ECONNRESET
#define NGX_ENOTCONN      ENOTCONN
#define NGX_ETIMEDOUT     ETIMEDOUT
#define NGX_ECONNREFUSED  ECONNREFUSED
#define NGX_ENAMETOOLONG  ENAMETOOLONG
#define NGX_ENETDOWN      ENETDOWN
#define NGX_ENETUNREACH   ENETUNREACH
#define NGX_EHOSTDOWN     EHOSTDOWN
#define NGX_EHOSTUNREACH  EHOSTUNREACH
#define NGX_ENOSYS        ENOSYS
#define NGX_ECANCELED     ECANCELED
#define NGX_EILSEQ        EILSEQ
#define NGX_ENOMOREFILES  0
#define NGX_ELOOP         ELOOP
#define NGX_EBADF         EBADF

#if (NGX_HAVE_OPENAT)
#define NGX_EMLINK        EMLINK
#endif

/*
EWOULDBLOCK���ڷ�����ģʽ������Ҫ���¶�����д
���⣬�������EINTR��errnoΪ4����������Interrupted system call������ҲӦ�ü�����
EINTRָ�������жϻ��ѣ���Ҫ���¶�/д

��Linux�����¿��������������ܶ����(����errno)������EAGAIN�����бȽϳ�����һ������(�������ڷ�����������)��
������������������ʾ����һ�Ρ�������󾭳������ڵ�Ӧ�ó������һЩ������(non-blocking)����(���ļ���socket)��ʱ��
���磬�� O_NONBLOCK�ı�־���ļ�/socket/FIFO�������������read������û�����ݿɶ�����ʱ���򲻻����������ȴ�����׼���������أ�
read�����᷵��һ������EAGAIN����ʾ���Ӧ�ó�������û�����ݿɶ����Ժ����ԡ�

��Linux��ʹ�÷�������socket�������¡� 
��һ������ʱ

�������ͻ�ͨ��Socket�ṩ��send�������ʹ�����ݰ�ʱ���Ϳ��ܷ���һ��EAGAIN�Ĵ��󡣸ô��������ԭ��������send �����е�size������С������
tcp_sendspace��ֵ��tcp_sendspace������Ӧ���ڵ���send֮ǰ�ܹ���kernel�л��������������Ӧ�ó�����socket��������O_NDELAY����O_NONBLOCK���Ժ�
������ͻ��汻ռ����send�ͻ᷵��EAGAIN�Ĵ��� 

����Ϊ�������ô��������ַ�������ѡ�� 
����1.����tcp_sendspace��ʹ֮����send�е�size���� 
����---no -p -o tcp_sendspace=65536 

����2.�ڵ���sendǰ����setsockopt������ΪSNDBUF���ø����ֵ 

����3.ʹ��write���send����Ϊwriteû������O_NDELAY����O_NONBLOCK

����������ʱ

       ��������ʱ������Resource temporarily unavailable����ʾ��errno����Ϊ11(EAGAIN)����������ڷ�����ģʽ�µ���������������
       �ڸò���û����ɾͷ����������������󲻻��ƻ�socket��ͬ�������ù������´�ѭ������recv�Ϳ��ԡ��Է�����socket���ԣ�
       EAGAIN����һ�ִ�����VxWorks��Windows�ϣ�EAGAIN�����ֽ���EWOULDBLOCK����ʵ���㲻�ϴ���ֻ��һ���쳣���ѡ�

�������⣬�������EINTR��errnoΪ4����������Interrupted system call������ҲӦ�ü�����
����������recv�ķ���ֵΪ0���Ǳ����Է��ѽ����ӶϿ������ǵĽ��ղ���ҲӦ�ý�����

��������������һ�ֽ���

���緢�Ͷ��������ڽ��ն˵�����(��˼��epoll���ڵĳ������ת����socketҪ��),�����Ƿ�������socket,��ôsend()������Ȼ����,��ʵ�ʻ���
�������ݲ�δ�����������ն�,�������ϵĶ��ͷ�������������������EAGAIN����(�ο�man send),ͬʱ,�������������͵�����.
*/

#if (__hpux__)
#define NGX_EAGAIN        EWOULDBLOCK
#else
#define NGX_EAGAIN        EAGAIN //11
#endif


#define ngx_errno                  errno
#define ngx_socket_errno           errno
#define ngx_set_errno(err)         errno = err
#define ngx_set_socket_errno(err)  errno = err


u_char *ngx_strerror(ngx_err_t err, u_char *errstr, size_t size);
ngx_int_t ngx_strerror_init(void);


#endif /* _NGX_ERRNO_H_INCLUDED_ */
