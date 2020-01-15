
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct ngx_http_header_val_s  ngx_http_header_val_t;

typedef ngx_int_t (*ngx_http_set_header_pt)(ngx_http_request_t *r,
    ngx_http_header_val_t *hv, ngx_str_t *value);


typedef struct { //ngx_http_set_headers�л���
    ngx_str_t                  name;
    ngx_uint_t                 offset;
    ngx_http_set_header_pt     handler;
} ngx_http_set_header_t;


struct ngx_http_header_val_s { //�����ռ�͸�ֵ��ngx_http_headers_add
    ngx_http_complex_value_t   value;//add_header name value�е�value
    ngx_str_t                  key;//add_header name value;�е�name
    //Ĭ��ngx_http_add_header  �����add_header name value;�е�name��ngx_http_set_headers�е���ԣ����ӦhanderΪngx_http_set_headers�е�handler
    ngx_http_set_header_pt     handler; 
    //���add_header name value;�е�name��ngx_http_set_headers�е���ԣ����ӦhanderΪngx_http_set_headers�е�handler
    ngx_uint_t                 offset; //��Ӧngx_http_set_headers�е�offset
    ////add_header name value always;
    ngx_uint_t                 always;  /* unsigned  always:1 */
}; 


typedef enum { //��Ч��ngx_http_headers_expires  ngx_http_parse_expires
    NGX_HTTP_EXPIRES_OFF,  //expire modified off
    NGX_HTTP_EXPIRES_EPOCH, //expire modified epoch
    NGX_HTTP_EXPIRES_MAX,   //expire modified max
    NGX_HTTP_EXPIRES_ACCESS,
    NGX_HTTP_EXPIRES_MODIFIED,
    NGX_HTTP_EXPIRES_DAILY,
    NGX_HTTP_EXPIRES_UNSET
} ngx_http_expires_t;

//expires xx���ô洢����Ϊngx_http_headers_expires�����������Ч����Ϊngx_http_set_expires
typedef struct {//�������͸��ͻ��˵�ͷ����װ��ngx_http_headers_filter
    //expires time��������б���������洢��expires �� expires_time��
    ngx_http_expires_t         expires; //expires time���ͣ���ֵΪngx_http_expires_t  ����ngx_http_set_expires
    time_t                     expires_time; //expires time�е�time����ngx_http_set_expires

    //expires time������б���������洢��expires_value��
    ngx_http_complex_value_t  *expires_value;

    //add_header name value;����          �������͸��ͻ��˵�ͷ����װ��ngx_http_headers_filter
    ngx_array_t               *headers; //��ngx_http_headers_add   ��Ա����ngx_http_header_val_t
} ngx_http_headers_conf_t;


static ngx_int_t ngx_http_set_expires(ngx_http_request_t *r,
    ngx_http_headers_conf_t *conf);
static ngx_int_t ngx_http_parse_expires(ngx_str_t *value,
    ngx_http_expires_t *expires, time_t *expires_time, char **err);
static ngx_int_t ngx_http_add_cache_control(ngx_http_request_t *r,
    ngx_http_header_val_t *hv, ngx_str_t *value);
static ngx_int_t ngx_http_add_header(ngx_http_request_t *r,
    ngx_http_header_val_t *hv, ngx_str_t *value);
static ngx_int_t ngx_http_set_last_modified(ngx_http_request_t *r,
    ngx_http_header_val_t *hv, ngx_str_t *value);
static ngx_int_t ngx_http_set_response_header(ngx_http_request_t *r,
    ngx_http_header_val_t *hv, ngx_str_t *value);

static void *ngx_http_headers_create_conf(ngx_conf_t *cf);
static char *ngx_http_headers_merge_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_headers_filter_init(ngx_conf_t *cf);
static char *ngx_http_headers_expires(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_headers_add(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

//add_header name value;�е�name����Ϊ�����⼸��
static ngx_http_set_header_t  ngx_http_set_headers[] = {

    { ngx_string("Cache-Control"), 0, ngx_http_add_cache_control },
/*
nginx����expire�����Ӧ��ͷ����������⼸��Cache-Control��Last-Modified��expireͷ����,�ͻ����ٴ�������Я��If-Modified-Since(���ݾ���nginx֮ǰ���͵�Last-Modified����)
�������յ����If-Modified-Since������жϣ����ļ��Ƿ��и���(expire���õ�ʱ��)��û����ֱ�ӷ���304 Not modified �ο�http://www.fastcache.com.cn/html/3264072438.html
 */
    { ngx_string("Last-Modified"),
                 offsetof(ngx_http_headers_out_t, last_modified),
                 ngx_http_set_last_modified }, 

    { ngx_string("ETag"),
                 offsetof(ngx_http_headers_out_t, etag),
                 ngx_http_set_response_header },

    { ngx_null_string, 0, NULL }
};

/*
ngx_http_headers_moduleģ���ṩ��������Ҫ��ָ��add_header��expires������� ��Expires�� �� ��Cache-Control�� ͷ�ֶΣ�����Ӧͷ��
���κ����ֶΡ�add_header����������ʾ������ʵ���̨�������ϣ����Ҳ����ͨ��nginxģ��nginx-http-footer-filter�о�ʹ����ʵ�֡�
expiresָ����������������ػ���Ŀ��ơ�
*/

static ngx_command_t  ngx_http_headers_filter_commands[] = {
/*
����վ���в������޸ĵľ�̬���ݣ���ͼƬ��js��css���������ڷ�����������expires����ʱ�䣬������������棬�ﵽ��Ч��С�������������ͷ�����ѹ����Ŀ�ġ� 

��nginx������Ϊ����

location ~ .*\.(gif|jpg|jpeg|png|bmp|swf)$ 
{ 
#����ʱ��Ϊ30�죬 #ͼƬ�ļ�����ô���£����ڿ������һ�㣬 #���Ƶ�����£���������õ�Сһ�㡣 
expires 30d; } 

location ~ .*\.(js|css)$ 
{ expires 10d; }����������expires��web��������Ӧ��Ϣͷ�ֶΣ�����Ӧhttp����ʱ����������ڹ���ʱ��ǰ���������ֱ�Ӵ����������ȡ���ݣ�
�������ٴ����� 

��������ϡ�1��cache-control����cache-control��expires������һ�£�����ָ����ǰ��Դ����Ч�ڣ�����������Ƿ�ֱ�Ӵ����������ȡ���ݻ���
���·����󵽷�����ȡ���ݡ�ֻ����cache-control��ѡ����࣬���ø�ϸ�£����ͬʱ���õĻ��������ȼ�����expires�� 


�﷨: expires [modified] time;
expires epoch | max | off;
Ĭ��ֵ: expires off;
���ö�: http, server, location, if in location
�ڶ���Ӧ����Ϊ200��201��204��206��301��302��303��304����307ͷ�����Ƿ����ԡ�Expires���͡�Cache-Control�������Ӻ��޸Ĳ�����
����ָ��һ�����򸺵�ʱ��ֵ��Expiresͷ�е�ʱ�����Ŀǰʱ���ָ����ָ����ʱ��ĺ�����á�

epoch��ʾ��1970��һ��һ��00:00:01 GMT�ľ���ʱ�䣬maxָ��Expires��ֵΪ2037��12��31��23:59:59��Cache-Control��ֵΪ10 years��
Cache-Controlͷ��������Ԥ���ʱ���ʶָ����
������Ϊ������ʱ��ֵ:Cache-Control: no-cache��
������Ϊ������0��ʱ��ֵ��Cache-Control: max-age = #������#�ĵ�λΪ�룬��ָ����ָ����
����off��ֹ�޸�Ӧ��ͷ�е�"Expires"��"Cache-Control"��

ʵ��һ����ͼƬ��flash�ļ�����������ػ���30��

location ~ .*\.(gif|jpg|jpeg|png|bmp|swf)$
 {
           expires 30d;
 }

ʵ��������js��css�ļ�����������ػ���1Сʱ

location ~ .*\.(js|css)$
 {
            expires 1h;
 }

Expires�Ǹ�һ����Դ�趨һ������ʱ�䣬Ҳ����˵����ȥ�������֤��ֱ��ͨ�����������ȷ���Ƿ���ڼ��ɣ����Բ��������������������ַ����ǳ��ʺ�
�������䶯����Դ������ļ��䶯��Ƶ������Ҫʹ��Expires�����档

syntax:  add_header name value;
default:  ��  
context:  http, server, location
 
Adds the specified field to a response header provided that the response code equals 200, 204, 206, 301, 302, 303, 304, or 307. 
    A value can contain variables. 

syntax:  expires [modified] time;
expires epoch | max | off;
 
default:  expires off;
 
context:  http, server, location
 

Enables or disables adding or modifying the ��Expires�� and ��Cache-Control�� response header fields. A parameter can be a positive 
or negative time. 

A time in the ��Expires�� field is computed as a sum of the current time and time specified in the directive. If the modified parameter 
is used (0.7.0, 0.6.32) then time is computed as a sum of the file��s modification time and time specified in the directive. 

In addition, it is possible to specify a time of the day using the ��@�� prefix (0.7.9, 0.6.34): 

expires @15h30m;


The epoch parameter corresponds to the absolute time ��Thu, 01 Jan 1970 00:00:01 GMT��. The contents of the ��Cache-Control�� field 
depends on the sign of the specified time: 
? time is negative �� ��Cache-Control: no-cache��. 
? time is positive or zero �� ��Cache-Control: max-age=t��, where t is a time specified in the directive, in seconds. 


The max parameter sets ��Expires�� to the value ��Thu, 31 Dec 2037 23:55:55 GMT��, and ��Cache-Control�� to 10 years. 
The off parameter disables adding or modifying the ��Expires�� and ��Cache-Control�� response header fields. 



*/  //�����û��޸�Ӧ��ͷ�е�Cache-Controlͷ���У��Ӷ���������Ի�ȡ�����ļ��Ļ���ʱ�䣬��������ʱ���ڵ��������ٴλ�ȡ���ļ���
//��������֧��expires�����Լ��ж�û�й��ڣ���������ᷢ������nginx�������ʹ�ñ��ػ��档Ҳ������������ݸ�ʱ�����ֱ�ӻ�ȡ������������棬�����Ǵ�nginx���»�ȡ���Ӷ����Ч�� 
//����������֧�֣�Я��������������ǿ���ֱ�ӻ�Ӧ304 Not Modified����ʾû�䶯��������������ж�ֱ��ʹ����������ػ���

    /*
    nginx����expire�����Ӧ��ͷ����������⼸��Cache-Control��Last-Modified��expireͷ����,�ͻ����ٴ�������Я��If-Modified-Since(���ݾ���nginx֮ǰ���͵�Last-Modified����)
    �������յ����If-Modified-Since������жϣ����ļ��Ƿ��и���(expire���õ�ʱ��)��û����ֱ�ӷ���304 Not modified 
    �ο�http://www.fastcache.com.cn/html/3264072438.html
     */
    { ngx_string("expires"), //Ҳ��������Ӧ���Ƿ�Я��ͷ����:Expires: Thu, 01 Dec 2010 16:00:00 GMT����Ϣ
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE12,
      ngx_http_headers_expires,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

/*
add_headerָ��
�﷨: add_header name value;
Ĭ��ֵ: ��
���ö�: http, server, location, if in location
����Ӧ����Ϊ200��201��204��206��301��302��303��304����307����Ӧ����ͷ�ֶ�����������磺
add_header From ttlsa.com


syntax:  add_header name value;
 
default:  ��  
context:  http, server, location

Adds the specified field to a response header provided that the response code equals 200, 204, 206, 301, 302, 303, 304, or 307. A value can contain variables. 
*/
    { ngx_string("add_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE23,
      ngx_http_headers_add,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

      ngx_null_command
};


static ngx_http_module_t  ngx_http_headers_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_headers_filter_init,          /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_headers_create_conf,          /* create location configuration */
    ngx_http_headers_merge_conf            /* merge location configuration */
};

/*
��6-1  Ĭ�ϼ������Nginx��HTTP����ģ��
���������������������������������������ש�������������������������������������������������������������������
��Ĭ�ϼ������Nginx��HTTP����ģ��     ��    ����                                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ڷ���200�ɹ�ʱ������������If-              ��
��                                    ��Modified-Since����If-Unmodified-Sinceͷ��ȡ������������ļ���ʱ   ��
��ngx_http_not_modified_filter_module ��                                                                  ��
��                                    ���䣬�ٷ��������û��ļ�������޸�ʱ�䣬�Դ˾����Ƿ�ֱ�ӷ���304     ��
��                                    �� Not Modified��Ӧ���û�                                           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���������е�Range��Ϣ������Range�е�Ҫ�󷵻��ļ���һ���ָ�      ��
��ngx_http_range_body_filter_module   ��                                                                  ��
��                                    ���û�                                                              ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP�������������û����͵�ngx_chain_t�ṹ��HTTP��         ��
��                                    ���帴�Ƶ��µ�ngx_chain_t�ṹ�У����Ǹ���ָ��ĸ��ƣ�������ʵ��     ��
��ngx_http_copy_filter_module         ��                                                                  ��
��                                    ��HTTP��Ӧ���ݣ���������HTTP����ģ�鴦���ngx_chain_t���͵ĳ�       ��
��                                    ��Ա����ngx_http_copy_filter_moduleģ�鴦���ı���                 ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ������������ͨ���޸�nginx.conf�����ļ����ڷ���      ��
��ngx_http_headers_filter_module      ��                                                                  ��
��                                    �����û�����Ӧ����������HTTPͷ��                                  ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ�������������ִ��configure����ʱ�ᵽ��http_        ��
��ngx_http_userid_filter_module       ��                                                                  ��
��                                    ��userid moduleģ�飬������cookie�ṩ�˼򵥵���֤������           ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���Խ��ı����ͷ��ظ��û�����Ӧ��������nginx��conf�е���������   ��
��ngx_http_charset_filter_module      ��                                                                  ��
��                                    �����б��룬�ٷ��ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ֧��SSI��Server Side Include����������Ƕ�룩���ܣ����ļ����ݰ�  ��
��ngx_http_ssi_filter_module          ��                                                                  ��
��                                    ��������ҳ�в����ظ��û�                                            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTP����������5.5.2����ϸ���ܹ��ù���ģ�顣����Ӧ����     ��
��ngx_http_postpone_filter_module     ��subrequest��������������ʹ�ö��������ͬʱ��ͻ��˷�����Ӧʱ    ��
��                                    ���ܹ�������ν�ġ������ǿ����չ����������˳������Ӧ            ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ���ض���HTTP��Ӧ���壨����ҳ�����ı��ļ�������gzipѹ������      ��
��ngx_http_gzip_filter_module         ��                                                                  ��
��                                    ����ѹ��������ݷ��ظ��û�                                          ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_range_header_filter_module ��  ֧��rangeЭ��                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_chunked_filter_module      ��  ֧��chunk����                                                   ��
�ǩ������������������������������������贈������������������������������������������������������������������
��                                    ��  ����HTTPͷ���������ù���ģ�齫���r->headers out�ṹ��        ��
��                                    ���еĳ�Ա���л�Ϊ���ظ��û���HTTP��Ӧ�ַ�����������Ӧ��(��         ��
��ngx_http_header_filter_module       ��                                                                  ��
��                                    ��HTTP/I.1 200 0K)����Ӧͷ������ͨ������ngx_http_write filter       ��
��                                    �� module����ģ���еĹ��˷���ֱ�ӽ�HTTP��ͷ���͵��ͻ���             ��
�ǩ������������������������������������贈������������������������������������������������������������������
��ngx_http_write_filter_module        ��  ����HTTP������������ģ�鸺����ͻ��˷���HTTP��Ӧ              ��
���������������������������������������ߩ�������������������������������������������������������������������

*/

/*
ngx_http_headers_moduleģ���ṩ��������Ҫ��ָ��add_header��expires������� ��Expires�� �� ��Cache-Control�� ͷ�ֶΣ�����Ӧͷ��
���κ����ֶΡ�add_header����������ʾ������ʵ���̨�������ϣ����Ҳ����ͨ��nginxģ��nginx-http-footer-filter�о�ʹ����ʵ�֡�
expiresָ����������������ػ���Ŀ��ơ�
*/
ngx_module_t  ngx_http_headers_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_headers_filter_module_ctx,   /* module context */
    ngx_http_headers_filter_commands,      /* module directives */
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


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;


static ngx_int_t
ngx_http_headers_filter(ngx_http_request_t *r)
{
    ngx_str_t                 value;
    ngx_uint_t                i, safe_status;
    ngx_http_header_val_t    *h;
    ngx_http_headers_conf_t  *conf;

    conf = ngx_http_get_module_loc_conf(r, ngx_http_headers_filter_module);

    if ((conf->expires == NGX_HTTP_EXPIRES_OFF && conf->headers == NULL)
        || r != r->main) //expires off����û������add_header������Ϊ������
    {
        return ngx_http_next_header_filter(r);
    }

    switch (r->headers_out.status) {

    case NGX_HTTP_OK:
    case NGX_HTTP_CREATED:
    case NGX_HTTP_NO_CONTENT:
    case NGX_HTTP_PARTIAL_CONTENT:
    case NGX_HTTP_MOVED_PERMANENTLY:
    case NGX_HTTP_MOVED_TEMPORARILY:
    case NGX_HTTP_SEE_OTHER:
    case NGX_HTTP_NOT_MODIFIED:
    case NGX_HTTP_TEMPORARY_REDIRECT:
        safe_status = 1;
        break;

    default:
        safe_status = 0;
        break;
    }

    if (conf->expires != NGX_HTTP_EXPIRES_OFF && safe_status) {
        if (ngx_http_set_expires(r, conf) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    if (conf->headers) {
        h = conf->headers->elts;
        for (i = 0; i < conf->headers->nelts; i++) {

            if (!safe_status && !h[i].always) {
                continue;
            }

            if (ngx_http_complex_value(r, &h[i].value, &value) != NGX_OK) {
                return NGX_ERROR;
            }

            if (h[i].handler(r, &h[i], &value) != NGX_OK) {
                return NGX_ERROR;
            }
        }
    }

    return ngx_http_next_header_filter(r);
}

//expires���ý������ᴥ����������r->headers_out.expires r->headers_out.cache_control
static ngx_int_t //expires xx���ô洢����Ϊngx_http_headers_expires�����������Ч����Ϊngx_http_set_expires
ngx_http_set_expires(ngx_http_request_t *r, ngx_http_headers_conf_t *conf)
{
    char                *err;
    size_t               len;
    time_t               now, expires_time, max_age;
    ngx_str_t            value;
    ngx_int_t            rc;
    ngx_uint_t           i;
    ngx_table_elt_t     *e, *cc, **ccp;
    ngx_http_expires_t   expires;

    expires = conf->expires;
    expires_time = conf->expires_time;

    if (conf->expires_value != NULL) { //˵��expires���õ��Ǳ�������

        if (ngx_http_complex_value(r, conf->expires_value, &value) != NGX_OK) {
            return NGX_ERROR;
        }

        rc = ngx_http_parse_expires(&value, &expires, &expires_time, &err);

        if (rc != NGX_OK) {
            return NGX_OK;
        }

        if (expires == NGX_HTTP_EXPIRES_OFF) {
            return NGX_OK;
        }
    }

    e = r->headers_out.expires;

    if (e == NULL) {

        e = ngx_list_push(&r->headers_out.headers);
        if (e == NULL) {
            return NGX_ERROR;
        }

        r->headers_out.expires = e;

        e->hash = 1;
        ngx_str_set(&e->key, "Expires");
    }

    len = sizeof("Mon, 28 Sep 1970 06:00:00 GMT");
    e->value.len = len - 1;

    ccp = r->headers_out.cache_control.elts;

    if (ccp == NULL) {

        if (ngx_array_init(&r->headers_out.cache_control, r->pool,
                           1, sizeof(ngx_table_elt_t *))
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        ccp = ngx_array_push(&r->headers_out.cache_control);
        if (ccp == NULL) {
            return NGX_ERROR;
        }

        cc = ngx_list_push(&r->headers_out.headers);
        if (cc == NULL) {
            return NGX_ERROR;
        }

        cc->hash = 1;
        ngx_str_set(&cc->key, "Cache-Control");
        *ccp = cc;

    } else {
        for (i = 1; i < r->headers_out.cache_control.nelts; i++) {
            ccp[i]->hash = 0;
        }

        cc = ccp[0];
    }

    if (expires == NGX_HTTP_EXPIRES_EPOCH) {
        e->value.data = (u_char *) "Thu, 01 Jan 1970 00:00:01 GMT";
        ngx_str_set(&cc->value, "no-cache");//����r->headers_out.cache_control
        return NGX_OK;
    }

    if (expires == NGX_HTTP_EXPIRES_MAX) {
        e->value.data = (u_char *) "Thu, 31 Dec 2037 23:55:55 GMT";
        /* 10 years */
        ngx_str_set(&cc->value, "max-age=315360000");//����r->headers_out.cache_control
        return NGX_OK;
    }

    e->value.data = ngx_pnalloc(r->pool, len);
    if (e->value.data == NULL) {
        return NGX_ERROR;
    }

    if (expires_time == 0 && expires != NGX_HTTP_EXPIRES_DAILY) {
        ngx_memcpy(e->value.data, ngx_cached_http_time.data,
                   ngx_cached_http_time.len + 1);
        ngx_str_set(&cc->value, "max-age=0");//����r->headers_out.cache_control
        return NGX_OK;
    }

    now = ngx_time();

    if (expires == NGX_HTTP_EXPIRES_DAILY) {
        expires_time = ngx_next_time(expires_time);
        max_age = expires_time - now;

    } else if (expires == NGX_HTTP_EXPIRES_ACCESS
               || r->headers_out.last_modified_time == -1)
    {
        max_age = expires_time;
        expires_time += now;

    } else {
        expires_time += r->headers_out.last_modified_time;
        max_age = expires_time - now;
    }

    ngx_http_time(e->value.data, expires_time);

    if (conf->expires_time < 0 || max_age < 0) {
        ngx_str_set(&cc->value, "no-cache");//����r->headers_out.cache_control
        return NGX_OK;
    }

    //����r->headers_out.cache_control
    cc->value.data = ngx_pnalloc(r->pool,
                                 sizeof("max-age=") + NGX_TIME_T_LEN + 1);
    if (cc->value.data == NULL) {
        return NGX_ERROR;
    }

    cc->value.len = ngx_sprintf(cc->value.data, "max-age=%T", max_age)
                    - cc->value.data;

    return NGX_OK;
}

//��ȡexpires���õ�ʱ��
static ngx_int_t
ngx_http_parse_expires(ngx_str_t *value, ngx_http_expires_t *expires,
    time_t *expires_time, char **err)
{
    ngx_uint_t  minus;

    if (*expires != NGX_HTTP_EXPIRES_MODIFIED) {

        if (value->len == 5 && ngx_strncmp(value->data, "epoch", 5) == 0) {
            *expires = NGX_HTTP_EXPIRES_EPOCH; //expire modified epoch
            return NGX_OK;
        }

        if (value->len == 3 && ngx_strncmp(value->data, "max", 3) == 0) {
            *expires = NGX_HTTP_EXPIRES_MAX; //expire modified max
            return NGX_OK;
        }

        if (value->len == 3 && ngx_strncmp(value->data, "off", 3) == 0) {
            *expires = NGX_HTTP_EXPIRES_OFF;  //expire modified off
            return NGX_OK;
        }
    }

    if (value->len && value->data[0] == '@') {
        value->data++;
        value->len--;
        minus = 0;

        if (*expires == NGX_HTTP_EXPIRES_MODIFIED) {
            *err = "daily time cannot be used with \"modified\" parameter";
            return NGX_ERROR;
        }

        *expires = NGX_HTTP_EXPIRES_DAILY;

    } else if (value->len && value->data[0] == '+') {
        value->data++;
        value->len--;
        minus = 0;

    } else if (value->len && value->data[0] == '-') {
        value->data++;
        value->len--;
        minus = 1;

    } else {
        minus = 0;
    }

    *expires_time = ngx_parse_time(value, 1);

    if (*expires_time == (time_t) NGX_ERROR) {
        *err = "invalid value";
        return NGX_ERROR;
    }

    if (*expires == NGX_HTTP_EXPIRES_DAILY
        && *expires_time > 24 * 60 * 60)
    {
        *err = "daily time value must be less than 24 hours";
        return NGX_ERROR;
    }

    if (minus) { //ʱ��Ϊ������תΪ����
        *expires_time = - *expires_time;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_add_header(ngx_http_request_t *r, ngx_http_header_val_t *hv,
    ngx_str_t *value)
{
    ngx_table_elt_t  *h;

    if (value->len) {
        h = ngx_list_push(&r->headers_out.headers);
        if (h == NULL) {
            return NGX_ERROR;
        }

        h->hash = 1;
        h->key = hv->key;
        h->value = *value;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_add_cache_control(ngx_http_request_t *r, ngx_http_header_val_t *hv,
    ngx_str_t *value)
{
    ngx_table_elt_t  *cc, **ccp;

    if (value->len == 0) {
        return NGX_OK;
    }

    ccp = r->headers_out.cache_control.elts;

    if (ccp == NULL) {

        if (ngx_array_init(&r->headers_out.cache_control, r->pool,
                           1, sizeof(ngx_table_elt_t *))
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    ccp = ngx_array_push(&r->headers_out.cache_control);
    if (ccp == NULL) {
        return NGX_ERROR;
    }

    cc = ngx_list_push(&r->headers_out.headers);
    if (cc == NULL) {
        return NGX_ERROR;
    }

    cc->hash = 1;
    ngx_str_set(&cc->key, "Cache-Control");
    cc->value = *value;

    *ccp = cc;

    return NGX_OK;
}


static ngx_int_t
ngx_http_set_last_modified(ngx_http_request_t *r, ngx_http_header_val_t *hv,
    ngx_str_t *value)
{
    if (ngx_http_set_response_header(r, hv, value) != NGX_OK) {
        return NGX_ERROR;
    }

    r->headers_out.last_modified_time =
        (value->len) ? ngx_parse_http_time(value->data, value->len) : -1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_set_response_header(ngx_http_request_t *r, ngx_http_header_val_t *hv,
    ngx_str_t *value)
{
    ngx_table_elt_t  *h, **old;

    old = (ngx_table_elt_t **) ((char *) &r->headers_out + hv->offset);

    if (value->len == 0) {
        if (*old) {
            (*old)->hash = 0;
            *old = NULL;
        }

        return NGX_OK;
    }

    if (*old) {
        h = *old;

    } else {
        h = ngx_list_push(&r->headers_out.headers);
        if (h == NULL) {
            return NGX_ERROR;
        }

        *old = h;
    }

    h->hash = 1;
    h->key = hv->key;
    h->value = *value;

    return NGX_OK;
}


static void *
ngx_http_headers_create_conf(ngx_conf_t *cf)
{
    ngx_http_headers_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_headers_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->headers = NULL;
     *     conf->expires_time = 0;
     *     conf->expires_value = NULL;
     */

    conf->expires = NGX_HTTP_EXPIRES_UNSET;

    return conf;
}


static char *
ngx_http_headers_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_headers_conf_t *prev = parent;
    ngx_http_headers_conf_t *conf = child;

    if (conf->expires == NGX_HTTP_EXPIRES_UNSET) {
        conf->expires = prev->expires;
        conf->expires_time = prev->expires_time;
        conf->expires_value = prev->expires_value;

        if (conf->expires == NGX_HTTP_EXPIRES_UNSET) {
            conf->expires = NGX_HTTP_EXPIRES_OFF;
        }
    }

    if (conf->headers == NULL) {
        conf->headers = prev->headers;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_headers_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_headers_filter;

    return NGX_OK;
}


/*
expires
�﷨�� expires [time|epoch|max|off]

Ĭ��ֵ�� expires off

������ http, server, location

ʹ�ñ�ָ����Կ���HTTPӦ���еġ�Expires���͡�Cache-Control����ͷ�꣬���𵽿���ҳ�滺������ã���

������timeֵ��ʹ��������������Expires��ͷ���ֵ��ͨ����ǰϵͳʱ��������趨�� time ֵ����á�

epoch ָ����Expires����ֵΪ 1 January, 1970, 00:00:01 GMT��

max ָ����Expires����ֵΪ 31 December 2037 23:59:59 GMT����Cache-Control����ֵΪ10�ꡣ

-1 ָ����Expires����ֵΪ ��������ǰʱ�� -1s,����Զ����

��Cache-Control��ͷ���ֵ����ָ����ʱ����������

������Cache-Control: no-cache
�������㣺Cache-Control: max-age = #, # Ϊ��ָ��ʱ���������
"off" ��ʾ���޸ġ�Expires���͡�Cache-Control����ֵ
*/

/*
syntax:  add_header name value;
default:  ��  
context:  http, server, location
 
Adds the specified field to a response header provided that the response code equals 200, 204, 206, 301, 302, 303, 304, or 307. 
    A value can contain variables. 

syntax:  expires [modified] time;
expires epoch | max | off;
 
default:  expires off;
 
context:  http, server, location
 

Enables or disables adding or modifying the ��Expires�� and ��Cache-Control�� response header fields. A parameter can be a positive 
or negative time. 

A time in the ��Expires�� field is computed as a sum of the current time and time specified in the directive. If the modified parameter 
is used (0.7.0, 0.6.32) then time is computed as a sum of the file��s modification time and time specified in the directive. 

In addition, it is possible to specify a time of the day using the ��@�� prefix (0.7.9, 0.6.34): 

expires @15h30m;


The epoch parameter corresponds to the absolute time ��Thu, 01 Jan 1970 00:00:01 GMT��. The contents of the ��Cache-Control�� field 
depends on the sign of the specified time: 
? time is negative �� ��Cache-Control: no-cache��. 
? time is positive or zero �� ��Cache-Control: max-age=t��, where t is a time specified in the directive, in seconds. 


The max parameter sets ��Expires�� to the value ��Thu, 31 Dec 2037 23:55:55 GMT��, and ��Cache-Control�� to 10 years. 
The off parameter disables adding or modifying the ��Expires�� and ��Cache-Control�� response header fields. 
*/
static char *
ngx_http_headers_expires(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{ //expires xx���ô洢����Ϊngx_http_headers_expires�����������Ч����Ϊngx_http_set_expires
    ngx_http_headers_conf_t *hcf = conf;

    char                              *err;
    ngx_str_t                         *value;
    ngx_int_t                          rc;
    ngx_uint_t                         n;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    if (hcf->expires != NGX_HTTP_EXPIRES_UNSET) { //˵��֮ǰ���ù�
        return "is duplicate";
    }

    value = cf->args->elts;

    if (cf->args->nelts == 2) { //expires����ֱ�Ӹ���ʱ��

        hcf->expires = NGX_HTTP_EXPIRES_ACCESS;

        n = 1;

    } else { /* cf->args->nelts == 3 */  //expires modified xxx   �ο�ngx_http_parse_expires
 
        if (ngx_strcmp(value[1].data, "modified") != 0) {
            return "invalid value";
        }

        hcf->expires = NGX_HTTP_EXPIRES_MODIFIED;

        n = 2;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[n];
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cv.lengths != NULL) {

        hcf->expires_value = ngx_palloc(cf->pool,
                                        sizeof(ngx_http_complex_value_t));
        if (hcf->expires_value == NULL) {
            return NGX_CONF_ERROR;
        }

        *hcf->expires_value = cv;

        return NGX_CONF_OK;
    }

    rc = ngx_http_parse_expires(&value[n], &hcf->expires, &hcf->expires_time,
                                &err);
    if (rc != NGX_OK) {
        return err;
    }

    return NGX_CONF_OK;
}

//add_header name value;
static char *
ngx_http_headers_add(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_headers_conf_t *hcf = conf;

    ngx_str_t                         *value;
    ngx_uint_t                         i;
    ngx_http_header_val_t             *hv;
    ngx_http_set_header_t             *set;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (hcf->headers == NULL) {
        hcf->headers = ngx_array_create(cf->pool, 1,
                                        sizeof(ngx_http_header_val_t));
        if (hcf->headers == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    hv = ngx_array_push(hcf->headers);
    if (hv == NULL) {
        return NGX_CONF_ERROR;
    }

    hv->key = value[1]; //add_header name value;�е�name
    hv->handler = ngx_http_add_header;
    hv->offset = 0;
    hv->always = 0;

    set = ngx_http_set_headers;
    for (i = 0; set[i].name.len; i++) {
        if (ngx_strcasecmp(value[1].data, set[i].name.data) != 0) {
            continue;
        }

        //���add_header name value;�е�name��ngx_http_set_headers�е���ԣ����ӦoffsetΪngx_http_set_headers�е�offset
        hv->offset = set[i].offset;
        //���add_header name value;�е�name��ngx_http_set_headers�е���ԣ����ӦhanderΪngx_http_set_headers�е�handler
        hv->handler = set[i].handler;

        break;
    }

    if (value[2].len == 0) {
        ngx_memzero(&hv->value, sizeof(ngx_http_complex_value_t));
        return NGX_CONF_OK;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &hv->value; //��//add_header name value;�е�value������������hv->value

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts == 3) { //add_header name value;
        return NGX_CONF_OK;
    }

    //add_header name value always;
    if (ngx_strcmp(value[3].data, "always") != 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[3]);
        return NGX_CONF_ERROR;
    }

    hv->always = 1;

    return NGX_CONF_OK;
}
