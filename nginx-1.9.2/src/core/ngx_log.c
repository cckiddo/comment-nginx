
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static char *ngx_error_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_log_set_levels(ngx_conf_t *cf, ngx_log_t *log);
static void ngx_log_insert(ngx_log_t *log, ngx_log_t *new_log);


#if (NGX_DEBUG)

static void ngx_log_memory_writer(ngx_log_t *log, ngx_uint_t level,
    u_char *buf, size_t len);
static void ngx_log_memory_cleanup(void *data);


typedef struct {
    u_char        *start;
    u_char        *end;
    u_char        *pos;
    ngx_atomic_t   written;
} ngx_log_memory_buf_t;

#endif

/*
Nginx����־ģ�飨������˵����־ģ����ngx_errlog_moduleģ�飬��ngx_http_log_moduleģ�������ڼ�¼HTTP����ķ�����־�ģ�
���߹��ܲ�ͬ����ʵ����Ҳû���κι�ϵ��Ϊ����ģ���ṩ�˻����ļ�¼��־����
*/
//error_log path level path·�� level��ӡ����ֻ�б�level����ߵĲŴ�ӡ  ֵԽС���ȼ�Խ��
static ngx_command_t  ngx_errlog_commands[] = { //    error_log file [ debug | info | notice | warn | error | crit ] 
    {ngx_string("error_log"), //ngx_errlog_module�е�error_log����ֻ��ȫ�����ã�ngx_http_core_module��http{} server{} local{}������
     NGX_MAIN_CONF|NGX_CONF_1MORE,
     ngx_error_log,
     0,
     0,
     NULL},

    ngx_null_command
};


static ngx_core_module_t  ngx_errlog_module_ctx = {
    ngx_string("errlog"),
    NULL,
    NULL
};

/*
Nginx����־ģ�飨������˵����־ģ����ngx_errlog_moduleģ�飬��ngx_http_log_moduleģ�������ڼ�¼HTTP����ķ�����־�ģ�
���߹��ܲ�ͬ����ʵ����Ҳû���κι�ϵ��Ϊ����ģ���ṩ�˻����ļ�¼��־����
*/
ngx_module_t  ngx_errlog_module = {
    NGX_MODULE_V1,
    &ngx_errlog_module_ctx,                /* module context */
    ngx_errlog_commands,                   /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_log_t        ngx_log;//ָ�����ngx_log_file����ngx_log_init
static ngx_open_file_t  ngx_log_file;//NGX_ERROR_LOG_PATH�ļ� ngx_log_init
ngx_uint_t              ngx_use_stderr = 1;


static ngx_str_t err_levels[] = { //��Ӧ��־����NGX_LOG_STDERR--NGX_LOG_DEBUG���ο�ngx_log_set_levels
    ngx_null_string,
    ngx_string("emerg"),
    ngx_string("alert"),
    ngx_string("crit"),
    ngx_string("error"),
    ngx_string("warn"),
    ngx_string("notice"),
    ngx_string("info"),
    ngx_string("debug")
};

//debug_levels���������־����     err_levels���������־����  
static const char *debug_levels[] = { //��ӦλͼNGX_LOG_DEBUG_CORE---NGX_LOG_DEBUG_LAST  �ο�ngx_log_set_levels
    "debug_core", "debug_alloc", "debug_mutex", "debug_event",
    "debug_http", "debug_mail", "debug_mysql", "debug_stream"
};

void ngx_str_t_2buf(char *buf, ngx_str_t *str)
{
    if(buf == NULL || str == NULL)
        return;
    
    if(str->data != NULL && str->len != 0) {
        strncpy(buf, (char*)str->data, ngx_min(str->len, NGX_STR2BUF_LEN - 1));
        buf[str->len] = '\0';
    }
}

/*
ngx_log_error���ngx_log_debug�궼��������level��log��err��fmt������ֱ������
4�����������塣
    (1) level����
    ����ngx_log_error����˵��level��ʾ��ǰ������־�ļ�������ȡֵ��Χ����4-6��
	

�������������������ש������ש���������������������������������������������������������������������
��    ��������    ��  ֵ  ��    ����                                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��    ��߼�����־����־�����ݲ�����д��log����ָ�����ļ������ǻ�ֱ�� ��
��NGX_LOG_STDERR  ��    O ��                                                                    ��
��                ��      ������־�������׼�����豸�������̨��Ļ                              ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��  ����NGX��LOG ALERT���𣬶�С�ڻ����NGX LOG EMERG�����            ��
��NGX_LOG:EMERG   ��  1   ��                                                                    ��
��                ��      ����־���������log����ָ�����ļ���                                   ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG ALERT   ��    2 ��    ���NGX LOG CRIT����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG CRIT  ��    3 ��    ���NGX LOG ERR����                                             ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG ERR   ��    4 ��    ���NGX��LOG WARN����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG WARN  ��    5 ��    ����NGX LOG NOTICE����                                          ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG NOTICE  ��  6   ��  ����NGX__ LOG INFO����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG INFO  ��    7 ��    ����NGX��LOG DEBUG����                                           ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG DEBUG   ��    8 ��    ���Լ�����ͼ�����־                                          ��
�������������������ߩ������ߩ���������������������������������������������������������������������
    ʹ��ngx_log_error���¼��־ʱ��������˵�level����С�ڻ����log�����е���־
����ͨ������nginx.conf�����ļ���ָ�������ͻ������־���ݣ�����������־�ᱻ���ԡ�
    ��ʹ��ngx_log_debug��ʱ��level��������ȫ��ͬ�����������岻���Ǽ����Ѿ�
��DEBUG���𣩣�������־���ͣ���Ϊngx_log_debug���¼����־������NGX-LOG��
DEBUG���Լ���ģ������level�ɸ���ģ�鶨�塣level��ȡֵ��Χ�μ���4-7��
��4-7 ngx_log_debug��־�ӿ�level������ȡֵ��Χ
�����������������������ש��������ש�������������������������������������������������
��    ��������        ��  ֵ    ��    ����                                        ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG CORE  �� Ox010  ��    Nginx����ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG ALLOC �� Ox020  ��    Nginx�ڷ����ڴ�ʱʹ�õĵ�����־             ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG MUTEX �� Ox040  ��    Nginx��ʹ�ý�����ʱʹ�õĵ�����־           ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG EVENT �� Ox080  ��    Nginx�¼�ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG HTTP  �� Oxl00  ��    Nginx httpģ��ĵ�����־                    ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG DEBUG MAIL  �� Ox200  ��    Nginx�ʼ�ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX LOG_DEBUG_MYSQL �� Ox400  ��    ��ʾ��MySQL��ص�Nginxģ����ʹ�õĵ�����־  ��
�����������������������ߩ��������ߩ�������������������������������������������������
    ��HTTPģ�����ngx_log_debug���¼��־ʱ�����˵�level������NGX_LOG_DEBUG HTTP��
��ʱ���1og����������HTTPģ�飬��ʹ����event�¼�ģ���log����
��������κ���־��������ngx_log_debugӵ��level�������������ڡ�
    (2) log����
    ʵ���ϣ��ڿ���HTTPģ��ʱ���ǲ����ù���log�����Ĺ��죬��Ϊ�ڴ�������ʱngx_
http_request_t�ṹ�е�connection��Ա����һ��ngx_log_t���͵�log��Ա�����Դ���ngx_
log_error���ngx_log_debug���¼��־���ڶ�ȡ���ý׶Σ�ngx_conf_t�ṹҲ��log��Ա
����������¼��־����ȡ���ý׶�ʱ����־��Ϣ�������������̨��Ļ��������򵥵ؿ�һ
��ngx_log_t�Ķ��塣
typedef struct ngx_log_s ngx_log_t;
typedef u_char * (*ngx_log_handler_pt)   (ngx_log_t  *log,  u_char *buf,  size_t  len) ;

struct ngx_log_s  {
    ������־���������־����
    ngx_uint_t log_level;
    f����־�ļ�
    ngx_open_file_t��file;
    ��������������ΪOʱ���������־��
    ngx_atomic_uint_t connection;
    �����¼��־ʱ�Ļص���������handler�Ѿ�ʵ�֣���ΪNULL�������Ҳ���DEBUG���Լ���ʱ���Ż��
��handler���ӷ����
    ngx_log_handler_pt handler;
    ����ÿ��ģ�鶼�����Զ���data��ʹ�÷�����ͨ����data����������ʵ���������handler�ص�������
��ʹ�õġ����磬HTTP��ܾͶ�����handler����������data�з���������������������Ϣ������ÿ�������
־ʱ������������URI�������־��β��+��
    void��data;
    �����ʾ��ǰ�Ķ�����ʵ���ϣ�action��data��һ���ģ�ֻ����ʵ����handler�ص�������Ż�ʹ
�á����磬HTTP��ܾ���handler�����м��action�Ƿ�ΪNULL�������ΪNULL���ͻ�����־����롰while
��+action���Դ˱�ʾ��ǰ��־���ڽ���ʲô������������λ����+��
    char *action;
}

*/
#if (NGX_HAVE_VARIADIC_MACROS)
void
ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, ...) 
//�����ӡһ��Ҫע�⣬����λ�����%d %u��ӡ�ͻ���ֶδ���������%d��ӡngx_event_t->write;
//�����ӡngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream request(ev->write:%u %u)  %V", ngx_event_t->write, ngx_event_t->write); �δ���
#else

void
ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, va_list args)

#endif
{
#if (NGX_HAVE_VARIADIC_MACROS)
    va_list      args;
#endif
    u_char      *p, *last, *msg;
    ssize_t      n;
    ngx_uint_t   wrote_stderr, debug_connection;
    u_char       errstr[NGX_MAX_ERROR_STR];
    char filebuf[52];

    last = errstr + NGX_MAX_ERROR_STR;
    
    p = ngx_cpymem(errstr, ngx_cached_err_log_time.data,
                   ngx_cached_err_log_time.len);

    snprintf(filebuf, sizeof(filebuf), "[%40s, %5d]", filename, lineno);

    p = ngx_slprintf(p, last, "%s ", filebuf);  
    
    p = ngx_slprintf(p, last, " [%V] ", &err_levels[level]);

    /* pid#tid */
    p = ngx_slprintf(p, last, "%P#" NGX_TID_T_FMT ": ", ngx_log_pid, ngx_log_tid);
    
    if (log->connection) {
        p = ngx_slprintf(p, last, "*%uA ", log->connection);
    }

    msg = p;

#if (NGX_HAVE_VARIADIC_MACROS)

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

#else

    p = ngx_vslprintf(p, last, fmt, args);

#endif

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (level != NGX_LOG_DEBUG && log->handler) {
        p = log->handler(log, p, last - p); //��NGX_LOG_DEBUG�����ִ��handler,���������µ���Ϣ����ӡ����Ϣbuf��һ���ӡ
    }

    if (p > last - NGX_LINEFEED_SIZE) {
        p = last - NGX_LINEFEED_SIZE;
    }

    ngx_linefeed(p);

    wrote_stderr = 0;
    debug_connection = (log->log_level & NGX_LOG_DEBUG_CONNECTION) != 0;

    while (log) {

        if (log->log_level < level && !debug_connection) { //ֻ��log_level����level��������error_log������
            break;
        }

        if (log->writer) {
            log->writer(log, level, errstr, p - errstr);
            goto next;
        }

        if (ngx_time() == log->disk_full_time) {

            /*
             * on FreeBSD writing to a full filesystem with enabled softupdates
             * may block process for much longer time than writing to non-full
             * filesystem, so we skip writing to a log for one second
             */

            goto next;
        }

        n = ngx_write_fd(log->file->fd, errstr, p - errstr); //д��log�ļ���

        if (n == -1 && ngx_errno == NGX_ENOSPC) {
            log->disk_full_time = ngx_time();
        }

        if (log->file->fd == ngx_stderr) {
            wrote_stderr = 1;
        }

    next:

        log = log->next;
    }

    if (!ngx_use_stderr
        || level > NGX_LOG_WARN
        || wrote_stderr) /* ���������Щ�������򲻻������ӡ��ǰ̨��ֻ��д��errlog�ļ��� */
    {
        return;
    }

    msg -= (7 + err_levels[level].len + 3);

    (void) ngx_sprintf(msg, "nginx: [%V] ", &err_levels[level]);
    (void) ngx_write_console(ngx_stderr, msg, p - msg);
}


#if (NGX_HAVE_VARIADIC_MACROS)
void
ngx_log_error_coreall(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, ...) 
//�����ӡһ��Ҫע�⣬����λ�����%d %u��ӡ�ͻ���ֶδ���������%d��ӡngx_event_t->write;
//�����ӡngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream request(ev->write:%u %u)  %V", ngx_event_t->write, ngx_event_t->write); �δ���
#else

void
ngx_log_error_coreall(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, va_list args)

#endif

{
#if (NGX_HAVE_VARIADIC_MACROS)
    va_list      args;
#endif
    u_char      *p, *last, *msg;
    ssize_t      n;
    ngx_uint_t   wrote_stderr;//, debug_connection;
    u_char       errstr[NGX_MAX_ERROR_STR];
    char filebuf[52];

    last = errstr + NGX_MAX_ERROR_STR;
    
    p = ngx_cpymem(errstr, ngx_cached_err_log_time.data,
                   ngx_cached_err_log_time.len);

    snprintf(filebuf, sizeof(filebuf), "[%35s, %5d][yangyazhou @@@ test]", filename, lineno);

    p = ngx_slprintf(p, last, "%s ", filebuf);  
    
    p = ngx_slprintf(p, last, " [%V] ", &err_levels[level]);

    /* pid#tid */
    p = ngx_slprintf(p, last, "%P#" NGX_TID_T_FMT ": ", ngx_log_pid, ngx_log_tid); //����ID���߳�ID(�ڿ����̳߳ص�ʱ���߳�ID�ͽ���ID��ͬ)
    
    if (log->connection) {
        p = ngx_slprintf(p, last, "*%uA ", log->connection);
    }

    msg = p;

#if (NGX_HAVE_VARIADIC_MACROS)

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

#else

    p = ngx_vslprintf(p, last, fmt, args);

#endif

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (level != NGX_LOG_DEBUG && log->handler) {
        p = log->handler(log, p, last - p);
    }

    if (p > last - NGX_LINEFEED_SIZE) {
        p = last - NGX_LINEFEED_SIZE;
    }

    ngx_linefeed(p);

    wrote_stderr = 0;
    //debug_connection = (log->log_level & NGX_LOG_DEBUG_CONNECTION) != 0;

    while (log) {

        if (log->writer) {
            log->writer(log, level, errstr, p - errstr);
            goto next;
        }

        if (ngx_time() == log->disk_full_time) {

            /*
             * on FreeBSD writing to a full filesystem with enabled softupdates
             * may block process for much longer time than writing to non-full
             * filesystem, so we skip writing to a log for one second
             */

            goto next;
        }

        n = ngx_write_fd(log->file->fd, errstr, p - errstr); //д��log�ļ���

        if (n == -1 && ngx_errno == NGX_ENOSPC) {
            log->disk_full_time = ngx_time();
        }

        if (log->file->fd == ngx_stderr) {
            wrote_stderr = 1;
        }

    next:

        log = log->next;
    }

    if (!ngx_use_stderr
        || level > NGX_LOG_WARN
        || wrote_stderr) /* ���������Щ�������򲻻������ӡ��ǰ̨��ֻ��д��errlog�ļ��� */
    {
        return;
    }

    msg -= (7 + err_levels[level].len + 3);

    (void) ngx_sprintf(msg, "nginx: [%V] ", &err_levels[level]);
    (void) ngx_write_console(ngx_stderr, msg, p - msg);
}


#if !(NGX_HAVE_VARIADIC_MACROS)

void ngx_cdecl
ngx_log_error(ngx_uint_t level, ngx_log_t *log, ngx_err_t err,
    const char *fmt, ...)
{
    va_list  args;

    if (log->log_level >= level) {
        va_start(args, fmt);
        ngx_log_error_core(level, log,__FUNCTION__, __LINE__, err, fmt, args);
        va_end(args);
    }
}

void ngx_cdecl
ngx_log_errorall(ngx_uint_t level, ngx_log_t *log, ngx_err_t err,
    const char *fmt, ...)
{
    va_list  args;

    va_start(args, fmt);
    ngx_log_error_core(level, log,__FUNCTION__, __LINE__, err, fmt, args);
    va_end(args);
}



void ngx_cdecl
ngx_log_debug_core(ngx_log_t *log, ngx_err_t err, const char *fmt, ...)
{
    va_list  args;

    va_start(args, fmt);
    ngx_log_error_core(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, err, fmt, args);
    va_end(args);
}

#endif


void ngx_cdecl
ngx_log_abort(ngx_err_t err, const char *fmt, ...)
{
    u_char   *p;
    va_list   args;
    u_char    errstr[NGX_MAX_CONF_ERRSTR];

    va_start(args, fmt);
    p = ngx_vsnprintf(errstr, sizeof(errstr) - 1, fmt, args);
    va_end(args);

    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, err,
                  "%*s", p - errstr, errstr);
}


void ngx_cdecl
ngx_log_stderr(ngx_err_t err, const char *fmt, ...)
{
    u_char   *p, *last;
    va_list   args;
    u_char    errstr[NGX_MAX_ERROR_STR];

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, "nginx: ", 7);

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (p > last - NGX_LINEFEED_SIZE) {
        p = last - NGX_LINEFEED_SIZE;
    }

    ngx_linefeed(p);

    (void) ngx_write_console(ngx_stderr, errstr, p - errstr);
}


u_char *
ngx_log_errno(u_char *buf, u_char *last, ngx_err_t err)
{
    if (buf > last - 50) {

        /* leave a space for an error code */

        buf = last - 50;
        *buf++ = '.';
        *buf++ = '.';
        *buf++ = '.';
    }

#if (NGX_WIN32)
    buf = ngx_slprintf(buf, last, ((unsigned) err < 0x80000000)
                                       ? " (%d: " : " (%Xd: ", err);
#else
    buf = ngx_slprintf(buf, last, " (%d: ", err);
#endif

    buf = ngx_strerror(err, buf, last - buf);

    if (buf < last) {
        *buf++ = ')';
    }

    return buf;
}

//��NGX_ERROR_LOG_PATH�ļ�
ngx_log_t *ngx_log_init(u_char *prefix)
{
    u_char  *p, *name;
    size_t   nlen, plen;

    ngx_log.file = &ngx_log_file;
    ngx_log.log_level = NGX_LOG_NOTICE;

    name = (u_char *) NGX_ERROR_LOG_PATH;

    /*
     * we use ngx_strlen() here since BCC warns about
     * condition is always false and unreachable code
     */

    nlen = ngx_strlen(name);

    if (nlen == 0) {
        ngx_log_file.fd = ngx_stderr;
        return &ngx_log;
    }

    p = NULL;

#if (NGX_WIN32)
    if (name[1] != ':') {
#else
    if (name[0] != '/') {
#endif

        if (prefix) {
            plen = ngx_strlen(prefix);

        } else {
#ifdef NGX_PREFIX
            prefix = (u_char *) NGX_PREFIX;
            plen = ngx_strlen(prefix);
#else
            plen = 0;
#endif
        }

        if (plen) {
            name = malloc(plen + nlen + 2); //"NGX_PREFIX/NGX_ERROR_LOG_PATH"
            if (name == NULL) {
                return NULL;
            }

            p = ngx_cpymem(name, prefix, plen);

            if (!ngx_path_separator(*(p - 1))) {
                *p++ = '/';
            }

            ngx_cpystrn(p, (u_char *) NGX_ERROR_LOG_PATH, nlen + 1);

            p = name;
        }
    }

    ngx_log_file.fd = ngx_open_file(name, NGX_FILE_APPEND,
                                    NGX_FILE_CREATE_OR_OPEN,
                                    NGX_FILE_DEFAULT_ACCESS);//��logs/error.log�ļ�

    if (ngx_log_file.fd == NGX_INVALID_FILE) {
        ngx_log_stderr(ngx_errno,
                       "[alert] could not open error log file: "
                       ngx_open_file_n " \"%s\" failed", name);
#if (NGX_WIN32)
        ngx_event_log(ngx_errno,
                       "could not open error log file: "
                       ngx_open_file_n " \"%s\" failed", name);
#endif

        ngx_log_file.fd = ngx_stderr;
    }

    if (p) {
        ngx_free(p);
    }

    return &ngx_log;
}

/*
��������ļ���û��error_log������������ļ�����������errlogģ���ngx_log_open_default��������־�ȼ�Ĭ����ΪNGX_LOG_ERR��
��־�ļ�����ΪNGX_ERROR_LOG_PATH���ú�����configureʱָ���ģ���
*/
ngx_int_t
ngx_log_open_default(ngx_cycle_t *cycle)
{
    ngx_log_t         *log;
    static ngx_str_t   error_log = ngx_string(NGX_ERROR_LOG_PATH);

    /* �����ļ��в�������Ч��error_log������ʱnew_log.file��Ϊ�� */  
    if (ngx_log_get_file_log(&cycle->new_log) != NULL) {
        return NGX_OK;
    }

    if (cycle->new_log.log_level != 0) {
        /* there are some error logs, but no files */

        log = ngx_pcalloc(cycle->pool, sizeof(ngx_log_t));
        if (log == NULL) {
            return NGX_ERROR;
        }

    } else {
        /* no error logs at all */
        log = &cycle->new_log;
    }

    log->log_level = NGX_LOG_ERR;

    log->file = ngx_conf_open_file(cycle, &error_log);
    if (log->file == NULL) {
        return NGX_ERROR;
    }

    if (log != &cycle->new_log) {
        ngx_log_insert(&cycle->new_log, log);
    }

    return NGX_OK;
}

//��cycle->log fd����ΪSTDERR_FILENO
ngx_int_t ngx_log_redirect_stderr(ngx_cycle_t *cycle)
{
    ngx_fd_t  fd;

    if (cycle->log_use_stderr) {
        return NGX_OK;
    }

    /* file log always exists when we are called */
    fd = ngx_log_get_file_log(cycle->log)->file->fd;

    if (fd != ngx_stderr) {
        if (ngx_set_stderr(fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          ngx_set_stderr_n " failed");

            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

ngx_log_t *
ngx_log_get_file_log(ngx_log_t *head)
{
    ngx_log_t  *log;

    for (log = head; log; log = log->next) {
        if (log->file != NULL) {
            return log;
        }
    }

    return NULL;
}

/*

�﷨:  error_log file | stderr [debug | info | notice | warn | error | crit | alert | emerg];
 
Ĭ��ֵ:  error_log logs/error.log error;
 
������:  main, http, server, location
 
������־�� 
��һ�����������˴����־���ļ��� �������Ϊ����ֵstderr��nginx�Ὣ��־�������׼��������� 
�ڶ�������������־���� ��־�����������Ѿ������������ᵽ�ص�˳���г��� ����Ϊĳ����־���𽫻�ʹָ������͸��߼������־������¼������ 
���磬Ĭ�ϼ���error��ʹnginx��¼����error��crit��alert��emerg�������Ϣ�� ���ʡ�����������nginx��ʹ��error�� 
Ϊ��ʹdebug��־��������Ҫ���--with-debug����ѡ� 



error_log file [ debug | info | notice | warn | error | crit ]  | [{  debug_core | debug_alloc | debug_mutex | debug_event | debug_http | debug_mail | debug_mysql } ]
��־���� = ������־���� | ������־����; ����
��־���� = ������־����;
������־�ļ���: emerg, alert, crit, error, warn, notic, info, debug, 
������־�ļ���: debug_core, debug_alloc, debug_mutex, debug_event, debug_http, debug_mail, debug_mysql,

 error_log ָ�����־�������÷�Ϊ ������־����͵�����־����
 �� ������־ֻ������һ������ �� ������־������д�ڵ�����־�����ǰ�� �� ������־�������ö������
 �������÷������ܴﲻ�����Ԥ��. 
*/
static char *
ngx_log_set_levels(ngx_conf_t *cf, ngx_log_t *log)
{
    ngx_uint_t   i, n, d, found;
    ngx_str_t   *value;

    if (cf->args->nelts == 2) {
        log->log_level = NGX_LOG_ERR;
        return NGX_CONF_OK;
    }

    value = cf->args->elts;

    for (i = 2; i < cf->args->nelts; i++) {
        found = 0;

        for (n = 1; n <= NGX_LOG_DEBUG; n++) {
            if (ngx_strcmp(value[i].data, err_levels[n].data) == 0) {

                if (log->log_level != 0) {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "duplicate log level \"%V\"",
                                       &value[i]);
                    return NGX_CONF_ERROR;
                }

                log->log_level = n;
                found = 1;
                break;
            }
        }

        for (n = 0, d = NGX_LOG_DEBUG_FIRST; d <= NGX_LOG_DEBUG_LAST; d <<= 1) {
            if (ngx_strcmp(value[i].data, debug_levels[n++]) == 0) {
                if (log->log_level & ~NGX_LOG_DEBUG_ALL) {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "invalid log level \"%V\"",
                                       &value[i]);
                    return NGX_CONF_ERROR;
                }

                log->log_level |= d; //���õ��Կ���
                found = 1;
                break;
            }
        }


        if (!found) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid log level \"%V\"", &value[i]);
            return NGX_CONF_ERROR;
        }
    }

    if (log->log_level == NGX_LOG_DEBUG) {
        log->log_level = NGX_LOG_DEBUG_ALL;
    }

    
    return NGX_CONF_OK;
}

/* ȫ�������õ�error_log xxx�洢��ngx_cycle_s->new_log��http{}��server{}��local{}���õ�error_log������ngx_http_core_loc_conf_t->error_log,
   ��ngx_log_set_log,���ֻ����ȫ��error_log��������http{}��server{}��local{}����ngx_http_core_merge_loc_conf conf->error_log = &cf->cycle->new_log;  */ 
static char *
ngx_error_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_log_t  *dummy;

    dummy = &cf->cycle->new_log; 

    return ngx_log_set_log(cf, &dummy);
}



/*

�﷨:  error_log file | stderr [debug | info | notice | warn | error | crit | alert | emerg];
 
Ĭ��ֵ:  error_log logs/error.log error;
 
������:  main, http, server, location
 
������־�� 
��һ�����������˴����־���ļ��� �������Ϊ����ֵstderr��nginx�Ὣ��־�������׼��������� 
�ڶ�������������־���� ��־�����������Ѿ������������ᵽ�ص�˳���г��� ����Ϊĳ����־���𽫻�ʹָ������͸��߼������־������¼������ 
���磬Ĭ�ϼ���error��ʹnginx��¼����error��crit��alert��emerg�������Ϣ�� ���ʡ�����������nginx��ʹ��error�� 
Ϊ��ʹdebug��־��������Ҫ���--with-debug����ѡ� 



error_log file [ debug | info | notice | warn | error | crit ]  | [{  debug_core | debug_alloc | debug_mutex | debug_event | debug_http | debug_mail | debug_mysql } ]
��־���� = ������־���� | ������־����; ����
��־���� = ������־����;
������־�ļ���: emerg, alert, crit, error, warn, notic, info, debug, 
������־�ļ���: debug_core, debug_alloc, debug_mutex, debug_event, debug_http, debug_mail, debug_mysql,

 error_log ָ�����־�������÷�Ϊ ������־����͵�����־����
 �� ������־ֻ������һ������ �� ������־������д�ڵ�����־�����ǰ�� �� ������־�������ö������
 �������÷������ܴﲻ�����Ԥ��. 
*/  /* ȫ�������õ�error_log xxx�洢��ngx_cycle_s->new_log��http{}��server{}��local{}���õ�error_log������ngx_http_core_loc_conf_t->error_log,
    ��ngx_log_set_log,���ֻ����ȫ��error_log��������http{}��server{}��local{}����ngx_http_core_merge_loc_conf conf->error_log = &cf->cycle->new_log;  */
    
char *
ngx_log_set_log(ngx_conf_t *cf, ngx_log_t **head)
{
    ngx_log_t          *new_log;
    ngx_str_t          *value, name;
    ngx_syslog_peer_t  *peer;

    if (*head != NULL && (*head)->log_level == 0) {
        new_log = *head;

    } else {

        new_log = ngx_pcalloc(cf->pool, sizeof(ngx_log_t));
        if (new_log == NULL) {
            return NGX_CONF_ERROR;
        }

        if (*head == NULL) {
            *head = new_log;
        }
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "stderr") == 0) {
        ngx_str_null(&name);
        cf->cycle->log_use_stderr = 1;

        new_log->file = ngx_conf_open_file(cf->cycle, &name);
        if (new_log->file == NULL) {
            return NGX_CONF_ERROR;
        }

     } else if (ngx_strncmp(value[1].data, "memory:", 7) == 0) {

#if (NGX_DEBUG)
        size_t                 size, needed;
        ngx_pool_cleanup_t    *cln;
        ngx_log_memory_buf_t  *buf;

        value[1].len -= 7;
        value[1].data += 7;

        needed = sizeof("MEMLOG  :" NGX_LINEFEED)
                 + cf->conf_file->file.name.len
                 + NGX_SIZE_T_LEN
                 + NGX_INT_T_LEN
                 + NGX_MAX_ERROR_STR;

        size = ngx_parse_size(&value[1]);

        if (size == (size_t) NGX_ERROR || size < needed) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid buffer size \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

        buf = ngx_pcalloc(cf->pool, sizeof(ngx_log_memory_buf_t));
        if (buf == NULL) {
            return NGX_CONF_ERROR;
        }

        buf->start = ngx_pnalloc(cf->pool, size);
        if (buf->start == NULL) {
            return NGX_CONF_ERROR;
        }

        buf->end = buf->start + size;

        buf->pos = ngx_slprintf(buf->start, buf->end, "MEMLOG %uz %V:%ui%N",
                                size, &cf->conf_file->file.name,
                                cf->conf_file->line);

        ngx_memset(buf->pos, ' ', buf->end - buf->pos);

        cln = ngx_pool_cleanup_add(cf->pool, 0);
        if (cln == NULL) {
            return NGX_CONF_ERROR;
        }

        cln->data = new_log;
        cln->handler = ngx_log_memory_cleanup;

        new_log->writer = ngx_log_memory_writer;
        new_log->wdata = buf;

#else
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "nginx was built without debug support");
        return NGX_CONF_ERROR;
#endif

     } else if (ngx_strncmp(value[1].data, "syslog:", 7) == 0) {
        peer = ngx_pcalloc(cf->pool, sizeof(ngx_syslog_peer_t));
        if (peer == NULL) {
            return NGX_CONF_ERROR;
        }

        if (ngx_syslog_process_conf(cf, peer) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        new_log->writer = ngx_syslog_writer;
        new_log->wdata = peer;

    } else {
        new_log->file = ngx_conf_open_file(cf->cycle, &value[1]);
        if (new_log->file == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    if (ngx_log_set_levels(cf, new_log) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    if (*head != new_log) {
        ngx_log_insert(*head, new_log);
    }

    return NGX_CONF_OK;
}

//��־������а���־�ȼ��ӵ͵�������
static void
ngx_log_insert(ngx_log_t *log, ngx_log_t *new_log)
{
    ngx_log_t  tmp;

    if (new_log->log_level > log->log_level) {

        /*
         * list head address is permanent, insert new log after
         * head and swap its contents with head
         */

        tmp = *log;
        *log = *new_log;
        *new_log = tmp;

        log->next = new_log;
        return;
    }

    while (log->next) {
        if (new_log->log_level > log->next->log_level) {
            new_log->next = log->next;
            log->next = new_log;
            return;
        }

        log = log->next;
    }

    log->next = new_log;
}


#if (NGX_DEBUG)

static void
ngx_log_memory_writer(ngx_log_t *log, ngx_uint_t level, u_char *buf,
    size_t len)
{
    u_char                *p;
    size_t                 avail, written;
    ngx_log_memory_buf_t  *mem;

    mem = log->wdata;

    if (mem == NULL) {
        return;
    }

    written = ngx_atomic_fetch_add(&mem->written, len);

    p = mem->pos + written % (mem->end - mem->pos);

    avail = mem->end - p;

    if (avail >= len) {
        ngx_memcpy(p, buf, len);

    } else {
        ngx_memcpy(p, buf, avail);
        ngx_memcpy(mem->pos, buf + avail, len - avail);
    }
}


static void
ngx_log_memory_cleanup(void *data)
{
    ngx_log_t *log = data;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, log, 0, "destroy memory log buffer");

    log->wdata = NULL;
}

#endif
