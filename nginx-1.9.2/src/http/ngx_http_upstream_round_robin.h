
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_UPSTREAM_ROUND_ROBIN_H_INCLUDED_
#define _NGX_HTTP_UPSTREAM_ROUND_ROBIN_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct ngx_http_upstream_rr_peer_s   ngx_http_upstream_rr_peer_t;

/*
����upstream {
    server ip1;
    server ip2;
}
��ngx_http_upstream_rr_peers_s���������ngx_http_upstream_rr_peer_s��Ϣ
*/

/* //server backend1.example.com weight=5;
��weight = NUMBER - ���÷�����Ȩ�أ�Ĭ��Ϊ1��
��max_fails = NUMBER - ��һ��ʱ���ڣ����ʱ����fail_timeout���������ã��������������Ƿ����ʱ���������ʧ����������Ĭ��Ϊ1����������Ϊ0���Թرռ�飬��Щ������proxy_next_upstream��fastcgi_next_upstream��404���󲻻�ʹmax_fails���ӣ��ж��塣
��fail_timeout = TIME - �����ʱ���ڲ�����max_fails�����ô�С��ʧ�ܳ������������������������ܲ����ã�ͬ����ָ���˷����������õ�ʱ�䣨����һ�γ�������������֮ǰ����Ĭ��Ϊ10�룬fail_timeout��ǰ����Ӧʱ��û��ֱ�ӹ�ϵ����������ʹ��proxy_connect_timeout��proxy_read_timeout�����ơ�
��down - ��Ƿ�������������״̬��ͨ����ip_hashһ��ʹ�á�
��backup - (0.6.7�����)������еķǱ��ݷ�������崻���æ����ʹ�ñ����������޷���ip_hashָ�����ʹ�ã���
*/
//ngx_http_upstream_rr_peers_s�а��������˷�������Ϣ�������Ǵ浽ngx_http_upstream_srv_conf_t->peer.data����ngx_http_upstream_init_round_robin
struct ngx_http_upstream_rr_peer_s {

    struct sockaddr                *sockaddr; //��ʼ��ֵ��ngx_http_upstream_init_round_robin
    socklen_t                       socklen;  //��ʼ��ֵ��ngx_http_upstream_init_round_robin//��ֵ��ngx_http_upstream_init_round_robin
    ngx_str_t                       name; //��ʼ��ֵ��ngx_http_upstream_init_round_robin
    ngx_str_t                       server; //��ʼ��ֵ��ngx_http_upstream_init_round_robin

    ngx_int_t                       current_weight; //rr�㷨Ȩ�� //��ǰȨ�أ�nginx�������й����е�����Ȩ��
    ngx_int_t                       effective_weight; //rr�㷨Ȩ�� //��ʼ��ֵ��ngx_http_upstream_init_round_robin
    ngx_int_t                       weight;//���õ�Ȩ��

    ngx_uint_t                      conns; //�ú��peer����ĳɹ�������
    /*
        ��fails�ﵽ������޴���max_fails����fail_timeoutʱ�������ٴ�ѡ��ú�˷�������
        ѡ���˷������ɹ���ngx_http_upstream_free_round_robin_peer���fails��0
     */
    ngx_uint_t                      fails;//�ѳ���ʧ�ܴ���  ��ֵ��ngx_http_upstream_free_XXX_peer(ngx_http_upstream_free_round_robin_peer)
    //ѡȡ�ĺ�˷������쳣�����accessedʱ��Ϊ��ǰѡȡ��˷�������ʱ���⵽�쳣��ʱ�䣬��ngx_http_upstream_free_round_robin_peer
    time_t                          accessed;//���ʧ��ʱ�䣬���ڼ��㳬ʱ

    /*
       fail_timeout�¼��ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
       ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷������ˣ��ο�ngx_http_upstream_get_peer
       //checked�������ʱ�䣬����ĳ��ʱ���fail_timeout���ʱ����ʧЧ�ˣ���ô���fail_timeout���˺�Ҳ������̽ʹ�ø÷�����

       1�����server��ʧ�ܴ�����peers->peer[i].fails��û�дﵽ��max_fails�����õ����ʧ�ܴ��������server����Ч�ġ�
      2�����server�Ѿ��ﵽ��max_fails�����õ����ʧ�ܴ���������һʱ�̿�ʼ������fail_timeout �����õ�ʱ����ڣ� server����Ч�ġ�
      3����server��ʧ�ܴ�����peers->peer[i].fails��Ϊ����ʧ�ܴ��������������ڵ�ʱ��(���һ��ѡ�ٸ÷�����ʧ��)������fail_timeout �����õ�ʱ��Σ� ����peers->peer[i].fails =0��ʹ�ø�server������Ч��
    */ 
    //checked�������ʱ�䣬����ĳ��ʱ���fail_timeout���ʱ����ʧЧ�ˣ���ô���fail_timeout���˺�Ҳ������̽ʹ�ø÷�����
    time_t                          checked;//��fail_timeout����Ķ�  һ��fail_timeoutʱ��ε��ˣ������checekedΪ��ǰʱ��

    ngx_uint_t                      max_fails;//���ʧ�ܴ���
    time_t                          fail_timeout;//�೤ʱ���ڳ���max_fails��ʧ�ܱ���Ϊ���down����  �ο������checked

    //�Ƿ������߲�����״̬ ��ֵ��ngx_http_upstream_init_round_robin   
    //ֻ����server xxxx down;����down���ã��÷������Ų��ᱻ��ѯ����һ�㶼����Ϊָ�����ĳ�����������ˣ����޸������ļ�����down��Ȼ������reload nginx����
    ngx_uint_t                      down;          /* unsigned  down:1; *///ָ��ĳ����Ƿ����

#if (NGX_HTTP_SSL)
    void                           *ssl_session;
    int                             ssl_session_len;
#endif
    
    //����ͬһ�������(��backup����backup)��������Ϣֱ��ͨ��ngx_http_upstream_rr_peer_s->next���ӣ�
    //backup�������ͷ�backup������ͨ��ngx_http_upstream_rr_peers_s->next������һ�𣬼�ngx_http_upstream_init_round_robin
    ngx_http_upstream_rr_peer_t    *next; 
    
#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_atomic_t                    lock;
#endif
};


typedef struct ngx_http_upstream_rr_peers_s  ngx_http_upstream_rr_peers_t;

//�˺����ᴴ����˷������б����ҽ��Ǻ󱸷�������󱸷������ֿ����и��Ե���������ÿһ����˷�������һ���ṹ��
//ngx_http_upstream_rr_peer_t��֮��Ӧ��ngx_http_upstream_round_robin.h���� 

//ngx_http_upstream_init_round_robin�и�ֵ�ʹ����ռ�
struct ngx_http_upstream_rr_peers_s { //ÿ��upstream�ڵ����Ϣ  
    ngx_uint_t                      number;//����������  Ϊ��������˶��ٸ�������  ��ֵ��ngx_http_upstream_init_round_robin

#if (NGX_HTTP_UPSTREAM_ZONE)
    ngx_slab_pool_t                *shpool;
    ngx_atomic_t                    rwlock;
#endif

    ngx_uint_t                      total_weight; //���з�������Ȩ�غ�

    unsigned                        single:1;//�Ƿ�ֻ��һ��������������upstrem xxx {server ip}��������¾�һ��ngx_http_upstream_init_round_robin
    ////Ϊ1��ʾ�ܵ�Ȩ��ֵ���ڷ���������
    unsigned                        weighted:1; //���Ϊ1����ʾ��˷�����ȫ��һ��  ngx_http_upstream_init_round_robin

    ngx_str_t                      *name; //upstrem xxx {}�е�xxx�������fastcgi_pass IP:PORT,��û��name

    //����ͬһ�������(��backup����backup)��������Ϣֱ��ͨ��ngx_http_upstream_rr_peer_s->next���ӣ�
    //backup�������ͷ�backup������ͨ��ngx_http_upstream_rr_peers_s->next������һ�𣬼�ngx_http_upstream_init_round_robin
    ngx_http_upstream_rr_peers_t   *next; //�¸�upstream�ڵ㣬�������з�backup��������peers->next��ָ�����е�backup��������Ϣngx_http_upstream_init_round_robin
    
    /*
    ����upstream {
        server ip1;
        server ip2;
    }
    ��ngx_http_upstream_rr_peers_s���������ngx_http_upstream_rr_peer_s��Ϣ
    */
    ngx_http_upstream_rr_peer_t    *peer;//��������Ϣ //���е�peer[]��������Ϣͨ��peers->peer������һ��ngx_http_upstream_init_round_robin
}; //


#if (NGX_HTTP_UPSTREAM_ZONE)

#define ngx_http_upstream_rr_peers_rlock(peers)                               \
                                                                              \
    if (peers->shpool) {                                                      \
        ngx_rwlock_rlock(&peers->rwlock);                                     \
    }

#define ngx_http_upstream_rr_peers_wlock(peers)                               \
                                                                              \
    if (peers->shpool) {                                                      \
        ngx_rwlock_wlock(&peers->rwlock);                                     \
    }

#define ngx_http_upstream_rr_peers_unlock(peers)                              \
                                                                              \
    if (peers->shpool) {                                                      \
        ngx_rwlock_unlock(&peers->rwlock);                                    \
    }


#define ngx_http_upstream_rr_peer_lock(peers, peer)                           \
                                                                              \
    if (peers->shpool) {                                                      \
        ngx_rwlock_wlock(&peer->lock);                                        \
    }

#define ngx_http_upstream_rr_peer_unlock(peers, peer)                         \
                                                                              \
    if (peers->shpool) {                                                      \
        ngx_rwlock_unlock(&peer->lock);                                       \
    }

#else

#define ngx_http_upstream_rr_peers_rlock(peers)
#define ngx_http_upstream_rr_peers_wlock(peers)
#define ngx_http_upstream_rr_peers_unlock(peers)
#define ngx_http_upstream_rr_peer_lock(peers, peer)
#define ngx_http_upstream_rr_peer_unlock(peers, peer)

#endif

/*
current_weight,effective_weight,weight���ߵ������ǲ�ͬ�ģ�����һ�����������ȼ������������ӣ�
http://blog.sina.com.cn/s/blog_7303a1dc01014i0j.html
tried�ֶ�ʵ����һ��λͼ�����ں�˷�����С��32̨ʱ��ʹ��һ��32λint����ʶ�����������Ƿ������ӹ�������˷���������32̨ʱ��
�����ڴ�����������ڴ����洢��λͼ��data�Ǹ��ֶ�ʵ�ʴ洢��λ�á�current�ǵ�ǰ���Ե���̨��������
*/
typedef struct {
    ngx_http_upstream_rr_peers_t   *peers;//���з�������Ϣ   ngx_http_upstream_init_round_robin_peer)
    ngx_http_upstream_rr_peer_t    *current; //��ǰ������ current�ǵ�ǰ���Ե���̨��������
    uintptr_t                      *tried;//������λͼָ�룬���ڼ�¼��������ǰ��״̬  ��ֵ�� ngx_http_upstream_init_round_robin_peer)
    uintptr_t                       data;//triedλͼʵ�ʴ洢λ��  rrp->tried = &rrp->data; ��ֵ��ngx_http_upstream_init_round_robin_peer
} ngx_http_upstream_rr_peer_data_t;//ngx_http_upstream_get_peer��ngx_http_upstream_init_round_robin_peer����Ķ�


ngx_int_t ngx_http_upstream_init_round_robin(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us);
ngx_int_t ngx_http_upstream_init_round_robin_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
ngx_int_t ngx_http_upstream_create_round_robin_peer(ngx_http_request_t *r,
    ngx_http_upstream_resolved_t *ur);
ngx_int_t ngx_http_upstream_get_round_robin_peer(ngx_peer_connection_t *pc,
    void *data);
void ngx_http_upstream_free_round_robin_peer(ngx_peer_connection_t *pc,
    void *data, ngx_uint_t state);

#if (NGX_HTTP_SSL)
ngx_int_t
    ngx_http_upstream_set_round_robin_peer_session(ngx_peer_connection_t *pc,
    void *data);
void ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
    void *data);
#endif


#endif /* _NGX_HTTP_UPSTREAM_ROUND_ROBIN_H_INCLUDED_ */
