
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define ngx_http_upstream_tries(p) ((p)->number                               \
                                    + ((p)->next ? (p)->next->number : 0))


static ngx_http_upstream_rr_peer_t *ngx_http_upstream_get_peer(
    ngx_http_upstream_rr_peer_data_t *rrp);

#if (NGX_HTTP_SSL)

static ngx_int_t ngx_http_upstream_empty_set_session(ngx_peer_connection_t *pc,
    void *data);
static void ngx_http_upstream_empty_save_session(ngx_peer_connection_t *pc,
    void *data);

#endif

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
*/

ngx_int_t
ngx_http_upstream_init_round_robin(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us)//��ngx_http_upstream_init_main_conf��ִ��
{
    ngx_url_t                      u;
    ngx_uint_t                     i, j, n, w;
    ngx_http_upstream_server_t    *server;
    ngx_http_upstream_rr_peer_t   *peer, **peerp;
    ngx_http_upstream_rr_peers_t  *peers, *backup;

    us->peer.init = ngx_http_upstream_init_round_robin_peer;

    if (us->servers) {
        server = us->servers->elts;

        n = 0; //���з���������
        w = 0; //���з�������Ȩ��֮��
        

        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) { //���ݷ�������������
                continue;
            }

            n += server[i].naddrs;
            w += server[i].naddrs * server[i].weight;
        }

        if (n == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "no servers in upstream \"%V\" in %s:%ui",
                          &us->host, us->file_name, us->line);
            return NGX_ERROR;
        }

        peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t));
        if (peers == NULL) {
            return NGX_ERROR;
        }

        peer = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peer_t) * n); //Ϊ���еķ�backup������������ش洢�ռ�
        if (peer == NULL) {
            return NGX_ERROR;
        }

        peers->single = (n == 1); //n=1����ʾֻΪ���������һ��������
        peers->number = n;
        peers->weighted = (w != n); //w=n��ʾȨ�ض���ȣ�����1
        peers->total_weight = w;
        peers->name = &us->host;

        n = 0;
        peerp = &peers->peer;
        
        //��ʼ��ÿ��peer�ڵ����Ϣ
        for (i = 0; i < us->servers->nelts; i++) {
            if (server[i].backup) {
                continue;
            }

            for (j = 0; j < server[i].naddrs; j++) {
                peer[n].sockaddr = server[i].addrs[j].sockaddr;
                peer[n].socklen = server[i].addrs[j].socklen;
                peer[n].name = server[i].addrs[j].name;
                peer[n].weight = server[i].weight;
                peer[n].effective_weight = server[i].weight;
                peer[n].current_weight = 0;
                peer[n].max_fails = server[i].max_fails;
                peer[n].fail_timeout = server[i].fail_timeout;
                peer[n].down = server[i].down;
                peer[n].server = server[i].name;

                *peerp = &peer[n]; //���е�peer[]��������Ϣͨ��peers->peer������һ��
                peerp = &peer[n].next;
                n++;
            }
        }

        us->peer.data = peers;

        
        //��ʼ��backup servers
        /* backup servers */

        n = 0;
        w = 0;

        for (i = 0; i < us->servers->nelts; i++) {
            if (!server[i].backup) {
                continue;
            }

            n += server[i].naddrs;
            w += server[i].naddrs * server[i].weight;
        }

        if (n == 0) {
            return NGX_OK;
        }

        backup = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t));
        if (backup == NULL) {
            return NGX_ERROR;
        }

        peer = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peer_t) * n);
        if (peer == NULL) {
            return NGX_ERROR;
        }

        peers->single = 0;
        backup->single = 0;
        backup->number = n;
        backup->weighted = (w != n);
        backup->total_weight = w;
        backup->name = &us->host;

        n = 0;
        peerp = &backup->peer;

        for (i = 0; i < us->servers->nelts; i++) {
            if (!server[i].backup) {
                continue;
            }

            for (j = 0; j < server[i].naddrs; j++) {
                peer[n].sockaddr = server[i].addrs[j].sockaddr;
                peer[n].socklen = server[i].addrs[j].socklen;
                peer[n].name = server[i].addrs[j].name;
                peer[n].weight = server[i].weight;
                peer[n].effective_weight = server[i].weight;
                peer[n].current_weight = 0;
                peer[n].max_fails = server[i].max_fails;
                peer[n].fail_timeout = server[i].fail_timeout;
                peer[n].down = server[i].down;
                peer[n].server = server[i].name;

                *peerp = &peer[n]; //���е�backup������ͨ��next������һ��
                peerp = &peer[n].next;
                n++;
            }
        }

        peers->next = backup; //����backup����������Ϣ������������ķ�backup��������next���棬�������еķ�����(����backup�ͷ�backup)�������ӵ�us->peer.data

        return NGX_OK;
    }


    /* an upstream implicitly defined by proxy_pass, etc. */
    //us�����з�����ָ��Ϊ�գ������û�ֱ����proxy_pass��ָ������ú�˷�������ַ
    if (us->port == 0) {
        ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "no port in upstream \"%V\" in %s:%ui",
                      &us->host, us->file_name, us->line);
        return NGX_ERROR;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.host = us->host;
    u.port = us->port;

    if (ngx_inet_resolve_host(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "%s in upstream \"%V\" in %s:%ui",
                          u.err, &us->host, us->file_name, us->line);
        }

        return NGX_ERROR;
    }

    n = u.naddrs;

    peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t));
    if (peers == NULL) {
        return NGX_ERROR;
    }

    peer = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peer_t) * n);
    if (peer == NULL) {
        return NGX_ERROR;
    }

    peers->single = (n == 1);
    peers->number = n;
    peers->weighted = 0;
    peers->total_weight = n;
    peers->name = &us->host;

    peerp = &peers->peer;

    for (i = 0; i < u.naddrs; i++) {
        peer[i].sockaddr = u.addrs[i].sockaddr;
        peer[i].socklen = u.addrs[i].socklen;
        peer[i].name = u.addrs[i].name;
        peer[i].weight = 1;
        peer[i].effective_weight = 1;
        peer[i].current_weight = 0;
        peer[i].max_fails = 1;
        peer[i].fail_timeout = 10;
        *peerp = &peer[i];
        peerp = &peer[i].next;
    }

    us->peer.data = peers;

    /* implicitly defined upstream has no backup servers */

    return NGX_OK;
}

/*
Load-blanceģ����4���ؼ��ص�������
�ص�ָ��                  ��������                          round_robinģ��                     IP_hashģ��
 
uscf->peer.init_upstreamĬ��Ϊngx_http_upstream_init_round_robin ��ngx_http_upstream_init_main_conf��ִ��
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
//���û���ֶ����÷��ʺ�˷��������㷨����Ĭ����robin��ʽ  //��ѯ��ծ�����㷨ngx_http_upstream_init_round_robin_peer  iphash���ؾ����㷨ngx_http_upstream_init_ip_hash_peer
ngx_int_t //ngx_http_upstream_init_request׼����FCGI���ݣ�buffer�󣬻�����������һ��peer�ĳ�ʼ�����˴�����ѯpeer�ĳ�ʼ����
ngx_http_upstream_init_round_robin_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us) // (Ĭ��Ϊngx_http_upstream_init_round_robin ��ngx_http_upstream_init_main_conf��ִ��)
{//ngx_http_upstream_get_peer��ngx_http_upstream_init_round_robin_peer����Ķ�  
    ngx_uint_t                         n;
    ngx_http_upstream_rr_peer_data_t  *rrp;

    rrp = r->upstream->peer.data;

    if (rrp == NULL) {
        rrp = ngx_palloc(r->pool, sizeof(ngx_http_upstream_rr_peer_data_t));
        if (rrp == NULL) {
            return NGX_ERROR;
        }

        r->upstream->peer.data = rrp;
    }

    rrp->peers = us->peer.data; //��upstream {}���ڵ�
    rrp->current = NULL; //��ǰ�ǵ�0��

    n = rrp->peers->number;

    if (rrp->peers->next && rrp->peers->next->number > n) {
        n = rrp->peers->next->number; //��ȡbackup�ͷ�backup����������������ĸ���������backup����������Ϊ5����backup������Ϊ3��������Ϊ5
    }

    if (n <= 8 * sizeof(uintptr_t)) { //n<32ֱ����data����ʾλͼ��Ϣ
        rrp->tried = &rrp->data;
        rrp->data = 0;

    } else { //����32�������Ӧ�Ķ��INT���洢λͼ
        n = (n + (8 * sizeof(uintptr_t) - 1)) / (8 * sizeof(uintptr_t));

        rrp->tried = ngx_pcalloc(r->pool, n * sizeof(uintptr_t));
        if (rrp->tried == NULL) {
            return NGX_ERROR;
        }
    }

    r->upstream->peer.get = ngx_http_upstream_get_round_robin_peer;
    r->upstream->peer.free = ngx_http_upstream_free_round_robin_peer;
    r->upstream->peer.tries = ngx_http_upstream_tries(rrp->peers);
#if (NGX_HTTP_SSL)
    r->upstream->peer.set_session =
                               ngx_http_upstream_set_round_robin_peer_session;
    r->upstream->peer.save_session =
                               ngx_http_upstream_save_round_robin_peer_session;
#endif

    return NGX_OK;
}


ngx_int_t
ngx_http_upstream_create_round_robin_peer(ngx_http_request_t *r,
    ngx_http_upstream_resolved_t *ur)
{
    u_char                            *p;
    size_t                             len;
    socklen_t                          socklen;
    ngx_uint_t                         i, n;
    struct sockaddr                   *sockaddr;
    ngx_http_upstream_rr_peer_t       *peer, **peerp;
    ngx_http_upstream_rr_peers_t      *peers;
    ngx_http_upstream_rr_peer_data_t  *rrp;

    rrp = r->upstream->peer.data;

    if (rrp == NULL) {
        rrp = ngx_palloc(r->pool, sizeof(ngx_http_upstream_rr_peer_data_t));
        if (rrp == NULL) {
            return NGX_ERROR;
        }

        r->upstream->peer.data = rrp;
    }

    peers = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_rr_peers_t));
    if (peers == NULL) {
        return NGX_ERROR;
    }

    peer = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_rr_peer_t)
                                * ur->naddrs);
    if (peer == NULL) {
        return NGX_ERROR;
    }

    peers->single = (ur->naddrs == 1);
    peers->number = ur->naddrs;
    peers->name = &ur->host;

    if (ur->sockaddr) {
        peer[0].sockaddr = ur->sockaddr;
        peer[0].socklen = ur->socklen;
        peer[0].name = ur->host;
        peer[0].weight = 1;
        peer[0].effective_weight = 1;
        peer[0].current_weight = 0;
        peer[0].max_fails = 1;
        peer[0].fail_timeout = 10;
        peers->peer = peer;

    } else {
        peerp = &peers->peer;

        for (i = 0; i < ur->naddrs; i++) {

            socklen = ur->addrs[i].socklen;

            sockaddr = ngx_palloc(r->pool, socklen);
            if (sockaddr == NULL) {
                return NGX_ERROR;
            }

            ngx_memcpy(sockaddr, ur->addrs[i].sockaddr, socklen);

            switch (sockaddr->sa_family) {
#if (NGX_HAVE_INET6)
            case AF_INET6:
                ((struct sockaddr_in6 *) sockaddr)->sin6_port = htons(ur->port);
                break;
#endif
            default: /* AF_INET */
                ((struct sockaddr_in *) sockaddr)->sin_port = htons(ur->port);
            }

            p = ngx_pnalloc(r->pool, NGX_SOCKADDR_STRLEN);
            if (p == NULL) {
                return NGX_ERROR;
            }

            len = ngx_sock_ntop(sockaddr, socklen, p, NGX_SOCKADDR_STRLEN, 1);

            peer[i].sockaddr = sockaddr;
            peer[i].socklen = socklen;
            peer[i].name.len = len;
            peer[i].name.data = p;
            peer[i].weight = 1;
            peer[i].effective_weight = 1;
            peer[i].current_weight = 0;
            peer[i].max_fails = 1;
            peer[i].fail_timeout = 10;
            *peerp = &peer[i];
            peerp = &peer[i].next;
        }
    }

    rrp->peers = peers;
    rrp->current = NULL;

    if (rrp->peers->number <= 8 * sizeof(uintptr_t)) {
        rrp->tried = &rrp->data;
        rrp->data = 0;

    } else {
        n = (rrp->peers->number + (8 * sizeof(uintptr_t) - 1))
                / (8 * sizeof(uintptr_t));

        rrp->tried = ngx_pcalloc(r->pool, n * sizeof(uintptr_t));
        if (rrp->tried == NULL) {
            return NGX_ERROR;
        }
    }

    r->upstream->peer.get = ngx_http_upstream_get_round_robin_peer;
    r->upstream->peer.free = ngx_http_upstream_free_round_robin_peer;
    r->upstream->peer.tries = ngx_http_upstream_tries(rrp->peers);
#if (NGX_HTTP_SSL)
    r->upstream->peer.set_session = ngx_http_upstream_empty_set_session;
    r->upstream->peer.save_session = ngx_http_upstream_empty_save_session;
#endif

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

��ѯ���Ժ�IP��ϣ���ԶԱ�
��Ȩ��ѯ����
�ŵ㣺�����Ը�ǿ���������ڿͻ��˵��κ���Ϣ����ȫ������˷����������������ѡ���ܰѿͻ����������������ȵط��䵽������˷���������
ȱ�㣺ͬһ���ͻ��˵Ķ��������ܻᱻ���䵽��ͬ�ĺ�˷��������д����޷��������Ự���ֵ�Ӧ�õ�����

IP��ϣ����
�ŵ㣺�ܽϺõذ�ͬһ���ͻ��˵Ķ��������䵽ͬһ̨���������������˼�Ȩ��ѯ�޷����ûỰ���ֵ�����
ȱ�㣺��ĳ��ʱ������ĳ��IP��ַ�������ر�࣬��ô������ĳ̨��˷�������ѹ�����ܷǳ��󣬶�������˷�����ȴ���еĲ����������
*/

/*
 �ж�server �Ƿ���Ч�ķ����ǣ�
 1�����server��ʧ�ܴ�����peers->peer[i].fails��û�дﵽ��max_fails�����õ����ʧ�ܴ��������server����Ч�ġ�
 2�����server�Ѿ��ﵽ��max_fails�����õ����ʧ�ܴ���������һʱ�̿�ʼ������fail_timeout �����õ�ʱ����ڣ� server����Ч�ġ�
 3����server��ʧ�ܴ�����peers->peer[i].fails��Ϊ����ʧ�ܴ��������������ڵ�ʱ�䳬����fail_timeout �����õ�ʱ��Σ� ����peers->peer[i].fails =0��ʹ�ø�server������Ч��

2.2.2.1
���peers�����е�server������Ч��; �ͻ᳢��ȥbackup����������һ����Ч��server, ����ҵ��� ��ת��2.2.3; �����Ȼ�Ҳ�������ʾ��ʱ
upstream����server����ʹ�á��ͻ��������peers���������е�ʧ�ܴ����ļ�¼��ʹ����server���������Ч����������Ŀ����Ϊ�˷�ֹ�´���
���������ʱ�����Ҳ���һ����Ч��server.
    for (i = 0; i < peers->number; i++) {
            peers->peer[i].fails2 = 0;
    }
    �����ش������nginx,  nginx�õ��˴�����󣬾Ͳ������̨server�����󣬶�����nginx�Ĵ�����־�������no live upstreams while connecting to upstream��
    �ļ�¼�������no live����������ԭ�򣩣���ֱ�ӷ��ظ�����Ŀͻ���һ��502�Ĵ���
*/
/*ngx_http_upstream_get_round_robin_peer ngx_http_upstream_get_least_conn_peer ngx_http_upstream_get_hash_peer  
ngx_http_upstream_get_ip_hash_peer ngx_http_upstream_get_keepalive_peer�� */
ngx_int_t
ngx_http_upstream_get_round_robin_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_rr_peer_data_t  *rrp = data;

    ngx_int_t                      rc;
    ngx_uint_t                     i, n;
    ngx_http_upstream_rr_peer_t   *peer;
    ngx_http_upstream_rr_peers_t  *peers;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get rr peer, try: %ui", pc->tries);

    pc->cached = 0;
    pc->connection = NULL;

    peers = rrp->peers;
    ngx_http_upstream_rr_peers_wlock(peers);

    if (peers->single) {
        peer = peers->peer;

        if (peer->down) {  //�÷������Ѿ�ʧЧ��������ʹ�ø÷�������
            goto failed;
        }

        rrp->current = peer;

    } else {

        /* there are several peers */

        peer = ngx_http_upstream_get_peer(rrp);
        //get_peer�����������ȼ����ķ�����

        if (peer == NULL) {
            goto failed;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "get rr peer, current: %p %i",
                       peer, peer->current_weight);
    }
    
    //pc�м�¼��ѡȡ�ķ���������Ϣ
    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    ngx_http_upstream_rr_peers_unlock(peers);

    return NGX_OK;

failed: //ѡ��ʧ�ܣ�ת��󱸷�����

    if (peers->next) {

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, pc->log, 0, "backup servers");

        rrp->peers = peers->next;

        n = (rrp->peers->number + (8 * sizeof(uintptr_t) - 1))
                / (8 * sizeof(uintptr_t));

        for (i = 0; i < n; i++) {
             rrp->tried[i] = 0;
        }

        ngx_http_upstream_rr_peers_unlock(peers);

        rc = ngx_http_upstream_get_round_robin_peer(pc, rrp);

        if (rc != NGX_BUSY) {
            return rc;
        }

        ngx_http_upstream_rr_peers_wlock(peers);
    }

    /* all peers failed, mark them as live for quick recovery */
    /*
         ��ʾ��ʱupstream����server����ʹ�á��ͻ��������peers���������е�ʧ�ܴ����ļ�¼��ʹ����server���������Ч������
         ����Ŀ����Ϊ�˷�ֹ�´������������ʱ�����Ҳ���һ����Ч��server.
     */
    for (peer = peers->peer; peer; peer = peer->next) {
        peer->fails = 0;
    }

    ngx_http_upstream_rr_peers_unlock(peers);

    pc->name = peers->name;

    return NGX_BUSY;
}

/*
   fail_timeout�¼��ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
   ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷������ˣ��ο�ngx_http_upstream_get_peer
   //checked�������ʱ�䣬����ĳ��ʱ���fail_timeout���ʱ����ʧЧ�ˣ���ô���fail_timeout���˺�Ҳ������̽ʹ�ø÷�����
 */ 
//get_peer�����������ȼ����ķ����� //���յ�ǰ��������Ȩֵ����ѡ��
static ngx_http_upstream_rr_peer_t *
ngx_http_upstream_get_peer(ngx_http_upstream_rr_peer_data_t *rrp)//ngx_http_upstream_get_peer��ngx_http_upstream_init_round_robin_peer����Ķ�
{
    time_t                        now;
    uintptr_t                     m;
    ngx_int_t                     total;
    ngx_uint_t                    i, n, p;
    ngx_http_upstream_rr_peer_t  *peer, *best;

    now = ngx_time();

    best = NULL;
    total = 0;

#if (NGX_SUPPRESS_WARN)
    p = 0;
#endif

    for (peer = rrp->peers->peer, i = 0;
         peer;
         peer = peer->next, i++) //ngx_http_upstream_get_peer��ngx_http_upstream_init_round_robin_peer����Ķ�
    {
        //���㵱ǰ�������ı��λ��λͼ�е�λ��
        n = i / (8 * sizeof(uintptr_t));
        m = (uintptr_t) 1 << i % (8 * sizeof(uintptr_t));

        if (rrp->tried[n] & m) {//�Ѿ�ѡ���������
            continue;
        }

        if (peer->down) {//��ǰ��������崻����ų�
            continue;
        }

      /*
       fail_timeout�¼��ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
       ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷�������
       */
        if (peer->max_fails
            && peer->fails >= peer->max_fails
            && now - peer->checked <= peer->fail_timeout)//����ָ��һ��ʱ�������ʧ�ܴ������ж�
        {
            continue;
        }
        /*
          foreach peer in peers {
            peer->current_weight += peer->effective_weight;
                total += peer->effective_weight;
             
                if (best == NULL || peer->current_weight > best->current_weight) {
                    best = peer;
            }
            }
            best->current_weight -= total;
            ����㷨Ӧ��˵���Ƕ����ļ�Ȩ��̬���ȼ��㷨�������ص������㣺һ�����ȼ�current_weight�ı仯����Ȩeffective_weight�����Ƕ���ѡserver�����ȼ����д��ģ�����������̶�������server��Ȩֵ֮�͡������㷨�Ľ���ص�һ����Ȩ�ߵ�serverһ���ȱ�ѡ�У����Ҹ�Ƶ���ı�ѡ�У���Ȩ�͵�serverҲ���������������ȼ�����ѡ�С���������ı߽�����������㷨�õ���������a, a, b, a, c, a, a�����ȳ̶������ǳ�������
            ���������Լ������ӣ�����Ҳ����һ�£�
            selected server current_weight          before selected           current_weight after selected
                    a                                   { 5, 1, 2 }                 { -3, 1, 2 }
                    b                                   { 2, 2, 4 }                 { 2, 2, -4 }
                    a                                    { 7, 3, -2 }            { -1, 3, -2 }
                    a                                    { 4, 4, 0 }            { -4, 4, 0 }
                    b                                    { 1, 5, 2 }            { 1, -3, 2 }
                    a                                    { 6, -2, 4 }            { -2, -2, 4 }
                    b                                    { 3, -1, 6 }            { 3, -1, -2 }
                    a                                   { 8, 0, 0 }              { 0, 0, 0 }
            ����һ��ѡ���Ժ����ȼ��ָ�����ʼ״̬���������ʹ�ô���������̡�Cool!
          */
        //��Ȩ��ѵ�㷨���Բο�:http://blog.sina.com.cn/s/blog_7303a1dc01014i0j.html
        peer->current_weight += peer->effective_weight;
        total += peer->effective_weight;

        if (peer->effective_weight < peer->weight) {//����������effective_weight �𽥻ָ�����    
            peer->effective_weight++;
        }

        if (best == NULL || peer->current_weight > best->current_weight) {
            best = peer;
            p = i;
        }
    }

    if (best == NULL) {
        return NULL;
    }

    rrp->current = best;
    
    //��ѡ��ķ������ڷ������б��е�λ��
    n = p / (8 * sizeof(uintptr_t));
    m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));

    rrp->tried[n] |= m; //λͼ��Ӧλ����λ

    best->current_weight -= total;

    /*
       fail_timeout�¼��ڷ��ʺ�˳��ִ���Ĵ������ڵ���max_fails������Ϊ�÷����������ã���ô����������ˣ���˸÷������лָ�����ô�жϼ����?
       ��:�����fail_timeoutʱ��ι��˺󣬻�����peer->checked����ô�п�����̽�÷�������
     */
    if (now - best->checked > best->fail_timeout) {
        best->checked = now;
    }

    return best;
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

//ngx_http_upstream_free_round_robin_peer�������������ı�־�ֶζ��ָ�����ʼ״̬���Ա����ʹ��
void
ngx_http_upstream_free_round_robin_peer(ngx_peer_connection_t *pc, void *data,
    ngx_uint_t state)
{
    ngx_http_upstream_rr_peer_data_t  *rrp = data;

    time_t                       now;
    ngx_http_upstream_rr_peer_t  *peer;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "free rr peer %ui %ui", pc->tries, state);

    /* TODO: NGX_PEER_KEEPALIVE */

    peer = rrp->current;

    ngx_http_upstream_rr_peers_rlock(rrp->peers);
    ngx_http_upstream_rr_peer_lock(rrp->peers, peer);

    if (rrp->peers->single) {

        peer->conns--;

        ngx_http_upstream_rr_peer_unlock(rrp->peers, peer);
        ngx_http_upstream_rr_peers_unlock(rrp->peers);

        pc->tries = 0;
        return;
    }

    if (state & NGX_PEER_FAILED) { //��ngx_http_upstream_next�ߵ�����
        now = ngx_time();

        peer->fails++;
        peer->accessed = now; //ѡȡ�ĺ�˷������쳣�����accessedʱ��Ϊ��ǰѡȡ��˷�������ʱ���⵽�쳣��ʱ��
        peer->checked = now;

        if (peer->max_fails) {//�������쳣ʱ������effective_weight
            peer->effective_weight -= peer->weight / peer->max_fails;//�������쳣ʱ������effective_weight

            if (peer->fails >= peer->max_fails) { //������������ʧ�ܴ���
                ngx_log_error(NGX_LOG_WARN, pc->log, 0,
                              "upstream server temporarily disabled");
            }
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "free rr peer failed: %p %i",
                       peer, peer->effective_weight);

        if (peer->effective_weight < 0) {
            peer->effective_weight = 0;
        }

    } else {

        /* mark peer live if check passed */
        //һ��fail_timeoutʱ��ε��ˣ������checekedΪ��ǰʱ��
        if (peer->accessed < peer->checked) { 
        //�ӻ�ȡ���ʧ�ܲ����жϴﵽ�������ʧ�ܴ�������Ḵ��ú�˷�������ͬʱ�ٴ�ѡ��÷�����������÷���������Ч�ˣ��������checkedΪ��ǰʱ��
        //��ʱ��peer->accessed < peer->checked������������fails��0
            peer->fails = 0;
        }
    }

    peer->conns--;

    ngx_http_upstream_rr_peer_unlock(rrp->peers, peer);
    ngx_http_upstream_rr_peers_unlock(rrp->peers);

    if (pc->tries) {
        pc->tries--;
    }
}

#if (NGX_HTTP_SSL)

ngx_int_t
ngx_http_upstream_set_round_robin_peer_session(ngx_peer_connection_t *pc,
    void *data)
{
    ngx_http_upstream_rr_peer_data_t  *rrp = data;

    ngx_int_t                      rc;
    ngx_ssl_session_t             *ssl_session;
    ngx_http_upstream_rr_peer_t   *peer;
#if (NGX_HTTP_UPSTREAM_ZONE)
    int                            len;
#if OPENSSL_VERSION_NUMBER >= 0x0090707fL
    const
#endif
    u_char                        *p;
    ngx_http_upstream_rr_peers_t  *peers;
    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
#endif

    peer = rrp->current;

#if (NGX_HTTP_UPSTREAM_ZONE)
    peers = rrp->peers;

    if (peers->shpool) {
        ngx_http_upstream_rr_peers_rlock(peers);
        ngx_http_upstream_rr_peer_lock(peers, peer);

        if (peer->ssl_session == NULL) {
            ngx_http_upstream_rr_peer_unlock(peers, peer);
            ngx_http_upstream_rr_peers_unlock(peers);
            return NGX_OK;
        }

        len = peer->ssl_session_len;

        ngx_memcpy(buf, peer->ssl_session, len);

        ngx_http_upstream_rr_peer_unlock(peers, peer);
        ngx_http_upstream_rr_peers_unlock(peers);

        p = buf;
        ssl_session = d2i_SSL_SESSION(NULL, &p, len);

        rc = ngx_ssl_set_session(pc->connection, ssl_session);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "set session: %p", ssl_session);

        ngx_ssl_free_session(ssl_session);

        return rc;
    }
#endif

    ssl_session = peer->ssl_session;

    rc = ngx_ssl_set_session(pc->connection, ssl_session);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "set session: %p", ssl_session);

    return rc;
}


void
ngx_http_upstream_save_round_robin_peer_session(ngx_peer_connection_t *pc,
    void *data)
{
    ngx_http_upstream_rr_peer_data_t  *rrp = data;

    ngx_ssl_session_t             *old_ssl_session, *ssl_session;
    ngx_http_upstream_rr_peer_t   *peer;
#if (NGX_HTTP_UPSTREAM_ZONE)
    int                            len;
    u_char                        *p;
    ngx_http_upstream_rr_peers_t  *peers;
    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
#endif

#if (NGX_HTTP_UPSTREAM_ZONE)
    peers = rrp->peers;

    if (peers->shpool) {

        ssl_session = SSL_get0_session(pc->connection->ssl->connection);

        if (ssl_session == NULL) {
            return;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "save session: %p", ssl_session);

        len = i2d_SSL_SESSION(ssl_session, NULL);

        /* do not cache too big session */

        if (len > NGX_SSL_MAX_SESSION_SIZE) {
            return;
        }

        p = buf;
        (void) i2d_SSL_SESSION(ssl_session, &p);

        peer = rrp->current;

        ngx_http_upstream_rr_peers_rlock(peers);
        ngx_http_upstream_rr_peer_lock(peers, peer);

        if (len > peer->ssl_session_len) {
            ngx_shmtx_lock(&peers->shpool->mutex);

            if (peer->ssl_session) {
                ngx_slab_free_locked(peers->shpool, peer->ssl_session);
            }

            peer->ssl_session = ngx_slab_alloc_locked(peers->shpool, len);

            ngx_shmtx_unlock(&peers->shpool->mutex);

            if (peer->ssl_session == NULL) {
                peer->ssl_session_len = 0;

                ngx_http_upstream_rr_peer_unlock(peers, peer);
                ngx_http_upstream_rr_peers_unlock(peers);
                return;
            }

            peer->ssl_session_len = len;
        }

        ngx_memcpy(peer->ssl_session, buf, len);

        ngx_http_upstream_rr_peer_unlock(peers, peer);
        ngx_http_upstream_rr_peers_unlock(peers);

        return;
    }
#endif

    ssl_session = ngx_ssl_get_session(pc->connection);

    if (ssl_session == NULL) {
        return;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "save session: %p", ssl_session);

    peer = rrp->current;

    old_ssl_session = peer->ssl_session;
    peer->ssl_session = ssl_session;

    if (old_ssl_session) {

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "old session: %p", old_ssl_session);

        /* TODO: may block */

        ngx_ssl_free_session(old_ssl_session);
    }
}


static ngx_int_t
ngx_http_upstream_empty_set_session(ngx_peer_connection_t *pc, void *data)
{
    return NGX_OK;
}


static void
ngx_http_upstream_empty_save_session(ngx_peer_connection_t *pc, void *data)
{
    return;
}

#endif
