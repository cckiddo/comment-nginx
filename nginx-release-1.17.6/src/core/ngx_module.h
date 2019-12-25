
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Maxim Dounin
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_MODULE_H_INCLUDED_
#define _NGX_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


#define NGX_MODULE_UNSET_INDEX  (ngx_uint_t) -1


#define NGX_MODULE_SIGNATURE_0                                                \
    ngx_value(NGX_PTR_SIZE) ","                                               \
    ngx_value(NGX_SIG_ATOMIC_T_SIZE) ","                                      \
    ngx_value(NGX_TIME_T_SIZE) ","

#if (NGX_HAVE_KQUEUE)
#define NGX_MODULE_SIGNATURE_1   "1"
#else
#define NGX_MODULE_SIGNATURE_1   "0"
#endif

#if (NGX_HAVE_IOCP)
#define NGX_MODULE_SIGNATURE_2   "1"
#else
#define NGX_MODULE_SIGNATURE_2   "0"
#endif

#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
#define NGX_MODULE_SIGNATURE_3   "1"
#else
#define NGX_MODULE_SIGNATURE_3   "0"
#endif

#if (NGX_HAVE_AIO_SENDFILE || NGX_COMPAT)
#define NGX_MODULE_SIGNATURE_4   "1"
#else
#define NGX_MODULE_SIGNATURE_4   "0"
#endif

#if (NGX_HAVE_EVENTFD)
#define NGX_MODULE_SIGNATURE_5   "1"
#else
#define NGX_MODULE_SIGNATURE_5   "0"
#endif

#if (NGX_HAVE_EPOLL)
#define NGX_MODULE_SIGNATURE_6   "1"
#else
#define NGX_MODULE_SIGNATURE_6   "0"
#endif

#if (NGX_HAVE_KEEPALIVE_TUNABLE)
#define NGX_MODULE_SIGNATURE_7   "1"
#else
#define NGX_MODULE_SIGNATURE_7   "0"
#endif

#if (NGX_HAVE_INET6)
#define NGX_MODULE_SIGNATURE_8   "1"
#else
#define NGX_MODULE_SIGNATURE_8   "0"
#endif

#define NGX_MODULE_SIGNATURE_9   "1"
#define NGX_MODULE_SIGNATURE_10  "1"

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
#define NGX_MODULE_SIGNATURE_11  "1"
#else
#define NGX_MODULE_SIGNATURE_11  "0"
#endif

#define NGX_MODULE_SIGNATURE_12  "1"

#if (NGX_HAVE_SETFIB)
#define NGX_MODULE_SIGNATURE_13  "1"
#else
#define NGX_MODULE_SIGNATURE_13  "0"
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
#define NGX_MODULE_SIGNATURE_14  "1"
#else
#define NGX_MODULE_SIGNATURE_14  "0"
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
#define NGX_MODULE_SIGNATURE_15  "1"
#else
#define NGX_MODULE_SIGNATURE_15  "0"
#endif

#if (NGX_HAVE_VARIADIC_MACROS)
#define NGX_MODULE_SIGNATURE_16  "1"
#else
#define NGX_MODULE_SIGNATURE_16  "0"
#endif

#define NGX_MODULE_SIGNATURE_17  "0"
#define NGX_MODULE_SIGNATURE_18  "0"

#if (NGX_HAVE_OPENAT)
#define NGX_MODULE_SIGNATURE_19  "1"
#else
#define NGX_MODULE_SIGNATURE_19  "0"
#endif

#if (NGX_HAVE_ATOMIC_OPS)
#define NGX_MODULE_SIGNATURE_20  "1"
#else
#define NGX_MODULE_SIGNATURE_20  "0"
#endif

#if (NGX_HAVE_POSIX_SEM)
#define NGX_MODULE_SIGNATURE_21  "1"
#else
#define NGX_MODULE_SIGNATURE_21  "0"
#endif

#if (NGX_THREADS || NGX_COMPAT)
#define NGX_MODULE_SIGNATURE_22  "1"
#else
#define NGX_MODULE_SIGNATURE_22  "0"
#endif

#if (NGX_PCRE)
#define NGX_MODULE_SIGNATURE_23  "1"
#else
#define NGX_MODULE_SIGNATURE_23  "0"
#endif

#if (NGX_HTTP_SSL || NGX_COMPAT)
#define NGX_MODULE_SIGNATURE_24  "1"
#else
#define NGX_MODULE_SIGNATURE_24  "0"
#endif

#define NGX_MODULE_SIGNATURE_25  "1"

#if (NGX_HTTP_GZIP)
#define NGX_MODULE_SIGNATURE_26  "1"
#else
#define NGX_MODULE_SIGNATURE_26  "0"
#endif

#define NGX_MODULE_SIGNATURE_27  "1"

#if (NGX_HTTP_X_FORWARDED_FOR)
#define NGX_MODULE_SIGNATURE_28  "1"
#else
#define NGX_MODULE_SIGNATURE_28  "0"
#endif

#if (NGX_HTTP_REALIP)
#define NGX_MODULE_SIGNATURE_29  "1"
#else
#define NGX_MODULE_SIGNATURE_29  "0"
#endif

#if (NGX_HTTP_HEADERS)
#define NGX_MODULE_SIGNATURE_30  "1"
#else
#define NGX_MODULE_SIGNATURE_30  "0"
#endif

#if (NGX_HTTP_DAV)
#define NGX_MODULE_SIGNATURE_31  "1"
#else
#define NGX_MODULE_SIGNATURE_31  "0"
#endif

#if (NGX_HTTP_CACHE)
#define NGX_MODULE_SIGNATURE_32  "1"
#else
#define NGX_MODULE_SIGNATURE_32  "0"
#endif

#if (NGX_HTTP_UPSTREAM_ZONE)
#define NGX_MODULE_SIGNATURE_33  "1"
#else
#define NGX_MODULE_SIGNATURE_33  "0"
#endif

#if (NGX_COMPAT)
#define NGX_MODULE_SIGNATURE_34  "1"
#else
#define NGX_MODULE_SIGNATURE_34  "0"
#endif

#define NGX_MODULE_SIGNATURE                                                  \
    NGX_MODULE_SIGNATURE_0 NGX_MODULE_SIGNATURE_1 NGX_MODULE_SIGNATURE_2      \
    NGX_MODULE_SIGNATURE_3 NGX_MODULE_SIGNATURE_4 NGX_MODULE_SIGNATURE_5      \
    NGX_MODULE_SIGNATURE_6 NGX_MODULE_SIGNATURE_7 NGX_MODULE_SIGNATURE_8      \
    NGX_MODULE_SIGNATURE_9 NGX_MODULE_SIGNATURE_10 NGX_MODULE_SIGNATURE_11    \
    NGX_MODULE_SIGNATURE_12 NGX_MODULE_SIGNATURE_13 NGX_MODULE_SIGNATURE_14   \
    NGX_MODULE_SIGNATURE_15 NGX_MODULE_SIGNATURE_16 NGX_MODULE_SIGNATURE_17   \
    NGX_MODULE_SIGNATURE_18 NGX_MODULE_SIGNATURE_19 NGX_MODULE_SIGNATURE_20   \
    NGX_MODULE_SIGNATURE_21 NGX_MODULE_SIGNATURE_22 NGX_MODULE_SIGNATURE_23   \
    NGX_MODULE_SIGNATURE_24 NGX_MODULE_SIGNATURE_25 NGX_MODULE_SIGNATURE_26   \
    NGX_MODULE_SIGNATURE_27 NGX_MODULE_SIGNATURE_28 NGX_MODULE_SIGNATURE_29   \
    NGX_MODULE_SIGNATURE_30 NGX_MODULE_SIGNATURE_31 NGX_MODULE_SIGNATURE_32   \
    NGX_MODULE_SIGNATURE_33 NGX_MODULE_SIGNATURE_34


#define NGX_MODULE_V1                                                         \
    NGX_MODULE_UNSET_INDEX, NGX_MODULE_UNSET_INDEX,                           \
    NULL, 0, 0, nginx_version, NGX_MODULE_SIGNATURE

#define NGX_MODULE_V1_PADDING  0, 0, 0, 0, 0, 0, 0, 0


struct ngx_module_s {
    /* 对一类模块（由下面的 type 成员决定类别）而言， ctx_index 表示当前模块在这类模块中的
       序号。这个成员常常是由管理这类模块的一个 Nginx 核心模块设置的，对于所有的 HTTP 模块而
       言， ctx_index 是由核心模块 ngx_http_module 设置的。 ctx_index 非常重要， Nginx
       的模块化设计非常依赖于和个模块的顺序，它们既用于表达优先级，也用于表明每个模块的位置，借
       以帮助 Nginx 架构快速获得某个模块的数据  */
    ngx_uint_t            ctx_index;

    /* 表示当前模块在 ngx_modules 数组中的序号。注意， ctx_index 表示的是当前模块在一类模块
       中的序号，而 index 表示当前模块在所有模块中的序号，它同样关键。 Nginx 启动时会根据
       ngx_modules 数组设置各模块的 index 值  */
    ngx_uint_t            index;

    /* 模块名称 */
    char                 *name;

    /* spare 系列的保留变量，暂未使用 */
    ngx_uint_t            spare0;
    ngx_uint_t            spare1;

    /* 模块的版本，便于将来的扩展。目前只有一种，默认为1 */
    ngx_uint_t            version;
    const char           *signature;

    /* ctx 用于指定一类模块的上下文结构体。为什么需要 ctx 呢？因为前面说过， Nginx 模块
       有许多种类，不同模块之前的功能差别很大。例如，事件类型的砌块主要处理 I/O 事件相关的功能
       HTTP 类型的模块主要处理 HTTP 应用层的功能。这样，每个模块都有了自己的特性，而 ctx 将
       会指向特定类型模块的公共接口。例如， HTTP 模块中， ctx 需要指向 ngx_http_module_t
       结构体 */
    void                 *ctx;

    /* 处理 nginx.conf 中的配置项 */
    ngx_command_t        *commands;

    /* 表示该模块的类型，它与 ctx 指针是紧密相关的。在官方 Nginx 中，它的取值范围是以下5种：
     * NGX_HTTP_MODULE, NGX_CORE_MODULE, NGX_CONF_MODULE, NGX_EVENT_MODULE
     * NGX_MAIL_MODULE。还可以自定义新的模块类型 */
    ngx_uint_t            type;

    /** 在 Nginx 的启动、停止过程中，以下7个函数指针表示7个执行点会分别调用这7种方法。对于任何
       一个方法而言，如果不需要 Nginx 在某个时刻执行它，那么简单地把它设置为 NULL 空指针即可 */
    /*
     * 虽然从字面上理解应当在 master 进程启动时回调 init_master ，但到目前为止，
     * 构架代码从来不会调用它，因为，可将 init_master 设置为 NULL
    */
    ngx_int_t           (*init_master)(ngx_log_t *log);

    /** 在初始化所有模块时调用。 在 master/worker 模式下，这个阶段将在启动 worker 子进程前完成*/
    ngx_int_t           (*init_module)(ngx_cycle_t *cycle);

    /** 在正常服务前被调用。在 master/worker 模式下，多个 worker 子进程已经产生，在每个
     * worker 进程的初始化过程会调用所有模块的 init_process 函数 */
    ngx_int_t           (*init_process)(ngx_cycle_t *cycle);
    /** 由于 Nginx 暂不支持多线程模式，所以 init_thread ，exit_thread 在框架代码中
     * 没有被调用过，设为 NULL */
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);
    void                (*exit_thread)(ngx_cycle_t *cycle);
    /** 在服务停止前调用。在 master/worker 模式下， worker 进程会在退出前调用它 */
    void                (*exit_process)(ngx_cycle_t *cycle);
    /** 在 master 进程退出前被调用 */
    void                (*exit_master)(ngx_cycle_t *cycle);

    /** 以下8个 spare_hook 变量也是保留字段，目前没有使用，但可用 Nginx 提供
     * 的 NGX_MODULE_V1_PADDING 宏来填充。即全0 */
    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};


typedef struct {
    ngx_str_t             name;
    void               *(*create_conf)(ngx_cycle_t *cycle);
    char               *(*init_conf)(ngx_cycle_t *cycle, void *conf);
} ngx_core_module_t;


ngx_int_t ngx_preinit_modules(void);
ngx_int_t ngx_cycle_modules(ngx_cycle_t *cycle);
ngx_int_t ngx_init_modules(ngx_cycle_t *cycle);
ngx_int_t ngx_count_modules(ngx_cycle_t *cycle, ngx_uint_t type);


ngx_int_t ngx_add_module(ngx_conf_t *cf, ngx_str_t *file,
    ngx_module_t *module, char **order);


extern ngx_module_t  *ngx_modules[];
extern ngx_uint_t     ngx_max_module;

extern char          *ngx_module_names[];


#endif /* _NGX_MODULE_H_INCLUDED_ */
