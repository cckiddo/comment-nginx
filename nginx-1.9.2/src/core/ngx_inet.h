
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_INET_H_INCLUDED_
#define _NGX_INET_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * TODO: autoconfigure NGX_SOCKADDRLEN and NGX_SOCKADDR_STRLEN as
 *       sizeof(struct sockaddr_storage)
 *       sizeof(struct sockaddr_un)
 *       sizeof(struct sockaddr_in6)
 *       sizeof(struct sockaddr_in)
 */

#define NGX_INET_ADDRSTRLEN   (sizeof("255.255.255.255") - 1)
#define NGX_INET6_ADDRSTRLEN                                                 \
    (sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255") - 1)
#define NGX_UNIX_ADDRSTRLEN                                                  \
    (sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path))

#if (NGX_HAVE_UNIX_DOMAIN)
#define NGX_SOCKADDR_STRLEN   (sizeof("unix:") - 1 + NGX_UNIX_ADDRSTRLEN)
#else
#define NGX_SOCKADDR_STRLEN   (NGX_INET6_ADDRSTRLEN + sizeof("[]:65535") - 1)
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
#define NGX_SOCKADDRLEN       sizeof(struct sockaddr_un)
#else
#define NGX_SOCKADDRLEN       512
#endif


typedef struct {
    in_addr_t                 addr;
    in_addr_t                 mask;
} ngx_in_cidr_t;


#if (NGX_HAVE_INET6)

typedef struct {
    struct in6_addr           addr;
    struct in6_addr           mask;
} ngx_in6_cidr_t;

#endif


typedef struct {
    ngx_uint_t                family;
    union {
        ngx_in_cidr_t         in;
#if (NGX_HAVE_INET6)
        ngx_in6_cidr_t        in6;
#endif
    } u;
} ngx_cidr_t;


typedef struct {
    struct sockaddr          *sockaddr;
    socklen_t                 socklen;
    ngx_str_t                 name;
} ngx_addr_t;


typedef struct { //ͨ���������ngx_parse_inet_url
    ngx_str_t                 url;//����IP��ַ+�˿���Ϣ��e.g. 192.168.124.129:8011 �� money.163.com��
    ngx_str_t                 host;//����IP��ַ��Ϣ //proxy_pass  http://10.10.0.103:8080/tttxx; �е�10.10.0.103
    ngx_str_t                 port_text;//����port�ַ���
    ngx_str_t                 uri;//uri���֣��ں���ngx_parse_inet_url()������  http://10.10.0.103:8080/tttxx;�е�/tttxx

    in_port_t                 port;//�˿ڣ�e.g. listenָ����ָ���Ķ˿ڣ�listen 192.168.124.129:8011��
    //Ĭ�϶˿ڣ���no_port�ֶ�Ϊ��ʱ����Ĭ�϶˿ڸ�ֵ��port�ֶΣ� Ĭ�϶˿�ͨ����80��
    in_port_t                 default_port; //ngx_http_core_listen������Ϊ80 //Ĭ�϶˿ڣ���no_port�ֶ�Ϊ��ʱ����Ĭ�϶˿ڸ�ֵ��port�ֶΣ� Ĭ�϶˿�ͨ����80��
    int                       family;//address family, AF_xxx  //AF_UNIX�������׽���  AF_INET������ͨ�����׽���

    unsigned                  listen:1; //ngx_http_core_listen����1 //�Ƿ�Ϊָ�����������
    unsigned                  uri_part:1;
    unsigned                  no_resolve:1; //������������Ƿ����������������������IP��ַ��
    unsigned                  one_addr:1;  /* compatibility */ ///����1ʱ������һ��IP��ַ

    unsigned                  no_port:1;//��ʶurl��û����ʾָ���˿�(Ϊ1ʱû��ָ��)  uri���Ƿ���ָ���˿�
    unsigned                  wildcard:1; //��listen  *:80���λ��1 //��ʶ�Ƿ�ʹ��ͨ�����e.g. listen *:8000;��

    socklen_t                 socklen;//sizeof(struct sockaddr_in)
    u_char                    sockaddr[NGX_SOCKADDRLEN];//sockaddr_in�ṹָ����

    ngx_addr_t               *addrs;//�����С��naddrs�ֶΣ�ÿ��Ԫ�ض�Ӧ������IP��ַ��Ϣ(struct sockaddr_in)���ں����и�ֵ��ngx_inet_resolve_host()��
    ngx_uint_t                naddrs;//url��Ӧ��IP��ַ����,IP��ʽ�ĵ�ַ��Ĭ��Ϊ1

    char                     *err;//������Ϣ�ַ���
} ngx_url_t;


in_addr_t ngx_inet_addr(u_char *text, size_t len);
#if (NGX_HAVE_INET6)
ngx_int_t ngx_inet6_addr(u_char *p, size_t len, u_char *addr);
size_t ngx_inet6_ntop(u_char *p, u_char *text, size_t len);
#endif
size_t ngx_sock_ntop(struct sockaddr *sa, socklen_t socklen, u_char *text,
    size_t len, ngx_uint_t port);
size_t ngx_inet_ntop(int family, void *addr, u_char *text, size_t len);
ngx_int_t ngx_ptocidr(ngx_str_t *text, ngx_cidr_t *cidr);
ngx_int_t ngx_parse_addr(ngx_pool_t *pool, ngx_addr_t *addr, u_char *text,
    size_t len);
ngx_int_t ngx_parse_url(ngx_pool_t *pool, ngx_url_t *u);
ngx_int_t ngx_inet_resolve_host(ngx_pool_t *pool, ngx_url_t *u);
ngx_int_t ngx_cmp_sockaddr(struct sockaddr *sa1, socklen_t slen1,
    struct sockaddr *sa2, socklen_t slen2, ngx_uint_t cmp_port);


#endif /* _NGX_INET_H_INCLUDED_ */
