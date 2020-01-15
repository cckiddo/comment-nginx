
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_CONNECT_H_INCLUDED_
#define _NGX_EVENT_CONNECT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#define NGX_PEER_KEEPALIVE           1
#define NGX_PEER_NEXT                2
#define NGX_PEER_FAILED              4 //ʹ�ø�ֵ�ط���ngx_http_upstream_next


typedef struct ngx_peer_connection_s  ngx_peer_connection_t;

//��ʹ�ó����������η�����ͨ��ʱ����ͨ���÷��������ӳ��л�ȡһ��������
typedef ngx_int_t (*ngx_event_get_peer_pt)(ngx_peer_connection_t *pc,
    void *data);
//��ʹ�ó����������η�����ͨ��ʱ��ͨ���÷�����ʹ����ϵ������ͷŸ����ӳ�
typedef void (*ngx_event_free_peer_pt)(ngx_peer_connection_t *pc, void *data,
    ngx_uint_t state);
#if (NGX_SSL)

typedef ngx_int_t (*ngx_event_set_peer_session_pt)(ngx_peer_connection_t *pc,
    void *data);
typedef void (*ngx_event_save_peer_session_pt)(ngx_peer_connection_t *pc,
    void *data);
#endif

/*
Nginx�ж����˻��������ݽṹngx_connection_t����ʾ���ӣ�������ӱ�ʾ�ǿͻ�����������ġ�Nginx�������������ܵ�TCP���ӣ����ǿ��Լ򵥳�
��Ϊ�������ӡ�ͬʱ������Щ����Ĵ�������У�Nginx����ͼ�������������η������������ӣ����Դ����������η�����ͨ�ţ���ˣ�������
������ngx_connection_t���ǲ�ͬ�ģ�Nginx������}ngx_peer_connection_t�ṹ������ʾ�������ӣ���Ȼ��ngx_peer_connection_t����������
��ngx_connection-t�ṹ��Ϊ����ʵ�ֵġ����ڽ�˵�������������и��ֶε����壬ͬʱ��Ҫע����ǣ����������Ӷ����������ⴴ���������
���ӳ��л�ȡ��
*/
//��������(�ͻ�������nginx)��Ӧ�����ݽṹ��ngx_connection_s����������(nginx���Ӻ�˷�����)��Ӧ�����ݽṹ��ngx_peer_connection_s
struct ngx_peer_connection_s {
    /* һ����������ʵ����Ҳ��Ҫngx_connection_t�ṹ���еĴ󲿷ֳ�Ա�����ҳ������õĿ��Ƕ�������connection��Ա */
    ngx_connection_t                *connection; //����ֵ��ngx_event_connect_peer  ���ӵ�fd������������

    struct sockaddr                 *sockaddr;//Զ�˷�������socket��ַ
    socklen_t                        socklen; //sockaddr��ַ�ĳ���
    ngx_str_t                       *name; //Զ�˷����������� 

    //��ʾ������һ��Զ�˷�����ʱ����ǰ���ӳ����쳣ʧ�ܺ�������ԵĴ�����Ҳ������������ʧ�ܴ���
    ngx_uint_t                       tries; //��ֵ��ngx_http_upstream_init_xxx_peer(����ngx_http_upstream_init_round_robin_peer)
    ngx_msec_t                       start_time;//���˷������������ӵ�ʱ��ngx_http_upstream_init_request

    /*
       fail_timeout�¼��ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
       ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷������ˣ��ο�ngx_http_upstream_get_peer
       //checked�������ʱ�䣬����ĳ��ʱ���fail_timeout���ʱ����ʧЧ�ˣ���ô���fail_timeout���˺�Ҳ������̽ʹ�ø÷�����
     */ 
    //ngx_event_connect_peer��ִ�� ��ȡ���ӵķ��������ʹ�ó����ӹ��ɵ����ӳأ���ô����Ҫʵ��get����
    //ngx_http_upstream_get_round_robin_peer ngx_http_upstream_get_least_conn_peer 
    //ngx_http_upstream_get_hash_peer  ngx_http_upstream_get_ip_hash_peer ngx_http_upstream_get_keepalive_peer��
    ngx_event_get_peer_pt            get; //��ֵ��ngx_http_upstream_init_xxx_peer(����ngx_http_upstream_init_round_robin_peer)
    ngx_event_free_peer_pt           free; //��get������Ӧ���ͷ����ӵķ��� ngx_http_upstream_next����ngx_http_upstream_finalize_request��ִ��

    /*
     ���dataָ������ں������get��free������ϴ��ݲ��������ľ��庬����ʵ��get������free
     ������ģ����أ��ɲ���ngx_event_get_peer_pt��ngx_event_free_pee r_pt����ԭ���е�data����
     */ //�������iphash����data=ngx_http_upstream_ip_hash_peer_data_t->rrp,��ngx_http_upstream_init_ip_hash_peer
    void                            *data; //����rr�㷨,��Ӧ�ṹngx_http_upstream_rr_peer_data_t�������ռ���ngx_http_upstream_create_round_robin_peer

#if (NGX_SSL)
    ngx_event_set_peer_session_pt    set_session;
    ngx_event_save_peer_session_pt   save_session;
#endif

    ngx_addr_t                      *local; //������ַ��Ϣ //proxy_bind  fastcgi_bind ���õı���IP�˿ڵ�ַ���п����豸�кü���eth��ֻ������һ��

    int                              rcvbuf; //�׽��ֵĽ��ջ�������С

    ngx_log_t                       *log; //��¼��־��ngx_log_t����

    unsigned                         cached:1; //��־λ��Ϊ1ʱ��ʾ�����connection�����Ѿ�����

    /* ngx_connection_log_error_e */
    /*NGX_ERROR_IGNORE_EINVAL  ngx_connection_log_error_e
  ��ngx_connection_t���log_error��������ͬ�ģ��������������log_errorֻ����λ��ֻ�ܱ��4�ִ���NGX_ERROR_IGNORE_EINVAL�����޷����
     */
    unsigned                         log_error:2;
};


ngx_int_t ngx_event_connect_peer(ngx_peer_connection_t *pc);
ngx_int_t ngx_event_get_peer(ngx_peer_connection_t *pc, void *data);


#endif /* _NGX_EVENT_CONNECT_H_INCLUDED_ */

