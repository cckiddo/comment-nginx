
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


static ngx_int_t ngx_enable_accept_events(ngx_cycle_t *cycle);
static ngx_int_t ngx_disable_accept_events(ngx_cycle_t *cycle, ngx_uint_t all);
static void ngx_close_accepted_connection(ngx_connection_t *c);

/*
��ν���������
    �������ٹ��������������¼��Ļص�������ngx_event_accept����ԭ�����¡�void ngx_event_accept (ngx_event_t��ev)
    ����򵥽���һ���������̣���ͼ9-6��ʾ��
    ����������е�7���������˵����
    1)���ȵ���accept������ͼ���������ӣ����û��׼���õ��������¼���ngx_event_accept������ֱ�ӷ��ء�
    2)���ø��ؾ�����ֵngx_accept_disabled�������ֵ�ǽ������������������1/8��ȥ������������
    3)����ngx_get_connection���������ӳ��л�ȡһ��ngx_connection_t���Ӷ���
    4)Ϊngx_connection_t�е�poolָ�뽨���ڴ�ء�����������ͷŵ��������ӳ�ʱ���ͷ�pool�ڴ�ء�
    5)�����׽��ֵ����ԣ�����Ϊ�������׽��֡�
    6)����������Ӷ�Ӧ�Ķ��¼���ӵ�epoll���¼�����ģ���У������������������������յ��û�����epoll_wait���ͻ��ռ�������¼���
    7)���ü�������ngx_listening_t�е�handler�ص�������ngx_listening_t�ṹٺ��handler�ص��������ǵ��µ�TCP���Ӹոս������ʱ��������õġ�
    �����������¼���available��־λΪ1���ٴ�ѭ������1��������ngx_event_accept�����������¼���available��־λ��Ӧ��multi_accept����
    ���availableΪlʱ������Nginx -���Ծ�����ؽ��������ӣ�����ʵ��ԭ��Ҳ��������
*/
//�����event����ngx_event_process_init�д����ӳ��л�ȡ�� ngx_connection_t�е�->read���¼�
//accept����ngx_event_process_init(�����̻��߲����ø��ؾ����ʱ��)����(����̣����ø��ؾ���)��ʱ���accept�¼���ӵ�epoll��
void //���β��е�ngx_connection_t(ngx_event_t)��Ϊaccept�¼�����׼���Ŀռ䣬��accept���سɹ��󣬻����»�ȡһ��ngx_connection_t(ngx_event_t)������д������
ngx_event_accept(ngx_event_t *ev) //��ngx_process_events_and_timers��ִ��              
{ //һ��accept�¼���Ӧһ��ev���統ǰһ����4���ͻ���accept��Ӧ�ö�Ӧ4��ev�¼���һ�������accept�Ĵ����������do {}while��ʵ��
    socklen_t          socklen;
    ngx_err_t          err;
    ngx_log_t         *log;
    ngx_uint_t         level;
    ngx_socket_t       s;

//������ļ��첽i/o�е�ngx_event_aio_t����������ngx_event_aio_t->ngx_event_t(ֻ�ж�),����������¼��е�event,��Ϊngx_connection_s�е�event(��������д)
    ngx_event_t       *rev, *wev; 
    ngx_listening_t   *ls;
    ngx_connection_t  *c, *lc;
    ngx_event_conf_t  *ecf;
    u_char             sa[NGX_SOCKADDRLEN];
#if (NGX_HAVE_ACCEPT4)
    static ngx_uint_t  use_accept4 = 1;
#endif

    if (ev->timedout) {
        if (ngx_enable_accept_events((ngx_cycle_t *) ngx_cycle) != NGX_OK) {
            return;
        }

        ev->timedout = 0;
    }

    ecf = ngx_event_get_conf(ngx_cycle->conf_ctx, ngx_event_core_module);

    if (!(ngx_event_flags & NGX_USE_KQUEUE_EVENT)) {
        ev->available = ecf->multi_accept;   
    }

    lc = ev->data;
    ls = lc->listening;
    ev->ready = 0;

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "accept on %V, ready: %d", &ls->addr_text, ev->available);

    do { /* �����һ�ζ�ȡһ��accept�¼��Ļ���ѭ����ִֻ��һ�Σ� �����һ���Կ��Զ�ȡ���е�accept�¼��������ѭ����ִ�д���Ϊaccept�¼���*/
        socklen = NGX_SOCKADDRLEN;

#if (NGX_HAVE_ACCEPT4) //ngx_close_socket���Թر��׽���
        if (use_accept4) {
            s = accept4(lc->fd, (struct sockaddr *) sa, &socklen,
                        SOCK_NONBLOCK);
        } else {
            s = accept(lc->fd, (struct sockaddr *) sa, &socklen);
        }
#else
    /*
            ��Է�����I/Oִ�е�ϵͳ�����������������أ��������¼�����Ѿ�����������¼�û��������������Щϵͳ���þ�
        ���ء�1���ͳ�������һ������ʱ���Ǳ������errno�������������������accept��send��recv���ԣ��¼�δ��ţʱerrno
        ͨ�������ó�EAGAIN����Ϊ������һ�Ρ�������EWOULDBLOCK����Ϊ���ڴ�������������conncct���ԣ�errno��
        ���ó�EINPROGRESS����Ϊ���ڴ�����"����
          */
        s = accept(lc->fd, (struct sockaddr *) sa, &socklen);
#endif

        if (s == (ngx_socket_t) -1) {
            err = ngx_socket_errno;

            /* ���Ҫȥһ���Զ�ȡ���е�accept��Ϣ������ȡ��Ϻ�ͨ�����ﷵ�ء����е�accept�¼�����ȡ��� */
            if (err == NGX_EAGAIN) { //���event{}����multi_accept������accept���listen ip:port��Ӧ��ip�Ͷ˿����Ӻ󣬻�ͨ�����ﷵ��
                ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, err,
                               "accept() not ready");
                return;
            }

            level = NGX_LOG_ALERT;

            if (err == NGX_ECONNABORTED) {
                level = NGX_LOG_ERR;

            } else if (err == NGX_EMFILE || err == NGX_ENFILE) {
                level = NGX_LOG_CRIT;
            }

#if (NGX_HAVE_ACCEPT4)
            ngx_log_error(level, ev->log, err,
                          use_accept4 ? "accept4() failed" : "accept() failed");

            if (use_accept4 && err == NGX_ENOSYS) {
                use_accept4 = 0;
                ngx_inherited_nonblocking = 0;
                continue;
            }
#else
            ngx_log_error(level, ev->log, err, "accept() failed");
#endif

            if (err == NGX_ECONNABORTED) {
                if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
                    ev->available--;
                }

                if (ev->available) {
                    continue;
                }
            }

            if (err == NGX_EMFILE || err == NGX_ENFILE) {
                if (ngx_disable_accept_events((ngx_cycle_t *) ngx_cycle, 1)
                    != NGX_OK)
                {
                    return;
                }

                if (ngx_use_accept_mutex) {
                    if (ngx_accept_mutex_held) {
                        ngx_shmtx_unlock(&ngx_accept_mutex);
                        ngx_accept_mutex_held = 0;
                    }
//��ǰ��������accpetʧ�ܣ��������ʱ����Ϊ1���´�����ʱ�����������̾���accpet�������´θý��̼���������accept����Ϊ���´ε�ʱ��ngx_process_events_and_timers
//ngx_accept_disabled = 1; ��ȥ1��Ϊ0�����Լ�������
                    ngx_accept_disabled = 1; 
                } else { ////����ǲ���Ҫʵ�ָ��ؾ��⣬��ɨβ��ʱ�¼�����ngx_process_events_and_timers��accept
                    ngx_add_timer(ev, ecf->accept_mutex_delay, NGX_FUNC_LINE);
                }
            }

            return;
        }

#if (NGX_STAT_STUB)
        (void) ngx_atomic_fetch_add(ngx_stat_accepted, 1);
#endif
        //���ø��ؾ��ֵⷧ �ʼfree_connection_n=connection_n����ngx_event_process_init
        ngx_accept_disabled = ngx_cycle->connection_n / 8
                              - ngx_cycle->free_connection_n; //�жϿ������ӵ���Ŀ������Ŀ�İ˷�֮һ��С��������õ�С�ڰ˷�֮һ��Ϊ��

        //�ڷ�������accept�ͻ������ӳɹ�(ngx_event_accept)�󣬻�ͨ��ngx_get_connection�����ӳػ�ȡһ��ngx_connection_t�ṹ��Ҳ����ÿ���ͻ������Ӷ���һ��ngx_connection_t�ṹ��
        //����Ϊ�����һ��ngx_http_connection_t�ṹ��ngx_connection_t->data = ngx_http_connection_t����ngx_http_init_connection

        //�����ӳ��л�ȡһ������ngx_connection_t�����ڿͻ������ӽ����ɹ���������Ӷ�д���ݣ������β��е�ngx_event_t��Ӧ����Ϊaccept�¼���Ӧ��
        //ngx_connection_t�ж�Ӧ��event
        c = ngx_get_connection(s, ev->log);  //ngx_get_connection��c->fd = s;
        //ע�⣬�����ngx_connection_t�Ǵ����ӳ��д��»�ȡ�ģ���ngx_epoll_process_events�е�ngx_connection_t��������ͬ�ġ�

        if (c == NULL) {
            if (ngx_close_socket(s) == -1) {
                ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_socket_errno,
                              ngx_close_socket_n " failed");
            }

            return;
        }

#if (NGX_STAT_STUB)
        (void) ngx_atomic_fetch_add(ngx_stat_active, 1);
#endif

        c->pool = ngx_create_pool(ls->pool_size, ev->log);
        if (c->pool == NULL) {
            ngx_close_accepted_connection(c);
            return;
        }

        c->sockaddr = ngx_palloc(c->pool, socklen);
        if (c->sockaddr == NULL) {
            ngx_close_accepted_connection(c);
            return;
        }

        ngx_memcpy(c->sockaddr, sa, socklen);

        log = ngx_palloc(c->pool, sizeof(ngx_log_t));
        if (log == NULL) {
            ngx_close_accepted_connection(c);
            return;
        }

        /* set a blocking mode for iocp and non-blocking mode for others */

        if (ngx_inherited_nonblocking) {
            if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
                if (ngx_blocking(s) == -1) {
                    ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_socket_errno,
                                  ngx_blocking_n " failed");
                    ngx_close_accepted_connection(c);
                    return;
                }
            }

        } else {
            if (!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
                if (ngx_nonblocking(s) == -1) {
                    ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_socket_errno,
                                  ngx_nonblocking_n " failed");
                    ngx_close_accepted_connection(c);
                    return;
                }
            }
        }

        *log = ls->log;

        c->recv = ngx_recv;
        c->send = ngx_send;
        c->recv_chain = ngx_recv_chain;
        c->send_chain = ngx_send_chain;

        c->log = log;
        c->pool->log = log;

        c->socklen = socklen;
        c->listening = ls;
        c->local_sockaddr = ls->sockaddr;
        c->local_socklen = ls->socklen;

        c->unexpected_eof = 1;

#if (NGX_HAVE_UNIX_DOMAIN)
        if (c->sockaddr->sa_family == AF_UNIX) {
            c->tcp_nopush = NGX_TCP_NOPUSH_DISABLED;
            c->tcp_nodelay = NGX_TCP_NODELAY_DISABLED;
#if (NGX_SOLARIS)
            /* Solaris's sendfilev() supports AF_NCA, AF_INET, and AF_INET6 */
            c->sendfile = 0;
#endif
        }
#endif
        //ע�⣬�����ngx_connection_t�Ǵ����ӳ��д��»�ȡ�ģ���ngx_epoll_process_events�е�ngx_connection_t��������ͬ�ġ�
        rev = c->read; 
        wev = c->write;

        wev->ready = 1;

        if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
            rev->ready = 1;
        }

        if (ev->deferred_accept) {
            rev->ready = 1;
#if (NGX_HAVE_KQUEUE)
            rev->available = 1;
#endif
        }

        rev->log = log;
        wev->log = log;

        /*
         * TODO: MT: - ngx_atomic_fetch_add()
         *             or protection by critical section or light mutex
         *
         * TODO: MP: - allocated in a shared memory
         *           - ngx_atomic_fetch_add()
         *             or protection by critical section or light mutex
         */

        c->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

#if (NGX_STAT_STUB)
        (void) ngx_atomic_fetch_add(ngx_stat_handled, 1);
#endif

        if (ls->addr_ntop) {
            c->addr_text.data = ngx_pnalloc(c->pool, ls->addr_text_max_len);
            if (c->addr_text.data == NULL) {
                ngx_close_accepted_connection(c);
                return;
            }

            c->addr_text.len = ngx_sock_ntop(c->sockaddr, c->socklen,
                                             c->addr_text.data,
                                             ls->addr_text_max_len, 0);
            if (c->addr_text.len == 0) {
                ngx_close_accepted_connection(c);
                return;
            }
        }

#if (NGX_DEBUG)
        {

        ngx_str_t             addr;
        struct sockaddr_in   *sin;
        ngx_cidr_t           *cidr;
        ngx_uint_t            i;
        u_char                text[NGX_SOCKADDR_STRLEN];
#if (NGX_HAVE_INET6)
        struct sockaddr_in6  *sin6;
        ngx_uint_t            n;
#endif

        cidr = ecf->debug_connection.elts;
        for (i = 0; i < ecf->debug_connection.nelts; i++) {
            if (cidr[i].family != (ngx_uint_t) c->sockaddr->sa_family) {
                goto next;
            }

            switch (cidr[i].family) {

#if (NGX_HAVE_INET6)
            case AF_INET6:
                sin6 = (struct sockaddr_in6 *) c->sockaddr;
                for (n = 0; n < 16; n++) {
                    if ((sin6->sin6_addr.s6_addr[n]
                        & cidr[i].u.in6.mask.s6_addr[n])
                        != cidr[i].u.in6.addr.s6_addr[n])
                    {
                        goto next;
                    }
                }
                break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
            case AF_UNIX:
                break;
#endif

            default: /* AF_INET */
                sin = (struct sockaddr_in *) c->sockaddr;
                if ((sin->sin_addr.s_addr & cidr[i].u.in.mask)
                    != cidr[i].u.in.addr)
                {
                    goto next;
                }
                break;
            }

            log->log_level = NGX_LOG_DEBUG_CONNECTION|NGX_LOG_DEBUG_ALL;
            break;

        next:
            continue;
        }

        if (log->log_level & NGX_LOG_DEBUG_EVENT) {
            addr.data = text;
            addr.len = ngx_sock_ntop(c->sockaddr, c->socklen, text,
                                     NGX_SOCKADDR_STRLEN, 1);

            ngx_log_debug3(NGX_LOG_DEBUG_EVENT, log, 0,
                           "*%uA accept: %V fd:%d", c->number, &addr, s);
        }

        }
#endif

        if (ngx_add_conn && (ngx_event_flags & NGX_USE_EPOLL_EVENT) == 0) { //�����epoll,�����ߵ�������ȥ
            if (ngx_add_conn(c) == NGX_ERROR) {
                ngx_close_accepted_connection(c);
                return;
            }
        }

        log->data = NULL;
        log->handler = NULL;

        ls->handler(c);//ngx_http_init_connection

        if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
            ev->available--;
        }

    } while (ev->available); //һ���Զ�ȡ���е�ǰ��accept��ֱ��accept����NGX_EAGAIN��Ȼ���˳�
}

/*
���accept�������worker����һ�����Եõ������������������������̣��������̷��أ���ȡ�ɹ��Ļ�ngx_accept_mutex_held����Ϊ1��
�õ�������ζ�ż���������ŵ������̵�epoll���ˣ����û���õ��������������ᱻ��epoll��ȡ���� 
*/ //���Ի�ȡ���������ȡ��������ô��Ҫ����ǰ�����˿�ȫ��ע�ᵽ��ǰworker���̵�epoll����ȥ   
ngx_int_t
ngx_trylock_accept_mutex(ngx_cycle_t *cycle)
{
    if (ngx_shmtx_trylock(&ngx_accept_mutex)) {//

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "accept mutex locked");

        //��������Ѿ����������ֱ�ӷ���Ok,
        //���˸ñ�ǣ���ʾ�ý����Ѿ���accept�¼���ӵ�epoll�¼������ˣ������ظ�ִ�к����ngx_enable_accept_events���ú�������ϵͳ���ù��̣�Ӱ������
        if (ngx_accept_mutex_held && ngx_accept_events == 0) {
            return NGX_OK;
        }

   //�������˵�����»�����ɹ��������Ҫ�򿪱��رյ�listening���������ngx_enable_accept_events�������������˿�ע�ᵽ��ǰworker���̵�epoll����ȥ   
        if (ngx_enable_accept_events(cycle) == NGX_ERROR) {
            ngx_shmtx_unlock(&ngx_accept_mutex);
            return NGX_ERROR;
        }

        ngx_accept_events = 0;
        ngx_accept_mutex_held = 1; ////��ʾ��ǰ��ȡ����   

        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "accept mutex lock failed: %ui", ngx_accept_mutex_held);

//�����ʾ������ǰ������ȡ�����������ȴ��ȡʧ���ˣ���ô��Ҫ�������˿ڴӵ�ǰ��worker���̵�epoll�����Ƴ������õ���ngx_disable_accept_events����   
    if (ngx_accept_mutex_held) {
        if (ngx_disable_accept_events(cycle, 0) == NGX_ERROR) {
            return NGX_ERROR;
        }

        ngx_accept_mutex_held = 0;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_enable_accept_events(ngx_cycle_t *cycle)
{
    ngx_uint_t         i;
    ngx_listening_t   *ls;
    ngx_connection_t  *c;

    ls = cycle->listening.elts;

    /*
    ע��:�����ѭ��������е�listening���뵽��epoll�У��ǲ���ÿ�����̶�������е�listening���뵽epoll���𣬲����о�Ⱥ����?
    ��:ʵ�����ڱ�����ngx_enable_accept_events������listen���뱾����epoll�к󣬱����̻�ȡ��ngx_accept_mutex������ִ��accept�¼���
    ��������������������Ҳ��ʼngx_trylock_accept_mutex�����֮ǰ�Ѿ���ȡ�������������е�listen��ӵ���epoll�У���ʱ����Ϊû����ȡ��
    accept��������֮ǰ���뵽�����̣���û��accept����ʱ��ȫ���������ngx_disable_accept_events���ʹ��
    ����ֻ��һ��������accept��ͬһ���ͻ�������
     */
    for (i = 0; i < cycle->listening.nelts; i++) { 

        c = ls[i].connection;

        //�����ngx_add_event->ngx_epoll_add_event�а�listening�е�c->read->active��1�� ngx_epoll_del_event�а�listening����read->active��0
        if (c == NULL || c->read->active) { //֮ǰ�������Ѿ���ӹ��������ټ���epoll�¼��У������ظ�
            continue;
        }

        char tmpbuf[256];
        
        snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_READ_EVENT(et) read add", NGX_FUNC_LINE);
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, tmpbuf);
        if (ngx_add_event(c->read, NGX_READ_EVENT, 0) == NGX_ERROR) { //ngx_epoll_add_event
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_disable_accept_events(ngx_cycle_t *cycle, ngx_uint_t all)
{
    ngx_uint_t         i;
    ngx_listening_t   *ls;
    ngx_connection_t  *c;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        c = ls[i].connection;

        if (c == NULL || !c->read->active) { //ngx_epoll_add_event����1
            continue;
        }

#if (NGX_HAVE_REUSEPORT)

        /*
         * do not disable accept on worker's own sockets
         * when disabling accept events due to accept mutex
         */

        if (ls[i].reuseport && !all) {
            continue;
        }

#endif

        if (ngx_del_event(c->read, NGX_READ_EVENT, NGX_DISABLE_EVENT) //ngx_epoll_del_event
            == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}


static void
ngx_close_accepted_connection(ngx_connection_t *c)
{
    ngx_socket_t  fd;

    ngx_free_connection(c);

    fd = c->fd;
    c->fd = (ngx_socket_t) -1;

    if (ngx_close_socket(fd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_socket_errno,
                      ngx_close_socket_n " failed");
    }

    if (c->pool) {
        ngx_destroy_pool(c->pool);
    }

#if (NGX_STAT_STUB)
    (void) ngx_atomic_fetch_add(ngx_stat_active, -1);
#endif
}


u_char *
ngx_accept_log_error(ngx_log_t *log, u_char *buf, size_t len)
{
    return ngx_snprintf(buf, len, " while accepting new connection on %V",
                        log->data);
}
