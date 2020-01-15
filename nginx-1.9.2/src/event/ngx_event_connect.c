
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_connect.h>

ngx_int_t
ngx_event_connect_peer(ngx_peer_connection_t *pc)
{
    int                rc;
    ngx_int_t          event;
    ngx_err_t          err;
    ngx_uint_t         level;
    ngx_socket_t       s;
    ngx_event_t       *rev, *wev;
    ngx_connection_t  *c;

    /*ngx_http_upstream_get_round_robin_peer ngx_http_upstream_get_least_conn_peer ngx_http_upstream_get_hash_peer  
    ngx_http_upstream_get_ip_hash_peer ngx_http_upstream_get_keepalive_peer�� */
    rc = pc->get(pc, pc->data);//ngx_http_upstream_get_round_robin_peer��ȡһ��peer
    if (rc != NGX_OK) {/* ˵����˷�����ȫ��down�����������õ���keepalive��ʽ��ֱ��ѡ��ĳ��peer�������еĳ��������������ݣ���ֱ�Ӵ����ﷵ�� */
        return rc;
    }

    s = ngx_socket(pc->sockaddr->sa_family, SOCK_STREAM, 0);

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, pc->log, 0, "socket %d", s);

    if (s == (ngx_socket_t) -1) {
        ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno,
                      ngx_socket_n " failed");
        return NGX_ERROR;
    }

    /*
     ����Nginx���¼����Ҫ��ÿ�����Ӷ���һ��ngx_connection-t�ṹ�������أ������һ��������ngx_get_connection��������ngx_cycle_t
 ���Ľṹ����free_connectionsָ��Ŀ������ӳش���ȡ��һ��ngx_connection_t�ṹ�壬��Ϊ����Nginx�����η��������TCP����
     */
    c = ngx_get_connection(s, pc->log);

    if (c == NULL) {
        if (ngx_close_socket(s) == -1) {
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno,
                          ngx_close_socket_n "failed");
        }

        return NGX_ERROR;
    }

    if (pc->rcvbuf) {
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                       (const void *) &pc->rcvbuf, sizeof(int)) == -1)
        {
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno,
                          "setsockopt(SO_RCVBUF) failed");
            goto failed;
        }
    }

    if (ngx_nonblocking(s) == -1) { //����һ��TCP�׽��֣�ͬʱ������׽�����Ҫ����Ϊ������ģʽ��
        ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno,
                      ngx_nonblocking_n " failed");

        goto failed;
    }

    if (pc->local) {
        if (bind(s, pc->local->sockaddr, pc->local->socklen) == -1) {
            ngx_log_error(NGX_LOG_CRIT, pc->log, ngx_socket_errno,
                          "bind(%V) failed", &pc->local->name);

            goto failed;
        }
    }

    c->recv = ngx_recv;
    c->send = ngx_send;
    c->recv_chain = ngx_recv_chain;
    c->send_chain = ngx_send_chain;

    /*
       �ͺ�˵�ngx_connection_t��ngx_event_connect_peer������Ϊ1������ngx_http_upstream_connect��c->sendfile &= r->connection->sendfile;��
       �Ϳͻ����������ngx_connextion_t��sendfile��Ҫ��ngx_http_update_location_config���жϣ�������������Ƿ���configure��ʱ���Ƿ��м�
       sendfileѡ������������1������0
    */
    c->sendfile = 1;

    c->log_error = pc->log_error;

    if (pc->sockaddr->sa_family == AF_UNIX) {
        c->tcp_nopush = NGX_TCP_NOPUSH_DISABLED;
        c->tcp_nodelay = NGX_TCP_NODELAY_DISABLED;

#if (NGX_SOLARIS)
        /* Solaris's sendfilev() supports AF_NCA, AF_INET, and AF_INET6 */
        c->sendfile = 0;
#endif
    }

    rev = c->read;
    wev = c->write;

    rev->log = pc->log;
    wev->log = pc->log;

    pc->connection = c;

    c->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

    /*
     �¼�ģ���ngx_event_actions�ӿڣ����е�add_conn�������Խ�TCP�׽������ڴ��ɶ�����д�¼��ķ�ʽ��ӵ��¼��Ѽ����С�����
     epoll�¼�ģ����˵��add_conn�������ǰ��׽������ڴ�EPOLLIN EPOLLOUT�¼��ķ�ʽ����epoll�У���һ��������add_conn�����Ѹո�
     �������׽�����ӵ�epoll�У���ʾ�������׽����ϳ�����Ԥ�ڵ������¼�����ϣ��epoll�ܹ��ص�����handler������
     */
    if (ngx_add_conn) {
        /*
        ���������ngx_connection t�ϵĶ���д�¼���handler�ص�����������Ϊngx_http_upstream_handler��������������ngx_http_upstream_connect
         */
        if (ngx_add_conn(c) == NGX_ERROR) {
            goto failed;
        }
    }

    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, pc->log, 0,
                   "connect to %V, fd:%d #%uA", pc->name, s, c->number);

    /*
     ����connect���������η���������TCP���ӣ���Ϊ�������׽��֣�connect�����������̷������ӽ����ɹ���Ҳ���ܸ����û������ȴ����η���������Ӧ
     ��connect�����Ƿ����ɹ��ļ����ں��������u->read_event_handler = ngx_http_upstream_process_header;����������ngx_http_upstream_connect
     */
     /*
        ��Է�����I/Oִ�е�ϵͳ�����������������أ��������¼�����Ѿ�����������¼�û��������������Щϵͳ���þ�
    ���ء�1���ͳ�������һ������ʱ���Ǳ������errno�������������������accept��send��recv���ԣ��¼�δ��ţʱerrno
    ͨ�������ó�EAGAIN����Ϊ������һ�Ρ�������EWOULDBLOCK����Ϊ���ڴ�������������conncct���ԣ�errno��
    ���ó�EINPROGRESS����Ϊ���ڴ�����"����
      */ //connect��ʱ�򷵻سɹ���ʹ�õ�sock����socket������sock����ͷ�������accept�ɹ�����һ���µ�sock��һ��
      //�����ngx_add_conn�Ѿ��Ѷ�д�¼�һ����ӵ���epoll��
    rc = connect(s, pc->sockaddr, pc->socklen); //connect����ֵ���Բο�<linux�����ܷ���������> 9.5��

    if (rc == -1) {
        err = ngx_socket_errno;


        if (err != NGX_EINPROGRESS
#if (NGX_WIN32)
            /* Winsock returns WSAEWOULDBLOCK (NGX_EAGAIN) */
            && err != NGX_EAGAIN
#endif
            )
        {
            if (err == NGX_ECONNREFUSED
#if (NGX_LINUX)
                /*
                 * Linux returns EAGAIN instead of ECONNREFUSED
                 * for unix sockets if listen queue is full
                 */
                || err == NGX_EAGAIN
#endif
                || err == NGX_ECONNRESET
                || err == NGX_ENETDOWN
                || err == NGX_ENETUNREACH
                || err == NGX_EHOSTDOWN
                || err == NGX_EHOSTUNREACH)
            {
                level = NGX_LOG_ERR;

            } else {
                level = NGX_LOG_CRIT;
            }

            ngx_log_error(level, c->log, err, "connect() to %V failed",
                          pc->name);

            ngx_close_connection(c);
            pc->connection = NULL;

            return NGX_DECLINED;
        }
    }

    if (ngx_add_conn) {
        if (rc == -1) {
        //�����ʾ�������������������е�SYN������û�еȴ��Է���ȫӦ�������ʾ���ӳɹ�ͨ������
        //c->write->handler = ngx_http_upstream_handler;  u->write_event_handler = ngx_http_upstream_send_request_handler�ٷ����سɹ�
            /* NGX_EINPROGRESS */

            return NGX_AGAIN;
        }

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, "connected");

        wev->ready = 1;

        return NGX_OK;  
    }

    if (ngx_event_flags & NGX_USE_IOCP_EVENT) {

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, pc->log, ngx_socket_errno,
                       "connect(): %d", rc);

        if (ngx_blocking(s) == -1) {
            ngx_log_error(NGX_LOG_ALERT, pc->log, ngx_socket_errno,
                          ngx_blocking_n " failed");
            goto failed;
        }

        /*
         * FreeBSD's aio allows to post an operation on non-connected socket.
         * NT does not support it.
         *
         * TODO: check in Win32, etc. As workaround we can use NGX_ONESHOT_EVENT
         */

        rev->ready = 1;
        wev->ready = 1;

        return NGX_OK;
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {

        /* kqueue */

        event = NGX_CLEAR_EVENT;

    } else {

        /* select, poll, /dev/poll */

        event = NGX_LEVEL_EVENT;
    }

    char tmpbuf[256];
        
        snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_READ_EVENT(et) read add", NGX_FUNC_LINE);
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, tmpbuf);
    if (ngx_add_event(rev, NGX_READ_EVENT, event) != NGX_OK) {
        goto failed;
    }

    if (rc == -1) {

        /* NGX_EINPROGRESS */
        char tmpbuf[256];
        
        snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_WRITE_EVENT(et) read add", NGX_FUNC_LINE);
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, tmpbuf);
        if (ngx_add_event(wev, NGX_WRITE_EVENT, event) != NGX_OK) {
            goto failed;
        }

        return NGX_AGAIN; //�����ʾ�������������������е�SYN������û�еȴ��Է���ȫӦ�������ʾ���ӳɹ�
        //ͨ������ c->write->handler = ngx_http_upstream_handler;  u->write_event_handler = ngx_http_upstream_send_request_handler�ٷ����سɹ�
    }

    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, pc->log, 0, "connected");

    wev->ready = 1;

    return NGX_OK;

failed:

    ngx_close_connection(c);
    pc->connection = NULL;

    return NGX_ERROR;
}


ngx_int_t
ngx_event_get_peer(ngx_peer_connection_t *pc, void *data)
{
    return NGX_OK;
}
