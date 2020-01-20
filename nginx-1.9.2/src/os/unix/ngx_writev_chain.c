
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

//ngx_linux_sendfile_chain��ngx_writev_chain
ngx_chain_t *
ngx_writev_chain(ngx_connection_t *c, ngx_chain_t *in, off_t limit)
{//����writevһ�η��Ͷ�������������û�з�����ϣ��򷵻�ʣ�µ����ӽṹͷ����
//ngx_chain_writer����������÷�ʽΪ ctx->out = c->send_chain(c, ctx->out, ctx->limit);
//�ڶ�������ΪҪ���͵�����
    ssize_t        n, sent;
    off_t          send, prev_send;
    ngx_chain_t   *cl;
    ngx_event_t   *wev;
    ngx_iovec_t    vec;
    struct iovec   iovs[NGX_IOVS_PREALLOCATE];

    wev = c->write;//�õ�������ӵ�д�¼��ṹ

    ngx_log_debugall(c->log, 0, "@@@@@@@@@@@@@@@@@@@@@@@begin ngx_writev_chain @@@@@@@@@@@@@@@@@@@");

    if (!wev->ready) {//���ӻ�û׼���ã����ص�ǰ�Ľڵ㡣
        return in;
    }

#if (NGX_HAVE_KQUEUE)

    if ((ngx_event_flags & NGX_USE_KQUEUE_EVENT) && wev->pending_eof) {
        (void) ngx_connection_error(c, wev->kq_errno,
                               "kevent() reported about an closed connection");
        wev->error = 1;
        return NGX_CHAIN_ERROR;
    }

#endif

    /* the maximum limit size is the maximum size_t value - the page size */

    if (limit == 0 || limit > (off_t) (NGX_MAX_SIZE_T_VALUE - ngx_pagesize)) {
        limit = NGX_MAX_SIZE_T_VALUE - ngx_pagesize;//�����ˣ���������
    }

    send = 0;

    vec.iovs = iovs;
    vec.nalloc = NGX_IOVS_PREALLOCATE;

    for ( ;; ) {
        prev_send = send; //prev_sendΪ��һ�ε���ngx_writev���ͳ�ȥ���ֽ���

        /* create the iovec and coalesce the neighbouring bufs */
        //��in���е�buf������vec->iovs[n++]�У�ע��ֻ�´���ڴ��е����ݵ�iovec�У����´���ļ��е�
        cl = ngx_output_chain_to_iovec(&vec, in, limit - send, c->log);

        if (cl == NGX_CHAIN_ERROR) {
            return NGX_CHAIN_ERROR;
        }

        if (cl && cl->buf->in_file) {
            ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                          "file buf in writev "
                          "t:%d r:%d f:%d %p %p-%p %p %O-%O",
                          cl->buf->temporary,
                          cl->buf->recycled,
                          cl->buf->in_file,
                          cl->buf->start,
                          cl->buf->pos,
                          cl->buf->last,
                          cl->buf->file,
                          cl->buf->file_pos,
                          cl->buf->file_last);

            ngx_debug_point();

            return NGX_CHAIN_ERROR;
        }

        send += vec.size; //Ϊngx_output_chain_to_iovec�������in�����������ݳ��Ⱥ�

        n = ngx_writev(c, &vec); 
        //����������vec->size�ֽ����ݣ�����ʵ�����ں˷��ͳ�ȥ�ĺܿ��ܱ�vec->sizeС��nΪʵ�ʷ��ͳ�ȥ���ֽ����������Ҫ��������

        if (n == NGX_ERROR) {
            return NGX_CHAIN_ERROR;
        }

        sent = (n == NGX_AGAIN) ? 0 : n;//��¼���͵����ݴ�С��

        c->sent += sent;//����ͳ�����ݣ���������Ϸ��͵����ݴ�С

        in = ngx_chain_update_sent(in, sent); //send�Ǵ˴ε���ngx_wrtev���ͳɹ����ֽ���
        //ngx_chain_update_sent���غ��in���Ѿ�������֮ǰ���ͳɹ���in�ڵ��ˣ�������ֻ����ʣ�������
        
        if (send - prev_send != sent) { //����˵��������ngx_writev���γɹ����ͺ�����ͻ᷵��
            wev->ready = 0; //�����ʱ���ܷ��������ˣ���������epoll_addд�¼�
            return in;
        }

        if (send >= limit || in == NULL) { //���ݷ�����ϣ����߱��η��ͳɹ����ֽ�����limit���࣬�򷵻س�ȥ
            return in; //
        }
    }
}

//��in���е�buf������vec->iovs[n++]�У�ע��ֻ�´���ڴ��е����ݵ�iovec�У����´���ļ��е�
ngx_chain_t *
ngx_output_chain_to_iovec(ngx_iovec_t *vec, ngx_chain_t *in, size_t limit,
    ngx_log_t *log)
{
    size_t         total, size;
    u_char        *prev;
    ngx_uint_t     n;
    struct iovec  *iov;

    iov = NULL;
    prev = NULL;
    total = 0;
    n = 0;
    //ѭ���������ݣ�һ��һ��IOV_MAX��Ŀ�Ļ�������
    for ( /* void */ ; in && total < limit; in = in->next) {

        if (ngx_buf_special(in->buf)) {
            continue;
        }

        if (in->buf->in_file) { //���Ϊ1,��ʾ�������ļ��У���ngx_output_chain_copy_buf
            break;
        }

        if (!ngx_buf_in_memory(in->buf)) {
            ngx_log_error(NGX_LOG_ALERT, log, 0,
                          "bad buf in output chain "
                          "t:%d r:%d f:%d %p %p-%p %p %O-%O",
                          in->buf->temporary,
                          in->buf->recycled,
                          in->buf->in_file,
                          in->buf->start,
                          in->buf->pos,
                          in->buf->last,
                          in->buf->file,
                          in->buf->file_pos,
                          in->buf->file_last);

            ngx_debug_point();

            return NGX_CHAIN_ERROR;
        }

        size = in->buf->last - in->buf->pos;//��������ڵ�Ĵ�С

        if (size > limit - total) {//��������ʹ�С���ضϣ����ֻ������ô��
            size = limit - total;
        }

        if (prev == in->buf->pos) {//������ǵ��ڸղŵ�λ�ã��Ǿ͸���
            iov->iov_len += size;

        } else {//����Ҫ����һ���ڵ㡣����֮
            if (n == vec->nalloc) {
                break;
            }

            iov = &vec->iovs[n++];

            iov->iov_base = (void *) in->buf->pos;//�����￪ʼ
            iov->iov_len = size;//����ô����Ҫ����
        }

        prev = in->buf->pos + size;//��¼�ղŷ��������λ�ã�Ϊָ�롣
        total += size;//�����Ѿ���¼�����ݳ��ȡ�
    }

    vec->count = n; //���n����0�������chain���е����������������Ŀռ���
    vec->size = total;

    return in;
}

/*
���˵����ݷ��ͣ����ᾭ������filterģ�飬��ͻ��˵İ�����Ӧ�ᾭ������filterģ��
2016/01/05 21:02:43[           ngx_event_process_posted,    67]  [debug] 23495#23495: *1 delete posted event AEA04098
2016/01/05 21:02:43[          ngx_http_upstream_handler,  1400]  [debug] 23495#23495: *1 http upstream request(ev->write:1): "/test2.php?"
2016/01/05 21:02:43[ngx_http_upstream_send_request_handler,  2420]  [debug] 23495#23495: *1 http upstream send request handler
2016/01/05 21:02:43[     ngx_http_upstream_send_request,  2167]  [debug] 23495#23495: *1 http upstream send request
2016/01/05 21:02:43[ngx_http_upstream_send_request_body,  2305]  [debug] 23495#23495: *1 http upstream send request body
2016/01/05 21:02:43[                   ngx_output_chain,    67][yangya  [debug] 23495#23495: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/05 21:02:43[                   ngx_output_chain,    90][yangya  [debug] 23495#23495: *1 only one chain buf to output_filter
2016/01/05 21:02:43[                   ngx_chain_writer,   747]  [debug] 23495#23495: *1 chain writer buf fl:0 s:600
2016/01/05 21:02:43[                   ngx_chain_writer,   762]  [debug] 23495#23495: *1 chain writer in: 080F268C
2016/01/05 21:02:43[           ngx_linux_sendfile_chain,   161][yangya  [debug] 23495#23495: *1 @@@@@@@@@@@@@@@@@@@@@@@begin ngx_linux_sendfile_chain @@@@@@@@@@@@@@@@@@@
2016/01/05 21:02:43[                         ngx_writev,   201]  [debug] 23495#23495: *1 writev: 600 of 600
2016/01/05 21:02:43[                   ngx_chain_writer,   801]  [debug] 23495#23495: *1 chain writer out: 00000000


���˵����ݷ��ͣ����ᾭ������filterģ�飬��ͻ��˵İ�����Ӧ�ᾭ������filterģ��
2016/01/05 21:02:43[ ngx_event_pipe_write_to_downstream,   623]  [debug] 23495#23495: *1 pipe write downstream flush out
2016/01/05 21:02:43[             ngx_http_output_filter,  3338]  [debug] 23495#23495: *1 http output filter "/test2.php?"
2016/01/05 21:02:43[               ngx_http_copy_filter,   199]  [debug] 23495#23495: *1 http copy filter: "/test2.php?", r->aio:0
2016/01/05 21:02:43[                   ngx_output_chain,    67][yangya  [debug] 23495#23495: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/05 21:02:43[                      ngx_read_file,    83]  [debug] 23495#23495: *1 read file /var/yyz/cache_xxx/temp/1/00/0000000001: 15, 081109E0, 215, 206
2016/01/05 21:02:43[           ngx_http_postpone_filter,   176]  [debug] 23495#23495: *1 http postpone filter "/test2.php?" 080F2D4C
2016/01/05 21:02:43[       ngx_http_chunked_body_filter,   212]  [debug] 23495#23495: *1 http chunk: 215
2016/01/05 21:02:43[       ngx_http_chunked_body_filter,   273]  [debug] 23495#23495: *1 yang test ..........xxxxxxxx ################## lstbuf:0
2016/01/05 21:02:43[              ngx_http_write_filter,   151]  [debug] 23495#23495: *1 write old buf t:1 f:0 080F2AE8, pos 080F2AE8, size: 180 file: 0, size: 0
2016/01/05 21:02:43[              ngx_http_write_filter,   207]  [debug] 23495#23495: *1 write new buf t:1 f:0 080F2D98, pos 080F2D98, size: 4 file: 0, size: 0
2016/01/05 21:02:43[              ngx_http_write_filter,   207]  [debug] 23495#23495: *1 write new buf t:1 f:0 081109E0, pos 081109E0, size: 215 file: 0, size: 0
2016/01/05 21:02:43[              ngx_http_write_filter,   207]  [debug] 23495#23495: *1 write new buf t:0 f:0 00000000, pos 080CDEDD, size: 2 file: 0, size: 0
2016/01/05 21:02:43[              ngx_http_write_filter,   247]  [debug] 23495#23495: *1 http write filter: last:0 flush:1 size:401
2016/01/05 21:02:43[              ngx_http_write_filter,   379]  [debug] 23495#23495: *1 http write filter limit 0
2016/01/05 21:02:43[           ngx_linux_sendfile_chain,   161][yangya  [debug] 23495#23495: *1 @@@@@@@@@@@@@@@@@@@@@@@begin ngx_linux_sendfile_chain @@@@@@@@@@@@@@@@@@@
2016/01/05 21:02:43[                         ngx_writev,   201]  [debug] 23495#23495: *1 writev: 401 of 401
2016/01/05 21:02:43[              ngx_http_write_filter,   385]  [debug] 23495#23495: *1 http write filter 00000000
2016/01/05 21:02:43[               ngx_http_copy_filter,   276]  [debug] 23495#23495: *1 http copy filter rc: 0, buffered:0 "/test2.php?"
2016/01/05 21:02:43[ ngx_event_pipe_write_to_downstream,   662]  [debug] 23495#23495: *1 pipe write downstream done

*/
ssize_t
ngx_writev(ngx_connection_t *c, ngx_iovec_t *vec)
{ //���˵����ݷ��ͣ����ᾭ������filterģ�飬��ͻ��˵İ�����Ӧ�ᾭ������filterģ��
    ssize_t    n;
    ngx_err_t  err;

eintr:
    //����writev������Щ���ݣ����ط��͵����ݴ�С
    //readv ��writev����һ�¶�д��������������ݣ�read��writeֻ��һ�¶�дһ�������������ݣ� 
    /* On success, the readv() function returns the number of bytes read; the writev() function returns the number of bytes written.  
        On error, -1 is returned, and errno is  set appropriately. readv���ر������ֽ����������û�и������ݺ������ļ�ĩβʱ����0�ļ����� */
    n = writev(c->fd, vec->iovs, vec->count);

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "writev: %z of %uz", n, vec->size);

    if (n == -1) {
        err = ngx_errno;

        switch (err) {
        case NGX_EAGAIN:
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "writev() not ready");
            return NGX_AGAIN;

        case NGX_EINTR:
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "writev() was interrupted");
            goto eintr;

        default:
            c->write->error = 1;
            ngx_connection_error(c, err, "writev() failed");
            return NGX_ERROR;
        }
    }

    return n;
}
