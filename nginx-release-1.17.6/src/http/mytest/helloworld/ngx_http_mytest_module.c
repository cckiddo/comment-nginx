/**
 * 我们这样定义这个新增 HTTP 模块介入 Nginx 的方式：
 * 1、不希望模块对所有的 HTTP 请求起作用
 * 2、在 nginx.conf 文件中的 http{}, server{}, 或者 location{} 块内定义 mytest 配置项，
 * 如果一个用户请求通过主机域名、 URI 等匹配上了相应的配置块，而这个配置块下又具有 mytest 配置
 * 项，那么希望 mytest 模块开始处理请求。
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);


/** 用来定义模块的配置文件参数，每一个数组元素都是 ngx_command_t 类型，结尾用 ngx_null_command */
static ngx_command_t  ngx_http_mytest_commands[] =
{
    {
        ngx_string("mytest"),   /** 模块的名称设为 mytest */
        /** 可以配置在 main{}, server{}, location{}, 另外两个不是很清楚，后续再补充 */
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
        /** 模块的 set 函数，当某个配置块中出现 mytest 配置项时， Nginx 就会调用 */
        ngx_http_mytest,
        /**  */
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
    NULL,                              /* preconfiguration */
    NULL,                  		/* postconfiguration */

    NULL,                              /* create main configuration */
    NULL,                              /* init main configuration */

    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */

    NULL,       			/* create location configuration */
    NULL         			/* merge location configuration */
};

/** init_module, init_process, exit_process, exit_master， 调用它们的是 Nginx 的框架代码
 * 这4个回调方法与 HTTP 框架无关，所以定义 HTTP 模块时都把它们设为 NULL 空指针。这样，当 Nginx
 * 不作为 Web 服务器使用时，不会执行 HTTP 模块的任何代码
 */
ngx_module_t  ngx_http_mytest_module =
{
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,           /* module context */
    ngx_http_mytest_commands,              /* module directives */
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


static char *
ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    //首先找到mytest配置项所属的配置块，clcf看上去像是location块内的数据
    //结构，其实不然，它可以是main、srv或者loc级别配置项，也就是说在每个
    //http{}和server{}内也都有一个ngx_http_core_loc_conf_t结构体
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    //http框架在处理用户请求进行到 NGX_HTTP_CONTENT_PHASE 阶段时，如果
    //请求的主机域名、URI与mytest配置项所在的配置块相匹配，就将调用我们
    //实现的ngx_http_mytest_handler方法处理这个请求
    clcf->handler = ngx_http_mytest_handler;

    return NGX_CONF_OK;
}


static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
    //必须是GET或者HEAD方法，否则返回405 Not Allowed
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    //丢弃请求中的包体
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) {
        return rc;
    }

    //设置返回的Content-Type。注意，ngx_str_t有一个很方便的初始化宏
    //ngx_string，它可以把ngx_str_t的data和len成员都设置好
    ngx_str_t type = ngx_string("text/plain");
    //返回的包体内容
    ngx_str_t response = ngx_string("Hello World!");
    //设置返回状态码
    r->headers_out.status = NGX_HTTP_OK;
    //响应包是有包体内容的，所以需要设置Content-Length长度
    r->headers_out.content_length_n = response.len;
    //设置Content-Type
    r->headers_out.content_type = type;

    //发送 http 响应头部
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    //构造ngx_buf_t结构准备发送包体
    ngx_buf_t                 *b;
    b = ngx_create_temp_buf(r->pool, response.len); /** 从内存池中分配 ngx_buf_t */
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    //将Hello World拷贝到ngx_buf_t指向的内存中
    ngx_memcpy(b->pos, response.data, response.len);
    //注意，一定要设置好last指针
    b->last = b->pos + response.len;
    //声明这是最后一块缓冲区
    b->last_buf = 1;

    //构造发送时的ngx_chain_t结构体
    ngx_chain_t		out;
    //赋值ngx_buf_t
    out.buf = b;
    //设置next为NULL
    out.next = NULL;

    //最后一步发送包体，http框架会调用ngx_http_finalize_request方法，结束请求
    /** HTTP 框架在 NGX_HTTP_CONTENT_PHASE 阶段调用 ngx_http_my_handler 后，会将
     * ngx_http_mytest_handler 的返回值作为参数传给 ngx_http_finalize_request 方法。
     * 上面的 r->content_handler 会指向 ngx_http_mytest_handler 处理方法。也就是说，
     * 事实上 ngx_http_finalize_request 决定了 ngx_http_mytest_handler 如何起作用。
     */
    return ngx_http_output_filter(r, &out);
}


