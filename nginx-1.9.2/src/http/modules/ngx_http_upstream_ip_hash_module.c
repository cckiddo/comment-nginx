
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct { //��������ʼ����ֵ��ngx_http_upstream_init_ip_hash_peer
    /* the round robin data must be first */ 
    //�����ǵ�һ����Ա���ο�ngx_http_upstream_get_ip_hash_peer
    ngx_http_upstream_rr_peer_data_t   rrp;//r->upstream->peer.data = &iphp->rrp; ngx_http_upstream_init_round_robin_peer�и�ֵ

    ngx_uint_t                         hash; //��ʼ��Ϊ89����ngx_http_upstream_init_ip_hash_peer

    u_char                             addrlen;//iphp->addrlen = 3;//ת��IPv4ֻ�õ���ǰ3���ֽڣ���Ϊ�ں����hash���������ֻ�õ���3���ֽ�  
    u_char                            *addr; //�ͻ���IP��ַiphp->addr = (u_char *) &sin->sin_addr.s_addr;

    u_char                             tries;//�������ӵĴ���   ���ʧ�ܴ���20�Σ���������ֱ��ʹ��rr��ȡpeer

    ngx_event_get_peer_pt              get_rr_peer;//ngx_http_upstream_get_round_robin_peer
} ngx_http_upstream_ip_hash_peer_data_t;


static ngx_int_t ngx_http_upstream_init_ip_hash_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc,
    void *data);
static char *ngx_http_upstream_ip_hash(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_upstream_ip_hash_commands[] = {
/*
ip_hash
�﷨��ip_hash;
���ÿ飺upstream
����Щ�����£����ǿ��ܻ�ϣ������ĳһ���û�������ʼ���䵽�̶���һ̨���η������С����磬�������η������Ỻ��һЩ��Ϣ�����ͬһ���û�����������
��ת������Ⱥ�е���һ̨���η������У���ôÿһ̨���η��������п��ܻỺ��ͬһ����Ϣ����Ȼ������Դ���˷ѣ�Ҳ��������Ч�ع�������Ϣ��
ip_hash�������Խ����������ģ������ȸ��ݿͻ��˵�IP��ַ�����һ��key����key����upstream��Ⱥ������η�������������ȡģ��Ȼ����ȡģ��Ľ����
����ת������Ӧ�����η������С�������ȷ����ͬһ���ͻ��˵�����ֻ��ת����ָ�������η������С�
ip_hash��weight��Ȩ�أ����ò���ͬʱʹ�á����upstream��Ⱥ����һ̨���η�������ʱ�����ã�����ֱ��ɾ�������ã�����Ҫdown������ʶ��ȷ��ת�����Ե�һ���ԡ����磺
upstream backend {
  ip_hash;
  server   backend1.example.com;
  server   backend2.example.com;
  server   backend3.example.com  down;
  server   backend4.example.com;
}

ָ��nginx���ؾ�������ʹ�û��ڿͻ���ip�ĸ��ؾ����㷨��IPV4��ǰ3���˽���λ�����е�IPV6��ַ������һ��hash key���������ȷ����
��ͬ�ͻ��˵�����һֱ���͵���ͬ�ķ������ϳ����������������Ϊ��ͣ������������£�����ᱻ���͵����������ϣ�Ȼ����ܻ�һֱ�� 
�͵���������ϡ�
���nginx���ؾ������������һ��������Ҫ��ʱ�Ƴ�����Ӧ���ò���down��ǣ�����ֹ֮ǰ�Ŀͻ���IP��������������Ϸ�����
���ӣ�
[cpp] view plaincopyprint?

1.upstream backend {  
2.    ip_hash;  
3.   
4.    server backend1.example.com;  
5.    server backend2.example.com;  
6.    server backend3.example.com down;  
7.    server backend4.example.com;  
8.}  
*/
    { ngx_string("ip_hash"),
      NGX_HTTP_UPS_CONF|NGX_CONF_NOARGS,
      ngx_http_upstream_ip_hash,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_ip_hash_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

/*
���ؾ����������:
upstream
server
ip_hash:���ݿͻ��˵�IP����hash,�������squid -- nginx -- server(s)��ip��Զ��squid������ip,��˲�����,��Ҫngx_http_realip_module���ߵ�����ģ��
keepalive:���õ���˵���������������ֳ����ӣ�����Ϊÿһ���ͻ��˶����½���nginx�����PHP�ȷ����������ӣ���Ҫ���ֺͺ��
    �����ӣ�����fastcgi:fastcgi_keep_conn on;       proxy:  proxy_http_version 1.1;  proxy_set_header Connection "";
least_conn:������Ȩ��ֵ���������͵���Ծ���������ٵ���̨������
hash:���԰���uri  ip �Ȳ���������hash

�ο�http://tengine.taobao.org/nginx_docs/cn/docs/http/ngx_http_upstream_module.html#ip_hash
*/


/*
ip_hash�����ؾ�������:
squid -- nginx -- server(s)
    ǰ��ʹ��squid�����棬����ö�̨������������̨���������SESSION������Ϊ�������ؾ��⣬ʹ��nginx��ip_hash������ʹ����Դ�����ĻỰ�ǳ����ġ�
���Ǳ���������һ�����⣬ʹ��nginx��ip_hash�����������ؾ���ʱ���õ���IP��ʼ����squid������IP�����Ǹ��ؾ����ʧЧ�ˡ�
����취:�ο�http://bbs.chinaunix.net/thread-1985674-1-1.html
ngx_http_realip_module
*/
ngx_module_t  ngx_http_upstream_ip_hash_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_ip_hash_module_ctx, /* module context */
    ngx_http_upstream_ip_hash_commands,    /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static u_char ngx_http_upstream_ip_hash_pseudo_addr[3];

/*
Load-blanceģ����4���ؼ��ص�������
�ص�ָ��                  ��������                          round_robinģ��                     IP_hashģ��
 
uscf->peer.init_upstream
���������ļ������е��ã�����upstream�����server����������ʼ׼������������ĺ��Ĺ��������ûص�ָ��us->peer.init�������ļ���������ٱ�����

ngx_http_upstream_init_round_robin
���ã�us->peer.init = ngx_http_upstream_init_round_robin_peer;
 
ngx_http_upstream_init_ip_hash
���ã�us->peer.init = ngx_http_upstream_init_ip_hash_peer;
 


us->peer.init
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���Ϊ����ת��ѡ����ʵĺ�˷���������ʼ׼������������ĺ��Ĺ�
�������ûص�ָ��r->upstream->peer.get��r->upstream->peer.free��
 
ngx_http_upstream_init_round_robin_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_round_robin_peer;
r->upstream->peer.free = ngx_http_upstream_free_round_robin_peer;
 
ngx_http_upstream_init_ip_hash_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_ip_hash_peer;
r->upstream->peer.freeΪ��
 


r->upstream->peer.get
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���ʵ�־����λ����ת��ѡ����ʵĺ�˷��������㷨�߼�����
���ѡ���ȡ���ʺ�˷������Ĺ���
 
ngx_http_upstream_get_round_robin_peer
��Ȩѡ��ǰȨֵ��ߵĺ�˷�����
 
ngx_http_upstream_get_ip_hash_peer
����IP��ϣֵѡ���˷�����
 




r->upstream->peer.free
��ÿһ��Nginx������˷�����֮��Ľ����󶼻���øú�����
ngx_http_upstream_free_round_robin_peer
���������ֵ������rrp->current
*/

static ngx_int_t
ngx_http_upstream_init_ip_hash(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    us->peer.init = ngx_http_upstream_init_ip_hash_peer;

    return NGX_OK;
}

/*
Load-blanceģ����4���ؼ��ص�������
�ص�ָ��                  ��������                          round_robinģ��                     IP_hashģ��
 
uscf->peer.init_upstream (Ĭ��Ϊngx_http_upstream_init_round_robin ��ngx_http_upstream_init_main_conf��ִ��)
���������ļ������е��ã�����upstream�����server����������ʼ׼������������ĺ��Ĺ��������ûص�ָ��us->peer.init�������ļ���������ٱ�����

ngx_http_upstream_init_round_robin
���ã�us->peer.init = ngx_http_upstream_init_round_robin_peer;
 
ngx_http_upstream_init_ip_hash
���ã�us->peer.init = ngx_http_upstream_init_ip_hash_peer;
 


us->peer.init
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���Ϊ����ת��ѡ����ʵĺ�˷���������ʼ׼������������ĺ��Ĺ�
�������ûص�ָ��r->upstream->peer.get��r->upstream->peer.free��
 
ngx_http_upstream_init_round_robin_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_round_robin_peer;
r->upstream->peer.free = ngx_http_upstream_free_round_robin_peer;
 
ngx_http_upstream_init_ip_hash_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_ip_hash_peer;
r->upstream->peer.freeΪ��
 


r->upstream->peer.get
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���ʵ�־����λ����ת��ѡ����ʵĺ�˷��������㷨�߼�����
���ѡ���ȡ���ʺ�˷������Ĺ���
 
ngx_http_upstream_get_round_robin_peer
��Ȩѡ��ǰȨֵ��ߵĺ�˷�����
 
ngx_http_upstream_get_ip_hash_peer
����IP��ϣֵѡ���˷�����
 




r->upstream->peer.free
��ÿһ��Nginx������˷�����֮��Ľ����󶼻���øú�����
ngx_http_upstream_free_round_robin_peer
���������ֵ������rrp->current

��ѯ���Ժ�IP��ϣ���ԶԱ�
��Ȩ��ѯ����
�ŵ㣺�����Ը�ǿ���������ڿͻ��˵��κ���Ϣ����ȫ������˷����������������ѡ���ܰѿͻ����������������ȵط��䵽������˷���������
ȱ�㣺ͬһ���ͻ��˵Ķ��������ܻᱻ���䵽��ͬ�ĺ�˷��������д����޷��������Ự���ֵ�Ӧ�õ�����

IP��ϣ����
�ŵ㣺�ܽϺõذ�ͬһ���ͻ��˵Ķ��������䵽ͬһ̨���������������˼�Ȩ��ѯ�޷����ûỰ���ֵ�����
ȱ�㣺��ĳ��ʱ������ĳ��IP��ַ�������ر�࣬��ô������ĳ̨��˷�������ѹ�����ܷǳ��󣬶�������˷�����ȴ���еĲ����������

*/
//��ѯ��ծ�����㷨ngx_http_upstream_init_round_robin_peer  iphash���ؾ����㷨ngx_http_upstream_init_ip_hash_peer
static ngx_int_t
ngx_http_upstream_init_ip_hash_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    struct sockaddr_in                     *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6                    *sin6;
#endif
    ngx_http_upstream_ip_hash_peer_data_t  *iphp;

    iphp = ngx_palloc(r->pool, sizeof(ngx_http_upstream_ip_hash_peer_data_t));
    if (iphp == NULL) {
        return NGX_ERROR;
    }

    r->upstream->peer.data = &iphp->rrp;
    //������RR�㷨�еĳ�ʼ������
    if (ngx_http_upstream_init_round_robin_peer(r, us) != NGX_OK) {
        return NGX_ERROR;
    }

    r->upstream->peer.get = ngx_http_upstream_get_ip_hash_peer;  

    switch (r->connection->sockaddr->sa_family) {

    case AF_INET:
        sin = (struct sockaddr_in *) r->connection->sockaddr;
        iphp->addr = (u_char *) &sin->sin_addr.s_addr;
        iphp->addrlen = 3;//ת��IPv4ֻ�õ���ǰ3���ֽڣ���Ϊ�ں����hash���������ֻ�õ���3���ֽ�  
        break;

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;
        iphp->addr = (u_char *) &sin6->sin6_addr.s6_addr;
        iphp->addrlen = 16;
        break;
#endif

    default:
        iphp->addr = ngx_http_upstream_ip_hash_pseudo_addr;
        iphp->addrlen = 3;
    }

    iphp->hash = 89;
    iphp->tries = 0;
    iphp->get_rr_peer = ngx_http_upstream_get_round_robin_peer;

    return NGX_OK;
}

/*
Load-blanceģ����4���ؼ��ص�������
�ص�ָ��                  ��������                          round_robinģ��                     IP_hashģ��
 
uscf->peer.init_upstream (Ĭ��Ϊngx_http_upstream_init_round_robin ��ngx_http_upstream_init_main_conf��ִ��)
���������ļ������е��ã�����upstream�����server����������ʼ׼������������ĺ��Ĺ��������ûص�ָ��us->peer.init�������ļ���������ٱ�����

ngx_http_upstream_init_round_robin
���ã�us->peer.init = ngx_http_upstream_init_round_robin_peer;
 
ngx_http_upstream_init_ip_hash
���ã�us->peer.init = ngx_http_upstream_init_ip_hash_peer;
 


us->peer.init
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���Ϊ����ת��ѡ����ʵĺ�˷���������ʼ׼������������ĺ��Ĺ�
�������ûص�ָ��r->upstream->peer.get��r->upstream->peer.free��
 
ngx_http_upstream_init_round_robin_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_round_robin_peer;
r->upstream->peer.free = ngx_http_upstream_free_round_robin_peer;
 
ngx_http_upstream_init_ip_hash_peer
���ã�r->upstream->peer.get = ngx_http_upstream_get_ip_hash_peer;
r->upstream->peer.freeΪ��
 


r->upstream->peer.get
��ÿһ��Nginx׼��ת���ͻ������󵽺�˷�����ǰ������øú������ú���ʵ�־����λ����ת��ѡ����ʵĺ�˷��������㷨�߼�����
���ѡ���ȡ���ʺ�˷������Ĺ���
 
ngx_http_upstream_get_round_robin_peer
��Ȩѡ��ǰȨֵ��ߵĺ�˷�����
 
ngx_http_upstream_get_ip_hash_peer
����IP��ϣֵѡ���˷�����

r->upstream->peer.free
��ÿһ��Nginx������˷�����֮��Ľ����󶼻���øú�����
ngx_http_upstream_free_round_robin_peer
���������ֵ������rrp->current

��ѯ���Ժ�IP��ϣ���ԶԱ�
��Ȩ��ѯ����
�ŵ㣺�����Ը�ǿ���������ڿͻ��˵��κ���Ϣ����ȫ������˷����������������ѡ���ܰѿͻ����������������ȵط��䵽������˷���������
ȱ�㣺ͬһ���ͻ��˵Ķ��������ܻᱻ���䵽��ͬ�ĺ�˷��������д����޷��������Ự���ֵ�Ӧ�õ�����

IP��ϣ����
�ŵ㣺�ܽϺõذ�ͬһ���ͻ��˵Ķ��������䵽ͬһ̨���������������˼�Ȩ��ѯ�޷����ûỰ���ֵ�����
ȱ�㣺��ĳ��ʱ������ĳ��IP��ַ�������ر�࣬��ô������ĳ̨��˷�������ѹ�����ܷǳ��󣬶�������˷�����ȴ���еĲ����������

*/
/*ngx_http_upstream_get_round_robin_peer ngx_http_upstream_get_least_conn_peer ngx_http_upstream_get_hash_peer  
ngx_http_upstream_get_ip_hash_peer ngx_http_upstream_get_keepalive_peer�� */
static ngx_int_t
ngx_http_upstream_get_ip_hash_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_ip_hash_peer_data_t  *iphp = data; 
    //����dataָ�����ngx_http_upstream_ip_hash_peer_data_t->rrp,��Ϊrrp�Ǹýṹ�еĵ�һ����Ա�����Ҳ��ֱ�ӿ��Ի�ȡ�ýṹ������rrp�����ǵ�һ����Ա

    time_t                        now;
    ngx_int_t                     w;
    uintptr_t                     m;
    ngx_uint_t                    i, n, p, hash;
    ngx_http_upstream_rr_peer_t  *peer;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get ip hash peer, try: %ui", pc->tries);

    /* TODO: cached */

    ngx_http_upstream_rr_peers_wlock(iphp->rrp.peers);

    //���ʧ�ܴ���̫�࣬����ֻ��һ����˷�����ôֱ����RRѡ��
    if (iphp->tries > 20 || iphp->rrp.peers->single) {
        ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);
        return iphp->get_rr_peer(pc, &iphp->rrp);
    }

    now = ngx_time();

    pc->cached = 0;
    pc->connection = NULL;

    hash = iphp->hash;

    for ( ;; ) {
        //����IP��hashֵ 
        /*
        1����IP�����ϣֵ���㷨���£� ���й�ʽ��hash��ʼֵΪ89��iphp->addr[i]��ʾ�ͻ��˵�IP�� ͨ�����ι�ϣ����ó�һ��IP�Ĺ�ϣֵ��
            for (i = 0; i < 3; i++) {
                  hash = (hash * 113 + iphp->addr[i]) % 6271;
            }
         
        2����ѡ����һ��serverʱ��ip_hash��ѡ������������ģ�
           ������һ�ι�ϣֵ�Ļ����ϣ��ٴι�ϣ���ͻ�õ�һ��ȫ�µĹ�ϣֵ���ٸ��ݹ�ϣֵѡ������һ����̨�ķ�������
            ��ϣ�㷨��Ȼ��
            for (i = 0; i < 3; i++) {
                  hash = (hash * 113 + iphp->addr[i]) % 6271;
            }
          */
        for (i = 0; i < (ngx_uint_t) iphp->addrlen; i++) { //iphp->hashĬ��89�������ͬһ���ͻ�����������������������hash�϶���ͬ
            //113�����������ù�ϣ�����ɢ��  
            hash = (hash * 113 + iphp->addr[i]) % 6271; //����IP��ַ��ǰ��λ��
        }

        w = hash % iphp->rrp.peers->total_weight;
        peer = iphp->rrp.peers->peer;
        p = 0;

        //���ݹ�ϣ����õ���ѡ�еĺ�˷�����  
        while (w >= peer->weight) {
            w -= peer->weight;
            peer = peer->next;
            p++;
        }

        //��������Ӧ��λͼ�е�λ�ü���  
        n = p / (8 * sizeof(uintptr_t));
        m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));

        if (iphp->rrp.tried[n] & m) {
            goto next;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "get ip hash peer, hash: %ui %04XA", p, m);

        if (peer->down) {
            goto next;
        }

/*
   fail_timeoutʱ���ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
   ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷������ˣ��ο�ngx_http_upstream_get_peer
   //checked�������ʱ�䣬����ĳ��ʱ���fail_timeout���ʱ����ʧЧ�ˣ���ô���fail_timeout���˺�Ҳ������̽ʹ�ø÷�����
 */ 
        if (peer->max_fails
            && peer->fails >= peer->max_fails
            && now - peer->checked <= peer->fail_timeout) //ʧ�ܴ����Ѵ�����  
        {
            goto next;
        }

        break;

    next:

        /*
            ������ip_hash���ԣ����һ����̨�����������ṩ��������ӳ�ʱ�����ʱ�����÷�������ʧ�ܴ����ͻ��һ����һ����������ʧ�ܴ�
        ���ﵽmax_fails�����õ�ֵ���ͻ���fail_timeout�����õ�ʱ����ڲ��ܶ����ṩ��������RR��һ�µġ�
            �����ǰserver�����ṩ���񣬾ͻ���ݵ�ǰ�Ĺ�ϣֵ�ٹ�ϣ��һ���¹�ϣֵ��ѡ����һ���������������ԣ����Ե�������upstream��
        server�ĸ��������server�ĸ�������20��Ҳ����Ҫ����Դ�����20�����ϣ������Դ����ﵽ20�Σ���Ȼ�Ҳ���һ�����ʵķ�������
        ip_hah���Բ��ٳ���ip��ϣֵ��ѡ��server, ����ʣ��ĳ����У�����ת��ʹ��RR�Ĳ��ԣ�ʹ����ѭ�ķ�����ѡ���µ�server��
          */
        if (++iphp->tries > 20) {//�Ѿ�������20����˷���������û�ҵ�һ�����õķ���������ֱ����ʣ��ķ������в�����ѯ�㷨
            ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);
            return iphp->get_rr_peer(pc, &iphp->rrp);
        }
    }

    //��ǰ��������  
    iphp->rrp.current = peer;

    //��������ַ�����ֱ���
    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    if (now - peer->checked > peer->fail_timeout) {
        peer->checked = now;
    }

    ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);

    iphp->rrp.tried[n] |= m; //λͼ����   
    iphp->hash = hash;//�������ӣ�ʹ�´�get_ip_hash_peer��ʱ���ܹ�ѡ��ͬһ��peer��  

    return NGX_OK;
}

static char *
ngx_http_upstream_ip_hash(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf;

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    if (uscf->peer.init_upstream) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "load balancing method redefined");
    }

    uscf->peer.init_upstream = ngx_http_upstream_init_ip_hash;

    uscf->flags = NGX_HTTP_UPSTREAM_CREATE
                  |NGX_HTTP_UPSTREAM_WEIGHT
                  |NGX_HTTP_UPSTREAM_MAX_FAILS
                  |NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
                  |NGX_HTTP_UPSTREAM_DOWN;

    return NGX_CONF_OK;
}
