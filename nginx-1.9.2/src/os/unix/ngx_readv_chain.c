
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


ssize_t
ngx_readv_chain(ngx_connection_t *c, ngx_chain_t *chain, off_t limit)
{//?????readv???????????chain????????????error??eof?
//??????????
    u_char        *prev;
    ssize_t        n, size;
    ngx_err_t      err;
    ngx_array_t    vec;
    ngx_event_t   *rev;
    struct iovec  *iov, iovs[NGX_IOVS_PREALLOCATE];//16??

    rev = c->read;

#if (NGX_HAVE_KQUEUE)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "readv: eof:%d, avail:%d, err:%d",
                       rev->pending_eof, rev->available, rev->kq_errno);

        if (rev->available == 0) {
            if (rev->pending_eof) {
                rev->ready = 0;
                rev->eof = 1;

                ngx_log_error(NGX_LOG_INFO, c->log, rev->kq_errno,
                              "kevent() reported about an closed connection");

                if (rev->kq_errno) {
                    rev->error = 1;
                    ngx_set_socket_errno(rev->kq_errno);
                    return NGX_ERROR;
                }

                return 0;

            } else {
                return NGX_AGAIN;
            }
        }
    }

#endif

    prev = NULL;
    iov = NULL;
    size = 0;

    vec.elts = iovs; //vec?????NGX_IOVS_PREALLOCATE?struct iovec??
    vec.nelts = 0;
    vec.size = sizeof(struct iovec);
    vec.nalloc = NGX_IOVS_PREALLOCATE;
    vec.pool = c->pool;

    /* coalesce the neighbouring bufs */

    while (chain) {//??chain??????????struct iovec??????readv????????2?????????????????
        n = chain->buf->end - chain->buf->last; //?chain->buf????????????

        if (limit) {
            if (size >= limit) {
                break;
            }

            if (size + n > limit) {
                n = (ssize_t) (limit - size);
            }
        }

        if (prev == chain->buf->last) { //??????chain?end?????chain?last???????????chain?????? ??2?????????????????
            iov->iov_len += n;

        } else {
            if (vec.nelts >= IOV_MAX) {
                break;
            }

            iov = ngx_array_push(&vec);
            if (iov == NULL) {
                return NGX_ERROR;
            }

            //???????????????????????????????????????????????????????????????
            iov->iov_base = (void *) chain->buf->last;
            iov->iov_len = n;//????????????
        }

        size += n;
        prev = chain->buf->end;
        chain = chain->next;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "readv: %d, last(iov_len):%d", vec.nelts, iov->iov_len);

    do {
        //read??????0???????FIN?
		//If any portion of a regular file prior to the end-of-file has not been written, read() shall return bytes with value 0.
		//??????????????-1???errno?EAGAIN?????????
		//???????readv?????????????????????????read????????????
        /* On success, the readv() function returns the number of bytes read; the writev() function returns the number of bytes written.
        On error, -1 is returned, and errno is  set appropriately. readv????????????????????????????0???? */
        n = readv(c->fd, (struct iovec *) vec.elts, vec.nelts);

        if (n >= 0) {

#if (NGX_HAVE_KQUEUE)

            if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
                rev->available -= n;

                /*
                 * rev->available may be negative here because some additional
                 * bytes may be received between kevent() and recv()
                 */

                if (rev->available <= 0) {
                    if (!rev->pending_eof) {
                        rev->ready = 0;
                    }

                    if (rev->available < 0) {
                        rev->available = 0;
                    }
                }

                if (n == 0) {//readv??0?????????????????

                    /*
                     * on FreeBSD recv() may return 0 on closed socket
                     * even if kqueue reported about available data
                     */

#if 0
                    ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                                  "readv() returned 0 while kevent() reported "
                                  "%d available bytes", rev->available);
#endif

                    rev->ready = 0;
                    rev->eof = 1;
                    rev->available = 0;
                }

                return n;
            }

#endif /* NGX_HAVE_KQUEUE */

            if (n < size && !(ngx_event_flags & NGX_USE_GREEDY_EVENT)) {
                rev->ready = 0; //??????????????????????????  epoll?????if??
            }

            if (n == 0) {//??readv????????????????????? readv????????????????????????????0????

                rev->eof = 1; //???????????0??????????
            }

            return n; //??epoll????????????readv?1
        }

        //??n<0   On error, -1 is returned, and errno is  set appropriately

        err = ngx_socket_errno;

        //readv??-1?????EAGAIN????? ????????????????readv?????NGX_EAGAIN
        if (err == NGX_EAGAIN || err == NGX_EINTR) {
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "readv() not ready");
            n = NGX_AGAIN;

        } else {
            n = ngx_connection_error(c, err, "readv() failed");
            break;
        }

    } while (err == NGX_EINTR);

    rev->ready = 0;//?????

    if (n == NGX_ERROR) {
        c->read->error = 1;//????????
    }

    return n;
}
