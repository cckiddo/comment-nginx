
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#if (NGX_HAVE_KQUEUE)

ssize_t
ngx_unix_recv(ngx_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    ngx_err_t     err;
    ngx_event_t  *rev;

    rev = c->read;

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "recv: eof:%d, avail:%d, err:%d",
                       rev->pending_eof, rev->available, rev->kq_errno);

        if (rev->available == 0) {
            if (rev->pending_eof) {
                rev->ready = 0;
                rev->eof = 1;

                if (rev->kq_errno) {
                    rev->error = 1;
                    ngx_set_socket_errno(rev->kq_errno);

                    return ngx_connection_error(c, rev->kq_errno,
                               "kevent() reported about an closed connection");
                }

                return 0;

            } else {
                rev->ready = 0;
                return NGX_AGAIN;
            }
        }
    }

    do {
        n = recv(c->fd, buf, size, 0);

        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "recv: fd:%d %d of %d", c->fd, n, size);

        if (n >= 0) {
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

                if (n == 0) {

                    /*
                     * on FreeBSD recv() may return 0 on closed socket
                     * even if kqueue reported about available data
                     */

                    rev->ready = 0;
                    rev->eof = 1;
                    rev->available = 0;
                }

                return n;
            }

            if ((size_t) n < size
                && !(ngx_event_flags & NGX_USE_GREEDY_EVENT))
            {
                rev->ready = 0;
            }

            if (n == 0) {
                rev->eof = 1;
            }

            return n;
        }

        err = ngx_socket_errno;

        if (err == NGX_EAGAIN || err == NGX_EINTR) {
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "recv() not ready");
            n = NGX_AGAIN;

        } else {
            n = ngx_connection_error(c, err, "recv() failed");
            break;
        }

    } while (err == NGX_EINTR);

    rev->ready = 0;

    if (n == NGX_ERROR) {
        rev->error = 1;
    }

    return n;
}

#else /* ! NGX_HAVE_KQUEUE */

ssize_t
ngx_unix_recv(ngx_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    ngx_err_t     err;
    ngx_event_t  *rev;
    int ready = 0;

    rev = c->read;

    do {
        /*
            ��Է�����I/Oִ�е�ϵͳ�����������������أ��������¼�����Ѿ�����������¼�û��������������Щϵͳ���þ�
        ���ء�1���ͳ�������һ������ʱ���Ǳ������errno�������������������accept��send��recv���ԣ��¼�δ��ţʱerrno
        ͨ�������ó�EAGAIN����Ϊ������һ�Ρ�������EWOULDBLOCK����Ϊ���ڴ�������������conncct���ԣ�errno��
        ���ó�EINPROGRESS����Ϊ���ڴ�����"����
          */
        //n = recv(c->fd, buf, size, 0); yang test
        //These calls return the number of bytes received, or -1 if an error occurred.  The return value will be 0 when the peer has performed an orderly shutdown.
        n = recv(c->fd, buf, size, 0);//��ʾTCP���󣬼�ngx_http_read_request_header   recv����0��ʾ�Է��Ѿ��ر�����

        //��ȡ�ɹ���ֱ�ӷ���   



        //recv����0�����˲�Ӧ��ȥ�ر����ӣ��������Ϊ�Զ�ʹ����shutdown���رհ����ӣ����˻��ǿ��Է������ݵģ�֪ʶ���ܶ����ݣ�����������ready=0
        //������ǶԶ�shutdown����ô˵������Ϊ�����������ݶ����ˣ�û�����ˣ����������ݣ����Է���0������0���ܱ��˲��ܹر��׽���
        //recv����0����ʾ�Զ�ʹ��shutdown��ʵ�ְ�رջ����첽��д������£�������û�����ݿɶ���Ҳ�᷵��0��send����0���������������
        if (n == 0) { //��ʾTCP���󣬼�ngx_http_read_request_header   recv����0��ʾ�Է��Ѿ��ر����� The return value will be 0 when the peer has performed an orderly shutdown.
            rev->ready = 0;//���ݶ�ȡ���ready��0
            rev->eof = 1;
            goto end;

        } else if (n > 0) {
            //�ڴ�����1000�ֽڣ�ʵ���Ϸ���500�ֽڣ�˵���ں˻��������յ���500�ֽں��Ѿ����ˣ�������д, readΪ0��ֻ�е�epollд�¼����� read
            //���ǣ���������ڴ�����1000�ֽڣ�����500�ֽ���˵�����ں˻�������ֻ��500�ֽڣ���˿��Լ���recv��ready����Ϊ1
            if ((size_t) n < size
                && !(ngx_event_flags & NGX_USE_GREEDY_EVENT)) //���ݶ�ȡ���ready��0,��Ҫ�������add epoll event
            {
                rev->ready = 0; //��������ȡ1000�ֽڣ���ʵ���Ϸ���500�ֽڣ�˵���ں˻����������Ѿ���ȡ���  epoll�����ߵ�����
            }

            goto end;
        }

        //����ں����ݽ�����ϣ����ߵ�����nΪ-1��errΪNGX_EAGAIN
        err = ngx_socket_errno;
        
        /* 
          EINTR����Ĳ�������������ĳ����ϵͳ���õ�һ�����̲���ĳ���ź�����Ӧ�źŴ���������ʱ����ϵͳ���ÿ��ܷ���һ��EINTR����   
          ��linux���з�������socket��������ʱ��������Resource temporarily unavailable��errno����Ϊ11(EAGAIN)����������ڷ�����ģʽ�µ���������������
          �ڸò���û����ɾͷ����������������󲻻��ƻ�socket��ͬ�������ù������´�ѭ������recv�Ϳ��ԡ��Է�����socket���ԣ�EAGAIN����һ�ִ���
          ��VxWorks��Windows�ϣ�EAGAIN�����ֽ���EWOULDBLOCK�� ���⣬�������EINTR��errnoΪ4����������Interrupted system call������ҲӦ�ü�����

          ��Linux�����¿��������������ܶ����(����errno)������EAGAIN�����бȽϳ�����һ������(�������ڷ�����������)��
������������������ʾ����һ�Ρ�������󾭳������ڵ�Ӧ�ó������һЩ������(non-blocking)����(���ļ���socket)��ʱ��
���磬�� O_NONBLOCK�ı�־���ļ�/socket/FIFO�������������read������û�����ݿɶ�����ʱ���򲻻����������ȴ�����׼���������أ�
read�����᷵��һ������EAGAIN����ʾ���Ӧ�ó�������û�����ݿɶ����Ժ����ԡ�
          */
        if (err == NGX_EAGAIN || err == NGX_EINTR) {  //��������� ,��Ҫ������
            
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, err,
                           "recv() not ready"); //recv() not ready (11: Resource temporarily unavailable)
            n = NGX_AGAIN; //���������ʾ�ں��������е������Ѿ���ȡ�꣬��Ҫ����add epoll event������������epoll����

        } else {//TCP���ӳ�����
            n = ngx_connection_error(c, err, "recv() failed");
            break;
        }

    } while (err == NGX_EINTR); //����������б��ж��л����������

    rev->ready = 0;

    if (n == NGX_ERROR) {
        rev->error = 1;
    }

end:
    ready = rev->ready;
    ngx_log_debug4(NGX_LOG_DEBUG_EVENT, c->log, 0,
                           "recv: fd:%d read-size:%d of %d, ready:%d", c->fd, n, size, ready);
    return n;
}

#endif /* NGX_HAVE_KQUEUE */
