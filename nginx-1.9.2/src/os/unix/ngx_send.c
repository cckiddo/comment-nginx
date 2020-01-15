
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


ssize_t
ngx_unix_send(ngx_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    ngx_err_t     err;
    ngx_event_t  *wev;
    int ready = 0;

    wev = c->write;

#if (NGX_HAVE_KQUEUE)

    if ((ngx_event_flags & NGX_USE_KQUEUE_EVENT) && wev->pending_eof) {
        (void) ngx_connection_error(c, wev->kq_errno,
                               "kevent() reported about an closed connection");
        wev->error = 1;
        return NGX_ERROR;
    }

#endif

    for ( ;; ) {
        n = send(c->fd, buf, size, 0);

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "send: fd:%d %d of %d", c->fd, n, size);

        if (n > 0) {
            //�ڴ�����1000�ֽڣ�ʵ���Ϸ���500�ֽڣ�˵���ں˻��������յ���500�ֽں��Ѿ����ˣ�������д, readΪ0��ֻ�е�epollд�¼����� read
            //���ǣ���������ڴ�����1000�ֽڣ�����500�ֽ���˵�����ں˻�������ֻ��500�ֽڣ���˿��Լ���recv��ready����Ϊ1
            if (n < (ssize_t) size) { //˵��������n�ֽڵ��������󣬻���������
                wev->ready = 0;
            }

            c->sent += n;

            ready = wev->ready;
            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "send, ready:%d", ready);
            return n;
        }

        err = ngx_socket_errno;

        if (n == 0) { //recv����0����ʾ���ӶϿ���send����0���������������
            ngx_log_error(NGX_LOG_ALERT, c->log, err, "send() returned zero, ready:0");
            wev->ready = 0;
            return n;
        }

        if (err == NGX_EAGAIN || err == NGX_EINTR) {
            wev->ready = 0;

            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "send() not ready, ready:0");

            if (err == NGX_EAGAIN) { //�ں˻���������
                return NGX_AGAIN;
            }

        } else {
            wev->error = 1;
            (void) ngx_connection_error(c, err, "send() failed");
            return NGX_ERROR;
        }
    }
}

