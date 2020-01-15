/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#if (NGX_THREADS)
#include <ngx_thread_pool.h>
static void ngx_thread_read_handler(void *data, ngx_log_t *log);
#endif


#if (NGX_HAVE_FILE_AIO)

/*
�ļ��첽IO
    �¼�����ģ�鶼���ڴ��������¼�����û���漰�������ļ��Ĳ�������
�ڽ�����Linux�ں�2.6.2x֮��汾��֧�ֵ��ļ��첽I/O���Լ�ngx_epoll_moduleģ����
������ļ��첽I/O����ṩ����ġ������ᵽ���ļ��첽I/O������glibc���ṩ���ļ���
��I/O��glibc���ṩ���첽I/O�ǻ��ڶ��߳�ʵ�ֵģ����������������ϵ��첽I/O��������
˵�����첽I/O����Linux�ں�ʵ�֣�ֻ�����ں��гɹ�������˴��̲������ں˲Ż�֪ͨ
���̣�����ʹ�ô����ļ��Ĵ����������¼��Ĵ���ͬ����Ч��
    ʹ�����ַ�ʽ��ǰ����Linux�ں˰汾�б���֧���ļ��첽I/O����Ȼ���������ĺô�
Ҳ�ǳ����ԣ�Nginx�Ѷ�ȡ�ļ��Ĳ����첽���ύ���ں˺��ں˻�֪ͨI/O�豸������ִ
�в�����������Nginx���̿��Լ�����ֵ�ռ��CPU�����ң����������¼��ѻ���I/O�豸
�Ķ�����ʱ�����ᷢ�ӳ��ں��С������㷨�������ƣ��Ӷ����������ȡ���������ĳɱ���
    ע��Linux�ں˼�����ļ��첽I/O�ǲ�֧�ֻ�������ģ�Ҳ����˵����ʹ��Ҫ����
���ļ�����Linux�ļ������д��ڣ�Ҳ����ͨ����ȡ�����Ļ����е��ļ���������ʵ�ʶԴ�
�̵Ĳ�������Ȼ������worker���̵ĽǶ�����˵���˺ܴ��ת�����ǶԵ���������˵������
�п��ܽ���ʵ�ʴ�����ٶȣ���Ϊԭ�ȿ��Դ��ڴ��п��ٻ�ȡ���ļ�����ʹ�����첽I/O��
��һ����Ӵ����϶�ȡ���첽�ļ�I/O�ǰѡ�˫�н������ؼ�Ҫ��ʹ�ó���������󲿷��û�
������ļ��Ĳ��������䵽�ļ������У���ô��Ҫʹ���첽I/O����֮���������ʹ���ļ�
�첽I/O����һ���Ƿ��Ϊ����������������ϵ�������
    Ŀǰ��Nginx��֧���ڶ�ȡ�ļ�ʱʹ���첽I/O����Ϊ����д���ļ�ʱ������д���ڴ�
�о����̷��أ�Ч�ʺܸߣ���ʹ���첽I/Oд��ʱ�ٶȻ������½���
�ļ��첽AIO�ŵ�:
        �첽I/O����Linux�ں�ʵ�֣�ֻ�����ں��гɹ�������˴��̲������ں˲Ż�֪ͨ
    ���̣�����ʹ�ô����ļ��Ĵ����������¼��Ĵ���ͬ����Ч�������Ͳ�������worker���̡�
ȱ��:
        ��֧�ֻ�������ģ�Ҳ����˵����ʹ��Ҫ�������ļ�����Linux�ļ������д��ڣ�Ҳ����ͨ����ȡ��
    ���Ļ����е��ļ���������ʵ�ʶԴ��̵Ĳ������п��ܽ���ʵ�ʴ�����ٶȣ���Ϊԭ�ȿ��Դ��ڴ��п���
    ��ȡ���ļ�����ʹ�����첽I/O����һ����Ӵ����϶�ȡ
������ѡ���첽I/O������ͨI/O������?
        �첽�ļ�I/O�ǰѡ�˫�н������ؼ�Ҫ��ʹ�ó���������󲿷��û�
    ������ļ��Ĳ��������䵽�ļ������У���ô��Ҫʹ���첽I/O����֮���������ʹ���ļ�
    �첽I/O����һ���Ƿ��Ϊ����������������ϵ�������
        Ŀǰ��Nginx��֧���ڶ�ȡ�ļ�ʱʹ���첽I/O����Ϊ����д���ļ�ʱ������д���ڴ�
    �о����̷��أ�Ч�ʺܸߣ���ʹ���첽I/Oд��ʱ�ٶȻ������½����첽I/O��֧��д��������Ϊ
    �첽I/O�޷����û��棬��д����ͨ�����䵽�����ϣ�linux���Զ����ļ��л����е�����д������
    
    ��ͨ�ļ���д����:
    ������ϵͳ����read/write���������������أ�
    - ��ȡ���ں˻�������Ҫ���ļ�����:�ں˻�����->�û�������;û��:Ӳ��->�ں˻�����->�û�������;
    - д�أ����ݻ���û���ַ�ռ俽��������ϵͳ�ں˵�ַ�ռ��ҳ������ȥ������write�ͻ�ֱ�ӷ��أ�����ϵͳ����ǡ����ʱ��д����̣�����Ǵ�˵�е�
*/
//direct AIO���Բο�http://blog.csdn.net/bengda/article/details/21871413

ngx_uint_t  ngx_file_aio = 1; //�������ngx_eventfdʧ�ܣ���0����ʾ��֧��AIO

#endif


ssize_t
ngx_read_file(ngx_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n;

    ngx_log_debug5(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "read file %V: %d, %p, %uz, %O", &file->name, file->fd, buf, size, offset);

#if (NGX_HAVE_PREAD)  //�����ýű��и�ֵauto/unix:ngx_feature_name="NGX_HAVE_PREAD"

    n = pread(file->fd, buf, size, offset);//pread() ���ļ� fd ָ����ƫ�� offset (����ļ���ͷ) �϶�ȡ count ���ֽڵ� buf ��ʼλ�á��ļ���ǰλ��ƫ�Ʊ��ֲ��䡣 

    if (n == -1) {
        ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                      "pread() \"%s\" failed", file->name.data);
        return NGX_ERROR;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        file->sys_offset = offset;
    }

    n = read(file->fd, buf, size);

    if (n == -1) {
        ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                      "read() \"%s\" failed", file->name.data);
        return NGX_ERROR;
    }

    file->sys_offset += n;

#endif

    file->offset += n;//ÿ��n�ֽڣ��ļ���ȡƫ�����ͼ�n

    return n;
}


#if (NGX_THREADS)

//ngx_thread_read�д����ռ�͸�ֵ
typedef struct {
    ngx_fd_t     fd; //�ļ�fd
    u_char      *buf; //��ȡ�ļ����ݵ���buf��
    size_t       size; //��ȡ�ļ����ݴ�С
    off_t        offset; //���ļ�offset��ʼ����ȡsize�ֽڵ�buf��

    size_t       read; //ͨ��ngx_thread_read_handler��ȡ�����ֽ���
    ngx_err_t    err; //ngx_thread_read_handler��ȡ���غ�Ĵ�����Ϣ
} ngx_thread_read_ctx_t; //��ngx_thread_read���ýṹ��ngx_thread_task_t->ctxָ��

//��һ�ν�����ʱ���ʾ��ʼ�Ѷ���������̳߳��д�����ʾ���ڿ�ʼ�����ڶ��ν�����ʱ���ʾ�����Ѿ�ͨ��notify_epoll֪ͨ��ȡ��ϣ����Դ����ˣ���һ�η���NAX_AGAIN
//�ڶ��ηŻ��̳߳��е��̴߳���������ȡ�����ֽ���
ssize_t
ngx_thread_read(ngx_thread_task_t **taskp, ngx_file_t *file, u_char *buf,
    size_t size, off_t offset, ngx_pool_t *pool)
{
    /*
        �ú���һ���������Σ���һ����ͨ��ԭʼ���ݷ��ʹ����ߵ������ʱ��complete = 0���ڶ����ǵ��̳߳ض�ȡ������ɣ����ͨ��
        ngx_thread_pool_handler->ngx_http_copy_thread_event_handler->ngx_http_request_handler->ngx_http_writer�ڴ��ߵ�����
     */
    ngx_thread_task_t      *task;
    ngx_thread_read_ctx_t  *ctx;

    ngx_log_debug4(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "thread read: fd:%d, buf:%p, size:%uz, offset:%O",
                   file->fd, buf, size, offset);

    task = *taskp;

    if (task == NULL) {
        task = ngx_thread_task_alloc(pool, sizeof(ngx_thread_read_ctx_t));
        if (task == NULL) {
            return NGX_ERROR;
        }

        task->handler = ngx_thread_read_handler;

        *taskp = task;
    }

    ctx = task->ctx;

    if (task->event.complete) {
    /*
    �ú���һ���������Σ���һ����ͨ��ԭʼ���ݷ��ʹ����ߵ������ʱ��complete = 0���ڶ����ǵ��̳߳ض�ȡ������ɣ����ͨ��
    ngx_thread_pool_handler->ngx_http_copy_thread_event_handler->ngx_http_request_handler->ngx_http_writer�ڴ��ߵ��������
    ���complete�Ѿ���ngx_thread_pool_handler��1
     */   
        task->event.complete = 0;

        if (ctx->err) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ctx->err,
                          "pread() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        return ctx->read;
    }

    ctx->fd = file->fd;
    ctx->buf = buf;
    ctx->size = size;
    ctx->offset = offset;

    //�������task->event��Ϣ��task�У���task->handlerָ�����ͨ��nginx_notify���Լ���ͨ��epoll_wait����ִ��task->event
    //�ͻ��˹���������л�����ڣ���ngx_http_file_cache_aio_read�и�ֵΪngx_http_cache_thread_handler;  
    //����ǴӺ�˻�ȡ�����ݣ�Ȼ���͸��ͻ��ˣ���ngx_output_chain_as_is�и�ֵδngx_http_copy_thread_handler
    if (file->thread_handler(task, file) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_AGAIN;
}


#if (NGX_HAVE_PREAD)
//��ngx_thread_read�Ѹ�handler��ӵ��̳߳���
static void //ngx_thread_read->ngx_thread_read_handler
ngx_thread_read_handler(void *data, ngx_log_t *log)
{//�ú���ִ�к󣬻�ͨ��ngx_notifyִ��event.handler = ngx_http_cache_thread_event_handler;
    ngx_thread_read_ctx_t *ctx = data;

    ssize_t  n;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, log, 0, "thread read handler");

    //�����ļ����ݻ´����dst�У�Ҳ����ngx_output_chain_ctx_t->buf,Ȼ����ngx_output_chain_copy_buf�����������°�ctx->buf��ֵ���µ�chain��Ȼ��write��ȥ
    n = pread(ctx->fd, ctx->buf, ctx->size, ctx->offset);

    if (n == -1) {
        ctx->err = ngx_errno;

    } else {
        ctx->read = n;
        ctx->err = 0;
    }

#if 0
    ngx_time_update();
#endif

    ngx_log_debug4(NGX_LOG_DEBUG_CORE, log, 0,
                   "pread read return read size: %z (err: %i) of buf-size%uz offset@%O",
                   n, ctx->err, ctx->size, ctx->offset);
}

#else

#error pread() is required!

#endif

#endif /* NGX_THREADS */


ssize_t
ngx_write_file(ngx_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n, written;

    ngx_log_debug5(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "write to filename:%V,fd: %d, buf:%p, size:%uz, offset:%O", &file->name, file->fd, buf, size, offset);

    written = 0;

#if (NGX_HAVE_PWRITE)

    for ( ;; ) {
        //pwrite() �ѻ����� buf ��ͷ�� count ���ֽ�д���ļ������� fd offset ƫ��λ���ϡ��ļ�ƫ��û�иı䡣
        n = pwrite(file->fd, buf + written, size, offset);

        if (n == -1) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                          "pwrite() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        offset += n;
        size -= n;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        file->sys_offset = offset;
    }

    for ( ;; ) {
        n = write(file->fd, buf + written, size);

        if (n == -1) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                          "write() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        size -= n;
    }
#endif
}


ngx_fd_t
ngx_open_tempfile(u_char *name, ngx_uint_t persistent, ngx_uint_t access)
{
    ngx_fd_t  fd;

    fd = open((const char *) name, O_CREAT|O_EXCL|O_RDWR,
              access ? access : 0600);

    if (fd != -1 && !persistent) {
        /*
        unlink����ʹ�ļ���������һ����������Ϊ��ʱ������ϵͳ��ɾ���ļ��������н����Ѿ����ļ�����ֻ�����һ�����ø��ļ����ļ�
        �������رգ����ļ��Żᱻɾ����
          */
        (void) unlink((const char *) name); //���һ���ļ�����unlink���򵱹ر�fd��ʱ�򣬻�ɾ�����ļ�
    }

    return fd;
}


#define NGX_IOVS  8
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

ssize_t
ngx_write_chain_to_file(ngx_file_t *file, ngx_chain_t *cl, off_t offset,
    ngx_pool_t *pool)
{
    u_char        *prev;
    size_t         size;
    ssize_t        total, n;
    ngx_array_t    vec;
    struct iovec  *iov, iovs[NGX_IOVS];

    /* use pwrite() if there is the only buf in a chain */

    if (cl->next == NULL) { //ֻ��һ��buf�ڵ�
        return ngx_write_file(file, cl->buf->pos,
                              (size_t) (cl->buf->last - cl->buf->pos),
                              offset);
    }

    total = 0; //�����ܹ�д���ļ��е��ֽ�������cl������bufָ����ڴ�ռ��С���

    vec.elts = iovs;
    vec.size = sizeof(struct iovec);
    vec.nalloc = NGX_IOVS;
    vec.pool = pool;

    do {
        prev = NULL;
        iov = NULL;
        size = 0;

        vec.nelts = 0;

        /* create the iovec and coalesce the neighbouring bufs */

        while (cl && vec.nelts < IOV_MAX) { //��cl���е�����ÿһ��chain�ڵ����ӵ�һ��iov��
            if (prev == cl->buf->pos) { //��һ��chain���е�����buf�ŵ�һ��iov��
                iov->iov_len += cl->buf->last - cl->buf->pos;

            } else {
                iov = ngx_array_push(&vec);
                if (iov == NULL) {
                    return NGX_ERROR;
                }

                iov->iov_base = (void *) cl->buf->pos;
                iov->iov_len = cl->buf->last - cl->buf->pos;
            }

            size += cl->buf->last - cl->buf->pos; //clΪ�������ݵĳ��Ⱥ�
            prev = cl->buf->last;
            cl = cl->next;
        } //���cl���е�����chain����������IOV_MAX��������Ҫ�´μ����ں���while (cl);�ع�������

        /* use pwrite() if there is the only iovec buffer */

        if (vec.nelts == 1) {
            iov = vec.elts;

            n = ngx_write_file(file, (u_char *) iov[0].iov_base,
                               iov[0].iov_len, offset);

            if (n == NGX_ERROR) {
                return n;
            }

            return total + n;
        }

        if (file->sys_offset != offset) {
            if (lseek(file->fd, offset, SEEK_SET) == -1) {
                ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                              "lseek() \"%s\" failed", file->name.data);
                return NGX_ERROR;
            }

            file->sys_offset = offset;
        }

        n = writev(file->fd, vec.elts, vec.nelts);

        if (n == -1) {
            ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                          "writev() \"%s\" failed", file->name.data);
            return NGX_ERROR;
        }

        if ((size_t) n != size) {
            ngx_log_error(NGX_LOG_CRIT, file->log, 0,
                          "writev() \"%s\" has written only %z of %uz",
                          file->name.data, n, size);
            return NGX_ERROR;
        }

        ngx_log_debug3(NGX_LOG_DEBUG_CORE, file->log, 0,
                       "writev to filename:%V,fd: %d, readsize: %z", &file->name, file->fd, n);

        file->sys_offset += n;
        file->offset += n;
        offset += n;
        total += n;

    } while (cl);//���cl���е�����chain����������IOV_MAX��������Ҫ�´μ����ں���while (cl);�ع�������

    return total;
}


ngx_int_t
ngx_set_file_time(u_char *name, ngx_fd_t fd, time_t s)
{
    struct timeval  tv[2];

    tv[0].tv_sec = ngx_time();
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes((char *) name, tv) != -1) {
        return NGX_OK;
    }

    return NGX_ERROR;
}


ngx_int_t
ngx_create_file_mapping(ngx_file_mapping_t *fm)
{
    fm->fd = ngx_open_file(fm->name, NGX_FILE_RDWR, NGX_FILE_TRUNCATE,
                           NGX_FILE_DEFAULT_ACCESS);
    if (fm->fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_CRIT, fm->log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", fm->name);
        return NGX_ERROR;
    }

    if (ftruncate(fm->fd, fm->size) == -1) {
        ngx_log_error(NGX_LOG_CRIT, fm->log, ngx_errno,
                      "ftruncate() \"%s\" failed", fm->name);
        goto failed;
    }

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_CRIT, fm->log, ngx_errno,
                  "mmap(%uz) \"%s\" failed", fm->size, fm->name);

failed:

    if (ngx_close_file(fm->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, fm->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", fm->name);
    }

    return NGX_ERROR;
}


void
ngx_close_file_mapping(ngx_file_mapping_t *fm)
{
    if (munmap(fm->addr, fm->size) == -1) {
        ngx_log_error(NGX_LOG_CRIT, fm->log, ngx_errno,
                      "munmap(%uz) \"%s\" failed", fm->size, fm->name);
    }

    if (ngx_close_file(fm->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, fm->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", fm->name);
    }
}


ngx_int_t
ngx_open_dir(ngx_str_t *name, ngx_dir_t *dir)
{
    dir->dir = opendir((const char *) name->data);

    if (dir->dir == NULL) {
        return NGX_ERROR;
    }

    dir->valid_info = 0;

    return NGX_OK;
}


ngx_int_t
ngx_read_dir(ngx_dir_t *dir)
{
    dir->de = readdir(dir->dir);

    if (dir->de) {
#if (NGX_HAVE_D_TYPE)
        dir->type = dir->de->d_type;
#else
        dir->type = 0;
#endif
        return NGX_OK;
    }

    return NGX_ERROR;
}


ngx_int_t
ngx_open_glob(ngx_glob_t *gl)
{
    int  n;

    n = glob((char *) gl->pattern, 0, NULL, &gl->pglob);

    if (n == 0) {
        return NGX_OK;
    }

#ifdef GLOB_NOMATCH

    if (n == GLOB_NOMATCH && gl->test) {
        return NGX_OK;
    }

#endif

    return NGX_ERROR;
}


ngx_int_t
ngx_read_glob(ngx_glob_t *gl, ngx_str_t *name)
{
    size_t  count;

#ifdef GLOB_NOMATCH
    count = (size_t) gl->pglob.gl_pathc;
#else
    count = (size_t) gl->pglob.gl_matchc;
#endif

    if (gl->n < count) {

        name->len = (size_t) ngx_strlen(gl->pglob.gl_pathv[gl->n]);
        name->data = (u_char *) gl->pglob.gl_pathv[gl->n];
        gl->n++;

        return NGX_OK;
    }

    return NGX_DONE;
}


void
ngx_close_glob(ngx_glob_t *gl)
{
    globfree(&gl->pglob);
}











































/*
Linux fcntl�������(2011-07-18 20:22:14)ת�ب���ǩ�� fcntllinuxcit  
���������������ļ��������������ļ������ԡ�
#include <unistd.h>
#include <fcntl.h>
int fcntl(int fd, int cmd);
int fcntl(int fd, int cmd, long arg);
int fcntl(int fd, int cmd, struct flock *lock);
[����]
fcntl()���(�ļ�)�������ṩ���ơ�����fd�Ǳ�����cmd����(�����������)�������������cmd��ֵ��fcntl�ܹ����ܵ���������int arg��
[����ֵ]
fcntl()�ķ���ֵ�������йء������������������أ�1������ɹ��򷵻�ĳ������ֵ�����������������ض�����ֵ��F_DUPFD , F_GETFD , F_GETFL�Լ�F_GETOWN��
    F_DUPFD   �����µ��ļ�������
    F_GETFD   ������Ӧ��־
    F_GETFL , F_GETOWN   ����һ�����Ľ���ID�򸺵Ľ�����ID
 
fcntl������5�ֹ��ܣ�
1. ����һ�����е�������(cmd=F_DUPFD).
2. ��ã������ļ����������(cmd=F_GETFD��F_SETFD).
3. ��ã������ļ�״̬���(cmd=F_GETFL��F_SETFL).
4. ��ã������첽I/O����Ȩ(cmd=F_GETOWN��F_SETOWN).
5. ��ã����ü�¼��(cmd=F_GETLK , F_SETLK��F_SETLKW).
1. cmdֵ��F_DUPFD ��
F_DUPFD    ����һ������������(�ļ�)��������
        ����С�Ĵ��ڻ����arg��һ�����õ�������
        ����ԭʼ������һ����ĳ���������
        ������������ļ�(file)�Ļ����򷵻�һ���µ��������������������arg������ͬ��ƫ����(offset)
        ����ͬ�ķ���ģʽ(����д���/д)
        ����ͬ���ļ�״̬��־(�磺�����ļ�������������ͬ��״̬��־)
        �����µ��ļ������������һ���close-on-exec��־�����óɽ���ʽ����execve(2)��ϵͳ����
ʵ���ϵ���dup(oldfd)��
��Ч��
        fcntl(oldfd, F_DUPFD, 0);
������dup2(oldfd, newfd)��
��Ч��
        close(oldfd)��
        fcntl(oldfd, F_DUPFD, newfd)��
2. cmdֵ��F_GETFD��F_SETFD��     
F_GETFD    ȡ�����ļ�������fd���ϵ�close-on-exec��־������FD_CLOEXEC���������ֵ��FD_CLOEXEC��������������0�Ļ����ļ����ֽ���ʽ����exec()���������ͨ��exec���еĻ����ļ������ر�(arg ������)       
F_SETFD    ����close-on-exec��־���ñ�־�Բ���arg��FD_CLOEXECλ������Ӧ���˽�ܶ��ִ���漰�ļ���������־�ĳ��򲢲�ʹ�ó��� FD_CLOEXEC�����ǽ��˱�־����Ϊ0(ϵͳĬ�ϣ���execʱ���ر�)��1(��execʱ�ر�)    
���޸��ļ���������־���ļ�״̬��־ʱ�����������Ҫȡ�����ڵı�־ֵ��Ȼ����ϣ���޸�������������±�־ֵ������ֻ��ִ��F_SETFD��F_SETFL���������ر���ǰ���õı�־λ�� 
3. cmdֵ��F_GETFL��F_SETFL��  
F_GETFL    ȡ��fd���ļ�״̬��־����ͬ���������һ��(arg������)����˵��open����ʱ����˵��
���ļ�״̬��־�����ҵ��ǣ�������ȡ��ʽ��־ (O_RDONLY , O_WRONLY , �Լ�O_RDWR)������ռ1λ��(�����ֱ�־��ֵ����0 , 1��2��������ʷԭ��������ֵ���� �� һ���ļ�ֻ����������ֵ֮һ��) ������ȱ�����������O_ACCMODE����ȡ�ô�ȡ��ʽλ��Ȼ�󽫽����������ֵ��Ƚϡ�      
F_SETFL    ���ø�arg������״̬��־�����Ը��ĵļ�����־�ǣ�O_APPEND��O_NONBLOCK��O_SYNC �� O_ASYNC����fcntl���ļ�״̬��־�ܹ���7����O_RDONLY , O_WRONLY , O_RDWR , O_APPEND , O_NONBLOCK , O_SYNC��O_ASYNC
�ɸ��ĵļ�����־�������������
    O_NONBLOCK   ������I/O�����read(2)����û�пɶ�ȡ�����ݣ��������write(2)��������������read��write���ý�����-1��EAGAIN����
    O_APPEND     ǿ��ÿ��д(write)������������ļ����ĩβ���൱��open(2)��O_APPEND��־
    O_DIRECT     ��С����ȥ��reading��writing�Ļ���Ӱ�졣ϵͳ����ͼ���⻺����Ķ���д�����ݡ�������ܹ����⻺�棬��ô������С���Ѿ��������˵�������ɵ�Ӱ�졣��������־�õĲ����ã������Ľ�������
    O_ASYNC      ��I/O���õ�ʱ������SIGIO�źŷ��͵������飬���磺�������ݿ��Զ���ʱ��
4. cmdֵ��F_GETOWN��F_SETOWN��  
F_GETOWN   ȡ�õ�ǰ���ڽ���SIGIO����SIGURG�źŵĽ���id�������id��������id���ص��Ǹ�ֵ(arg������)    
F_SETOWN   ���ý�����SIGIO��SIGURG�źŵĽ���id�������id��������idͨ���ṩ��ֵ��arg��˵��(arg����ֵ��һ��������ID)������arg������Ϊ�ǽ���id
 5. cmdֵ��F_GETLK, F_SETLK��F_SETLKW�� ��ã����ü�¼���Ĺ��ܣ��ɹ��򷵻�0�����д����򷵻�-1������ԭ�����errno��
F_GETLK    ͨ������������arg(һ��ָ��flock�Ľṹ��)ȡ�õ�һ������lock descriptionָ�������ȡ�õ���Ϣ�����Ǵ���fcntl()��flock�ṹ����Ϣ�����û�з����ܹ���ֹ������(flock)���ɵ���������ṹ�������ı䣬�����������ͱ����ó�F_UNLCK   
F_SETLK    ����ָ��ṹ��flock��ָ��ĵ���������arg��������������Ϣ���û������һ���ļ���segment����F_SETLK������ʵ�ֹ���(���)��(F_RDLCK)���ռ(д)��(F_WRLCK)��ͬ������ȥ����������(F_UNLCK)��������������ռ�����ܱ����ã�fcntl()����������EAGAIN    
F_SETLKW   ���˹��������ռ������������������������⣬��������F_SETLK��һ���ġ�������������ռ���������������������̽��ȴ�ֱ����������ܹ���ɡ���fcntl()���ڵȴ��ļ���ĳ�������ʱ��׽��һ���źţ��������ź�û�б�ָ��SA_RESTART, fcntl�����ж�
��һ����������set��һ���ļ���ĳ�ε�ʱ�������Ľ��̿���set������������λ�����ε�һ���֡���������ֹ�κ���������set��ռ������α���������κβ��֡�����ļ�������û���Զ��ķ��ʷ�ʽ�򿪵Ļ��������������������ʧ�ܡ�
��ռ����ֹ�κ������Ľ�������α��������κ�λ�����ù��������ռ��������ļ�������������д�ķ��ʷ�ʽ�򿪵Ļ�����ռ���������ʧ�ܡ�
�ṹ��flock��ָ�룺
struct flcok
{
short int l_type;
//���µ������������ڷֶζ��ļ����������������ļ���������l_whence=SEEK_SET, l_start=0, l_len=0
short int l_whence;
off_t l_start;
off_t l_len;
pid_t l_pid;
};
l_type ������״̬��
F_RDLCK   ����һ������ȡ�õ�����
F_WRLCK   ����һ����д���õ�����
F_UNLCK   ɾ��֮ǰ����������
l_whence Ҳ�����ַ�ʽ��
SEEK_SET   ���ļ���ͷΪ��������ʼλ��
SEEK_CUR   ��Ŀǰ�ļ���дλ��Ϊ��������ʼλ��
SEEK_END   ���ļ���βΪ��������ʼλ��
fcntl�ļ������������ͣ�����������ǿ������
���������������涨�ģ�ÿ��ʹ�������ļ��Ľ��̶�Ҫ����Ƿ��������ڣ���Ȼ�����������е������ں˺�ϵͳ�����϶���ֲ�ʹ�ý���������������������Ա��������涨��
ǿ�����������ں�ִ�еģ����ļ�������������д�����ʱ�����������ļ��Ľ����ͷŸ���֮ǰ���ں˻���ֹ�κζԸ��ļ��Ķ���д���ʣ�ÿ�ζ���д���ʶ��ü�����Ƿ���ڡ�
ϵͳĬ��fcntl���ǽ���������ǿ�������Ƿ�POSIX��׼�ġ����Ҫʹ��ǿ��������Ҫʹ����ϵͳ����ʹ��ǿ����������ô����Ҫ���¹����ļ�ϵͳ��mountʹ�ò��� -0 mand ��ǿ�����������߹ر��Ѽ����ļ�����ִ��Ȩ�޲��Ҵ򿪸��ļ���set-GIDȨ��λ��
��������ֻ��cooperating processes֮������á���cooperating process�����������Ҫ�ģ���ָ���ǻ�Ӱ���������̵Ľ��̻򱻱�Ľ�����Ӱ��Ľ��̣����������ӣ�
(1) ���ǿ���ͬʱ����������������ͬһ�������ͬһ���ļ����в�������ô���������̾���cooperating  processes
(2) cat file | sort����ôcat��sort�����Ľ��̾���ʹ����pipe��cooperating processes
ʹ��fcntl�ļ�������I/O��������С�ģ������ڿ�ʼ�κ�I/O����ǰ���ȥ���������ڶ��ļ�����ǰ���������еĲ������Ǳ��뿼�ǵġ������������֮ǰ���ļ������߶�ȡ����֮��ر��ļ�����һ�����̾Ϳ���������/���������ʹ�/�رղ���֮��ļ���֮һ���ڷ��ʸ��ļ�����һ�����̶��ļ��������������Ƿ��ͷ����ӵ�����ֻҪ�ļ��رգ��ں˶����Զ��ͷż����ļ��ϵĽ�������(��Ҳ�ǽ���������ǿ���������������)�����Բ�Ҫ�����ý����������ﵽ���ò��ñ�Ľ��̷����ļ���Ŀ��(ǿ�������ſ���)��ǿ������������н��������á�
fcntlʹ���������� F_SETLK/F_SETLKW�� F_UNLCK��F_GETLK ���ֱ�Ҫ���ͷš�����record locks��record locks�Ƕ��ļ�һ���ֶ����������ļ�����������ϸ�µĿ���ʹ�ý��̸��õ�Э���Թ����ļ���Դ��fcntl�ܹ����ڶ�ȡ����д������read lockҲ��shared lock(������)�� ��Ϊ���cooperating process�ܹ����ļ���ͬһ���ֽ�����ȡ����write lock����Ϊexclusive lock(�ų���)����Ϊ�κ�ʱ��ֻ����һ��cooperating process���ļ���ĳ�����Ͻ���д���������cooperating processes���ļ����в�������ô���ǿ���ͬʱ���ļ���read lock����һ��cooperating process��write lock֮ǰ�������ͷű��cooperating process���ڸ��ļ���read lock��wrtie lock��Ҳ����˵�������ļ�ֻ����һ��write lock���ڣ�read lock��wrtie lock���ܹ��档
���������ʹ��F_GETFL��ȡfd���ļ�״̬��־��
#include<fcntl.h>
#include<unistd.h>
#include<iostream>
#include<errno.h>
using namespace std;
int main(int argc,char* argv[])
{
  int fd, var;
  //  fd=open("new",O_RDWR);
  if (argc!=2)
  {
      perror("--");
      cout<<"��������������ļ�����"<<endl;
  }
  if((var=fcntl(atoi(argv[1]), F_GETFL, 0))<0)
  {
     strerror(errno);
     cout<<"fcntl file error."<<endl;
  }
  switch(var & O_ACCMODE)
  {
   case O_RDONLY : cout<<"Read only.."<<endl;
                   break;
   case O_WRONLY : cout<<"Write only.."<<endl;
                   break;
   case O_RDWR   : cout<<"Read wirte.."<<endl;
                   break;
   default  : break;
  }
 if (val & O_APPEND)
    cout<<",append"<<endl;
 if (val & O_NONBLOCK)
    cout<<",noblocking"<<endl;
 cout<<"exit 0"<<endl;
 exit(0);
}
Linux fcntl������� .
���ࣺ fcntl 2013-12-07 16:43 183���Ķ� ����(0) �ղ� �ٱ� 
���������������ļ��������������ļ������ԡ�
�ļ����ƺ���          fcntl -- file control
ͷ�ļ���
#include <unistd.h>
#include <fcntl.h>
����ԭ�ͣ�          
int fcntl(int fd, int cmd);
int fcntl(int fd, int cmd, long arg);         
int fcntl(int fd, int cmd, struct flock *lock);
������
           fcntl()���(�ļ�)�������ṩ����.����fd�Ǳ�����cmd����(�����������)��������.            
�����������cmd��ֵ,fcntl�ܹ����ܵ�����������arg��
fcntl������5�ֹ��ܣ�
�������� 1.����һ�����е���������cmd=F_DUPFD��.
����       2.��ã������ļ����������(cmd=F_GETFD��F_SETFD).
            3.��ã������ļ�״̬���(cmd=F_GETFL��F_SETFL).
            4.��ã������첽I/O����Ȩ(cmd=F_GETOWN��F_SETOWN).
            5.��ã����ü�¼��(cmd=F_GETLK,F_SETLK��F_SETLKW).
 
 cmd ѡ�
            F_DUPFD      ����һ������������(�ļ�)������:                            
         ����������������������1����С�Ĵ��ڻ����arg��һ�����õ�������                          
   ��������������������������2����ԭʼ������һ����ĳ���������               
            ����������������  ��3������������ļ�(file)�Ļ�,����һ���µ�������,�����������arg������ͬ��ƫ����(offset)                    
������������������������   ��4����ͬ�ķ���ģʽ(��,д���/д)                          
����������������������������5����ͬ���ļ�״̬��־(��:�����ļ�������������ͬ��״̬��־)                            
����������������������������6�����µ��ļ������������һ���close-on-exec��־�����óɽ���ʽ����execve(2)��ϵͳ����                     
             F_GETFD     ȡ�����ļ�������fd����close-on-exec��־,����FD_CLOEXEC.�������ֵ��FD_CLOEXEC��������������0�Ļ�,�ļ����ֽ���ʽ����exec(),������������                      �������ͨ��exec���еĻ�,�ļ������ر�(arg������)                  
             F_SETFD     ����close-on-exec��ꡣ������Բ���arg��FD_CLOEXECλ������                   
             F_GETFL     ȡ��fd���ļ�״̬��־,��ͬ���������һ��(arg������)                    
             F_SETFL     ���ø�arg������״̬��־,���Ը��ĵļ�����־�ǣ�O_APPEND�� O_NONBLOCK��O_SYNC��O_ASYNC��
             F_GETOWN ȡ�õ�ǰ���ڽ���SIGIO����SIGURG�źŵĽ���id�������id,������id���سɸ�ֵ(arg������)                    
             F_SETOWN ���ý�����SIGIO��SIGURG�źŵĽ���id�������id,������idͨ���ṩ��ֵ��arg��˵��,����,arg������Ϊ�ǽ���id
              
������(cmd)F_GETFL��F_SETFL�ı�־�����������:            
             O_NONBLOCK        ������I/O;���read(2)����û�пɶ�ȡ������,�������write(2)����������,read��write���÷���-1��EAGAIN����               ��������       ����O_APPEND             ǿ��ÿ��д(write)������������ļ����ĩβ,�൱��open(2)��O_APPEND��־         
             O_DIRECT             ��С����ȥ��reading��writing�Ļ���Ӱ��.ϵͳ����ͼ���⻺����Ķ���д������.
                             ������ܹ����⻺��,��ô������С���Ѿ��������˵��� ����ɵ�Ӱ��.��������־�õĲ�����,�����Ľ�������                      
             O_ASYNC              ��I/O���õ�ʱ��,����SIGIO�źŷ��͵�������,����:�������ݿ��Զ���ʱ��
 ע�⣺      ���޸��ļ���������־���ļ�״̬��־ʱ�����������Ҫȡ�����ڵı�־ֵ��Ȼ����ϣ���޸�������������±�־ֵ������ֻ��ִ��F_SETFD��F_SETFL���������ر���ǰ���õı�־λ��
fcntl�ķ���ֵ��  �������йء������������������أ�1������ɹ��򷵻�ĳ������ֵ�����������������ض�����ֵ��F_DUPFD,F_GETFD,F_GETFL�Լ�F_GETOWN����һ�������µ��ļ����������ڶ���������Ӧ��־�����һ������һ�����Ľ���ID�򸺵Ľ�����ID��
 
һ����һ��������dup�����������ﲻ����������fcnlt(oldfd, F_DUPFD, 0) <==>dup2(oldfd, newfd)��
��������close-on-exec���
�ڴ˺����д����ӽ��̣�����execl
 1 #include <stdio.h>
 2 #include <stdlib.h>
 3 #include <string.h>
 4 
 5 int main()
 6 {
 7     pid_t pid;
 8     //��׷�ӵ���ʽ���ļ�
 9     int fd = fd = open("test.txt", O_TRUNC | O_RDWR | O_APPEND | O_CREAT, 0777);
10     if(fd < 0)
11     {
12         perror("open");
13         return -1;
14     }
15     printf("fd = %d\n", fd);
16     
17     fcntl(fd, F_SETFD, 0);//�ر�fd��close-on-exec��־
18 
19     write(fd, "hello c program\n", strlen("hello c program!\n"));
20 
21     pid = fork();
22     if(pid < 0)
23     {
24             perror("fork");
25             return -1;
26     }
27     if(pid == 0)
28     {
29         printf("fd = %d\n", fd);
30         
31         int ret = execl("./main", "./main", (char *)&fd, NULL);
32         if(ret < 0)
33         {
34             perror("execl");
35             exit(-1);
36         }
37         exit(0);
38     }
39 
40     wait(NULL);
41 
42     write(fd, "hello c++ program!\n", strlen("hello c++ program!\n"));
43 
44     close(fd);
45 
46     return 0;
47 }main���Ժ���
 1 int main(int argc, char *argv[])
 2 {
 3     int fd = (int)(*argv[1]);//������
 4     
 5     printf("fd = %d\n", fd);
 6 
 7     int ret = write(fd, "hello linux\n", strlen("hello linux\n"));
 8     if(ret < 0)
 9     {
10         perror("write");
11         return -1;
12     }
13 
14     close(fd);
15 
16     return 0;
17 }ִ�к��ļ������
[root@centOS5 class_2]# cat test.txt 
hello c program
hello linux
hello c++ program!
 
����������F_GETFL��F_SETFL�����ļ���־�����������������
 1 #include <stdio.h>
 2 #include <sys/types.h>
 3 #include <unistd.h>
 4 #include <sys/stat.h>
 5 #include <fcntl.h>
 6 #include <string.h>
 7 
 8 / **********************ʹ�ܷ�����I/O********************
 9 *int flags;
10 *if(flags = fcntl(fd, F_GETFL, 0) < 0)
11 *{
12 *    perror("fcntl");
13 *    return -1;
14 *}
15 *flags |= O_NONBLOCK;
16 *if(fcntl(fd, F_SETFL, flags) < 0)
17 *{
18 *    perror("fcntl");
19 *    return -1;
20 *}
21 ******************************************************* /
22 
23 / **********************�رշ�����I/O******************
24 flags &= ~O_NONBLOCK;
25 if(fcntl(fd, F_SETFL, flags) < 0)
26 {
27     perror("fcntl");
28     return -1;
29 }
30 ******************************************************* /
31 
32 int main()
33 {
34     char buf[10] = {0};
35     int ret;
36     int flags;
37     
38     //ʹ�÷�����io
39     if(flags = fcntl(STDIN_FILENO, F_GETFL, 0) < 0)
40     {
41         perror("fcntl");
42         return -1;
43     }
44     flags |= O_NONBLOCK;
45     if(fcntl(STDIN_FILENO, F_SETFL, flags) < 0)
46     {
47         perror("fcntl");
48         return -1;
49     }
50 
51     while(1)
52     {
53         sleep(2);
54         ret = read(STDIN_FILENO, buf, 9);
55         if(ret == 0)
56         {
57             perror("read--no");
58         }
59         else
60         {
61             printf("read = %d\n", ret);
62         }
63         
64         write(STDOUT_FILENO, buf, 10);
65         memset(buf, 0, 10);
66     }
67 
68     return 0;
69 }�ģ������첽IO��û����Ժ�ʵ�֣��ǺǺǡ�����������
�壺���û�ȡ��¼��
�ṹ��flock��ָ�룺
struct flcok
{
���� short int l_type;  ������״̬
��������//�������������ڷֶζ��ļ����������������ļ���������l_whence=SEEK_SET,l_start=0,l_len=0;
���� short int l_whence;����l_startλ��* /
���� off_t l_start; / * ��������Ŀ�ͷλ�� * /
���� off_t l_len; / *��������Ĵ�С* /
���� pid_t l_pid; / *���������Ľ���* /
};
l_type ������״̬:
���� F_RDLCK ����һ������ȡ�õ�����
���� F_WRLCK ����һ����д���õ�����
       F_UNLCK ɾ��֮ǰ����������
l_whence Ҳ�����ַ�ʽ:
����SEEK_SET ���ļ���ͷΪ��������ʼλ�á�
     SEEK_CUR ��Ŀǰ�ļ���дλ��Ϊ��������ʼλ��
     SEEK_END ���ļ���βΪ��������ʼλ�á�
 
 
 1 #include "filelock.h"
 2 
 3 / * ����һ�Ѷ���  * /
 4 int readLock(int fd, short start, short whence, short len) 
 5 {
 6     struct flock lock;
 7     lock.l_type = F_RDLCK;
 8     lock.l_start = start;
 9     lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
10     lock.l_len = len;
11     lock.l_pid = getpid();
12 //  ������ʽ����
13     if(fcntl(fd, F_SETLKW, &lock) == 0)
14         return 1;
15     
16     return 0;
17 }
18 
19 / * ����һ�Ѷ��� , ���ȴ� * /
20 int readLocknw(int fd, short start, short whence, short len) 
21 {
22     struct flock lock;
23     lock.l_type = F_RDLCK;
24     lock.l_start = start;
25     lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
26     lock.l_len = len;
27     lock.l_pid = getpid();
28 //  ��������ʽ����
29     if(fcntl(fd, F_SETLK, &lock) == 0)
30         return 1;
31     
32     return 0;
33 }
34 / * ����һ��д�� * /
35 int writeLock(int fd, short start, short whence, short len) 
36 {
37     struct flock lock;
38     lock.l_type = F_WRLCK;
39     lock.l_start = start;
40     lock.l_whence = whence;
41     lock.l_len = len;
42     lock.l_pid = getpid();
43 
44     //������ʽ����
45     if(fcntl(fd, F_SETLKW, &lock) == 0)
46         return 1;
47     
48     return 0;
49 }
50 
51 / * ����һ��д��  * /
52 int writeLocknw(int fd, short start, short whence, short len) 
53 {
54     struct flock lock;
55     lock.l_type = F_WRLCK;
56     lock.l_start = start;
57     lock.l_whence = whence;
58     lock.l_len = len;
59     lock.l_pid = getpid();
60 
61     //��������ʽ����
62     if(fcntl(fd, F_SETLK, &lock) == 0)
63         return 1;
64     
65     return 0;
66 }
67 
68 / * ���� * /
69 int unlock(int fd, short start, short whence, short len) 
70 {
71     struct flock lock;
72     lock.l_type = F_UNLCK;
73     lock.l_start = start;
74     lock.l_whence = whence;
75     lock.l_len = len;
76     lock.l_pid = getpid();
77 
78     if(fcntl(fd, F_SETLKW, &lock) == 0)
79         return 1;
80 
81     return 0;
82 }
*/


/*
����fd�������Ѿ��ɹ��㿪���ļ������ʵ���ϣ�nginx.conf�ļ��е�lock_file������ָ�����ļ�·�������������ļ��������ģ�
����ļ����򿪺�õ��ľ����������Ϊfd�������ݸ�fcntl�������ṩһ�������ơ�
struct flcok
{
���� short int l_type;  ������״̬ //�������������ڷֶζ��ļ����������������ļ���������l_whence=SEEK_SET,l_start=0,l_len=0;
���� short int l_whence;����l_startλ��* /  ��������ʼ��ַ�����λ��
���� off_t l_start; / * ��������Ŀ�ͷλ�� * /  ��������ʼ��ַƫ������ͬ1_whence��ͬȷ��������
���� off_t l_len; / *��������Ĵ�С* /  ���ĳ��ȣ�O��ʾ�����ļ�ĩ
���� pid_t l_pid; / *���������Ľ���* /  ӵ�����Ľ���ID
};
�����cmd������Nginx��ֻ��������ֵ��F��SETLK��F��SETLKW�����Ƕ���ʾ��ͼ��û���������ʹ��F��SETLKʱ����������Ѿ�����������ռ�ã�
fcntl��������ȴ����������ͷ������Լ��õ�����ŷ��أ������������ػ�ȡ������ʧ�ܣ�ʹ��F��SETLKWʱ��ͬ������ռ�ú�fcntl������һֱ
�ȴ�������������û���ͷ���ʱ����ǰ���̾ͻ�������fcntl�����У����������ᵼ�µ�ǰ�����ɿ�ִ��״̬תΪ˯��״̬��
 ��flock�ṹ���п��Կ������ļ����Ĺ��ܾ���������������ͨ�Ļ�����������������ס�ļ��еĲ������ݡ���Nginx��װ���ļ��������ڱ�����
��ε�˳��ִ�У����磬�ڽ��и��ؾ���ʱ��ʹ�û�������֤ͬһʱ�̽���һ��worker���̿��Դ����µ�TCP���ӣ���ʹ�÷�ʽҪ�򵥵öࣺһ��
lock_file�ļ���Ӧһ��ȫ�ֻ���������������master���̻���worker���̶���Ч����ˣ�����Lstart��l_len��l_pid������Ϊ0����1_whence����Ϊ
SEEK_SET��ֻ��Ҫ����ļ��ṩһ������l_type��ֵ��ȡ�����û�����ʵ������˯����������ʵ�ַ���������˯�ߵ�����
*/


//���ر�fd�����Ӧ���ļ�ʱ����ǰ���̽��Զ��ͷ��Ѿ��õ�������

/*
�����ļ�����Nginx��װ��3��������ngx_trylock_fdʵ���˲����������̡������ý��̽���˯��״̬�Ļ�������ngx_lock_fd�ṩ�Ļ���������
�Ѿ������������õ�ʱ���ᵼ�µ�ǰ���̽���˯��״̬��ֱ��˳���õ�������󣬵�ǰ���̲ŻᱻLinux�ں����µ��ȣ�������������������
ngx_unlock fd�����ͷŻ�������
*/
ngx_err_t
ngx_trylock_fd(ngx_fd_t fd)
{
    struct flock  fl;

    ngx_memzero(&fl, sizeof(struct flock)); //����ļ��������������ļ��е����ݣ����Ϊ0
    fl.l_type = F_WRLCK; //F_WRLCK��ζ�Ų��ᵼ�½���˯��
    fl.l_whence = SEEK_SET; //

    //��ȡfd��Ӧ�Ļ��������������
    /*
    ʹ��ngx_trylock_fd������ȡ�������ɹ�ʱ�᷵��0�����򷵻ص���ʵ��errno�����룬�����������ΪNGX- EAGAIN����NGX EACCESSʱ
    ��ʾ��ǰû���õ������������������Ϊfcntlִ�д���
     */
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return ngx_errno;
    }

    return 0;
}

/*
ngx_lock_fd���������������̵�ִ�У�ʹ��ʱ��Ҫ�ǳ������������ܻᵼ��worker��������˯��Ҳ������������������
*/
ngx_err_t
ngx_lock_fd(ngx_fd_t fd)
{
    struct flock  fl;

    ngx_memzero(&fl, sizeof(struct flock));

    //F_WRLCK�ᵼ�½���˯��
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    //�������-1�����ʾfcntlִ�д���һ������0����ʾ�ɹ����õ�����
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        return ngx_errno;
    }

    return 0;
}

/*
ngx_unlock_fd���������ͷŵ�ǰ�����Ѿ��õ��Ļ�����
*/ //���ر�fd�����Ӧ���ļ�ʱ����ǰ���̽��Զ��ͷ��Ѿ��õ�������
ngx_err_t
ngx_unlock_fd(ngx_fd_t fd)
{
    struct flock  fl;

    ngx_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_UNLCK;//F_UNLCK��ʾ��Ҫ�ͷ���
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return  ngx_errno;
    }

    return 0;
}


#if (NGX_HAVE_POSIX_FADVISE) && !(NGX_HAVE_F_READAHEAD)

ngx_int_t
ngx_read_ahead(ngx_fd_t fd, size_t n)
{
    int  err;

    err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

    if (err == 0) {
        return 0;
    }

    ngx_set_errno(err);
    return NGX_FILE_ERROR;
}

#endif


#if (NGX_HAVE_O_DIRECT)
/*
�ļ��첽IO
    �¼�����ģ�鶼���ڴ��������¼�����û���漰�������ļ��Ĳ�������
�ڽ�����Linux�ں�2.6.2x֮��汾��֧�ֵ��ļ��첽I/O���Լ�ngx_epoll_moduleģ����
������ļ��첽I/O����ṩ����ġ������ᵽ���ļ��첽I/O������glibc���ṩ���ļ���
��I/O��glibc���ṩ���첽I/O�ǻ��ڶ��߳�ʵ�ֵģ����������������ϵ��첽I/O��������
˵�����첽I/O����Linux�ں�ʵ�֣�ֻ�����ں��гɹ�������˴��̲������ں˲Ż�֪ͨ
���̣�����ʹ�ô����ļ��Ĵ����������¼��Ĵ���ͬ����Ч��
    ʹ�����ַ�ʽ��ǰ����Linux�ں˰汾�б���֧���ļ��첽I/O����Ȼ���������ĺô�
Ҳ�ǳ����ԣ�Nginx�Ѷ�ȡ�ļ��Ĳ����첽���ύ���ں˺��ں˻�֪ͨI/O�豸������ִ
�в�����������Nginx���̿��Լ�����ֵ�ռ��CPU�����ң����������¼��ѻ���I/O�豸
�Ķ�����ʱ�����ᷢ�ӳ��ں��С������㷨�������ƣ��Ӷ����������ȡ���������ĳɱ���
    ע��Linux�ں˼�����ļ��첽I/O�ǲ�֧�ֻ�������ģ�Ҳ����˵����ʹ��Ҫ����
���ļ�����Linux�ļ������д��ڣ�Ҳ����ͨ����ȡ�����Ļ����е��ļ���������ʵ�ʶԴ�
�̵Ĳ�������Ȼ������worker���̵ĽǶ�����˵���˺ܴ��ת�����ǶԵ���������˵������
�п��ܽ���ʵ�ʴ�����ٶȣ���Ϊԭ�ȿ��Դ��ڴ��п��ٻ�ȡ���ļ�����ʹ�����첽I/O��
��һ����Ӵ����϶�ȡ���첽�ļ�I/O�ǰѡ�˫�н������ؼ�Ҫ��ʹ�ó���������󲿷��û�
������ļ��Ĳ��������䵽�ļ������У���ô��Ҫʹ���첽I/O����֮���������ʹ���ļ�
�첽I/O����һ���Ƿ��Ϊ����������������ϵ�������
    Ŀǰ��Nginx��֧���ڶ�ȡ�ļ�ʱʹ���첽I/O����Ϊ����д���ļ�ʱ������д���ڴ�
�о����̷��أ�Ч�ʺܸߣ���ʹ���첽I/Oд��ʱ�ٶȻ������½���
�ļ��첽AIO�ŵ�:
        �첽I/O����Linux�ں�ʵ�֣�ֻ�����ں��гɹ�������˴��̲������ں˲Ż�֪ͨ
    ���̣�����ʹ�ô����ļ��Ĵ����������¼��Ĵ���ͬ����Ч�������Ͳ�������worker���̡�
ȱ��:
        ��֧�ֻ�������ģ�Ҳ����˵����ʹ��Ҫ�������ļ�����Linux�ļ������д��ڣ�Ҳ����ͨ����ȡ��
    ���Ļ����е��ļ���������ʵ�ʶԴ��̵Ĳ������п��ܽ���ʵ�ʴ�����ٶȣ���Ϊԭ�ȿ��Դ��ڴ��п���
    ��ȡ���ļ�����ʹ�����첽I/O����һ����Ӵ����϶�ȡ
������ѡ���첽I/O������ͨI/O������?
        �첽�ļ�I/O�ǰѡ�˫�н������ؼ�Ҫ��ʹ�ó���������󲿷��û�
    ������ļ��Ĳ��������䵽�ļ������У���ô��Ҫʹ���첽I/O����֮���������ʹ���ļ�
    �첽I/O����һ���Ƿ��Ϊ����������������ϵ�������
    Ŀǰ��Nginx��֧���ڶ�ȡ�ļ�ʱʹ���첽I/O����Ϊ����д���ļ�ʱ������д���ڴ�
�о����̷��أ�Ч�ʺܸߣ���ʹ���첽I/Oд��ʱ�ٶȻ������½����첽I/O��֧��д��������Ϊ
�첽I/O�޷����û��棬��д����ͨ�����䵽�����ϣ�linux���Զ����ļ��л����е�����д������
��ͨ�ļ���д����:
������ϵͳ����read/write���������������أ�
- ��ȡ���ں˻�������Ҫ���ļ�����:�ں˻�����->�û�������;û��:Ӳ��->�ں˻�����->�û�������;
- д�أ����ݻ���û���ַ�ռ俽��������ϵͳ�ں˵�ַ�ռ��ҳ������ȥ������write�ͻ�ֱ�ӷ��أ�����ϵͳ����ǡ����ʱ��д����̣�����Ǵ�˵�е�
*/

//direct AIO���Բο�http://blog.csdn.net/bengda/article/details/21871413
ngx_int_t
ngx_directio_on(ngx_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return NGX_FILE_ERROR;
    }

    /* 
    ��ͨ����I/O�ŵ�: ���� I/O ʹ���˲���ϵͳ�ں˻���������һ���̶��Ϸ�����Ӧ�ó���ռ��ʵ�ʵ������豸������ I/O ���Լ��ٶ��̵Ĵ������Ӷ�������ܡ�
    ȱ�㣺�ڻ��� I/O �����У�DMA ��ʽ���Խ�����ֱ�ӴӴ��̶���ҳ�����У����߽����ݴ�ҳ����ֱ��д�ص������ϣ�������ֱ����Ӧ�ó����ַ�ռ�ʹ���֮��
        �������ݴ��䣬�����Ļ��������ڴ����������Ҫ��Ӧ�ó����ַ�ռ��ҳ����֮����ж�����ݿ�����������Щ���ݿ��������������� CPU �Լ��ڴ濪���Ƿǳ���ġ�
    direct I/O�ŵ�:ֱ�� I/O ����Ҫ���ŵ����ͨ�����ٲ���ϵͳ�ں˻�������Ӧ�ó����ַ�ռ�����ݿ��������������˶��ļ���ȡ��д��ʱ�������� CPU 
        ��ʹ���Լ��ڴ�����ռ�á������ĳЩ�����Ӧ�ó��򣬱����Ի���Ӧ�ó�����˵����ʧΪһ�ֺõ�ѡ�����Ҫ������������ܴ�ʹ��ֱ�� I/O 
        �ķ�ʽ�������ݴ��䣬������Ҫ����ϵͳ�ں˵�ַ�ռ俽�����ݲ����Ĳ��룬�⽫����������ܡ�
    direct I/Oȱ��: ����ֱ�� I/O �Ŀ����ǳ��󣬶�ֱ�� I/O �ֲ����ṩ���� I/O �����ơ����� I/O �Ķ��������ԴӸ��ٻ���洢���л�ȡ���ݣ���ֱ�� 
        I/O �Ķ����ݲ�������ɴ��̵�ͬ�����������������ϵĲ��� , ���ҵ��½�����Ҫ�ϳ���ʱ�����ִ����
    �ܽ�:
    Linux �е�ֱ�� I/O �����ļ���ʽ���Լ��� CPU ��ʹ�����Լ��ڴ�����ռ�ã�����ֱ�� I/O ��ʱ��Ҳ������ܲ�������Ӱ�졣������ʹ��
    ֱ�� I/O ֮ǰһ��Ҫ��Ӧ�ó�����һ�������ѵ���ʶ��ֻ����ȷ�������û��� I/O �Ŀ����ǳ��޴������£��ſ���ʹ��ֱ�� I/O��ֱ�� I/O 
    ������Ҫ���첽 I/O �������ʹ��
    
    ��ͨ����I/O: Ӳ��->�ں˻�����->�û������� д����д���������оͷ��أ�һ�����ں˶���д������(����ֱ�ӵ���APIָ��Ҫд�����)��
    ���������ȼ�黺�����Ƿ���������ļ����ݣ�û�оͳ���̶����ں˻��������ڴ��ں˻��������û�������
    O_DIRECTΪֱ��I/O��ʽ��Ӳ��->�û��������������ں˻���������������ֱ�Ӵ��̲����ܷ�ʱ,����ֱ��I/Oһ�����AIO��EPOLLʵ��
    �ο�:http://blog.csdn.net/bengda/article/details/21871413  http://www.ibm.com/developerworks/cn/linux/l-cn-directio/index.html
    */
    return fcntl(fd, F_SETFL, flags | O_DIRECT);
}


ngx_int_t
ngx_directio_off(ngx_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return NGX_FILE_ERROR;
    }

    return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
}

#endif


#if (NGX_HAVE_STATFS)

/*
����������   
��ѯ�ļ�ϵͳ��ص���Ϣ�� 
    
�÷���   
#include <sys/vfs.h>    / * ���� <sys/statfs.h> * / 
int statfs(const char *path, struct statfs *buf); 
int fstatfs(int fd, struct statfs *buf); 
  
  ������   
path: λ����Ҫ��ѯ��Ϣ���ļ�ϵͳ���ļ�·������     
fd�� λ����Ҫ��ѯ��Ϣ���ļ�ϵͳ���ļ������ʡ� 
buf�����½ṹ���ָ����������ڴ����ļ�ϵͳ��ص���Ϣ 
struct statfs { 
    long    f_type;     / * �ļ�ϵͳ����  * / 
   long    f_bsize;    / * �����Ż��Ĵ�����С  * / 
   long    f_blocks;   / * �ļ�ϵͳ���ݿ����� * / 
   long    f_bfree;    / * ���ÿ��� * / 
     long    f_bavail;   / * �ǳ����û��ɻ�ȡ�Ŀ��� * / 
   long    f_files;    / * �ļ�������� * / 
   long    f_ffree;    / * �����ļ������ * / 
   fsid_t  f_fsid;     / * �ļ�ϵͳ��ʶ * / 
   long    f_namelen;  / * �ļ�������󳤶� * / 
}; 
 
����˵����   
�ɹ�ִ��ʱ������0��ʧ�ܷ���-1��errno����Ϊ���µ�ĳ��ֵ   
  
EACCES�� (statfs())�ļ���·�����а�����Ŀ¼���ɷ��� 
EBADF �� (fstatfs()) �ļ���������Ч 
EFAULT�� �ڴ��ַ��Ч 
EINTR �� �������ź��ж� 
EIO    �� ��д���� 
ELOOP �� (statfs())����·���������д���̫��ķ������� 
ENAMETOOLONG��(statfs()) ·����̫�� 
ENOENT��(statfs()) �ļ������� 
ENOMEM�� �����ڴ治�� 
ENOSYS�� �ļ�ϵͳ��֧�ֵ��� 
ENOTDIR��(statfs())·�����е���Ŀ¼���������Ŀ¼ 
EOVERFLOW����Ϣ���
 
һ���򵥵����ӣ�
#include <sys/vfs.h>
#include <stdio.h>
int main()
{
    struct statfs diskInfo;
    statfs("/",&diskInfo);
    unsigned long long blocksize = diskInfo.f_bsize;// ÿ��block����������ֽ���
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;//�ܵ��ֽ���
    printf("TOTAL_SIZE == %lu MB/n",totalsize>>20); // 1024*1024 =1MB  �����MB��λ
    unsigned long long freeDisk = diskInfo.f_bfree*blocksize; //�ټ�����ʣ��Ŀռ��С
    printf("DISK_FREE == %ld MB/n",freeDisk>>20);
 return 0;
}
*/

//��ȡ�ļ�ϵͳ��block size  
size_t
ngx_fs_bsize(u_char *name)
{
    struct statfs  fs;

    if (statfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_bsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_bsize; // ÿ��block����������ֽ���
}

#elif (NGX_HAVE_STATVFS)

size_t
ngx_fs_bsize(u_char *name)
{
    struct statvfs  fs;

    if (statvfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_frsize;
}

#else

size_t
ngx_fs_bsize(u_char *name)
{
    return 512;
}

#endif

