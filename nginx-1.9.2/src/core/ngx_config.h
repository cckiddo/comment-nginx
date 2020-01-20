
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONFIG_H_INCLUDED_
#define _NGX_CONFIG_H_INCLUDED_


#include <ngx_auto_headers.h>


#if defined __DragonFly__ && !defined __FreeBSD__
#define __FreeBSD__        4
#define __FreeBSD_version  480101
#endif


#if (NGX_FREEBSD)
#include <ngx_freebsd_config.h>


#elif (NGX_LINUX)
#include <ngx_linux_config.h>


#elif (NGX_SOLARIS)
#include <ngx_solaris_config.h>


#elif (NGX_DARWIN)
#include <ngx_darwin_config.h>


#elif (NGX_WIN32)
#include <ngx_win32_config.h>


#else /* POSIX */
#include <ngx_posix_config.h>

#endif


#ifndef NGX_HAVE_SO_SNDLOWAT
#define NGX_HAVE_SO_SNDLOWAT     1
#endif


#if !(NGX_WIN32)

#define ngx_signal_helper(n)     SIG##n
#define ngx_signal_value(n)      ngx_signal_helper(n)

#define ngx_random               random

/* TODO: #ifndef */
#define NGX_SHUTDOWN_SIGNAL      QUIT
#define NGX_TERMINATE_SIGNAL     TERM
#define NGX_NOACCEPT_SIGNAL      WINCH
#define NGX_RECONFIGURE_SIGNAL   HUP  //nginx -s reload�ᴥ�����º�

#if (NGX_LINUXTHREADS)
#define NGX_REOPEN_SIGNAL        INFO
#define NGX_CHANGEBIN_SIGNAL     XCPU
#else
#define NGX_REOPEN_SIGNAL        USR1
#define NGX_CHANGEBIN_SIGNAL     USR2
#endif

#define ngx_cdecl
#define ngx_libc_cdecl

#endif

/*
������linux��ͷ�ļ��в���������͵Ķ��壬��/usr/include/stdint.h���ͷ�ļ����ҵ���������͵Ķ��壨��֪����ô���������ͼƬ������ʹ�����֣���
                           
[cpp] view plaincopy
117  Types for `void *' pointers.  
118 #if __WORDSIZE == 64  
119 # ifndef __intptr_t_defined  
120 typedef long int        intptr_t;  
121 #  define __intptr_t_defined  
122 # endif  
123 typedef unsigned long int   uintptr_t;  
124 #else  
125 # ifndef __intptr_t_defined  
126 typedef int         intptr_t;  
127 #  define __intptr_t_defined  
128 # endif  
129 typedef unsigned int        uintptr_t;  
130 #endif  

������intptr_t����ָ�����ͣ������ϱߵ�һ��ע�ͣ� Types for `void *' pointers. �����˺��ɻ󡣼�Ȼ����ָ�����ͣ�����Ϊʲô˵������Ϊ�ˡ�void *��ָ�룿 
�ֲ���һ���ڡ��������Linux�ں�Դ�롷���ҵ��˴𰸣�ԭ���������£�

�����ڻ�ϲ�ͬ��������ʱ�����С��, ��ʱ�кܺõ�����������. һ���������Ϊ�ڴ��ȡ, ���ں����ʱ�������. ������, ���ܵ�ַ��ָ��, 
�ڴ������ʹ��һ���޷��ŵ��������͸��õ����; �ں˶Դ������ڴ���ͬһ��������, �����ڴ��ַֻ��һ����������. ��һ����, һ��ָ�����׽�����; 
��ֱ�Ӵ����ڴ��ȡʱ, �㼸���Ӳ��������ַ�ʽ������. ʹ��һ���������ͱ��������ֽ�����, ��˱����� bug. ���, �ں���ͨ�����ڴ��ַ������ unsigned long, 
������ָ��ͳ�����һֱ����ͬ��С�������ʵ, ������ Linux Ŀǰ֧�ֵ�����ƽ̨��.

��Ϊ����ֵ��ԭ��, C99 ��׼������ intptr_t �� uintptr_t ���͸�һ�����Գ���һ��ָ��ֵ�����ͱ���. ����, ��Щ���ͼ���û�� 2.6 �ں���ʹ��
*/
/* 
Nginxʹ��ngx_int_t��װ�з������ͣ�ʹ��ngx_uint_t��װ�޷������͡�Nginx��ģ��ı������嶼�����ʹ�õģ������������Nginx��ϰ�ߣ��Դ����int��unsinged int��

��Linuxƽ̨�£�Nginx��ngx_int_t��ngx_uint_t�Ķ������£�
typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
*/
typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t; //һ�������������е� ON | OFFѡ����  1����ON 0����OFF  ��ʼֵһ��Ҫ����ΪNGX_CONF_UNSET�����򱨴���ngx_conf_set_flag_slot

#define NGX_INT32_LEN   (sizeof("-2147483648") - 1)
#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#if (NGX_PTR_SIZE == 4)
#define NGX_INT_T_LEN   NGX_INT32_LEN
#define NGX_MAX_INT_T_VALUE  2147483647

#else
#define NGX_INT_T_LEN   NGX_INT64_LEN
#define NGX_MAX_INT_T_VALUE  9223372036854775807
#endif


#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
//// �� m ���䵽�ڴ�����ַ 
#define ngx_align_ptr(p, a)                                                    \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#define ngx_abort       abort


/* TODO: platform specific: array[NGX_INVALID_ARRAY_INDEX] must cause SIGSEGV */
#define NGX_INVALID_ARRAY_INDEX 0x80000000


/* TODO: auto_conf: ngx_inline   inline __inline __inline__ */
#ifndef ngx_inline
#define ngx_inline      inline
#endif

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif

#ifdef MAXHOSTNAMELEN
#define NGX_MAXHOSTNAMELEN  MAXHOSTNAMELEN
#else
#define NGX_MAXHOSTNAMELEN  256
#endif


#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define NGX_MAX_UINT32_VALUE  (uint32_t) 0xffffffffLL
#else
#define NGX_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#endif

#define NGX_MAX_INT32_VALUE   (uint32_t) 0x7fffffff


#endif /* _NGX_CONFIG_H_INCLUDED_ */

