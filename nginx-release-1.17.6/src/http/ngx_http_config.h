
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CONFIG_H_INCLUDED_
#define _NGX_HTTP_CONFIG_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    void        **main_conf;
    void        **srv_conf;
    void        **loc_conf;
} ngx_http_conf_ctx_t;

/** 定义了8个阶段 HTTP 框架在启动过程中会在每个阶段中调用 ngx_http_module_t 中相应的方法
 * 定义的顺序和调用的顺序是不同的，调用顺序：
 * create_main_conf -> create_srv_conf -> create_loc_conf ->
 * preconfiguration -> init_main_conf ->
 * merge_srv_conf -> create_loc_conf -> postconfiguration
 */
typedef struct {
    /** 解析配置文件前调用 */
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf);
    /** 完成配置文件的解析后调用 */
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf);

    /** 当需要创建数据结构用于存储 main 级别（直属于 http{...}块的配置项）的全局配置时，
     * 可以通过 create_main_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_main_conf)(ngx_conf_t *cf);

    /** 常用于初始化 main 级别配置项 */
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);

    /** 当需要创建数据结构用于存储 srv 级别（直属于 server{...}块的配置项）的全局配置时，
     * 可以通过 create_srv_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_srv_conf)(ngx_conf_t *cf);

    /** 主要用于合并 main 级别和 srv 级别下的同名配置项 */
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

    /** 当需要创建数据结构用于存储 loc 级别（直属于 location{...}块的配置项）的全局配置时，
     * 可以通过 create_loc_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_loc_conf)(ngx_conf_t *cf);
    /** 主要用于合并 srv 级别和 loc 级别下的同名配置项 */
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
} ngx_http_module_t;


#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

#define NGX_HTTP_MAIN_CONF        0x02000000
#define NGX_HTTP_SRV_CONF         0x04000000
#define NGX_HTTP_LOC_CONF         0x08000000
#define NGX_HTTP_UPS_CONF         0x10000000
#define NGX_HTTP_SIF_CONF         0x20000000
#define NGX_HTTP_LIF_CONF         0x40000000
#define NGX_HTTP_LMT_CONF         0x80000000


#define NGX_HTTP_MAIN_CONF_OFFSET  offsetof(ngx_http_conf_ctx_t, main_conf)
#define NGX_HTTP_SRV_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, srv_conf)
#define NGX_HTTP_LOC_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, loc_conf)


#define ngx_http_get_module_main_conf(r, module)                             \
    (r)->main_conf[module.ctx_index]
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]
#define ngx_http_get_module_loc_conf(r, module)  (r)->loc_conf[module.ctx_index]


#define ngx_http_conf_get_module_main_conf(cf, module)                        \
    ((ngx_http_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_http_conf_get_module_srv_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]
#define ngx_http_conf_get_module_loc_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]

#define ngx_http_cycle_get_module_main_conf(cycle, module)                    \
    (cycle->conf_ctx[ngx_http_module.index] ?                                 \
        ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index])      \
            ->main_conf[module.ctx_index]:                                    \
        NULL)


#endif /* _NGX_HTTP_CONFIG_H_INCLUDED_ */
