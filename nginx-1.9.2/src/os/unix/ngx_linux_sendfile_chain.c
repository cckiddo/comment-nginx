
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

/*
tcp_nopush
�ٷ�:

tcp_nopush

Syntax: tcp_nopush on | off

Default: off

Context: http

server

location

Reference: tcp_nopush

 

This directive permits or forbids the use of thesocket options TCP_NOPUSH on FreeBSD or TCP_CORK on Linux. This option is onlyavailable when using sendfile.

Setting this option causes nginx to attempt to sendit��s HTTP response headers in one packet on Linux and FreeBSD 4.x

You can read more about the TCP_NOPUSH and TCP_CORKsocket options here.

 

linux ����tcp_cork,�������˼����˵����ʹ��sendfile����ʱ��tcp_nopush�������ã�����ָ��tcp_nodelay�ǻ���ġ�tcp_cork��linux��tcp/ip�����һ����׼�ˣ������׼�Ĵ�ŵ���˼�ǣ�һ������£���tcp�����Ĺ����У���Ӧ�ó�����յ����ݰ������ϴ��ͳ�ȥ�����ȴ�����tcp_corkѡ�������ݰ��������ϴ��ͳ�ȥ���ȵ����ݰ����ʱ��һ���ԵĴ����ȥ�����������ڽ������������Ѿ���Ĭ���ˡ�

Ҳ����˵tcp_nopush = on �����õ���tcp_cork���������Ҳ��Ĭ�ϵģ�����������ݰ��������ϴ��ͳ�ȥ���ȵ����ݰ����ʱ��һ���ԵĴ����ȥ�����������ڽ�����������


�Կ��Ͷ�ݾ���˵��һ�£��������ҵ���⣬Ҳ���ǲ���ȷ�ģ�������ݶ���ʱ�����Ա�յ�һ������������Ͷ�ݣ�������֤�˼�ʱ�ԣ����ǻ�ķѴ����������������������ϱ��־��ǻ����������������������յ�һ���������Ѱ����ŵ���ɢ�أ���һ��������ͳһͶ�ݣ���������tcp_cork��ѡ��ɵ����飬�����Ļ�������󻯵�����������Դ����Ȼ��һ����ӳ١�

����nginx�����ļ��е�tcp_nopush��Ĭ�Ͼ���tcp_nopush,����Ҫ�ر�ָ�������ѡ�����www��ftp�ȴ��ļ����а���

 

tcp_nodelay
        TCP_NODELAY��TCP_CORK�����Ͽ����˰��ġ�Nagle������Nagle��������ĺ����ǲ���Nagle�㷨�ѽ�С�İ���װΪ�����֡�� John Nagle��Nagle�㷨�ķ����ˣ����߾��������������������ģ�����1984���״������ַ��������Խ������������˾������ӵ�����⣨���˽�������ο�IETF RFC 896��������������������ν��silly window syndrome�����ĳơ��޴�����֢��Ⱥ�������庬���ǣ���Ϊ�ձ��ն�Ӧ�ó���ÿ����һ�λ��������ͻᷢ��һ�����������������һ������ӵ��һ���ֽڵ������غ��Լ�40���ֽڳ��İ�ͷ�����ǲ���4000%�Ĺ��أ������׵ؾ��������緢��ӵ��,�� Nagle����������һ�ֱ�׼�����������������ϵ���ʵ�֡��������Ѿ���Ϊȱʡ�����ˣ��������ǿ�������Щ�����°���һѡ��ص�Ҳ�ǺϺ���Ҫ�ġ�

       ���������Ǽ���ĳ��Ӧ�ó��򷢳���һ������ϣ������С�����ݡ����ǿ���ѡ�������������ݻ��ߵȴ��������������Ȼ����һ�η������ֲ��ԡ�����������Ϸ������ݣ���ô�����Ե��Լ��ͻ�/�������͵�Ӧ�ó��򽫼�������档�����������������ô��Ӧʱ��Ҳ���һЩ�����ϲ�������ͨ�������׽��ֵ�TCP_NODELAY = on ѡ������ɣ������ͽ�����Nagle �㷨��

       ����һ���������Ҫ���ǵȵ��������ﵽ���ʱ��ͨ������һ�η���ȫ�����ݣ��������ݴ��䷽ʽ�����ڴ������ݵ�ͨ�����ܣ����͵�Ӧ�þ����ļ���������Ӧ�� Nagle�㷨����������¾ͻ�������⡣���ǣ���������ڷ��ʹ������ݣ����������TCP_CORKѡ�����Nagle�����䷽ʽ����ͬ TCP_NODELAY�෴��TCP_CORK�� TCP_NODELAY�ǻ����ų�ģ���




endfile
=================
nginx �� sendfile ѡ������� nginx �����ļ�ʱʹ��ϵͳ���� sendfile(2)


sendfile(2) �����ڴ����ļ�����ʱʹ���ں˿ռ��е�һЩ���ݡ��������Խ�ʡ��������Դ��
1.sendfile(2) ��ϵͳ���ã������ζ�Ų��������ں�����ɵģ����û�а�����������л���Դ
2.sendfile(2) ȡ����ϵͳ read �� write ������
3.sendfile(2) �����㸴�ƣ�Ҳ���ǿ���ֱ�Ӵ� DMA д����


������һ�²��� sendfile �Ĵ�ͳ���紫����̣�


read(file, tmp_buf, len);
write(socket, tmp_buf, len);


Ӳ�� >> kernel buffer >> user buffer >> kernel socket buffer >> Э��ջ


һ����˵һ������Ӧ����ͨ����Ӳ�����ݣ�Ȼ��д���ݵ� socket ��������紫��ġ�
��������ǣ�


1��ϵͳ���� read() ����һ���������л����� user mode �л��� kernel mode��Ȼ�� DMA ִ�п��������ļ����ݴ�Ӳ�̶���һ�� kernel buffer �
2�����ݴ� kernel buffer ������ user buffer��Ȼ��ϵͳ���� read() ���أ���ʱ�ֲ���һ���������л�����kernel mode �л��� user mode��
3��ϵͳ���� write() ����һ���������л����� user mode �л��� kernel mode��Ȼ��Ѳ���2���� user buffer �����ݿ����� kernel buffer�����ݵ�2�ο����� kernel buffer������������Ǹ���ͬ�� kernel buffer����� buffer �� socket �������
4��ϵͳ���� write() ���أ�����һ���������л����� kernel mode �л��� user mode����4���л��ˣ���Ȼ�� DMA �� kernel buffer �������ݵ�Э��ջ����4�ο����ˣ���


����4��������4���������л�����4�ο�������kernel 2.0+ �汾�У�ϵͳ���� sendfile() �������������沽���������ܵġ�
sendfile() �����ܼ����л��������һ��ܼ��ٿ���������


------------------------------
������һ���� sendfile() ���������紫��Ĺ��̣�


sendfile(socket, file, len);


Ӳ�� >> kernel buffer (���ٿ�����kernel socket buffer) >> Э��ջ


1��ϵͳ���� sendfile() ͨ�� DMA ��Ӳ�����ݿ����� kernel buffer��Ȼ�����ݱ� kernel ֱ�ӿ���������һ���� socket ��ص� kernel buffer������û�� user mode �� kernel mode ֮����л����� kernel ��ֱ������˴�һ�� buffer ����һ�� buffer �Ŀ�����
2��DMA �����ݴ� kernel buffer ֱ�ӿ�����Э��ջ��û���л���Ҳ����Ҫ���ݴ� user mode ������ kernel mode����Ϊ���ݾ��� kernel �

*/
static ssize_t ngx_linux_sendfile(ngx_connection_t *c, ngx_buf_t *file,
    size_t size);

#if (NGX_THREADS)
#include <ngx_thread_pool.h>

#if !(NGX_HAVE_SENDFILE64)
#error sendfile64() is required!
#endif

static ngx_int_t ngx_linux_sendfile_thread(ngx_connection_t *c, ngx_buf_t *file,
    size_t size, size_t *sent);
static void ngx_linux_sendfile_thread_handler(void *data, ngx_log_t *log);
#endif


/*
 * On Linux up to 2.4.21 sendfile() (syscall #187) works with 32-bit
 * offsets only, and the including <sys/sendfile.h> breaks the compiling,
 * if off_t is 64 bit wide.  So we use own sendfile() definition, where offset
 * parameter is int32_t, and use sendfile() for the file parts below 2G only,
 * see src/os/unix/ngx_linux_config.h
 *
 * Linux 2.4.21 has the new sendfile64() syscall #239.
 *
 * On Linux up to 2.6.16 sendfile() does not allow to pass the count parameter
 * more than 2G-1 bytes even on 64-bit platforms: it returns EINVAL,
 * so we limit it to 2G-1 bytes.
 */

#define NGX_SENDFILE_MAXSIZE  2147483647L


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

//ngx_linux_io
ngx_chain_t * //ֻҪ֧��sendfile��������û������sendfile on�����ߵ��ú����У����ǿ������첽aio
ngx_linux_sendfile_chain(ngx_connection_t *c, ngx_chain_t *in, off_t limit)
{ //���˵����ݷ��ͣ����ᾭ������filterģ�飬��ͻ��˵İ�����Ӧ�ᾭ������filterģ��
    int            tcp_nodelay;
    off_t          send, prev_send;
    size_t         file_size, sent;
    ssize_t        n;
    ngx_err_t      err;
    ngx_buf_t     *file;
    ngx_event_t   *wev;
    ngx_chain_t   *cl;
    ngx_iovec_t    header;
    struct iovec   headers[NGX_IOVS_PREALLOCATE];
#if (NGX_THREADS)
    ngx_int_t      rc;
    ngx_uint_t     thread_handled, thread_complete;
#endif

    ngx_log_debugall(c->log, 0, "@@@@@@@@@@@@@@@@@@@@@@@begin ngx_linux_sendfile_chain @@@@@@@@@@@@@@@@@@@");
    wev = c->write;

    if (!wev->ready) {
        return in;
    }

    /* the maximum limit size is 2G-1 - the page size */

    if (limit == 0 || limit > (off_t) (NGX_SENDFILE_MAXSIZE - ngx_pagesize)) {
        limit = NGX_SENDFILE_MAXSIZE - ngx_pagesize;
    }


    send = 0;

    header.iovs = headers;
    header.nalloc = NGX_IOVS_PREALLOCATE;

    for ( ;; ) {
        prev_send = send;
#if (NGX_THREADS)
        thread_handled = 0;
        thread_complete = 0;
#endif

        /* create the iovec and coalesce the neighbouring bufs */
        //��in���е�buf������vec->iovs[n++]�У�ע��ֻ�´���ڴ��е����ݵ�iovec�У����´���ļ��е�
        cl = ngx_output_chain_to_iovec(&header, in, limit - send, c->log);

        if (cl == NGX_CHAIN_ERROR) {
            return NGX_CHAIN_ERROR;
        }

        send += header.size; //in����������size֮��

        /* set TCP_CORK if there is a header before a file */

        if (c->tcp_nopush == NGX_TCP_NOPUSH_UNSET
            && header.count != 0 //����0�������chain���е������������ļ���
            && cl
            && cl->buf->in_file)
        {
            /* the TCP_CORK and TCP_NODELAY are mutually exclusive */

            if (c->tcp_nodelay == NGX_TCP_NODELAY_SET) {

                tcp_nodelay = 0;

                if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                               (const void *) &tcp_nodelay, sizeof(int)) == -1)
                {
                    err = ngx_socket_errno;

                    /*
                     * there is a tiny chance to be interrupted, however,
                     * we continue a processing with the TCP_NODELAY
                     * and without the TCP_CORK
                     */

                    if (err != NGX_EINTR) {
                        wev->error = 1;
                        ngx_connection_error(c, err,
                                             "setsockopt(TCP_NODELAY) failed");
                        return NGX_CHAIN_ERROR;
                    }

                } else {
                    c->tcp_nodelay = NGX_TCP_NODELAY_UNSET;

                    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, 0,
                                   "no tcp_nodelay");
                }
            }

            if (c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {

                if (ngx_tcp_nopush(c->fd) == NGX_ERROR) {
                    err = ngx_socket_errno;

                    /*
                     * there is a tiny chance to be interrupted, however,
                     * we continue a processing without the TCP_CORK
                     */

                    if (err != NGX_EINTR) {
                        wev->error = 1;
                        ngx_connection_error(c, err,
                                             ngx_tcp_nopush_n " failed");
                        return NGX_CHAIN_ERROR;
                    }

                } else {
                    c->tcp_nopush = NGX_TCP_NOPUSH_SET;

                    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, 0,
                                   "tcp_nopush");
                }
            }
        }

        /* get the file buf */
        //=0����0�������chain���е������������ļ��У�һ��sendfile on��ʱ��������
        /*
         ˵��chain�е�������in_file�ģ�Ҳ�����ڻ����ļ��У�һ�㿪��sendfile on��ʱ��������,��Ϊngx_output_chain_as_is����1���������¿����ڴ�ռ��ȡ�������ݡ�
         in_file�е��ڴ滹��in_file�ģ������´�����·�����ڴ��У� �ο�ngx_http_copy_filter->ngx_output_chain   ngx_output_chain_as_is��
            */
        if (header.count == 0 && cl && cl->buf->in_file && send < limit) {
            file = cl->buf;

            /* coalesce the neighbouring file bufs */

            file_size = (size_t) ngx_chain_coalesce_file(&cl, limit - send);

            send += file_size;
#if 1
            if (file_size == 0) {
                ngx_debug_point();
                return NGX_CHAIN_ERROR;
            }
#endif

#if (NGX_THREADS)
            if (file->file->thread_handler) {
                rc = ngx_linux_sendfile_thread(c, file, file_size, &sent);

                switch (rc) {
                case NGX_OK:
                    thread_handled = 1;
                    break;

                case NGX_DONE:
                    thread_complete = 1;
                    break;

                case NGX_AGAIN:
                    break;

                default: /* NGX_ERROR */
                    return NGX_CHAIN_ERROR;
                }

            } else
#endif
            {
                n = ngx_linux_sendfile(c, file, file_size);

                if (n == NGX_ERROR) {
                    return NGX_CHAIN_ERROR;
                }

                sent = (n == NGX_AGAIN) ? 0 : n;
            }

        } else {
         /*
         ˵��chain�е��������ڴ��У�һ�㲻����sendfile on��ʱ��������,��Ϊngx_http_copy_filter->ngx_output_chain�л�����
         �����ڴ��ȡ�����ļ����ݣ���ngx_output_chain_as_is��֮ǰbuf->in_file�����ݾͻ����ڴ��͵�
            */n = ngx_writev(c, &header);

            if (n == NGX_ERROR) {
                return NGX_CHAIN_ERROR;
            }

            sent = (n == NGX_AGAIN) ? 0 : n;
        }

        c->sent += sent;

        in = ngx_chain_update_sent(in, sent);

        if ((size_t) (send - prev_send) != sent) {
#if (NGX_THREADS)
            if (thread_handled) {
                return in;
            }

            if (thread_complete) {
                send = prev_send + sent;
                continue;
            }
#endif
            wev->ready = 0;
            return in;
        }

        if (send >= limit || in == NULL) {
            return in;
        }
    }
}

/*
rocktmq�ж��㿽���Ľ���
��1���㿽��ԭ��Consumer������Ϣ���̣�ʹ�����㿽�����㿽������һ��2�з�ʽ��RocketMQʹ�õ�һ�ַ�ʽ����С�����ݴ����Ҫ��Ч����sendfile��ʽ��
    a )ʹ��mmap+write��ʽ   (mmap��һ���ļ�������������ӳ����ڴ�)
     �ŵ㣺��ʹƵ�����ã�ʹ��С�ļ��鴫�䣬Ч��Ҳ�ܸ�
     ȱ�㣺���ܺܺõ�����DMA��ʽ�����sendfile������CPU��Դ���ڴ氲ȫ�Կ��Ƹ��ӣ���Ҫ����JVM Crash����
    b��ʹ��sendfile��ʽ
     �ŵ㣺��������DMA��ʽ������CPU��Դ�٣�����ļ�����Ч�ʸߣ����ڴ氲ȫ������
     ȱ�㣺С���ļ�Ч�ʵ���mmap��ʽ��ֻ����BIO��ʽ���䣬����ʹ��NIO

    mmap��һ���ڴ�ӳ���ļ��ķ���������һ���ļ�������������ӳ�䵽���̵ĵ�ַ�ռ䣬ʵ���ļ����̵�ַ�ͽ��������ַ�ռ�
��һ�������ַ��һһ��ӳ��ϵ��ʵ��������ӳ���ϵ�󣬽��̾Ϳ��Բ���ָ��ķ�ʽ��д������һ���ڴ棬��ϵͳ���Զ���
д��ҳ�浽��Ӧ���ļ������ϣ�������˶��ļ��Ĳ����������ٵ���read,write��ϵͳ���ú������෴���ں˿ռ���������
���޸�Ҳֱ�ӷ�ӳ�û��ռ䣬�Ӷ�����ʵ�ֲ�ͬ���̼���ļ�����


http://www.linuxjournal.com/article/6345?page=0,0
http://blog.csdn.net/kisimple/article/details/42499225
*/
static ssize_t
ngx_linux_sendfile(ngx_connection_t *c, ngx_buf_t *file, size_t size)
{
#if (NGX_HAVE_SENDFILE64)
    off_t      offset;
#else
    int32_t    offset;
#endif
    ssize_t    n;
    ngx_err_t  err;

#if (NGX_HAVE_SENDFILE64)
    offset = file->file_pos;
#else
    offset = (int32_t) file->file_pos;
#endif

eintr:

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "sendfile: @%O %uz", file->file_pos, size);
  //һ��󻺴��ļ���aio���ͣ�С�ļ���sendfile����Ϊaio���첽�ģ���Ӱ���������̣�����sendfile��ͬ���ģ�̫��Ļ�������Ҫ���sendfile���ܷ����꣬���������о�

    //���������ں�̬���û�̬֮��������ڴ渴�ƣ������ͻ�Ӵ����ж�ȡ�ļ���ֱ�����ں�̬���͵������豸��
    n = sendfile(c->fd, file->file->fd, &offset, size);

    if (n == -1) {
        err = ngx_errno;

        switch (err) {
        case NGX_EAGAIN:
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "sendfile() is not ready");
            return NGX_AGAIN;

        case NGX_EINTR:
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "sendfile() was interrupted");
            goto eintr;

        default:
            c->write->error = 1;
            ngx_connection_error(c, err, "sendfile() failed");
            return NGX_ERROR;
        }
    }

    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0, "sendfile: %z of %uz @%O",
                   n, size, file->file_pos);

    return n;
}


#if (NGX_THREADS)
//ngx_linux_sendfile_thread�д����ռ�͸�ֵ
typedef struct {
    ngx_buf_t     *file;
    ngx_socket_t   socket;
    size_t         size;

    size_t         sent;
    ngx_err_t      err;
} ngx_linux_sendfile_ctx_t;


static ngx_int_t
ngx_linux_sendfile_thread(ngx_connection_t *c, ngx_buf_t *file, size_t size,
    size_t *sent)
{
    ngx_uint_t                 flags;
    ngx_event_t               *wev;
    ngx_thread_task_t         *task;
    ngx_linux_sendfile_ctx_t  *ctx;

    ngx_log_debug3(NGX_LOG_DEBUG_CORE, c->log, 0,
                   "linux sendfile thread: %d, %uz, %O",
                   file->file->fd, size, file->file_pos);

    task = c->sendfile_task;

    if (task == NULL) {
        task = ngx_thread_task_alloc(c->pool, sizeof(ngx_linux_sendfile_ctx_t));
        if (task == NULL) {
            return NGX_ERROR;
        }

        task->handler = ngx_linux_sendfile_thread_handler;

        c->sendfile_task = task;
    }

    ctx = task->ctx;
    wev = c->write;

    if (task->event.complete) {
        task->event.complete = 0;

        if (ctx->err && ctx->err != NGX_EAGAIN) {
            wev->error = 1;
            ngx_connection_error(c, ctx->err, "sendfile() failed");
            return NGX_ERROR;
        }

        *sent = ctx->sent;

        return (ctx->sent == ctx->size) ? NGX_DONE : NGX_AGAIN;
    }

    ctx->file = file;
    ctx->socket = c->fd;
    ctx->size = size;

    if (wev->active) {
        flags = (ngx_event_flags & NGX_USE_CLEAR_EVENT) ? NGX_CLEAR_EVENT
                                                        : NGX_LEVEL_EVENT;

        if (ngx_del_event(wev, NGX_WRITE_EVENT, flags) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    if (file->file->thread_handler(task, file->file) != NGX_OK) {
        return NGX_ERROR;
    }

    *sent = 0;

    return NGX_OK;
}


static void
ngx_linux_sendfile_thread_handler(void *data, ngx_log_t *log)
{
    ngx_linux_sendfile_ctx_t *ctx = data;

    off_t       offset;
    ssize_t     n;
    ngx_buf_t  *file;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, log, 0, "linux sendfile thread handler");

    file = ctx->file;
    offset = file->file_pos;

again:

    n = sendfile(ctx->socket, file->file->fd, &offset, ctx->size);

    if (n == -1) {
        ctx->err = ngx_errno;

    } else {
        ctx->sent = n;
        ctx->err = 0;
    }

#if 0
    ngx_time_update();
#endif

    ngx_log_debug4(NGX_LOG_DEBUG_EVENT, log, 0,
                   "sendfile: %z (err: %i) of %uz @%O",
                   n, ctx->err, ctx->size, file->file_pos);

    if (ctx->err == NGX_EINTR) {
        goto again;
    }
}

#endif /* NGX_THREADS */
