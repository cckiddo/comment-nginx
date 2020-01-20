
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#if 0
#define NGX_SENDFILE_LIMIT  4096
#endif

/*
 * When DIRECTIO is enabled FreeBSD, Solaris, and MacOSX read directly
 * to an application memory from a device if parameters are aligned
 * to device sector boundary (512 bytes).  They fallback to usual read
 * operation if the parameters are not aligned.
 * Linux allows DIRECTIO only if the parameters are aligned to a filesystem
 * sector boundary, otherwise it returns EINVAL.  The sector size is
 * usually 512 bytes, however, on XFS it may be 4096 bytes.
 */

#define NGX_NONE            1


static ngx_inline ngx_int_t
    ngx_output_chain_as_is(ngx_output_chain_ctx_t *ctx, ngx_buf_t *buf);
#if (NGX_HAVE_AIO_SENDFILE)
static ngx_int_t ngx_output_chain_aio_setup(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);
#endif
static ngx_int_t ngx_output_chain_add_copy(ngx_pool_t *pool,
    ngx_chain_t **chain, ngx_chain_t *in);
static ngx_int_t ngx_output_chain_align_file_buf(ngx_output_chain_ctx_t *ctx,
    off_t bsize);
static ngx_int_t ngx_output_chain_get_buf(ngx_output_chain_ctx_t *ctx,
    off_t bsize);
static ngx_int_t ngx_output_chain_copy_buf(ngx_output_chain_ctx_t *ctx);

/*
����Ŀ���Ƿ��� in �е����ݣ�ctx �������淢�͵������ģ���Ϊ����ͨ������£�����һ����ɡ�nginx ��Ϊʹ���� ET ģʽ��
���������¼������ϼ��ˣ����Ǳ���д����¼������ˣ���Ҫ��ͣ��ѭ���������¼��ĺ����ص�������Ҳ��ȷ���������
Ҫʹ�� context �����Ķ��������淢�͵�ʲô�����ˡ�
*/
//���ngx_http_xxx_create_request(ngx_http_fastcgi_create_request)�Ķ���ctx->in�е�����ʵ�����Ǵ�ngx_http_xxx_create_request���ngx_chain_t���ģ�������Դ��ngx_http_xxx_create_request
ngx_int_t //���˷�������ĵ��ù���ngx_http_upstream_send_request_body->ngx_output_chain->ngx_chain_writer



/*
�����aio on | thread_pool��ʽ���������ִ�иú������������в�������һ����ֻ��aio���ȡֵ��仯����־����:
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   604]  [debug] 20923#20923: *1 pipe write downstream, write ready: 1
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   649]  [debug] 20923#20923: *1 pipe write downstream flush out
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:0
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0

ע���һ����ngx_thread_read����ӡ��Ϣ�͵�һ����ȫһ��
2016/01/07 18:47:27[                    ngx_thread_read,   147]  [debug] 20923#20923: *1 thread read: fd:14, buf:08115A90, size:1220, offset:206
2016/01/07 18:47:27[              ngx_thread_mutex_lock,   145]  [debug] 20923#20923: pthread_mutex_lock(080F0458) enter
2016/01/07 18:47:27[               ngx_thread_task_post,   280][yangya  [debug] 20923#20923: ngx add task to thread, task id:158
2016/01/07 18:47:27[             ngx_thread_cond_signal,    54]  [debug] 20923#20923: pthread_cond_signal(080F047C)
2016/01/07 18:47:27[               ngx_thread_cond_wait,    96]  [debug] 20923#20928: pthread_cond_wait(080F047C) exit
2016/01/07 18:47:27[            ngx_thread_mutex_unlock,   171]  [debug] 20923#20928: pthread_mutex_unlock(080F0458) exit
2016/01/07 18:47:27[              ngx_thread_pool_cycle,   370]  [debug] 20923#20928: run task #158 in thread pool name:"yang_pool"
2016/01/07 18:47:27[            ngx_thread_read_handler,   201]  [debug] 20923#20928: thread read handler
2016/01/07 18:47:27[            ngx_thread_read_handler,   219]  [debug] 20923#20928: pread: 1220 (err: 0) of 1220 @206
2016/01/07 18:47:27[              ngx_thread_pool_cycle,   376]  [debug] 20923#20928: complete task #158 in thread pool name: "yang_pool"
2016/01/07 18:47:27[              ngx_thread_mutex_lock,   145]  [debug] 20923#20928: pthread_mutex_lock(080F0458) enter
2016/01/07 18:47:27[               ngx_thread_cond_wait,    70]  [debug] 20923#20928: pthread_cond_wait(080F047C) enter
2016/01/07 18:47:27[            ngx_thread_mutex_unlock,   171]  [debug] 20923#20923: pthread_mutex_unlock(080F0458) exit
2016/01/07 18:47:27[               ngx_thread_task_post,   297]  [debug] 20923#20923: task #158 added to thread pool name: "yang_pool" complete
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: -2, buffered:4 "/test2.php?"
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   688]  [debug] 20923#20923: *1 pipe write downstream done
2016/01/07 18:47:27[                ngx_event_del_timer,    39]  [debug] 20923#20923: *1 <           ngx_event_pipe,    91>  event timer del: 12: 464761188
2016/01/07 18:47:27[  ngx_http_upstream_process_request,  4233][yangya  [debug] 20923#20923: *1 ngx http cache, p->length:-1, u->headers_in.content_length_n:-1, tf->offset:1426, r->cache->body_start:206
2016/01/07 18:47:27[         ngx_http_file_cache_update,  1557]  [debug] 20923#20923: *1 http file cache update, c->body_start:206
2016/01/07 18:47:27[         ngx_http_file_cache_update,  1570]  [debug] 20923#20923: *1 http file cache rename: "/var/yyz/cache_xxx/temp/2/00/0000000002" to "/var/yyz/cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f", expire time:1800
2016/01/07 18:47:27[                     ngx_shmtx_lock,   168]  [debug] 20923#20923: shmtx lock
2016/01/07 18:47:27[                   ngx_shmtx_unlock,   249]  [debug] 20923#20923: shmtx unlock
2016/01/07 18:47:27[  ngx_http_upstream_process_request,  4270]  [debug] 20923#20923: *1 http upstream exit: 00000000
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4541]  [debug] 20923#20923: *1 finalize http upstream request rc: 0
2016/01/07 18:47:27[  ngx_http_fastcgi_finalize_request,  3215]  [debug] 20923#20923: *1 finalize http fastcgi request
2016/01/07 18:47:27[ngx_http_upstream_free_round_robin_peer,   887]  [debug] 20923#20923: *1 free rr peer 1 0
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4594]  [debug] 20923#20923: *1 close http upstream connection: 12
2016/01/07 18:47:27[               ngx_close_connection,  1120]  [debug] 20923#20923: *1 delete posted event AEA6B098
2016/01/07 18:47:27[            ngx_reusable_connection,  1177]  [debug] 20923#20923: *1 reusable connection: 0
2016/01/07 18:47:27[               ngx_close_connection,  1139][yangya  [debug] 20923#20923: close socket:12
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4608]  [debug] 20923#20923: *1 http upstream temp fd: 14
2016/01/07 18:47:27[              ngx_http_send_special,  3871][yangya  [debug] 20923#20923: *1 ngx http send special, flags:1, last_buf:1, sync:0, last_in_chain:0, flush:0
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:1
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:1, ctx->directio:0
2016/01/07 18:47:27[                   ngx_output_chain,   117][yangya  [debug] 20923#20923: *1 ctx->aio = 1, wait kernel complete read
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: -2, buffered:4 "/test2.php?"
2016/01/07 18:47:27[          ngx_http_finalize_request,  2598]  [debug] 20923#20923: *1 http finalize request rc: -2, "/test2.php?" a:1, c:1
2016/01/07 18:47:27[                ngx_event_add_timer,   100]  [debug] 20923#20923: *1 <ngx_http_set_write_handler,  3029>  event timer add fd:13, expire-time:60 s, timer.key:464761210
2016/01/07 18:47:27[           ngx_trylock_accept_mutex,   405]  [debug] 20923#20923: accept mutex locked
2016/01/07 18:47:27[           ngx_epoll_process_events,  1725]  [debug] 20923#20923: epoll: fd:9 EPOLLIN  (ev:0001) d:080E36C0
2016/01/07 18:47:27[           ngx_epoll_process_events,  1771]  [debug] 20923#20923: post event 080E3680
2016/01/07 18:47:27[           ngx_event_process_posted,    65]  [debug] 20923#20923: begin to run befor posted event 080E3680
2016/01/07 18:47:27[           ngx_event_process_posted,    67]  [debug] 20923#20923: delete posted event 080E3680
2016/01/07 18:47:27[            ngx_thread_pool_handler,   401]  [debug] 20923#20923: thread pool handler
2016/01/07 18:47:27[            ngx_thread_pool_handler,   422]  [debug] 20923#20923: run completion handler for task #158
2016/01/07 18:47:27[ ngx_http_copy_thread_event_handler,   429][yangya  [debug] 20923#20923: *1 ngx http aio thread event handler
2016/01/07 18:47:27[           ngx_http_request_handler,  2407]  [debug] 20923#20923: *1 http run request(ev->write:1): "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3058]  [debug] 20923#20923: *1 http writer handler: "/test2.php?"
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:0
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0

ע��ڶ�����ngx_thread_read����ӡ��Ϣ�͵�һ����ȫһ��
2016/01/07 18:47:27[                    ngx_thread_read,   147]  [debug] 20923#20923: *1 thread read: fd:14, buf:08115A90, size:1220, offset:206

2016/01/07 18:47:27[             ngx_output_chain_as_is,   314][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:1, in_file:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0
2016/01/07 18:47:27[           ngx_http_postpone_filter,   176]  [debug] 20923#20923: *1 http postpone filter "/test2.php?" 080F3E94
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   212]  [debug] 20923#20923: *1 http chunk: 1220
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   212]  [debug] 20923#20923: *1 http chunk: 0
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   273]  [debug] 20923#20923: *1 yang test ..........xxxxxxxx ################## lstbuf:1
2016/01/07 18:47:27[              ngx_http_write_filter,   151]  [debug] 20923#20923: *1 write old buf t:1 f:0 080F3B60, pos 080F3B60, size: 180 file: 0, size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:1 buf-in-file:0, buf->start:080F3EE0, buf->pos:080F3EE0, buf_size: 5 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:1 buf-in-file:0, buf->start:08115A90, buf->pos:08115A90, buf_size: 1220 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:0 buf-in-file:0, buf->start:00000000, buf->pos:080CF058, buf_size: 7 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   248]  [debug] 20923#20923: *1 http write filter: last:1 flush:1 size:1412
2016/01/07 18:47:27[              ngx_http_write_filter,   380]  [debug] 20923#20923: *1 http write filter limit 0
2016/01/07 18:47:27[           ngx_linux_sendfile_chain,   201][yangya  [debug] 20923#20923: *1 @@@@@@@@@@@@@@@@@@@@@@@begin ngx_linux_sendfile_chain @@@@@@@@@@@@@@@@@@@
2016/01/07 18:47:27[                         ngx_writev,   238]  [debug] 20923#20923: *1 writev: 1412 of 1412
2016/01/07 18:47:27[              ngx_http_write_filter,   386]  [debug] 20923#20923: *1 http write filter 00000000
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: 0, buffered:0 "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3124]  [debug] 20923#20923: *1 http writer output filter: 0, "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3156]  [debug] 20923#20923: *1 http writer done: "/test2.php?"
2016/01/07 18:47:27[          ngx_http_finalize_request,  2598]  [debug] 20923#20923: *1 http finalize request rc: 0, "/test2.php?" a:1, c:1
*/
//�����aio on | thread_pool��ʽ���������ִ�иú������������в�������һ�����ο�������־�����ļ����غ����ļ���ȡ���̻���һ����ֻ��
//��ngx_http_writer�������ж��Ƿ�д��ɣ�ͨ��r->buffered�Ƿ�Ϊ0������



/* ע��:�������inʵ�������Ѿ�ָ���������ݲ��֣�����������͵�������Ҫ���ļ��ж�ȡ��in��Ҳ��ָ���ļ�file_pos��file_last�Ѿ��ļ�fd��,
   ���Բο�ngx_http_cache_send ngx_http_send_header ngx_http_output_filter */
ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in)  //inΪ��Ҫ���͵�chain��������洢����ʵ��Ҫ���͵�����
{//ctxΪ&u->output�� inΪu->request_bufs����nginx filter����Ҫ�߼����������������,��in��������Ļ���鿽����
//ctx->in,Ȼ��ctx->in�����ݿ�����out,Ȼ�����output_filter���ͳ�ȥ��

//�����ȡ������ݷ����ͻ��ˣ�Ĭ��������//ngx_event_pipe->ngx_event_pipe_write_to_downstream->p->output_filter(p->output_ctx, p->out);�ߵ�����
    off_t         bsize;
    ngx_int_t     rc, last;
    ngx_chain_t  *cl, *out, **last_out;

    ngx_uint_t sendfile = ctx->sendfile;
    ngx_uint_t aio = ctx->aio;
    ngx_uint_t directio = ctx->directio;
    
    ngx_log_debugall(ctx->pool->log, 0, "ctx->sendfile:%ui, ctx->aio:%ui, ctx->directio:%ui", sendfile, aio, directio);
    if (ctx->in == NULL && ctx->busy == NULL
#if (NGX_HAVE_FILE_AIO || NGX_THREADS)
        && !ctx->aio
#endif
       ) //in�Ǵ����͵����ݣ�busy���Ѿ�����ngx_chain_writer����û�з�����ϡ�
    {
        /*
         * the short path for the case when the ctx->in and ctx->busy chains
         * are empty, the incoming chain is empty too or has the single buf
         * that does not require the copy
         */

        if (in == NULL) { //���Ҫ���͵�����Ϊ�գ�Ҳ����ɶҲ���÷��͡��Ǿ�ֱ�ӵ���output_filter���ˡ�
            ngx_log_debugall(ctx->pool->log, 0, "ngx output chain, in = NULL");
            return ctx->output_filter(ctx->filter_ctx, in);
        }

        if (in->next == NULL //˵������bufֻ��һ��
#if (NGX_SENDFILE_LIMIT)
            && !(in->buf->in_file && in->buf->file_last > NGX_SENDFILE_LIMIT)
#endif
            && ngx_output_chain_as_is(ctx, in->buf)) //���������Ҫ�����ж��Ƿ���Ҫ����buf������1,��ʾ����Ҫ����������Ϊ��Ҫ���� 
        {
            ngx_log_debugall(ctx->pool->log, 0, "only one chain buf to output_filter");
            return ctx->output_filter(ctx->filter_ctx, in);
        }
    }

    /* add the incoming buf to the chain ctx->in */
   
    if (in) {//����һ�����ݵ�ctx->in���棬��Ҫ����ʵʵ�Ľ������ݿ����ˡ���in������������ݿ�����ctx->in���档���˸�in
        if (ngx_output_chain_add_copy(ctx->pool, &ctx->in, in) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    /* outΪ������Ҫ�����chain��Ҳ���ǽ���ʣ�µ�filter�����chain */  
    out = NULL;
    last_out = &out; //�������ctx->in���е����ݲ�����ӵ���last_out�У�Ҳ������ӵ�out����
    last = NGX_NONE;
	//�������ˣ�in�����Ļ��������Ѿ�������ctx->in�����ˡ�����׼�����Ͱɡ�

    for ( ;; ) { //ѭ����ȡ�����л����ڴ��е����ݷ���

#if (NGX_HAVE_FILE_AIO || NGX_THREADS)
//ʵ�����ڽ����������ݺ�����ͻ��˷��Ͱ��岿�ֵ�ʱ�򣬻����ε��øú�����һ����ngx_event_pipe_write_to_downstream-> p->output_filter(),
//��һ����ngx_http_upstream_finalize_request->ngx_http_send_special,

//�����aio(aio on | aio thread_pool)��ʽ�����һ�θ�ֵΪ0�����ǵڶ��δ�ngx_http_send_special�ߵ������ʱ���Ѿ���ngx_output_chain->ngx_file_aio_read->ngx_http_copy_aio_handler��1

//��ʼ��ȡ���ݵ�ʱ����1��һ��Ҫ�ȵ�aio onģʽ����µ��ں��첽����ɻ���aio thread_poolģʽ�µ��̶߳�������ɣ�����ͨ��notify_epoll֪ͨ��������epoll_in��ʱ��������0����ʾ���ݶ�ȡ��ϣ�ֻ������������ܷ�������write
        if (ctx->aio) { //�����aio�������ں����READ��read�ɹ����epoll�������أ�ִ����ngx_file_aio_event_handler
            ngx_log_debugall(ctx->pool->log, 0, "ctx->aio = 1, wait AIO kernel complete read or wait thread pool to read complete");
            return NGX_AGAIN;
        }
#endif
        //���ngx_http_xxx_create_request(ngx_http_fastcgi_create_request)�Ķ���ctx->in�е�����ʵ�����Ǵ�ngx_http_xxx_create_request���ngx_chain_t���ģ�������Դ��ngx_http_xxx_create_request
        while (ctx->in) {//�������д����͵����ݡ�������һ����������outָ���������

            /*
             * cycle while there are the ctx->in bufs
             * and there are the free output bufs to copy in
             */

            bsize = ngx_buf_size(ctx->in->buf);
            //����ڴ��СΪ0��Ȼ���ֲ���special ���������⡣ �����special��buf��Ӧ���Ǵ�ngx_http_send_special������
            if (bsize == 0 && !ngx_buf_special(ctx->in->buf)) {

                ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, 0,
                              "zero size buf in output "
                              "t:%d r:%d f:%d %p %p-%p %p %O-%O",
                              ctx->in->buf->temporary,
                              ctx->in->buf->recycled,
                              ctx->in->buf->in_file,
                              ctx->in->buf->start,
                              ctx->in->buf->pos,
                              ctx->in->buf->last,
                              ctx->in->buf->file,
                              ctx->in->buf->file_pos,
                              ctx->in->buf->file_last);

                ngx_debug_point();

                ctx->in = ctx->in->next;

                continue;
            }
            /* �ж��Ƿ���Ҫ����buf */    
            if (ngx_output_chain_as_is(ctx, ctx->in->buf)) {
                //��ctx->in->buf��ctx->in����ȡ������Ȼ����뵽lst_out������
                /* move the chain link to the output chain */
                /* �������Ҫ���ƣ���ֱ������chain��out��Ȼ�����ѭ�� */ 
                cl = ctx->in;
                ctx->in = cl->next; //�Ѿ���ֵ�Ļ��ctx->in����ժ��

                *last_out = cl;
                last_out = &cl->next;
                cl->next = NULL;

                continue;
            }


//ע��Ӻ�˽��յ����ݵ������ļ��к���filterģ���У��п������µ�buf����ָ���ˣ���Ϊngx_http_copy_filter->ngx_output_chain�л����·����ڴ��ȡ�����ļ�����

            //�������Ҫ��ֵbuf(һ�㶼��sendfile��ʱ��)���û��ռ��ڴ�����û�����ݣ�������Ҫ���ٿռ������ļ��е����ݸ�ֵһ�ݳ���
            
            /* �������˵��������Ҫ����buf������buf���ն��ᱻ������ctx->buf�У� ����������ж�ctx->buf�Ƿ�Ϊ�� */ 
            if (ctx->buf == NULL) { //ÿ�ο�������ǰ���ȸ�ctx->buf����ռ䣬�������ngx_output_chain_get_buf������

                /* ���Ϊ�գ���ȡ��buf������Ҫע�⣬һ����˵���û�п���directio�Ļ�������������᷵��NGX_DECLINED */  
                rc = ngx_output_chain_align_file_buf(ctx, bsize);

                if (rc == NGX_ERROR) {
                    return NGX_ERROR;
                }

                if (rc != NGX_OK) {

                    if (ctx->free) {

                        /* get the free buf */

                        cl = ctx->free;
                        /* �õ�free buf */    
                        ctx->buf = cl->buf;
                        ctx->free = cl->next;
                        /* ��Ҫ���õ�chain���ӵ�ctx->poll�У��Ա���chain������ */  
                        ngx_free_chain(ctx->pool, cl);

                    } else if (out || ctx->allocated == ctx->bufs.num) {//output_buffers 1 32768��������
                   /* 
                        ����Ѿ�����buf�ĸ������ƣ�������ѭ���������Ѿ����ڵ�buf�� ������Կ������out���ڵĻ���nginx������ѭ����Ȼ����out��
                        �ȷ�������ٴδ�������ܺõ�������nginx����ʽ���� 
                        */ 
                        break;

                    } else if (ngx_output_chain_get_buf(ctx, bsize) != NGX_OK) {/* �����������Ҳ�ȽϹؼ���������ȡ��buf������������ϸ��������� */
                        //�ú�����ȡ�����ڴ汣�浽ctx->buf��
                        return NGX_ERROR;
                    }
                }
            }
            
            /* ��ԭ����buf�п������ݻ��ߴ�ԭ�����ļ��ж�ȡ���� */  //ע�������aio on����aio thread=poll��ʽ���ص���NGX_AGAIN
            rc = ngx_output_chain_copy_buf(ctx); //��ctx->in->buf�е����ݸ�ֵ��ctx->buf
//ngx_output_chain_copy_bufc��tx->in�е��ڴ����ݻ��߻����ļ����ݻ´����dst�У�Ҳ����ctx->buf,Ȼ����ngx_output_chain_copy_buf����
//�������°�ctx->buf��ֵ���µ�chain��Ȼ��write��ȥ ,������Ĵ�����chain

            if (rc == NGX_ERROR) {
                return rc;
            }

            if (rc == NGX_AGAIN) { 
            //AIO���첽��ʽ�����ں����з��ͳ�ȥ��Ӧ�ò㲻�ùܣ���ȡ�ļ���������Ϻ�epoll�ᴥ��ִ��ngx_file_aio_event_handler��ִ��ngx_http_copy_aio_event_handler,��ʾ�ں��Ѿ���ȡ���
                if (out) {
                    break;
                }

                return rc;
            }

            /* delete the completed buf from the ctx->in chain */

            if (ngx_buf_size(ctx->in->buf) == 0) {//����ڵ��СΪ0���ƶ�����һ���ڵ㡣
                ctx->in = ctx->in->next;
            }

            cl = ngx_alloc_chain_link(ctx->pool);
            if (cl == NULL) {
                return NGX_ERROR;
            }
            //��ngx_output_chain_copy_buf�д�ԭsrc���������ݸ�ֵ��cl->buf��Ȼ����ӵ�lst_out��ͷ��  Ҳ������ӵ�out����
            cl->buf = ctx->buf;
            cl->next = NULL;
            *last_out = cl;
            last_out = &cl->next;
            ctx->buf = NULL;

            //ע������û��continue;ֱ��������
        }

        if (out == NULL && last != NGX_NONE) {

            if (ctx->in) {
                return NGX_AGAIN;
            }

            return last;
        }

        last = ctx->output_filter(ctx->filter_ctx, out); //ngx_chain_writer

        if (last == NGX_ERROR || last == NGX_DONE) {
            return last;
        }

        ngx_chain_update_chains(ctx->pool, &ctx->free, &ctx->busy, &out,
                                ctx->tag);
        last_out = &out;
    }
}

/*
�ú�������1�����ʾ���ݿ���ֱ�ӷ��ͳ�ȥ���������0�����ʾ���ݻ��ڴ����ļ��ڣ���Ҫ����directio��ȡ����ȷҪ����ʹ��sendfileֱ�ӷ��͡�
��ȷҪ������ڴ滺��������ע�⣺buf->file->directio��of.is_directio��������directio���չ�������

    ����ngx_output_chain_as_is()����1������Ͳ����ˣ�ԭ���ø�������ngx_http_write_filter() -> ngx_linux_sendfile_chain()���̵����
�ڴ�����ͨ��writev()���ͣ������ļ�������ͨ��sendfile()���͡� 
    ������0�������ʾҪ��ȡ���ݵ�����������������������������ģ�Ҳ��������aio���ж�ȡ��Ҳ�������̣� 
ngx_output_chain_copy_buf() -> ngx_file_aio_read() 
*/
static ngx_inline ngx_int_t
ngx_output_chain_as_is(ngx_output_chain_ctx_t *ctx, ngx_buf_t *buf)//ngx_output_chain_as_is��aio����sendfile�ķ�֧��
{//��������ڵ��Ƿ���Կ��������content�Ƿ����ļ��С��ж��Ƿ���Ҫ����buf.
//����1��ʾ�ϲ㲻��Ҫ����buf,������Ҫ����allocһ���ڵ㣬����ʵ���ڴ浽����һ���ڵ㡣

/* �������0����ʾ��Ҫ��ngx_output_chain���¿��ٿռ䣬���֮ǰ��in_file�ģ������¶�ȡ�����ļ����ݵ��ڴ��У��ͱ�Ϊ�ڴ���chain buf�� */

/*
    һ�㿪��sendfile on��ʱ�򷵻�1,��Ϊngx_output_chain_as_is����1���������¿����ڴ�ռ��ȡ�������ݡ�Ȼ��ͨ��
ngx_linux_sendfile_chain�е�ngx_linux_sendfileֱ��ͨ��sendfile���ͳ�ȥ���������ȡ�����ļ����ݵ��ڴ棬Ȼ����ڴ��з��͡�

    һ�㲻�������湦�ܵ�ʱ�򣬸ú���Ҳ�᷵��1����ʱ�������ݲ��Ỻ�浽�ļ������ǽ��յ��ڴ���Ȼ���ͣ���ngx_linux_sendfile_chain��
ֱ�ӵ���ngx_writev���ͣ�������sendfile����

    �������0�������ngx_http_copy_filter->ngx_output_chain->ngx_output_chain_align_file_buf�������ڴ棬Ȼ���ngx_output_chain_copy_buf�д���
��ȡ�����ļ����ݵ��µ��ڴ��У�Ȼ������ڴ����ݷ��ͳ�ȥ����ngx_linux_sendfile_chain��ֱ�ӵ���ngx_writev���ͣ�������sendfile����
 */

    ngx_uint_t  sendfile;

    unsigned int buf_special = ngx_buf_special(buf);
    unsigned int in_file = buf->in_file;
    unsigned int buf_in_mem = ngx_buf_in_memory(buf);
    unsigned int need_in_memory = ctx->need_in_memory;
    unsigned int need_in_temp = ctx->need_in_temp;
    unsigned int memory = buf->memory;
    unsigned int mmap = buf->mmap;
   if (buf->in_file) {
       unsigned int directio = buf->file->directio;
       ngx_log_debugall(ngx_cycle->log, 0, 
        "ngx_output_chain_as_is--- buf_special:%ui, in_file:%ui, directio:%ui, buf_in_mem:%ui,"
            "need_in_memory:%ui, need_in_temp:%ui, memory:%ui, mmap:%ui", 
            buf_special, in_file, directio, buf_in_mem, need_in_memory, need_in_temp, memory, mmap);
    } else {
        ngx_log_debugall(ngx_cycle->log, 0, 
        "ngx_output_chain_as_is--- buf_special:%ui, in_file:%ui, buf_in_mem:%ui,"
            "need_in_memory:%ui, need_in_temp:%ui, memory:%ui, mmap:%ui", 
            buf_special, in_file, buf_in_mem, need_in_memory, need_in_temp, memory, mmap);
    }
    
    if (ngx_buf_special(buf)) { 
    //˵��buf��û��ʵ������  ��������ڷ������ݵ���˺󣬶������һ��ngx_http_send_special������ngx_http_write_filter���̰����ݷ��ͳ�ȥ
    //��ʱ��ngx_http_send_special��������һ����buf
        return 1;
    }

#if (NGX_THREADS)
    if (buf->in_file) {
        //ngx_http_copy_filter�и�ֵΪngx_http_copy_thread_handler
        buf->file->thread_handler = ctx->thread_handler;
        buf->file->thread_ctx = ctx->filter_ctx;
    }
#endif

    /*
     Ngx_http_echo_subrequest.c (src\echo-nginx-module-master\src):        b->file->directio = of.is_directio;
     Ngx_http_flv_module.c (src\http\modules):    b->file->directio = of.is_directio;
     Ngx_http_gzip_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
     Ngx_http_mp4_module.c (src\http\modules):    b->file->directio = of.is_directio;
     Ngx_http_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
    ֻ���������⼸��ģ������1�������ﷵ�أ�Ҳ����˵����������⼸��ģ�������ͬʱ����sendfile on; aio on;directio xxx����ǰ���£�
    ������ﷵ�س�ȥ��Ȼ����»�ȡ�ռ�

    ������������������Щģ�飬��ͬʱ����sendfile on; aio on;directio xxx;������»��ǻ᷵��1��Ҳ���ǻ��ǲ���sendfile��ʽ
     */
    if (buf->in_file && buf->file->directio) {  
        return 0;//���buf���ļ��У�ʹ����directio����Ҫ����buf
    }

    sendfile = ctx->sendfile;

#if (NGX_SENDFILE_LIMIT)

    if (buf->in_file && buf->file_pos >= NGX_SENDFILE_LIMIT) { //�ļ������ݳ�����sendfile���������,��ֻ�����¶�ȡ�ļ����ڴ��з���
        sendfile = 0;
    }

#endif

    if (!sendfile) {

        if (!ngx_buf_in_memory(buf)) { //һ�㲻����sendfile on��ʱ��������˳�ȥ  ���������aio on��û������sendfile on������£�Ҳ��������ȥ
        //������sendfile(Ҫôδ����sendfile��Ҫô������sendfile�������ļ�̫�󣬳���sendfile����)������buf���ļ��У�����0����Ҫ���»�ȡ�ļ�����
            return 0;
        }

        buf->in_file = 0; 
    }

#if (NGX_HAVE_AIO_SENDFILE)
    if (ctx->aio_preload && buf->in_file) {
        (void) ngx_output_chain_aio_setup(ctx, buf->file);
    }
#endif
    /* (ʹ��sendfile�Ļ����ڴ���û���ļ��Ŀ����ģ���������ʱ��Ҫ�����ļ��������Ҫ�����ļ�����*/
    if (ctx->need_in_memory && !ngx_buf_in_memory(buf)) {
        return 0;
    }

    if (ctx->need_in_temp && (buf->memory || buf->mmap)) {
        return 0;
    }

    //����sendfile onһ��������ȥ��������������棬Ҳ���ǲ��������ݵ��ļ���һ��Ҳ������ﷵ��
    return 1;
}


#if (NGX_HAVE_AIO_SENDFILE)

static ngx_int_t
ngx_output_chain_aio_setup(ngx_output_chain_ctx_t *ctx, ngx_file_t *file)
{
    ngx_event_aio_t  *aio;

    if (file->aio == NULL && ngx_file_aio_init(file, ctx->pool) != NGX_OK) {
        return NGX_ERROR;
    }

    aio = file->aio;

    aio->data = ctx->filter_ctx;
    aio->preload_handler = ctx->aio_preload;

    return NGX_OK;
}

#endif

//���»�ȡһ��ngx_chain_t�ṹ���ýṹ��bufָ��in->buf���ú������µ�ngx_chain_t��ӵ�chain����ĩβ
static ngx_int_t
ngx_output_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in)
{//ngx_output_chain���������u->request_bufsҲ���ǲ��� in�����ݿ�����chain���档
//����Ϊ:(ctx->pool, &ctx->in, in)��in����Ҫ���͵ģ�Ҳ��������Ļ���������
    ngx_chain_t  *cl, **ll;
#if (NGX_SENDFILE_LIMIT)
    ngx_buf_t    *b, *buf;
#endif

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next; //llָ��chain��ĩβ
    }

    while (in) {

        cl = ngx_alloc_chain_link(pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

#if (NGX_SENDFILE_LIMIT)

        buf = in->buf;

        if (buf->in_file
            && buf->file_pos < NGX_SENDFILE_LIMIT
            && buf->file_last > NGX_SENDFILE_LIMIT)
        {//�������buffer���ļ��У������ļ�û�г������ƣ��ǾͿ����������ǣ��������ļ�������limit������ô�죬��ֳ�2��buffer��
            /* split a file buf on two bufs by the sendfile limit */

            b = ngx_calloc_buf(pool);
            if (b == NULL) {
                return NGX_ERROR;
            }

            ngx_memcpy(b, buf, sizeof(ngx_buf_t));

            if (ngx_buf_in_memory(buf)) {
                buf->pos += (ssize_t) (NGX_SENDFILE_LIMIT - buf->file_pos);
                b->last = buf->pos;
            }

            buf->file_pos = NGX_SENDFILE_LIMIT;
            b->file_last = NGX_SENDFILE_LIMIT;

            cl->buf = b;

        } else {
            cl->buf = buf;
            in = in->next;
        }

#else
        cl->buf = in->buf;
        in = in->next;

#endif

        cl->next = NULL;
        *ll = cl;
        ll = &cl->next;
    }

    return NGX_OK;
}

//ֻ��ngx_output_chain_align_file_buf�������ڴ�ֱ�ӷ��غ�Ż���ngx_output_chain_get_buf������������ngx_buf_in_memory���ڴ�ռ�
static ngx_int_t
ngx_output_chain_align_file_buf(ngx_output_chain_ctx_t *ctx, off_t bsize)
{ //���øú�����ǰ������Ҫ���·���ռ䣬ngx_output_chain_as_is����0
    size_t      size;
    ngx_buf_t  *in;

    in = ctx->in->buf;

    /*
     //Ngx_http_echo_subrequest.c (src\echo-nginx-module-master\src):        b->file->directio = of.is_directio;
     //Ngx_http_flv_module.c (src\http\modules):    b->file->directio = of.is_directio;
     //Ngx_http_gzip_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
     //Ngx_http_mp4_module.c (src\http\modules):    b->file->directio = of.is_directio;
     //Ngx_http_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
     ������ݲ����ļ��ж����ڴ��У�����û���������ļ������������⼸��ģ��������Ϣ�����߻�ȡ���ļ���СС��directio���õĴ�С����ֱ�ӷ���
     */
    if (in->file == NULL || !in->file->directio) {
    //���û������direction,��ֱ�ӷ��أ�ʵ�ʿռ��ڸú������ngx_output_chain_get_buf�д���
        return NGX_DECLINED;
    }


    /* �������ļ����棬���ҳ������ߵ��� b->file->directio = of.is_directio;�⼸��ģ�飬
        �����ļ���С����directio xxx�еĴ�С */
    ctx->directio = 1;

    size = (size_t) (in->file_pos - (in->file_pos & ~(ctx->alignment - 1)));

    if (size == 0) {

        if (bsize >= (off_t) ctx->bufs.size) {
            return NGX_DECLINED;
        }

        size = (size_t) bsize;

    } else {
        size = (size_t) ctx->alignment - size;

        if ((off_t) size > bsize) {
            size = (size_t) bsize;
        }
    }

    ctx->buf = ngx_create_temp_buf(ctx->pool, size);
    if (ctx->buf == NULL) {
        return NGX_ERROR;
    }
    //ע�����û��ָ������ڴ������ڴ�ռ䣬ngx_output_chain_copy_buf->ngx_buf_in_memory������������
    
    /*
     * we do not set ctx->buf->tag, because we do not want
     * to reuse the buf via ctx->free list
     */

#if (NGX_HAVE_ALIGNED_DIRECTIO)
    ctx->unaligned = 1;
#endif

    return NGX_OK;
}

//ֻ��ngx_output_chain_align_file_buf�������ڴ�ֱ�ӷ��غ�Ż���ngx_output_chain_get_buf������������ngx_buf_in_memory���ڴ�ռ�

//��ȡbsize�ֽڵĿռ�
static ngx_int_t
ngx_output_chain_get_buf(ngx_output_chain_ctx_t *ctx, off_t bsize)
{ /* ���濪�ٵ�����������ngx_buf_in_memory���ڴ�ռ� */ //���øú�����ǰ������Ҫ���·���ռ䣬ngx_output_chain_as_is����0
    size_t       size;
    ngx_buf_t   *b, *in;
    ngx_uint_t   recycled;

    in = ctx->in->buf;
    size = ctx->bufs.size;
    recycled = 1;

    if (in->last_in_chain) {

        if (bsize < (off_t) size) {

            /*
             * allocate a small temp buf for a small last buf
             * or its small last part
             */

            size = (size_t) bsize;
            recycled = 0;

        } else if (!ctx->directio
                   && ctx->bufs.num == 1
                   && (bsize < (off_t) (size + size / 4)))
        {
            /*
             * allocate a temp buf that equals to a last buf,
             * if there is no directio, the last buf size is lesser
             * than 1.25 of bufs.size and the temp buf is single
             */

            size = (size_t) bsize;
            recycled = 0;
        }
    }

    b = ngx_calloc_buf(ctx->pool);
    if (b == NULL) {
        return NGX_ERROR;
    }

    if (ctx->directio) {//�ڸú�������ǰ��ngx_output_chain_align_file_buf����directioΪ1

        /*
         * allocate block aligned to a disk sector size to enable
         * userland buffer direct usage conjunctly with directio
         */

        b->start = ngx_pmemalign(ctx->pool, size, (size_t) ctx->alignment);
        if (b->start == NULL) {
            return NGX_ERROR;
        }

    } else {
        b->start = ngx_palloc(ctx->pool, size);
        if (b->start == NULL) {
            return NGX_ERROR;
        }
    }

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;
    b->tag = ctx->tag;
    b->recycled = recycled;

    ctx->buf = b;//�ú�����ȡ�����ڴ汣�浽ctx->buf��
    ctx->allocated++;

    return NGX_OK;
}

/*
�����aio on | thread_pool��ʽ���������ִ�иú������������в�������һ����ֻ��aio���ȡֵ��仯����־����:
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   604]  [debug] 20923#20923: *1 pipe write downstream, write ready: 1
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   649]  [debug] 20923#20923: *1 pipe write downstream flush out
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:0
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0

ע���һ����ngx_thread_read����ӡ��Ϣ�͵�һ����ȫһ��
2016/01/07 18:47:27[                    ngx_thread_read,   147]  [debug] 20923#20923: *1 thread read: fd:14, buf:08115A90, size:1220, offset:206
2016/01/07 18:47:27[              ngx_thread_mutex_lock,   145]  [debug] 20923#20923: pthread_mutex_lock(080F0458) enter
2016/01/07 18:47:27[               ngx_thread_task_post,   280][yangya  [debug] 20923#20923: ngx add task to thread, task id:158
2016/01/07 18:47:27[             ngx_thread_cond_signal,    54]  [debug] 20923#20923: pthread_cond_signal(080F047C)
2016/01/07 18:47:27[               ngx_thread_cond_wait,    96]  [debug] 20923#20928: pthread_cond_wait(080F047C) exit
2016/01/07 18:47:27[            ngx_thread_mutex_unlock,   171]  [debug] 20923#20928: pthread_mutex_unlock(080F0458) exit
2016/01/07 18:47:27[              ngx_thread_pool_cycle,   370]  [debug] 20923#20928: run task #158 in thread pool name:"yang_pool"
2016/01/07 18:47:27[            ngx_thread_read_handler,   201]  [debug] 20923#20928: thread read handler
2016/01/07 18:47:27[            ngx_thread_read_handler,   219]  [debug] 20923#20928: pread: 1220 (err: 0) of 1220 @206
2016/01/07 18:47:27[              ngx_thread_pool_cycle,   376]  [debug] 20923#20928: complete task #158 in thread pool name: "yang_pool"
2016/01/07 18:47:27[              ngx_thread_mutex_lock,   145]  [debug] 20923#20928: pthread_mutex_lock(080F0458) enter
2016/01/07 18:47:27[               ngx_thread_cond_wait,    70]  [debug] 20923#20928: pthread_cond_wait(080F047C) enter
2016/01/07 18:47:27[            ngx_thread_mutex_unlock,   171]  [debug] 20923#20923: pthread_mutex_unlock(080F0458) exit
2016/01/07 18:47:27[               ngx_thread_task_post,   297]  [debug] 20923#20923: task #158 added to thread pool name: "yang_pool" complete
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: -2, buffered:4 "/test2.php?"
2016/01/07 18:47:27[ ngx_event_pipe_write_to_downstream,   688]  [debug] 20923#20923: *1 pipe write downstream done
2016/01/07 18:47:27[                ngx_event_del_timer,    39]  [debug] 20923#20923: *1 <           ngx_event_pipe,    91>  event timer del: 12: 464761188
2016/01/07 18:47:27[  ngx_http_upstream_process_request,  4233][yangya  [debug] 20923#20923: *1 ngx http cache, p->length:-1, u->headers_in.content_length_n:-1, tf->offset:1426, r->cache->body_start:206
2016/01/07 18:47:27[         ngx_http_file_cache_update,  1557]  [debug] 20923#20923: *1 http file cache update, c->body_start:206
2016/01/07 18:47:27[         ngx_http_file_cache_update,  1570]  [debug] 20923#20923: *1 http file cache rename: "/var/yyz/cache_xxx/temp/2/00/0000000002" to "/var/yyz/cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f", expire time:1800
2016/01/07 18:47:27[                     ngx_shmtx_lock,   168]  [debug] 20923#20923: shmtx lock
2016/01/07 18:47:27[                   ngx_shmtx_unlock,   249]  [debug] 20923#20923: shmtx unlock
2016/01/07 18:47:27[  ngx_http_upstream_process_request,  4270]  [debug] 20923#20923: *1 http upstream exit: 00000000
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4541]  [debug] 20923#20923: *1 finalize http upstream request rc: 0
2016/01/07 18:47:27[  ngx_http_fastcgi_finalize_request,  3215]  [debug] 20923#20923: *1 finalize http fastcgi request
2016/01/07 18:47:27[ngx_http_upstream_free_round_robin_peer,   887]  [debug] 20923#20923: *1 free rr peer 1 0
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4594]  [debug] 20923#20923: *1 close http upstream connection: 12
2016/01/07 18:47:27[               ngx_close_connection,  1120]  [debug] 20923#20923: *1 delete posted event AEA6B098
2016/01/07 18:47:27[            ngx_reusable_connection,  1177]  [debug] 20923#20923: *1 reusable connection: 0
2016/01/07 18:47:27[               ngx_close_connection,  1139][yangya  [debug] 20923#20923: close socket:12
2016/01/07 18:47:27[ ngx_http_upstream_finalize_request,  4608]  [debug] 20923#20923: *1 http upstream temp fd: 14
2016/01/07 18:47:27[              ngx_http_send_special,  3871][yangya  [debug] 20923#20923: *1 ngx http send special, flags:1, last_buf:1, sync:0, last_in_chain:0, flush:0
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:1
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:1, ctx->directio:0
2016/01/07 18:47:27[                   ngx_output_chain,   117][yangya  [debug] 20923#20923: *1 ctx->aio = 1, wait kernel complete read
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: -2, buffered:4 "/test2.php?"
2016/01/07 18:47:27[          ngx_http_finalize_request,  2598]  [debug] 20923#20923: *1 http finalize request rc: -2, "/test2.php?" a:1, c:1
2016/01/07 18:47:27[                ngx_event_add_timer,   100]  [debug] 20923#20923: *1 <ngx_http_set_write_handler,  3029>  event timer add fd:13, expire-time:60 s, timer.key:464761210
2016/01/07 18:47:27[           ngx_trylock_accept_mutex,   405]  [debug] 20923#20923: accept mutex locked
2016/01/07 18:47:27[           ngx_epoll_process_events,  1725]  [debug] 20923#20923: epoll: fd:9 EPOLLIN  (ev:0001) d:080E36C0
2016/01/07 18:47:27[           ngx_epoll_process_events,  1771]  [debug] 20923#20923: post event 080E3680
2016/01/07 18:47:27[           ngx_event_process_posted,    65]  [debug] 20923#20923: begin to run befor posted event 080E3680
2016/01/07 18:47:27[           ngx_event_process_posted,    67]  [debug] 20923#20923: delete posted event 080E3680
2016/01/07 18:47:27[            ngx_thread_pool_handler,   401]  [debug] 20923#20923: thread pool handler
2016/01/07 18:47:27[            ngx_thread_pool_handler,   422]  [debug] 20923#20923: run completion handler for task #158
2016/01/07 18:47:27[ ngx_http_copy_thread_event_handler,   429][yangya  [debug] 20923#20923: *1 ngx http aio thread event handler
2016/01/07 18:47:27[           ngx_http_request_handler,  2407]  [debug] 20923#20923: *1 http run request(ev->write:1): "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3058]  [debug] 20923#20923: *1 http writer handler: "/test2.php?"
2016/01/07 18:47:27[             ngx_http_output_filter,  3377]  [debug] 20923#20923: *1 http output filter "/test2.php?"
2016/01/07 18:47:27[               ngx_http_copy_filter,   206]  [debug] 20923#20923: *1 http copy filter: "/test2.php?", r->aio:0
2016/01/07 18:47:27[                   ngx_output_chain,    67][yangya  [debug] 20923#20923: *1 ctx->sendfile:0, ctx->aio:0, ctx->directio:0
2016/01/07 18:47:27[             ngx_output_chain_as_is,   309][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:0, in_file:1, directio:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0

ע��ڶ�����ngx_thread_read����ӡ��Ϣ�͵�һ����ȫһ��
2016/01/07 18:47:27[                    ngx_thread_read,   147]  [debug] 20923#20923: *1 thread read: fd:14, buf:08115A90, size:1220, offset:206

2016/01/07 18:47:27[             ngx_output_chain_as_is,   314][yangya  [debug] 20923#20923: ngx_output_chain_as_is--- buf_special:1, in_file:0, buf_in_mem:0,need_in_memory:0, need_in_temp:0, memory:0, mmap:0
2016/01/07 18:47:27[           ngx_http_postpone_filter,   176]  [debug] 20923#20923: *1 http postpone filter "/test2.php?" 080F3E94
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   212]  [debug] 20923#20923: *1 http chunk: 1220
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   212]  [debug] 20923#20923: *1 http chunk: 0
2016/01/07 18:47:27[       ngx_http_chunked_body_filter,   273]  [debug] 20923#20923: *1 yang test ..........xxxxxxxx ################## lstbuf:1
2016/01/07 18:47:27[              ngx_http_write_filter,   151]  [debug] 20923#20923: *1 write old buf t:1 f:0 080F3B60, pos 080F3B60, size: 180 file: 0, size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:1 buf-in-file:0, buf->start:080F3EE0, buf->pos:080F3EE0, buf_size: 5 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:1 buf-in-file:0, buf->start:08115A90, buf->pos:08115A90, buf_size: 1220 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   208]  [debug] 20923#20923: *1 write new buf temporary:0 buf-in-file:0, buf->start:00000000, buf->pos:080CF058, buf_size: 7 file_pos: 0, in_file_size: 0
2016/01/07 18:47:27[              ngx_http_write_filter,   248]  [debug] 20923#20923: *1 http write filter: last:1 flush:1 size:1412
2016/01/07 18:47:27[              ngx_http_write_filter,   380]  [debug] 20923#20923: *1 http write filter limit 0
2016/01/07 18:47:27[           ngx_linux_sendfile_chain,   201][yangya  [debug] 20923#20923: *1 @@@@@@@@@@@@@@@@@@@@@@@begin ngx_linux_sendfile_chain @@@@@@@@@@@@@@@@@@@
2016/01/07 18:47:27[                         ngx_writev,   238]  [debug] 20923#20923: *1 writev: 1412 of 1412
2016/01/07 18:47:27[              ngx_http_write_filter,   386]  [debug] 20923#20923: *1 http write filter 00000000
2016/01/07 18:47:27[               ngx_http_copy_filter,   284]  [debug] 20923#20923: *1 http copy filter rc: 0, buffered:0 "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3124]  [debug] 20923#20923: *1 http writer output filter: 0, "/test2.php?"
2016/01/07 18:47:27[                    ngx_http_writer,  3156]  [debug] 20923#20923: *1 http writer done: "/test2.php?"
2016/01/07 18:47:27[          ngx_http_finalize_request,  2598]  [debug] 20923#20923: *1 http finalize request rc: 0, "/test2.php?" a:1, c:1
*/

//�����aio on | thread_pool��ʽ���������ִ�иú������������в�������һ�����ο�������־
//ngx_output_chain_as_is  ngx_output_chain_copy_buf��aio��sendfile����ͨ�ļ���д�ķ�֧��
static ngx_int_t //ע�������aio on����aio thread=poll��ʽ���ص���NGX_AGAIN
ngx_output_chain_copy_buf(ngx_output_chain_ctx_t *ctx)
{//��ctx->in->buf�Ļ��忽����ctx->buf����ȥ��  ע���Ǵ��·��������ݿռ䣬�����洢ԭ����in->buf�е����ݣ�ʵ�������ھ���������ͬ��������(bufָ����ͬ���ڴ�ռ�)
    off_t        size;
    ssize_t      n;
    ngx_buf_t   *src, *dst;
    ngx_uint_t   sendfile;

    src = ctx->in->buf;//���ngx_http_xxx_create_request(ngx_http_fastcgi_create_request)�Ķ���ctx->in�е�����ʵ�����Ǵ�ngx_http_xxx_create_request���ngx_chain_t���ģ�������Դ��ngx_http_xxx_create_request
//ctx->in�е��ڴ����ݻ��߻����ļ����ݻ´����dst�У�Ҳ����ngx_output_chain_ctx_t->buf,Ȼ����ngx_output_chain_copy_buf�����������°�ctx->buf��ֵ���µ�chain��Ȼ��write��ȥ
    dst = ctx->buf; 
    
    /* ������������ӻ����ļ��ж�ȡ������nginx���ط������ļ��ܴ���dst�ռ����32768�ֽڣ���ngx_output_chain_get_buf��Ҳ���ǿ�����һ�������ļ��л�ȡ32768�ֽ� */
    size = ngx_buf_size(src); //���bufָ������ļ��������ļ��е����ݣ��������ڴ�buf�е�����
    size = ngx_min(size, dst->end - dst->pos); //����dst�ռ䲻����װ����src�е����ݴ�С

//ע��:һ�㻺���е��ļ�ͨ��sendfile���͵�ʱ��һ����ngx_output_chain_as_is����1����ʾ�����¿��ٿռ䣬��˲����ߵ��ú�������������ngx_output_chain_as_is��need_in_memory��1�����
    sendfile = ctx->sendfile & !ctx->directio;//�Ƿ����sendfile  Ҳ����˵���ͬʱ������sendfile��aio xxx;directio xxx����ctx->directioΪ1,��Ĭ�Ϲر�sendfile

#if (NGX_SENDFILE_LIMIT)

    if (src->in_file && src->file_pos >= NGX_SENDFILE_LIMIT) {//˵���ļ����ݳ����ˣ�����ʹ��sendfile
        sendfile = 0;
    }

#endif
    //�ú������ֻ��ngx_output_chain_align_file_buf�������ڴ�ֱ�ӷ��غ�Ż���ngx_output_chain_get_buf������������ngx_buf_in_memory���ڴ�ռ�
    if (ngx_buf_in_memory(src)) {
    /* (�������ڴ���)������(�������ļ����棬����b->file->directio = 0;�����ļ���СС��directio xxx�еĴ�С) */
        ngx_memcpy(dst->pos, src->pos, (size_t) size); 
        //�����sizeΪʲô�ܱ�֤��Խ�磬����Ϊ�����ڴ��ʱ������ngx_output_chain_get_buf��ʱ��bsize�͵���bsize = ngx_buf_size(ctx->in->buf);
        src->pos += (size_t) size;
        dst->last += (size_t) size; //ע��dst->pose��û���ƶ�

        if (src->in_file) { //????????????? �ⲿ���е�ûŪ���� sendfile�е��Σ��������
        //size������Ҫô�������ļ��У�Ҫô�����ڴ��С�ǰ���size = ngx_buf_size(src);ҳ���Կ�����

            if (sendfile) {
            //��ͬʱ����sendfile on; aio on�Լ�direction xxx����ǰ���£������ģ��ִ�й�b->file->directio = 1(of.is_directio);
            //�����ļ���СС��direction�����ã�����ʹ��sendfile
            
                dst->in_file = 1;
                dst->file = src->file;

                //Դ�ļ��д洢������ָ��
                dst->file_pos = src->file_pos;
                dst->file_last = src->file_pos + size;

            } else {
                dst->in_file = 0;
            }

            src->file_pos += size;

        } else {
            dst->in_file = 0;
        }

        if (src->pos == src->last) {  
            dst->flush = src->flush;
            dst->last_buf = src->last_buf;
            dst->last_in_chain = src->last_in_chain;
        }

    } else {//�����ļ������ڴ����棬��Ҫ�Ӵ��̶�ȡ��

#if (NGX_HAVE_ALIGNED_DIRECTIO)

        if (ctx->unaligned) {
            if (ngx_directio_off(src->file->fd) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, ngx_errno,
                              ngx_directio_off_n " \"%s\" failed",
                              src->file->name.data);
            }
        }

#endif
//aio on | off | threads[=pool];
#if (NGX_HAVE_FILE_AIO)
        if (ctx->aio_handler) {// aio on�������  ngx_output_chain_copy_buf  ngx_file_aio_read
            n = ngx_file_aio_read(src->file, dst->pos, (size_t) size,
                                  src->file_pos, ctx->pool);
            if (n == NGX_AGAIN) {//��һ�ε������ʾ��ʼ֪ͨ�ں�ͨ��AIO�첽��ȡ���ݣ���������»ط���NGX_AGAIN
            //AIO���첽��ʽ�����ں����з��ͳ�ȥ��Ӧ�ò㲻�ùܣ�������Ϻ��ִ��ngx_file_aio_event_handler��ִ��ngx_http_copy_aio_event_handler,��ʾ�ں��Զ��������
                ctx->aio_handler(ctx, src->file); //�ú������ngx_http_copy_filter��ֵΪctx->aio_handler = ngx_http_copy_aio_handler;
                return NGX_AGAIN;
            }
            
            //���ͨ��notify_epoll֪ͨAIO on��ʽ�ں˶�ȡ������ɣ�������ﷵ���̳߳����̶߳�ȡ������ֽ���
        } else
#endif //aio on | off | threads[=pool];
#if (NGX_THREADS)
        if (src->file->thread_handler) {//aio thread=poll�����
            n = ngx_thread_read(&ctx->thread_task, src->file, dst->pos,
                                (size_t) size, src->file_pos, ctx->pool);
            if (n == NGX_AGAIN) {//��һ�ε������ʾ��ʼ֪ͨ�̳߳ض�ȡ���ݣ���������»ط���NGX_AGAIN
                ctx->aio = 1;
                return NGX_AGAIN;
            }

            //���ͨ��notify_epoll֪ͨ�̳߳��е��̴߳����������ɣ�������ﷵ���̳߳����̶߳�ȡ������ֽ���
        } else
#endif
        { //Ϊ����aio��sendfile�����ֱ�Ӵ������ȡ�����ļ�
            n = ngx_read_file(src->file, dst->pos, (size_t) size,
                              src->file_pos); //��src->file�ļ���src->file_pos����ȡsize�ֽڵ�dst->posָ����ڴ�ռ�
        }

#if (NGX_HAVE_ALIGNED_DIRECTIO)
        /* �������ļ����棬���ҳ������ߵ��� b->file->directio = of.is_directio;�⼸��ģ�飬
        �����ļ���С����directio xxx�еĴ�С */
        if (ctx->unaligned) {
            ngx_err_t  err;

            err = ngx_errno;
            
            if (ngx_directio_on(src->file->fd) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, ngx_errno,
                              ngx_directio_on_n " \"%s\" failed",
                              src->file->name.data);
            }

            ngx_set_errno(err);

            ctx->unaligned = 0;
        }

#endif

        if (n == NGX_ERROR) {
            return (ngx_int_t) n;
        }

        if (n != size) {
            ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, 0,
                          ngx_read_file_n " read only %z of %O from \"%s\"",
                          n, size, src->file->name.data);
            return NGX_ERROR;
        }

        /* �Ѷ�ȡ�������ݷ���dstͬʱָ����д�����Ѿ����ļ��ж�ȡ�浽�ڴ��� */
        
        dst->last += n; //pos��lstָ����ƶ�n�ֽڣ���ʾ�ڴ��ж�����ô�࣬ע��posû���ƶ�

        if (sendfile) { //�����sendfile��ͨ�������ngx_read_file��Ӵ����ļ���ȡһ�ݵ��û��ռ�
            dst->in_file = 1; //��ʶ��buf����in_file
            dst->file = src->file;
            dst->file_pos = src->file_pos;
            dst->file_last = src->file_pos + n;

        } else {
            dst->in_file = 0; //����sendfile�ģ�ֱ�Ӱ�in_file��0
        }

        src->file_pos += n; //file_pos�����ƶ�n�ֽڣ���ʾ��n�ֽ��Ѿ���ȡ���ڴ���

        if (src->file_pos == src->file_last) { //�����е������Ѿ�ȫ����ȡ��Ӧ�ò��ڴ���  
            //����������һ����������ݣ����ͨ������ı�ʶ֪ͨwirteģ�飬����ֱ��write��ȥ��
            dst->flush = src->flush;
            dst->last_buf = src->last_buf;
            dst->last_in_chain = src->last_in_chain;
        }
    }

    return NGX_OK;
}

//���˷�������ĵ��ù���ngx_http_upstream_send_request_body->ngx_output_chain->ngx_chain_writer
ngx_int_t
ngx_chain_writer(void *data, ngx_chain_t *in)
{
    ngx_chain_writer_ctx_t *ctx = data;

    off_t              size;
    ngx_chain_t       *cl, *ln, *chain;
    ngx_connection_t  *c;

    c = ctx->connection;
    /*�����ѭ������in�����ÿһ�����ӽڵ㣬��ӵ�ctx->filter_ctx��ָ�������С�����¼��Щin������Ĵ�С��*/
    for (size = 0; in; in = in->next) {

#if 1
        if (ngx_buf_size(in->buf) == 0 && !ngx_buf_special(in->buf)) {

            ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, 0,
                          "zero size buf in chain writer "
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

            continue;
        }
#endif

        size += ngx_buf_size(in->buf);

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, c->log, 0,
                       "chain writer buf fl:%d s:%uO",
                       in->buf->flush, ngx_buf_size(in->buf));

        cl = ngx_alloc_chain_link(ctx->pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = in->buf; //��in->buf��ֵ���µ�cl->buf��
        cl->next = NULL;
        //����������ʵ���Ͼ��ǰ�cl��ӵ�ctx->out����ͷ�У�
        *ctx->last = cl; 
        ctx->last = &cl->next; //����ƶ�lastָ�룬ָ���µ����һ���ڵ��next������ַ���ٴ�ѭ���ߵ������ʱ�򣬵���ctx->last=cl����µ�cl��ӵ�out��β��
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, c->log, 0,
                   "chain writer in: %p", ctx->out);
                   
    //�����ո�׼����������ͳ�����С������ɶ��˼?ctx->outΪ����ͷ��������������������еġ�
    for (cl = ctx->out; cl; cl = cl->next) {

#if 1
        if (ngx_buf_size(cl->buf) == 0 && !ngx_buf_special(cl->buf)) {

            ngx_log_error(NGX_LOG_ALERT, ctx->pool->log, 0,
                          "zero size buf in chain writer "
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

            continue;
        }
#endif

        size += ngx_buf_size(cl->buf);
    }

    if (size == 0 && !c->buffered) {//ɶ���ݶ�ô�У����÷��˶�
        return NGX_OK;
    }

    //����writev��ctx->out������ȫ�����ͳ�ȥ�����û�����꣬�򷵻�û������ϵĲ��֡���¼��out����
	//��ngx_event_connect_peer�������η�������ʱ�����õķ������Ӻ���ngx_send_chain=ngx_writev_chain��
    chain = c->send_chain(c, ctx->out, ctx->limit); //ngx_send_chain->ngx_writev_chain  ����˵��������ǲ�����filter����ģ��ģ�����ֱ�ӵ���ngx_writev_chain->ngx_writev���͵����

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, c->log, 0,
                   "chain writer out: %p", chain);

    if (chain == NGX_CHAIN_ERROR) {
        return NGX_ERROR;
    }

    for (cl = ctx->out; cl && cl != chain; /* void */) { //��ctx->out���Ѿ�ȫ�����ͳ�ȥ��in�ڵ��out����ժ������free�У��ظ�����
        ln = cl;
        cl = cl->next;
        ngx_free_chain(ctx->pool, ln);
    }

    ctx->out = chain; //ctx->out��������ֻʣ�»�û�з��ͳ�ȥ��in�ڵ���

    if (ctx->out == NULL) { //˵���Ѿ�ctx->out���е����������Ѿ�ȫ���������
        ctx->last = &ctx->out;

        if (!c->buffered) { 
        //���͵���˵�������֮ǰbufferedһֱ��û�в�����Ϊ0�������Ӧ����ͻ��˵���Ӧ����buffered�����ڽ���ngx_http_write_filter����
        //c->send_chain()֮ǰ�Ѿ��и�ֵ�������͸��ͻ��˰����ʱ��ᾭ�����е�filterģ���ߵ�����
            return NGX_OK;
        }
    }

    return NGX_AGAIN; //��������chain = c->send_chain(c, ctx->out, ctx->limit)��out�л��������򷵻�NGX_AGAIN�ȴ��ٴ��¼���������
}
