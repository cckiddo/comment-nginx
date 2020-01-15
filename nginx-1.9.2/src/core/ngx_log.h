
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LOG_H_INCLUDED_
#define _NGX_LOG_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*
��4-6 ngx_log_error��־�ӿ�level������ȡֵ��Χ
�������������������ש������ש���������������������������������������������������������������������
��    ��������    ��  ֵ  ��    ����                                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��    ��߼�����־����־�����ݲ�����д��log����ָ�����ļ������ǻ�ֱ�� ��
��NGX_LOG_STDERR  ��    O ��                                                                    ��
��                ��      ������־�������׼�����豸�������̨��Ļ                              ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��  ����NGX��LOG ALERT���𣬶�С�ڻ����NGX LOG EMERG�����           ��
��NGX_LOG_EMERG   ��   1  ��                                                                    ��
��                ��      ����־���������log����ָ�����ļ���                                   ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG_ALERT   ��    2 ��    ���NGX LOG CRIT����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG_CRIT  ��    3 ��    ���NGX LOG ERR����                                             ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG_ERR   ��    4 ��    ���NGX��LOG WARN����                                           ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG_WARN  ��    5 ��    ����NGX LOG NOTICE����                                          ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX_LOG_NOTICE  ��  6   ��  ����NGX__ LOG INFO����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX_LOG_INFO  ��    7 ��    ����NGX��LOG DEBUG����                                          ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX_LOG_DEBUG   ��    8 ��    ���Լ�����ͼ�����־                                          ��
�������������������ߩ������ߩ���������������������������������������������������������������������
*/ 
//stderr (0)>= emerg(1) >= alert(2) >= crit(3) >= err(4)>= warn(5) >= notice(6) >= info(7) >= debug(8) 
//debug������ͣ�stderr������ߣ�Բ�����е������Ƕ�Ӧ��־�ȼ���ֵ��
//log->log_level�еĵ�4λȡֵΪNGX_LOG_STDERR��  5-12λȡֵΪλͼ����ʾ��Ӧģ�����־   ����NGX_LOG_DEBUG_CONNECTION NGX_LOG_DEBUG_ALL��Ӧconnect��־��������־
//������Щͨ��ngx_log_error���  ��Ӧerr_levels[]�ο�ngx_log_set_levels
#define NGX_LOG_STDERR            0
#define NGX_LOG_EMERG             1
#define NGX_LOG_ALERT             2
#define NGX_LOG_CRIT              3
#define NGX_LOG_ERR               4
#define NGX_LOG_WARN              5 //���level > NGX_LOG_WARN�򲻻�����Ļǰ̨��ӡ����ngx_log_error_core
#define NGX_LOG_NOTICE            6
#define NGX_LOG_INFO              7
#define NGX_LOG_DEBUG             8

/*
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
    ��HTTPģ�����ngx_log_debug���¼��־ʱ�����˵�level������NGX- LOG��
DEBUG HTTP����ʱ���109����������HTTPģ�飬��ʹ����event�¼�ģ���log����
��������κ���־��������ngx_log_debugӵ��level�������������ڡ�
*/ //������Щ��ͨ��������ж��Ƿ���Ҫ��ӡ�����Բο�ngx_log_debug7
//log->log_level�еĵ�4λȡֵΪNGX_LOG_STDERR��  5-12λȡֵΪλͼ����ʾ��Ӧģ�����־   ����NGX_LOG_DEBUG_CONNECTION NGX_LOG_DEBUG_ALL��Ӧconnect��־��������־
//���ͨ���Ӳ���debug_http����NGX_LOG_DEBUG_HTTP���أ���debug_levels  ngx_log_set_levels,��������濪���е�һ������NGX_LOG_STDERR��NGX_LOG_DEBUG��ȫ���򿪣���Ϊlog_level�ܴ�
//������Щͨ��ngx_log_debug0 -- ngx_log_debug8���  ��Ӧdebug_levels[] �ο�ngx_log_set_levels
#define NGX_LOG_DEBUG_CORE        0x010
#define NGX_LOG_DEBUG_ALLOC       0x020
#define NGX_LOG_DEBUG_MUTEX       0x040
#define NGX_LOG_DEBUG_EVENT       0x080  
#define NGX_LOG_DEBUG_HTTP        0x100
#define NGX_LOG_DEBUG_MAIL        0x200
#define NGX_LOG_DEBUG_MYSQL       0x400
#define NGX_LOG_DEBUG_STREAM      0x800

/*
 * do not forget to update debug_levels[] in src/core/ngx_log.c
 * after the adding a new debug level
 */
//log->log_level�еĵ�4λȡֵΪNGX_LOG_STDERR��  5-12λȡֵΪλͼ����ʾ��Ӧģ�����־   ����NGX_LOG_DEBUG_CONNECTION NGX_LOG_DEBUG_ALL��Ӧconnect��־��������־
#define NGX_LOG_DEBUG_FIRST       NGX_LOG_DEBUG_CORE
#define NGX_LOG_DEBUG_LAST        NGX_LOG_DEBUG_STREAM
#define NGX_LOG_DEBUG_CONNECTION  0x80000000 // --with-debug)                    NGX_DEBUG=YES  ���������־ 
#define NGX_LOG_DEBUG_ALL         0x7ffffff0


typedef u_char *(*ngx_log_handler_pt) (ngx_log_t *log, u_char *buf, size_t len);
typedef void (*ngx_log_writer_pt) (ngx_log_t *log, ngx_uint_t level,
    u_char *buf, size_t len);


struct ngx_log_s {  
    //������õ�log����Ϊdebug�������ngx_log_set_levels��level����ΪNGX_LOG_DEBUG_ALL
    //��ֵ��ngx_log_set_levels
    ngx_uint_t           log_level;//��־���������־����  Ĭ��ΪNGX_LOG_ERR  ���ͨ��error_log  logs/error.log  info;��Ϊ���õĵȼ�  �ȸü����µ���־���Դ�ӡ
    ngx_open_file_t     *file; //��־�ļ�

    ngx_atomic_uint_t    connection;//����������ΪOʱ���������־��

    time_t               disk_full_time;

    /* ��¼��־ʱ�Ļص���������handler�Ѿ�ʵ�֣���ΪNULL�������Ҳ���DEBUG���Լ���ʱ���Ż����handler���ӷ��� */
    ngx_log_handler_pt   handler; //�����ӳػ�ȡngx_connection_t��c->log->handler = ngx_http_log_error;

    /*
    ÿ��ģ�鶼�����Զ���data��ʹ�÷�����ͨ����data����������ʵ���������handler�ص�������
    ��ʹ�õġ����磬HTTP��ܾͶ�����handler����������data�з���������������������Ϣ������ÿ�������
    ־ʱ������������URI�������־��β��
    */
    void                *data; //ָ��ngx_http_log_ctx_t����ngx_http_init_connection

    ngx_log_writer_pt    writer;
    void                *wdata;

    /*
     * we declare "action" as "char *" because the actions are usually
     * the static strings and in the "u_char *" case we have to override
     * their types all the time
     */
     
    /*
    ��ʾ��ǰ�Ķ�����ʵ���ϣ�action��data��һ���ģ�ֻ����ʵ����handler�ص�������Ż�ʹ
    �á����磬HTTP��ܾ���handler�����м��action�Ƿ�ΪNULL�������ΪNULL���ͻ�����־����롰while
    ��+action���Դ˱�ʾ��ǰ��־���ڽ���ʲô������������λ����
    */
    char                *action;
    //ngx_log_insert���룬��ngx_log_error_core�ҵ���Ӧ�������־���ý����������Ϊ��������error_log��ͬ�������־�洢�ڲ�ͬ����־�ļ���
    ngx_log_t           *next;
};

#define NGX_STR2BUF_LEN 256
void ngx_str_t_2buf(char *buf, ngx_str_t *str);


#define NGX_MAX_ERROR_STR   2048


/*********************************/

#if (NGX_HAVE_C99_VARIADIC_MACROS)

#define NGX_HAVE_VARIADIC_MACROS  1
/*
��4-6 ngx_log_error��־�ӿ�level������ȡֵ��Χ
�������������������ש������ש���������������������������������������������������������������������
��    ��������    ��  ֵ  ��    ����                                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��    ��߼�����־����־�����ݲ�����д��log����ָ�����ļ������ǻ�ֱ�� ��
��NGX_LOG_STDERR  ��    O ��                                                                    ��
��                ��      ������־�������׼�����豸�������̨��Ļ                              ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��                ��      ��  ����NGX��LOG ALERT���𣬶�С�ڻ����NGX LOG EMERG�����           ��
��NGX_LOG:EMERG   ��  1   ��                                                                    ��
��                ��      ����־���������log����ָ�����ļ���                                   ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG ALERT   ��    2 ��    ���NGX LOG CRIT����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG CRIT  ��    3 ��    ���NGX LOG ERR����                                             ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG ERR   ��    4 ��    ���NGX��LOG WARN����                                           ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG WARN  ��    5 ��    ����NGX LOG NOTICE����                                          ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG NOTICE  ��  6   ��  ����NGX__ LOG INFO����                                            ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��  NGX LOG INFO  ��    7 ��    ����NGX��LOG DEBUG����                                          ��
�ǩ����������������贈�����贈��������������������������������������������������������������������
��NGX LOG DEBUG   ��    8 ��    ���Լ�����ͼ�����־                                          ��
�������������������ߩ������ߩ���������������������������������������������������������������������
    ʹ��ngx_log_error���¼��־ʱ��������˵�level����С�ڻ����log�����е���־
����ͨ������nginx.conf�����ļ���ָ�������ͻ������־���ݣ�����������־�ᱻ���ԡ�
*/
#define ngx_log_error(level, log, ...)                                        \
    if ((log)->log_level >= level) ngx_log_error_core(level, log,__FUNCTION__, __LINE__, __VA_ARGS__)

void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, ...);
    
void ngx_log_error_coreall(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, ...);

/*
    ��ʹ��ngx_log_debug��ʱ��level��������ȫ��ͬ�����������岻���Ǽ����Ѿ�
��DEBUG���𣩣�������־���ͣ���Ϊngx_log_debug���¼����־������NGX-LOG��
DEBUG���Լ���ģ������level�ɸ���ģ�鶨�塣level��ȡֵ��Χ�μ���4-7��
��4-7 ngx_log_debug��־�ӿ�level������ȡֵ��Χ
�����������������������ש��������ש�������������������������������������������������
��    ��������        ��  ֵ    ��    ����                                        ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_CORE  �� Ox010  ��    Nginx����ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_ALLOC �� Ox020  ��    Nginx�ڷ����ڴ�ʱʹ�õĵ�����־             ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_MUTEX �� Ox040  ��    Nginx��ʹ�ý�����ʱʹ�õĵ�����־           ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_EVENT �� Ox080  ��    Nginx�¼�ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_HTTP  �� Oxl00  ��    Nginx httpģ��ĵ�����־                    ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_MAIL  �� Ox200  ��    Nginx�ʼ�ģ��ĵ�����־                     ��
�ǩ��������������������贈�������贈������������������������������������������������
��NGX_LOG_DEBUG_MYSQL �� Ox400  ��    ��ʾ��MySQL��ص�Nginxģ����ʹ�õĵ�����־  ��
�����������������������ߩ��������ߩ�������������������������������������������������
    ��HTTPģ�����ngx_log_debug���¼��־ʱ�����˵�level������NGX_LOG_DEBUG HTTP��
��ʱ���1og����������HTTPģ�飬��ʹ����event�¼�ģ���log����
��������κ���־��������ngx_log_debugӵ��level�������������ڡ�
*/
#define ngx_log_debug(level, log, ...)                                        \
    if ((log)->log_level & level)                                             \
        ngx_log_error_core(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, __VA_ARGS__)
        
#define ngx_log_debugall(log, ...)                                        \
            ngx_log_error_coreall(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, __VA_ARGS__)

/*********************************/

#elif (NGX_HAVE_GCC_VARIADIC_MACROS)

#define NGX_HAVE_VARIADIC_MACROS  1

#define ngx_log_error(level, log, args...)                                    \
    if ((log)->log_level >= level) ngx_log_error_core(level, log,__FUNCTION__, __LINE__, args)

void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, ...);
    void ngx_log_error_coreall(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
        const char *fmt, ...);

#define ngx_log_debug(level, log, args...)                                    \
    if ((log)->log_level & level)                                             \
        ngx_log_error_core(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, args)

#define ngx_log_debugall(log, args...)                                    \
            ngx_log_error_coreall(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, args)

/*********************************/

#else /* no variadic macros */

#define NGX_HAVE_VARIADIC_MACROS  0

void ngx_cdecl ngx_log_error(ngx_uint_t level, ngx_log_t *log, ngx_err_t err,
    const char *fmt, ...);
void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, va_list args);
void ngx_log_error_coreall(ngx_uint_t level, ngx_log_t *log, const char* filename, int lineno, ngx_err_t err,
    const char *fmt, va_list args);
#define ngx_log_debugall(log, args...)                                    \
            ngx_log_error_coreall(NGX_LOG_DEBUG, log,__FUNCTION__, __LINE__, args)

void ngx_cdecl ngx_log_debug_core(ngx_log_t *log, ngx_err_t err,
    const char *fmt, ...);


#endif /* variadic macros */


/*********************************/

#if (NGX_DEBUG)

#if (NGX_HAVE_VARIADIC_MACROS)
/* ע�ⲻ����%V������������ֶδ��� */
#define ngx_log_debug0(level, log, err, fmt)                                  \
        ngx_log_debug(level, log, err, fmt)

#define ngx_log_debug1(level, log, err, fmt, arg1)                            \
        ngx_log_debug(level, log, err, fmt, arg1)

#define ngx_log_debug2(level, log, err, fmt, arg1, arg2)                      \
        ngx_log_debug(level, log, err, fmt, arg1, arg2)

#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3)                \
        ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3)

#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)          \
        ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4)

#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)    \
        ngx_log_debug(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)

#define ngx_log_debug6(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6)                    \
        ngx_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6)

#define ngx_log_debug7(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)              \
        ngx_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define ngx_log_debug8(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)        \
        ngx_log_debug(level, log, err, fmt,                                   \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)



#else /* no variadic macros */

#define ngx_log_debug0(level, log, err, fmt)                                  \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt)

#define ngx_log_debug1(level, log, err, fmt, arg1)                            \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1)

#define ngx_log_debug2(level, log, err, fmt, arg1, arg2)                      \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1, arg2)

#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3)                \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3)

#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)          \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4)

#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)    \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5)

#define ngx_log_debug6(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6)                    \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)

#define ngx_log_debug7(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)              \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt,                                     \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define ngx_log_debug8(level, log, err, fmt,                                  \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)        \
    if ((log)->log_level & level)                                             \
        ngx_log_debug_core(log, err, fmt,                                     \
                       arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

#endif

#else /* !NGX_DEBUG */

#define ngx_log_debug0(level, log, err, fmt)
#define ngx_log_debug1(level, log, err, fmt, arg1)
#define ngx_log_debug2(level, log, err, fmt, arg1, arg2)
#define ngx_log_debug3(level, log, err, fmt, arg1, arg2, arg3)
#define ngx_log_debug4(level, log, err, fmt, arg1, arg2, arg3, arg4)
#define ngx_log_debug5(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5)
#define ngx_log_debug6(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
#define ngx_log_debug7(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                       arg6, arg7)
#define ngx_log_debug8(level, log, err, fmt, arg1, arg2, arg3, arg4, arg5,    \
                       arg6, arg7, arg8)

#endif

/*********************************/

ngx_log_t *ngx_log_init(u_char *prefix);
void ngx_cdecl ngx_log_abort(ngx_err_t err, const char *fmt, ...);
void ngx_cdecl ngx_log_stderr(ngx_err_t err, const char *fmt, ...);
u_char *ngx_log_errno(u_char *buf, u_char *last, ngx_err_t err);
ngx_int_t ngx_log_open_default(ngx_cycle_t *cycle);
ngx_int_t ngx_log_redirect_stderr(ngx_cycle_t *cycle);
ngx_log_t *ngx_log_get_file_log(ngx_log_t *head);
char *ngx_log_set_log(ngx_conf_t *cf, ngx_log_t **head);

/*
 * ngx_write_stderr() cannot be implemented as macro, since
 * MSVC does not allow to use #ifdef inside macro parameters.
 *
 * ngx_write_fd() is used instead of ngx_write_console(), since
 * CharToOemBuff() inside ngx_write_console() cannot be used with
 * read only buffer as destination and CharToOemBuff() is not needed
 * for ngx_write_stderr() anyway.
 */
static ngx_inline void
ngx_write_stderr(char *text)
{
    (void) ngx_write_fd(ngx_stderr, text, ngx_strlen(text));
}


static ngx_inline void
ngx_write_stdout(char *text)
{
    (void) ngx_write_fd(ngx_stdout, text, ngx_strlen(text));
}


extern ngx_module_t  ngx_errlog_module;
extern ngx_uint_t    ngx_use_stderr;


#endif /* _NGX_LOG_H_INCLUDED_ */
