
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_event_pipe.h>


static ngx_int_t ngx_event_pipe_read_upstream(ngx_event_pipe_t *p);
static ngx_int_t ngx_event_pipe_write_to_downstream(ngx_event_pipe_t *p);

static ngx_int_t ngx_event_pipe_write_chain_to_temp_file(ngx_event_pipe_t *p);
static ngx_inline void ngx_event_pipe_remove_shadow_links(ngx_buf_t *buf);
static ngx_int_t ngx_event_pipe_drain_chains(ngx_event_pipe_t *p);

/*
����buffering��ʱ��ʹ��event_pipe�������ݵ�ת��������ngx_event_pipe_write_to_downstream������ȡ���ݣ����߷������ݸ��ͻ��ˡ�
ngx_event_pipe��upstream��Ӧ���ͻؿͻ��ˡ�do_write�����Ƿ�Ҫ���ͻ��˷��ͣ�д���ݡ�
��������ˣ���ô���ȷ����ͻ��ˣ��ٶ�upstream���ݣ���Ȼ�������ȡ�����ݣ�Ҳ���������ġ�
*/ //ngx_event_pipe->ngx_event_pipe_write_to_downstream
ngx_int_t
ngx_event_pipe(ngx_event_pipe_t *p, ngx_int_t do_write)
{//ע���ߵ������ʱ�򣬺�˷��͵�ͷ������Ϣ�Ѿ���ǰ���ngx_http_upstream_send_response->ngx_http_send_header�Ѿ���ͷ���в��ַ��͸��ͻ�����
//�ú��������ֻ�Ǻ�˷Żع�������ҳ���岿��
    ngx_int_t     rc;
    ngx_uint_t    flags;
    ngx_event_t  *rev, *wev;
    
    //���forѭ���ǲ��ϵ���ngx_event_pipe_read_upstream��ȡ�ͻ������ݣ�Ȼ�����ngx_event_pipe_write_to_downstream
    for ( ;; ) {
        if (do_write) { //ע�������do_write��Ϊ1����д������Դ�ѭ����Ϊ0���ȶ���д���Դ�ѭ��
            p->log->action = "sending to client";

            rc = ngx_event_pipe_write_to_downstream(p);
            

            if (rc == NGX_ABORT) {
                return NGX_ABORT;
            }

            if (rc == NGX_BUSY) {
                return NGX_OK;
            }
        }

        p->read = 0;
        p->upstream_blocked = 0;

        p->log->action = "reading upstream";
        
        //��upstream��ȡ���ݵ�chain���������棬Ȼ����������ĵ���input_filter����Э��Ľ���������HTTP��������p->in��p->last_in���������档
        if (ngx_event_pipe_read_upstream(p) == NGX_ABORT) {
            return NGX_ABORT;
        }

        /* ��cachable��ʽ�£�ָ���ڴ����������ݻ�û�ж��������£������Ǻ�˰����ȡ��ϣ��������ﷵ�أ���������¶�����������һֱѭ�� */

        //p->read��ֵ���Բο�ngx_event_pipe_read_upstream->p->upstream->recv_chain()->ngx_readv_chain�����Ƿ�ֵΪ0
        //upstream_blocked����ngx_event_pipe_read_upstream�������õı���,�����Ƿ��������Ѿ���upstream��ȡ�ˡ�
        if (!p->read && !p->upstream_blocked) { //�ں˻����������Ѿ����꣬���߱���ָ���ڴ��Ѿ����꣬���Ƴ�
            break; //��ȡ��˷���NGX_AGAIN��read��0
        }

        do_write = 1;//��Ҫд����Ϊ����ζ�����һЩ����
    }

    if (p->upstream->fd != (ngx_socket_t) -1) {
        rev = p->upstream->read;

        flags = (rev->eof || rev->error) ? NGX_CLOSE_EVENT : 0;

        //�õ�������ӵĶ�д�¼��ṹ������䷢���˴�����ô�����д�¼�ע��ɾ���������򱣴�ԭ����
        if (ngx_handle_read_event(rev, flags, NGX_FUNC_LINE) != NGX_OK) {
            return NGX_ABORT;
        }

        if (!rev->delayed) {
            if (rev->active && !rev->ready) {//û�ж�д�����ˣ��Ǿ�����һ������ʱ��ʱ��
                ngx_add_timer(rev, p->read_timeout, NGX_FUNC_LINE); //���ֶ�ȡ���������ϣ���ӳ�ʱ��ʱ���������������ʱ�䵽��û���ݣ���ʾ��ʱ

            } else if (rev->timer_set) {
             /*
                ����ɾ���Ķ�ʱ���Ƿ������ݵ���˺���Ҫ�ȴ����Ӧ����
                ngx_http_upstream_send_request->ngx_add_timer(c->read, u->conf->read_timeout, NGX_FUNC_LINE); ����ӵĶ�ʱ�� 
                */
                ngx_del_timer(rev, NGX_FUNC_LINE);
            }
        }
    }

    if (p->downstream->fd != (ngx_socket_t) -1
        && p->downstream->data == p->output_ctx)
    {
        wev = p->downstream->write;
        if (ngx_handle_write_event(wev, p->send_lowat, NGX_FUNC_LINE) != NGX_OK) {
            return NGX_ABORT;
        }

        if (!wev->delayed) {
            if (wev->active && !wev->ready) { //��ͻ��˵�д��ʱ����
                ngx_add_timer(wev, p->send_timeout, NGX_FUNC_LINE);

            } else if (wev->timer_set) {
                ngx_del_timer(wev, NGX_FUNC_LINE);
            }
        }
    }

    return NGX_OK;
}

/*
1.��preread_bufs��free_raw_bufs����ngx_create_temp_bufѰ��һ����еĻ򲿷ֿ��е��ڴ棻
2.����p->upstream->recv_chain==ngx_readv_chain����writev�ķ�ʽ��ȡFCGI������,���chain��
3.��������buf�����˵�chain�ڵ����input_filter(ngx_http_fastcgi_input_filter)����upstreamЭ�����������FCGIЭ�飬������Ľ������p->in���棻
4.����û���������buffer�ڵ㣬����free_raw_bufs�Դ��´ν���ʱ�Ӻ������׷�ӡ�
5.��Ȼ�ˣ�����Զ˷���������FIN�ˣ��Ǿ�ֱ�ӵ���input_filter����free_raw_bufs������ݡ�
*/
/*
    buffering��ʽ��������ǰ���ȿ���һ���ռ䣬��ngx_event_pipe_read_upstream->ngx_readv_chain�п���һ��ngx_buf_t(buf1)�ṹָ����������ݣ�
Ȼ���ڶ�ȡ���ݵ�in�����ʱ����ngx_http_fastcgi_input_filter�����´���һ��ngx_buf_t(buf1)������������buf1->shadow=buf2->shadow
buf2->shadow=buf1->shadow��ͬʱ��buf2��ӵ�p->in�С���ͨ��ngx_http_write_filter�������ݵ�ʱ����p->in�е�������ӵ�ngx_http_request_t->out��Ȼ���ͣ�
���һ��û�з�����ɣ������ڵ����ݻ�����ngx_http_request_t->out�У���д�¼������ٴη��͡�������ͨ��p->output_filter(p->output_ctx, out)���ͺ�buf2
�ᱻ��ӵ�p->free�У�buf1�ᱻ��ӵ�free_raw_bufs�У���ngx_event_pipe_write_to_downstream
*/
static ngx_int_t
ngx_event_pipe_read_upstream(ngx_event_pipe_t *p) 
//ngx_event_pipe_write_to_downstreamд���ݵ��ͻ��ˣ�ngx_event_pipe_read_upstream�Ӻ�˶�ȡ����
{//ngx_event_pipe���������ȡ��˵����ݡ�
    off_t         limit;
    ssize_t       n, size;
    ngx_int_t     rc;
    ngx_buf_t    *b;
    ngx_msec_t    delay;
    ngx_chain_t  *chain, *cl, *ln;
    int upstream_eof = 0;
    int upstream_error = 0;
    int single_buf = 0;
    int leftsize = 0;
    int upstream_done = 0;
    ngx_chain_t  *free_raw_bufs = NULL;

    if (p->upstream_eof || p->upstream_error || p->upstream_done) {
        return NGX_OK;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                   "pipe read upstream, read ready: %d", p->upstream->read->ready);

    for ( ;; ) {
        //���ݶ�ȡ��ϣ����߳���ֱ���˳�ѭ��
        if (p->upstream_eof || p->upstream_error || p->upstream_done) {
            break;
        }

        //���û��Ԥ�����ݣ����Ҹ�upstream�����ӻ�û��read���ǾͿ����˳��ˣ���Ϊû���ݿɶ���
        if (p->preread_bufs == NULL && !p->upstream->read->ready) { //������Э��ջ���ݶ�ȡ��ϣ�����NGX_AGAIN����ready����0
            break;
        }

        /*
          ����������if-else�͸�һ������: Ѱ��һ����е��ڴ滺���������������Ŷ�ȡ������upstream�����ݡ�
		���preread_bufs��Ϊ�գ�������֮�����򿴿�free_raw_bufs��û�У���������һ��
          */
        if (p->preread_bufs) {

            /* use the pre-read bufs if they exist */

            chain = p->preread_bufs;
            p->preread_bufs = NULL;
            n = p->preread_size;

            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                           "pipe preread: %z", n); //���Ƕ�ȡͷ���е���Ϣ��ʱ��˳����ȡ���İ��峤��

            if (n) {
                p->read = 1; //��ʾ�ж����ͻ��˰���
            }
        } else {
#if (NGX_HAVE_KQUEUE)

            /*
             * kqueue notifies about the end of file or a pending error.
             * This test allows not to allocate a buf on these conditions
             * and not to call c->recv_chain().
             */

            if (p->upstream->read->available == 0
                && p->upstream->read->pending_eof)
            {
                p->upstream->read->ready = 0;
                p->upstream->read->eof = 1;
                p->upstream_eof = 1;
                p->read = 1;

                if (p->upstream->read->kq_errno) {
                    p->upstream->read->error = 1;
                    p->upstream_error = 1;
                    p->upstream_eof = 0;

                    ngx_log_error(NGX_LOG_ERR, p->log,
                                  p->upstream->read->kq_errno,
                                  "kevent() reported that upstream "
                                  "closed connection");
                }

                break;
            }
#endif

            if (p->limit_rate) {
                if (p->upstream->read->delayed) {
                    break;
                }

                limit = (off_t) p->limit_rate * (ngx_time() - p->start_sec + 1)
                        - p->read_length;

                if (limit <= 0) {
                    p->upstream->read->delayed = 1;
                    delay = (ngx_msec_t) (- limit * 1000 / p->limit_rate + 1);
                    ngx_add_timer(p->upstream->read, delay, NGX_FUNC_LINE);
                    break;
                }

            } else {
                limit = 0;
            }

            if (p->free_raw_bufs) { //�ϴη�����chain->buf�󣬵���ngx_readv_chain��ȡ���ݵ�ʱ�򷵻�NGX_AGAIN,������µ�epoll���¼�������ֱ��ʹ���ϴ�û���õ�chain�����¶�ȡ����
                //�������n = p->upstream->recv_chain����NGX_AGAIN,�´�epoll�ٴδ�������ʱ��ֱ����free_raw_bufs

                /* use the free bufs if they exist */

                chain = p->free_raw_bufs;
                if (p->single_buf) { //���������NGX_USE_AIO_EVENT��־�� the posted aio operation may currupt a shadow buffer
                    p->free_raw_bufs = p->free_raw_bufs->next;
                    chain->next = NULL;
                } else { //�������AIO����ô�����ö���ڴ�һ����readv��ȡ�ġ�
                    p->free_raw_bufs = NULL;
                }

            } else if (p->allocated < p->bufs.num) {

                /* allocate a new buf if it's still allowed */
                /*
                    ���û�г���fastcgi_buffers��ָ������ƣ���ô����һ���ڴ�ɡ���Ϊ����û�п����ڴ��ˡ�
                    allocate a new buf if it's still allowed����һ��ngx_buf_t�Լ�size��С�����ݡ������洢��FCGI��ȡ�����ݡ�
                    */
                b = ngx_create_temp_buf(p->pool, p->bufs.size);
                if (b == NULL) {
                    return NGX_ABORT;
                }

                p->allocated++;

                chain = ngx_alloc_chain_link(p->pool);
                if (chain == NULL) {
                    return NGX_ABORT;
                }

                chain->buf = b;
                chain->next = NULL;

            } else if (!p->cacheable
                       && p->downstream->data == p->output_ctx
                       && p->downstream->write->ready
                       && !p->downstream->write->delayed)
            {
            //û�п������ɣ�����ǰ���Ѿ�������5��3Kbuf�Ѿ��������ˣ������ڷ���ռ���
            //�������˵��û�������ڴ��ˣ�������������ûҪ������ȱ�����cache������ǿ��԰ɵ�ǰ�����ݷ��͸��ͻ����ˡ�����ѭ������write������Ȼ��ͻ���ദ�ռ�����������
                /*
                 * if the bufs are not needed to be saved in a cache and
                 * a downstream is ready then write the bufs to a downstream
                 */

                p->upstream_blocked = 1; 

                ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "pipe downstream ready");

                break;

            } else if (p->cacheable
                       || p->temp_file->offset < p->max_temp_file_size)  //���������ݳ�����max_temp_file_size���򲻻���
            
            /* ��ǰfastcgi_buffers ��fastcgi_buffer_size���õĿռ䶼�Ѿ������ˣ�����Ҫ�Ѷ�ȡ��(����fastcgi_buffers ��
                fastcgi_buffer_sizeָ���Ŀռ��б���Ķ�ȡ����)������д����ʱ�ļ���ȥ */ 
            
            {//���뻺�棬���ҵ�ǰ�Ļ����ļ���λ�ƣ����СС�ڿ�����Ĵ�С����good������д���ļ��ˡ�
             //������Կ������ڿ���cache��ʱ��ֻ��ǰ���fastcgi_buffers  5 3K���Ѿ������ˣ��Ż�д����ʱ�ļ���ȥ//���潫r->in������д����ʱ�ļ�
                /*
                 * if it is allowed, then save some bufs from p->in
                 * to a temporary file, and add them to a p->out chain
                 */

                rc = ngx_event_pipe_write_chain_to_temp_file(p);//���潫r->in������д����ʱ�ļ�

                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "pipe temp offset: %O", p->temp_file->offset);

                if (rc == NGX_BUSY) {
                    break;
                }

                if (rc == NGX_AGAIN) {
                    if (ngx_event_flags & NGX_USE_LEVEL_EVENT
                        && p->upstream->read->active
                        && p->upstream->read->ready)
                    {
                        if (ngx_del_event(p->upstream->read, NGX_READ_EVENT, 0)
                            == NGX_ERROR)
                        {
                            return NGX_ABORT;
                        }
                    }
                }

                if (rc != NGX_OK) {
                    return rc;
                }

                chain = p->free_raw_bufs;
                if (p->single_buf) {
                    p->free_raw_bufs = p->free_raw_bufs->next;
                    chain->next = NULL;
                } else {
                    p->free_raw_bufs = NULL;
                }

            } else {

                /* there are no bufs to read in */

                ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "no pipe bufs to read in");

                break;
            }

        //������϶����ҵ����е�buf�ˣ�chainָ��֮�ˡ�ngx_readv_chain .����readv���ϵĶ�ȡ���ӵ����ݡ�����chain���������������
        //chain�ǲ���ֻ��һ��? ��next��ԱΪ���أ���һ�������free_raw_bufs��Ϊ�գ�����Ļ�ȡ����bufֻҪû��ʹ��AIO�Ļ����Ϳ����ж��buffer����ġ�
        //ע��:������ֻ�ǰѶ��������ݷ�����chain->buf�У�����û���ƶ�β��lastָ�룬ʵ���ϸú������غ�pos��last������ָ���ȡ���ݵ�ͷ����
            n = p->upstream->recv_chain(p->upstream, chain, limit); //chain->buf�ռ������洢recv_chain�Ӻ�˽��յ�������

            leftsize = chain->buf->end - chain->buf->last;
            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                           "pipe recv chain: %z, left-size:%d", n, leftsize);

            if (p->free_raw_bufs) { //free_raw_bufs��Ϊ�գ��Ǿͽ�chainָ������ŵ�free_raw_bufsͷ����
                chain->next = p->free_raw_bufs;
            }
            p->free_raw_bufs = chain; //�Ѷ�ȡ���Ĵ��к�����ݵ�chain��ֵ��free_raw_bufs

            if (n == NGX_ERROR) {
                p->upstream_error = 1;
                return NGX_ERROR;
            }

            if (n == NGX_AGAIN) { //ѭ����ȥͨ��epoll���¼�����������,һ�㶼�ǰ��ں˻��������ݶ��������ﷵ��
                if (p->single_buf) {
                    ngx_event_pipe_remove_shadow_links(chain->buf);
                }

                single_buf = p->single_buf;
                /*
                    2025/04/27 00:40:55[                    ngx_readv_chain,   179]  [debug] 22653#22653: *3 readv() not ready (11: Resource temporarily unavailable)
                    2025/04/27 00:40:55[       ngx_event_pipe_read_upstream,   337]  [debug] 22653#22653: *3 pipe recv chain: -2
                    */
                ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "ngx_event_pipe_read_upstream recv return ngx_again, single_buf:%d ", single_buf);
                break; //���´�epoll�ٴδ�������ʱ�򣬾�ֱ��ʹ��p->free_raw_bufs
            }

            p->read = 1; //��ʾ�ж������ݣ����ҿ��Լ�����

            if (n == 0) { 
                p->upstream_eof = 1;//upstream_eof��ʾ�ں�Э��ջ�Ѿ���ȡ��ϣ��ں�Э��ջ�Ѿ�û�������ˣ���Ҫ�ٴ�epoll����������
                break; //����ѭ��
            }
        } //������forѭ���տ�ʼ��if (p->preread_bufs) {���������Ѱ��һ�����еĻ�������Ȼ���ȡ�������chain�������ġ�
//��ȡ�����ݣ�����Ҫ����FCGIЭ������������ˡ�

        delay = p->limit_rate ? (ngx_msec_t) n * 1000 / p->limit_rate : 0;

        p->read_length += n; //��ʾ��ȡ���ĺ�˰��岿�����ݳ�������n�ֽ�
        cl = chain; //cl������ǰ��ngx_readv_chain��ʱ���ȡ������
        p->free_raw_bufs = NULL;

        while (cl && n > 0) {

		    //����ĺ�����c->buf����shadowָ���������������������нڵ��recycled,temporary,shadow��Ա�ÿա�
            ngx_event_pipe_remove_shadow_links(cl->buf);

            /* ǰ���n = p->upstream->recv_chain()��ȡ���ݺ�û���ƶ�lastָ�룬ʵ���ϸú������غ�pos��last������ָ���ȡ���ݵ�ͷ���� */
            size = cl->buf->end - cl->buf->last; //buf��ʣ��Ŀռ�

            if (n >= size) { //��ȡ�����ݱȵ�һ��cl->buf(Ҳ����chain->buf)�࣬˵�����������ݿ��԰ѵ�һ��buf����
                cl->buf->last = cl->buf->end; //������ȫ������,readv��������ݡ�

                /* STUB */ cl->buf->num = p->num++; //�ڼ��飬cl����(cl->next)�еĵڼ���

                //��Ҫ���ܾ��ǽ���fastcgi��ʽ���壬����������󣬰Ѷ�Ӧ��buf���뵽p->in
                //FCGIΪngx_http_fastcgi_input_filter������Ϊngx_event_pipe_copy_input_filter �����������ض���ʽ����
                if (p->input_filter(p, cl->buf) == NGX_ERROR) { //����buffer�ĵ���Э��������
                    //�����棬���cl->buf������ݽ���������DATA���ݣ���ôcl->buf->shadow��Աָ��һ������
                //ͨ��shadow��Ա��������������ÿ����Ա������ɢ��fcgi data���ݲ��֡�
                    
                    return NGX_ABORT;
                }

                n -= size;

                //����������һ�飬���ͷ�����ڵ㡣
                ln = cl;
                cl = cl->next; 
                ngx_free_chain(p->pool, ln);

            } else {  //˵�����ζ�����n�ֽ����ݲ���װ��һ��buf�����ƶ�lastָ�룬ͬʱ���س�ȥ������

            //�������ڵ�Ŀ����ڴ���Ŀ����ʣ��Ҫ����ģ��ͽ�ʣ�µĴ������� ͨ�������if (p->free_raw_bufs && p->length != -1){}ִ��p->input_filter(p, cl->buf)
                /*
                    ɶ��˼�����õ���input_filter���𣬲��ǡ��������ģ����ʣ�µ�������ݻ�����������ǰ���cl�Ļ����С��
                    �Ǿ��ȴ���������ô����: ���ͷ�cl�ˣ�ֻ���ƶ����С��Ȼ��n=0ʹѭ���˳���Ȼ�������漸�е�if (cl) {������Լ�⵽�������
                    �����������if����Ὣ���ln�������ݷ���free_raw_bufs��ͷ��������������ж��������? �����еġ�
                    */
                cl->buf->last += n;
                n = 0;
            }
        }

        if (cl) {
            //������û������һ���ڴ����������ӷŵ�free_raw_bufs��ǰ�档ע�������޸���cl->buf->last�������Ķ������ݲ���
            //������Щ���ݵġ���ngx_readv_chainȻ�������
            for (ln = cl; ln->next; ln = ln->next) { /* void */ }  

            ln->next = p->free_raw_bufs;
            p->free_raw_bufs = cl;
        }

        if (delay > 0) {
            p->upstream->read->delayed = 1;
            ngx_add_timer(p->upstream->read, delay, NGX_FUNC_LINE);
            break;
        }
    }//ע��������forѭ����ֻ������p->upstream_eof || p->upstream_error || p->upstream_done���Ƴ�

#if (NGX_DEBUG)

    for (cl = p->busy; cl; cl = cl->next) {
        ngx_log_debug8(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe buf busy s:%d t:%d f:%d "
                       "%p, pos %p, size: %z "
                       "file: %O, size: %O",
                       (cl->buf->shadow ? 1 : 0),
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

    for (cl = p->out; cl; cl = cl->next) {
        ngx_log_debug8(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe buf out  s:%d t:%d f:%d "
                       "%p, pos %p, size: %z "
                       "file: %O, size: %O",
                       (cl->buf->shadow ? 1 : 0),
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

    for (cl = p->in; cl; cl = cl->next) {
        ngx_log_debug8(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe buf in   s:%d t:%d f:%d "
                       "%p, pos %p, size: %z "
                       "file: %O, size: %O",
                       (cl->buf->shadow ? 1 : 0),
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

    for (cl = p->free_raw_bufs; cl; cl = cl->next) {
        ngx_log_debug8(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe buf free s:%d t:%d f:%d "
                       "%p, pos %p, size: %z "
                       "file: %O, size: %O",
                       (cl->buf->shadow ? 1 : 0),
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

    upstream_eof = p->upstream_eof;
    upstream_error = p->upstream_error;
    free_raw_bufs = p->free_raw_bufs;
    upstream_done = p->upstream_done;
    ngx_log_debug5(NGX_LOG_DEBUG_EVENT, p->log, 0,
                   "pipe length: %O, p->upstream_eof:%d, p->upstream_error:%d, p->free_raw_bufs:%p, upstream_done:%d", 
                   p->length, upstream_eof, upstream_error, free_raw_bufs, upstream_done);

#endif

    if (p->free_raw_bufs && p->length != -1) { //ע��ǰ���Ѿ��Ѷ�ȡ����chain���ݼ��뵽��free_raw_bufs
        cl = p->free_raw_bufs;

        if (cl->buf->last - cl->buf->pos >= p->length) { //�����ȡ���

            p->free_raw_bufs = cl->next;

            /* STUB */ cl->buf->num = p->num++;

            //��Ҫ���ܾ��ǽ���fastcgi��ʽ���壬����������󣬰Ѷ�Ӧ��buf���뵽p->in
            //FCGIΪngx_http_fastcgi_input_filter������Ϊngx_event_pipe_copy_input_filter �����������ض���ʽ����
            if (p->input_filter(p, cl->buf) == NGX_ERROR) {
                 return NGX_ABORT;
            }

            ngx_free_chain(p->pool, cl);
        }
    }

    if (p->length == 0) { //���ҳ��������ݶ�ȡ��ϻ��߱�����û�а��壬��upstream_done��1
        p->upstream_done = 1;
        p->read = 1;
    }

    //upstream_eof��ʾ�ں�Э��ջ�Ѿ���ȡ��ϣ��ں�Э��ջ�Ѿ�û�������ˣ���Ҫ�ٴ�epoll����������  //ע��ǰ���Ѿ��Ѷ�ȡ����chain���ݼ��뵽��free_raw_bufs
    if ((p->upstream_eof || p->upstream_error) && p->free_raw_bufs) {//û�취�ˣ����쵽ͷ�ˣ����߳��ִ����ˣ����Դ���һ����鲻������buffer

        /* STUB */ p->free_raw_bufs->buf->num = p->num++;
        //������ݶ�ȡ����ˣ����ߺ�˳��������ˣ����ң�free_raw_bufs��Ϊ�գ����滹��һ�������ݣ�
		//��Ȼֻ������һ�顣�Ǿ͵���input_filter��������FCGIΪngx_http_fastcgi_input_filter ��ngx_http_fastcgi_handler�������õ�

		//���￼��һ�����: �������һ�������ˣ�û��������û��data���ݣ�����ngx_http_fastcgi_input_filter�����ngx_event_pipe_add_free_buf������
		//������ڴ����free_raw_bufs��ǰ�棬���Ǿ���֪�������һ�鲻�������ݲ��ֵ��ڴ����õ���free_raw_bufs����Ϊfree_raw_bufs��û���ü��ı䡣
		//���ԣ��Ͱ��Լ����滻���ˡ���������ᷢ����?
        if (p->input_filter(p, p->free_raw_bufs->buf) == NGX_ERROR) {
            return NGX_ABORT;
        }

        p->free_raw_bufs = p->free_raw_bufs->next;

        if (p->free_bufs && p->buf_to_file == NULL) {
            for (cl = p->free_raw_bufs; cl; cl = cl->next) {
                if (cl->buf->shadow == NULL) {
                //���shadow��Աָ���������buf������СFCGI���ݿ�buf��ָ���б����ΪNULL����˵�����bufû��data�������ͷ��ˡ�
                    ngx_pfree(p->pool, cl->buf->start);
                }
            }
        }
    }

    if (p->cacheable && (p->in || p->buf_to_file)) {  

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0, "pipe write chain");

        if (ngx_event_pipe_write_chain_to_temp_file(p) == NGX_ABORT) {
            return NGX_ABORT;
        }
    }

    upstream_done = p->upstream_done;
    if(upstream_done)
        ngx_log_debugall(p->log, 0, "pipe read upstream upstream_done:%d", upstream_done);

    return NGX_OK;
}

/*
    buffering��ʽ��������ǰ���ȿ���һ���ռ䣬��ngx_event_pipe_read_upstream->ngx_readv_chain�п���һ��ngx_buf_t(buf1)�ṹָ����������ݣ�
Ȼ���ڶ�ȡ���ݵ�in�����ʱ����ngx_http_fastcgi_input_filter�����´���һ��ngx_buf_t(buf1)������������buf1->shadow=buf2->shadow
buf2->shadow=buf1->shadow��ͬʱ��buf2��ӵ�p->in�С���ͨ��ngx_http_write_filter�������ݵ�ʱ����p->in�е�������ӵ�ngx_http_request_t->out��Ȼ���ͣ�
���һ��û�з�����ɣ������ڵ����ݻ�����ngx_http_request_t->out�У���д�¼������ٴη��͡�������ͨ��p->output_filter(p->output_ctx, out)���ͺ�buf2
�ᱻ��ӵ�p->free�У�buf1�ᱻ��ӵ�free_raw_bufs�У���ngx_event_pipe_write_to_downstream
*/
//ngx_event_pipe_write_to_downstreamд���ݵ��ͻ��ˣ�ngx_event_pipe_read_upstream�Ӻ�˶�ȡ����
static ngx_int_t
ngx_event_pipe_write_to_downstream(ngx_event_pipe_t *p) 
{//ngx_event_pipe��������������ݷ��͸��ͻ��ˣ������Ѿ�׼����p->out,p->in�����ˡ�
    u_char            *prev;
    size_t             bsize;
    ngx_int_t          rc;
    ngx_uint_t         flush, flushed, prev_last_shadow;
    ngx_chain_t       *out, **ll, *cl;
    ngx_connection_t  *downstream;

    downstream = p->downstream;   //��ͻ��˵�������Ϣ

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                   "pipe write downstream, write ready: %d", downstream->write->ready);

    flushed = 0;

    for ( ;; ) {
        if (p->downstream_error) { //����ͻ������ӳ����ˡ�drain=��ˮ������,
            return ngx_event_pipe_drain_chains(p);//���upstream�������ģ���������ʽ���HTML���ݡ��������free_raw_bufs���档
        }

        /*
         ngx_event_pipe_write_to_downstream
         if (p->upstream_eof || p->upstream_error || p->upstream_done) {
            p->output_filter(p->output_ctx, p->out);
         }
          */

        //upstream_eof��ʾ�ں˻����������Ѿ����� ���upstream�������Ѿ��ر��ˣ���������ˣ����߷�������ˣ��ǾͿ��Է����ˡ�
        if (p->upstream_eof || p->upstream_error || p->upstream_done) {
            //ʵ�����ڽ����������ݺ�����ͻ��˷��Ͱ��岿�ֵ�ʱ�򣬻����ε��øú�����һ����ngx_event_pipe_write_to_downstream-> p->output_filter(),
            //��һ����ngx_http_upstream_finalize_request->ngx_http_send_special,
            
            /* pass the p->out and p->in chains to the output filter */

            for (cl = p->busy; cl; cl = cl->next) {
                cl->buf->recycled = 0;//����Ҫ�����ظ������ˣ���Ϊupstream_done�ˣ������ٸ��ҷ��������ˡ�
            }


/*
���ͻ����ļ������ݵ��ͻ��˹���:
 ngx_http_file_cache_open->ngx_http_file_cache_read->ngx_http_file_cache_aio_read������̻�ȡ�ļ���ǰ���ͷ����Ϣ������ݣ�����ȡ����
 �ļ�stat��Ϣ�������ļ���С�ȡ�
 ͷ��������ngx_http_cache_send->ngx_http_send_header���ͣ�
 �����ļ�����İ��岿����ngx_http_cache_send��벿�����д�����filterģ���з���

 ���պ�����ݲ�ת�����ͻ��˴������ݷ��͹���:
 ngx_event_pipe_write_to_downstream�е�
 if (p->upstream_eof || p->upstream_error || p->upstream_done) {
    ����p->in ���߱���p->out��Ȼ��ִ�����
    p->output_filter(p->output_ctx, p->out);
 }
 */
            //���û�п������棬���ݲ���д����ʱ�ļ��У�p->out = NULL
            if (p->out) {  //����ʱ�ļ����,������ɴ�������ʱ�ļ��У�������
                ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "pipe write downstream flush out");

                for (cl = p->out; cl; cl = cl->next) {
                    cl->buf->recycled = 0;
                }

                //���棬��Ϊp->out����������һ��鶼�ǽ�����ĺ�˷�����ҳ�����ݣ�����ֱ�ӵ���ngx_http_output_filter�������ݷ��;����ˡ�
                //ע��: û�з�����ϵ����ݻᱣ�浽ngx_http_request_t->out�У�HTTP��ܻᴥ���ٴΰ�r->outд��ȥ�������Ǵ���p->out�е�
                rc = p->output_filter(p->output_ctx, p->out);

                if (rc == NGX_ERROR) {
                    p->downstream_error = 1;
                    return ngx_event_pipe_drain_chains(p);
                }

                p->out = NULL;
            }

            //ngx_event_pipe_read_upstream��ȡ���ݺ�ͨ��ngx_http_fastcgi_input_filter�Ѷ�ȡ�������ݼ��뵽p->in����
            //����������棬������д����ʱ�ļ��У�p->in=NULL
            if (p->in) { //��outͬ���򵥵���ngx_http_output_filter�������filter���͹����С�
                ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "pipe write downstream flush in");

                for (cl = p->in; cl; cl = cl->next) {
                    cl->buf->recycled = 0; //�Ѿ��������ˣ�����Ҫ������
                }

                //ע������ķ��Ͳ������writev�ˣ��ÿ�������������Ƿ���Ҫrecycled,�Ƿ������һ��ȡ�ngx_http_write_filter���ж�����ġ�
                rc = p->output_filter(p->output_ctx, p->in);//����ngx_http_output_filter���ͣ����һ����ngx_http_write_filter

                if (rc == NGX_ERROR) {
                    p->downstream_error = 1;
                    return ngx_event_pipe_drain_chains(p);
                }

                p->in = NULL; //��ִ�������output_filter()��p->in�е����ݻ���ӵ�r->out��
            }

            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, p->log, 0,  "pipe write downstream done");

            /* TODO: free unused bufs */

            p->downstream_done = 1;
            break; //������˳�ѭ��
        }

        //����upstream���ݻ�û�з�����ϡ�
        if (downstream->data != p->output_ctx
            || !downstream->write->ready
            || downstream->write->delayed)
        {
            break;
        }

        /* bsize is the size of the busy recycled bufs */

        prev = NULL;
        bsize = 0;

        //���������Ҫbusy������ڷ��ͣ��Ѿ����ù�output_filter��buf��������һ����Щ���Ի����ظ����õ�buf
        //������Щbuf����������ע�����ﲻ�Ǽ���busy�л��ж�������û������writev��ȥ�����������ܹ����������
        for (cl = p->busy; cl; cl = cl->next) {

            if (cl->buf->recycled) {
                if (prev == cl->buf->start) {
                    continue;
                }

                bsize += cl->buf->end - cl->buf->start; //���㻹û�з��ͳ�ȥ��ngx_buf_t��ָ�����пռ�Ĵ�С
                prev = cl->buf->start;
            }
        }

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe write busy: %uz", bsize);

        out = NULL;

        //busy_sizeΪfastcgi_busy_buffers_size ָ�����õĴ�С��ָ�������͵�busy״̬���ڴ��ܴ�С��
		//������������С��nginx�᳢��ȥ�����µ����ݲ�������Щbusy״̬��buf��
        if (bsize >= (size_t) p->busy_size) {
            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "break while(), bsize:%uz >= (size_t) p->busy_size: %uz", bsize, (size_t) p->busy_size);
            flush = 1;
            goto flush;
        }

        flush = 0;
        ll = NULL;
        prev_last_shadow = 1; //�����һ���ڵ��ǲ���������һ��FCGI buffer�����һ�����ݽڵ㡣

        //����p->out,p->in�����δ�������ݣ������Ƿŵ�out������棬ע�����﷢�͵����ݲ�����busy_size��Ϊ���������ˡ�
        for ( ;; ) {
        //ѭ�������ѭ������ֹ�����Ǿ��ܻ�ü���HTML���ݽڵ㣬�������ǿ�Խ��1�����ϵ�FCGI���ݿ�Ĳ������һ�����last_shadow������
            if (p->out) { //buf��tempfile�����ݻ�ŵ�out���档һ��read��˷�������ݷ���NGX_AGIAN��ʼ���ͻ����е�����
                //˵�����ݻ��浽����ʱ�ļ���
                cl = p->out; 

                if (cl->buf->recycled) {
                    ngx_log_error(NGX_LOG_ALERT, p->log, 0,
                                  "recycled buffer in pipe out chain");
                }

                p->out = p->out->next;

            } else if (!p->cacheable && p->in) { //˵������ʱ���浽�ڴ��е�
                cl = p->in;

                ngx_log_debug3(NGX_LOG_DEBUG_EVENT, p->log, 0,
                               "pipe write buf ls:%d %p %z",
                               cl->buf->last_shadow, //
                               cl->buf->pos,
                               cl->buf->last - cl->buf->pos);


                //1.������in��������ݣ��������Ҫ����;
				//2.��������ĳһ���FCGI buf�����һ����Чhtml���ݽڵ㣻
				//3.���ҵ�ǰ��û���͵Ĵ�С����busy_size, �Ǿ���Ҫ����һ���ˣ���Ϊ������buffer����
                if (cl->buf->recycled && prev_last_shadow) {
                    if (bsize + cl->buf->end - cl->buf->start > p->busy_size) {
                        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "break while(), bsize + cl->buf->end - cl->buf->start:%uz > p->busy_size: %uz", 
                            bsize, (size_t) p->busy_size);
                        flush = 1;//�����˴�С�����һ�´�������Ҫ�������͵ġ������������û���Ӷ������ã���Ϊ���治��ô�жϡ�
                        break;//ֹͣ���������ڴ�飬��Ϊ�����Ѿ�����busy_size�ˡ�
                    }

                    bsize += cl->buf->end - cl->buf->start;
                }

                prev_last_shadow = cl->buf->last_shadow;

                p->in = p->in->next;

            } else {
                break; //һ��
            }

            cl->next = NULL;

            if (out) {
                *ll = cl;
            } else {
                out = cl;//ָ���һ������
            }
            ll = &cl->next;
        }

    //�������outָ��ָ��һ������������������Ǵ�p->out,p->in����Ҫ���͵����ݡ���ngx_http_output_filter
    flush:

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe write: out:%p, flush:%d", out, flush);

        //���潫outָ��ָ����ڴ����output_filter������filter���̡�
        //������������д����ʱ�ļ�����out=NULL��ֻ���ڻ�ȡ��ȫ��������ݲ�д����ʱ�ļ��󣬲Ż�ͨ��ǰ���if (p->upstream_eof || p->upstream_error || p->upstream_done) {p->output_filter()}���ͳ�ȥ
        if (out == NULL) { //�������ngx_chain_update_chains���п�����ΪNULL����ʾout���ϵ����ݷ������

            if (!flush) {
                break;
            }

            /* a workaround for AIO */
            if (flushed++ > 10) { //���ѭ��10�Σ��Ӷ����Դﵽ�첽��Ч�������������淴��ѭ��
                return NGX_BUSY;
            }
        }

        rc = p->output_filter(p->output_ctx, out);//�򵥵���ngx_http_output_filter�������filter���͹����С�

/*
    ������ǰ���ȿ���һ���ռ䣬��ngx_event_pipe_read_upstream->ngx_readv_chain�п���һ��ngx_buf_t(buf1)�ṹָ����������ݣ�
Ȼ���ڶ�ȡ���ݵ�in�����ʱ����ngx_http_fastcgi_input_filter�����´���һ��ngx_buf_t(buf1)������������buf1->shadow=buf2->shadow
buf2->shadow=buf1->shadow��ͬʱ��buf2��ӵ�p->in�С���ͨ��ngx_http_write_filter�������ݵ�ʱ����p->in�е�������ӵ�ngx_http_request_t->out��Ȼ���ͣ�
���һ��û�з�����ɣ���ʣ������ݻ�����p->out�С�������ͨ��p->output_filter(p->output_ctx, out)���ͺ�buf2�ᱻ��ӵ�p->free�У�
buf1�ᱻ��ӵ�free_raw_bufs�У���ngx_event_pipe_write_to_downstream
*/
        //��û��ȫ�����͵�buf(last != end)���뵽busy���Ѿ�ȫ�������˵�buf(end = last)����free��
        //ʵ����p->busy����ָ�����ngx_http_write_filter��δ�������r->out�б�������ݣ���ngx_http_write_filter
        /*ʵ����p->busy����ָ�����ngx_http_write_filter��δ�������r->out�б�������ݣ��ⲿ������ʼ����r->out����ǰ�棬�����ڶ������ݺ���
    ngx_http_write_filter�л�����������ݼӵ�r->out���棬Ҳ����δ���͵�������r->outǰ���������������棬����ʵ��write��֮ǰδ���͵��ȷ��ͳ�ȥ*/
        ngx_chain_update_chains(p->pool, &p->free, &p->busy, &out, p->tag);

        if (rc == NGX_ERROR) {
            p->downstream_error = 1;
            return ngx_event_pipe_drain_chains(p);
        }

        for (cl = p->free; cl; cl = cl->next) { 

            if (cl->buf->temp_file) {
                if (p->cacheable || !p->cyclic_temp_file) {
                    continue;
                }

                /* reset p->temp_offset if all bufs had been sent */

                if (cl->buf->file_last == p->temp_file->offset) {
                    p->temp_file->offset = 0;
                }
            }

            /* TODO: free buf if p->free_bufs && upstream done */

            /* add the free shadow raw buf to p->free_raw_bufs */

            if (cl->buf->last_shadow) {
                if (ngx_event_pipe_add_free_buf(p, cl->buf->shadow) != NGX_OK) { //��ϲο�ngx_http_fastcgi_input_filter�Ķ�
                //Ҳ�����ڶ�ȡ������ݵ�ʱ�򴴽���ngx_buf_t(��ȡ����ʱ�����ĵ�һ��ngx_buf_t)����free_raw_bufs
                    return NGX_ABORT;
                }

                cl->buf->last_shadow = 0;
            }

            cl->buf->shadow = NULL;
        }
    }

    return NGX_OK;
}

/*
2015/12/16 04:25:19[           ngx_event_process_posted,    67]  [debug] 19348#19348: *3 delete posted event B0895098
2015/12/16 04:25:19[          ngx_http_upstream_handler,  1332]  [debug] 19348#19348: *3 http upstream request(ev->write:0): "/test3.php?"
2015/12/16 04:25:19[   ngx_http_upstream_process_header,  2417]  [debug] 19348#19348: *3 http upstream process header, fd:14, buffer_size:512
2015/12/16 04:25:19[                      ngx_unix_recv,   204]  [debug] 19348#19348: *3 recv: fd:14 read-size:367 of 367, ready:1
2015/12/16 04:25:19[    ngx_http_fastcgi_process_record,  3080]  [debug] 19348#19348: *3 http fastcgi record byte: 00
2015/12/16 04:25:19[    ngx_http_fastcgi_process_record,  3152]  [debug] 19348#19348: *3 http fastcgi record length: 8184
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2325]  [debug] 19348#19348: *3 http fastcgi parser: 0
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2433]  [debug] 19348#19348: *3 http fastcgi header: "X-Powered-By: PHP/5.2.13"
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2325]  [debug] 19348#19348: *3 http fastcgi parser: 0
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2433]  [debug] 19348#19348: *3 http fastcgi header: "Content-type: text/html"
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2325]  [debug] 19348#19348: *3 http fastcgi parser: 1
2015/12/16 04:25:19[    ngx_http_fastcgi_process_header,  2449]  [debug] 19348#19348: *3 http fastcgi header done
2015/12/16 04:25:19[               ngx_http_send_header,  3150]  [debug] 19348#19348: *3 ngx http send header

2015/12/16 04:25:19[             ngx_http_header_filter,   677]  [debug] 19348#19348: *3 HTTP/1.1 200 OK
Server: nginx/1.9.2
Date: Tue, 15 Dec 2015 20:25:19 GMT
Content-Type: text/html
Transfer-Encoding: chunked
Connection: keep-alive
X-Powered-By: PHP/5.2.13

2015/12/16 04:25:19[              ngx_http_write_filter,   204]  [debug] 19348#19348: *3 write new buf t:1 f:0 080F2CE8, pos 080F2CE8, size: 180 file: 0, size: 0
2015/12/16 04:25:19[              ngx_http_write_filter,   244]  [debug] 19348#19348: *3 http write filter: l:0 f:0 s:180
2015/12/16 04:25:19[    ngx_http_upstream_send_response,  3120]  [debug] 19348#19348: *3 ngx_http_upstream_send_response, buffering flag:1
2015/12/16 04:25:19[     ngx_http_file_cache_set_header,  1256]  [debug] 19348#19348: *3 http file cache set header
2015/12/16 04:25:19[    ngx_http_upstream_send_response,  3303]  [debug] 19348#19348: *3 http cacheable: 1
2015/12/16 04:25:19[    ngx_http_upstream_send_response,  3343]  [debug] 19348#19348: *3 ngx_http_upstream_send_response, p->cacheable:1, tempfile:/var/yyz/cache_xxx/temp, pathfile:/var/yyz/cache_xxx
2015/12/16 04:25:19[ ngx_http_upstream_process_upstream,  4052]  [debug] 19348#19348: *3 http upstream process upstream
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   145]  [debug] 19348#19348: *3 pipe read upstream: 1
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   171]  [debug] 19348#19348: *3 pipe preread: 306
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #0 080F2BB6
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2BB6 306
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 1, last(iov_len):512
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: 512, left-size:512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #1 080F2E7C
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2E7C 512
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 1, last(iov_len):512
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: 512, left-size:512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #2 080F30E4
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F30E4 512
2015/12/16 04:25:19[ngx_event_pipe_write_chain_to_temp_file,   840]  [debug] 19348#19348: *3 ngx_event_pipe_write_chain_to_temp_file, p->buf_to_file:080F27E4, p->cacheable:1, tempfile:/var/yyz/cache_xxx/temp
2015/12/16 04:25:19[               ngx_create_temp_file,   169]  [debug] 19348#19348: *3 hashed path: /var/yyz/cache_xxx/temp/2/00/0000000002
2015/12/16 04:25:19[               ngx_create_temp_file,   174]  [debug] 19348#19348: *3 temp fd:-1
2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2"
2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2/00"
2015/12/16 04:25:19[               ngx_create_temp_file,   169]  [debug] 19348#19348: *3 hashed path: /var/yyz/cache_xxx/temp/2/00/0000000002
2015/12/16 04:25:19[               ngx_create_temp_file,   174]  [debug] 19348#19348: *3 temp fd:15
2015/12/16 04:25:19[            ngx_write_chain_to_file,   355]  [debug] 19348#19348: *3 writev: 15, 1536
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   296]  [debug] 19348#19348: *3 pipe temp offset: 1536
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 3, last(iov_len):512
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: 1536, left-size:512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #3 080F2AE8
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2AE8 512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #4 080F2E7C
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2E7C 512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #5 080F30E4
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F30E4 512
2015/12/16 04:25:19[ngx_event_pipe_write_chain_to_temp_file,   840]  [debug] 19348#19348: *3 ngx_event_pipe_write_chain_to_temp_file, p->buf_to_file:00000000, p->cacheable:1, tempfile:/var/yyz/cache_xxx/temp
2015/12/16 04:25:19[            ngx_write_chain_to_file,   355]  [debug] 19348#19348: *3 writev: 15, 1536
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   296]  [debug] 19348#19348: *3 pipe temp offset: 3072
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 3, last(iov_len):512
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: 1536, left-size:512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #6 080F2AE8
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2AE8 512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #7 080F2E7C
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2E7C 512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #8 080F30E4
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F30E4 512
2015/12/16 04:25:19[ngx_event_pipe_write_chain_to_temp_file,   840]  [debug] 19348#19348: *3 ngx_event_pipe_write_chain_to_temp_file, p->buf_to_file:00000000, p->cacheable:1, tempfile:/var/yyz/cache_xxx/temp
2015/12/16 04:25:19[            ngx_write_chain_to_file,   355]  [debug] 19348#19348: *3 writev: 15, 1536
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   296]  [debug] 19348#19348: *3 pipe temp offset: 7680
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 3, last(iov_len):512
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: 657, left-size:512
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2773]  [debug] 19348#19348: *3 input buf #15 080F2AE8
2015/12/16 04:25:19[      ngx_http_fastcgi_input_filter,  2815]  [debug] 19348#19348: *3 input buf 080F2AE8 512
2015/12/16 04:25:19[                    ngx_readv_chain,   106]  [debug] 19348#19348: *3 readv: 2, last(iov_len):512
2015/12/16 04:25:19[                    ngx_readv_chain,   179]  [debug] 19348#19348: *3 readv() not ready (11: Resource temporarily unavailable)
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   344]  [debug] 19348#19348: *3 pipe recv chain: -2, left-size:367
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   367]  [debug] 19348#19348: *3 ngx_event_pipe_read_upstream recv return ngx_again, single_buf:0 
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   468]  [debug] 19348#19348: *3 pipe buf out  s:0 t:0 f:1 00000000, pos 00000000, size: 0 file: 206, size: 7474
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   481]  [debug] 19348#19348: *3 pipe buf in   s:1 t:1 f:0 080F2AE8, pos 080F2AE8, size: 512 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   494]  [debug] 19348#19348: *3 pipe buf free s:0 t:1 f:0 080F2E7C, pos 080F2E7C, size: 145 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   494]  [debug] 19348#19348: *3 pipe buf free s:0 t:1 f:0 080F30E4, pos 080F30E4, size: 0 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   502]  [debug] 19348#19348: *3 pipe length: -1, p->upstream_eof:0, p->upstream_error:0, p->free_raw_bufs:080F339C
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   559]  [debug] 19348#19348: *3 pipe write chain
2015/12/16 04:25:19[ngx_event_pipe_write_chain_to_temp_file,   840]  [debug] 19348#19348: *3 ngx_event_pipe_write_chain_to_temp_file, p->buf_to_file:00000000, p->cacheable:1, tempfile:/var/yyz/cache_xxx/temp
2015/12/16 04:25:19[                     ngx_write_file,   182]  [debug] 19348#19348: *3 write: 15, 080F2AE8, 512, 7680
2015/12/16 04:25:19[ ngx_event_pipe_write_to_downstream,   590]  [debug] 19348#19348: *3 pipe write downstream: 1
2015/12/16 04:25:19[ ngx_event_pipe_write_to_downstream,   685]  [debug] 19348#19348: *3 pipe write busy: 0
2015/12/16 04:25:19[ ngx_event_pipe_write_to_downstream,   762]  [debug] 19348#19348: *3 pipe write: out:080F27DC, f:0
2015/12/16 04:25:19[             ngx_http_output_filter,  3200]  [debug] 19348#19348: *3 http output filter "/test3.php?"
2015/12/16 04:25:19[               ngx_http_copy_filter,   157]  [debug] 19348#19348: *3 http copy filter: "/test3.php?"
2015/12/16 04:25:19[                      ngx_read_file,    31]  [debug] 19348#19348: *3 read: 15, 081109E0, 7986, 206
2015/12/16 04:25:19[           ngx_http_postpone_filter,   176]  [debug] 19348#19348: *3 http postpone filter "/test3.php?" 080F3434
2015/12/16 04:25:19[       ngx_http_chunked_body_filter,   212]  [debug] 19348#19348: *3 http chunk: 7986
2015/12/16 04:25:19[       ngx_http_chunked_body_filter,   273]  [debug] 19348#19348: *3 yang test ..........xxxxxxxx ################## lstbuf:0
2015/12/16 04:25:19[              ngx_http_write_filter,   148]  [debug] 19348#19348: *3 write old buf t:1 f:0 080F2CE8, pos 080F2CE8, size: 180 file: 0, size: 0
2015/12/16 04:25:19[              ngx_http_write_filter,   204]  [debug] 19348#19348: *3 write new buf t:1 f:0 080F3480, pos 080F3480, size: 6 file: 0, size: 0
2015/12/16 04:25:19[              ngx_http_write_filter,   204]  [debug] 19348#19348: *3 write new buf t:1 f:0 081109E0, pos 081109E0, size: 7986 file: 0, size: 0
2015/12/16 04:25:19[              ngx_http_write_filter,   204]  [debug] 19348#19348: *3 write new buf t:0 f:0 00000000, pos 080CD85D, size: 2 file: 0, size: 0
2015/12/16 04:25:19[              ngx_http_write_filter,   244]  [debug] 19348#19348: *3 http write filter: l:0 f:1 s:8174
2015/12/16 04:25:19[              ngx_http_write_filter,   372]  [debug] 19348#19348: *3 http write filter limit 0
2015/12/16 04:25:19[                         ngx_writev,   199]  [debug] 19348#19348: *3 writev: 8174 of 8174
2015/12/16 04:25:19[              ngx_http_write_filter,   378]  [debug] 19348#19348: *3 http write filter 00000000
2015/12/16 04:25:19[               ngx_http_copy_filter,   221]  [debug] 19348#19348: *3 http copy filter: 0 "/test3.php?"
2015/12/16 04:25:19[ ngx_event_pipe_write_to_downstream,   685]  [debug] 19348#19348: *3 pipe write busy: 0
2015/12/16 04:25:19[ ngx_event_pipe_write_to_downstream,   762]  [debug] 19348#19348: *3 pipe write: out:00000000, f:0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   145]  [debug] 19348#19348: *3 pipe read upstream: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   494]  [debug] 19348#19348: *3 pipe buf free s:0 t:1 f:0 080F2E7C, pos 080F2E7C, size: 145 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   494]  [debug] 19348#19348: *3 pipe buf free s:0 t:1 f:0 080F30E4, pos 080F30E4, size: 0 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   494]  [debug] 19348#19348: *3 pipe buf free s:0 t:1 f:0 080F2AE8, pos 080F2AE8, size: 0 file: 0, size: 0
2015/12/16 04:25:19[       ngx_event_pipe_read_upstream,   502]  [debug] 19348#19348: *3 pipe length: -1, p->upstream_eof:0, p->upstream_error:0, p->free_raw_bufs:080F339C
2015/12/16 04:25:19[                ngx_event_add_timer,    77]  [debug] 19348#19348: *3 <           ngx_event_pipe,    80>  event timer: 14, old: 2807200323, new: 2807200547, 
2015/12/16 04:25:19[           ngx_event_process_posted,    65]  [debug] 19348#19348: begin to run befor posted event AEA95098
2015/12/16 04:25:19[           ngx_event_process_posted,    67]  [debug] 19348#19348: *3 delete posted event AEA95098
2015/12/16 04:25:19[          ngx_http_upstream_handler,  1332]  [debug] 19348#19348: *3 http upstream request(ev->write:1): "/test3.php?"
2015/12/16 04:25:19[    ngx_http_upstream_dummy_handler,  4286]  [debug] 19348#19348: *3 http upstream dummy handler
2015/12/16 04:25:19[           ngx_worker_process_cycle,  1141]  [debug] 19348#19348: worker(19348) cycle again
2015/12/16 04:25:19[           ngx_trylock_accept_mutex,   405]  [debug] 19348#19348: accept mutex locked
2015/12/16 04:25:19[           ngx_epoll_process_events,  1622]  [debug] 19348#19348: begin to epoll_wait, epoll timer: 59776 
2015/12/16 04:25:19[           ngx_epoll_process_events,  1710]  [debug] 19348#19348: epoll: fd:14 EPOLLIN EPOLLOUT  (ev:0005) d:B2695159

*/

/*
�������xxx_buffers  XXX_buffer_sizeָ���Ŀռ䶼�����ˣ����ѻ����е�����д����ʱ�ļ���Ȼ�������������ngx_event_pipe_write_chain_to_temp_file
��д����ʱ�ļ���ֱ��read����NGX_AGAIN,Ȼ����ngx_event_pipe_write_to_downstream->ngx_output_chain->ngx_output_chain_copy_buf�ж�ȡ��ʱ�ļ�����
���͵���ˣ������ݼ���������ͨ��epoll read����ѭ��������
*/

/*ngx_http_upstream_init_request->ngx_http_upstream_cache �ͻ��˻�ȡ���� ���Ӧ��������ݺ���ngx_http_upstream_send_response->ngx_http_file_cache_create
�д�����ʱ�ļ���Ȼ����ngx_event_pipe_write_chain_to_temp_file�Ѷ�ȡ�ĺ������д����ʱ�ļ��������
ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update�а���ʱ�ļ�����rename(�൱��mv)��proxy_cache_pathָ��
��cacheĿ¼����
*/
/*������ݶ�ȡ��ϣ�����ȫ��д����ʱ�ļ���Ż�ִ��rename���̣�Ϊʲô��Ҫ��ʱ�ļ���ԭ����:����֮ǰ�Ļ�������ˣ������и��������ڴӺ��
��ȡ����д����ʱ�ļ��������ֱ��д�뻺���ļ������ڻ�ȡ������ݹ����У��������һ���ͻ��������������proxy_cache_use_stale updating����
������������ֱ�ӻ�ȡ֮ǰ�ϾɵĹ��ڻ��棬�Ӷ����Ա����ͻ(ǰ�������д�ļ�������������ȡ�ļ�����) 
*/
static ngx_int_t
ngx_event_pipe_write_chain_to_temp_file(ngx_event_pipe_t *p)
{
    ssize_t       size, bsize, n;
    ngx_buf_t    *b;
    ngx_uint_t    prev_last_shadow;
    int cacheable = p->cacheable;
    ngx_chain_t  *cl, *tl, *next, *out, **ll, **last_out, **last_free, fl;

    ngx_log_debugall(p->log, 0, "ngx_event_pipe_write_chain_to_temp_file, p->buf_to_file:%p, "
        "p->cacheable:%d, tempfile:%V", p->buf_to_file, cacheable, &p->temp_file->path->name);
    if (p->buf_to_file) { //fl��ӵ�p->inͷ�����ú�ֵ��out��out�����ӵ�fl + p��buf_to_file�洢���Ǻ�˵�ͷ���в��֣�����������
        fl.buf = p->buf_to_file;
        fl.next = p->in;
        out = &fl; 

    } else {
        out = p->in; //˵��ͷ���е�һ����д��ʱ�ļ���ʱ���Ѿ�д��ȥ�ˣ�������Ķ�����ҳ���岿��
    }

    if (!p->cacheable) {

        size = 0;
        cl = out;
        ll = NULL;
        prev_last_shadow = 1;

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "pipe offset: %O", p->temp_file->offset);

        do {
            bsize = cl->buf->last - cl->buf->pos;

            ngx_log_debug4(NGX_LOG_DEBUG_EVENT, p->log, 0,
                           "pipe buf ls:%d %p, pos %p, size: %z",
                           cl->buf->last_shadow, cl->buf->start,
                           cl->buf->pos, bsize);

            if (prev_last_shadow
                && ((size + bsize > p->temp_file_write_size)
                    || (p->temp_file->offset + size + bsize
                        > p->max_temp_file_size)))
            {
                break;
            }

            prev_last_shadow = cl->buf->last_shadow;

            size += bsize;
            ll = &cl->next;
            cl = cl->next;

        } while (cl);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0, "size: %z", size);

        if (ll == NULL) {
            return NGX_BUSY;
        }

        if (cl) {
           p->in = cl;
           *ll = NULL;

        } else {
           p->in = NULL;
           p->last_in = &p->in;
        }

    } else {
        p->in = NULL; //ע�������in��ΪNULL����Ϊin�е������Ѿ��ŵ���out�У��ں���д����ʱ�ļ�
        p->last_in = &p->in;
    }

    //������ʱ�ļ���д��
    n = ngx_write_chain_to_temp_file(p->temp_file, out);

    if (n == NGX_ERROR) {
        return NGX_ABORT;
    }

    if (p->buf_to_file) { //˵���ǵ�һ��д�����ʱ�ļ�
        //���ͷ���в��ֵ����ݳ��ȣ���ngx_http_upstream_send_response
        p->temp_file->offset = p->buf_to_file->last - p->buf_to_file->pos;  //��ʱ��offsetΪ���ͷ�������ݳ���
        //n��д���ļ��е�����(�������ͷ���к���ҳ����)�����n������ҳ�������ݳ���
        n -= p->buf_to_file->last - p->buf_to_file->pos; //n�Ǳ���(��һ��д��ʱ�ļ�)д�����ݲ��ֵĳ��ȣ�off+n���ǵ�һ��д����ʱ�ļ�ͷ���кͰ��岿�ֳ���
        p->buf_to_file = NULL;
        out = out->next;
    }

    if (n > 0) {
        /* update previous buffer or add new buffer */

        if (p->out) {//˵��ǰ���Ѿ�д����ʱ�ļ�
            for (cl = p->out; cl->next; cl = cl->next) { /* void */ } //������ʱ�ļ�buf�ڵ㣬

            b = cl->buf;

            if (b->file_last == p->temp_file->offset) {
                p->temp_file->offset += n;
                b->file_last = p->temp_file->offset;
                goto free;
            }

            last_out = &cl->next; //����������д����ʱ�ļ���������ٺ��洴��һ��chain�����뵽p->outβ��

        } else { //��һ��д��ʱ�ļ�
            last_out = &p->out; //ע������last_outָ����&p->out
        }

        cl = ngx_chain_get_free_buf(p->pool, &p->free);
        if (cl == NULL) {
            return NGX_ABORT;
        }

        b = cl->buf;

        ngx_memzero(b, sizeof(ngx_buf_t));

        b->tag = p->tag;

        /* �¿��̵�b��file_posָ�򱾴�д����ʱ�ļ��е�����ͷ��file_lastָ�򱾴�д����ʱ�ļ��е�����β�� */
        
        b->file = &p->temp_file->file; //b->fileָ�������ʱ�ļ�
        b->file_pos = p->temp_file->offset; //file_pos��С���ں��ͷ���в��ֵ����ݳ��ȣ�Ҳ����ָ���˷��ص���ҳ���岿������ͷ��
        p->temp_file->offset += n; //Ҳ����ʵ�ʴ浽��ʱ�ļ��е��ֽ���(����ͷ��������+��ҳ��������)��������ʱtemp·������������ļ����ݴ�С
        b->file_last = p->temp_file->offset;

        b->in_file = 1;
        b->temp_file = 1;

        *last_out = cl; //���´�����ָ���Ӧ��ʱ�ļ���cl��ӵ�p->out
    }

free:

    for (last_free = &p->free_raw_bufs;
         *last_free != NULL;
         last_free = &(*last_free)->next)
    {
        /* void */
    }

    for (cl = out; cl; cl = next) { 
    //p->in���еĸ���chainָ����ڴ���Ϣ�Ѿ�д����ʱ�ļ�����ͨ�������µ�chain�ڵ�ָ���ļ�����ĸ���ƫ����Ϣ����ô֮ǰp->in�е�
    //������chain��Ҫ����free���У��Ա����Է��临��
        next = cl->next;

        cl->next = p->free;
        p->free = cl;

        b = cl->buf;

        if (b->last_shadow) {

            tl = ngx_alloc_chain_link(p->pool);
            if (tl == NULL) {
                return NGX_ABORT;
            }

            tl->buf = b->shadow;
            tl->next = NULL;

            *last_free = tl;
            last_free = &tl->next;

            b->shadow->pos = b->shadow->start;
            b->shadow->last = b->shadow->start;

            ngx_event_pipe_remove_shadow_links(b->shadow);
        }
    }

    return NGX_OK;
}


/* the copy input filter */

ngx_int_t
ngx_event_pipe_copy_input_filter(ngx_event_pipe_t *p, ngx_buf_t *buf)
{
    ngx_buf_t    *b;
    ngx_chain_t  *cl;

    if (buf->pos == buf->last) {
        return NGX_OK;
    }

    cl = ngx_chain_get_free_buf(p->pool, &p->free);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    b = cl->buf;

    ngx_memcpy(b, buf, sizeof(ngx_buf_t));
    b->shadow = buf;
    b->tag = p->tag;
    b->last_shadow = 1;
    b->recycled = 1;
    buf->shadow = b;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0, "input buf #%d", b->num);

    if (p->in) {
        *p->last_in = cl;
    } else {
        p->in = cl;
    }
    p->last_in = &cl->next;

    if (p->length == -1) {
        return NGX_OK;
    }

    p->length -= b->last - b->pos;

    return NGX_OK;
}


/*
//ɾ�����ݵ�shadow���Լ�recycled����Ϊ0����ʾ����Ҫѭ�����ã�����ʵ����buffering����
//��Ϊngx_http_write_filter���������ж������recycled��־���ͻ����������ݷ��ͳ�ȥ��
//������ｫ��Щ��־��գ���ngx_http_write_filter����ͻᾡ������ġ�
*/
static ngx_inline void
ngx_event_pipe_remove_shadow_links(ngx_buf_t *buf)
{
    ngx_buf_t  *b, *next;

    b = buf->shadow; //���shadowָ�����buf�����FCGI���ݵĵ�һ�����ݽڵ�

    if (b == NULL) {
        return;
    }

    while (!b->last_shadow) { //����������һ�����ݽڵ㣬�������������
        next = b->shadow;

        b->temporary = 0;
        b->recycled = 0;

        b->shadow = NULL;//��shadow��Ա�ÿա�
        b = next;
    }

    b->temporary = 0;
    b->recycled = 0;
    b->last_shadow = 0;

    b->shadow = NULL;

    buf->shadow = NULL;
}


ngx_int_t
ngx_event_pipe_add_free_buf(ngx_event_pipe_t *p, ngx_buf_t *b)
{ //��������b��������ݿ����free_raw_bufs�Ŀ�ͷ���ߵڶ���λ�á�bΪ�ϲ����û���˵����ݿ顣
    ngx_chain_t  *cl;

    cl = ngx_alloc_chain_link(p->pool);//���ﲻ�����b�͵���free_raw_bufs->buf�������
    if (cl == NULL) {
        return NGX_ERROR;
    }

    if (p->buf_to_file && b->start == p->buf_to_file->start) { 
        b->pos = p->buf_to_file->last;
        b->last = p->buf_to_file->last;

    } else {
        b->pos = b->start;//�ÿ���������
        b->last = b->start;
    }

    b->shadow = NULL;

    cl->buf = b;

    if (p->free_raw_bufs == NULL) { //����ñ���û�нڵ㣬��ֱ�Ӱ�������cl�ӽ���
        p->free_raw_bufs = cl;
        cl->next = NULL;

        return NGX_OK;
    }

    //�������ע�ͣ���˼�ǣ������ǰ���free_raw_bufs��û�����ݣ��ǾͰɵ�ǰ������ݷ���ͷ�����С�
	//���������ǰfree_raw_bufs�����ݣ��Ǿ͵÷ŵ�������ˡ�Ϊʲô����������?���磬��ȡһЩ���ݺ󣬻�ʣ��һ��β�ʹ����free_raw_bufs��Ȼ��ʼ���ͻ���д����
	//д�����ȻҪ��û�õ�buffer���뵽�����������������ngx_event_pipe_write_to_downstream�������ġ����߸ɴ���ngx_event_pipe_drain_chains��������
	//��Ϊ���������inpupt_filter��������Ǵ����ݿ鿪ʼ����Ȼ�󵽺���ģ�
	//�����ڵ���input_filter֮ǰ�ǻὫfree_raw_bufs�ÿյġ�Ӧ���������ط�Ҳ�е��á�
    if (p->free_raw_bufs->buf->pos == p->free_raw_bufs->buf->last) { 
    //ͷ��bufû������  �ο�����ngx_event_pipe_read_upstream����Ϊ��ȡ�������δ����һ��bufָ��Ļ�������������free��ͷ

        /* add the free buf to the list start */

        cl->next = p->free_raw_bufs; //���뵽����ͷ��
        p->free_raw_bufs = cl;

        return NGX_OK;
    }

    /* the first free buf is partially filled, thus add the free buf after it */

    cl->next = p->free_raw_bufs->next; //���뵽β��
    p->free_raw_bufs->next = cl;

    return NGX_OK;
}

/*
����p->in/out/busy����������������fastcgi���ݿ��ͷţ����뵽free_raw_bufs�м�ȥ��Ҳ���ǣ����upstream�������ģ���������ʽ���HTML PHP�����ݡ�
*/
static ngx_int_t
ngx_event_pipe_drain_chains(ngx_event_pipe_t *p)
{
    ngx_chain_t  *cl, *tl;

    for ( ;; ) {
        if (p->busy) {
            cl = p->busy;
            p->busy = NULL;

        } else if (p->out) {
            cl = p->out;
            p->out = NULL;

        } else if (p->in) {
            cl = p->in;
            p->in = NULL;

        } else {
            return NGX_OK;
        }

        while (cl) {/*Ҫ֪��������cl���棬����p->in�������Щngx_buf_t�ṹ��ָ��������ڴ�ʵ��������
        ngx_event_pipe_read_upstream�����input_filter����Э�������ʱ������Ϊ���ӿͻ��˶�ȡ����ʱ��buf���õģ�Ҳ������ν��Ӱ�ӡ�
		Ȼ����Ȼp->inָ������������кܶ�ܶ���ڵ㣬ÿ���ڵ����һ��HTML PHP�ȴ��룬�������ǲ����Ƕ�ռһ���ڴ�ģ����ǿ��ܹ���ģ�
		����һ����buffer��������3��FCGI��STDOUT���ݰ�������data���֣���ô������3��b�Ľڵ����ӵ�p->in��ĩβ�����ǵ�shadow��Ա
		�ֱ�ָ����һ���ڵ㣬���һ���ڵ��ָ���������Ĵ��ڴ�ṹ��������ngx_http_fastcgi_input_filterʵ�֡�
        */
            if (cl->buf->last_shadow) {//������ĳ����FCGI���ݿ�����һ���ڵ㣬�ͷ�ֻ��Ȼ�������һ����������ĳ��Сhtml ���ݿ顣
                if (ngx_event_pipe_add_free_buf(p, cl->buf->shadow) != NGX_OK) {
                    return NGX_ABORT;
                }

                cl->buf->last_shadow = 0;
            }

            cl->buf->shadow = NULL;
            tl = cl->next;
            cl->next = p->free;//��cl���Сbuf�ڵ����p->free����ngx_http_fastcgi_input_filter�����ظ�ʹ�á�
            p->free = cl;
            cl = tl;
        }
    }
}
