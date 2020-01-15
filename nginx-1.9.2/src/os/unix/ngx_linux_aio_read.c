
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

/*
 Linux�ں��ṩ��5��ϵͳ��������ļ��������첽I/O���ܣ�����9-7��
��9-7  Linux�ں��ṩ���ļ��첽1/0��������
�������������������������������������������ש������������������������������������ש�������������������������������
��    ������                              ��    ��������                        ��    ִ������                  ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��  nr events��ʾ��Ҫ��ʼ�����첽     ��  ��ʼ���ļ��첽I/O�������ģ� ��
��int io_setup(unsigned nr_events,        ��1/0�����Ŀ��Դ�����¼�����С��     ��ִ�гɹ���ctxp���Ƿ��������  ��
��aio context_t *ctxp)                    ������ctxp���ļ��첽1/0������������   ����������������첽1/0������   ��
��                                        ��                                    �������ٿ��Դ���nr- events����  ��
��                                        ����ָ��                              ��                              ��
��                                        ��                                    ����������0��ʾ�ɹ�             ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                                        ��                                    ��  �����ļ��첽1/0�������ġ�   ��
��int io_destroy (aio_context_t ctx)      ��  ctx���ļ��첽1/0��������������    ��                              ��
��                                        ��                                    ������0��ʾ�ɹ�                 ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_submit(aiio_context_t ctx,       ��  ctx���ļ��첽1/0����������������  ��  �ύ�ļ��첽1/0����������   ��
��                                        ��nr��һ���ύ���¼�������cbp����Ҫ   ��                              ��
��long nr, struct iocb *cbp[])            ��                                    ��ֵ��ʾ�ɹ��ύ���¼�����      ��
��                                        ���ύ���¼������е�����Ԫ�ص�ַ      ��                              ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��int io_cancel(aio_context_t ctx, struct ��  ctx���ļ��첽1/0������������      ��  ȡ��֮ǰʹ��io��sumbit�ύ  ��
��                                        ������iocb��Ҫȡ�����첽1/0��������   ����һ���ļ��첽I/O����������   ��
��iocb *iocb, struct io_event *result)    ��                                    ��                              ��
��                                        ��result��ʾ���������ִ�н��        ��O��ʾ�ɹ�                     ��
�ǩ����������������������������������������贈�����������������������������������贈������������������������������
��                I                       ��  ctx���ļ��첽1/0������������      ��                              ��
��int io_getevents(aio_context_t ctx,     ������min_nr��ʾ����Ҫ��ȡmln_ nr��   ��                              ��
��long min_nr, lon, g nr,                 ���¼�����nr��ʾ�����ȡnr���¼���    ��  ���Ѿ���ɵ��ļ��첽I/O��   ��
��struct io  event "*events, struct       ������events����ĸ���һ������ͬ�ģ�  ���������ж�ȡ����              ��
��timespec *timeout)                      ��events��ִ����ɵ��¼����飻tlmeout ��                              ��
��         I "                            ���ǳ�ʱʱ�䣬Ҳ�����ڻ�ȡmm- nr��    ��                              ��
��                                        ���¼�ǰ�ĵȴ�ʱ��                    ��                              ��
�������������������������������������������ߩ������������������������������������ߩ�������������������������������
    ��9-7���оٵ���5�ַ����ṩ���ں˼�����ļ��첽I/O���ƣ�ʹ��ǰ��Ҫ�ȵ���io_setup������ʼ���첽I/O�����ġ���Ȼһ�����̿���ӵ��
����첽I/O�����ģ���ͨ����һ�����㹻�ˡ�����io_setup�������������첽I/O�����ĵ���������aio��context_t��
�ͣ��������������epoll_create���ص�������һ�����ǹᴩʼ�յġ�ע�⣬nr_ eventsֻ��
ָ�����첽I/O���ٳ�ʼ��������������������û�����������Դ�����첽I/O�¼���Ŀ��
Ϊ�˱�����⣬������io_setup��epoll_create���жԱȣ����ǻ��Ǻ����Ƶġ�
    ��Ȼ��epoll���첽I/O���жԱȣ���ô��Щ�����൱��epoll_ctrl�أ�����io��submit
��io cancel������io- submit�൱�����첽I/O������¼�����io- cancel���൱�ڴ��첽I/O
���Ƴ��¼���io��submit���õ���һ���ṹ��iocb������򵥵ؿ�һ�����Ķ��塣
    struct iocb  f
    ��t�洢��ҵ����Ҫ��ָ�롣���磬��Nginx�У�����ֶ�ͨ���洢�Ŷ�Ӧ��ngx_event_tͤ����ָ
�롣��ʵ������io_getevents�����з��ص�io event�ṹ���data��Ա����ȫһ�µ�+��
    u int64 t aio data;
7 7����Ҫ����
u  int32��PADDED (aio_key,  aio_raservedl)j
���������룬��ȡֵ��Χ��io iocb cmd t�е�ö������
u int16 t aio lio_opcode;
������������ȼ�
int16 t aio_reqprio,
����i41-������
u int32 t aio fildes;
��������д������Ӧ���û�̬������
u int64 t aio buf;
��������д�������ֽڳ���
u int64 t aio_nbytes;
��������д������Ӧ���ļ��е�ƫ����
int64 t aio offset;
7 7�����ֶ�
u int64��aio reserved2;
    ��+��ʾ��������ΪIOCB FLAG RESFD����������ں˵����첽I/O���������ʱʹ��eventfd��
��֪ͨ������epoll���ʹ��+��
    u int32��aio_flags��
������ʾ��ʹ��IOCB FLAG RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
U int32 t aio resfd;
    ��ˣ������ú�iocb�ṹ��󣬾Ϳ������첽I/O�ύ�¼��ˡ�aio_lio_opcode������
ָ��������¼��Ĳ������ͣ�����ȡֵ��Χ���¡�
    typedef enum io_iocb_cmd{
    �����첽������
    IOL CMD��PREAD=O��
    �����첽д����
    IO��CMD��PWRITE=1��
    ����ǿ��ͬ��
    IO_ CMD��FSYNC=2��
    ����Ŀǰ��ʹ��
    IO��CMD- FDSYNC=3��
    ����Ŀǰδʹ��
    IO��CMD��POLL=5��
    ���������κ�����
    IOһCMD- NOOP=6��
    )  io_iocIb_cmd_t j
    ��Nginx�У���ʹ����IO��CMD_ PREAD���������ΪĿǰ��֧���ļ����첽I/O��
ȡ����֧���첽I/O��д�롣������һ����Ҫ��ԭ�����ļ����첽I/O�޷����û��棬��д
�ļ�����ͨ�����䵽�����еģ�Linux����ͳһ�������С��ࡱ����ˢ�µ����̵Ļ��ơ�
    ������ʹ��io submit���ں��ύ���ļ��첽I/O�������¼�����ʹ��io_ cancel���
�Խ��Ѿ��ύ���¼�ȡ����
    ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�
epoll_wait�����������õ���ioһevent�ṹ�壬���濴һ�����Ķ��塣
struct io event  {
    �������ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_ data��һ�µ�
    uint64 t  data;
    ����ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    uint64_t   obj��
    �����첽I/O����Ľṹ��res���ڻ����0ʱ��ʾ�ɹ���С��0ʱ��ʾʧ��
    int64һt    res��
    ��7��������
    int64һ��    res2 j
)��
    ���������ݻ�ȡ��io��event�ṹ�����飬�Ϳ��Ի���Ѿ���ɵ��첽I/O�����ˣ��ر�
��iocb�ṹ���е�aio data��Ա��io��event�е�data�������ڴ���ָ�룬Ҳ����˵��ҵ����
�����ݽṹ���¼���ɺ�Ļص������������
    �����˳�ʱ��Ҫ����io_destroy���������첽I/O�����ģ����൱�ڵ���close�ر�
epoll����������
    Linux�ں��ṩ���ļ��첽I/O�����÷��ǳ��򵥣���������������ں���CPU��I/O
�豸�Ǹ��Զ�����������һ���ԣ����ύ���첽I/O�����󣬽�����ȫ����������������ֱ
�����������鿴�첽I/O�����Ƿ���ɡ�

*/

extern int            ngx_eventfd;
extern aio_context_t  ngx_aio_ctx;


static void ngx_file_aio_event_handler(ngx_event_t *ev);


static int
io_submit(aio_context_t ctx, long n, struct iocb **paiocb) //�൱��epoll�е�epoll_ctrl ����io_submit�൱�����첽I/O������¼�
{
    return syscall(SYS_io_submit, ctx, n, paiocb);
}

//fileΪҪ��ȡ��file�ļ���Ϣ
ngx_int_t
ngx_file_aio_init(ngx_file_t *file, ngx_pool_t *pool)
{
    ngx_event_aio_t  *aio;

    aio = ngx_pcalloc(pool, sizeof(ngx_event_aio_t));
    if (aio == NULL) {
        return NGX_ERROR;
    }

    aio->file = file;
    aio->fd = file->fd;
    aio->event.data = aio;
    aio->event.ready = 1;
    aio->event.log = file->log;

    file->aio = aio;

    return NGX_OK;
}

/*
��io_cancel���൱�ڴ��첽I/O���Ƴ��¼���io_submit���õ���һ���ṹ��iocb������򵥵ؿ�һ�����Ķ��塣
    struct iocb  f
    ��t�洢��ҵ����Ҫ��ָ�롣���磬��Nginx�У�����ֶ�ͨ���洢�Ŷ�Ӧ��ngx_event_tͤ����ָ
�롣��ʵ������io_getevents�����з��ص�io event�ṹ���data��Ա����ȫһ�µ�+��
    u int64 t aio data;
7 7����Ҫ����
u  int32��PADDED (aio_key,  aio_raservedl)j
���������룬��ȡֵ��Χ��io_iocb_cmd_t�е�ö������
u int16 t aio lio_opcode;
������������ȼ�
int16 t aio_reqprio,
����i41-������
u int32 t aio fildes;
��������д������Ӧ���û�̬������
u int64 t aio buf;
��������д�������ֽڳ���
u int64 t aio_nbytes;
��������д������Ӧ���ļ��е�ƫ����
int64 t aio offset;
7 7�����ֶ�
u int64��aio reserved2;
    ��+��ʾ��������ΪIOCB FLAG RESFD����������ں˵����첽I/O���������ʱʹ��eventfd��
��֪ͨ������epoll���ʹ��+��
    u int32��aio_flags��
������ʾ��ʹ��IOCB FLAG RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
U int32 t aio resfd;
    ��ˣ������ú�iocb�ṹ��󣬾Ϳ������첽I/O�ύ�¼��ˡ�aio_lio_opcode������
ָ��������¼��Ĳ������ͣ�����ȡֵ��Χ���¡�
    typedef enum io_iocb_cmd_t{
    �����첽������
    IO_CMD_PREAD=O��
    �����첽д����
    IO_CMD_PWRITE=1��
    ����ǿ��ͬ��
    IO_CMD_FSYNC=2��
    ����Ŀǰ��ʹ��
    IO_CMD_FDSYNC=3��
    ����Ŀǰδʹ��
    IO_CMD_POLL=5��
    ���������κ�����
    IO_CMD_NOOP=6��
    )  io_iocb_cmd_t
    ��Nginx�У���ʹ����IO_CMD_PREAD���������ΪĿǰ��֧���ļ����첽I/O��ȡ����֧���첽I/O��д�롣������һ����Ҫ��ԭ�����ļ���
�첽I/O�޷����û��棬��д�ļ�����ͨ�����䵽�����еģ�Linux����ͳһ�������С��ࡱ����ˢ�µ����̵Ļ��ơ�
    ������ʹ��io submit���ں��ύ���ļ��첽I/O�������¼�����ʹ��io_cancel����Խ��Ѿ��ύ���¼�ȡ����
    ��λ�ȡ�Ѿ���ɵ��첽I/O�¼��أ�io_getevents�����������������൱��epoll�е�epoll_wait�����������õ���io_event�ṹ�壬���濴һ�����Ķ��塣
struct io event  {
    �������ύ�¼�ʱ��Ӧ��iocb�ṹ���е�aio_data��һ�µ�
    uint64 t  data;
    ����ָ���ύ�¼�ʱ��Ӧ��iocb�ṹ��
    uint64_t   obj��
    �����첽I/O����Ľṹ��res���ڻ����0ʱ��ʾ�ɹ���С��0ʱ��ʾʧ��
    int64һt    res��
    ��7��������
    int64һ��    res2 j
)��

*/

/*
�������첽I/O���������ύ�첽I/O�����أ�ngx_linux_aio read.c�ļ��е�ngx_file_aio read�������ڴ��ļ��첽I/O������������Ḻ������ļ��Ķ�ȡ
*/
/*
ngx_epoll_aio_init��ʼ��aio�¼��б� ngx_file_aio_read��Ӷ��ļ��¼�������ȡ��Ϻ�epoll�ᴥ��
ngx_epoll_eventfd_handler->ngx_file_aio_event_handler 
nginx file aioֻ�ṩ��read�ӿڣ����ṩwrite�ӿڣ���Ϊ�첽aioֻ�Ӵ��̶���д������aio��ʽһ��д���䵽
���̻��棬���Բ��ṩ�ýӿڣ�����첽ioд���ܻ����
*/

//��ngx_epoll_eventfd_handler��Ӧ,��ִ��ngx_file_aio_read���첽I/O����Ӷ��¼���Ȼ��ͨ��epoll�������ض�ȡ���ݳɹ�����ִ��ngx_epoll_eventfd_handler
ssize_t
ngx_file_aio_read(ngx_file_t *file, u_char *buf, size_t size, off_t offset,
    ngx_pool_t *pool)
{//ע��aio�ں˶�ȡ��Ϻ��Ƿ���ngx_output_chain_ctx_t->buf�еģ���ngx_output_chain_copy_buf->ngx_file_aio_read
    ngx_err_t         err;
    struct iocb      *piocb[1];
    ngx_event_t      *ev; //������ļ��첽i/o�е�ngx_event_aio_t����������ngx_event_aio_t->ngx_event_t(ֻ�ж�),����������¼��е�event,��Ϊngx_connection_s�е�event(��������д)
    ngx_event_aio_t  *aio;

    if (!ngx_file_aio) { //��֧���ļ��첽I/O
        return ngx_read_file(file, buf, size, offset);
    }

    if (file->aio == NULL && ngx_file_aio_init(file, pool) != NGX_OK) {
        return NGX_ERROR;
    }

    aio = file->aio;
    ev = &aio->event;

    if (!ev->ready) { //ngx_epoll_eventfd_handler����1
        ngx_log_error(NGX_LOG_ALERT, file->log, 0,
                      "second aio post for \"%V\"", &file->name);
        return NGX_AGAIN;
    }

    ngx_log_debug4(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "aio complete:%d @%O:%z %V",
                   ev->complete, offset, size, &file->name);

    //����ͻ��˷��ͺ�˰����ʱ�򣬻�����ִ�иú�����һ����ngx_event_pipe_write_to_downstream-> p->output_filter(),��һ����aio���¼�
    //֪ͨ�ں˶�ȡ��ϣ���ngx_file_aio_event_handler�ߵ������һ�ε������ʱ��completeΪ0���ڶ��ε�ʱ��Ϊ1
    if (ev->complete) { //ngx_epoll_eventfd_handler����1
        ev->active = 0;
        ev->complete = 0;

        if (aio->res >= 0) {
            ngx_set_errno(0);
            return aio->res;
        }

        ngx_set_errno(-aio->res);

        ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                      "aio read \"%s\" failed", file->name.data);

        return NGX_ERROR;
    }

    ngx_memzero(&aio->aiocb, sizeof(struct iocb));
    /*
    ע�⣬aio_data�Ѿ�����Ϊ���ngx_event_t�¼���ָ�룬��������io_getevents������ȡ��io_event�����е�dataҲ�����ָ��
     */
    aio->aiocb.aio_data = (uint64_t) (uintptr_t) ev; //��ngx_epoll_eventfd_handler�л��ȡ��ev
    aio->aiocb.aio_lio_opcode = IOCB_CMD_PREAD;  //ֻ���ļ��첽I/O���ж�����  ĿǰNGINX��֧���첽��ȡ����֧���첽AIOд��
    aio->aiocb.aio_fildes = file->fd; //�ļ����
    aio->aiocb.aio_buf = (uint64_t) (uintptr_t) buf; //��/д������Ӧ���û�̬������
    aio->aiocb.aio_nbytes = size; //��/д�������ֽڳ���
    aio->aiocb.aio_offset = offset; //�� д������Ӧ���ļ��е�ƫ����
//��ʾ��������ΪIOCB_FLAG_RESFD����������ں˵����첽I/O���������ʱʹ��eventfd����֪ͨ������epoll���ʹ�ã�����Nginx�е�ʹ�÷����ɲμ�9.9.2��
    aio->aiocb.aio_flags = IOCB_FLAG_RESFD;
//��ʾ��ʹ��IOCB_FLAG_RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
    aio->aiocb.aio_resfd = ngx_eventfd;
/*
�첽�ļ�i/o�����¼��Ļص�����Ϊngx_file_aio_event_handler�����ĵ��ù�ϵ����������epoll_wait�е���ngx_epoll_eventfd_handler��������ǰ�¼�
���뵽ngx_posted_events�����У����Ӻ�ִ�еĶ����е���ngx_file_aio_event_handler����
*/
    ev->handler = ngx_file_aio_event_handler; //��ngx_epoll_eventfd_handler�л��ȡ��ev��Ȼ���ִ�и�handler

    piocb[0] = &aio->aiocb;
    //����io_submit��ngx_aio_ctx�첽1/0�����������1���¼�������1��ʾ�ɹ�
    if (io_submit(ngx_aio_ctx, 1, piocb) == 1) {
        ev->active = 1;
        ev->ready = 0;
        ev->complete = 0;

        return NGX_AGAIN;
    }

    err = ngx_errno;

    if (err == NGX_EAGAIN) {
        return ngx_read_file(file, buf, size, offset);
    }

    ngx_log_error(NGX_LOG_CRIT, file->log, err,
                  "io_submit(\"%V\") failed", &file->name);

    if (err == NGX_ENOSYS) {
        ngx_file_aio = 0;
        return ngx_read_file(file, buf, size, offset);
    }

    return NGX_ERROR;
}

/*
�첽�ļ�i/o�����¼��Ļص�����Ϊngx_file_aio_event_handler�����ĵ��ù�ϵ����������epoll_wait�е���ngx_epoll_eventfd_handler��������ǰ�¼�
���뵽ngx_posted_events�����У����Ӻ�ִ�еĶ����е���ngx_file_aio_event_handler����
*/
static void
ngx_file_aio_event_handler(ngx_event_t *ev)
{
    ngx_event_aio_t  *aio;

    aio = ev->data;

    ngx_log_debug2(NGX_LOG_DEBUG_CORE, ev->log, 0,
                   "aio event handler fd:%d %V", aio->fd, &aio->file->name);

    /* ���������ngx_event_aio_t�ṹ���handler�ص�����������ص���������������ҵ��
ģ��ʵ�ֵģ�Ҳ����˵����һ��ҵ��ģ����ʹ���ļ��첽I/O���Ϳ���ʵ��handler�����
�������ļ��첽������ɺ󣬸÷����ͻᱻ�ص��� */
    aio->handler(ev);//����ngx_output_chain_copy_buf->ngx_http_copy_aio_handler
}

