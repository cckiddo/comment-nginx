
/*
 * Copyright (C) Maxim Dounin
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    //��󻺴����Ӹ�������keepalive����ָ����keepalive connection��  Ĭ��0��������keepalive con-num����
    ngx_uint_t                         max_cached; //keepalive�ĵ�һ������  ����max_cached��ngx_http_upstream_keepalive_cache_t

    /*
    //�����Ӷ��У�����cacheΪ�������ӳأ�freeΪ�������ӳء���ʼ��ʱ����keepaliveָ��Ĳ�����ʼ��free���У����������ӹ�����free����
    //ȡ���ӣ�����������󽫳����ӻ��浽cache���У����ӱ��Ͽ�����ʱ���ٴ�cache���з���free����

    //ngx_http_upstream_free_keepalive_peer��kp->conf->cache����ӻ���ngx_connection_t��ngx_http_upstream_get_keepalive_peer�ӻ�����
    //ȡ���ͺ�˵����ӻ���ngx_connection_t�����Ա����ظ��Ľ����͹ر�TCP����
     */
    ngx_queue_t                        cache; // keepalive�ĵڶ�������
    ngx_queue_t                        free; //��ʼ��������max_cached��ngx_http_upstream_keepalive_cache_t����ӵ���free������

    /*keepalive module�����hash  ip_hash least_connʹ�õģ���������Щģ��֮��original_init_upstream��original_init_peerָ����
    ����ģ���uscf->peer.init_upstream����uscf->peer.init*/
    ngx_http_upstream_init_pt          original_init_upstream;
    ngx_http_upstream_init_peer_pt     original_init_peer;

} ngx_http_upstream_keepalive_srv_conf_t;//�����upstream���keepaliveָ�������������ݽṹ��


typedef struct {//ngx_http_upstream_init_keepalive�п���max_cached(keepalive����)���ռ�
    ngx_http_upstream_keepalive_srv_conf_t  *conf;

    ngx_queue_t                        queue;
    ngx_connection_t                  *connection;

     //�������ӳ��б���ĺ�˷������ĵ�ַ���������Ǹ�����ͬ��socket��ַ���ҳ���Ӧ�����ӣ���ʹ�ø�����
    socklen_t                          socklen;
    u_char                             sockaddr[NGX_SOCKADDRLEN];

} ngx_http_upstream_keepalive_cache_t;


typedef struct { //ngx_http_upstream_init_keepalive_peer�п��ٿռ�
    ngx_http_upstream_keepalive_srv_conf_t  *conf;//ָ��ngx_http_upstream_keepalive_srv_conf_t

    ngx_http_upstream_t               *upstream; //r->upstream

    void                              *data;

    /*
    //����ԭʼ��ȡpeer���ͷ�peer�Ĺ��ӣ�����ͨ����ngx_http_upstream_get_round_robin_peer��ngx_http_upstream_free_round_robin_peer��
    nginx���ؾ���Ĭ����ʹ����ѯ�㷨
     */
     /*keepalive module�����hash  ip_hash least_connʹ�õģ���������Щģ��֮��original_init_upstream��original_init_peerָ����
    ����ģ���get��free*/
    ngx_event_get_peer_pt              original_get_peer;
    ngx_event_free_peer_pt             original_free_peer;

#if (NGX_HTTP_SSL)
    ngx_event_set_peer_session_pt      original_set_session;
    ngx_event_save_peer_session_pt     original_save_session;
#endif

} ngx_http_upstream_keepalive_peer_data_t;


static ngx_int_t ngx_http_upstream_init_keepalive_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_keepalive_peer(ngx_peer_connection_t *pc,
    void *data);
static void ngx_http_upstream_free_keepalive_peer(ngx_peer_connection_t *pc,
    void *data, ngx_uint_t state);

static void ngx_http_upstream_keepalive_dummy_handler(ngx_event_t *ev);
static void ngx_http_upstream_keepalive_close_handler(ngx_event_t *ev);
static void ngx_http_upstream_keepalive_close(ngx_connection_t *c);

#if (NGX_HTTP_SSL)
static ngx_int_t ngx_http_upstream_keepalive_set_session(
    ngx_peer_connection_t *pc, void *data);
static void ngx_http_upstream_keepalive_save_session(ngx_peer_connection_t *pc,
    void *data);
#endif

static void *ngx_http_upstream_keepalive_create_conf(ngx_conf_t *cf);
static char *ngx_http_upstream_keepalive(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

//ָ�������ڳ����ӵ�������
static ngx_command_t  ngx_http_upstream_keepalive_commands[] = {
    // Ĭ��0��������keepalive con-num����
    { ngx_string("keepalive"), //������ٸ������ӣ�һ��ͺ���ж��ٸ����������������Ϊ���٣����nginx������ÿһ�����ֻ����һ��TCP����
      NGX_HTTP_UPS_CONF|NGX_CONF_TAKE1,
      ngx_http_upstream_keepalive,   //keepalive connections
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_keepalive_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    ngx_http_upstream_keepalive_create_conf, /* create server configuration */
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
���Ҫʵ��upstream�����ӣ���ÿ��������Ҫ����һ��connection pool�����涼�ǳ����ӡ�һ�����˷������������ӣ����ڵ�ǰ�������ӽ�
��֮�󲻻������ر����ӣ����ǰ���������ӱ�����һ��keepalive connection pool���棬�Ժ�ÿ����Ҫ����������ӵ�ʱ��ֻ��Ҫ�����
���ӳ������ң�����ҵ����ʵ����ӵĻ����Ϳ���ֱ������������ӣ�����Ҫ���´���socket���߷���connect()��������ʡ�½�������ʱ����
�ֵ�ʱ�����ģ��ֿ��Ա���TCP���ӵ�slow start�������keepalive���ӳ��Ҳ������ʵ����ӣ��ǾͰ���ԭ���Ĳ������½������ӡ��������Ӳ�
��ʱ����Ժ��Բ��ƣ���ô���ַ����϶���������޺��ģ���Ȼ����Ҫ����������ڴ棩��
*/ 
//���������keepalive  num�����ж��ٸ��ͻ������󵽺�ˣ�nginx�ͻ�ͺ�˽������ٸ�tcp���ӣ�������˸ò�������num��connect����
//�������´��пͻ��������ֱ��ʹ�û����е����ӣ����ⲻͣ����TCP����
ngx_module_t  ngx_http_upstream_keepalive_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_keepalive_module_ctx, /* module context */
    ngx_http_upstream_keepalive_commands,    /* module directives */
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


static ngx_int_t
ngx_http_upstream_init_keepalive(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_uint_t                               i;
    ngx_http_upstream_keepalive_srv_conf_t  *kcf;
    ngx_http_upstream_keepalive_cache_t     *cached;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
                   "init keepalive");

    kcf = ngx_http_conf_upstream_srv_conf(us,
                                          ngx_http_upstream_keepalive_module);

    // ��ִ��ԭʼ��ʼ��upstream��������ngx_http_upstream_init_round_robin�����ú�����������õĺ�˵�ַ������socket��ַ����
    //�����Ӻ�ˡ�������us->peer.init����Ϊngx_http_upstream_init_round_robin_peer
    if (kcf->original_init_upstream(cf, us) != NGX_OK) { //Ĭ��ngx_http_upstream_init_round_robin
        return NGX_ERROR;
    }

    // ����ԭ���ӣ�����keepalive�Ĺ��Ӹ��Ǿɹ��ӣ���ʼ����������ʱ����������¹���
    kcf->original_init_peer = us->peer.init;

    us->peer.init = ngx_http_upstream_init_keepalive_peer;

    /* allocate cache items and add to free queue */
     /* ���뻺�������ӵ�free�����У������ô�free��������ȡ */
    cached = ngx_pcalloc(cf->pool,
                sizeof(ngx_http_upstream_keepalive_cache_t) * kcf->max_cached);
    if (cached == NULL) {
        return NGX_ERROR;
    }

    ngx_queue_init(&kcf->cache);
    ngx_queue_init(&kcf->free);

    /*
    ��Ԥ����max_cached�����������Ϣngx_http_upstream_keepalive_cache_t�������������ӹ�����free����
    ȡ���ӣ�����������󽫳����ӻ��浽cache���У����ӱ��Ͽ�����ʱ���ٴ�cache���з���free����
    */
    for (i = 0; i < kcf->max_cached; i++) {
        ngx_queue_insert_head(&kcf->free, &cached[i].queue);
        cached[i].conf = kcf;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_init_keepalive_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_upstream_keepalive_peer_data_t  *kp;
    ngx_http_upstream_keepalive_srv_conf_t   *kcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "init keepalive peer");

    kcf = ngx_http_conf_upstream_srv_conf(us,
                                          ngx_http_upstream_keepalive_module);

    kp = ngx_palloc(r->pool, sizeof(ngx_http_upstream_keepalive_peer_data_t));
    if (kp == NULL) {
        return NGX_ERROR;
    }

    /* 
    ��ִ��ԭʼ�ĳ�ʼ��peer��������ngx_http_upstream_init_round_robin_peer����
    �����ڲ�����һЩ�븺�ؾ�����صĲ������ֱ����������ĸ����ӣ�
    r->upstream->peer.get�� r->upstream->peer.free
    r->upstream->peer.set_session�� r->upstream->peer.save_session 
    */
    if (kcf->original_init_peer(r, us) != NGX_OK) {
        return NGX_ERROR;
    }

    kp->conf = kcf;
    kp->upstream = r->upstream;

     // keepaliveģ���򱣴�����ԭʼ���ӣ���ʹ���µĸ��๳�Ӹ��Ǿɹ���
    kp->data = r->upstream->peer.data;
    kp->original_get_peer = r->upstream->peer.get;
    kp->original_free_peer = r->upstream->peer.free;

    r->upstream->peer.data = kp;
    r->upstream->peer.get = ngx_http_upstream_get_keepalive_peer;
    r->upstream->peer.free = ngx_http_upstream_free_keepalive_peer;

#if (NGX_HTTP_SSL)
    kp->original_set_session = r->upstream->peer.set_session;
    kp->original_save_session = r->upstream->peer.save_session;
    r->upstream->peer.set_session = ngx_http_upstream_keepalive_set_session;
    r->upstream->peer.save_session = ngx_http_upstream_keepalive_save_session;
#endif

    return NGX_OK;
}

/*
//ngx_event_connect_peer����pc->get���ӡ���ǰ������������keepalive upstream����ù�����
//ngx_http_upstream_get_keepalive_peer����ʱ������ڻ��泤���Ӹú������÷��ص���
//NGX_DONE��ֱ�ӷ����ϲ���ö������������ִ�л�ȡ�µ����Ӳ�����socket��
//��������ڻ���ĳ����ӣ���᷵��NGX_OK.
//���Ƿ�keepalive upstream���ù�����ngx_http_upstream_get_round_robin_peer��
*/

//ngx_http_upstream_free_keepalive_peer��kp->conf->cache����ӻ���ngx_connection_t��ngx_http_upstream_get_keepalive_peer�ӻ�����
//ȡ���ͺ�˵����ӻ���ngx_connection_t�����Ա����ظ��Ľ����͹ر�TCP����

/*ngx_http_upstream_get_round_robin_peer ngx_http_upstream_get_least_conn_peer ngx_http_upstream_get_hash_peer  
ngx_http_upstream_get_ip_hash_peer ngx_http_upstream_get_keepalive_peer�� */
static ngx_int_t
ngx_http_upstream_get_keepalive_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_keepalive_peer_data_t  *kp = data;
    ngx_http_upstream_keepalive_cache_t      *item;

    ngx_int_t          rc;
    ngx_queue_t       *q, *cache;
    ngx_connection_t  *c;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get keepalive peer");

    /* ask balancer */
    // �ȵ���ԭʼgetpeer���ӣ�ngx_http_upstream_get_round_robin_peer��ѡ����
    rc = kp->original_get_peer(pc, kp->data); //ͨ��hash  ip_hash rr��ʽѡ���˵ķ�����peer

    if (rc != NGX_OK) {
        return rc;
    }

    /* �Ѿ�ѡ��Ӧ�ð����������ĳ���ڵ㣬Ȼ�������ѡ�������ڵ��ĳ�����еĳ��������������� */
    
    /* search cache for suitable connection */

    cache = &kp->conf->cache;
    // ����socket��ַ��������cache�أ��ҵ�ֱ�ӷ���NGX_DONE���ϲ���þͲ����ȡ�µ�����
    for (q = ngx_queue_head(cache);
         q != ngx_queue_sentinel(cache);
         q = ngx_queue_next(q))
    {
        item = ngx_queue_data(q, ngx_http_upstream_keepalive_cache_t, queue);
        c = item->connection;

        if (ngx_memn2cmp((u_char *) &item->sockaddr, (u_char *) pc->sockaddr,
                         item->socklen, pc->socklen)
            == 0)
        {
            ngx_queue_remove(q);
            ngx_queue_insert_head(&kp->conf->free, q);

            goto found;
        }
    }

    return NGX_OK;

found:

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get keepalive peer: using connection %p", c);

    c->idle = 0;
    c->sent = 0;
    c->log = pc->log;
    c->read->log = pc->log;
    c->write->log = pc->log;
    c->pool->log = pc->log;

    pc->connection = c;
    pc->cached = 1;

    return NGX_DONE;
}
/*
������ngx_http_upstream_free_keepalive_peer�������ڻع�ͷȥ��ngx_http_upstream_get_keepalive_peer�͸����������θ���keepalive���ӵģ�
free��������ǰ���ӻ��浽cache�����У�����������Ӷ�Ӧ��˵�socket��ַ��get����������Ҫ���Ӻ�˵�socket��ַ����������cache���У�����ҵ�
��ʹ����ǰ����ĳ����ӣ�δ�ҵ������½����µ����ӡ�

��free�������ֵ�ǰ����cache item����ʱ�����������Ӵﵽ���ޣ�����ر����δ��ʹ�õ��Ǹ����ӣ����������µ����ӡ�Nginx�ٷ��Ƽ�keepalive��
������Ӧ�����õľ�����С��������ֱ����������̫�������µ������������ʱ�޷���ȡ���ӵ������һ��worker���̵������ӳ������޵ģ���
*/
//��һ��HTTP��������Ϻ�ͨ�������ngx_http_upstream_finalize_request�������󣬲������ͷ�peer�Ĳ���
//�ù��ӻὫ���ӻ��浽������cache�أ�����u->peer.connection���óɿգ���ֹngx_http_upstream_finalize_request�����沿�ִ���ر����ӡ�

//ngx_http_upstream_free_keepalive_peer��kp->conf->cache����ӻ���ngx_connection_t��ngx_http_upstream_get_keepalive_peer�ӻ�����
//ȡ���ͺ�˵����ӻ���ngx_connection_t�����Ա����ظ��Ľ����͹ر�TCP����
static void
ngx_http_upstream_free_keepalive_peer(ngx_peer_connection_t *pc, void *data,
    ngx_uint_t state)
{
    ngx_http_upstream_keepalive_peer_data_t  *kp = data;
    ngx_http_upstream_keepalive_cache_t      *item;

    ngx_queue_t          *q;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "free keepalive peer");

    /* cache valid connections */

    u = kp->upstream;
    c = pc->connection;

    if (state & NGX_PEER_FAILED
        || c == NULL
        || c->read->eof
        || c->read->error
        || c->read->timedout
        || c->write->error
        || c->write->timedout)
    {
        goto invalid;
    }

    if (!u->keepalive) { 
    //˵�����κͺ�˵�����ʹ�õ��ǻ���cache(keepalive����)connection��TCP���ӣ�Ҳ����ʹ�õ���֮ǰ�Ѿ��ͺ�˽����õ�TCP����ngx_connection_t
        goto invalid;
    }

    //ͨ������keepalive�����Ӷ����ɺ��web������ģ������Ҫ��Ӷ��¼�
    if (ngx_handle_read_event(c->read, 0, NGX_FUNC_LINE) != NGX_OK) {
        goto invalid;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "free keepalive peer: saving connection %p", c);

    //���free�����п���cache itemsΪ�գ����cache����ȡһ���������ʹ��item��
    //����item��Ӧ���Ǹ����ӹرգ���item���ڱ��浱ǰ��Ҫ�ͷŵ�����
    if (ngx_queue_empty(&kp->conf->free)) { //free���Ѿ�û�нڵ���Ϣ�ˣ���˰�cache������ʹ�õ��Ǹ�ȡ�������Ѹ����ӹرգ�������ӵ�cache

        q = ngx_queue_last(&kp->conf->cache);
        ngx_queue_remove(q);

        item = ngx_queue_data(q, ngx_http_upstream_keepalive_cache_t, queue);

        ngx_http_upstream_keepalive_close(item->connection);

    } else {
         //free���в�����ֱ�ӴӶ���ͷȡһ��item���ڱ��浱ǰ����
        q = ngx_queue_head(&kp->conf->free);
        ngx_queue_remove(q);

        item = ngx_queue_data(q, ngx_http_upstream_keepalive_cache_t, queue);
    }

    ngx_queue_insert_head(&kp->conf->cache, q);
    
    //���浱ǰ���ӣ���item����cache���У�Ȼ��pc->connection�ÿգ���ֹ�ϲ����
    //ngx_http_upstream_finalize_request�رո����ӣ�����ú�����
    item->connection = c;

    pc->connection = NULL;

    //�رն�д��ʱ�����������Ա���Ѻ�˷�������tcp���ӹرյ�
    if (c->read->timer_set) {
        ngx_del_timer(c->read, NGX_FUNC_LINE);
    }
    if (c->write->timer_set) {
        ngx_del_timer(c->write, NGX_FUNC_LINE);
    }

    
    //�������Ӷ�д���ӡ�д������һ���ٹ��ӣ�keepalive���Ӳ����ɿͻ��������رգ�
    //�����Ӵ���ر�keepalive���ӵĲ��������յ����Ժ��web��������FIN�ֽڣ�
    c->write->handler = ngx_http_upstream_keepalive_dummy_handler;
    c->read->handler = ngx_http_upstream_keepalive_close_handler;

    c->data = item;
    c->idle = 1;
    c->log = ngx_cycle->log;
    c->read->log = ngx_cycle->log;
    c->write->log = ngx_cycle->log;
    c->pool->log = ngx_cycle->log;

    // ����socket��ַ�����Ϣ����������ͨ��������ͬ��socket��ַ�����ø�����
    item->socklen = pc->socklen;
    ngx_memcpy(&item->sockaddr, pc->sockaddr, pc->socklen);

    if (c->read->ready) {
        ngx_http_upstream_keepalive_close_handler(c->read);
    }

invalid:

    kp->original_free_peer(pc, kp->data, state); //ָ��ԭ���ؾ����㷨��Ӧ��free
}


static void
ngx_http_upstream_keepalive_dummy_handler(ngx_event_t *ev)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ev->log, 0,
                   "keepalive dummy handler");
}


static void
ngx_http_upstream_keepalive_close_handler(ngx_event_t *ev)
{
    ngx_http_upstream_keepalive_srv_conf_t  *conf;
    ngx_http_upstream_keepalive_cache_t     *item;

    int                n;
    char               buf[1];
    ngx_connection_t  *c;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ev->log, 0,
                   "keepalive close handler");

    c = ev->data;

    if (c->close) {
        goto close;
    }

    n = recv(c->fd, buf, 1, MSG_PEEK);

    if (n == -1 && ngx_socket_errno == NGX_EAGAIN) {
        ev->ready = 0;

        if (ngx_handle_read_event(c->read, 0, NGX_FUNC_LINE) != NGX_OK) {
            goto close;
        }

        return;
    }

close:

    item = c->data;
    conf = item->conf;

    ngx_http_upstream_keepalive_close(c);

    ngx_queue_remove(&item->queue);
    ngx_queue_insert_head(&conf->free, &item->queue);
}


static void
ngx_http_upstream_keepalive_close(ngx_connection_t *c)
{

#if (NGX_HTTP_SSL)

    if (c->ssl) {
        c->ssl->no_wait_shutdown = 1;
        c->ssl->no_send_shutdown = 1;

        if (ngx_ssl_shutdown(c) == NGX_AGAIN) {
            c->ssl->handler = ngx_http_upstream_keepalive_close;
            return;
        }
    }

#endif

    ngx_destroy_pool(c->pool);
    ngx_close_connection(c);
}


#if (NGX_HTTP_SSL)

static ngx_int_t
ngx_http_upstream_keepalive_set_session(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_keepalive_peer_data_t  *kp = data;

    return kp->original_set_session(pc, kp->data);
}


static void
ngx_http_upstream_keepalive_save_session(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_keepalive_peer_data_t  *kp = data;

    kp->original_save_session(pc, kp->data);
    return;
}

#endif


static void *
ngx_http_upstream_keepalive_create_conf(ngx_conf_t *cf)
{
    ngx_http_upstream_keepalive_srv_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool,
                       sizeof(ngx_http_upstream_keepalive_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->original_init_upstream = NULL;
     *     conf->original_init_peer = NULL;
     *     conf->max_cached = 0;
     */

    return conf;
}

//keepalive connections
static char *
ngx_http_upstream_keepalive(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t            *uscf;
    ngx_http_upstream_keepalive_srv_conf_t  *kcf = conf;

    ngx_int_t    n;
    ngx_str_t   *value;

    if (kcf->max_cached) {
        return "is duplicate";
    }

    /* read options */

    value = cf->args->elts;

    n = ngx_atoi(value[1].data, value[1].len);

    if (n == NGX_ERROR || n == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid value \"%V\" in \"%V\" directive",
                           &value[1], &cmd->name);
        return NGX_CONF_ERROR;
    }

    kcf->max_cached = n;

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    /*
    When using load balancer methods other than the default round-robin method, it is necessary to activate them before the keepalive directive.
     */

    /*
    &ngx_http_upstream_hash_module,
    &ngx_http_upstream_ip_hash_module,
    &ngx_http_upstream_least_conn_module,
    &ngx_http_upstream_keepalive_module,  keepalive module�����hash  ip_hash least_connʹ�õģ���������Щģ��֮��original_init_upstreamָ���⼸��ģ���init_upstream
    */
  
    /* ����ԭ���ĳ�ʼ��upstream�Ĺ��ӣ��������µĹ��� */ //����Ǹ�ֵ�����㷨��һЩ��ʼ��������original_init_upstream����
    kcf->original_init_upstream = uscf->peer.init_upstream
                                  ? uscf->peer.init_upstream
                                  : ngx_http_upstream_init_round_robin;

    uscf->peer.init_upstream = ngx_http_upstream_init_keepalive; //ԭʼ�ĸ�ծ���⹳�ӱ�����kcf->original_init_upstream

    return NGX_CONF_OK;
}
