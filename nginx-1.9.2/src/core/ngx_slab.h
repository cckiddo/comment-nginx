
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_array_t                    caches;  /* ngx_http_file_cache_t * */
} ngx_http_fastcgi_main_conf_t;


typedef struct { //创建空间和赋值见ngx_http_fastcgi_init_params
    ngx_array_t                   *flushes;
    ngx_array_t                   *lengths;//fastcgi_param  HTTP_xx  XXX设置的非HTTP_变量的相关变量的长度code添加到这里面
    ngx_array_t                   *values;//fastcgi_param  HTTP_xx  XXX设置的非HTTP_变量的相关变量的value值code添加到这里面
    ngx_uint_t                     number; //fastcgi_param  HTTP_  XXX;环境中通过fastcgi_param设置的HTTP_xx变量个数
    ngx_hash_t                     hash;//fastcgi_param  HTTP_  XXX;环境中通过fastcgi_param设置的HTTP_xx通过hash运算存到该hash表中,实际存到hash中时会把HTTP_这5个字符去掉
} ngx_http_fastcgi_params_t; //ngx_http_fastcgi_loc_conf_t->params(params_source)中存储


typedef struct {
    ngx_http_upstream_conf_t       upstream; //例如fastcgi在ngx_http_fastcgi_pass中创建upstream空间，ngx_http_xxx_pass

    ngx_str_t                      index;
    //在ngx_http_fastcgi_init_params中通过脚本解析引擎把变量code添加到params中
    ngx_http_fastcgi_params_t      params; //Params数据包，用于传递执行页面所需要的参数和环境变量。
#if (NGX_HTTP_CACHE)
    ngx_http_fastcgi_params_t      params_cache;
#endif

    //fastcgi_param设置的传送到FastCGI服务器的相关参数都添加到该数组中，见ngx_http_upstream_param_set_slot
    ngx_array_t                   *params_source;  //最终会在ngx_http_fastcgi_init_params中通过脚本解析引擎把变量code添加到params中

    /*
    Sets a string to search for in the error stream of a response received from a FastCGI server. If the string is found then
    it is considered that the FastCGI server has returned an invalid response. This allows handling application errors in nginx, for example:

    location /php {
        fastcgi_pass backend:9000;
        ...
        fastcgi_catch_stderr "PHP Fatal error";
        fastcgi_next_upstream error timeout invalid_header;
    }
    */ //如果后端返回的fastcgi ERRSTD信息中的data部分带有fastcgi_catch_stderr配置的字符串，则会请求下一个后端服务器 参考ngx_http_fastcgi_process_header
    ngx_array_t                   *catch_stderr; //fastcgi_catch_stderr xxx_catch_stderr

    //在ngx_http_fastcgi_eval中执行对应的code，从而把相关变量转换为普通字符串
    //赋值见ngx_http_fastcgi_pass
    ngx_array_t                   *fastcgi_lengths; //fastcgi相关参数的长度code  如果fastcgi_pass xxx中有变量，则该数组为空
    ngx_array_t                   *fastcgi_values; //fastcgi相关参数的值code

    ngx_flag_t                     keep_conn; //fastcgi_keep_conn  on | off  默认off

#if (NGX_HTTP_CACHE)
    ngx_http_complex_value_t       cache_key;
    //fastcgi_cache_key proxy_cache_key指令的时候计算出来的复杂表达式结构，存放在flcf->cache_key中 ngx_http_fastcgi_cache_key ngx_http_proxy_cache_key
#endif

#if (NGX_PCRE)
    ngx_regex_t                   *split_regex;
    ngx_str_t                      split_name;
#endif
} ngx_http_fastcgi_loc_conf_t;

//http://my.oschina.net/goal/blog/196599
typedef enum { //对应ngx_http_fastcgi_header_t的各个字段   参考ngx_http_fastcgi_process_record解析过程 组包过程ngx_http_fastcgi_create_request
    //fastcgi头部
    ngx_http_fastcgi_st_version = 0,
    ngx_http_fastcgi_st_type,
    ngx_http_fastcgi_st_request_id_hi,
    ngx_http_fastcgi_st_request_id_lo,
    ngx_http_fastcgi_st_content_length_hi,
    ngx_http_fastcgi_st_content_length_lo,
    ngx_http_fastcgi_st_padding_length,
    ngx_http_fastcgi_st_reserved,

    ngx_http_fastcgi_st_data, //fastcgi内容
    ngx_http_fastcgi_st_padding //8字节对齐填充字段
} ngx_http_fastcgi_state_e; //fastcgi报文格式，头部(8字节)+内容(一般是8内容头部+数据)+填充字段(8字节对齐引起的填充字节数)


typedef struct {
    u_char                        *start;
    u_char                        *end;
} ngx_http_fastcgi_split_part_t; //创建和赋值见ngx_http_fastcgi_process_header  如果一次解析fastcgi头部行信息没完成，需要再次读取后端数据解析

//在解析从后端发送过来的fastcgi头部信息的时候用到，见ngx_http_fastcgi_process_header
typedef struct { //ngx_http_fastcgi_handler分配空间
//用他来记录每次读取解析过程中的各个状态(如果需要多次epoll触发读取，就需要记录前面读取解析时候的状态)f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);
    ngx_http_fastcgi_state_e       state; //标识解析到了fastcgi 8字节头部中的那个地方
    u_char                        *pos; //指向要解析内容的头
    u_char                        *last;//指向要解析内容的尾部
    ngx_uint_t                     type; //交互标识，例如NGX_HTTP_FASTCGI_STDOUT等
    size_t                         length; //该条fastcgi信息的包体内容长度 不包括padding填充
    size_t                         padding; //填充了多少个字节，从而8字节对齐

    ngx_chain_t                   *free;
    ngx_chain_t                   *busy;

    unsigned                       fastcgi_stdout:1; //标识有收到fastcgi stdout标识信息
    unsigned                       large_stderr:1; //标识有收到fastcgi stderr标识信息
    unsigned                       header_sent:1;
    //创建和赋值见ngx_http_fastcgi_process_header  如果一次解析fastcgi头部行信息没完成，需要再次读取后端数据解析
    ngx_array_t                   *split_parts;

    ngx_str_t                      script_name;
    ngx_str_t                      path_info;
} ngx_http_fastcgi_ctx_t;
//用他来记录每次读取解析过程中的各个状态(如果需要多次epoll触发读取，就需要记录前面读取解析时候的状态)f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

#define NGX_HTTP_FASTCGI_KEEP_CONN      1  //NGX_HTTP_FASTCGI_RESPONDER标识的fastcgi header中的flag为该值表示和后端使用长连接

//FASTCGI交互流程标识，可以参考http://my.oschina.net/goal/blog/196599
#define NGX_HTTP_FASTCGI_RESPONDER      1 //到后端服务器的标识信息 参考ngx_http_fastcgi_create_request  这个标识携带长连接还是短连接ngx_http_fastcgi_request_start

#define NGX_HTTP_FASTCGI_BEGIN_REQUEST  1 //到后端服务器的标识信息 参考ngx_http_fastcgi_create_request  请求开始 ngx_http_fastcgi_request_start
#define NGX_HTTP_FASTCGI_ABORT_REQUEST  2
#define NGX_HTTP_FASTCGI_END_REQUEST    3 //后端到nginx 参考ngx_http_fastcgi_process_record
#define NGX_HTTP_FASTCGI_PARAMS         4 //到后端服务器的标识信息 参考ngx_http_fastcgi_create_request 客户端请求行中的HTTP_xx信息和fastcgi_params参数通过他发送
#define NGX_HTTP_FASTCGI_STDIN          5 //到后端服务器的标识信息 参考ngx_http_fastcgi_create_request  客户端发送到服务端的包体用这个标识

#define NGX_HTTP_FASTCGI_STDOUT         6 //后端到nginx 参考ngx_http_fastcgi_process_record  该标识一般会携带数据，通过解析到的ngx_http_fastcgi_ctx_t->length表示数据长度
#define NGX_HTTP_FASTCGI_STDERR         7 //后端到nginx 参考ngx_http_fastcgi_process_record
#define NGX_HTTP_FASTCGI_DATA           8


/*
typedef struct {
unsigned char version;
unsigned char type;
unsigned char requestIdB1;
unsigned char requestIdB0;
unsigned char contentLengthB1;
unsigned char contentLengthB0;
unsigned char paddingLength;     //填充字节数
unsigned char reserved;

unsigned char contentData[contentLength]; //内容不符
unsigned char paddingData[paddingLength];  //填充字符
} FCGI_Record;

*/
//fastcgi报文格式，头部(8字节)+内容(一般是8内容头部+数据)+填充字段(8字节对齐引起的填充字节数)  可以参考http://my.oschina.net/goal/blog/196599
typedef struct { //解析的时候对应前面的ngx_http_fastcgi_state_e
    u_char  version;
    u_char  type; //NGX_HTTP_FASTCGI_BEGIN_REQUEST  等
    u_char  request_id_hi;//序列号，请求应答一般一致
    u_char  request_id_lo;
    u_char  content_length_hi; //内容字节数
    u_char  content_length_lo;
    u_char  padding_length; //填充字节数
    u_char  reserved;//保留字段
} ngx_http_fastcgi_header_t; //   参考ngx_http_fastcgi_process_record解析过程 组包过程ngx_http_fastcgi_create_request


typedef struct {
    u_char  role_hi;
    u_char  role_lo; //NGX_HTTP_FASTCGI_RESPONDER或者0
    u_char  flags;//NGX_HTTP_FASTCGI_KEEP_CONN或者0  如果设置了和后端长连接flcf->keep_conn则为NGX_HTTP_FASTCGI_KEEP_CONN否则为0，见ngx_http_fastcgi_create_request
    u_char  reserved[5];
} ngx_http_fastcgi_begin_request_t;//包含在ngx_http_fastcgi_request_start_t


typedef struct {
    u_char  version;
    u_char  type;
    u_char  request_id_hi;
    u_char  request_id_lo;
} ngx_http_fastcgi_header_small_t; //包含在ngx_http_fastcgi_request_start_t


typedef struct {
    ngx_http_fastcgi_header_t         h0;//请求开始头包括正常头，加上开始请求的头部，
    ngx_http_fastcgi_begin_request_t  br;

    //这是什么�?莫非是下一个请求的参数部分的头部，预先追加在此?对，当为NGX_HTTP_FASTCGI_PARAMS模式时，后面直接追加KV
    ngx_http_fastcgi_header_small_t   h1;
} ngx_http_fastcgi_request_start_t; //见ngx_http_fastcgi_request_start


static ngx_int_t ngx_http_fastcgi_eval(ngx_http_request_t *r,
    ngx_http_fastcgi_loc_conf_t *flcf);
#if (NGX_HTTP_CACHE)
static ngx_int_t ngx_http_fastcgi_create_key(ngx_http_request_t *r);
#endif
static ngx_int_t ngx_http_fastcgi_create_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_fastcgi_reinit_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_fastcgi_body_output_filter(void *data,
    ngx_chain_t *in);
static ngx_int_t ngx_http_fastcgi_process_header(ngx_http_request_t *r);
static ngx_int_t ngx_http_fastcgi_input_filter_init(void *data);
static ngx_int_t ngx_http_fastcgi_input_filter(ngx_event_pipe_t *p,
    ngx_buf_t *buf);
static ngx_int_t ngx_http_fastcgi_non_buffered_filter(void *data,
    ssize_t bytes);
static ngx_int_t ngx_http_fastcgi_process_record(ngx_http_request_t *r,
    ngx_http_fastcgi_ctx_t *f);
static void ngx_http_fastcgi_abort_request(ngx_http_request_t *r);
static void ngx_http_fastcgi_finalize_request(ngx_http_request_t *r,
    ngx_int_t rc);

static ngx_int_t ngx_http_fastcgi_add_variables(ngx_conf_t *cf);
static void *ngx_http_fastcgi_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_fastcgi_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_fastcgi_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_fastcgi_init_params(ngx_conf_t *cf,
    ngx_http_fastcgi_loc_conf_t *conf, ngx_http_fastcgi_params_t *params,
    ngx_keyval_t *default_params);

static ngx_int_t ngx_http_fastcgi_script_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_fastcgi_path_info_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_http_fastcgi_ctx_t *ngx_http_fastcgi_split(ngx_http_request_t *r,
    ngx_http_fastcgi_loc_conf_t *flcf);

static char *ngx_http_fastcgi_pass(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_fastcgi_split_path_info(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
static char *ngx_http_fastcgi_store(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#if (NGX_HTTP_CACHE)
static char *ngx_http_fastcgi_cache(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_fastcgi_cache_key(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#endif

static char *ngx_http_fastcgi_lowat_check(ngx_conf_t *cf, void *post,
    void *data);


static ngx_conf_post_t  ngx_http_fastcgi_lowat_post =
    { ngx_http_fastcgi_lowat_check };


static ngx_conf_bitmask_t  ngx_http_fastcgi_next_upstream_masks[] = {
    { ngx_string("error"), NGX_HTTP_UPSTREAM_FT_ERROR },
    { ngx_string("timeout"), NGX_HTTP_UPSTREAM_FT_TIMEOUT },
    { ngx_string("invalid_header"), NGX_HTTP_UPSTREAM_FT_INVALID_HEADER },
    { ngx_string("http_500"), NGX_HTTP_UPSTREAM_FT_HTTP_500 },
    { ngx_string("http_503"), NGX_HTTP_UPSTREAM_FT_HTTP_503 },
    { ngx_string("http_403"), NGX_HTTP_UPSTREAM_FT_HTTP_403 },
    { ngx_string("http_404"), NGX_HTTP_UPSTREAM_FT_HTTP_404 },
    { ngx_string("updating"), NGX_HTTP_UPSTREAM_FT_UPDATING },
    { ngx_string("off"), NGX_HTTP_UPSTREAM_FT_OFF },
    { ngx_null_string, 0 }
};


ngx_module_t  ngx_http_fastcgi_module;


static ngx_command_t  ngx_http_fastcgi_commands[] = {
/*
语法：fastcgi_pass fastcgi-server
默认值：none
使用字段：http, server, location
指定FastCGI服务器监听端口与地址，可以是本机或者其它：
fastcgi_pass   localhost:9000;

使用Unix socket:
fastcgi_pass   unix:/tmp/fastcgi.socket;

同样可以使用一个upstream字段名称：
upstream backend  {
  server   localhost:1234;
}

fastcgi_pass   backend;
*/
    { ngx_string("fastcgi_pass"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE1,
      ngx_http_fastcgi_pass,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
fastcgi_index
语法：fastcgi_index file
默认值：none
使用字段：http, server, location
如果URI以斜线结尾，文件名将追加到URI后面，这个值将存储在变量$fastcgi_script_name中。例如：

fastcgi_index  index.php;
fastcgi_param  SCRIPT_FILENAME  /home/www/scripts/php$fastcgi_script_name;请求"/page.php"的参数SCRIPT_FILENAME将被设置为
"/home/www/scripts/php/page.php"，但是"/"为"/home/www/scripts/php/index.php"。
*/
    { ngx_string("fastcgi_index"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, index),
      NULL },
/*
语法：fastcgi_split_path_info regex
使用字段：location
可用版本：0.7.31以上，示例：

location ~ ^(.+\.php)(.*)$ {
...
fastcgi_split_path_info ^(.+\.php)(.*)$;
fastcgi_param SCRIPT_FILENAME /path/to/php$fastcgi_script_name;
fastcgi_param PATH_INFO $fastcgi_path_info;
fastcgi_param PATH_TRANSLATED $document_root$fastcgi_path_info;
...
}请求"/show.php/article/0001"的参数SCRIPT_FILENAME将设置为"/path/to/php/show.php"，参数PATH_INFO为"/article/0001"。
*/
    { ngx_string("fastcgi_split_path_info"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_fastcgi_split_path_info,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
语法：fastcgi_store [on | off | path]
默认值：fastcgi_store off
使用字段：http, server, location
制定了存储前端文件的路径，参数on指定了将使用root和alias指令相同的路径，off禁止存储，此外，参数中可以使用变量使路径名更明确：

fastcgi_store   /data/www$original_uri;应答中的"Last-Modified"头将设置文件的最后修改时间，为了使这些文件更加安全，可以将其在一个目录中存为临时文件，使用fastcgi_temp_path指令。
这个指令可以用在为那些不是经常改变的后端动态输出创建本地拷贝的过程中。如：

location /images/ {
  root                 /data/www;
  error_page           404 = /fetch$uri;
}

location /fetch {
  internal;

  fastcgi_pass           fastcgi://backend;
  fastcgi_store          on;
  fastcgi_store_access   user:rw  group:rw  all:r;
  fastcgi_temp_path      /data/temp;

  alias                  /data/www;
}fastcgi_store并不是缓存，某些需求下它更像是一个镜像。
*/   //fastcgi_store和fastcgi_cache只能配置一个
    { ngx_string("fastcgi_store"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_fastcgi_store,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
语法：fastcgi_store_access users:permissions [users:permission ...]
默认值：fastcgi_store_access user:rw
使用字段：http, server, location
这个参数指定创建文件或目录的权限，例如：

fastcgi_store_access  user:rw  group:rw  all:r;如果要指定一个组的人的相关权限，可以不写用户，如：
fastcgi_store_access  group:rw  all:r;
*/
    { ngx_string("fastcgi_store_access"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE123,
      ngx_conf_set_access_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.store_access),
      NULL },

/*
语法：fastcgi_buffers the_number is_size;
默认值：fastcgi_buffers 8 4k/8k;
使用字段：http, server, location
这个参数指定了从FastCGI服务器到来的应答，本地将用多少和多大的缓冲区读取，默认这个参数等于分页大小，根据环境的不同可能是4K, 8K或16K。
*/
    { ngx_string("fastcgi_buffering"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.buffering),
      NULL },

    //设置是否缓存请求body 到磁盘时  注意fastcgi_request_buffering和fastcgi_buffering的区别，一个是客户端包体，一个是后端服务器应答的包体
    { ngx_string("fastcgi_request_buffering"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.request_buffering),
      NULL },

/*
语法：fastcgi_ignore_client_abort on|off
默认值：fastcgi_ignore_client_abort off
使用字段：http, server, location
"如果当前连接请求FastCGI服务器失败，为防止其与nginx服务器断开连接，可以用这个指令。
*/
    { ngx_string("fastcgi_ignore_client_abort"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.ignore_client_abort),
      NULL },

    { ngx_string("fastcgi_bind"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_upstream_bind_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.local),
      NULL },

/*
语法：fastcgi_connect_timeout time
默认值：fastcgi_connect_timeout 60
使用字段：http, server, location
指定同FastCGI服务器的连接超时时间，这个值不能超过75秒。
*/ //accept后端服务器的时候，有可能connect返回NGX_AGAIN，表示散步握手的ack还没有回来。因此设置这个超时事件表示如果60s还没有ack过来，则做超时处理
    { ngx_string("fastcgi_connect_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.connect_timeout),
      NULL },
    //向FastCGI传送请求的超时时间，这个值是指已经完成两次握手后向FastCGI传送请求的超时时间。
    { ngx_string("fastcgi_send_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.send_timeout),
      NULL },

    { ngx_string("fastcgi_send_lowat"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.send_lowat),
      &ngx_http_fastcgi_lowat_post },

/*
语法：fastcgi_buffer_size the_size ;
默认值：fastcgi_buffer_size 4k/8k ;
使用字段：http, server, location
这个参数指定将用多大的缓冲区来读取从FastCGI服务器到来应答的第一部分。
通常来说在这个部分中包含一个小的应答头。
默认的缓冲区大小为fastcgi_buffers指令中的每块大小，可以将这个值设置更小。
*/ //头部行部分(也就是第一个fastcgi data标识信息，里面也会携带一部分网页数据)的fastcgi标识信息开辟的空间用buffer_size配置指定
//ngx_http_upstream_process_header中分配fastcgi_buffer_size指定的空间
    { ngx_string("fastcgi_buffer_size"),  //注意和后面的fastcgi_buffers的区别  //指定的空间开辟在ngx_http_upstream_process_header
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.buffer_size),
      NULL },

    { ngx_string("fastcgi_pass_request_headers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.pass_request_headers),
      NULL },

    //是否转发客户端浏览器过来的包体到后端去
    { ngx_string("fastcgi_pass_request_body"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.pass_request_body),
      NULL },

/*
语法：fastcgi_intercept_errors on|off
默认值：fastcgi_intercept_errors off
使用字段：http, server, location
这个指令指定是否传递4xx和5xx错误信息到客户端，或者允许nginx使用error_page处理错误信息。
你必须明确的在error_page中指定处理方法使这个参数有效，正如Igor所说“如果没有适当的处理方法，nginx不会拦截一个错误，这个错误
不会显示自己的默认页面，这里允许通过某些方法拦截错误。
*/
    { ngx_string("fastcgi_intercept_errors"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.intercept_errors),
      NULL },
/*
语法：fastcgi_read_timeout time
默认值：fastcgi_read_timeout 60
使用字段：http, server, location
前端FastCGI服务器的响应超时时间，如果有一些直到它们运行完才有输出的长时间运行的FastCGI进程，或者在错误日志中出现前端服务器响应超
时错误，可能需要调整这个值。
*/ //接收FastCGI应答的超时时间，这个值是指已经完成两次握手后接收FastCGI应答的超时时间。
    { ngx_string("fastcgi_read_timeout"), //读取后端数据的超时时间
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.read_timeout),
      NULL },

    /* 读取完后端的第一个fastcgi data标识数据后(一般是头部行和部分数据，该部分空间大小用fastcgi_buffer_size)指定，如果网页包体比较大
        则需要开辟多个新的buff来存储包体部分，分配fastcgi_buffers 5 3K，表示如果这部分空间用完(例如后端速度快，到客户端速度慢)，则不在接收
        后端数据，等待着5个3K中的部分发送出去后，在重新用空余出来的空间接收后端数据
     */ //注意这个只对buffing方式有效  //该配置配置的空间真正分配空间在//在ngx_event_pipe_read_upstream中创建空间
    { ngx_string("fastcgi_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.bufs),
      NULL },

    //xxx_buffers指定为接收后端服务器数据最多开辟这么多空间，xxx_busy_buffers_size指定一次发送后有可能数据没有全部发送出去，因此放入busy链中
    //当没有发送出去的busy链中的buf指向的数据(不是buf指向的空间未发送的数据)达到xxx_busy_buffers_size就不能从后端读取数据，只有busy链中的数据发送一部分出去后小与xxx_busy_buffers_size才能继续读取
    //buffring方式下生效，生效地方可以参考ngx_event_pipe_write_to_downstream
    { ngx_string("fastcgi_busy_buffers_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.busy_buffers_size_conf),
      NULL },

    { ngx_string("fastcgi_force_ranges"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.force_ranges),
      NULL },

    { ngx_string("fastcgi_limit_rate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.limit_rate),
      NULL },

#if (NGX_HTTP_CACHE)
/*
       nginx的存储系统分两类，一类是通过proxy_store开启的，存储方式是按照url中的文件路径，存储在本地。比如/file/2013/0001/en/test.html，
     那么nginx就会在指定的存储目录下依次建立各个目录和文件。另一类是通过proxy_cache开启，这种方式存储的文件不是按照url路径来组织的，
     而是使用一些特殊方式来管理的(这里称为自定义方式)，自定义方式就是我们要重点分析的。那么这两种方式各有什么优势呢？


    按url路径存储文件的方式，程序处理起来比较简单，但是性能不行。首先有的url巨长，我们要在本地文件系统上建立如此深的目录，那么文件的打开
    和查找都很会很慢(回想kernel中通过路径名查找inode的过程吧)。如果使用自定义方式来处理模式，尽管也离不开文件和路径，但是它不会因url长度
    而产生复杂性增加和性能的降低。从某种意义上说这是一种用户态文件系统，最典型的应该算是squid中的CFS。nginx使用的方式相对简单，主要依靠
    url的md5值来管理
     */

/*
语法：fastcgi_cache zone|off;
默认值：off
使用字段：http, server, location
为缓存实际使用的共享内存指定一个区域，相同的区域可以用在不同的地方。
*/   //fastcgi_store和fastcgi_cache只能配置一个
//xxx_cache(proxy_cache fastcgi_cache) abc必须xxx_cache_path(proxy_cache_path fastcgi_cache_path) xxx keys_zone=abc:10m;一起，否则在ngx_http_proxy_merge_loc_conf会失败，因为没有为该abc创建ngx_http_file_cache_t
//fastcgi_cache 指令指定了在当前作用域中使用哪个缓存维护缓存条目，参数对应的缓存必须事先由 fastcgi_cache_path 指令定义。
//获取该结构ngx_http_upstream_cache_get，实际上是通过proxy_cache xxx或者fastcgi_cache xxx来获取共享内存块名的，因此必须设置proxy_cache或者fastcgi_cache

    { ngx_string("fastcgi_cache"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_fastcgi_cache,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
/*
语法：fastcgi_cache_key line
默认值：none
使用字段：http, server, location
设置缓存的关键字，如：

fastcgi_cache_key localhost:9000$request_uri;
*/ //proxy和fastcgi区别:Default:  proxy_cache_key $scheme$proxy_host$request_uri; fastcgi_cache_key没有默认值
    { ngx_string("fastcgi_cache_key"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_fastcgi_cache_key,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
语法：fastcgi_cache_path path [levels=m:n] keys_zone=name:size [inactive=time] [max_size=size]
默认值：none
使用字段：http
clean_time参数在0.7.45版本中已经移除。
这个指令指定FastCGI缓存的路径以及其他的一些参数，所有的数据以文件的形式存储，缓存的关键字(key)和文件名为代理的url计算出的MD5值。
Level参数设置缓存目录的目录分级以及子目录的数量，例如指令如果设置为：

fastcgi_cache_path  /data/nginx/cache  levels=1:2   keys_zone=one:10m;那么数据文件将存储为：
比如levels 2:2会生成256*256个字目录，keys_zone是这个缓存空间的名字
/data/nginx/cache/c/29/b7f54b2df7773722d382f4809d65029c缓存中的文件首先被写入一个临时文件并且随后被移动到缓存目录的最后位置，0.8.9版本
之后可以将临时文件和缓存文件存储在不同的文件系统，但是需要明白这种移动并不是简单的原子重命名系统调用，而是整个文件的拷贝，所以最好
在fastcgi_temp_path和fastcgi_cache_path的值中使用相同的文件系统。
另外，所有活动的关键字及数据相关信息都存储于共享内存池，这个值的名称和大小通过key_zone参数指定，inactive参数指定了内存中的数据存储时间，默认为10分钟。
max_size参数设置缓存的最大值，一个指定的cache manager进程将周期性的删除旧的缓存数据。
*/ //XXX_cache缓存是先写在xxx_temp_path再移到xxx_cache_path，所以这两个目录最好在同一个分区
//xxx_cache(proxy_cache fastcgi_cache) abc必须xxx_cache_path(proxy_cache_path fastcgi_cache_path) xxx keys_zone=abc:10m;一起，否则在ngx_http_proxy_merge_loc_conf会失败，因为没有为该abc创建ngx_http_file_cache_t
//fastcgi_cache 指令指定了在当前作用域中使用哪个缓存维护缓存条目，参数对应的缓存必须事先由 fastcgi_cache_path 指令定义。
//获取该结构ngx_http_upstream_cache_get，实际上是通过proxy_cache xxx或者fastcgi_cache xxx来获取共享内存块名的，因此必须设置proxy_cache或者fastcgi_cache

/*
非缓存方式(p->cacheable=0)p->temp_file->path = u->conf->temp_path; 由ngx_http_fastcgi_temp_path指定路径
缓存方式(p->cacheable=1) p->temp_file->path = r->cache->file_cache->temp_path;见proxy_cache_path或者fastcgi_cache_path use_temp_path=指定路径
见ngx_http_upstream_send_response

当前fastcgi_buffers 和fastcgi_buffer_size配置的空间都已经用完了，则需要把数据写道临时文件中去，参考ngx_event_pipe_read_upstream
*/  //从ngx_http_file_cache_update可以看出，后端数据先写到临时文件后，在写入xxx_cache_path中，见ngx_http_file_cache_update
    { ngx_string("fastcgi_cache_path"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_2MORE,
      ngx_http_file_cache_set_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_main_conf_t, caches),
      &ngx_http_fastcgi_module },

    //XXX_cache_bypass  xx1 xx2设置的xx2不为空或者不为0，则不会从缓存中取，而是直接冲后端读取  但是这些请求的后端响应数据依然可以被 upstream 模块缓存。
    //XXX_no_cache  xx1 xx2设置的xx2不为空或者不为0，则后端回来的数据不会被缓存
    { ngx_string("fastcgi_cache_bypass"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_set_predicate_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_bypass),
      NULL },

/*
语法：fastcgi_no_cache variable [...]
默认值：None
使用字段：http, server, location
确定在何种情况下缓存的应答将不会使用，示例：

fastcgi_no_cache $cookie_nocache  $arg_nocache$arg_comment;
fastcgi_no_cache $http_pragma     $http_authorization;如果为空字符串或者等于0，表达式的值等于false，例如，在上述例子中，如果
在请求中设置了cookie "nocache"，缓存将被绕过。
*/
    //XXX_cache_bypass  xx1 xx2设置的xx2不为空或者不为0，则不会从缓存中取，而是直接冲后端读取
    //XXX_no_cache  xx1 xx2设置的xx2不为空或者不为0，则后端回来的数据不会被缓存
    { ngx_string("fastcgi_no_cache"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_set_predicate_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.no_cache),
      NULL },

/*
语法：fastcgi_cache_valid [http_error_code|time]
默认值：none
使用字段：http, server, location
为指定的http返回代码指定缓存时间，例如：

fastcgi_cache_valid  200 302  10m;
fastcgi_cache_valid  404      1m;将响应状态码为200和302缓存10分钟，404缓存1分钟。
默认情况下缓存只处理200，301，302的状态。
同样也可以在指令中使用any表示任何一个。

fastcgi_cache_valid  200 302 10m;
fastcgi_cache_valid  301 1h;
fastcgi_cache_valid  any 1m;
*/
    /*
       注意open_file_cache inactive=20s和fastcgi_cache_valid 20s的区别，前者指的是如果客户端在20s内没有请求到来，则会把该缓存文件对应的stat属性信息
       从ngx_open_file_cache_t->rbtree(expire_queue)中删除(客户端第一次请求该uri对应的缓存文件的时候会把该文件对应的stat信息节点ngx_cached_open_file_s添加到
       ngx_open_file_cache_t->rbtree(expire_queue)中)，从而提高获取缓存文件的效率
       fastcgi_cache_valid指的是何时缓存文件过期，过期则删除，定时执行ngx_cache_manager_process_handler->ngx_http_file_cache_manager
    */ //进程退出后缓存文件会被全部清楚，即使没有到期

    { ngx_string("fastcgi_cache_valid"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_file_cache_valid_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_valid),
      NULL },
/*
语法：fastcgi_cache_min_uses n
默认值：fastcgi_cache_min_uses 1
使用字段：http, server, location
指令指定了经过多少次请求的相同URL将被缓存。
*/
    { ngx_string("fastcgi_cache_min_uses"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_min_uses),
      NULL },

/*
语法：proxy_cache_use_stale [error|timeout|updating|invalid_header|http_500|http_502|http_503|http_504|http_404|off] […];
默认值：proxy_cache_use_stale off;
使用字段：http, server, location
这个指令告诉nginx何时从代理缓存中提供一个过期的响应，参数类似于proxy_next_upstream指令。
为了防止缓存失效（在多个线程同时更新本地缓存时），你可以指定'updating'参数，它将保证只有一个线程去更新缓存，并且在这个
线程更新缓存的过程中其他的线程只会响应当前缓存中的过期版本。
*/
/*
例如如果设置了fastcgi_cache_use_stale updating，表示说虽然该缓存文件失效了，已经有其他客户端请求在获取后端数据，但是该客户端请求现在还没有获取完整，
这时候就可以把以前过期的缓存发送给当前请求的客户端 //可以配合ngx_http_upstream_cache阅读
*/
    { ngx_string("fastcgi_cache_use_stale"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_use_stale),
      &ngx_http_fastcgi_next_upstream_masks },

/*
语法：fastcgi_cache_methods [GET HEAD POST];
默认值：fastcgi_cache_methods GET HEAD;
使用字段：main,http,location
无法禁用GET/HEAD ，即使你只是这样设置：
fastcgi_cache_methods  POST;
*/
    { ngx_string("fastcgi_cache_methods"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_methods),
      &ngx_http_upstream_cache_method_mask },

/*
这个主要解决一个问题:
假设现在又两个客户端，一个客户端正在获取后端数据，并且后端返回了一部分，则nginx会缓存这一部分，并且等待所有后端数据返回继续缓存。
但是在缓存的过程中如果客户端2页来想后端去同样的数据uri等都一样，则会去到客户端缓存一半的数据，这时候就可以通过该配置来解决这个问题，
也就是客户端1还没缓存完全部数据的过程中客户端2只有等客户端1获取完全部后端数据，或者获取到proxy_cache_lock_timeout超时，则客户端2只有从后端获取数据
*/
    { ngx_string("fastcgi_cache_lock"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_lock),
      NULL },

    { ngx_string("fastcgi_cache_lock_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_lock_timeout),
      NULL },

    { ngx_string("fastcgi_cache_lock_age"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_lock_age),
      NULL },

    { ngx_string("fastcgi_cache_revalidate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.cache_revalidate),
      NULL },

#endif
        /*
    默认情况下p->temp_file->path = u->conf->temp_path; 也就是由ngx_http_fastcgi_temp_path指定路径，但是如果是缓存方式(p->cacheable=1)并且配置
    proxy_cache_path(fastcgi_cache_path) /a/b的时候带有use_temp_path=off(表示不使用ngx_http_fastcgi_temp_path配置的path)，
    则p->temp_file->path = r->cache->file_cache->temp_path; 也就是临时文件/a/b/temp。use_temp_path=off表示不使用ngx_http_fastcgi_temp_path
    配置的路径，而使用指定的临时路径/a/b/temp   见ngx_http_upstream_send_response
    */
/*后端数据读取完毕，并且全部写入临时文件后才会执行rename过程，为什么需要临时文件的原因是:例如之前的缓存过期了，现在有个请求正在从后端
获取数据写入临时文件，如果是直接写入缓存文件，则在获取后端数据过程中，如果在来一个客户端请求，如果允许proxy_cache_use_stale updating，则
后面的请求可以直接获取之前老旧的过期缓存，从而可以避免冲突(前面的请求写文件，后面的请求获取文件内容)
*/
    ////XXX_cache缓存是先写在xxx_temp_path再移到xxx_cache_path，所以这两个目录最好在同一个分区
    { ngx_string("fastcgi_temp_path"), //从ngx_http_file_cache_update可以看出，后端数据先写到临时文件后，在写入xxx_cache_path中，见ngx_http_file_cache_update
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1234,
      ngx_conf_set_path_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.temp_path),
      NULL },

/*
在buffering标志位为1时，如果上游速度快于下游速度，将有可能把来自上游的响应存储到临时文件中，而max_temp_file_size指定了临时文件的
最大长度。实际上，它将限制ngx_event_pipe_t结构体中的temp_file
*/
    { ngx_string("fastcgi_max_temp_file_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.max_temp_file_size_conf),
      NULL },
//表示将缓冲区中的响应写入临时文件时一次写入字符流的最大长度
    { ngx_string("fastcgi_temp_file_write_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.temp_file_write_size_conf),
      NULL },
/*
语法：fastcgi_next_upstream error|timeout|invalid_header|http_500|http_503|http_404|off
默认值：fastcgi_next_upstream error timeout
使用字段：http, server, location
指令指定哪种情况请求将被转发到下一个FastCGI服务器：
·error — 传送中的请求或者正在读取应答头的请求在连接服务器的时候发生错误。
·timeout — 传送中的请求或者正在读取应答头的请求在连接服务器的时候超时。
·invalid_header — 服务器返回空的或者无效的应答。
·http_500 — 服务器返回500应答代码。
·http_503 — 服务器返回503应答代码。
·http_404 — 服务器返回404应答代码。
·off — 禁止请求传送到下一个FastCGI服务器。
注意传送请求在传送到下一个服务器之前可能已经将空的数据传送到了客户端，所以，如果在数据传送中有错误或者超时发生，这个指令可能
无法修复一些传送错误。
*/
    { ngx_string("fastcgi_next_upstream"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.next_upstream),
      &ngx_http_fastcgi_next_upstream_masks },

    { ngx_string("fastcgi_next_upstream_tries"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.next_upstream_tries),
      NULL },

    { ngx_string("fastcgi_next_upstream_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.next_upstream_timeout),
      NULL },
/*
语法：fastcgi_param parameter value
默认值：none
使用字段：http, server, location
指定一些传递到FastCGI服务器的参数。
可以使用字符串，变量，或者其组合，这里的设置不会继承到其他的字段，设置在当前字段会清除掉任何之前的定义。
下面是一个PHP需要使用的最少参数：

  fastcgi_param  SCRIPT_FILENAME  /home/www/scripts/php$fastcgi_script_name;
  fastcgi_param  QUERY_STRING     $query_string;
  PHP使用SCRIPT_FILENAME参数决定需要执行哪个脚本，QUERY_STRING包含请求中的某些参数。
如果要处理POST请求，则需要另外增加三个参数：

  fastcgi_param  REQUEST_METHOD   $request_method;
  fastcgi_param  CONTENT_TYPE     $content_type;
  fastcgi_param  CONTENT_LENGTH   $content_length;如果PHP在编译时带有--enable-force-cgi-redirect，则必须传递值为200的REDIRECT_STATUS参数：

fastcgi_param  REDIRECT_STATUS  200;
*/
    { ngx_string("fastcgi_param"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE23,
      ngx_http_upstream_param_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, params_source),
      NULL },

    { ngx_string("fastcgi_pass_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.pass_headers),
      NULL },
/*
语法：fastcgi_hide_header name
使用字段：http, server, location
默认情况下nginx不会将来自FastCGI服务器的"Status"和"X-Accel-..."头传送到客户端，这个参数也可以隐藏某些其它的头。
如果必须传递"Status"和"X-Accel-..."头，则必须使用fastcgi_pass_header强制其传送到客户端。
*/
    { ngx_string("fastcgi_hide_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.hide_headers),
      NULL },
/*
语法：fastcgi_ignore_headers name [name...]
使用字段：http, server, location
这个指令禁止处理一些FastCGI服务器应答的头部字段，比如可以指定像"X-Accel-Redirect", "X-Accel-Expires", "Expires"或"Cache-Control"等。
If not disabled, processing of these header fields has the following effect:

“X-Accel-Expires”, “Expires”, “Cache-Control”, “Set-Cookie”, and “Vary” set the parameters of response caching;
“X-Accel-Redirect” performs an internal redirect to the specified URI;
“X-Accel-Limit-Rate” sets the rate limit for transmission of a response to a client;
“X-Accel-Buffering” enables or disables buffering of a response;
“X-Accel-Charset” sets the desired charset of a response.
*/
    { ngx_string("fastcgi_ignore_headers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, upstream.ignore_headers),
      &ngx_http_upstream_ignore_headers_masks },

/*
Sets a string to search for in the error stream of a response received from a FastCGI server. If the string is found then it is
considered that the FastCGI server has returned an invalid response. This allows handling application errors in nginx, for example:

location /php {
    fastcgi_pass backend:9000;
    ...
    fastcgi_catch_stderr "PHP Fatal error";
    fastcgi_next_upstream error timeout invalid_header;
}
*/
    { ngx_string("fastcgi_catch_stderr"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, catch_stderr),
      NULL },

    { ngx_string("fastcgi_keep_conn"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fastcgi_loc_conf_t, keep_conn),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_fastcgi_module_ctx = {
    ngx_http_fastcgi_add_variables,        /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_fastcgi_create_main_conf,     /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_fastcgi_create_loc_conf,      /* create location configuration */
    ngx_http_fastcgi_merge_loc_conf        /* merge location configuration */
};

/*
step1. web 服务器收到客户端（浏览器）的请求Http Request，启动CGI程序，并通过环境变量、标准输入传递数据
step2. cgi进程启动解析器、加载配置（如业务相关配置）、连接其它服务器（如数据库服务器）、逻辑处理等
step3. cgi程将处理结果通过标准输出、标准错误，传递给web 服务器
step4. web 服务器收到cgi返回的结果，构建Http Response返回给客户端，并杀死cgi进程

http://blog.sina.com.cn/s/blog_4d8cf3140101pa8c.html
FastCGI 的主要优点是把动态语言和HTTP Server分离开来，所以Nginx与PHP/PHP-FPM
经常被部署在不同的服务器上，以分担前端Nginx服务器的压力，使Nginx
专一处理静态请求和转发动态请求，而PHP/PHP-FPM服务器专一解析PHP动态请求。

*/ //http://chenzhenianqing.cn/articles/category/%e5%90%84%e7%a7%8dserver/nginx
ngx_module_t  ngx_http_fastcgi_module = {
    NGX_MODULE_V1,
    &ngx_http_fastcgi_module_ctx,          /* module context */
    ngx_http_fastcgi_commands,             /* module directives */
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


static ngx_http_fastcgi_request_start_t  ngx_http_fastcgi_request_start = { //参考ngx_http_fastcgi_create_request
    //该头部表示nginx开始请求, 请求由FCGI_BEGIN_REQUEST开始
    { 1,                                               /* version */
      NGX_HTTP_FASTCGI_BEGIN_REQUEST,                  /* type */
      0,                                               /* request_id_hi */
      1,                                               /* request_id_lo */
      0,                                               /* content_length_hi */
      sizeof(ngx_http_fastcgi_begin_request_t),        /* content_length_lo */
      0,                                               /* padding_length */
      0 },                                             /* reserved */

    //该头部说明是否和后端采用长连接
    { 0,                                               /* role_hi */
      NGX_HTTP_FASTCGI_RESPONDER,                      /* role_lo */
      0, /* NGX_HTTP_FASTCGI_KEEP_CONN */              /* flags */
      { 0, 0, 0, 0, 0 } },                             /* reserved[5] */

    //params参数头部的前4字节，剩余的全部在参数中一起填充，可以参考ngx_http_fastcgi_create_request
    { 1,                                               /* version */
      NGX_HTTP_FASTCGI_PARAMS,                         /* type */
      0,                                               /* request_id_hi */
      1 },                                             /* request_id_lo */

};


static ngx_http_variable_t  ngx_http_fastcgi_vars[] = {

    { ngx_string("fastcgi_script_name"), NULL,
      ngx_http_fastcgi_script_name_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("fastcgi_path_info"), NULL,
      ngx_http_fastcgi_path_info_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};

//最终添加到了ngx_http_upstream_conf_t->hide_headers_hash表中  不需要发送给客户端
static ngx_str_t  ngx_http_fastcgi_hide_headers[] = {
    ngx_string("Status"),
    ngx_string("X-Accel-Expires"),
    ngx_string("X-Accel-Redirect"),
    ngx_string("X-Accel-Limit-Rate"),
    ngx_string("X-Accel-Buffering"),
    ngx_string("X-Accel-Charset"),
    ngx_null_string
};


#if (NGX_HTTP_CACHE)

static ngx_keyval_t  ngx_http_fastcgi_cache_headers[] = {
    { ngx_string("HTTP_IF_MODIFIED_SINCE"),
      ngx_string("$upstream_cache_last_modified") },
    { ngx_string("HTTP_IF_UNMODIFIED_SINCE"), ngx_string("") },
    { ngx_string("HTTP_IF_NONE_MATCH"), ngx_string("$upstream_cache_etag") },
    { ngx_string("HTTP_IF_MATCH"), ngx_string("") },
    { ngx_string("HTTP_RANGE"), ngx_string("") },
    { ngx_string("HTTP_IF_RANGE"), ngx_string("") },
    { ngx_null_string, ngx_null_string }
};

#endif


static ngx_path_init_t  ngx_http_fastcgi_temp_path = {
    ngx_string(NGX_HTTP_FASTCGI_TEMP_PATH), { 1, 2, 0 }
};

/*
ngx_http_fastcgi_handler函数作为nginx读取请求的header头部后,就会调用ngx_http_core_content_phase进一步调用到这里，可以看到upstream还没有到，
其实upstream是由这些fastcgi模块或者proxy模块使用的。或者说互相使用：fastcgi启动upstream，设置相关的回调，然后upstream会调用这些回调完成功能
*/
static ngx_int_t
ngx_http_fastcgi_handler(ngx_http_request_t *r)
{//FCGI处理入口,ngx_http_core_run_phases里面当做一个内容处理模块调用的。(NGX_HTTP_CONTENT_PHASE阶段执行)，实际赋值在:
//ngx_http_core_find_config_phase里面的ngx_http_update_location_config设置。真正调用该函数的地方是ngx_http_core_content_phase->ngx_http_finalize_request(r, r->content_handler(r));
    ngx_int_t                      rc;
    ngx_http_upstream_t           *u;
    ngx_http_fastcgi_ctx_t        *f;
    ngx_http_fastcgi_loc_conf_t   *flcf;
#if (NGX_HTTP_CACHE)
    ngx_http_fastcgi_main_conf_t  *fmcf;
#endif

    //创建一个ngx_http_upstream_t结构，放到r->upstream里面去。
    if (ngx_http_upstream_create(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    f = ngx_pcalloc(r->pool, sizeof(ngx_http_fastcgi_ctx_t)); //创建fastcgi所属的upstream上下文ngx_http_fastcgi_ctx_t
    if (f == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_http_set_ctx(r, f, ngx_http_fastcgi_module); //除了配置中的参数可以设置ctx外，其他需要的参数也可以通过r->ctx[]数组来设置，从而可以得到保存，只要知道r，就可以通过r->ctx[]获取到

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);//得到fcgi的配置。(r)->loc_conf[module.ctx_index]

    if (flcf->fastcgi_lengths) {//如果这个fcgi有变量，那么久需要解析一下变量。
        if (ngx_http_fastcgi_eval(r, flcf) != NGX_OK) { //计算fastcgi_pass   127.0.0.1:9000;后面的URL的内容。也就是域名解析
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    u = r->upstream; //就是上面ngx_http_upstream_create中创建的

    ngx_str_set(&u->schema, "fastcgi://");
    u->output.tag = (ngx_buf_tag_t) &ngx_http_fastcgi_module;

    u->conf = &flcf->upstream;

#if (NGX_HTTP_CACHE)
    fmcf = ngx_http_get_module_main_conf(r, ngx_http_fastcgi_module);

    u->caches = &fmcf->caches;
    u->create_key = ngx_http_fastcgi_create_key;
#endif

    u->create_request = ngx_http_fastcgi_create_request; //在ngx_http_upstream_init_request中执行
    u->reinit_request = ngx_http_fastcgi_reinit_request; //在ngx_http_upstream_reinit中执行
    u->process_header = ngx_http_fastcgi_process_header; //在ngx_http_upstream_process_header中执行
    u->abort_request = ngx_http_fastcgi_abort_request;
    u->finalize_request = ngx_http_fastcgi_finalize_request; //在ngx_http_upstream_finalize_request中执行
    r->state = 0;

    //下面的数据结构是给event_pipe用的，用来对FCGI的数据进行buffering处理的。
    u->buffering = flcf->upstream.buffering; //默认为1

    u->pipe = ngx_pcalloc(r->pool, sizeof(ngx_event_pipe_t));
    if (u->pipe == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    //设置读取fcgi协议格式数据的回调，当解析完带有\r\n\r\n的头部的FCGI包后，后面的包解析都由这个函数进行处理。
    u->pipe->input_filter = ngx_http_fastcgi_input_filter;
    u->pipe->input_ctx = r;

    u->input_filter_init = ngx_http_fastcgi_input_filter_init;
    u->input_filter = ngx_http_fastcgi_non_buffered_filter;
    u->input_filter_ctx = r;

    if (!flcf->upstream.request_buffering
        && flcf->upstream.pass_request_body)
    { //如果需要透传并且不需要换成包体
        r->request_body_no_buffering = 1;
    }

    //读取请求包体
    rc = ngx_http_read_client_request_body(r, ngx_http_upstream_init); //读取完客户端发送来的包体后执行ngx_http_upstream_init

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }

    return NGX_DONE;
}

//计算fastcgi_pass   127.0.0.1:9000;后面的URL内容，设置到u->resolved上面去
static ngx_int_t
ngx_http_fastcgi_eval(ngx_http_request_t *r, ngx_http_fastcgi_loc_conf_t *flcf)
{
    ngx_url_t             url;
    ngx_http_upstream_t  *u;

    ngx_memzero(&url, sizeof(ngx_url_t));
    //根据lcodes和codes计算目标字符串的内容、目标字符串结果存放在value->data;里面，也就是url.url
    if (ngx_http_script_run(r, &url.url, flcf->fastcgi_lengths->elts, 0,
                            flcf->fastcgi_values->elts)
        == NULL)
    {
        return NGX_ERROR;
    }

    url.no_resolve = 1;

    if (ngx_parse_url(r->pool, &url) != NGX_OK) {//对u参数里面的url,unix,inet6等地址进行简析；
         if (url.err) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "%s in upstream \"%V\"", url.err, &url.url);
        }

        return NGX_ERROR;
    }

    u = r->upstream;

    u->resolved = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_resolved_t));
    if (u->resolved == NULL) {
        return NGX_ERROR;
    }

    if (url.addrs && url.addrs[0].sockaddr) {
        u->resolved->sockaddr = url.addrs[0].sockaddr;
        u->resolved->socklen = url.addrs[0].socklen;
        u->resolved->naddrs = 1;
        u->resolved->host = url.addrs[0].name;

    } else {
        u->resolved->host = url.host;
        u->resolved->port = url.port;
        u->resolved->no_port = url.no_port;
    }

    return NGX_OK;
}


#if (NGX_HTTP_CACHE)

//解析fastcgi_cache_key xxx 参数值到r->cache->keys
static ngx_int_t //ngx_http_upstream_cache中执行
ngx_http_fastcgi_create_key(ngx_http_request_t *r)
{//根据之前在解析scgi_cache_key指令的时候计算出来的复杂表达式结构，存放在flcf->cache_key中的，计算出cache_key。
    ngx_str_t                    *key;
    ngx_http_fastcgi_loc_conf_t  *flcf;

    key = ngx_array_push(&r->cache->keys);
    if (key == NULL) {
        return NGX_ERROR;
    }

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

    //从flcf->cache_key(fastcgi_cache_key配置项)中解析出对应code函数的相关变量字符串，存到 r->cache->keys
    if (ngx_http_complex_value(r, &flcf->cache_key, key) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

#endif

/*
2025/03/22 03:55:55[    ngx_http_core_post_access_phase,  2163]  [debug] 2357#2357: *3 post access phase: 8 (NGX_HTTP_POST_ACCESS_PHASE)
2025/03/22 03:55:55[        ngx_http_core_content_phase,  2485]  [debug] 2357#2357: *3 content phase(content_handler): 9 (NGX_HTTP_CONTENT_PHASE)
2025/03/22 03:55:55[             ngx_http_upstream_init,   617]  [debug] 2357#2357: *3 http init upstream, client timer: 0
2025/03/22 03:55:55[                ngx_epoll_add_event,  1398]  [debug] 2357#2357: *3 epoll add event: fd:3 op:3 ev:80002005
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SCRIPT_FILENAME"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/var/yyz/www"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "/"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/test.php"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SCRIPT_FILENAME: /var/yyz/www//test.php"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "QUERY_STRING" //空变量，没value，也发送了，如果加上if_no_emputy参数就不会发送
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "QUERY_STRING: "
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REQUEST_METHOD"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "GET"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REQUEST_METHOD: GET"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "CONTENT_TYPE"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "CONTENT_TYPE: "
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "CONTENT_LENGTH"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "CONTENT_LENGTH: "
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SCRIPT_NAME"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/test.php"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SCRIPT_NAME: /test.php"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REQUEST_URI"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/test.php"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REQUEST_URI: /test.php"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "DOCUMENT_URI"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/test.php"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "DOCUMENT_URI: /test.php"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "DOCUMENT_ROOT"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "/var/yyz/www"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "DOCUMENT_ROOT: /var/yyz/www"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SERVER_PROTOCOL"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "HTTP/1.1"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SERVER_PROTOCOL: HTTP/1.1"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REQUEST_SCHEME"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "http"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REQUEST_SCHEME: http"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: ""
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "GATEWAY_INTERFACE"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "CGI/1.1"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "GATEWAY_INTERFACE: CGI/1.1"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SERVER_SOFTWARE"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "nginx/"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "1.9.2"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SERVER_SOFTWARE: nginx/1.9.2"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REMOTE_ADDR"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "10.2.13.1"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REMOTE_ADDR: 10.2.13.1"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REMOTE_PORT"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "52365"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REMOTE_PORT: 52365"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SERVER_ADDR"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "10.2.13.167"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SERVER_ADDR: 10.2.13.167"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SERVER_PORT"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "80"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SERVER_PORT: 80"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "SERVER_NAME"
2025/03/22 03:55:55[      ngx_http_script_copy_var_code,   988]  [debug] 2357#2357: *3 http script var: "localhost"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "SERVER_NAME: localhost"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "REDIRECT_STATUS"
2025/03/22 03:55:55[          ngx_http_script_copy_code,   864]  [debug] 2357#2357: *3 http script copy: "200"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1353]  [debug] 2357#2357: *3 fastcgi param: "REDIRECT_STATUS: 200"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_ACCEPT: application/x-ms-application, image/jpeg, application/xaml+xml, image/gif, image/pjpeg, application/x-ms-xbap, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, * / *"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_ACCEPT_LANGUAGE: zh-CN"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_USER_AGENT: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; WOW64; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0; InfoPath.3)"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_ACCEPT_ENCODING: gzip, deflate"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_HOST: 10.2.13.167"
2025/03/22 03:55:55[    ngx_http_fastcgi_create_request,  1426]  [debug] 2357#2357: *3 fastcgi param: "HTTP_CONNECTION: Keep-Alive"
2025/03/22 03:55:55[               ngx_http_cleanup_add,  3986]  [debug] 2357#2357: *3 http cleanup add: 080EE06C
2025/03/22 03:55:55[ngx_http_upstream_get_round_robin_peer,   429]  [debug] 2357#2357: *3 get rr peer, try: 1
2025/03/22 03:55:55[             ngx_event_connect_peer,    32]  [debug] 2357#2357: *3 socket 11
2025/03/22 03:55:55[           ngx_epoll_add_connection,  1483]  [debug] 2357#2357: *3 epoll add connection: fd:11 ev:80002005
2025/03/22 03:55:55[             ngx_event_connect_peer,   125]  [debug] 2357#2357: *3 connect to 127.0.0.1:3666, fd:11 #4
2025/03/22 03:55:55[          ngx_http_upstream_connect,  1520]  [debug] 2357#2357: *3 http upstream connect: -2
2025/03/22 03:55:55[                ngx_event_add_timer,    88]  [debug] 2357#2357: *3 <ngx_http_upstream_connect,  1624>  event timer add: 11: 60000:3125260832
上面发送了很多value为空的变量，加上if_no_emputy可以避免发送空变量
*/
//设置FCGI的各种请求开始，请求头部，HTTP BODY数据部分的拷贝，参数拷贝等。后面基本就可以发送数据了
//存放在u->request_bufs链接表里面。
static ngx_int_t //ngx_http_fastcgi_create_request和ngx_http_fastcgi_init_params配对阅读
ngx_http_fastcgi_create_request(ngx_http_request_t *r) //ngx_http_upstream_init_request中执行该函数
{
    off_t                         file_pos;
    u_char                        ch, *pos, *lowcase_key;
    size_t                        size, len, key_len, val_len, padding,
                                  allocated;
    ngx_uint_t                    i, n, next, hash, skip_empty, header_params;
    ngx_buf_t                    *b;
    ngx_chain_t                  *cl, *body;
    ngx_list_part_t              *part;
    ngx_table_elt_t              *header, **ignored;
    ngx_http_upstream_t          *u;
    ngx_http_script_code_pt       code;
    ngx_http_script_engine_t      e, le;
    ngx_http_fastcgi_header_t    *h;
    ngx_http_fastcgi_params_t    *params; //
    ngx_http_fastcgi_loc_conf_t  *flcf;
    ngx_http_script_len_code_pt   lcode;

    len = 0;
    header_params = 0;
    ignored = NULL;

    u = r->upstream;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

#if (NGX_HTTP_CACHE)
    params = u->cacheable ? &flcf->params_cache : &flcf->params;
#else
    params = &flcf->params; //fastcgi_params设置的变量
#endif

    //和ngx_http_fastcgi_init_params配合阅读 //ngx_http_fastcgi_create_request和ngx_http_fastcgi_init_params配对阅读
    if (params->lengths) { //获取fastcgi_params配置的所有变量长度，也就是所有的fastcgi_params key value；中的key字符串长度，如果有多个配置，则是多个key之和
        ngx_memzero(&le, sizeof(ngx_http_script_engine_t));

        ngx_http_script_flush_no_cacheable_variables(r, params->flushes);
        le.flushed = 1;

        le.ip = params->lengths->elts;
        le.request = r;

        while (*(uintptr_t *) le.ip) { //计算所有的fastcgi_param设置的变量的key和value字符串之和

            ////fastcgi_params设置的变量
            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            key_len = lcode(&le);

            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            skip_empty = lcode(&le);


            //也就是取出fastcgi_param  SCRIPT_FILENAME  xxx;中字符串xxx的字符串长度
            for (val_len = 0; *(uintptr_t *) le.ip; val_len += lcode(&le)) {
                lcode = *(ngx_http_script_len_code_pt *) le.ip;
                //为什么这里解析到一个参数和值后会退出for呢?因为在ngx_http_fastcgi_init_params中在value对应的code后面添加了一个NULL空指针，也就是下面的le.ip += sizeof(uintptr_t);
            }
            le.ip += sizeof(uintptr_t);

            //和ngx_http_fastcgi_init_params  ngx_http_upstream_param_set_slot配合阅读
            if (skip_empty && val_len == 0) { //如果fastcgi_param  SCRIPT_FILENAME  xxx  if_not_empty; 如果xxx解析后是空的，则直接跳过该变量
                continue;
            }

            //fastcgi_param设置的变量的key和value字符串之和
            len += 1 + key_len + ((val_len > 127) ? 4 : 1) + val_len; //((val_len > 127) ? 4 : 1)表示存储val_len字节value字符需要多少个字节来表示该字符长度
        }
    }

    if (flcf->upstream.pass_request_headers) { //计算 request header 的长度

        allocated = 0;
        lowcase_key = NULL;

        if (params->number) {
            n = 0;
            part = &r->headers_in.headers.part;

            while (part) { //客户端请求头部个数和+fastcgi_param HTTP_XX变量个数
                n += part->nelts;
                part = part->next;
            }

            ignored = ngx_palloc(r->pool, n * sizeof(void *)); //创建一个 ignored 数组
            if (ignored == NULL) {
                return NGX_ERROR;
            }
        }

        part = &r->headers_in.headers.part; //取得 headers 的第一个 part数组信息
        header = part->elts; //取得 headers 的第一个 part数组首元素

        for (i = 0; /* void */; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }

                part = part->next; //下一个数组 如果当前 part 处理完毕，继续 next part数组
                header = part->elts;//下一个数组的头部元素位置
                i = 0;
            }

            if (params->number) { //如果有配置fastcgi_param  HTTP_  XXX
                if (allocated < header[i].key.len) {
                    allocated = header[i].key.len + 16; //注意这里比"host"长度多16
                    lowcase_key = ngx_pnalloc(r->pool, allocated);
                    //为浏览器发送过来的每一个请求头的key分配空间，例如为host:www.sina.com中的"host"字符串分配空间
                    if (lowcase_key == NULL) {
                        return NGX_ERROR;
                    }
                }

                hash = 0;


                /* 把 key 转成小写，并计算出最后一个 ch 的 hash 值 */
                for (n = 0; n < header[i].key.len; n++) {
                    ch = header[i].key.data[n];

                    if (ch >= 'A' && ch <= 'Z') {
                        ch |= 0x20;

                    } else if (ch == '-') {
                        ch = '_';
                    }

                    hash = ngx_hash(hash, ch);
                    lowcase_key[n] = ch; //请求头部行中的key转换为小写存入lowcase_key数组
                }

                /*
                    查找这个 header 是否在 ignore 名单之内
                    // yes ，把这个 header数组指针放在 ignore 数组内，后面有用，然后继续处理下一个
                    */ //客户端请求头部行关键字key是否在fastcgi_param  HTTP_xx  XXX，存储HTTP_xx的hash表params->hash中是否能查找到和头部行key一样的
                if (ngx_hash_find(&params->hash, hash, lowcase_key, n)) {
                    ignored[header_params++] = &header[i];
                    //请求头中的key在fastcgi_param HTTP_XX 已经有了HTTP_XX,则把该请求行信息添加到ignored数组中
                    continue;
                }

               // n 的值的计算和下面的其实一样
               // 至于 sizeof 后再减一，是因为只需要附加个 "HTTP" 到 Header 上去，不需要 "_"
                n += sizeof("HTTP_") - 1;

            } else {
                n = sizeof("HTTP_") - 1 + header[i].key.len; //如果请求头key不是HTTP_，会多一个"HTTP_头出来"
            }

            //计算 FASTCGI 报文长度+请求头部key+value长度和了
            len += ((n > 127) ? 4 : 1) + ((header[i].value.len > 127) ? 4 : 1)
                + n + header[i].value.len;
        }
    }

    //到这里已经计算了fastcgi_param设置的变量key+value和请求头key+value(HTTP_xx)所有这些字符串的长度和了(总长度len)

    if (len > 65535) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "fastcgi request record is too big: %uz", len);
        return NGX_ERROR;
    }

    //FASTCGI 协议规定，数据必须 8 bit 对齐
    padding = 8 - len % 8;
    padding = (padding == 8) ? 0 : padding;

    //计算总的所需空间大小
    size = sizeof(ngx_http_fastcgi_header_t) //#1
           + sizeof(ngx_http_fastcgi_begin_request_t) //#2

           + sizeof(ngx_http_fastcgi_header_t) //#3 /* NGX_HTTP_FASTCGI_PARAMS */ //前面这三个实际内容在ngx_http_fastcgi_request_start
           + len + padding  //#4
           + sizeof(ngx_http_fastcgi_header_t) //#5 /* NGX_HTTP_FASTCGI_PARAMS */

           + sizeof(ngx_http_fastcgi_header_t);  //#6 /* NGX_HTTP_FASTCGI_STDIN */


    b = ngx_create_temp_buf(r->pool, size);
    if (b == NULL) {
        return NGX_ERROR;
    }

    cl = ngx_alloc_chain_link(r->pool);// 创建 buffer chain，把刚创建的 buffer 链进去
    if (cl == NULL) {
        return NGX_ERROR;
    }

    cl->buf = b;

    ngx_http_fastcgi_request_start.br.flags =
        flcf->keep_conn ? NGX_HTTP_FASTCGI_KEEP_CONN : 0;
    //    前两个 header 的内容是已经定义好的，这里简单复制过来
    ngx_memcpy(b->pos, &ngx_http_fastcgi_request_start,
               sizeof(ngx_http_fastcgi_request_start_t));//直接拷贝默认的FCGI头部字节，以及参数部分的头部

    //h 跳过标准的ngx_http_fastcgi_request_start请求头部，跳到参数开始头部。也就是NGX_HTTP_FASTCGI_PARAMS部分
    h = (ngx_http_fastcgi_header_t *)
             (b->pos + sizeof(ngx_http_fastcgi_header_t)
                     + sizeof(ngx_http_fastcgi_begin_request_t)); //跳到上面的#3位置头


    //根据参数内容， 填充剩余params参数头部中剩余4字节  和ngx_http_fastcgi_request_start配合阅读
    h->content_length_hi = (u_char) ((len >> 8) & 0xff);
    h->content_length_lo = (u_char) (len & 0xff);
    h->padding_length = (u_char) padding;
    h->reserved = 0;

    //和ngx_http_fastcgi_request_start配合阅读  //现在b->last指向参数部分的开头，跳过第一个参数头部。因为其数据已经设置，如上。
    b->last = b->pos + sizeof(ngx_http_fastcgi_header_t)
                     + sizeof(ngx_http_fastcgi_begin_request_t)
                     + sizeof(ngx_http_fastcgi_header_t); //跳到#4位置

    /* 下面就开始填充params参数 + 客户端"HTTP_xx" 请求行字符串了 */

    if (params->lengths) {//处理FCGI的参数，进行相关的拷贝操作。
        ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

        e.ip = params->values->elts; //这下面就是解析key-value对应的key字符串值和value字符串值
        e.pos = b->last;//FCGI的参数先紧跟b后面追加
        e.request = r;
        e.flushed = 1;

        le.ip = params->lengths->elts;
        //和ngx_http_fastcgi_init_params配合阅读 //ngx_http_fastcgi_create_request和ngx_http_fastcgi_init_params配对阅读
        while (*(uintptr_t *) le.ip) {//获取对应的变量参数字符串
            //为ngx_http_script_copy_len_code，得到脚本长度。 也就是fastcgi_param  SCRIPT_FILENAME  xxx;中字符串SCRIPT_FILENAME字符串
            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            key_len = (u_char) lcode(&le);

            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            skip_empty = lcode(&le);//fastcgi_param  SCRIPT_FILENAME  xxx  if_not_empty;是否有if_not_empty参数，如果有该值为1

            for (val_len = 0; *(uintptr_t *) le.ip; val_len += lcode(&le)) { //也就是取出fastcgi_param  SCRIPT_FILENAME  xxx;中字符串xxx的字符串
                lcode = *(ngx_http_script_len_code_pt *) le.ip;
                //为什么这里解析到一个参数和值后会退出for呢?因为在ngx_http_fastcgi_init_params中在value对应的code后面添加了一个NULL空指针，也就是下面的le.ip += sizeof(uintptr_t);
            }
            le.ip += sizeof(uintptr_t);

            if (skip_empty && val_len == 0) { //如果是设置了if_not_emputy，则该条配置的key value就不会发送给后端
                e.skip = 1; //ngx_http_script_copy_code，没有数据，在该函数中无需拷贝数据

                while (*(uintptr_t *) e.ip) {
                    code = *(ngx_http_script_code_pt *) e.ip;
                    code((ngx_http_script_engine_t *) &e);
                }
                e.ip += sizeof(uintptr_t);

                e.skip = 0;

                continue;
            }

            *e.pos++ = (u_char) key_len; //KEY长度到b中

            //VALUE字符串长度到b中，如果是4字节表示的长度，第一位为1，否则为0，根据该位区分是4字节还是1字节报文内容长度
            if (val_len > 127) {
                *e.pos++ = (u_char) (((val_len >> 24) & 0x7f) | 0x80);
                *e.pos++ = (u_char) ((val_len >> 16) & 0xff);
                *e.pos++ = (u_char) ((val_len >> 8) & 0xff);
                *e.pos++ = (u_char) (val_len & 0xff);

            } else {
                *e.pos++ = (u_char) val_len;
            }

            //这里面的ngx_http_script_copy_code会拷贝脚本引擎解析出的对应的value变量中的值到b中
            while (*(uintptr_t *) e.ip) { //每条配置fastcgi_param  SCRIPT_FILENAME  xxx的value code后面都有一个NULL指针，所以这里每一条value对应的code会里面退出
                code = *(ngx_http_script_code_pt *) e.ip;
                code((ngx_http_script_engine_t *) &e);
            }
            e.ip += sizeof(uintptr_t);

            ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "fastcgi param: \"%*s: %*s\"",
                           key_len, e.pos - (key_len + val_len),
                           val_len, e.pos - val_len);
        }

        b->last = e.pos;
    }
    //处理完所有的fastcgi_param  xxx   xxx;参数

    //添加客户端请求行中的HTTP_XX字符串信息到上面的#4中
    if (flcf->upstream.pass_request_headers) {

        part = &r->headers_in.headers.part;
        header = part->elts;

        for (i = 0; /* void */; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }

                part = part->next;
                header = part->elts;
                i = 0;
            }

            for (n = 0; n < header_params; n++) {
                if (&header[i] == ignored[n]) { //浏览器请求行中的key和fastcgi_param设置的产生key完全一样，因为上面已经拷贝到b中了，所以这里不需要再拷贝
                    goto next;
                }
            }

            key_len = sizeof("HTTP_") - 1 + header[i].key.len; //因为要附加一个HTTP到请求头key中，例如host:xxx;则会变化HTTPhost发送到后端服务器
            if (key_len > 127) {
                *b->last++ = (u_char) (((key_len >> 24) & 0x7f) | 0x80);
                *b->last++ = (u_char) ((key_len >> 16) & 0xff);
                *b->last++ = (u_char) ((key_len >> 8) & 0xff);
                *b->last++ = (u_char) (key_len & 0xff);

            } else {
                *b->last++ = (u_char) key_len;
            }

            val_len = header[i].value.len;
            if (val_len > 127) {
                *b->last++ = (u_char) (((val_len >> 24) & 0x7f) | 0x80);
                *b->last++ = (u_char) ((val_len >> 16) & 0xff);
                *b->last++ = (u_char) ((val_len >> 8) & 0xff);
                *b->last++ = (u_char) (val_len & 0xff);

            } else {
                *b->last++ = (u_char) val_len;
            }

            b->last = ngx_cpymem(b->last, "HTTP_", sizeof("HTTP_") - 1); //在请求头前面加个HTTP字符串

            for (n = 0; n < header[i].key.len; n++) {//把头部行key转成 大写，然后复制到b buffer 中，流入host:www.sina.com则key变为HTTPHOST
                ch = header[i].key.data[n];

                if (ch >= 'a' && ch <= 'z') {
                    ch &= ~0x20;

                } else if (ch == '-') {
                    ch = '_';
                }

                *b->last++ = ch;
            }

            b->last = ngx_copy(b->last, header[i].value.data, val_len); //拷贝host:www.sina.com中的字符串www.sina.com到b中

            ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "fastcgi param: \"%*s: %*s\"",
                           key_len, b->last - (key_len + val_len),
                           val_len, b->last - val_len);
        next:

            continue;
        }
    }


    /* 这上面的fastcgi_param参数和客户端请求头key公用一个cl，客户端包体另外占用一个或者多个cl，他们通过next连接在一起，最终前部连接到u->request_bufs
        所有需要发往后端的数据就在u->request_bufs中了，发送的时候从里面取出来即可*/

    if (padding) { //填充使他8字节对齐
        ngx_memzero(b->last, padding);
        b->last += padding;
    }

    //最后面带一个NGX_HTTP_FASTCGI_PARAMS，且起内容长度为0的头部行，表示内容结束
    h = (ngx_http_fastcgi_header_t *) b->last;
    b->last += sizeof(ngx_http_fastcgi_header_t);

    h->version = 1;
    h->type = NGX_HTTP_FASTCGI_PARAMS;
    h->request_id_hi = 0;
    h->request_id_lo = 1;
    h->content_length_hi = 0;//标记为参数部分的头，且下面的内容为空，表示是结尾。
    h->content_length_lo = 0;
    h->padding_length = 0;
    h->reserved = 0;

    /* 到这里客户端头部行已经处理完毕，考试处理包体了 */

    if (r->request_body_no_buffering) { //没有缓存包体，则直接把头部行按照fastcgi协议格式发送到后端

        u->request_bufs = cl;

        u->output.output_filter = ngx_http_fastcgi_body_output_filter;
        u->output.filter_ctx = r;

    } else if (flcf->upstream.pass_request_body) {
        //客户端请求包体零食用body指向 ngx_http_upstream_init_request中取出的客户端包体结构
        body = u->request_bufs; //这个有数据了吗，有的，在ngx_http_upstream_init_request开头设置的。设置为客户端发送的HTTP BODY
        u->request_bufs = cl; //request_bufs从新指向上面赋值好的头部行和fastcgi_param变量内容的空间

#if (NGX_SUPPRESS_WARN)
        file_pos = 0;
        pos = NULL;
#endif
        /* 这上面的fastcgi_param参数和客户端请求头key公用一个cl，客户端包体另外占用一个或者多个cl，他们通过next连接在一起，最终前部连接到u->request_bufs
                所有需要发往后端的数据就在u->request_bufs中了，发送的时候从里面取出来即可*/

        while (body) {

            if (body->buf->in_file) {//如果在文件里面
                file_pos = body->buf->file_pos;

            } else {
                pos = body->buf->pos;
            }

            next = 0;

            do {
                b = ngx_alloc_buf(r->pool);//申请一块ngx_buf_s元数据结构
                if (b == NULL) {
                    return NGX_ERROR;
                }

                ngx_memcpy(b, body->buf, sizeof(ngx_buf_t));//拷贝元数据

                if (body->buf->in_file) {
                    b->file_pos = file_pos;
                    file_pos += 32 * 1024;//一次32K的大小。

                    if (file_pos >= body->buf->file_last) { //file_pos不能超过文件中内容的总长度
                        file_pos = body->buf->file_last;
                        next = 1; //说明数据一次就可以拷贝完，如果为0，表示文件中缓存的比32K还多，则需要多次循环连接到cl->next中
                    }

                    b->file_last = file_pos;
                    len = (ngx_uint_t) (file_pos - b->file_pos);

                } else {
                    b->pos = pos;
                    b->start = pos;
                    pos += 32 * 1024;

                    if (pos >= body->buf->last) {
                        pos = body->buf->last;
                        next = 1; //
                    }

                    b->last = pos;
                    len = (ngx_uint_t) (pos - b->pos);
                }

                padding = 8 - len % 8;
                padding = (padding == 8) ? 0 : padding;

                h = (ngx_http_fastcgi_header_t *) cl->buf->last;
                cl->buf->last += sizeof(ngx_http_fastcgi_header_t);

                h->version = 1;
                h->type = NGX_HTTP_FASTCGI_STDIN; //发送BODY部分
                h->request_id_hi = 0;
                h->request_id_lo = 1; //NGINX 永远只用了1个。
                h->content_length_hi = (u_char) ((len >> 8) & 0xff);//说明NGINX对于BODY是一块块发送的，不一定是一次发送。
                h->content_length_lo = (u_char) (len & 0xff);
                h->padding_length = (u_char) padding;
                h->reserved = 0;

                cl->next = ngx_alloc_chain_link(r->pool);//申请一个新的链接结构，存放这块BODY，参数啥的存放在第一块BODY部分啦
                if (cl->next == NULL) {
                    return NGX_ERROR;
                }

                cl = cl->next;
                cl->buf = b;//设置这块新的连接结构的数据为刚刚的部分BODY内容。  前面的param变量+客户端请求变量内容 的下一个buf就是该客户端包体

                /* 又重新分配了一个b空间，只存储一个头部和padding字段 */
                b = ngx_create_temp_buf(r->pool,
                                        sizeof(ngx_http_fastcgi_header_t)
                                        + padding);//创建一个新的头部缓冲，存放头部的数据，以及填充字节
                if (b == NULL) {
                    return NGX_ERROR;
                }

                if (padding) {
                    ngx_memzero(b->last, padding);
                    b->last += padding;
                }

                cl->next = ngx_alloc_chain_link(r->pool);
                if (cl->next == NULL) {
                    return NGX_ERROR;
                }

                cl = cl->next;//将这个下一个头部的缓冲区放入链接表。好吧，这个链接表算长的了。
                cl->buf = b;

            } while (!next); //为0，表示包体大于32K，需要多次循环判断
            //下一块BODY数据
            body = body->next;
        }

    } else {//如果不用发送请求的BODY部分。直接使用刚才的链接表就行。不用拷贝BODY了
        u->request_bufs = cl;
    }

    if (!r->request_body_no_buffering) {
        h = (ngx_http_fastcgi_header_t *) cl->buf->last;
        cl->buf->last += sizeof(ngx_http_fastcgi_header_t);

        h->version = 1;
        h->type = NGX_HTTP_FASTCGI_STDIN;//老规矩，一种类型结尾来一个全0的头部。
        h->request_id_hi = 0;
        h->request_id_lo = 1;
        h->content_length_hi = 0;
        h->content_length_lo = 0;
        h->padding_length = 0;
        h->reserved = 0;
    }

    cl->next = NULL;//结尾了、

    return NGX_OK;
}


static ngx_int_t
ngx_http_fastcgi_reinit_request(ngx_http_request_t *r)
{
    ngx_http_fastcgi_ctx_t  *f;

    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    if (f == NULL) {
        return NGX_OK;
    }

    f->state = ngx_http_fastcgi_st_version;
    f->fastcgi_stdout = 0;
    f->large_stderr = 0;

    if (f->split_parts) {
        f->split_parts->nelts = 0;
    }

    r->state = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_fastcgi_body_output_filter(void *data, ngx_chain_t *in)
{
    ngx_http_request_t  *r = data;

    off_t                       file_pos;
    u_char                     *pos, *start;
    size_t                      len, padding;
    ngx_buf_t                  *b;
    ngx_int_t                   rc;
    ngx_uint_t                  next, last;
    ngx_chain_t                *cl, *tl, *out, **ll;
    ngx_http_fastcgi_ctx_t     *f;
    ngx_http_fastcgi_header_t  *h;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "fastcgi output filter");

    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    if (in == NULL) {
        out = in;
        goto out;
    }

    out = NULL;
    ll = &out;

    if (!f->header_sent) {
        /* first buffer contains headers, pass it unmodified */

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "fastcgi output header");

        f->header_sent = 1;

        tl = ngx_alloc_chain_link(r->pool);
        if (tl == NULL) {
            return NGX_ERROR;
        }

        tl->buf = in->buf;
        *ll = tl;
        ll = &tl->next;

        in = in->next;

        if (in == NULL) {
            tl->next = NULL;
            goto out;
        }
    }

    cl = ngx_chain_get_free_buf(r->pool, &f->free);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    b = cl->buf;

    b->tag = (ngx_buf_tag_t) &ngx_http_fastcgi_body_output_filter;
    b->temporary = 1;

    if (b->start == NULL) {
        /* reserve space for maximum possible padding, 7 bytes */

        b->start = ngx_palloc(r->pool,
                              sizeof(ngx_http_fastcgi_header_t) + 7);
        if (b->start == NULL) {
            return NGX_ERROR;
        }

        b->pos = b->start;
        b->last = b->start;

        b->end = b->start + sizeof(ngx_http_fastcgi_header_t) + 7;
    }

    *ll = cl;

    last = 0;
    padding = 0;

#if (NGX_SUPPRESS_WARN)
    file_pos = 0;
    pos = NULL;
#endif

    while (in) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
                       "fastcgi output in  l:%d f:%d %p, pos %p, size: %z "
                       "file: %O, size: %O",
                       in->buf->last_buf,
                       in->buf->in_file,
                       in->buf->start, in->buf->pos,
                       in->buf->last - in->buf->pos,
                       in->buf->file_pos,
                       in->buf->file_last - in->buf->file_pos);

        if (in->buf->last_buf) {
            last = 1;
        }

        if (ngx_buf_special(in->buf)) {
            in = in->next;
            continue;
        }

        if (in->buf->in_file) {
            file_pos = in->buf->file_pos;

        } else {
            pos = in->buf->pos;
        }

        next = 0;

        do {
            tl = ngx_chain_get_free_buf(r->pool, &f->free);
            if (tl == NULL) {
                return NGX_ERROR;
            }

            b = tl->buf;
            start = b->start;

            ngx_memcpy(b, in->buf, sizeof(ngx_buf_t));

            /*
             * restore b->start to preserve memory allocated in the buffer,
             * to reuse it later for headers and padding
             */

            b->start = start;

            if (in->buf->in_file) {
                b->file_pos = file_pos;
                file_pos += 32 * 1024;

                if (file_pos >= in->buf->file_last) {
                    file_pos = in->buf->file_last;
                    next = 1;
                }

                b->file_last = file_pos;
                len = (ngx_uint_t) (file_pos - b->file_pos);

            } else {
                b->pos = pos;
                pos += 32 * 1024;

                if (pos >= in->buf->last) {
                    pos = in->buf->last;
                    next = 1;
                }

                b->last = pos;
                len = (ngx_uint_t) (pos - b->pos);
            }

            b->tag = (ngx_buf_tag_t) &ngx_http_fastcgi_body_output_filter;
            b->shadow = in->buf;
            b->last_shadow = next;

            b->last_buf = 0;
            b->last_in_chain = 0;

            padding = 8 - len % 8;
            padding = (padding == 8) ? 0 : padding;

            h = (ngx_http_fastcgi_header_t *) cl->buf->last;
            cl->buf->last += sizeof(ngx_http_fastcgi_header_t);

            h->version = 1;
            h->type = NGX_HTTP_FASTCGI_STDIN;
            h->request_id_hi = 0;
            h->request_id_lo = 1;
            h->content_length_hi = (u_char) ((len >> 8) & 0xff);
            h->content_length_lo = (u_char) (len & 0xff);
            h->padding_length = (u_char) padding;
            h->reserved = 0;

            cl->next = tl;
            cl = tl;

            tl = ngx_chain_get_free_buf(r->pool, &f->free);
            if (tl == NULL) {
                return NGX_ERROR;
            }

            b = tl->buf;

            b->tag = (ngx_buf_tag_t) &ngx_http_fastcgi_body_output_filter;
            b->temporary = 1;

            if (b->start == NULL) {
                /* reserve space for maximum possible padding, 7 bytes */

                b->start = ngx_palloc(r->pool,
                                      sizeof(ngx_http_fastcgi_header_t) + 7);
                if (b->start == NULL) {
                    return NGX_ERROR;
                }

                b->pos = b->start;
                b->last = b->start;

                b->end = b->start + sizeof(ngx_http_fastcgi_header_t) + 7;
            }

            if (padding) {
                ngx_memzero(b->last, padding);
                b->last += padding;
            }

            cl->next = tl;
            cl = tl;

        } while (!next);

        in = in->next;
    }

    if (last) {
        h = (ngx_http_fastcgi_header_t *) cl->buf->last;
        cl->buf->last += sizeof(ngx_http_fastcgi_header_t);

        h->version = 1;
        h->type = NGX_HTTP_FASTCGI_STDIN;
        h->request_id_hi = 0;
        h->request_id_lo = 1;
        h->content_length_hi = 0;
        h->content_length_lo = 0;
        h->padding_length = 0;
        h->reserved = 0;

        cl->buf->last_buf = 1;

    } else if (padding == 0) {
        /* TODO: do not allocate buffers instead */
        cl->buf->temporary = 0;
        cl->buf->sync = 1;
    }

    cl->next = NULL;

out:

#if (NGX_DEBUG)

    for (cl = out; cl; cl = cl->next) {
        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, r->connection->log, 0,
                       "fastcgi output out l:%d f:%d %p, pos %p, size: %z "
                       "file: %O, size: %O",
                       cl->buf->last_buf,
                       cl->buf->in_file,
                       cl->buf->start, cl->buf->pos,
                       cl->buf->last - cl->buf->pos,
                       cl->buf->file_pos,
                       cl->buf->file_last - cl->buf->file_pos);
    }

#endif

    rc = ngx_chain_writer(&r->upstream->writer, out);

    ngx_chain_update_chains(r->pool, &f->free, &f->busy, &out,
                         (ngx_buf_tag_t) &ngx_http_fastcgi_body_output_filter);

    for (cl = f->free; cl; cl = cl->next) {

        /* mark original buffers as sent */

        if (cl->buf->shadow) {
            if (cl->buf->last_shadow) {
                b = cl->buf->shadow;
                b->pos = b->last;
            }

            cl->buf->shadow = NULL;
        }
    }

    return rc;
}

/*
后端发送过来的包体格式
1. 头部行包体+内容包体类型fastcgi格式:8字节fastcgi头部行+ 数据(头部行信息+ 空行 + 实际需要发送的包体内容) + 填充字段
..... 中间可能后端包体比较大，这里会包括多个NGX_HTTP_FASTCGI_STDOUT类型fastcgi标识
2. NGX_HTTP_FASTCGI_END_REQUEST类型fastcgi格式:就只有8字节头部

注意:这两部分内容有可能在一次recv就全部读完，也有可能需要读取多次
参考<深入剖析nginx> P270
*/
//解析从后端服务器读取到的fastcgi头部信息，去掉8字节头部ngx_http_fastcgi_header_t以及头部行数据后面的填充字段后，把实际数据通过u->buffer指向
//在ngx_http_upstream_process_header中执行该函数
static ngx_int_t //读取fastcgi请求行头部用ngx_http_fastcgi_process_header 读取fastcgi包体用ngx_http_fastcgi_input_filter
ngx_http_fastcgi_process_header(ngx_http_request_t *r)
{//解析FCGI的请求返回记录，如果是返回标准输出，则解析其请求的HTTP头部并回调其头部数据的回调。数据部分还没有解析。
//ngx_http_upstream_process_header会每次读取数据后，调用这里。
//请注意这个函数执行完，不一定是所有BODY数据也读取完毕了，可能是包含HTTP HEADER的某个FCGI包读取完毕了，然后进行解析的时候
//ngx_http_parse_header_line函数碰到了\r\n\r\n于是返回NGX_HTTP_PARSE_HEADER_DONE，然后本函数就执行完成。

    u_char                         *p, *msg, *start, *last,
                                   *part_start, *part_end;
    size_t                          size;
    ngx_str_t                      *status_line, *pattern;
    ngx_int_t                       rc, status;
    ngx_buf_t                       buf;
    ngx_uint_t                      i;
    ngx_table_elt_t                *h;
    ngx_http_upstream_t            *u;
    ngx_http_fastcgi_ctx_t         *f;
    ngx_http_upstream_header_t     *hh;
    ngx_http_fastcgi_loc_conf_t    *flcf;
    ngx_http_fastcgi_split_part_t  *part;
    ngx_http_upstream_main_conf_t  *umcf;

    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    u = r->upstream;

    for ( ;; ) { //启用cache的情况下，注意这时候buf的内容实际上已经在ngx_http_upstream_process_header中出去了为缓存如文件中预留的头部内存

        if (f->state < ngx_http_fastcgi_st_data) {//上次的状态都没有读完一个头部,先解析这些头部看看是不是有问题。

            f->pos = u->buffer.pos;
            f->last = u->buffer.last;

            rc = ngx_http_fastcgi_process_record(r, f);

            u->buffer.pos = f->pos;
            u->buffer.last = f->last;

            if (rc == NGX_AGAIN) { //说明头部8字节还没读完，需要继续recv后继续调用ngx_http_fastcgi_process_record解析
                return NGX_AGAIN;
            }

            if (rc == NGX_ERROR) {
                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
            }

            if (f->type != NGX_HTTP_FASTCGI_STDOUT
                && f->type != NGX_HTTP_FASTCGI_STDERR)  //说明来的是NGX_HTTP_FASTCGI_END_REQUEST
                //从ngx_http_fastcgi_process_record解析type可以看出只能为 NGX_HTTP_FASTCGI_STDOUT NGX_HTTP_FASTCGI_STDERR NGX_HTTP_FASTCGI_END_REQUEST
            {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream sent unexpected FastCGI record: %d",
                              f->type);

                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
            }

            if (f->type == NGX_HTTP_FASTCGI_STDOUT && f->length == 0) { //接收到的是携带数据的fastcgi标识，单length雀为0，
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream prematurely closed FastCGI stdout");

                return NGX_HTTP_UPSTREAM_INVALID_HEADER;
            }
        }

        if (f->state == ngx_http_fastcgi_st_padding) { //正常情况下走到这里，表示头部行fastcgi标识中没有解析到真正的包体部分，因此需要再次读取解析包体
        //见最后面的if (f->length == 0) { //头部行解析完毕后，由于没有包体数据，单可能有填充字段
        //实际上如果有padding，只有在后面的STDOUT类型fastcgi 头部行解析完毕后(也就是遇到一个空行)，并且没有包体，也就是f=>length=0，才会执行到这里，如果有padding，这里是最后执行的地方

            if (u->buffer.pos + f->padding < u->buffer.last) {  //说明buffer中的内容也包括padding，则直接跳过padding字段
                f->state = ngx_http_fastcgi_st_version;
                u->buffer.pos += f->padding; //

                continue;  //说明这个fastcgi标识信息后面还有其他fastcgi标识信息
            }

            if (u->buffer.pos + f->padding == u->buffer.last) {
                f->state = ngx_http_fastcgi_st_version;
                u->buffer.pos = u->buffer.last;

                return NGX_AGAIN;
            }

            //说明buffer中padding填充的字段还没有读完，需要再次recv才能读取到padding字段
            f->padding -= u->buffer.last - u->buffer.pos;
            u->buffer.pos = u->buffer.last;

            return NGX_AGAIN;
        }

        //这下面只能是fastcgi信息的NGX_HTTP_FASTCGI_STDOUT或者NGX_HTTP_FASTCGI_STDERR标识信息

        //到这里，表示是一条fastcgi信息的data数据部分了
         /* f->state == ngx_http_fastcgi_st_data */

        if (f->type == NGX_HTTP_FASTCGI_STDERR) {

            if (f->length) {
                msg = u->buffer.pos; //msg指向数据部分pos处

                if (u->buffer.pos + f->length <= u->buffer.last) { //包体中包含完整的data部分
                    u->buffer.pos += f->length; //直接跳过一条padding去处理，把包体后面的填充字段去掉
                    f->length = 0;
                    f->state = ngx_http_fastcgi_st_padding;

                } else {
                    f->length -= u->buffer.last - u->buffer.pos; //计算ngx_http_fastcgi_st_data阶段的数据部分还差多少字节，也就是需要在读取recv多少字节才能把length读完
                    u->buffer.pos = u->buffer.last;
                }

                for (p = u->buffer.pos - 1; msg < p; p--) {//从错误信息的后面往前面扫，直到找到一个部位\r,\n . 空格 的字符为止，也就是过滤后面的这些字符吧。
                //在错误STDERR的数据部分从尾部向前查找 \r \n . 空格字符的位置，例如abc.dd\rkkk，则p指向\r字符串位置
                    if (*p != LF && *p != CR && *p != '.' && *p != ' ') {
                        break;
                    }
                }

                p++; //例如abc.dd\rkkk，则p指向\r字符串位置的下一个位置k

                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "FastCGI sent in stderr: \"%*s\"", p - msg, msg);//例如abc.dd\rkkk，打印结果为abc.dd

                flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

                if (flcf->catch_stderr) {
                    pattern = flcf->catch_stderr->elts;

                    for (i = 0; i < flcf->catch_stderr->nelts; i++) {
                        if (ngx_strnstr(msg, (char *) pattern[i].data,
                                        p - msg) //fastcgi_catch_stderr "XXX";中的xxx和p-msg信息中匹配，则返回invalid，该函数返回后然后请求下一个后端服务器
                            != NULL)
                        {
                            return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                        }
                    }
                }

                if (u->buffer.pos == u->buffer.last) { //说明没有padding填充字段，刚好数据部分解析移动后，pos=last

                    if (!f->fastcgi_stdout) {//在stderr标识信息之前没有收到过stdout标识信息

                        /*
                         * the special handling the large number
                         * of the PHP warnings to not allocate memory
                         */

#if (NGX_HTTP_CACHE)
                        if (r->cache) {
                            u->buffer.pos = u->buffer.start
                                                     + r->cache->header_start;
                        } else {
                            u->buffer.pos = u->buffer.start;
                        }
#else
                        u->buffer.pos = u->buffer.start;
                        //解析完该条fastcgi err信息后，刚好把recv的数据解析完，也就是last=pos,则该buffer可以从新recv了，然后循环在解析
#endif
                        u->buffer.last = u->buffer.pos;
                        f->large_stderr = 1;
                    }

                    return NGX_AGAIN; //应该还没有解析到fastcgi的结束标记信息
                }

            } else { //说明后面还有padding信息
                f->state = ngx_http_fastcgi_st_padding;
            }

            continue;
        }


        /* f->type == NGX_HTTP_FASTCGI_STDOUT */ //头部行包体

#if (NGX_HTTP_CACHE)

        if (f->large_stderr && r->cache) {
            u_char                     *start;
            ssize_t                     len;
            ngx_http_fastcgi_header_t  *fh;

            start = u->buffer.start + r->cache->header_start;

            len = u->buffer.pos - start - 2 * sizeof(ngx_http_fastcgi_header_t);

            /*
             * A tail of large stderr output before HTTP header is placed
             * in a cache file without a FastCGI record header.
             * To workaround it we put a dummy FastCGI record header at the
             * start of the stderr output or update r->cache_header_start,
             * if there is no enough place for the record header.
             */

            if (len >= 0) {
                fh = (ngx_http_fastcgi_header_t *) start;
                fh->version = 1;
                fh->type = NGX_HTTP_FASTCGI_STDERR;
                fh->request_id_hi = 0;
                fh->request_id_lo = 1;
                fh->content_length_hi = (u_char) ((len >> 8) & 0xff);
                fh->content_length_lo = (u_char) (len & 0xff);
                fh->padding_length = 0;
                fh->reserved = 0;

            } else {
                r->cache->header_start += u->buffer.pos - start
                                           - sizeof(ngx_http_fastcgi_header_t);
            }

            f->large_stderr = 0;
        }

#endif

        f->fastcgi_stdout = 1; //说明接收到了fastcgi stdout标识信息

        start = u->buffer.pos;

        if (u->buffer.pos + f->length < u->buffer.last) {

            /*
             * set u->buffer.last to the end of the FastCGI record data
             * for ngx_http_parse_header_line()
             */

            last = u->buffer.last;
            u->buffer.last = u->buffer.pos + f->length; //last指向数据部分的末尾处，因为数据中可能有带padding等，所有过滤掉padding

        } else {
            last = NULL;
        }

        for ( ;; ) { //STDOUT NGX_HTTP_FASTCGI_STDOUT
            //NGX_HTTP_FASTCGI_STDOUT真实数据的头部和尾部，不包括padding
            part_start = u->buffer.pos;
            part_end = u->buffer.last;

            rc = ngx_http_parse_header_line(r, &u->buffer, 1); //解析fastcgi后端服务器回送来的请求行

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http fastcgi parser: %d", rc);

            if (rc == NGX_AGAIN) { //一行请求行没有解析完毕
                break;
            }

            if (rc == NGX_OK) {//解析到了一行请求行数据了。 NGX_HTTP_PARSE_HEADER_DONE表示所有请求行解析完毕，通过两个\r\n确定所有头部行完毕，也就是出现一个空行

                /* a header line has been parsed successfully */

                h = ngx_list_push(&u->headers_in.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }

                //如果之前是一段段头部数据分析的，则现在需要组合在一起，然后再次解析。
                if (f->split_parts && f->split_parts->nelts) {

                    part = f->split_parts->elts;
                    size = u->buffer.pos - part_start;

                    for (i = 0; i < f->split_parts->nelts; i++) {
                        size += part[i].end - part[i].start;
                    }

                    p = ngx_pnalloc(r->pool, size);
                    if (p == NULL) {
                        return NGX_ERROR;
                    }

                    buf.pos = p;

                    for (i = 0; i < f->split_parts->nelts; i++) {
                        p = ngx_cpymem(p, part[i].start,
                                       part[i].end - part[i].start);
                    }

                    p = ngx_cpymem(p, part_start, u->buffer.pos - part_start);

                    buf.last = p;

                    f->split_parts->nelts = 0;

                    rc = ngx_http_parse_header_line(r, &buf, 1);

                    if (rc != NGX_OK) {
                        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                                      "invalid header after joining "
                                      "FastCGI records");
                        return NGX_ERROR;
                    }

                    h->key.len = r->header_name_end - r->header_name_start;
                    h->key.data = r->header_name_start;
                    h->key.data[h->key.len] = '\0';

                    h->value.len = r->header_end - r->header_start;
                    h->value.data = r->header_start;
                    h->value.data[h->value.len] = '\0';

                    h->lowcase_key = ngx_pnalloc(r->pool, h->key.len);
                    if (h->lowcase_key == NULL) {
                        return NGX_ERROR;
                    }

                } else {
                    //把请求行中的key:value保存到u->headers_in.headers中的链表成员中
                    h->key.len = r->header_name_end - r->header_name_start;
                    h->value.len = r->header_end - r->header_start;

                    h->key.data = ngx_pnalloc(r->pool,
                                              h->key.len + 1 + h->value.len + 1
                                              + h->key.len);
                    if (h->key.data == NULL) {
                        return NGX_ERROR;
                    }

                    //上面开辟的空间存储的是:key.data + '\0' + value.data + '\0' + lowcase_key.data
                    h->value.data = h->key.data + h->key.len + 1; //value.data为key.data的末尾加一个'\0'字符的后面一个字符
                    h->lowcase_key = h->key.data + h->key.len + 1
                                     + h->value.len + 1;

                    ngx_memcpy(h->key.data, r->header_name_start, h->key.len); //拷贝key字符串到key.data
                    h->key.data[h->key.len] = '\0';
                    ngx_memcpy(h->value.data, r->header_start, h->value.len); //拷贝value字符串到value.data
                    h->value.data[h->value.len] = '\0';
                }

                h->hash = r->header_hash;

                if (h->key.len == r->lowcase_index) {
                    ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);

                } else {
                    ngx_strlow(h->lowcase_key, h->key.data, h->key.len); //把key.data转换为小写字符存到lowcase_key
                }

                hh = ngx_hash_find(&umcf->headers_in_hash, h->hash,
                                   h->lowcase_key, h->key.len); //通过lowcase_key关键字查找ngx_http_upstream_headers_in中对应的成员

                //请求行中对应的key的字符串为"Status"对应的value为"ttt"，则r->upstream->headers_in.statas.data = "ttt";
                //通过这里的for循环和该handler函数，可以获取到所有包体的内容，并由r->upstream->headers_in中的相关成员指向
                if (hh && hh->handler(r, h, hh->offset) != NGX_OK) { //执行ngx_http_upstream_headers_in中的各个成员的handler函数
                    return NGX_ERROR;
                }

                ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "http fastcgi header: \"%V: %V\"",
                               &h->key, &h->value);

                if (u->buffer.pos < u->buffer.last) {
                    continue;
                }

                /* the end of the FastCGI record */

                break;
            }

            if (rc == NGX_HTTP_PARSE_HEADER_DONE) { //所有的请求行解析完毕，下面只有请求体的body数据了。

                /* a whole header has been parsed successfully */

                ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "http fastcgi header done");

                if (u->headers_in.status) {
                    status_line = &u->headers_in.status->value;

                    status = ngx_atoi(status_line->data, 3);

                    if (status == NGX_ERROR) {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                      "upstream sent invalid status \"%V\"",
                                      status_line);
                        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
                    }

                    u->headers_in.status_n = status;
                    u->headers_in.status_line = *status_line;

                } else if (u->headers_in.location) { //说明上游有返回"location"需要重定向
                    u->headers_in.status_n = 302;
                    ngx_str_set(&u->headers_in.status_line,
                                "302 Moved Temporarily");

                } else {
                    u->headers_in.status_n = 200; //直接返回成功
                    ngx_str_set(&u->headers_in.status_line, "200 OK");
                }

                if (u->state && u->state->status == 0) {
                    u->state->status = u->headers_in.status_n;
                }

                break;
            }

            /* there was error while a header line parsing */

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "upstream sent invalid header");

            return NGX_HTTP_UPSTREAM_INVALID_HEADER;
        }

        if (last) {
            u->buffer.last = last;
        }

        f->length -= u->buffer.pos - start; //把上面的头部行包体长度去掉，剩下的应该就是 包体数据 + padding 填充了

        if (f->length == 0) { //头部行解析完毕后，由于没有包体数据，单可能有填充字段
            f->state = ngx_http_fastcgi_st_padding;
        }

        if (rc == NGX_HTTP_PARSE_HEADER_DONE) { //头部行解析完毕
            return NGX_OK;//结束了，解析头部全部完成。该fastcgi STDOUT类型头部行包体全部解析完毕
        }

        if (rc == NGX_OK) {
            continue;
        }

        /* rc == NGX_AGAIN */

        //说明一个fastcgi的请求行格式包体还没有解析完毕，内核缓冲区中已经没有数据了，需要把剩余的字节再次读取，从新进行解析 因此需要记住上次解析的位置等
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "upstream split a header line in FastCGI records");

        if (f->split_parts == NULL) {
            f->split_parts = ngx_array_create(r->pool, 1,
                                        sizeof(ngx_http_fastcgi_split_part_t));
            if (f->split_parts == NULL) {
                return NGX_ERROR;
            }
        }

        part = ngx_array_push(f->split_parts);
        if (part == NULL) {
            return NGX_ERROR;
        }

        part->start = part_start;//记录开始解析前，头部行包体的pos位置
        part->end = part_end; //记录开始解析前，头部行包体的last位置

        if (u->buffer.pos < u->buffer.last) {
            continue;
        }

        return NGX_AGAIN;
    }
}


static ngx_int_t
ngx_http_fastcgi_input_filter_init(void *data)
{
    ngx_http_request_t           *r = data;
    ngx_http_fastcgi_loc_conf_t  *flcf;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

    r->upstream->pipe->length = flcf->keep_conn ?
                                (off_t) sizeof(ngx_http_fastcgi_header_t) : -1;

    return NGX_OK;
}

/*
    buffering方式，读数据前首先开辟一块大空间，在ngx_event_pipe_read_upstream->ngx_readv_chain中开辟一个ngx_buf_t(buf1)结构指向读到的数据，
然后在读取数据到in链表的时候，在ngx_http_fastcgi_input_filter会重新创建一个ngx_buf_t(buf1)，这里面设置buf1->shadow=buf2->shadow
buf2->shadow=buf1->shadow。同时把buf2添加到p->in中。当通过ngx_http_write_filter发送数据的时候会把p->in中的数据添加到p->out，然后发送，
如果一次没有发送完成，则属于的数据会留在p->out中。当数据通过p->output_filter(p->output_ctx, out)发送后，buf2会被添加到p->free中，
buf1会被添加到free_raw_bufs中，见ngx_event_pipe_write_to_downstream
*/

//buffering方式，为ngx_http_fastcgi_input_filter  非buffering方式为ngx_http_fastcgi_non_buffered_filter
//读取fastcgi请求行头部用ngx_http_fastcgi_process_header 读取fastcgi包体用ngx_http_fastcgi_input_filter
static ngx_int_t //从ngx_event_pipe_read_upstream掉用该函数
ngx_http_fastcgi_input_filter(ngx_event_pipe_t *p, ngx_buf_t *buf)
//主要功能就是解析fastcgi格式包体，解析出包体后，把对应的buf加入到p->in
{//当解析完带有\r\n\r\n的头部的FCGI包后，后面的包解析都由这个函数进行处理。
    u_char                       *m, *msg;
    ngx_int_t                     rc;
    ngx_buf_t                    *b, **prev;
    ngx_chain_t                  *cl;
    ngx_http_request_t           *r;
    ngx_http_fastcgi_ctx_t       *f;
    ngx_http_fastcgi_loc_conf_t  *flcf;

    if (buf->pos == buf->last) {
        return NGX_OK;
    }

    //ngx_http_get_module_ctx存储运行过程中的各种状态(例如读取后端数据，可能需要多次读取)  ngx_http_get_module_loc_conf获取该模块在local{}中的配置信息
    r = p->input_ctx;
    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module); //获取fastcgi模块解析后端数据过程中的各种状态信息，因为可能epoll触发好几次读后端数据
    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

    b = NULL;
    prev = &buf->shadow;

    f->pos = buf->pos;
    f->last = buf->last;

    for ( ;; ) {
        if (f->state < ngx_http_fastcgi_st_data) {//小于ngx_http_fastcgi_st_data状态的比较好处理，读，解析吧。后面就只有data,padding 2个状态了。

            rc = ngx_http_fastcgi_process_record(r, f);//下面简单处理一下FCGI的头部，将信息赋值到f的type,length,padding成员上。

            if (rc == NGX_AGAIN) {
                break;//没数据了，等待读取
            }

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (f->type == NGX_HTTP_FASTCGI_STDOUT && f->length == 0) {//如果协议头表示是标准输出，并且长度为0，那就是说明没有内容
                f->state = ngx_http_fastcgi_st_padding; //又从下一个包头开始，也就是版本号。

                if (!flcf->keep_conn) {
                    p->upstream_done = 1;
                }

                ngx_log_debug0(NGX_LOG_DEBUG_HTTP, p->log, 0,
                               "http fastcgi closed stdout");

                continue;
            }

            if (f->type == NGX_HTTP_FASTCGI_END_REQUEST) {//FCGI发送了关闭连接的请求。

                if (!flcf->keep_conn) {
                    p->upstream_done = 1;
                    break;
                }

                ngx_log_debug2(NGX_LOG_DEBUG_HTTP, p->log, 0,
                               "http fastcgi sent end request, flcf->keep_conn:%d, p->upstream_done:%d",
                                flcf->keep_conn, p->upstream_done);

                continue;
            }
        }


        if (f->state == ngx_http_fastcgi_st_padding) { //下面是读取padding的阶段，

            if (f->type == NGX_HTTP_FASTCGI_END_REQUEST) {

                if (f->pos + f->padding < f->last) {//而正好当前缓冲区后面有足够的padding长度，那就直接用它，然后标记到下一个状态，继续处理吧
                    p->upstream_done = 1;
                    break;
                }

                if (f->pos + f->padding == f->last) {//刚好结束，那就退出循环，完成一块数据的解析。
                    p->upstream_done = 1;
                    r->upstream->keepalive = 1;
                    break;
                }

                f->padding -= f->last - f->pos;

                break;
            }

            if (f->pos + f->padding < f->last) {
                f->state = ngx_http_fastcgi_st_version;
                f->pos += f->padding;

                continue;
            }

            if (f->pos + f->padding == f->last) {
                f->state = ngx_http_fastcgi_st_version;

                break;
            }

            f->padding -= f->last - f->pos;

            break;
        }

        //到这里，就只有读取数据部分了。

        /* f->state == ngx_http_fastcgi_st_data */

        if (f->type == NGX_HTTP_FASTCGI_STDERR) {//这是标准错误输出，nginx会怎么处理呢，打印一条日志就行了。

            if (f->length) {//代表数据长度

                if (f->pos == f->last) {//后面没东西了，还需要下次再读取一点数据才能继续了
                    break;
                }

                msg = f->pos;

                if (f->pos + f->length <= f->last) {//错误信息已经全部读取到了，
                    f->pos += f->length;
                    f->length = 0;
                    f->state = ngx_http_fastcgi_st_padding;//下一步去处理padding

                } else {
                    f->length -= f->last - f->pos;
                    f->pos = f->last;
                }

                for (m = f->pos - 1; msg < m; m--) {//从错误信息的后面往前面扫，直到找到一个部位\r,\n . 空格 的字符为止，也就是过滤后面的这些字符吧。
                    if (*m != LF && *m != CR && *m != '.' && *m != ' ') {
                        break;
                    }
                }

                ngx_log_error(NGX_LOG_ERR, p->log, 0,
                              "FastCGI sent in stderr: \"%*s\"",
                              m + 1 - msg, msg);

            } else {
                f->state = ngx_http_fastcgi_st_padding;
            }

            continue;
        }

        if (f->type == NGX_HTTP_FASTCGI_END_REQUEST) {

            if (f->pos + f->length <= f->last) {
                f->state = ngx_http_fastcgi_st_padding;
                f->pos += f->length;

                continue;
            }

            f->length -= f->last - f->pos;

            break;
        }


        /* f->type == NGX_HTTP_FASTCGI_STDOUT */

        if (f->pos == f->last) {
            break;
        }

        cl = ngx_chain_get_free_buf(p->pool, &p->free);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        b = cl->buf;
        //用这个新的缓存描述结构，指向buf这块内存里面的标准输出数据部分，注意这里并没有拷贝数据，而是用b指向了f->pos也就是buf的某个数据地方。
        ngx_memzero(b, sizeof(ngx_buf_t));

        b->pos = f->pos; //从pos到end  b中的指针和buf中的指针指向相同的内存空间

        b->start = buf->start; //b 跟buf共享一块客户端发送过来的数据。这就是shadow的地方， 类似影子?
        b->end = buf->end; //b 跟buf共享一块客户端发送过来的数据。这就是shadow的地方， 类似影子?
        b->tag = p->tag;
        b->temporary = 1;
        /*
        设置为需要回收的标志，这样在发送数据时，会考虑回收这块内存的。为什么要设置为1呢，那buffer在哪呢在函数开始处，
        prev = &buf->shadow;下面就用buf->shadow指向了这块新分配的b描述结构，其实数据是分开的，只是2个描述结构指向同一个buffer
        */
        b->recycled = 1;

        //注意:在后面也会让b->shadow = buf; 也就是b是buf的影子
        *prev = b; //注意在最前面设置了:prev = &buf->shadow; 也就是buf->shadow=b
        /*
          这里用最开始的buf，也就是客户端接收到数据的那块数据buf的shadow成员，形成一个链表，里面每个元素都是FCGI的一个包的data部分数据。
          */
        prev = &b->shadow; //这个感觉没用????没任何意义

        //下面将当前分析得到的FCGI数据data部分放入p->in的链表里面(加入到链表末尾处)。
        if (p->in) {
            *p->last_in = cl;
        } else {
            p->in = cl;
        }
        p->last_in = &cl->next;//记住最后一块

        //同样，拷贝一下数据块序号。不过这里注意，buf可能包含好几个FCGI协议数据块，
		//那就可能存在多个in里面的b->num等于一个相同的buf->num.
        /* STUB */ b->num = buf->num;

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "input buf #%d %p", b->num, b->pos);

        if (f->pos + f->length <= f->last) {//如果数据足够长，那修改一下f->pos，和f->state从而进入下一个数据包的处理。数据已经放入了p->in了的。
            f->state = ngx_http_fastcgi_st_padding;
            f->pos += f->length;
            b->last = f->pos; //移动last

            continue;//接收这块数据，继续下一块
        }

        //到这里，表示当前读取到的数据还少了，不够一个完整包的，那就用完这一点，然后返回，
		//等待下次event_pipe的时候再次read_upstream来读取一些数据再处理了。
        f->length -= f->last - f->pos;

        b->last = f->last;//移动b->last

        break;

    }

    if (flcf->keep_conn) {

        /* set p->length, minimal amount of data we want to see */

        if (f->state < ngx_http_fastcgi_st_data) {
            p->length = 1;

        } else if (f->state == ngx_http_fastcgi_st_padding) {
            p->length = f->padding;

        } else {
            /* ngx_http_fastcgi_st_data */

            p->length = f->length;
        }
    }

    int upstream_done = p->upstream_done;
    if(upstream_done)
        ngx_log_debugall(p->log, 0, "fastcgi input filter upstream_done:%d", upstream_done);

    if (b) { //刚才已经解析到了数据部分。
        b->shadow = buf; //buf是b的影子，前面有设置buf->shadow=b
        b->last_shadow = 1;

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "input buf %p %z", b->pos, b->last - b->pos); //这时候的b->last - b->pos已经去掉了8字节头部

        return NGX_OK;
    }

    //走到这里一般是外层有分配buf空间，但是却发现buf中没有读取到实际的网页包体内容，因此需要把该buf指向内存放入free_raw_bufs链表中，以备在下次
    //读取后端包体的时候直接从上面取

    /* there is no data record in the buf, add it to free chain */
    //将buf挂入free_raw_bufs头部或者第二个位置，如果第一个位置有数据的话。
    //
    if (ngx_event_pipe_add_free_buf(p, buf) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

/*
后端发送过来的包体格式
1. 头部行包体+内容包体类型fastcgi格式:8字节fastcgi头部行+ 数据(头部行信息+ 空行 + 实际需要发送的包体内容) + 填充字段
..... 中间可能后端包体比较大，这里会包括多个NGX_HTTP_FASTCGI_STDOUT类型fastcgi标识
2. NGX_HTTP_FASTCGI_END_REQUEST类型fastcgi格式:就只有8字节头部

注意:这两部分内容有可能在一次recv就全部读完，也有可能需要读取多次
参考<深入剖析nginx> P270
*/
//data实际上是客户端的请求ngx_http_request_t *r
//buffering方式，为ngx_http_fastcgi_input_filter  非buffering方式为ngx_http_fastcgi_non_buffered_filter
static ngx_int_t //ngx_http_upstream_send_response中执行
ngx_http_fastcgi_non_buffered_filter(void *data, ssize_t bytes) //把后端返回的包体信息添加到u->out_bufs末尾
{
    u_char                  *m, *msg;
    ngx_int_t                rc;
    ngx_buf_t               *b, *buf;
    ngx_chain_t             *cl, **ll;
    ngx_http_request_t      *r;
    ngx_http_upstream_t     *u;
    ngx_http_fastcgi_ctx_t  *f;

    r = data;
    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    u = r->upstream;
    buf = &u->buffer;

    //执行真实的包体部分
    buf->pos = buf->last;
    buf->last += bytes;

    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next) {
        ll = &cl->next; //u->out_bufs指向末尾处
    }

    f->pos = buf->pos;
    f->last = buf->last;

    for ( ;; ) {
        //在接收请求行+包体数据的时候，有可能NGX_HTTP_FASTCGI_END_REQUEST类型fastcgi格式也接收到，因此需要解析
        if (f->state < ngx_http_fastcgi_st_data) {

            rc = ngx_http_fastcgi_process_record(r, f);

            if (rc == NGX_AGAIN) {
                break;
            }

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (f->type == NGX_HTTP_FASTCGI_STDOUT && f->length == 0) {
                f->state = ngx_http_fastcgi_st_padding;

                ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "http fastcgi closed stdout");

                continue;
            }
        }

        if (f->state == ngx_http_fastcgi_st_padding) { //有可能从后面的if (f->pos + f->length <= f->last) 走到这里

            if (f->type == NGX_HTTP_FASTCGI_END_REQUEST) {

                if (f->pos + f->padding < f->last) {
                    u->length = 0;
                    break;
                }

                if (f->pos + f->padding == f->last) {
                    u->length = 0;
                    u->keepalive = 1;
                    break;
                }

                f->padding -= f->last - f->pos;

                break;
            }

            if (f->pos + f->padding < f->last) { //说明padding后面还有其他新的fastcgi标识类型需要解析
                f->state = ngx_http_fastcgi_st_version;
                f->pos += f->padding;

                continue;
            }

            if (f->pos + f->padding == f->last) {
                f->state = ngx_http_fastcgi_st_version;

                break;
            }

            f->padding -= f->last - f->pos;

            break;
        }


        /* f->state == ngx_http_fastcgi_st_data */

        if (f->type == NGX_HTTP_FASTCGI_STDERR) {

            if (f->length) {

                if (f->pos == f->last) {
                    break;
                }

                msg = f->pos;

                if (f->pos + f->length <= f->last) {
                    f->pos += f->length;
                    f->length = 0;
                    f->state = ngx_http_fastcgi_st_padding;

                } else {
                    f->length -= f->last - f->pos;
                    f->pos = f->last;
                }

                for (m = f->pos - 1; msg < m; m--) { //从错误信息的后面往前面扫，直到找到一个部位\r,\n . 空格 的字符为止，也就是过滤后面的这些字符吧。
                    if (*m != LF && *m != CR && *m != '.' && *m != ' ') {
                        break;
                    }
                }
                //就用来打印个日志。没其他的。
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "FastCGI sent in stderr: \"%*s\"",
                              m + 1 - msg, msg);

            } else {
                f->state = ngx_http_fastcgi_st_padding;
            }

            continue;
        }

        if (f->type == NGX_HTTP_FASTCGI_END_REQUEST) {

            if (f->pos + f->length <= f->last) { //说明data + padding数据后面还有新的fastcgi格式包体
                f->state = ngx_http_fastcgi_st_padding;
                f->pos += f->length;

                continue;
            }

            f->length -= f->last - f->pos;

            break;
        }


        /* f->type == NGX_HTTP_FASTCGI_STDOUT */
        //到这里就是标准的输出啦，也就是网页内容。


        if (f->pos == f->last) {
            break;//正好没有数据，返回
        }

        cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs); //从free空闲ngx_buf_t结构中取一个
        if (cl == NULL) {
            return NGX_ERROR;
        }

        //把cl节点添加到u->out_bufs的尾部
        *ll = cl;
        ll = &cl->next;

        b = cl->buf; //通过后面赋值从而指向实际u->buffer中的包体部分

        b->flush = 1;
        b->memory = 1;

        b->pos = f->pos;
        b->tag = u->output.tag;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http fastcgi output buf %p", b->pos);

        if (f->pos + f->length <= f->last) { //把包体部分取出来，用b指向
            f->state = ngx_http_fastcgi_st_padding;
            f->pos += f->length; //f绕过包体
            b->last = f->pos; //包体的末尾

            continue;
        }

        f->length -= f->last - f->pos;
        b->last = f->last;

        break;
    }

    /* provide continuous buffer for subrequests in memory */

    if (r->subrequest_in_memory) {

        cl = u->out_bufs;

        if (cl) {
            buf->pos = cl->buf->pos;
        }

        buf->last = buf->pos;

        for (cl = u->out_bufs; cl; cl = cl->next) {
            ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http fastcgi in memory %p-%p %uz",
                           cl->buf->pos, cl->buf->last, ngx_buf_size(cl->buf));

            if (buf->last == cl->buf->pos) {
                buf->last = cl->buf->last;
                continue;
            }

            buf->last = ngx_movemem(buf->last, cl->buf->pos,
                                    cl->buf->last - cl->buf->pos);

            cl->buf->pos = buf->last - (cl->buf->last - cl->buf->pos);
            cl->buf->last = buf->last;
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_fastcgi_process_record(ngx_http_request_t *r,
    ngx_http_fastcgi_ctx_t *f)
{
    u_char                     ch, *p;
    ngx_http_fastcgi_state_e   state;

    state = f->state;

    for (p = f->pos; p < f->last; p++) {

        ch = *p;

        //这里是把8字节fastcgi协议头部打印出来
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http fastcgi record byte: %02Xd", ch);

        switch (state) {

        case ngx_http_fastcgi_st_version:
            if (ch != 1) { //第一个字节必须是1
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream sent unsupported FastCGI "
                              "protocol version: %d", ch);
                return NGX_ERROR;
            }
            state = ngx_http_fastcgi_st_type;
            break;

        case ngx_http_fastcgi_st_type:
            switch (ch) {
            case NGX_HTTP_FASTCGI_STDOUT:
            case NGX_HTTP_FASTCGI_STDERR:
            case NGX_HTTP_FASTCGI_END_REQUEST:
                 f->type = (ngx_uint_t) ch;
                 break;
            default:
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream sent invalid FastCGI "
                              "record type: %d", ch);
                return NGX_ERROR;

            }
            state = ngx_http_fastcgi_st_request_id_hi;
            break;

        /* we support the single request per connection */

        case ngx_http_fastcgi_st_request_id_hi:
            if (ch != 0) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream sent unexpected FastCGI "
                              "request id high byte: %d", ch);
                return NGX_ERROR;
            }
            state = ngx_http_fastcgi_st_request_id_lo;
            break;

        case ngx_http_fastcgi_st_request_id_lo:
            if (ch != 1) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream sent unexpected FastCGI "
                              "request id low byte: %d", ch);
                return NGX_ERROR;
            }
            state = ngx_http_fastcgi_st_content_length_hi;
            break;

        case ngx_http_fastcgi_st_content_length_hi:
            f->length = ch << 8;
            state = ngx_http_fastcgi_st_content_length_lo;
            break;

        case ngx_http_fastcgi_st_content_length_lo:
            f->length |= (size_t) ch;
            state = ngx_http_fastcgi_st_padding_length;
            break;

        case ngx_http_fastcgi_st_padding_length:
            f->padding = (size_t) ch;
            state = ngx_http_fastcgi_st_reserved;
            break;

        case ngx_http_fastcgi_st_reserved: //8字节头部完毕
            state = ngx_http_fastcgi_st_data;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http fastcgi record length: %z", f->length); //fastcgi格式包体内容长度()

            f->pos = p + 1;
            f->state = state;

            return NGX_OK;

        /* suppress warning */
        case ngx_http_fastcgi_st_data:
        case ngx_http_fastcgi_st_padding:
            break;
        }
    }

    f->state = state;

    return NGX_AGAIN;
}


static void
ngx_http_fastcgi_abort_request(ngx_http_request_t *r)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "abort http fastcgi request");

    return;
}


static void
ngx_http_fastcgi_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "finalize http fastcgi request");

    return;
}


static ngx_int_t
ngx_http_fastcgi_add_variables(ngx_conf_t *cf)
{
   ngx_http_variable_t  *var, *v;

    for (v = ngx_http_fastcgi_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static void *
ngx_http_fastcgi_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_fastcgi_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fastcgi_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

#if (NGX_HTTP_CACHE)
    if (ngx_array_init(&conf->caches, cf->pool, 4,
                       sizeof(ngx_http_file_cache_t *))
        != NGX_OK)
    {
        return NULL;
    }
#endif

    return conf;
}


static void *
ngx_http_fastcgi_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_fastcgi_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fastcgi_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->upstream.bufs.num = 0;
     *     conf->upstream.ignore_headers = 0;
     *     conf->upstream.next_upstream = 0;
     *     conf->upstream.cache_zone = NULL;
     *     conf->upstream.cache_use_stale = 0;
     *     conf->upstream.cache_methods = 0;
     *     conf->upstream.temp_path = NULL;
     *     conf->upstream.hide_headers_hash = { NULL, 0 };
     *     conf->upstream.uri = { 0, NULL };
     *     conf->upstream.location = NULL;
     *     conf->upstream.store_lengths = NULL;
     *     conf->upstream.store_values = NULL;
     *
     *     conf->index.len = { 0, NULL };
     */

    conf->upstream.store = NGX_CONF_UNSET;
    conf->upstream.store_access = NGX_CONF_UNSET_UINT;
    conf->upstream.next_upstream_tries = NGX_CONF_UNSET_UINT;
    conf->upstream.buffering = NGX_CONF_UNSET;
    conf->upstream.request_buffering = NGX_CONF_UNSET;
    conf->upstream.ignore_client_abort = NGX_CONF_UNSET;
    conf->upstream.force_ranges = NGX_CONF_UNSET;

    conf->upstream.local = NGX_CONF_UNSET_PTR;

    conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.send_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.next_upstream_timeout = NGX_CONF_UNSET_MSEC;

    conf->upstream.send_lowat = NGX_CONF_UNSET_SIZE;
    conf->upstream.buffer_size = NGX_CONF_UNSET_SIZE;
    conf->upstream.limit_rate = NGX_CONF_UNSET_SIZE;

    conf->upstream.busy_buffers_size_conf = NGX_CONF_UNSET_SIZE;
    conf->upstream.max_temp_file_size_conf = NGX_CONF_UNSET_SIZE;
    conf->upstream.temp_file_write_size_conf = NGX_CONF_UNSET_SIZE;

    conf->upstream.pass_request_headers = NGX_CONF_UNSET;
    conf->upstream.pass_request_body = NGX_CONF_UNSET;

#if (NGX_HTTP_CACHE)
    conf->upstream.cache = NGX_CONF_UNSET;
    conf->upstream.cache_min_uses = NGX_CONF_UNSET_UINT;
    conf->upstream.cache_bypass = NGX_CONF_UNSET_PTR;
    conf->upstream.no_cache = NGX_CONF_UNSET_PTR;
    conf->upstream.cache_valid = NGX_CONF_UNSET_PTR;
    conf->upstream.cache_lock = NGX_CONF_UNSET;
    conf->upstream.cache_lock_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.cache_lock_age = NGX_CONF_UNSET_MSEC;
    conf->upstream.cache_revalidate = NGX_CONF_UNSET;
#endif

    conf->upstream.hide_headers = NGX_CONF_UNSET_PTR;
    conf->upstream.pass_headers = NGX_CONF_UNSET_PTR;

    conf->upstream.intercept_errors = NGX_CONF_UNSET;

    /* "fastcgi_cyclic_temp_file" is disabled */
    conf->upstream.cyclic_temp_file = 0;

    conf->upstream.change_buffering = 1;

    conf->catch_stderr = NGX_CONF_UNSET_PTR;

    conf->keep_conn = NGX_CONF_UNSET;

    ngx_str_set(&conf->upstream.module, "fastcgi");

    return conf;
}


static char *
ngx_http_fastcgi_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_fastcgi_loc_conf_t *prev = parent;
    ngx_http_fastcgi_loc_conf_t *conf = child;

    size_t                        size;
    ngx_int_t                     rc;
    ngx_hash_init_t               hash;
    ngx_http_core_loc_conf_t     *clcf;

#if (NGX_HTTP_CACHE)

    if (conf->upstream.store > 0) {
        conf->upstream.cache = 0;
    }

    if (conf->upstream.cache > 0) {
        conf->upstream.store = 0;
    }

#endif

    if (conf->upstream.store == NGX_CONF_UNSET) {
        ngx_conf_merge_value(conf->upstream.store,
                              prev->upstream.store, 0);

        conf->upstream.store_lengths = prev->upstream.store_lengths;
        conf->upstream.store_values = prev->upstream.store_values;
    }

    ngx_conf_merge_uint_value(conf->upstream.store_access,
                              prev->upstream.store_access, 0600);

    ngx_conf_merge_uint_value(conf->upstream.next_upstream_tries,
                              prev->upstream.next_upstream_tries, 0);

    ngx_conf_merge_value(conf->upstream.buffering,
                              prev->upstream.buffering, 1);

    ngx_conf_merge_value(conf->upstream.request_buffering,
                              prev->upstream.request_buffering, 1);

    ngx_conf_merge_value(conf->upstream.ignore_client_abort,
                              prev->upstream.ignore_client_abort, 0);

    ngx_conf_merge_value(conf->upstream.force_ranges,
                              prev->upstream.force_ranges, 0);

    ngx_conf_merge_ptr_value(conf->upstream.local,
                              prev->upstream.local, NULL);

    ngx_conf_merge_msec_value(conf->upstream.connect_timeout,
                              prev->upstream.connect_timeout, 60000);

    ngx_conf_merge_msec_value(conf->upstream.send_timeout,
                              prev->upstream.send_timeout, 60000);

    ngx_conf_merge_msec_value(conf->upstream.read_timeout,
                              prev->upstream.read_timeout, 60000);

    ngx_conf_merge_msec_value(conf->upstream.next_upstream_timeout,
                              prev->upstream.next_upstream_timeout, 0);

    ngx_conf_merge_size_value(conf->upstream.send_lowat,
                              prev->upstream.send_lowat, 0);

    ngx_conf_merge_size_value(conf->upstream.buffer_size,
                              prev->upstream.buffer_size,
                              (size_t) ngx_pagesize);

    ngx_conf_merge_size_value(conf->upstream.limit_rate,
                              prev->upstream.limit_rate, 0);


    ngx_conf_merge_bufs_value(conf->upstream.bufs, prev->upstream.bufs,
                              8, ngx_pagesize);

    if (conf->upstream.bufs.num < 2) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "there must be at least 2 \"fastcgi_buffers\"");
        return NGX_CONF_ERROR;
    }


    size = conf->upstream.buffer_size;
    if (size < conf->upstream.bufs.size) {
        size = conf->upstream.bufs.size;
    }


    ngx_conf_merge_size_value(conf->upstream.busy_buffers_size_conf,
                              prev->upstream.busy_buffers_size_conf,
                              NGX_CONF_UNSET_SIZE);

    if (conf->upstream.busy_buffers_size_conf == NGX_CONF_UNSET_SIZE) {
        conf->upstream.busy_buffers_size = 2 * size;
    } else {
        conf->upstream.busy_buffers_size =
                                         conf->upstream.busy_buffers_size_conf;
    }

    if (conf->upstream.busy_buffers_size < size) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
             "\"fastcgi_busy_buffers_size\" must be equal to or greater than "
             "the maximum of the value of \"fastcgi_buffer_size\" and "
             "one of the \"fastcgi_buffers\"");

        return NGX_CONF_ERROR;
    }

    if (conf->upstream.busy_buffers_size
        > (conf->upstream.bufs.num - 1) * conf->upstream.bufs.size)
    {
        size_t buf1 = (size_t)((conf->upstream.bufs.num - 1) * conf->upstream.bufs.size);
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
             "\"fastcgi_busy_buffers_size:%z\" must be less than "
             "the size of all \"fastcgi_buffers\" :%z minus one buffer",
             conf->upstream.busy_buffers_size,
             buf1);

        return NGX_CONF_ERROR;
    }


    ngx_conf_merge_size_value(conf->upstream.temp_file_write_size_conf,
                              prev->upstream.temp_file_write_size_conf,
                              NGX_CONF_UNSET_SIZE);

    if (conf->upstream.temp_file_write_size_conf == NGX_CONF_UNSET_SIZE) {
        conf->upstream.temp_file_write_size = 2 * size;
    } else {
        conf->upstream.temp_file_write_size =
                                      conf->upstream.temp_file_write_size_conf;
    }

    if (conf->upstream.temp_file_write_size < size) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
             "\"fastcgi_temp_file_write_size\" must be equal to or greater "
             "than the maximum of the value of \"fastcgi_buffer_size\" and "
             "one of the \"fastcgi_buffers\"");

        return NGX_CONF_ERROR;
    }


    ngx_conf_merge_size_value(conf->upstream.max_temp_file_size_conf,
                              prev->upstream.max_temp_file_size_conf,
                              NGX_CONF_UNSET_SIZE);

    if (conf->upstream.max_temp_file_size_conf == NGX_CONF_UNSET_SIZE) {
        conf->upstream.max_temp_file_size = 1024 * 1024 * 1024;
    } else {
        conf->upstream.max_temp_file_size =
                                        conf->upstream.max_temp_file_size_conf;
    }

    if (conf->upstream.max_temp_file_size != 0
        && conf->upstream.max_temp_file_size < size)
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
             "\"fastcgi_max_temp_file_size\" must be equal to zero to disable "
             "temporary files usage or must be equal to or greater than "
             "the maximum of the value of \"fastcgi_buffer_size\" and "
             "one of the \"fastcgi_buffers\"");

        return NGX_CONF_ERROR;
    }


    ngx_conf_merge_bitmask_value(conf->upstream.ignore_headers,
                              prev->upstream.ignore_headers,
                              NGX_CONF_BITMASK_SET);


    ngx_conf_merge_bitmask_value(conf->upstream.next_upstream,
                              prev->upstream.next_upstream,
                              (NGX_CONF_BITMASK_SET
                               |NGX_HTTP_UPSTREAM_FT_ERROR
                               |NGX_HTTP_UPSTREAM_FT_TIMEOUT));

    if (conf->upstream.next_upstream & NGX_HTTP_UPSTREAM_FT_OFF) {
        conf->upstream.next_upstream = NGX_CONF_BITMASK_SET
                                       |NGX_HTTP_UPSTREAM_FT_OFF;
    }

    if (ngx_conf_merge_path_value(cf, &conf->upstream.temp_path,
                              prev->upstream.temp_path,
                              &ngx_http_fastcgi_temp_path)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

#if (NGX_HTTP_CACHE)

    if (conf->upstream.cache == NGX_CONF_UNSET) {
        ngx_conf_merge_value(conf->upstream.cache,
                              prev->upstream.cache, 0);

        conf->upstream.cache_zone = prev->upstream.cache_zone;
        conf->upstream.cache_value = prev->upstream.cache_value;
    }

    if (conf->upstream.cache_zone && conf->upstream.cache_zone->data == NULL) {
        ngx_shm_zone_t  *shm_zone;

        shm_zone = conf->upstream.cache_zone;

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"fastcgi_cache\" zone \"%V\" is unknown",
                           &shm_zone->shm.name);

        return NGX_CONF_ERROR;
    }

    ngx_conf_merge_uint_value(conf->upstream.cache_min_uses,
                              prev->upstream.cache_min_uses, 1);

    ngx_conf_merge_bitmask_value(conf->upstream.cache_use_stale,
                              prev->upstream.cache_use_stale,
                              (NGX_CONF_BITMASK_SET
                               |NGX_HTTP_UPSTREAM_FT_OFF));

    if (conf->upstream.cache_use_stale & NGX_HTTP_UPSTREAM_FT_OFF) {
        conf->upstream.cache_use_stale = NGX_CONF_BITMASK_SET
                                         |NGX_HTTP_UPSTREAM_FT_OFF;
    }

    if (conf->upstream.cache_use_stale & NGX_HTTP_UPSTREAM_FT_ERROR) {
        conf->upstream.cache_use_stale |= NGX_HTTP_UPSTREAM_FT_NOLIVE;
    }

    if (conf->upstream.cache_methods == 0) {
        conf->upstream.cache_methods = prev->upstream.cache_methods;
    }

    conf->upstream.cache_methods |= NGX_HTTP_GET|NGX_HTTP_HEAD;

    ngx_conf_merge_ptr_value(conf->upstream.cache_bypass,
                             prev->upstream.cache_bypass, NULL);

    ngx_conf_merge_ptr_value(conf->upstream.no_cache,
                             prev->upstream.no_cache, NULL);

    ngx_conf_merge_ptr_value(conf->upstream.cache_valid,
                             prev->upstream.cache_valid, NULL);

    if (conf->cache_key.value.data == NULL) {
        conf->cache_key = prev->cache_key;
    }

    if (conf->upstream.cache && conf->cache_key.value.data == NULL) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "no \"fastcgi_cache_key\" for \"fastcgi_cache\"");
    }

    ngx_conf_merge_value(conf->upstream.cache_lock,
                              prev->upstream.cache_lock, 0);

    ngx_conf_merge_msec_value(conf->upstream.cache_lock_timeout,
                              prev->upstream.cache_lock_timeout, 5000);

    ngx_conf_merge_msec_value(conf->upstream.cache_lock_age,
                              prev->upstream.cache_lock_age, 5000);

    ngx_conf_merge_value(conf->upstream.cache_revalidate,
                              prev->upstream.cache_revalidate, 0);

#endif

    ngx_conf_merge_value(conf->upstream.pass_request_headers,
                              prev->upstream.pass_request_headers, 1);
    ngx_conf_merge_value(conf->upstream.pass_request_body,
                              prev->upstream.pass_request_body, 1);

    ngx_conf_merge_value(conf->upstream.intercept_errors,
                              prev->upstream.intercept_errors, 0);

    ngx_conf_merge_ptr_value(conf->catch_stderr, prev->catch_stderr, NULL);

    ngx_conf_merge_value(conf->keep_conn, prev->keep_conn, 0);


    ngx_conf_merge_str_value(conf->index, prev->index, "");

    hash.max_size = 512;
    hash.bucket_size = ngx_align(64, ngx_cacheline_size);
    hash.name = "fastcgi_hide_headers_hash";

    if (ngx_http_upstream_hide_headers_hash(cf, &conf->upstream,
             &prev->upstream, ngx_http_fastcgi_hide_headers, &hash)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    if (clcf->noname
        && conf->upstream.upstream == NULL && conf->fastcgi_lengths == NULL)
    {
        conf->upstream.upstream = prev->upstream.upstream;
        conf->fastcgi_lengths = prev->fastcgi_lengths;
        conf->fastcgi_values = prev->fastcgi_values;
    }

    if (clcf->lmt_excpt && clcf->handler == NULL
        && (conf->upstream.upstream || conf->fastcgi_lengths))
    {
        clcf->handler = ngx_http_fastcgi_handler;
    }

#if (NGX_PCRE)
    if (conf->split_regex == NULL) {
        conf->split_regex = prev->split_regex;
        conf->split_name = prev->split_name;
    }
#endif

    if (conf->params_source == NULL) {
        conf->params = prev->params;
#if (NGX_HTTP_CACHE)
        conf->params_cache = prev->params_cache;
#endif
        conf->params_source = prev->params_source;
    }

    rc = ngx_http_fastcgi_init_params(cf, conf, &conf->params, NULL);
    if (rc != NGX_OK) {
        return NGX_CONF_ERROR;
    }

#if (NGX_HTTP_CACHE)

    if (conf->upstream.cache) {
        rc = ngx_http_fastcgi_init_params(cf, conf, &conf->params_cache,
                                          ngx_http_fastcgi_cache_headers);
        if (rc != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

#endif

    return NGX_CONF_OK;
}

//ngx_http_fastcgi_create_request和ngx_http_fastcgi_init_params配对阅读
static ngx_int_t
ngx_http_fastcgi_init_params(ngx_conf_t *cf, ngx_http_fastcgi_loc_conf_t *conf,
    ngx_http_fastcgi_params_t *params, ngx_keyval_t *default_params)
{
    u_char                       *p;
    size_t                        size;
    uintptr_t                    *code;
    ngx_uint_t                    i, nsrc;
    ngx_array_t                   headers_names, params_merged;
    ngx_keyval_t                 *h;
    ngx_hash_key_t               *hk;
    ngx_hash_init_t               hash;
    ngx_http_upstream_param_t    *src, *s;
    ngx_http_script_compile_t     sc;
    ngx_http_script_copy_code_t  *copy;

    if (params->hash.buckets) {
        return NGX_OK;
    }

    if (conf->params_source == NULL && default_params == NULL) {
        params->hash.buckets = (void *) 1;
        return NGX_OK;
    }

    params->lengths = ngx_array_create(cf->pool, 64, 1);
    if (params->lengths == NULL) {
        return NGX_ERROR;
    }

    params->values = ngx_array_create(cf->pool, 512, 1);
    if (params->values == NULL) {
        return NGX_ERROR;
    }

    if (ngx_array_init(&headers_names, cf->temp_pool, 4, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (conf->params_source) {
        src = conf->params_source->elts;
        nsrc = conf->params_source->nelts;

    } else {
        src = NULL;
        nsrc = 0;
    }

    if (default_params) {
        if (ngx_array_init(&params_merged, cf->temp_pool, 4,
                           sizeof(ngx_http_upstream_param_t))
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        for (i = 0; i < nsrc; i++) {

            s = ngx_array_push(&params_merged);
            if (s == NULL) {
                return NGX_ERROR;
            }

            *s = src[i];
        }

        h = default_params;

        while (h->key.len) {

            src = params_merged.elts;
            nsrc = params_merged.nelts;

            for (i = 0; i < nsrc; i++) {
                if (ngx_strcasecmp(h->key.data, src[i].key.data) == 0) {
                    goto next;
                }
            }

            s = ngx_array_push(&params_merged);
            if (s == NULL) {
                return NGX_ERROR;
            }

            s->key = h->key;
            s->value = h->value;
            s->skip_empty = 1;

        next:

            h++;
        }

        src = params_merged.elts;
        nsrc = params_merged.nelts;
    }

    for (i = 0; i < nsrc; i++) {

        if (src[i].key.len > sizeof("HTTP_") - 1
            && ngx_strncmp(src[i].key.data, "HTTP_", sizeof("HTTP_") - 1) == 0)
        {
            hk = ngx_array_push(&headers_names);
            if (hk == NULL) {
                return NGX_ERROR;
            }

            hk->key.len = src[i].key.len - 5;
            hk->key.data = src[i].key.data + 5; //把头部的HTTP_这5个字符去掉，然后拷贝到key->data
            hk->key_hash = ngx_hash_key_lc(hk->key.data, hk->key.len);
            hk->value = (void *) 1;

            if (src[i].value.len == 0) {
                continue;
            }
        }

        ////fastcgi_param  SCRIPT_FILENAME  aaa中变量的SCRIPT_FILENAME的字符串长度长度code
        copy = ngx_array_push_n(params->lengths,
                                sizeof(ngx_http_script_copy_code_t));
        if (copy == NULL) {
            return NGX_ERROR;
        }

        copy->code = (ngx_http_script_code_pt) ngx_http_script_copy_len_code;
        copy->len = src[i].key.len;


        ////fastcgi_param  SCRIPT_FILENAME  aaa  if_not_empty，标识该fastcgi_param配置的变量SCRIPT_FILENAME是否有带if_not_empty参数，创建对应的长度code，
        copy = ngx_array_push_n(params->lengths,
                                sizeof(ngx_http_script_copy_code_t));
        if (copy == NULL) {
            return NGX_ERROR;
        }

        copy->code = (ngx_http_script_code_pt) ngx_http_script_copy_len_code;
        copy->len = src[i].skip_empty; //这1字节表示是否有配置时带上"if_not_empty"


        //fastcgi_param  SCRIPT_FILENAME  aaa字符串SCRIPT_FILENAME(key)对应的SCRIPT_FILENAME字符串code
        size = (sizeof(ngx_http_script_copy_code_t)
                + src[i].key.len + sizeof(uintptr_t) - 1)
               & ~(sizeof(uintptr_t) - 1);

        copy = ngx_array_push_n(params->values, size);
        if (copy == NULL) {
            return NGX_ERROR;
        }

        copy->code = ngx_http_script_copy_code;
        copy->len = src[i].key.len;

        p = (u_char *) copy + sizeof(ngx_http_script_copy_code_t);
        ngx_memcpy(p, src[i].key.data, src[i].key.len);


        //fastcgi_param  SCRIPT_FILENAME  aaa配置中变量对应的aaa值，如果值是变量组成，例如/home/www/scripts/php$fastcgi_script_name
        //则需要使用脚本解析各种变量，这些么就是fastcgi_param  SCRIPT_FILENAME  aaa中字符串aaa对应的字符串拷贝
        ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

        sc.cf = cf;
        sc.source = &src[i].value;
        sc.flushes = &params->flushes;
        sc.lengths = &params->lengths;
        sc.values = &params->values;

        //把上面的conf->params_source[]中的各个成员src对应的code信息添加到params->lengths[]  params->values[]中
        if (ngx_http_script_compile(&sc) != NGX_OK) {
            return NGX_ERROR;
        }

        code = ngx_array_push_n(params->lengths, sizeof(uintptr_t));
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;


        code = ngx_array_push_n(params->values, sizeof(uintptr_t));
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;
    }

    code = ngx_array_push_n(params->lengths, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_ERROR;
    }

    *code = (uintptr_t) NULL;

    params->number = headers_names.nelts;

    hash.hash = &params->hash;//fastcgi_param  HTTP_  XXX;环境中通过fastcgi_param设置的HTTP_xx变量通过hash运算存到该hash表中
    hash.key = ngx_hash_key_lc;
    hash.max_size = 512;
    hash.bucket_size = 64;
    hash.name = "fastcgi_params_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    return ngx_hash_init(&hash, headers_names.elts, headers_names.nelts);
}


static ngx_int_t
ngx_http_fastcgi_script_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                       *p;
    ngx_http_fastcgi_ctx_t       *f;
    ngx_http_fastcgi_loc_conf_t  *flcf;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

    f = ngx_http_fastcgi_split(r, flcf);

    if (f == NULL) {
        return NGX_ERROR;
    }

    if (f->script_name.len == 0
        || f->script_name.data[f->script_name.len - 1] != '/')
    {
        v->len = f->script_name.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = f->script_name.data;

        return NGX_OK;
    }

    v->len = f->script_name.len + flcf->index.len;

    v->data = ngx_pnalloc(r->pool, v->len);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    p = ngx_copy(v->data, f->script_name.data, f->script_name.len);
    ngx_memcpy(p, flcf->index.data, flcf->index.len);

    return NGX_OK;
}


static ngx_int_t
ngx_http_fastcgi_path_info_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_fastcgi_ctx_t       *f;
    ngx_http_fastcgi_loc_conf_t  *flcf;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_fastcgi_module);

    f = ngx_http_fastcgi_split(r, flcf);

    if (f == NULL) {
        return NGX_ERROR;
    }

    v->len = f->path_info.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = f->path_info.data;

    return NGX_OK;
}


static ngx_http_fastcgi_ctx_t *
ngx_http_fastcgi_split(ngx_http_request_t *r, ngx_http_fastcgi_loc_conf_t *flcf)
{
    ngx_http_fastcgi_ctx_t       *f;
#if (NGX_PCRE)
    ngx_int_t                     n;
    int                           captures[(1 + 2) * 3];

    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    if (f == NULL) {
        f = ngx_pcalloc(r->pool, sizeof(ngx_http_fastcgi_ctx_t));
        if (f == NULL) {
            return NULL;
        }

        ngx_http_set_ctx(r, f, ngx_http_fastcgi_module);
    }

    if (f->script_name.len) {
        return f;
    }

    if (flcf->split_regex == NULL) {
        f->script_name = r->uri;
        return f;
    }

    n = ngx_regex_exec(flcf->split_regex, &r->uri, captures, (1 + 2) * 3);

    if (n >= 0) { /* match */
        f->script_name.len = captures[3] - captures[2];
        f->script_name.data = r->uri.data + captures[2];

        f->path_info.len = captures[5] - captures[4];
        f->path_info.data = r->uri.data + captures[4];

        return f;
    }

    if (n == NGX_REGEX_NO_MATCHED) {
        f->script_name = r->uri;
        return f;
    }

    ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                  ngx_regex_exec_n " failed: %i on \"%V\" using \"%V\"",
                  n, &r->uri, &flcf->split_name);
    return NULL;

#else

    f = ngx_http_get_module_ctx(r, ngx_http_fastcgi_module);

    if (f == NULL) {
        f = ngx_pcalloc(r->pool, sizeof(ngx_http_fastcgi_ctx_t));
        if (f == NULL) {
            return NULL;
        }

        ngx_http_set_ctx(r, f, ngx_http_fastcgi_module);
    }

    f->script_name = r->uri;

    return f;

#endif
}

/*
这个ngx_http_fastcgi_handler是在nginx 解析配置的时候，解析到了ngx_string(“fastcgi_pass”),指令的时候会调用ngx_http_fastcgi_pass（）进行指令解析
*/
static char *
ngx_http_fastcgi_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_fastcgi_loc_conf_t *flcf = conf;

    ngx_url_t                   u;
    ngx_str_t                  *value, *url;
    ngx_uint_t                  n;
    ngx_http_core_loc_conf_t   *clcf;
    ngx_http_script_compile_t   sc;

    if (flcf->upstream.upstream || flcf->fastcgi_lengths) {
        return "is duplicate";
    }

    //获取当前的location，即在哪个location配置的"fastcgi_pass"指令
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    /* 设置句柄，会在ngx_http_update_location_config里面设置为content_handle的，从而在content phase中被调用

     //设置loc的handler，这个clcf->handler会在ngx_http_update_location_config()里面赋予r->content_handler，从
     而在NGX_HTTP_CONTENT_PHASE里面调用这个handler，即ngx_http_fastcgi_handler。
     */
    clcf->handler = ngx_http_fastcgi_handler;

    if (clcf->name.data[clcf->name.len - 1] == '/') {
        clcf->auto_redirect = 1;
    }

    value = cf->args->elts;

    url = &value[1];

    n = ngx_http_script_variables_count(url);//以'$'开头的变量有多少

    if (n) {

        ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

        sc.cf = cf;
        sc.source = url;
        sc.lengths = &flcf->fastcgi_lengths;
        sc.values = &flcf->fastcgi_values;
        sc.variables = n;
        sc.complete_lengths = 1;
        sc.complete_values = 1;

        //涉及到变量的参数通过该函数把长度code和value code添加到flcf->fastcgi_lengths和flcf->fastcgi_values中
        if (ngx_http_script_compile(&sc) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        return NGX_CONF_OK;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.no_resolve = 1;

    //当做单个server的upstream加入到upstream里面,和 upstream {}类似
    flcf->upstream.upstream = ngx_http_upstream_add(cf, &u, 0);
    if (flcf->upstream.upstream == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_fastcgi_split_path_info(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
#if (NGX_PCRE)
    ngx_http_fastcgi_loc_conf_t *flcf = conf;

    ngx_str_t            *value;
    ngx_regex_compile_t   rc;
    u_char                errstr[NGX_MAX_CONF_ERRSTR];

    value = cf->args->elts;

    flcf->split_name = value[1];

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = value[1];
    rc.pool = cf->pool;
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    if (ngx_regex_compile(&rc) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V", &rc.err);
        return NGX_CONF_ERROR;
    }

    if (rc.captures != 2) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "pattern \"%V\" must have 2 captures", &value[1]);
        return NGX_CONF_ERROR;
    }

    flcf->split_regex = rc.regex;

    return NGX_CONF_OK;

#else

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%V\" requires PCRE library", &cmd->name);
    return NGX_CONF_ERROR;

#endif
}


static char *
ngx_http_fastcgi_store(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_fastcgi_loc_conf_t *flcf = conf;

    ngx_str_t                  *value;
    ngx_http_script_compile_t   sc;

    if (flcf->upstream.store != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        flcf->upstream.store = 0;
        return NGX_CONF_OK;
    }

#if (NGX_HTTP_CACHE)
    if (flcf->upstream.cache > 0) {
        return "is incompatible with \"fastcgi_cache\"";
    }
#endif

    flcf->upstream.store = 1;

    if (ngx_strcmp(value[1].data, "on") == 0) {
        return NGX_CONF_OK;
    }

    /* include the terminating '\0' into script */
    value[1].len++;

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = &value[1];
    sc.lengths = &flcf->upstream.store_lengths;
    sc.values = &flcf->upstream.store_values;
    sc.variables = ngx_http_script_variables_count(&value[1]);
    sc.complete_lengths = 1;
    sc.complete_values = 1;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


#if (NGX_HTTP_CACHE)

static char *
ngx_http_fastcgi_cache(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_fastcgi_loc_conf_t *flcf = conf;

    ngx_str_t                         *value;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (flcf->upstream.cache != NGX_CONF_UNSET) { //说明已经设置过fastcgi_cache xx了,这里检测到有配置fastcgi_cache，报错重复
        return "is duplicate";
    }

    if (ngx_strcmp(value[1].data, "off") == 0) {
        flcf->upstream.cache = 0;
        return NGX_CONF_OK;
    }

    if (flcf->upstream.store > 0) {
        return "is incompatible with \"fastcgi_store\"";
    }

    flcf->upstream.cache = 1;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cv.lengths != NULL) {

        flcf->upstream.cache_value = ngx_palloc(cf->pool,
                                             sizeof(ngx_http_complex_value_t));
        if (flcf->upstream.cache_value == NULL) {
            return NGX_CONF_ERROR;
        }

        *flcf->upstream.cache_value = cv;

        return NGX_CONF_OK;
    }

    flcf->upstream.cache_zone = ngx_shared_memory_add(cf, &value[1], 0,
                                                      &ngx_http_fastcgi_module);
    if (flcf->upstream.cache_zone == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

//fastcgi_cache_key
static char *
ngx_http_fastcgi_cache_key(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_fastcgi_loc_conf_t *flcf = conf;

    ngx_str_t                         *value;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (flcf->cache_key.value.data) {
        return "is duplicate";
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &flcf->cache_key;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

#endif


static char *
ngx_http_fastcgi_lowat_check(ngx_conf_t *cf, void *post, void *data)
{
#if (NGX_FREEBSD)
    ssize_t *np = data;

    if ((u_long) *np >= ngx_freebsd_net_inet_tcp_sendspace) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"fastcgi_send_lowat\" must be less than %d "
                           "(sysctl net.inet.tcp.sendspace)",
                           ngx_freebsd_net_inet_tcp_sendspace);

        return NGX_CONF_ERROR;
    }

#elif !(NGX_HAVE_SO_SNDLOWAT)
    ssize_t *np = data;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "\"fastcgi_send_lowat\" is not supported, ignored");

    *np = 0;

#endif

    return NGX_CONF_OK;
}
