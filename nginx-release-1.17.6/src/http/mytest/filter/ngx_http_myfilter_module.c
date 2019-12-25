/** HTTP 框架允许普通的 HTTP 处理模块介入其中的7个阶段处理请求，但是通常大部分 HTTP 模块
 * （官方模块或者第三方模块）都只有两种介入方式，
 * 第一种是，任一个 HTTP 模块对所有的用户请求产生作用，
 * 第二种是，只对请求的 URI 匹配了 nginx.conf 中的某些 location 表达式下的 HTTP 模块起作用。
 *
 * 如果希望多个 HTTP 模块共同处理一个请求，则多半是由 subrequest 功能来完成，即原始请求分为
 * 多个子请求，每个子请求再由一个 HTTP 模块在 NGX_HTTP_CONTENT_PHASE 阶段处理。
 *
 * 然而，HTTP 过滤模块则不同于此，一个请求可以被任意个 HTTP 过滤模块处理。因此，普通的 HTTP
 * 模块更货币于完成请求的核心功能，如 static 模块负责静态文件的处理。如 gzip 过滤模块。
 *
 * HTTP 过滤模块的别一个特性是，在普通 HTTP 模块处理请求完毕，并调用 ngx_http_send_header
 * 发送 HTTP头部，或者调用 ngx_http_output_filter 发送 HTTP 包体时，才会由这两个方法依次
 * 调用所有的 HTTP 过滤模块来处理这个请求。因此， HTTP 过滤模块仅处理服务器发往客户端的 HTTP
 * 响应，而不处理客户端发往服务器的 HTTP 请求。
 *
 * 过滤模块可以选择性地只处理 HTTP 头部或者 HTTP 包体，当然也可以二者皆处理。
 *
 * HTTP 框架中定义的链表入口：
 * ngx_http_top_header_filter;当执行 ngx_http_send_header 发送 HTTP 头部时，
 * 就从 ngx_http_top_header_filter 指针开始遍历所有的 HTTP 头部过滤模块。
 *
 * ngx_http_top_body_filter;而当执行 ngx_http_output_filter 发送 HTTP 包体时，
 * 就从 ngx_top_body_filter 指针开始遍历所有的 HTTP 包体过滤模块
 *
 * 每个过滤模块在初始化时，会插入到链表的首部。
 *
 * demo 功能：给响应实体的开关处添加一个前缀
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct
{
    ngx_flag_t		enable;
} ngx_http_myfilter_conf_t;

typedef struct
{
    ngx_int_t   	add_prefix;
} ngx_http_myfilter_ctx_t;


static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

//将在包体中添加这个前缀
static ngx_str_t filter_prefix = ngx_string("[my filter prefix]");



static void* ngx_http_myfilter_create_conf(ngx_conf_t *cf);
static char *
ngx_http_myfilter_merge_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t ngx_http_myfilter_init(ngx_conf_t *cf);
static ngx_int_t
ngx_http_myfilter_header_filter(ngx_http_request_t *r);
static ngx_int_t
ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in);




static ngx_command_t  ngx_http_myfilter_commands[] =
{
    {
        ngx_string("add_prefix"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_FLAG,
        /** 设置 set 为 on/off 类型的配置检查 */
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_myfilter_conf_t, enable),
        NULL
    },

    ngx_null_command
};

/** 大多数官方 HTTP 过滤模块都会把初始化方法放到 postconfiguration 指针中，那么它将当前模块
 * 加入到过滤链表中。不建议把初始化方法放到 ngx_http_module_t 的其他成员中，那样会导致 HTTP
 * 过滤模块的顺序不可控。
 *
 * 只需要查看 configure 命令生成的 ngx_modules.c 文件就可以知道所有 HTTP 过滤模块的顺序了。
 * 对于 HTTP 过滤模块来说，在 ngx_modules 数组中的位置越靠后，在实际执行请求时就越优先执行。
 * 因为在初始化 HTTP 过滤模块时，每一个 HTTP 过滤模块都是将自己插入到整个单链表的首部的。
 *
 * P197 模块加载顺序：官方和第三方加载顺序
 *  */
static ngx_http_module_t  ngx_http_myfilter_module_ctx =
{
    NULL,                                  /* preconfiguration方法  */
    ngx_http_myfilter_init,            /* postconfiguration方法 */

    NULL,                                  /*create_main_conf 方法 */
    NULL,                                  /* init_main_conf方法 */

    NULL,                                  /* create_srv_conf方法 */
    NULL,                                  /* merge_srv_conf方法 */

    ngx_http_myfilter_create_conf,    /* create_loc_conf方法 */
    ngx_http_myfilter_merge_conf      /*merge_loc_conf方法*/
};


ngx_module_t  ngx_http_myfilter_module =
{
    NGX_MODULE_V1,
    &ngx_http_myfilter_module_ctx,     /* module context */
    ngx_http_myfilter_commands,        /* module directives */
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



static ngx_int_t ngx_http_myfilter_init(ngx_conf_t *cf)
{
    //插入到头部处理方法链表的首部
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_myfilter_header_filter;

    //插入到包体处理方法链表的首部
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_myfilter_body_filter;

    return NGX_OK;
}

static ngx_int_t
ngx_http_myfilter_header_filter(ngx_http_request_t *r)
{
    ngx_http_myfilter_ctx_t   *ctx;
    ngx_http_myfilter_conf_t  *conf;

    //如果不是返回成功，这时是不需要理会是否加前缀的，直接交由下一个过滤模块
    //处理响应码非200的情形
    if (r->headers_out.status != NGX_HTTP_OK) {
        return ngx_http_next_header_filter(r);
    }

    //获取http上下文
    ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
    if (ctx) {
        //该请求的上下文已经存在，这说明
        // ngx_http_myfilter_header_filter已经被调用过1次，
        //直接交由下一个过滤模块处理
        return ngx_http_next_header_filter(r);
    }

    //获取存储配置项的 ngx_http_myfilter_conf_t 结构体
    conf = ngx_http_get_module_loc_conf(r, ngx_http_myfilter_module);

    //如果enable成员为0，也就是配置文件中没有配置add_prefix配置项，
    //或者add_prefix配置项的参数值是off，这时直接交由下一个过滤模块处理
    if (conf->enable == 0) {
        return ngx_http_next_header_filter(r);
    }

    //构造http上下文结构体ngx_http_myfilter_ctx_t
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_myfilter_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    //add_prefix为0表示不加前缀
    ctx->add_prefix = 0;

    //将构造的上下文设置到当前请求中
    ngx_http_set_ctx(r, ctx, ngx_http_myfilter_module);

    //myfilter过滤模块只处理Content-Type是"text/plain"类型的http响应
    if (r->headers_out.content_type.len >= sizeof("text/plain") - 1
        && ngx_strncasecmp(r->headers_out.content_type.data, (u_char *) "text/plain", sizeof("text/plain") - 1) == 0)
    {
        //1表示需要在http包体前加入前缀
        ctx->add_prefix = 1;

        //如果处理模块已经在Content-Length写入了http包体的长度，由于
        //我们加入了前缀字符串，所以需要把这个字符串的长度也加入到 Content-Length 中
        if (r->headers_out.content_length_n > 0)
            r->headers_out.content_length_n += filter_prefix.len;
    }

    //交由下一个过滤模块继续处理
    return ngx_http_next_header_filter(r);
}


static ngx_int_t
ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_myfilter_ctx_t   *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
    //如果获取不到上下文，或者上下文结构体中的add_prefix为0或者2时，
    //都不会添加前缀，这时直接交给下一个http过滤模块处理
    if (ctx == NULL || ctx->add_prefix != 1) {
        return ngx_http_next_body_filter(r, in);
    }

    //将add_prefix设置为2，这样即使ngx_http_myfilter_body_filter
    //再次回调时，也不会重复添加前缀
    ctx->add_prefix = 2;

    //从请求的内存池中分配内存，用于存储字符串前缀
    ngx_buf_t* b = ngx_create_temp_buf(r->pool, filter_prefix.len);
    //将ngx_buf_t中的指针正确地指向filter_prefix字符串
    b->start = b->pos = filter_prefix.data;
    b->last = b->pos + filter_prefix.len;

    //从请求的内存池中生成ngx_chain_t链表，将刚分配的ngx_buf_t设置到
    //其buf成员中，并将它添加到原先待发送的http包体前面
    ngx_chain_t *cl = ngx_alloc_chain_link(r->pool);
    cl->buf = b;
    cl->next = in;

    //调用下一个模块的http包体处理方法，注意这时传入的是新生成的cl链表
    return ngx_http_next_body_filter(r, cl);
}


static void* ngx_http_myfilter_create_conf(ngx_conf_t *cf)
{
    ngx_http_myfilter_conf_t  *mycf;

    //创建存储配置项的结构体
    mycf = (ngx_http_myfilter_conf_t  *)ngx_pcalloc(cf->pool, sizeof(ngx_http_myfilter_conf_t));
    if (mycf == NULL) {
        return NULL;
    }

    //ngx_flat_t类型的变量，如果使用预设函数ngx_conf_set_flag_slot
    //解析配置项参数，必须初始化为NGX_CONF_UNSET
    mycf->enable = NGX_CONF_UNSET;

    return mycf;
}

static char *
ngx_http_myfilter_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_myfilter_conf_t *prev = (ngx_http_myfilter_conf_t *)parent;
    ngx_http_myfilter_conf_t *conf = (ngx_http_myfilter_conf_t *)child;

    //合并ngx_flat_t类型的配置项enable
    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    return NGX_CONF_OK;
}


