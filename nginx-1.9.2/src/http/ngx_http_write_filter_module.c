
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static ngx_int_t ngx_http_write_filter_init(ngx_conf_t *cf);


static ngx_http_module_t  ngx_http_write_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_write_filter_init,            /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL,                                  /* merge location configuration */
};

/*
��6-1  Ĭ�ϼ������Nginx��HTTP����ģ��
���������������������������������������ש�������������������������������������������������������������������
��Ĭ�ϼ������Nginx��HTTP����ģ��     ��    ����                                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ڷ���200�ɹ�ʱ������������If-              ��
��                                    ��Modified-Since����If-Unmodified-Sinceͷ��ȡ������������ļ���ʱ   ��
��ngx_http_not_modified_filter_module ��                                                                  ��
��                                    ���䣬�ٷ��������û��ļ�������޸�ʱ�䣬�Դ˾����Ƿ�ֱ�ӷ���304     ��
��                                    �� Not Modified��Ӧ���û�                                           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���������е�Range��Ϣ������Range�е�Ҫ�󷵻��ļ���һ���ָ�      ��
��ngx_http_range_body_filter_module   ��                                                                  ��
��                                    ���û�                                                              ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP�������������û����͵�ngx_chain_t�ṹ��HTTP��         ��
��                                    ���帴�Ƶ��µ�ngx_chain_t�ṹ�У����Ǹ���ָ��ĸ��ƣ�������ʵ��     ��
��ngx_http_copy_filter_module         ��                                                                  ��
��                                    ��HTTP��Ӧ���ݣ���������HTTP����ģ�鴦���ngx_chain_t���͵ĳ�       ��
��                                    ��Ա����ngx_http_copy_filter_moduleģ�鴦���ı���                 ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ������������ͨ���޸�nginx.conf�����ļ����ڷ���      ��
��ngx_http_headers_filter_module      ��                                                                  ��
��                                    �����û�����Ӧ����������HTTPͷ��                                  ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ�������������ִ��configure����ʱ�ᵽ��http_        ��
��ngx_http_userid_filter_module       ��                                                                  ��
��                                    ��userid moduleģ�飬������cookie�ṩ�˼򵥵���֤������           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���Խ��ı����ͷ��ظ��û�����Ӧ��������nginx��conf�е���������   ��
��ngx_http_charset_filter_module      ��                                                                  ��
��                                    �����б��룬�ٷ��ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ֧��SSI��Server Side Include����������Ƕ�룩���ܣ����ļ����ݰ�  ��
��ngx_http_ssi_filter_module          ��                                                                  ��
��                                    ��������ҳ�в����ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP����������                             ����Ӧ����     ��
��ngx_http_postpone_filter_module     ��subrequest��������������ʹ�ö��������ͬʱ��ͻ��˷�����Ӧʱ    ��
��                                    ���ܹ�������ν�ġ������ǿ����չ����������˳������Ӧ          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���ض���HTTP��Ӧ���壨����ҳ�����ı��ļ�������gzipѹ������      ��
��ngx_http_gzip_filter_module         ��                                                                  ��
��                                    ����ѹ��������ݷ��ظ��û�                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_range_header_filter_module ��  ֧��rangeЭ��                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_chunked_filter_module      ��  ֧��chunk����                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ù���ģ�齫���r->headers out�ṹ��        ��
��                                    ���еĳ�Ա���л�Ϊ���ظ��û���HTTP��Ӧ�ַ�����������Ӧ��(��         ��
��ngx_http_header_filter_module       ��                                                                  ��
��                                    ��HTTP/I.1 200 0K)����Ӧͷ������ͨ������ngx_http_write filter       ��
��                                    �� module����ģ���еĹ��˷���ֱ�ӽ�HTTP��ͷ���͵��ͻ���             ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_write_filter_module        ��  ����HTTP������������ģ�鸺����ͻ��˷���HTTP��Ӧ              ��
���������������������������������������ߩ�������������������������������������������������������������������

*/ngx_module_t  ngx_http_write_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_write_filter_module_ctx,     /* module context */
    NULL,                                  /* module directives */
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

/*
ngx_http_header_filter����ͷ��������ͨ������ngx_http_write_filter������������Ӧͷ���ġ���ʵ�ϣ���������ǰ������ģ�������е�
���һ��ģ��ngx_http_write_filter_module�Ĵ���������HTTPģ�����ngx_http_output_filter�������Ͱ���ʱ������Ҳ��ͨ���÷���������Ӧ��
����һ���޷�����ȫ���Ļ���������ʱ��ngx_http_write_filter�����ǻ᷵��NGX_AGAIN�ģ�ͬʱ��δ������ɵĻ������ŵ������out��Ա
�У���Ҳ����˵��������Ӧͷ����ngx_http_header_filter�����᷵��NGX_AGAIN���������Ҫ�ٷ��Ͱ��壬��ô��ʱ����Ҫ����
ngx_http_finalize_request�����������������е�2���������Ҫ����NGX_AGAIN������HTTP��ܲŻ��������д�¼�ע�ᵽepoll������
���ذ������out��Ա�л��������HTTP��Ӧ������ϲŻ��������
*/ //�������ݵ�ʱ�����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_writer���epoll write�¼�������
ngx_int_t
ngx_http_write_filter(ngx_http_request_t *r, ngx_chain_t *in)
//ngx_http_write_filter��in�е�����ƴ�ӵ�out���棬Ȼ�����writev���ͣ�û�з�����������������out��
{//��r->out��������ݣ��Ͳ������������һ����writev�Ļ��Ʒ��͸��ͻ��ˣ����û�з��������еģ���ʣ�µķ���r->out

//����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_set_write_handler->ngx_http_writer���epoll write�¼�������

    off_t                      size, sent, nsent, limit;
    ngx_uint_t                 last, flush, sync;
    ngx_msec_t                 delay;
    ngx_chain_t               *cl, *ln, **ll, *chain;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    //���errorΪ1��ʾ���������ôֱ�ӷ���NGX_ERROR
    if (c->error) {
        return NGX_ERROR;
    }

    size = 0;
    flush = 0;
    sync = 0;
    last = 0;
    ll = &r->out;

    /* find the size, the flush point and the last link of the saved chain */
//�ҵ������ngx_http_request_t�ṹ���д�ŵĵȴ����͵Ļ���������out���������ngx_chain_t���͵Ļ���������
//�����out��������ռ���˶����ֽ��������out����ͨ���������Ŵ����͵���Ӧ�����磬�ڵ���ngx_http_send header����ʱ��
//���HTTP��Ӧͷ���������޷�һ���Է����꣬��ôʣ�����Ӧͷ���ͻ���out�����С�
    for (cl = r->out; cl; cl = cl->next) { //out�������ϴη���û�з����������
        ll = &cl->next;

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "write old buf t:%d f:%d %p, pos %p, size: %z "
                       "file: %O, size: %O",
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);

#if 1
        if (ngx_buf_size(cl->buf) == 0 && !ngx_buf_special(cl->buf)) {
            ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                          "zero size buf in writer "
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
            return NGX_ERROR;
        }
#endif

        size += ngx_buf_size(cl->buf);

        if (cl->buf->flush || cl->buf->recycled) {
            flush = 1;
        }

        if (cl->buf->sync) {
            sync = 1;
        }

        if (cl->buf->last_buf) {
            last = 1;
        }
    }

    /* add the new chain to the existent one */
    //�������ngx_chain_t���͵Ļ�������in����in�еĻ��������뵽out�����ĩβ��������out��������ռ�ö����ֽ���
    for (ln = in; ln; ln = ln->next) { //in��ʾ����¼ӽ�����Ҫ���͵�����
        cl = ngx_alloc_chain_link(r->pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = ln->buf;
        *ll = cl;
        ll = &cl->next;

//ע��Ӻ�˽��յ����ݵ������ļ��к���filterģ���У��п������µ�buf����ָ���ˣ���Ϊngx_http_copy_filter->ngx_output_chain�л����·����ڴ��ȡ�����ļ�����
        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "write new buf temporary:%d buf-in-file:%d, buf->start:%p, buf->pos:%p, buf_size: %z "
                       "file_pos: %O, in_file_size: %O",
                       cl->buf->temporary, cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);

#if 1
        if (ngx_buf_size(cl->buf) == 0 && !ngx_buf_special(cl->buf)) {
            ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                          "zero size buf in writer "
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
            return NGX_ERROR;
        }
#endif

        size += ngx_buf_size(cl->buf);

        if (cl->buf->flush || cl->buf->recycled) {
            flush = 1;
        }

        if (cl->buf->sync) {
            sync = 1;
        }

        if (cl->buf->last_buf) {
            last = 1;
        }
    }

    *ll = NULL;

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http write filter: last:%d flush:%d size:%O", last, flush, size);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    /*
     * avoid the output if there are no last buf, no flush point,
     * there are the incoming bufs and the size of all bufs
     * is smaller than "postpone_output" directive
     */
    /*
2025/02/08 00:56:18 [debug] 13330#0: *1 HTTP/1.1 200 OK
Server: nginx/1.9.2
Date: Fri, 07 Feb 2025 16:56:18 GMT
Content-Type: text/plain
Content-Length: 43
Connection: keep-alive

2025/02/08 00:56:18 [debug] 13330#0: *1 write new buf t:1 f:0 080E68BC, pos 080E68BC, size: 147 file: 0, size: 0
2025/02/08 00:56:18 [debug] 13330#0: *1 http write filter: l:0 f:0 s:147  
2025/02/08 00:56:18 [debug] 13330#0: *1 http output filter "/mytest?" //��ǰ��Ķ���ͷ����ӡ��ע�⵽ʵ���ϲ�û��writev�����Ǻ�����İ���һ��writev��
2025/02/08 00:56:18 [debug] 13330#0: *1 http copy filter: "/mytest?"  //ngx_http_copy_filter������ʼ
2025/02/08 00:56:18 [debug] 13330#0: *1 http postpone filter "/mytest?" 080E6A40
2025/02/08 00:56:18 [debug] 13330#0: *1 write old buf t:1 f:0 080E68BC, pos 080E68BC, size: 147 file: 0, size: 0
2025/02/08 00:56:18 [debug] 13330#0: *1 write new buf t:1 f:0 080C82EC, pos 080C82EC, size: 18 file: 0, size: 0
2025/02/08 00:56:18 [debug] 13330#0: *1 write new buf t:1 f:0 080E69A0, pos 080E69A0, size: 25 file: 0, size: 0
2025/02/08 00:56:18 [debug] 13330#0: *1 http write filter: l:1 f:0 s:190
2025/02/08 00:56:18 [debug] 13330#0: *1 http write filter limit 0
2025/02/08 00:56:18 [debug] 13330#0: *1 writev: 190 of 190
 2025/02/08 00:56:18 [debug] 13330#0: *1 http write filter 00000000
 2025/02/08 00:56:18 [debug] 13330#0: *1 http copy filter: 0 "/mytest?" //ngx_http_copy_filter������β��Ҳ�����м��filter������ngx_http_copy_filter����ִ�е�
 2025/02/08 00:56:18 [debug] 13330#0: *1 http finalize request: 0, "/mytest?" a:1, c:1
 2025/02/08 00:56:18 [debug] 13330#0: *1 set http keepalive handler
 2025/02/08 00:56:18 [debug] 13330#0: *1 http close request

     */
    
    /*
    3����־λͬʱΪ0���������͵�out������û��һ����������ʾ��Ӧ�Ѿ���������Ҫ���̷��ͳ�ȥ�������ұ���Ҫ���͵Ļ�����in��Ȼ��Ϊ�գ�
    �������������м�����Ĵ�������Ӧ�Ĵ�С��С�������ļ��е�postpone_output��������ô˵����ǰ�Ļ������ǲ���������û�б�Ҫ���̷���
     */ //���������ͷ�������а��壬��һ����β����ͷ��filter����ngx_http_header_filter->ngx_http_write_filter�������ʱ��һ��ͷ���ֶ�
     //���٣�����ֱ�ӷ���NGX_OK�������Ϳ�����ͷ���Ͱ�������β���İ���filter����ngx_http_write_filter->ngx_http_write_filter�Ͱ�����һ�������з��ͳ�ȥ
    if (!last && !flush && in && size < (off_t) clcf->postpone_output) {
        ngx_log_debugall(c->log, 0, "send size:%O < min postpone_output:%O, do not send", size, (off_t) clcf->postpone_output);
        //���last flush������0������in��ΪNULL������������е�����С��postpone_output����ֱ�ӷ��أ���ʾ�����ݸ���(�ﵽpostpone_output)������ָ��last flush�����
        return NGX_OK;
    }

/*
 ���ȼ��������д�¼��ı�־λdelayed�����delayedΪ1�����ʾ��һ�ε�epoll��������������Ҫ���٣��ǲ����Է�����Ӧ�ģ�delayedΪ1
 ָ������Ӧ��Ҫ�ӳٷ��ͣ����delayedΪ0����ʾ���β���Ҫ���٣���ô�ټ��ngx_http_request_t�ṹ���е�limit_rate
 ������Ӧ�����ʣ����limit_rateΪ0����ʾ���������Ҫ���Ʒ����ٶȣ����limit rate���0����˵��������Ӧ���ٶȲ��ܳ���limit_rateָ�����ٶȡ�
 */
    if (c->write->delayed) { //�ں������������1
//���ͻ��˶�Ӧ��buffered��־λ����NGX_HTTP_WRITE_BUFFERED�꣬ͬʱ����NGX AGAIN�������ڸ���HTTP���out�������л�����Ӧ�ȴ����͡�
        c->buffered |= NGX_HTTP_WRITE_BUFFERED;
        return NGX_AGAIN; 
        //����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_set_write_handler->ngx_http_writer���epoll write�¼�������
    }

    if (size == 0
        && !(c->buffered & NGX_LOWLEVEL_BUFFERED)
        && !(last && c->need_last_buf))
    {
        if (last || flush || sync) {
            for (cl = r->out; cl; /* void */) {
                ln = cl;
                cl = cl->next;
                ngx_free_chain(r->pool, ln);
            }

            r->out = NULL;
            c->buffered &= ~NGX_HTTP_WRITE_BUFFERED;

            return NGX_OK;
        }

        ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                      "the http output chain is empty");

        ngx_debug_point();

        return NGX_ERROR;
    }

    if (r->limit_rate) {
        if (r->limit_rate_after == 0) {
            r->limit_rate_after = clcf->limit_rate_after;
        }

    /*
    ngx_time()��������ȡ���˵�ǰʱ�䣬��start sec��ʾ��ʼ���յ��ͻ����������ݵ�ʱ�䣬c->sent��ʾ�����������Ѿ������˵�HTTP��
    Ӧ���ȣ�����������ı���limit�ͱ�ʾ���ο��Է��͵��ֽ����ˡ����limitС�ڻ����0������ʾ��������ϵķ�����Ӧ�ٶ��Ѿ�����
    ��limit_rate����������ƣ����Ա��β����Լ������ͣ�������7��ִ�У����limit����0����ʾ���ο��Է���limit�ֽڵ���Ӧ����ʼ������Ӧ��
      */
        limit = (off_t) r->limit_rate * (ngx_time() - r->start_sec + 1)
                - (c->sent - r->limit_rate_after); 
                //ʵ�����ⷢ�͹����оͱ�ʵ�ʵ�limit_rate�෢��limit_rate_after��Ҳ�����ȷ���limit_rate_after��ſ�ʼ�����Ƿ�����

        if (limit <= 0) {
            c->write->delayed = 1; //���ڴﵽ������Ӧ���ٶ����ޣ���ʱ��������д�¼���delayed��־λ��Ϊ1��

            /* limit���Ѿ��������ֽ���������0���߸����������ʱ���ĳ�ʱʱ���ǳ����ֽ�������limit_rate���������Ҫ�ȴ���ʱ��
            �ټ���l���룬������ʹNginx��ʱ��׼ȷ������������Ӧʱ�������� */
            delay = (ngx_msec_t) (- limit * 1000 / r->limit_rate + 1);
            //��Ӷ�ʱ����ʱ��Ϊʲôû��ngx_handle_write_event? ��Ϊһ�����wrie epoll�¼�����ôֻҪ�ں����ݷ��ͳ�ȥ�ͻᴥ��write�¼���
            //�Ӷ�ִ��ngx_http_writer����������Ǻܿ�ģ��������𲻵����ٵ�������
            ngx_add_timer(c->write, delay, NGX_FUNC_LINE); //handleӦ����ngx_http_request_handler

            c->buffered |= NGX_HTTP_WRITE_BUFFERED;

            return NGX_AGAIN;
            //����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_set_write_handler->ngx_http_writer���epoll write�¼�������
        }

    /*
        ����������Ӧ���͸��ͻ��ˡ�Ȼ�����������е���Ӧ���ܷǳ�����ô��һ��Ӧ�÷��Ͷ����ֽ��أ���Ҫ����ǰ��������limit������
    ǰ��ȡ�õ�������sendfile_max_chunk�����㣬ͬʱҪ���ݱ���������������Ĵ������ֽ�������������3��ֵ�е���Сֵ����Ϊ��
    �η��͵���Ӧ���ȡ� ʵ�����ͨ��ngx_writev_chain�������ݵ�ʱ�򣬻�������һ��
    */
        if (clcf->sendfile_max_chunk
            && (off_t) clcf->sendfile_max_chunk < limit)
        {
            limit = clcf->sendfile_max_chunk;
        }

    } else {
        limit = clcf->sendfile_max_chunk;
    }

    sent = c->sent;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http write filter limit %O", limit);
    //ע�����﷢�͵�ʱ����ܻ���ֳ��٣������ڷ��ͳɹ�������¼����Ƿ��٣��Ӷ������Ƿ���Ҫ������ʱ���ӳٷ���
    //����ֵӦ����out�л�û�з��ͳ�ȥ�����ݴ����chain��
    chain = c->send_chain(c, r->out, limit); //����������¼���ʵ���Ѿ����ͳ�ȥ�˶����ֽ�

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http write filter %p", chain);

    if (chain == NGX_CHAIN_ERROR) {
        c->error = 1;
        return NGX_ERROR;
    }

    //������Ӧ���ٴμ�������limit_rate��־λ�����limit_rateΪ0�����ʾ����Ҫ���٣����limit_rate���0�����ʾ��Ҫ���١� 
    if (r->limit_rate) {

        nsent = c->sent;//������ִ��c->send_chain��ʵ�ʷ��͵��ֽ�����ӵ�c->send����

        if (r->limit_rate_after) {

            sent -= r->limit_rate_after;
            if (sent < 0) {
                sent = 0;
            }

            nsent -= r->limit_rate_after;
            if (nsent < 0) {
                nsent = 0;
            }
        }

        //���¼����Ƿ����ˣ�����������������ӳٶ�ʱ���ӳٷ���
        delay = (ngx_msec_t) ((nsent - sent) * 1000 / r->limit_rate);

        /*
          ǰ�����c->send_chain���͵���Ӧ�ٶȻ��ǹ����ˣ��Ѿ�������һЩ��Ӧ�����¼��������Ҫ�������ٺ����ſ��Լ������ͣ�
          ����ngx_add_timer������д�¼��������������ĺ�����Ϊ��ʱʱ����ӵ���ʱ���С�ͬʱ����д�¼���delayed��־λ��Ϊ1��
          */
        if (delay > 0) {
            limit = 0;
            c->write->delayed = 1;
            //��Ӷ�ʱ����ʱ��Ϊʲôû��ngx_handle_write_event? ��Ϊһ�����wrie epoll�¼�����ôֻҪ�ں����ݷ��ͳ�ȥ�ͻᴥ��write�¼���
            //�Ӷ�ִ��ngx_http_writer����������Ǻܿ�ģ��������𲻵����ٵ�������
            ngx_add_timer(c->write, delay, NGX_FUNC_LINE);
        }
    }

    if (limit
        && c->write->ready
        && c->sent - sent >= limit - (off_t) (2 * ngx_pagesize)) //������ٵ��ֽ���������
    {
        c->write->delayed = 1;
        //��Ӷ�ʱ����ʱ��Ϊʲôû��ngx_handle_write_event? ��Ϊһ�����wrie epoll�¼�����ôֻҪ�ں����ݷ��ͳ�ȥ�ͻᴥ��write�¼���
        //�Ӷ�ִ��ngx_http_writer����������Ǻܿ�ģ��������𲻵����ٵ�������
        ngx_add_timer(c->write, 1, NGX_FUNC_LINE);
    }

    /*
     ����ngx_http_request_t�ṹ���out�����������Ѿ����ͳɹ��Ļ������黹���ڴ�ء����out�����л���ʣ���û�з��ͳ�ȥ�Ļ�������
     ����ӵ�out����ͷ��������Ѿ���out�����е����л����������͸��ͻ�����,��r->out����Ϊ��
     */
    for (cl = r->out; cl && cl != chain; /* void */) { //chainΪr->out�л�δ���͵����ݲ���
        ln = cl;
        cl = cl->next;
        ngx_free_chain(r->pool, ln);
    }

/*ʵ����p->busy����ָ�����ngx_http_write_filter��δ�������r->out�б�������ݣ��ⲿ������ʼ����r->out����ǰ�棬�����ڶ������ݺ���
ngx_http_write_filter�л�����������ݼӵ�r->out���棬Ҳ����δ���͵�������r->outǰ���������������棬����ʵ��write��֮ǰδ���͵��ȷ��ͳ�ȥ*/

    r->out = chain; //�ѻ�û�з���������ݴ�����ӵ�out�У�ʵ����in�е����chain��buf��r->out�е����chain��bufָ������ͬ�Ļ�Ϊ���ͳ�ȥ�������ڴ�

    if (chain) { //��û�з�����ɣ���Ҫ��������
        c->buffered |= NGX_HTTP_WRITE_BUFFERED;
        return NGX_AGAIN; 
        //����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_set_write_handler->ngx_http_writer���epoll write�¼�������
    }

    c->buffered &= ~NGX_HTTP_WRITE_BUFFERED;
    
    /* �������filterģ��buffer��chain����postponedΪNULL����ô����NGX_AGAIN����Ҫ��������buf */  
    if ((c->buffered & NGX_LOWLEVEL_BUFFERED) && r->postponed == NULL) {
        return NGX_AGAIN;
        //����ngx_http_write_filterд���ݣ��������NGX_AGAIN,���Ժ��д���ݴ���ͨ����ngx_http_set_write_handler->ngx_http_writer���epoll write�¼�������
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_write_filter_init(ngx_conf_t *cf)
{
    ngx_http_top_body_filter = ngx_http_write_filter;

    return NGX_OK;
}

