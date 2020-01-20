
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>

//(proxy_cache_path)�������ߵ�����
typedef struct { //��ֵ��ngx_http_file_cache_set_slot
    //u->caches = &ngx_http_proxy_main_conf_t->caches;
    ngx_array_t                    caches;  /* ngx_http_file_cache_t * */ //ngx_http_file_cache_t��Ա�����ձ���ֵ��u->caches
} ngx_http_proxy_main_conf_t;


typedef struct ngx_http_proxy_rewrite_s  ngx_http_proxy_rewrite_t;

typedef ngx_int_t (*ngx_http_proxy_rewrite_pt)(ngx_http_request_t *r,
    ngx_table_elt_t *h, size_t prefix, size_t len,
    ngx_http_proxy_rewrite_t *pr);

/*
//���û������proxy_redirect��������Ϊproxy_redirect default,��pattern=plcf->url replacement=location(location /xxxx {}�е�/xxx),
��ngx_http_proxy_redirect��ngx_http_proxy_merge_loc_conf
*/
struct ngx_http_proxy_rewrite_s { //ngx_http_proxy_redirect�д����ýṹ�ռ�
    ngx_http_proxy_rewrite_pt      handler; //ngx_http_proxy_rewrite_complex_handler

    //����proxy_redirect   http://localhost:8000/    http://$host:$server_port/;�е�http://localhost:8000/
    union {
        ngx_http_complex_value_t   complex; 
#if (NGX_PCRE)
        ngx_http_regex_t          *regex;
#endif
    } pattern; 
    
    //����proxy_redirect   http://localhost:8000/    http://$host:$server_port/;�е� http://$host:$server_port/
    ngx_http_complex_value_t       replacement;
};

/*
  proxy_pass  http://10.10.0.103:8080/tttxxsss; 
  ���������:http://10.2.13.167/proxy1111/yangtest
  ʵ���ϵ���˵�uri���Ϊ/tttxxsss/yangtest
  plcf->location:/proxy1111, ctx->vars.uri:/tttxxsss,  r->uri:/proxy1111/yangtest, r->args:, urilen:19
 */
typedef struct {
    //proxy_pass  http://10.10.0.103:8080/tttxx;�е�http://10.10.0.103:8080
    ngx_str_t                      key_start; // ��ʼ״̬plcf->vars.key_start = plcf->vars.schema;
    /*  "http://" ���� "https://"  */ //ָ�����uri����ʱschema->len="http://" ���� "https://"�ַ�������7����8���ο�ngx_http_proxy_pass
    ngx_str_t                      schema; ////proxy_pass  http://10.10.0.103:8080/tttxx;�е�http://
    
    ngx_str_t                      host_header;//proxy_pass  http://10.10.0.103:8080/tttxx; �е�10.10.0.103:8080
    ngx_str_t                      port; //"80"����"443"����ngx_http_proxy_set_vars
    //proxy_pass  http://10.10.0.103:8080/tttxx; �е�/tttxx         uri����http://http://10.10.0.103:8080 url����http://10.10.0.103:8080
    ngx_str_t                      uri; //ngx_http_proxy_set_vars���渳ֵ   uri��proxy_pass  http://10.10.0.103:8080/xxx�е�/xxx�����
    //proxy_pass  http://10.10.0.103:8080��uri����Ϊ0��û��uri
} ngx_http_proxy_vars_t;


typedef struct {
    ngx_array_t                   *flushes;
     /* 
    ��proxy_set_header��ngx_http_proxy_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
    ��ngx_http_proxy_cache_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
     */
    //1. ����洢����ngx_http_proxy_headers��proxy_set_header���õ�ͷ����Ϣͷ��ӵ���lengths��values ����ngx_http_proxy_loc_conf_t->headers
    //2. ����洢����ngx_http_proxy_cache_headers���õ�ͷ����Ϣͷ��ӵ���lengths��values  ����ngx_http_proxy_loc_conf_t->headers_cache
    ngx_array_t                   *lengths; //�����ռ�͸�ֵ��ngx_http_proxy_init_headers
    ngx_array_t                   *values;  //�����ռ�͸�ֵ��ngx_http_proxy_init_headers
    ngx_hash_t                     hash;
} ngx_http_proxy_headers_t;


typedef struct {
    ngx_http_upstream_conf_t       upstream;

    /* �����⼸����proxy_set_body XXX������� proxy_set_body���ð���󣬾Ͳ��ᴫ�Ϳͻ��˷����������ݵ���˷�����(ֻ����proxy_set_body���õİ���)��
    ���û�����ã���ᷢ�Ϳͻ��˰������ݵ���ˣ� �ο�ngx_http_proxy_create_request */
    ngx_array_t                   *body_flushes; //��proxy_set_body XXX�����л�ȡ����ngx_http_proxy_merge_loc_conf
    ngx_array_t                   *body_lengths; //��proxy_set_body XXX�����л�ȡ����ngx_http_proxy_merge_loc_conf
    ngx_array_t                   *body_values;  //��proxy_set_body XXX�����л�ȡ����ngx_http_proxy_merge_loc_conf
    ngx_str_t                      body_source; //proxy_set_body XXX�����е�xxx

    //������Դ��ngx_http_proxy_headers  proxy_set_header����ngx_http_proxy_init_headers
    ngx_http_proxy_headers_t       headers;//�����ռ�͸�ֵ��ngx_http_proxy_merge_loc_conf
#if (NGX_HTTP_CACHE)
    //������Դ��ngx_http_proxy_cache_headers����ngx_http_proxy_init_headers
    ngx_http_proxy_headers_t       headers_cache;//�����ռ�͸�ֵ��ngx_http_proxy_merge_loc_conf
#endif
    //ͨ��proxy_set_header Host $proxy_host;���ò���ӵ���������
    ngx_array_t                   *headers_source; //�����ռ�͸�ֵ��ngx_http_proxy_init_headers

    //����uri�еı�����ʱ����õ�������������ngx_http_proxy_pass  proxy_pass xxx������б��������������Ͳ�Ϊ��
    ngx_array_t                   *proxy_lengths;
    ngx_array_t                   *proxy_values;
    //proxy_redirect [ default|off|redirect replacement ]; 
    //��Ա����ngx_http_proxy_rewrite_t
    ngx_array_t                   *redirects; //������proxy_redirect��أ���ngx_http_proxy_redirect�����ռ�
    ngx_array_t                   *cookie_domains;
    ngx_array_t                   *cookie_paths;

    /* ���������ʾת��ʱ��Э�鷽��������������Ϊ��proxy_method POST;��ô�ͻ��˷�����GET������ת��ʱ������Ҳ���ΪPOST */
    ngx_str_t                      method;
    ngx_str_t                      location; //��ǰlocation������ location xxx {} �е�xxx

    //proxy_pass  http://10.10.0.103:8080/tttxx; �е�http://10.10.0.103:8080/tttxx
    ngx_str_t                      url; //proxy_pass url���֣���ngx_http_proxy_pass

#if (NGX_HTTP_CACHE)
    ngx_http_complex_value_t       cache_key;//proxy_cache_keyΪ���潨������ʱʹ�õĹؼ��֣���ngx_http_proxy_cache_key
#endif

    ngx_http_proxy_vars_t          vars;//���汣��proxy_pass uri�е�uri��Ϣ
    //Ĭ��1
    ngx_flag_t                     redirect;//������proxy_redirect��أ���ngx_http_proxy_redirect
    //proxy_http_version���ã�
    ngx_uint_t                     http_version; //����˷��������õ�httpЭ��汾��Ĭ��NGX_HTTP_VERSION_10

    ngx_uint_t                     headers_hash_max_size;
    ngx_uint_t                     headers_hash_bucket_size;

#if (NGX_HTTP_SSL)
    ngx_uint_t                     ssl;
    ngx_uint_t                     ssl_protocols;
    ngx_str_t                      ssl_ciphers;
    ngx_uint_t                     ssl_verify_depth;
    ngx_str_t                      ssl_trusted_certificate;
    ngx_str_t                      ssl_crl;
    ngx_str_t                      ssl_certificate;
    ngx_str_t                      ssl_certificate_key;
    ngx_array_t                   *ssl_passwords;
#endif
} ngx_http_proxy_loc_conf_t;


typedef struct { //ngx_http_proxy_handler�д����ռ�
    ngx_http_status_t              status; //HTTP/1.1 200 OK ��ֵ��ngx_http_proxy_process_status_line
    ngx_http_chunked_t             chunked;
    ngx_http_proxy_vars_t          vars;
    off_t                          internal_body_length; //����Ϊ�ͻ��˷��͹����İ��峤��+proxy_set_body���õ��ַ����峤�Ⱥ�

    ngx_chain_t                   *free;
    ngx_chain_t                   *busy;

    unsigned                       head:1; //����˵�����������ʱ"HEAD"
    unsigned                       internal_chunked:1;
    unsigned                       header_sent:1;
} ngx_http_proxy_ctx_t; //proxyģ��ĸ�����������Ϣ�������ȡ������ݵ�ʱ����ж�ν�������������¼״̬��Ϣctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);


static ngx_int_t ngx_http_proxy_eval(ngx_http_request_t *r,
    ngx_http_proxy_ctx_t *ctx, ngx_http_proxy_loc_conf_t *plcf);
#if (NGX_HTTP_CACHE)
static ngx_int_t ngx_http_proxy_create_key(ngx_http_request_t *r);
#endif
static ngx_int_t ngx_http_proxy_create_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_proxy_reinit_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_proxy_body_output_filter(void *data, ngx_chain_t *in);
static ngx_int_t ngx_http_proxy_process_status_line(ngx_http_request_t *r);
static ngx_int_t ngx_http_proxy_process_header(ngx_http_request_t *r);
static ngx_int_t ngx_http_proxy_input_filter_init(void *data);
static ngx_int_t ngx_http_proxy_copy_filter(ngx_event_pipe_t *p,
    ngx_buf_t *buf);
static ngx_int_t ngx_http_proxy_chunked_filter(ngx_event_pipe_t *p,
    ngx_buf_t *buf);
static ngx_int_t ngx_http_proxy_non_buffered_copy_filter(void *data,
    ssize_t bytes);
static ngx_int_t ngx_http_proxy_non_buffered_chunked_filter(void *data,
    ssize_t bytes);
static void ngx_http_proxy_abort_request(ngx_http_request_t *r);
static void ngx_http_proxy_finalize_request(ngx_http_request_t *r,
    ngx_int_t rc);

static ngx_int_t ngx_http_proxy_host_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_proxy_port_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t
    ngx_http_proxy_add_x_forwarded_for_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t
    ngx_http_proxy_internal_body_length_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_proxy_internal_chunked_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_proxy_rewrite_redirect(ngx_http_request_t *r,
    ngx_table_elt_t *h, size_t prefix);
static ngx_int_t ngx_http_proxy_rewrite_cookie(ngx_http_request_t *r,
    ngx_table_elt_t *h);
static ngx_int_t ngx_http_proxy_rewrite_cookie_value(ngx_http_request_t *r,
    ngx_table_elt_t *h, u_char *value, ngx_array_t *rewrites);
static ngx_int_t ngx_http_proxy_rewrite(ngx_http_request_t *r,
    ngx_table_elt_t *h, size_t prefix, size_t len, ngx_str_t *replacement);

static ngx_int_t ngx_http_proxy_add_variables(ngx_conf_t *cf);
static void *ngx_http_proxy_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_proxy_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_proxy_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_proxy_init_headers(ngx_conf_t *cf,
    ngx_http_proxy_loc_conf_t *conf, ngx_http_proxy_headers_t *headers,
    ngx_keyval_t *default_headers);

static char *ngx_http_proxy_pass(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_proxy_redirect(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_proxy_cookie_domain(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_proxy_cookie_path(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_proxy_store(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#if (NGX_HTTP_CACHE)
static char *ngx_http_proxy_cache(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_proxy_cache_key(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
#endif
#if (NGX_HTTP_SSL)
static char *ngx_http_proxy_ssl_password_file(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
#endif

static char *ngx_http_proxy_lowat_check(ngx_conf_t *cf, void *post, void *data);

static ngx_int_t ngx_http_proxy_rewrite_regex(ngx_conf_t *cf,
    ngx_http_proxy_rewrite_t *pr, ngx_str_t *regex, ngx_uint_t caseless);

#if (NGX_HTTP_SSL)
static ngx_int_t ngx_http_proxy_set_ssl(ngx_conf_t *cf,
    ngx_http_proxy_loc_conf_t *plcf);
#endif
static void ngx_http_proxy_set_vars(ngx_url_t *u, ngx_http_proxy_vars_t *v);


static ngx_conf_post_t  ngx_http_proxy_lowat_post =
    { ngx_http_proxy_lowat_check };

//���������˷��ص�status�������Ƿ�Ѱ����һ����������������
static ngx_conf_bitmask_t  ngx_http_proxy_next_upstream_masks[] = {
    { ngx_string("error"), NGX_HTTP_UPSTREAM_FT_ERROR },
    { ngx_string("timeout"), NGX_HTTP_UPSTREAM_FT_TIMEOUT },
    { ngx_string("invalid_header"), NGX_HTTP_UPSTREAM_FT_INVALID_HEADER },
    { ngx_string("http_500"), NGX_HTTP_UPSTREAM_FT_HTTP_500 },
    { ngx_string("http_502"), NGX_HTTP_UPSTREAM_FT_HTTP_502 },
    { ngx_string("http_503"), NGX_HTTP_UPSTREAM_FT_HTTP_503 },
    { ngx_string("http_504"), NGX_HTTP_UPSTREAM_FT_HTTP_504 },
    { ngx_string("http_403"), NGX_HTTP_UPSTREAM_FT_HTTP_403 },
    { ngx_string("http_404"), NGX_HTTP_UPSTREAM_FT_HTTP_404 },
    { ngx_string("updating"), NGX_HTTP_UPSTREAM_FT_UPDATING },
    { ngx_string("off"), NGX_HTTP_UPSTREAM_FT_OFF },
    { ngx_null_string, 0 }
};


#if (NGX_HTTP_SSL)

static ngx_conf_bitmask_t  ngx_http_proxy_ssl_protocols[] = {
    { ngx_string("SSLv2"), NGX_SSL_SSLv2 },
    { ngx_string("SSLv3"), NGX_SSL_SSLv3 },
    { ngx_string("TLSv1"), NGX_SSL_TLSv1 },
    { ngx_string("TLSv1.1"), NGX_SSL_TLSv1_1 },
    { ngx_string("TLSv1.2"), NGX_SSL_TLSv1_2 },
    { ngx_null_string, 0 }
};

#endif


static ngx_conf_enum_t  ngx_http_proxy_http_version[] = {
    { ngx_string("1.0"), NGX_HTTP_VERSION_10 },
    { ngx_string("1.1"), NGX_HTTP_VERSION_11 },
    { ngx_null_string, 0 }
};

ngx_module_t  ngx_http_proxy_module;

/*
Nginx�ķ������ģ�黹�ṩ�˺ܶ������ã����������ӵĳ�ʱʱ�䡢��ʱ�ļ���δ洢���Լ�����Ҫ����λ������η�������Ӧ�ȹ��ܡ���Щ���ÿ���ͨ���Ķ�
ngx_http_proxy_moduleģ���˵���˽⣬ֻ���������⣬����ʵ��һ�������ܵķ�����������������ֻ�ǽ��ܷ������������Ļ������ܣ��ڵ�12��������
���������̽��upstream���ƣ�����ʱ������Ҳ��ᷢ��ngx_http_proxy_moduleģ��ֻ��ʹ��upstream����ʵ���˷�������ܶ��ѡ�
*/
static ngx_command_t  ngx_http_proxy_commands[] = {
/*
upstream��
�﷨��upstream name {...}
���ÿ飺http
upstream�鶨����һ�����η������ļ�Ⱥ�����ڷ�������е�proxy_passʹ�á����磺
upstream backend {
  server backend1.example.com;
  server backend2.example.com;
    server backend3.example.com;
}

server {
  location / {
    proxy_pass  http://backend;
  }
}

�﷨��proxy_pass URL 
Ĭ��ֵ��no 
ʹ���ֶΣ�location, location�е�if�ֶ� 
���ָ�����ñ�����������ĵ�ַ�ͱ�ӳ���URI����ַ����ʹ����������IP�Ӷ˿ںŵ���ʽ�����磺


proxy_pass http://localhost:8000/uri/;����һ��unix socket��


proxy_pass http://unix:/path/to/backend.socket:/uri/;·����unix�ؼ��ֵĺ���ָ����λ������ð��֮�䡣
ע�⣺HTTP Hostͷû��ת������������Ϊ����proxy_pass���������磬������ƶ���������example.com������һ̨������Ȼ��������������������example.com��һ���µ�IP����ͬʱ�ھɻ������ֶ����µ�example.comIPд��/etc/hosts��ͬʱʹ��proxy_pass�ض���http://example.com, Ȼ���޸�DNS���µ�IP��
����������ʱ��Nginx��location��Ӧ��URI�����滻��proxy_passָ������ָ���Ĳ��֣����������������ʹ���޷�ȷ�����ȥ�滻��


��locationͨ��������ʽָ����

����ʹ�ô����location������rewriteָ��ı�URI��ʹ��������ÿ��Ը��Ӿ�ȷ�Ĵ�������break����

location  /name/ {
  rewrite      /name/([^/] +)  /users?name=$1  break;
  proxy_pass   http://127.0.0.1;
}��Щ�����URI��û�б�ӳ�䴫�ݡ�
���⣬��Ҫ����һЩ����Ա�URI���ԺͿͻ�����ͬ�ķ�����ʽת���������Ǵ��������ʽ�����䴦���ڼ䣺


���������ϵ�б�ܽ����滻Ϊһ���� ��//�� �C ��/��; 

��ɾ�����õĵ�ǰĿ¼����/./�� �C ��/��; 

��ɾ�����õ���ǰĿ¼����/dir /../�� �C ��/����
����ڷ������ϱ�����δ���κδ������ʽ����URI����ô��proxy_passָ���б���ʹ��δָ��URI�Ĳ��֣�

location  /some/path/ {
  proxy_pass   http://127.0.0.1;
}��ָ����ʹ�ñ�����һ�ֱȽ������������������URL����ʹ�ò����������ȫ�ֹ����URL��
����ζ�����е����ò��������㷽��Ľ���ĳ������Ҫ����������Ŀ¼���������ǽ���ת������ͬ��URL����һ��server�ֶε����ã���


location / {
  proxy_pass   http://127.0.0.1:8080/VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot;
}���������ʹ��rewrite��proxy_pass����ϣ�


location / {
  rewrite ^(.*)$ /VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot$1 break;
  proxy_pass   http://127.0.0.1:8080;
}��������������URL������д�� proxy_pass�е���βб�ܲ�û��ʵ�����塣
�����Ҫͨ��ssl�������ӵ�һ�����η������飬proxy_passǰ׺Ϊ https://������ͬʱָ��ssl�Ķ˿ڣ��磺


upstream backend-secure {
  server 10.0.0.20:443;
}
 
server {
  listen 10.0.0.1:443;
  location / {
    proxy_pass https://backend-secure;
  }
}
*/
    { ngx_string("proxy_pass"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_pass,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
�﷨��proxy_redirect [ default|off|redirect replacement ];
Ĭ�ϣ�proxy_redirect default;
���ÿ飺http��server��location
�����η��������ص���Ӧ���ض����ˢ��������HTTP��Ӧ����301����302��ʱ��proxy_redirect��������HTTPͷ����location��refresh�ֶΡ�
���磬������η�������������Ӧ��302�ض�������location�ֶε�URI��http://localhost:8000/two/some/uri/����ô���������������£�ʵ��ת�����ͻ��˵�location��http://frontend/one/some/uri/��
proxy_redirect http://localhost:8000/two/ http://frontend/one/;
���ﻹ����ʹ��ngx-http-core-module�ṩ�ı����������µ�location�ֶΡ����磺
proxy_redirect   http://localhost:8000/    http://$host:$server_port/;
Ҳ����ʡ��replacement�����е����������֣���ʱ��������������������䡣���磺
proxy_redirect http://localhost:8000/two/ /one/;
ʹ��off����ʱ����ʹlocation����refresh�ֶ�ά�ֲ��䡣���磺
proxy_redirect off;
ʹ��Ĭ�ϵ�default����ʱ���ᰴ��proxy_pass�������������location���������鷢���ͻ��˵�locationͷ�������磬������������Ч����һ���ģ�
location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   default;
}
 
location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   http://upstream:port/two/   /one/;
}

�﷨��proxy_redirect [ default|off|redirect replacement ];
Ĭ��ֵ��proxy_redirect default;
ʹ���ֶΣ�http, server, location
���ָ��Ϊ�����������Ӧ���б���ı��Ӧ��ͷ����Location���͡�Refresh������ֵ��
���Ǽ��豻����ķ��������ص�Ӧ��ͷ�ֶ�Ϊ��Location: http://localhost:8000/two/some/uri/��
ָ�


proxy_redirect http://localhost:8000/two/ http://frontend/one/;�Ὣ����дΪ��Location: http://frontend/one/some/uri/��
����д�ֶ�������Բ�ʹ�÷���������

proxy_redirect http://localhost:8000/two/ /;������Ĭ�ϵķ��������Ͷ˿ڽ������ã��˿�Ĭ��80��
Ĭ�ϵ���д����ʹ�ò���default����ʹ��location��proxy_pass��ֵ��
�������������ǵȼ۵ģ�


location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   default;
}
 
location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   http://upstream:port/two/   /one/;
}ͬ��������д�ֶ��п���ʹ�ñ�����


proxy_redirect   http://localhost:8000/    http://$host:$server_port/;���ָ������ظ�ʹ�ã�


proxy_redirect   default;
proxy_redirect   http://localhost:8000/    /;
proxy_redirect   http://www.example.com/   /;����off�ڱ����н������е�proxy_redirectָ�


proxy_redirect   off;
proxy_redirect   default;
proxy_redirect   http://localhost:8000/    /;
proxy_redirect   http://www.example.com/   /;���ָ����Ժ����׵Ľ�������������ķ���������дΪ����������ķ���������

proxy_redirect   /   /;
*/ //ngx_http_proxy_merge_loc_conf��if�������ݺ�ngx_http_proxy_redirect�����proxy_redirect defaultһ����Ҳ����˵proxy_redirectĬ��Ϊdefault
    { ngx_string("proxy_redirect"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_proxy_redirect,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("proxy_cookie_domain"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_proxy_cookie_domain,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("proxy_cookie_path"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_proxy_cookie_path,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /*
     �﷨��proxy_store [on | off | path] 
     Ĭ��ֵ��proxy_store off 
     ʹ���ֶΣ�http, server, location 
     ���ָ��������Щ�������ļ������洢��������on�������ļ���alias��rootָ��ָ����Ŀ¼һ�£�������off�����رմ洢��·�����п���ʹ�ñ�����

     proxy_store   /data/www$original_uri;Ӧ��ͷ�еġ�Last-Modified���ֶ��������ļ�����޸�ʱ�䣬Ϊ���ļ��İ�ȫ������ʹ��proxy_temp_pathָ��һ����ʱ�ļ�Ŀ¼��
     ���ָ��Ϊ��Щ���Ǿ���ʹ�õ��ļ���һ�ݱ��ؿ������Ӷ����ٱ�������������ء�
     
     location /images/ {
       root                 /data/www;
       error_page           404 = /fetch$uri;
     }
      
     location /fetch {
       internal;
       proxy_pass           http://backend;
       proxy_store          on;
       proxy_store_access   user:rw  group:rw  all:r;
       proxy_temp_path      /data/temp;
       alias                /data/www;
     }����ͨ�����ַ�ʽ��
     
     location /images/ {
       root                 /data/www;
       error_page           404 = @fetch;
     }
      
     location @fetch {
       internal;
      
       proxy_pass           http://backend;
       proxy_store          on;
       proxy_store_access   user:rw  group:rw  all:r;
       proxy_temp_path      /data/temp;
      
       root                 /data/www;
     }ע��proxy_store����һ�����棬��������һ������

     nginx�Ĵ洢ϵͳ�����࣬һ����ͨ��proxy_store�����ģ��洢��ʽ�ǰ���url�е��ļ�·�����洢�ڱ��ء�����/file/2013/0001/en/test.html��
     ��ônginx�ͻ���ָ���Ĵ洢Ŀ¼�����ν�������Ŀ¼���ļ�����һ����ͨ��proxy_cache���������ַ�ʽ�洢���ļ����ǰ���url·������֯�ģ�
     ����ʹ��һЩ���ⷽʽ�������(�����Ϊ�Զ��巽ʽ)���Զ��巽ʽ��������Ҫ�ص�����ġ���ô�����ַ�ʽ����ʲô�����أ�


    ��url·���洢�ļ��ķ�ʽ�������������Ƚϼ򵥣��������ܲ��С������е�url�޳�������Ҫ�ڱ����ļ�ϵͳ�Ͻ���������Ŀ¼����ô�ļ��Ĵ�
    �Ͳ��Ҷ��ܻ����(����kernel��ͨ��·��������inode�Ĺ��̰�)�����ʹ���Զ��巽ʽ������ģʽ������Ҳ�벻���ļ���·����������������url����
    ���������������Ӻ����ܵĽ��͡���ĳ��������˵����һ���û�̬�ļ�ϵͳ������͵�Ӧ������squid�е�CFS��nginxʹ�õķ�ʽ��Լ򵥣���Ҫ����
    url��md5ֵ������
     */
     /*
       nginx�Ĵ洢ϵͳ�����࣬һ����ͨ��proxy_store�����ģ��洢��ʽ�ǰ���url�е��ļ�·�����洢�ڱ��ء�����/file/2013/0001/en/test.html��
     ��ônginx�ͻ���ָ���Ĵ洢Ŀ¼�����ν�������Ŀ¼���ļ�����һ����ͨ��proxy_cache���������ַ�ʽ�洢���ļ����ǰ���url·������֯�ģ�
     ����ʹ��һЩ���ⷽʽ�������(�����Ϊ�Զ��巽ʽ)���Զ��巽ʽ��������Ҫ�ص�����ġ���ô�����ַ�ʽ����ʲô�����أ�


    ��url·���洢�ļ��ķ�ʽ�������������Ƚϼ򵥣��������ܲ��С������е�url�޳�������Ҫ�ڱ����ļ�ϵͳ�Ͻ���������Ŀ¼����ô�ļ��Ĵ�
    �Ͳ��Ҷ��ܻ����(����kernel��ͨ��·��������inode�Ĺ��̰�)�����ʹ���Զ��巽ʽ������ģʽ������Ҳ�벻���ļ���·����������������url����
    ���������������Ӻ����ܵĽ��͡���ĳ��������˵����һ���û�̬�ļ�ϵͳ������͵�Ӧ������squid�е�CFS��nginxʹ�õķ�ʽ��Լ򵥣���Ҫ����
    url��md5ֵ������
     */
    { ngx_string("proxy_store"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_store,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /*
    �﷨��proxy_store_access users:permissions [users:permission ��] 
    Ĭ��ֵ��proxy_store_access user:rw 
    ʹ���ֶΣ�http, server, location 
    ָ�������ļ���Ŀ¼�����Ȩ�ޣ��磺
    proxy_store_access  user:rw  group:rw  all:r;�����ȷָ����������е�Ȩ�ޣ���û�б�Ҫȥָ���û���Ȩ�ޣ�
    proxy_store_access  group:rw  all:r;
     */
    { ngx_string("proxy_store_access"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE123,
      ngx_conf_set_access_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.store_access),
      NULL },

/*
�﷨��proxy_buffering on|off 
Ĭ��ֵ��proxy_buffering on 
ʹ���ֶΣ�http, server, location 
Ϊ��˵ķ���������Ӧ�𻺳塣
������û��壬nginx���豻����������ܹ��ǳ���Ĵ���Ӧ�𣬲�������뻺����������ʹ�� proxy_buffer_size��proxy_buffers������ز�����
�����Ӧ�޷�ȫ�������ڴ棬����д��Ӳ�̡�
������û��壬�Ӻ�˴�����Ӧ�����������͵��ͻ��ˡ�
nginx���Ա������������Ӧ����Ŀ������Ӧ��Ĵ�С������proxy_buffer_size��ָ����ֵ��
���ڻ��ڳ���ѯ��CometӦ����Ҫ�ر����ָ������첽��Ӧ�𽫱����岢��Comet�޷�����������
*/
    { ngx_string("proxy_buffering"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.buffering),
      NULL },

    /*
     Syntax:  proxy_request_buffering on | off;
     Default:  proxy_request_buffering on; 
     Context:  http, server, location

        Enables or disables buffering of a client request body. 
         When buffering is enabled, the entire request body is read from the client before sending the request to a proxied server. 
         When buffering is disabled, the request body is sent to the proxied server immediately as it is received. In this case, 
     the request cannot be passed to the next server if nginx already started sending the request body. 
         When HTTP/1.1 chunked transfer encoding is used to send the original request body, the request body will be buffered 
     regardless of the directive value unless HTTP/1.1 is enabled for proxying. 
     */
    { ngx_string("proxy_request_buffering"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.request_buffering),
      NULL },

    { ngx_string("proxy_ignore_client_abort"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ignore_client_abort),
      NULL },

    /*
     proxy_bind  192.168.1.1;
     �ڵ���connect()ǰ������socket�󶨵�һ�����ص�ַ����������ж������ӿڻ������������ϣ�����������ͨ��ָ���Ľ�ڻ��ַ��
     ����ʹ�����ָ�
     */
    { ngx_string("proxy_bind"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_upstream_bind_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.local),
      NULL },

/*
Ĭ��ֵ��proxy_connect_timeout 60 
ʹ���ֶΣ�http, server, location 
ָ��һ�����ӵ�����������ĳ�ʱʱ�䣬��Ҫע��������ʱ����ò�Ҫ����75�롣
���ʱ�䲢����ָ����������ҳ���ʱ�䣨���ʱ����proxy_read_timeout��������������ǰ�˴�����������������еģ���������һЩ״��
������û���㹻���߳�ȥ�����������󽫱�����һ�����ӳ����ӳٴ�������ô������������ڷ�����ȥ�������ӡ�
*/
    { ngx_string("proxy_connect_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.connect_timeout),
      NULL },

/*
Ĭ��ֵ��proxy_connect_timeout 60 
ʹ���ֶΣ�http, server, location 
ָ��һ�����ӵ�����������ĳ�ʱʱ�䣬��Ҫע��������ʱ����ò�Ҫ����75�롣
���ʱ�䲢����ָ����������ҳ���ʱ�䣨���ʱ����proxy_read_timeout��������������ǰ�˴�����������������еģ���������һЩ״
��������û���㹻���߳�ȥ�����������󽫱�����һ�����ӳ����ӳٴ�������ô������������ڷ�����ȥ�������ӡ�
*/
    { ngx_string("proxy_send_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.send_timeout),
      NULL },

    { ngx_string("proxy_send_lowat"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.send_lowat),
      &ngx_http_proxy_lowat_post },

    /*
     �﷨��proxy_intercept_errors [ on|off ] 
     Ĭ��ֵ��proxy_intercept_errors off 
     ʹ���ֶΣ�http, server, location 
     ʹnginx��ֹHTTPӦ�����Ϊ400���߸��ߵ�Ӧ��
     Ĭ������±����������������Ӧ�𶼽������ݡ� 
     �����������Ϊon��nginx�Ὣ��ֹ���ⲿ�ִ�����һ��error_pageָ�����������error_page��û��ƥ��Ĵ��������򱻴�����������ݵĴ���Ӧ��ᰴԭ�����ݡ�
     */
    { ngx_string("proxy_intercept_errors"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.intercept_errors),
      NULL },

    /*
    �﷨��proxy_set_header header value 
    Ĭ��ֵ�� Host and Connection
    ʹ���ֶΣ�http, server, location 
    ���ָ���������͵������������������ͷ���¶����������һЩ�ֶΡ�
    ���ֵ������һ���ı��������������ǵ���ϡ�
    proxy_set_header��ָ�����ֶ���û�ж���ʱ��������ϼ��ֶμ̳С�
    Ĭ��ֻ�������ֶο������¶��壺

    proxy_set_header Host $proxy_host;
    proxy_set_header Connection Close;δ�޸ĵ�����ͷ��Host�����������·�ʽ���ͣ�

    proxy_set_header Host $http_host;�����������ֶ��ڿͻ��˵�����ͷ�в����ڣ���ô���������ݵ��������������
    ������������ʹ��$Host����������ֵ��������ͷ�еġ�Host���ֶλ����������

    proxy_set_header Host $host;���⣬���Խ�������Ķ˿������������һ�𴫵ݣ�

    proxy_set_header Host $host:$proxy_port;�������Ϊ���ַ������򲻻ᴫ��ͷ������ˣ������������ý���ֹ���ʹ��gzipѹ����

    proxy_set_header  Accept-Encoding  "";
     */
    { ngx_string("proxy_set_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_keyval_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, headers_source),
      NULL },

    { ngx_string("proxy_headers_hash_max_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, headers_hash_max_size),
      NULL },

    { ngx_string("proxy_headers_hash_bucket_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, headers_hash_bucket_size),
      NULL },

/*
Allows redefining the request body passed to the proxied server. The value can contain text, variables, and their combination. 
*/ //����ͨ����˵�body��ֵ�����ֵ���԰���������
  /* �����⼸����proxy_set_body XXX������� proxy_set_body���ð���󣬾Ͳ��ᴫ�Ϳͻ��˷����������ݵ���˷�����(ֻ����proxy_set_body���õİ���)��
    ���û�����ã���ᷢ�Ϳͻ��˰������ݵ���ˣ� �ο�ngx_http_proxy_create_request */  
    { ngx_string("proxy_set_body"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, body_source),
      NULL },

/*
proxy_method

�﷨��proxy_method method;

���ÿ飺http��server��location

���������ʾת��ʱ��Э�鷽��������������Ϊ��

proxy_method POST;

��ô�ͻ��˷�����GET������ת��ʱ������Ҳ���ΪPOST
*/
    { ngx_string("proxy_method"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, method),
      NULL },

/*
�﷨��proxy_pass_request_headers on | off;
Ĭ�ϣ�proxy_pass_request_headers on;
���ÿ飺http��server��location
����Ϊȷ���Ƿ�ת��HTTPͷ����
*/
    { ngx_string("proxy_pass_request_headers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.pass_request_headers),
      NULL },

/*
�﷨��proxy_pass_request_body on | off;
Ĭ�ϣ�proxy_pass_request_body on;
���ÿ飺http��server��location
����Ϊȷ���Ƿ������η���������HTTP���岿�֡�
*/
    { ngx_string("proxy_pass_request_body"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.pass_request_body),
      NULL },

/*
proxy_buffer_size 
�﷨��proxy_buffer_size the_size 
Ĭ��ֵ��proxy_buffer_size 4k/8k 
ʹ���ֶΣ�http, server, location 
���ôӱ������������ȡ�ĵ�һ����Ӧ��Ļ�������С��
ͨ��������ⲿ��Ӧ���а���һ��С��Ӧ��ͷ��
Ĭ����������ֵ�Ĵ�СΪָ��proxy_buffers��ָ����һ���������Ĵ�С���������Խ�������Ϊ��С��
*/ //ָ���Ŀռ俪����ngx_http_upstream_process_header  
    { ngx_string("proxy_buffer_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.buffer_size),
      NULL },

    /*
    �﷨��proxy_read_timeout time 
    Ĭ��ֵ��proxy_read_timeout 60s 
    ʹ���ֶΣ�http, server, location 
        ������ȡ��˷�����Ӧ��ĳ�ʱʱ�䣬��λΪ�룬������nginx���ȴ����ʱ����ȡ��һ�������Ӧ�𡣳�ʱʱ����ָ������������ֺ�
    ��״̬Ϊestablished��ȴ���ȡ������ݵ��¼���
        �����proxy_connect_timeout�����ʱ�������׽��һ̨��������ӷ������ӳ��ӳٴ�����û�����ݴ��͵ķ�������ע�ⲻҪ����ֵ����̫�ͣ�
    ĳЩ����´�������������ܳ���ʱ�������ҳ��Ӧ�������統����һ����Ҫ�ܶ����ı���ʱ������Ȼ������ڲ�ͬ��location�������ò�ͬ��ֵ��
    ����ͨ��ָ��ʱ�䵥λ����������ң�֧�ֵ�ʱ�䵥λ�С�s��(��), ��ms��(����), ��y��(��), ��M��(��), ��w��(��), ��d��(��), ��h��(Сʱ),�� ��m��(����)��
    ���ֵ���ܴ���597Сʱ��
    */
    { ngx_string("proxy_read_timeout"), //��ȡ������ݵĳ�ʱʱ��
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.read_timeout),
      NULL },

    /*
     �﷨��proxy_buffers the_number is_size; 
     Ĭ��ֵ��proxy_buffers 8 4k/8k; 
     ʹ���ֶΣ�http, server, location 
     �������ڶ�ȡӦ�����Ա�������������Ļ�������Ŀ�ʹ�С��Ĭ�����ҲΪ��ҳ��С�����ݲ���ϵͳ�Ĳ�ͬ������4k����8k��
     */ //���������õĿռ���������ռ���//��ngx_event_pipe_read_upstream�д����ռ�
    { ngx_string("proxy_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.bufs),
      NULL },

    //Ĭ��ֵ��proxy_busy_buffers_size ["#proxy buffer size"] * 2; 
    //xxx_buffersָ��Ϊ���պ�˷�����������࿪����ô��ռ䣬xxx_busy_buffers_sizeָ��һ�η��ͺ��п�������û��ȫ�����ͳ�ȥ����˷���busy����
    //��û�з��ͳ�ȥ��busy���е����ݴﵽxxx_busy_buffers_size�Ͳ��ܴӺ�˶�ȡ���ݣ�ֻ��busy���е����ݷ���һ���ֳ�ȥ��С��xxx_busy_buffers_size���ܼ�����ȡ
    { ngx_string("proxy_busy_buffers_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.busy_buffers_size_conf),
      NULL },

    { ngx_string("proxy_force_ranges"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.force_ranges),
      NULL },

    { ngx_string("proxy_limit_rate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.limit_rate),
      NULL },

#if (NGX_HTTP_CACHE)
/*
       nginx�Ĵ洢ϵͳ�����࣬һ����ͨ��proxy_store�����ģ��洢��ʽ�ǰ���url�е��ļ�·�����洢�ڱ��ء�����/file/2013/0001/en/test.html��
     ��ônginx�ͻ���ָ���Ĵ洢Ŀ¼�����ν�������Ŀ¼���ļ�����һ����ͨ��proxy_cache���������ַ�ʽ�洢���ļ����ǰ���url·������֯�ģ�
     ����ʹ��һЩ���ⷽʽ�������(�����Ϊ�Զ��巽ʽ)���Զ��巽ʽ��������Ҫ�ص�����ġ���ô�����ַ�ʽ����ʲô�����أ�


    ��url·���洢�ļ��ķ�ʽ�������������Ƚϼ򵥣��������ܲ��С������е�url�޳�������Ҫ�ڱ����ļ�ϵͳ�Ͻ���������Ŀ¼����ô�ļ��Ĵ�
    �Ͳ��Ҷ��ܻ����(����kernel��ͨ��·��������inode�Ĺ��̰�)�����ʹ���Զ��巽ʽ������ģʽ������Ҳ�벻���ļ���·����������������url����
    ���������������Ӻ����ܵĽ��͡���ĳ��������˵����һ���û�̬�ļ�ϵͳ������͵�Ӧ������squid�е�CFS��nginxʹ�õķ�ʽ��Լ򵥣���Ҫ����
    url��md5ֵ������
     */

    /*
     �﷨��proxy_cache zone_name; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
����һ��������������ƣ�һ����ͬ����������ڲ�ͬ�ĵط�ʹ�á�
��0.7.48�󣬻�����ѭ��˵�"Expires", "Cache-Control: no-cache", "Cache-Control: max-age=XXX"ͷ���ֶΣ�0.7.66�汾�Ժ�
"Cache-Control:"private"��"no-store"ͷͬ������ѭ��nginx�ڻ�������в��ᴦ��"Vary"ͷ��Ϊ��ȷ��һЩ˽�����ݲ������е��û�������
��˱������� "no-cache"����"max-age=0"ͷ������proxy_cache_key�����û�ָ����������$cookie_xxx��ʹ��cookie��ֵ��Ϊproxy_cache_key
��һ���ֿ��Է�ֹ����˽�����ݣ����Կ����ڲ�ͬ��location�зֱ�ָ��proxy_cache_key��ֵ�Ա�ֿ�˽�����ݺ͹������ݡ�
����ָ��������������(buffers)�����proxy_buffers����Ϊoff�����治����Ч��
*/ 
//xxx_cache(proxy_cache fastcgi_cache) abc����xxx_cache_path(proxy_cache_path fastcgi_cache_path) xxx keys_zone=abc:10m;һ�𣬷�����ngx_http_proxy_merge_loc_conf��ʧ�ܣ���Ϊû��Ϊ��abc����ngx_http_file_cache_t
//fastcgi_cache ָ��ָ�����ڵ�ǰ��������ʹ���ĸ�����ά��������Ŀ��������Ӧ�Ļ������������ fastcgi_cache_path ָ��塣 
//��ȡ�ýṹngx_http_upstream_cache_get��ʵ������ͨ��proxy_cache xxx����fastcgi_cache xxx����ȡ�����ڴ�����ģ���˱�������proxy_cache����fastcgi_cache

    { ngx_string("proxy_cache"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_cache,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
�﷨��proxy_cache_key line; 
Ĭ��ֵ��$scheme$proxy_host$request_uri; 
ʹ���ֶΣ�http, server, location 
ָ��ָ���˰����ڻ����еĻ���ؼ��֡�
proxy_cache_key "$host$request_uri$cookie_user";ע��Ĭ������·���������������û�а���������ؼ����У������Ϊ���վ���ڲ�ͬ��location��ʹ�ö�����
�������Ҫ�ڻ���ؼ����а�����������

proxy_cache_key "$scheme$host$request_uri";  
*/
    { ngx_string("proxy_cache_key"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_cache_key,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

/*
proxy_cache_path 
�﷨��proxy_cache_path path [levels=number] keys_zone=zone_name:zone_size [inactive=time] [max_size=size]; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http 
ָ��ָ�������·����һЩ������������������ݴ洢���ļ��У�����ʹ�ô���url�Ĺ�ϣֵ��Ϊ�ؼ������ļ�����levels����ָ���������Ŀ¼�������磺 
proxy_cache_path  /data/nginx/cache  levels=1:2   keys_zone=one:10m;�ļ��������ڣ�

/data/nginx/cache/c/29/b7f54b2df7773722d382f4809d65029c 
����ʹ�������1λ��2λ������ΪĿ¼�ṹ���� X, X:X,��X:X:X e.g.: "2", "2:2", "1:1:2"���������ֻ��������Ŀ¼��
���л��key��Ԫ���ݴ洢�ڹ�����ڴ���У����������keys_zone����ָ����
ע��ÿһ��������ڴ�ر����ǲ��ظ���·�������磺

proxy_cache_path  /data/nginx/cache/one    levels=1      keys_zone=one:10m;
proxy_cache_path  /data/nginx/cache/two    levels=2:2    keys_zone=two:100m;
proxy_cache_path  /data/nginx/cache/three  levels=1:1:2  keys_zone=three:1000m;�����inactive����ָ����ʱ���ڻ��������û�б�������ɾ����
Ĭ��inactiveΪ10���ӡ�
һ����Ϊcache manager�Ľ��̿��ƴ��̵Ļ����С����������ɾ������Ļ���Ϳ��ƻ����С����Щ����max_size�����ж��壬��Ŀǰ�����ֵ����max_size
ָ����ֵ֮�󣬳������С������ʹ�����ݣ�LRU�滻�㷨������ɾ����
�ڴ�صĴ�С���ջ���ҳ�����ı����������ã�һ��ҳ�棨�ļ�����Ԫ���ݴ�С���ղ���ϵͳ������FreeBSD/i386��Ϊ64�ֽڣ�FreeBSD/amd64��Ϊ128�ֽڡ�
proxy_cache_path��proxy_temp_pathӦ��ʹ������ͬ���ļ�ϵͳ�ϡ�

Proxy_cache_path������Ĵ洢·����������Ϣ��
  path �����ļ��ĸ�Ŀ¼��
  level=N:N��Ŀ¼�ĵڼ���hashĿ¼�������ݣ�
  keys_zone=name:size ���������ؽ����̽�������ʱ���ڴ���������ڴ��������ʹ�С��
  interval=timeǿ�Ƹ��»���ʱ�䣬�涨ʱ����û�з�������ڴ���ɾ����Ĭ��10s��
  max_size=sizeӲ���л������ݵ����ޣ���cache manager�������������LRU����ɾ����
  loader_sleep=time�����ؽ����������α��������ͣʱ����Ĭ��50ms��
  loader_files=number�ؽ�����ʱÿ�μ�������Ԫ�ص����ޣ����̵ݹ������ȡӲ���ϵĻ���Ŀ¼���ļ�����ÿ���ļ����ڴ��н���������ÿ
  ����һ��������Ϊ����һ������Ԫ�أ�ÿ�α���ʱ��ͬʱ���ض������Ԫ�أ�Ĭ��100��
*/  //XXX_cache��������д��xxx_temp_path���Ƶ�xxx_cache_path������������Ŀ¼�����ͬһ������
//xxx_cache(proxy_cache fastcgi_cache) abc����xxx_cache_path(proxy_cache_path fastcgi_cache_path) xxx keys_zone=abc:10m;һ�𣬷�����ngx_http_proxy_merge_loc_conf��ʧ�ܣ���Ϊû��Ϊ��abc����ngx_http_file_cache_t
//fastcgi_cache ָ��ָ�����ڵ�ǰ��������ʹ���ĸ�����ά��������Ŀ��������Ӧ�Ļ������������ fastcgi_cache_path ָ��塣 
//��ȡ�ýṹngx_http_upstream_cache_get��ʵ������ͨ��proxy_cache xxx����fastcgi_cache xxx����ȡ�����ڴ�����ģ���˱�������proxy_cache����fastcgi_cache

/*
�ǻ��淽ʽ(p->cacheable=0)p->temp_file->path = u->conf->temp_path; ��ngx_http_fastcgi_temp_pathָ��·��
���淽ʽ(p->cacheable=1) p->temp_file->path = r->cache->file_cache->temp_path;��proxy_cache_path����fastcgi_cache_pathָ��·�� 
��ngx_http_upstream_send_response 

��ǰfastcgi_buffers ��fastcgi_buffer_size���õĿռ䶼�Ѿ������ˣ�����Ҫ������д����ʱ�ļ���ȥ���ο�ngx_event_pipe_read_upstream
*/  //��ngx_http_file_cache_update���Կ��������������д����ʱ�ļ�����д��xxx_cache_path�У���ngx_http_file_cache_update
    { ngx_string("proxy_cache_path"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_2MORE,
      ngx_http_file_cache_set_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_proxy_main_conf_t, caches),
      &ngx_http_proxy_module },

/*
�﷨: proxy_cache_bypass line [��];
Ĭ��ֵ: off
ʹ���ֶ�: http, server, location
���ָ��ָ����ʹ�û��淵��Ӧ������������ָ���ı�����������һ��Ϊ�ǿգ����߲����ڡ�0�������Ӧ�𽫲��ӻ����з��أ�

 proxy_cache_bypass $cookie_nocache $ arg_nocache$arg_comment;
 proxy_cache_bypass $http_pragma $http_authorization;���Խ��proxy_no_cacheʹ�á�

 
Defines conditions under which the response will not be taken from a cache. If at least one value of the string parameters is not 
empty and is not equal to ��0�� then the response will not be taken from the cache: 
*/ //ע��proxy_no_cache��proxy_cache_proxy������
    { ngx_string("proxy_cache_bypass"), 
    //proxy_cache_bypass  xx1 xx2���õ�xx2��Ϊ�ջ��߲�Ϊ0���򲻻�ӻ�����ȡ������ֱ�ӳ��˶�ȡ  ������Щ����ĺ����Ӧ������Ȼ���Ա� upstream ģ�黺�档 
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_set_predicate_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_bypass),
      NULL },

/*
�﷨��proxy_no_cache variable1 variable2 ��; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
ȷ���ں�������»����Ӧ�𽫲���ʹ�ã�ʾ����

proxy_no_cache $cookie_nocache  $arg_nocache$arg_comment;
proxy_no_cache $http_pragma     $http_authorization;���Ϊ���ַ������ߵ���0�����ʽ��ֵ����false�����磬�����������У������
������������cookie ��nocache�����������Ǵ�������������͵���ˡ�
ע�⣺���Ժ�˵�Ӧ����Ȼ�п��ܷ��ϻ�����������һ�ַ������Կ��ٵĸ��»����е����ݣ��Ǿ��Ƿ���һ��ӵ�����Լ����������ͷ���ֶ�
���������磺My-Secret-Header����ô��proxy_no_cacheָ���п����������壺
proxy_no_cache $http_my_secret_header;

Defines conditions under which the response will not be saved to a cache. If at least one value of the string parameters is 
not empty and is not equal to ��0�� then the response will not be saved(ע����not be saved): 
*/ //ע��proxy_no_cache��proxy_cache_proxy������

    //proxy_cache_bypass  xx1 xx2���õ�xx2��Ϊ�ջ��߲�Ϊ0���򲻻�ӻ�����ȡ������ֱ�ӳ��˶�ȡ
    //proxy_no_cache  xx1 xx2���õ�xx2��Ϊ�ջ��߲�Ϊ0�����˻��������ݲ��ᱻ����
    { ngx_string("proxy_no_cache"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_set_predicate_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.no_cache),
      NULL },

/*
�﷨��proxy_cache_valid reply_code [reply_code ...] time; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
Ϊ��ͬ��Ӧ�����ò�ͬ�Ļ���ʱ�䣬���磺 
  proxy_cache_valid  200 302  10m;
  proxy_cache_valid  404      1m;ΪӦ�����Ϊ200��302�����û���ʱ��Ϊ10���ӣ�404���뻺��1���ӡ�
���ֻ����ʱ�䣺
 proxy_cache_valid 5m;��ôֻ�Դ���Ϊ200, 301��302��Ӧ����л��档
ͬ������ʹ��any�����κ�Ӧ�� 
  proxy_cache_valid  200 302 10m;
  proxy_cache_valid  301 1h;
  proxy_cache_valid  any 1m;
*/
    { ngx_string("proxy_cache_valid"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_file_cache_valid_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_valid),
      NULL },

/*
�﷨��proxy_cache_min_uses the_number; 
Ĭ��ֵ��proxy_cache_min_uses 1; 
ʹ���ֶΣ�http, server, location 
���ٴεĲ�ѯ��Ӧ�𽫱����棬Ĭ��1��
*/ //Proxy_cache_min_uses number Ĭ��Ϊ1�����ͻ��˷�����ͬ����ﵽ�涨������nginx�Ŷ���Ӧ���ݽ��л��棻
////��������Proxy_cache_min_uses 5������Ҫ�ͻ�������5�Ų��ܴӻ�����ȡ���������ֻ��4�Σ�����Ҫ�Ӻ�˻�ȡ����,���û�дﵽ5�Σ�����ngx_http_upstream_cache��� u->cacheable = 0;
    { ngx_string("proxy_cache_min_uses"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_min_uses),
      NULL },

/*
�﷨��proxy_cache_use_stale [error|timeout|updating|invalid_header|http_500|http_502|http_503|http_504|http_404|off] [...]; 
Ĭ��ֵ��proxy_cache_use_stale off; 
ʹ���ֶΣ�http, server, location 
���ָ�����nginx��ʱ�Ӵ��������ṩһ�����ڵ���Ӧ������������proxy_next_upstreamָ�
Ϊ�˷�ֹ����ʧЧ���ڶ���߳�ͬʱ���±��ػ���ʱ���������ָ��'updating'������������ֻ֤��һ���߳�ȥ���»��棬����������̸߳�
�»���Ĺ������������߳�ֻ����Ӧ��ǰ�����еĹ��ڰ汾��
*/
/*
�������������fastcgi_cache_use_stale updating����ʾ˵��Ȼ�û����ļ�ʧЧ�ˣ��Ѿ��������ͻ��������ڻ�ȡ������ݣ����Ǹÿͻ����������ڻ�û�л�ȡ������
��ʱ��Ϳ��԰���ǰ���ڵĻ��淢�͸���ǰ����Ŀͻ��� //�������ngx_http_upstream_cache�Ķ�
*/
    { ngx_string("proxy_cache_use_stale"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_use_stale),
      &ngx_http_proxy_next_upstream_masks },

/*
�﷨��proxy_cache_methods [GET HEAD POST]; 
Ĭ��ֵ��proxy_cache_methods GET HEAD; 
ʹ���ֶΣ�http, server, location 
GET/HEAD����װ����䣬�����޷�����GET/HEAD��ʹ��ֻʹ������������ã� 
proxy_cache_methods POST;
*/
    { ngx_string("proxy_cache_methods"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_methods),
      &ngx_http_upstream_cache_method_mask },

/*
When enabled, only one request at a time will be allowed to populate a new cache element identified according to the proxy_cache_key 
directive by passing a request to a proxied server. Other requests of the same cache element will either wait for a response to appear 
in the cache or the cache lock for this element to be released, up to the time set by the proxy_cache_lock_timeout directive. 


�����Ҫ���һ������:
���������������ͻ��ˣ�һ���ͻ������ڻ�ȡ������ݣ����Һ�˷�����һ���֣���nginx�Ỻ����һ���֣����ҵȴ����к�����ݷ��ؼ������档
�����ڻ���Ĺ���������ͻ���2ҳ������ȥͬ��������uri�ȶ�һ�������ȥ���ͻ��˻���һ������ݣ���ʱ��Ϳ���ͨ�������������������⣬
Ҳ���ǿͻ���1��û������ȫ�����ݵĹ����пͻ���2ֻ�еȿͻ���1��ȡ��ȫ��������ݣ����߻�ȡ��proxy_cache_lock_timeout��ʱ����ͻ���2ֻ�дӺ�˻�ȡ����
*/
    { ngx_string("proxy_cache_lock"), //Ĭ�Ϲر�
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_lock),
      NULL },

/*
If the last request passed to the proxied server for populating a new cache element has not completed for the specified time, one 
more request may be passed to the proxied server. 
*/
    { ngx_string("proxy_cache_lock_timeout"),  //Ĭ��5S
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_lock_timeout),
      NULL },

    { ngx_string("proxy_cache_lock_age"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_lock_age),
      NULL },

    { ngx_string("proxy_cache_revalidate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.cache_revalidate),
      NULL },

#endif

    /*
     �﷨��proxy_temp_path dir-path [ level1 [ level2 [ level3 ] ; 
     Ĭ��ֵ����configureʱ�ɨChttp-proxy-temp-pathָ�� 
     ʹ���ֶΣ�http, server, location 
     ������http����ģ���е�client_body_temp_pathָ�ָ��һ����ַ������Ƚϴ�ı���������
     */ //XXX_cache��������д��xxx_temp_path���Ƶ�xxx_cache_path������������Ŀ¼�����ͬһ������

         /*
Ĭ�������p->temp_file->path = u->conf->temp_path; Ҳ������ngx_http_fastcgi_temp_pathָ��·������������ǻ��淽ʽ(p->cacheable=1)��������
proxy_cache_path(fastcgi_cache_path) /a/b��ʱ�����use_temp_path=off(��ʾ��ʹ��ngx_http_fastcgi_temp_path���õ�path)��
��p->temp_file->path = r->cache->file_cache->temp_path; Ҳ������ʱ�ļ�/a/b/temp��use_temp_path=off��ʾ��ʹ��ngx_http_fastcgi_temp_path
���õ�·������ʹ��ָ������ʱ·��/a/b/temp   ��ngx_http_upstream_send_response 
*/ 
    /*������ݶ�ȡ��ϣ�����ȫ��д����ʱ�ļ���Ż�ִ��rename���̣�Ϊʲô��Ҫ��ʱ�ļ���ԭ����:����֮ǰ�Ļ�������ˣ������и��������ڴӺ��
    ��ȡ����д����ʱ�ļ��������ֱ��д�뻺���ļ������ڻ�ȡ������ݹ����У��������һ���ͻ��������������proxy_cache_use_stale updating����
    ������������ֱ�ӻ�ȡ֮ǰ�ϾɵĹ��ڻ��棬�Ӷ����Ա����ͻ(ǰ�������д�ļ�������������ȡ�ļ�����) 
    */

//��ngx_http_file_cache_update���Կ��������������д����ʱ�ļ�����д��xxx_cache_path�У���ngx_http_file_cache_update
    { ngx_string("proxy_temp_path"), //��ngx_http_file_cache_update���Կ��������������д����ʱ�ļ�����д��xxx_cache_path�У���ngx_http_file_cache_update
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1234,
      ngx_conf_set_path_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.temp_path),
      NULL },

    /*
     �﷨��proxy_max_temp_file_size size; 
     Ĭ��ֵ��proxy_max_temp_file_size 1G; 
     ʹ���ֶΣ�http, server, location, if 
     ��������������ʱʹ��һ����ʱ�ļ������ֵ������ļ��������ֵ����ͬ�������������д����̽��л��档
     ������ֵ����Ϊ�㣬���ֹʹ����ʱ�ļ���
     */
    { ngx_string("proxy_max_temp_file_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.max_temp_file_size_conf),
      NULL },

    /*
     �﷨��proxy_temp_file_write_size size; 
     Ĭ��ֵ��proxy_temp_file_write_size [��#proxy buffer size��] * 2; 
     ʹ���ֶΣ�http, server, location, if 
     ������д��proxy_temp_pathʱ���ݵĴ�С����Ԥ��һ�����������ڴ����ļ�ʱ����̫����
     */
    { ngx_string("proxy_temp_file_write_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.temp_file_write_size_conf),
      NULL },

/*
�﷨��proxy_next_upstream [error | timeout | invalid_header | http_500 | http_502 | http_503 | http_504 | http_404 | off ];

Ĭ�ϣ�proxy_next_upstream error timeout;

���ÿ飺http��server��location

���������ʾ����һ̨���η�����ת��������ִ���ʱ��������һ̨���η�������������������η�����һ����ʼ����Ӧ��
Nginx�����������������̰�Ӧ���ת�����ͻ��ˡ���ˣ�һ��Nginx��ʼ��ͻ��˷�����Ӧ����֮��Ĺ����������ִ���Ҳ�ǲ�������һ̨
���η�������������ġ���ܺ���⣬�����ſ��Ը��õر�֤�ͻ���ֻ�յ�����һ�����η�������Ӧ��proxy_next_upstream�Ĳ�������˵��
����Щ����»����ѡ����һ̨���η�����ת������

error���������η������������ӡ��������󡢶�ȡ��Ӧʱ����

timeout������������ȡ��Ӧʱ������ʱ��
invalid_header�����η��������͵���Ӧ�ǲ��Ϸ��ġ�
http_500�����η��������ص�HTTP��Ӧ����500��
http_502�����η��������ص�HTTP��Ӧ����502��
http_503�����η��������ص�HTTP��Ӧ����503��
http_504�����η��������ص�HTTP��Ӧ����504��
http_404�����η��������ص�HTTP��Ӧ����404��
off���ر�proxy_next_upstream���ܡ������ѡ����һ̨���η������ٴ�ת����
*/
    { ngx_string("proxy_next_upstream"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.next_upstream),
      &ngx_http_proxy_next_upstream_masks },

    { ngx_string("proxy_next_upstream_tries"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.next_upstream_tries),
      NULL },

    { ngx_string("proxy_next_upstream_timeout"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.next_upstream_timeout),
      NULL },

/*
proxy_pass_header
�﷨��proxy_pass_header the_header;
���ÿ飺http��server��location
��proxy_hide_header�����෴��proxy_pass_header�Ὣԭ����ֹת����header����Ϊ����ת�������磺
proxy_pass_header X-Accel-Redirect;
*/
    { ngx_string("proxy_pass_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.pass_headers),
      NULL },

/*
proxy_hide_header
�﷨��proxy_hide_header the_header;
���ÿ飺http��server��location
Nginx�Ὣ���η���������Ӧת�����ͻ��ˣ���Ĭ�ϲ���ת������HTTPͷ���ֶΣ�Date��Server��X-Pad��X-Accel-*��ʹ��proxy_hide_header�����
�����ָ����ЩHTTPͷ���ֶβ��ܱ�ת�������磺
proxy_hide_header Cache-Control;
proxy_hide_header MicrosoftOfficeWebServer;

�﷨��proxy_hide_header the_header 
ʹ���ֶΣ�http, server, location 
nginx���Դӱ����������������"Date", "Server", "X-Pad"��"X-Accel-..."Ӧ�����ת�������������������һЩ������ͷ���ֶΣ��������
�����ᵽ��ͷ���ֶα��뱻ת��������ʹ��proxy_pass_headerָ����磺��Ҫ����MS-OfficeWebserver��AspNet-Version����ʹ���������ã� 
location / {
  proxy_hide_header X-AspNet-Version;
  proxy_hide_header MicrosoftOfficeWebServer;
}��ʹ��X-Accel-Redirectʱ���ָ��ǳ����á����磬�����Ҫ�ں��Ӧ�÷�������һ����Ҫ���ص��ļ�����һ������ͷ������X-Accel-Redirect
�ֶμ�Ϊ����ļ���ͬʱҪ��ǡ����Content-Type�����ǣ��ض����URL��ָ���������ļ����ļ�����������������������������Լ���Content-Type��
�����Ⲣ������ȷ�ģ������ͺ����˺��Ӧ�÷��������ݵ�Content-Type��Ϊ�˱���������������ʹ�����ָ� 
location / {
  proxy_pass http://backend_servers;
}
 
location /files/ {
  proxy_pass http://fileserver;
  proxy_hide_header Content-Type;

*/ //proxy_hide_header��proxy_pass_header�����෴
    { ngx_string("proxy_hide_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.hide_headers),
      NULL },

    /*
     �﷨��proxy_ignore_headers name [name ��] 
     Ĭ��ֵ��none 
     ʹ���ֶΣ�http, server, location 
     ���ָ��(0.7.54+) ��ֹ�������Դ����������Ӧ��
     ����ָ�����ֶ�Ϊ��X-Accel-Redirect��, ��X-Accel-Expires��, ��Expires����Cache-Control����
     */
    { ngx_string("proxy_ignore_headers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ignore_headers),
      &ngx_http_upstream_ignore_headers_masks },

    //����˷��������õ�http�汾�ţ�Ĭ��1.0����ngx_http_proxy_create_request
    { ngx_string("proxy_http_version"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, http_version),
      &ngx_http_proxy_http_version },

#if (NGX_HTTP_SSL)

    /*
     proxy_ssl_session_reuse
     �﷨��proxy_ssl_session_reuse [ on | off ];
     Ĭ��ֵ�� proxy_ssl_session_reuse on;
     ʹ���ֶΣ�http, server, location
     ʹ�ð汾���� 0.7.11
     ��ʹ��https���ӵ����η�����ʱ��������ssl�Ự��
     */
    { ngx_string("proxy_ssl_session_reuse"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ssl_session_reuse),
      NULL },

    { ngx_string("proxy_ssl_protocols"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_conf_set_bitmask_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_protocols),
      &ngx_http_proxy_ssl_protocols },

    { ngx_string("proxy_ssl_ciphers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_ciphers),
      NULL },

    { ngx_string("proxy_ssl_name"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ssl_name),
      NULL },

    { ngx_string("proxy_ssl_server_name"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ssl_server_name),
      NULL },

    { ngx_string("proxy_ssl_verify"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, upstream.ssl_verify),
      NULL },

    { ngx_string("proxy_ssl_verify_depth"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_verify_depth),
      NULL },

    { ngx_string("proxy_ssl_trusted_certificate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_trusted_certificate),
      NULL },

    { ngx_string("proxy_ssl_crl"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_crl),
      NULL },

    { ngx_string("proxy_ssl_certificate"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_certificate),
      NULL },

    { ngx_string("proxy_ssl_certificate_key"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_proxy_loc_conf_t, ssl_certificate_key),
      NULL },

    { ngx_string("proxy_ssl_password_file"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_proxy_ssl_password_file,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

#endif

      ngx_null_command
};


static ngx_http_module_t  ngx_http_proxy_module_ctx = {
    ngx_http_proxy_add_variables,          /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_proxy_create_main_conf,       /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_proxy_create_loc_conf,        /* create location configuration */
    ngx_http_proxy_merge_loc_conf          /* merge location configuration */
};


ngx_module_t  ngx_http_proxy_module = {
    NGX_MODULE_V1,
    &ngx_http_proxy_module_ctx,              /* module context */
    ngx_http_proxy_commands,                 /* module directives */
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


static char  ngx_http_proxy_version[] = " HTTP/1.0" CRLF;
static char  ngx_http_proxy_version_11[] = " HTTP/1.1" CRLF;


static ngx_keyval_t  ngx_http_proxy_headers[] = {
    { ngx_string("Host"), ngx_string("$proxy_host") },
    { ngx_string("Connection"), ngx_string("close") },
    { ngx_string("Content-Length"), ngx_string("$proxy_internal_body_length") },
    { ngx_string("Transfer-Encoding"), ngx_string("$proxy_internal_chunked") },
    { ngx_string("TE"), ngx_string("") }, //�������valueΪ�գ������proxy_set_header�����������ˣ����ǻ���Ч����������Ч����ngx_http_proxy_init_headers
    { ngx_string("Keep-Alive"), ngx_string("") },
    { ngx_string("Expect"), ngx_string("") },
    { ngx_string("Upgrade"), ngx_string("") },
    { ngx_null_string, ngx_null_string }
};

//������ӵ���ngx_http_upstream_conf_t->hide_headers_hash���� ����Ҫ���͸��ͻ���
static ngx_str_t  ngx_http_proxy_hide_headers[] = {
    ngx_string("Date"),
    ngx_string("Server"),
    ngx_string("X-Pad"),
    ngx_string("X-Accel-Expires"),
    ngx_string("X-Accel-Redirect"),
    ngx_string("X-Accel-Limit-Rate"),
    ngx_string("X-Accel-Buffering"),
    ngx_string("X-Accel-Charset"),
    ngx_null_string
};


#if (NGX_HTTP_CACHE)

static ngx_keyval_t  ngx_http_proxy_cache_headers[] = {
    { ngx_string("Host"), ngx_string("$proxy_host") },
    { ngx_string("Connection"), ngx_string("close") },
    { ngx_string("Content-Length"), ngx_string("$proxy_internal_body_length") },
    { ngx_string("Transfer-Encoding"), ngx_string("$proxy_internal_chunked") },
    { ngx_string("TE"), ngx_string("") },
    { ngx_string("Keep-Alive"), ngx_string("") },
    { ngx_string("Expect"), ngx_string("") },
    { ngx_string("Upgrade"), ngx_string("") },
    { ngx_string("If-Modified-Since"),
      ngx_string("$upstream_cache_last_modified") },
    { ngx_string("If-Unmodified-Since"), ngx_string("") },
    { ngx_string("If-None-Match"), ngx_string("$upstream_cache_etag") },
    { ngx_string("If-Match"), ngx_string("") },
    { ngx_string("Range"), ngx_string("") },
    { ngx_string("If-Range"), ngx_string("") },
    { ngx_null_string, ngx_null_string }
};

#endif


/*
��ģ���а���һЩ���ñ�������������proxy_set_headerָ�����Դ���ͷ����

$proxy_add_x_forwarded_for
�����ͻ��˵�����ͷ��X-Forwarded-For����$remote_addr���ö��ŷֿ������������X-Forwarded-For����ͷ����$proxy_add_x_forwarded_for����$remote_addr��

$proxy_host
���������������������˿ںš�

$proxy_internal_body_length
ͨ��proxy_set_body���õĴ�������ʵ��ĳ��ȡ�

$proxy_host
������������Ķ˿ں�
*/
static ngx_http_variable_t  ngx_http_proxy_vars[] = {

    { ngx_string("proxy_host"), NULL, ngx_http_proxy_host_variable, 0,
      NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("proxy_port"), NULL, ngx_http_proxy_port_variable, 0,
      NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("proxy_add_x_forwarded_for"), NULL,
      ngx_http_proxy_add_x_forwarded_for_variable, 0, NGX_HTTP_VAR_NOHASH, 0 },

#if 0
    { ngx_string("proxy_add_via"), NULL, NULL, 0, NGX_HTTP_VAR_NOHASH, 0 },
#endif

    { ngx_string("proxy_internal_body_length"), NULL,
      ngx_http_proxy_internal_body_length_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("proxy_internal_chunked"), NULL,
      ngx_http_proxy_internal_chunked_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_path_init_t  ngx_http_proxy_temp_path = {
    ngx_string(NGX_HTTP_PROXY_TEMP_PATH), { 1, 2, 0 }
};

//����proxy_pass����ngx_http_core_content_phase����ָ��ú���
/*
��ô������������ʵ��ض���location��ʱ��(�������location������proxy_passָ��)��
����������һ��������ø���phase��checker��handler������NGX_HTTP_CONTENT_PHASE��checker��
��ngx_http_core_content_phase()��ʱ�򣬻����r->content_handler(r)����ngx_http_proxy_handler��
*/
static ngx_int_t ngx_http_proxy_handler(ngx_http_request_t *r)
{
    ngx_int_t                    rc;
    ngx_http_upstream_t         *u;
    ngx_http_proxy_ctx_t        *ctx;
    ngx_http_proxy_loc_conf_t   *plcf;
#if (NGX_HTTP_CACHE)
    ngx_http_proxy_main_conf_t  *pmcf;
#endif

    if (ngx_http_upstream_create(r) != NGX_OK) { //����һ��ngx_http_upstream_t�ṹ����ֵ��r->upstream
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_proxy_ctx_t));
    if (ctx == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    //����������ķŵ������ctx�����ngx_http_proxy_moduleģ���У�r->ctx[module.ctx_index] = c;
    ngx_http_set_ctx(r, ctx, ngx_http_proxy_module);

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_module);

    u = r->upstream;

    if (plcf->proxy_lengths == NULL) {
        ctx->vars = plcf->vars;
        u->schema = plcf->vars.schema;
#if (NGX_HTTP_SSL)
        u->ssl = (plcf->upstream.ssl != NULL);
#endif

    } else {
        if (ngx_http_proxy_eval(r, ctx, plcf) != NGX_OK) { 
        //��ȡuri�б�����ֵ���Ӷ����Եõ�������uri����ngx_http_proxy_pass����Ķ�
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    u->output.tag = (ngx_buf_tag_t) &ngx_http_proxy_module;

    u->conf = &plcf->upstream;

#if (NGX_HTTP_CACHE)
    pmcf = ngx_http_get_module_main_conf(r, ngx_http_proxy_module);

    u->caches = &pmcf->caches;
    u->create_key = ngx_http_proxy_create_key;
#endif

    //Ϊupstream׼����������Ļص�
    u->create_request = ngx_http_proxy_create_request; //���ɷ��͵����η����������󻺳壨����һ������������Ҳ����Ҫ�������ε�����
    u->reinit_request = ngx_http_proxy_reinit_request;
    //����ص����ǵ�һ�еĻص�����һ�д�����������Ϊngx_http_proxy_process_header������һ��
    u->process_header = ngx_http_proxy_process_status_line;
    //һ��upstream��u->read_event_handler ���¼��ص�������Ϊngx_http_upstream_process_header;�����᲻�ϵĶ�ȡ���ݣ�Ȼ��
    //����process_header����FCGI����Ȼ�ǵ��ö�Ӧ�Ķ�ȡFCGI��ʽ�ĺ����ˣ����ڴ���ģ�飬ֻҪ����HTTP��ʽ����
    u->abort_request = ngx_http_proxy_abort_request;
    u->finalize_request = ngx_http_proxy_finalize_request;
    r->state = 0;

    if (plcf->redirects) {
        u->rewrite_redirect = ngx_http_proxy_rewrite_redirect;
    }

    if (plcf->cookie_domains || plcf->cookie_paths) {
        u->rewrite_cookie = ngx_http_proxy_rewrite_cookie;
    }

    u->buffering = plcf->upstream.buffering;

    u->pipe = ngx_pcalloc(r->pool, sizeof(ngx_event_pipe_t));
    if (u->pipe == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    //��������buffering��״̬�£�nginx��ȡupstream�Ļص��������FCGI�����Ƕ�Ӧ�Ļص�ngx_http_fastcgi_input_filter��������FCGIЭ������ݡ�
	//�������Ҫʵ�������Լ���Э���ʽ���Ǿ��ö�Ӧ�Ľ�����ʽ��
    u->pipe->input_filter = ngx_http_proxy_copy_filter;
    u->pipe->input_ctx = r;

    //buffering�����Ӧ����ʹ��ngx_event_pipe_t->input_filter  ��buffering��ʽ��Ӧ��˰���ʹ��ngx_http_upstream_s->input_filter ,��ngx_http_upstream_send_response�ֲ�

    u->input_filter_init = ngx_http_proxy_input_filter_init;
    u->input_filter = ngx_http_proxy_non_buffered_copy_filter;
    u->input_filter_ctx = r;

    u->accel = 1;

    if (!plcf->upstream.request_buffering
        && plcf->body_values == NULL && plcf->upstream.pass_request_body
        && (!r->headers_in.chunked
            || plcf->http_version == NGX_HTTP_VERSION_11))
    {
        r->request_body_no_buffering = 1;
    }

    /*
���Ķ�HTTP�������ģ��(ngx_http_proxy_module)Դ����ʱ���ᷢ������û�е���r->main->count++������proxyģ������������upstream���Ƶģ�
ngx_http_read_client_request_body(r��ngx_http_upstream_init);�����ʾ��ȡ���û������HTTP�����Ż����ngx_http_upstream_init����
����upstream���ơ�����ngx_http_read_client_request_body�ĵ�һ����Ч�����r->maln->count++������HTTP�������ģ�鲻��
�ٴ����������ִ��r->main->count++��

������̿������ƺ���������Ϊʲô��ʱ��Ҫ�����ü�����1����ʱȴ����Ҫ�أ���Ϊngx_http_read- client_request_body��ȡ���������
һ���첽��������Ҫepoll��ε��ȷ�����ɵĿɳ���Ϊ�첽��������ngx_http_upstream_init��������upstream����Ҳ��һ���첽��������ˣ�
����������˵��ÿִ��һ���첽����Ӧ�ð����ü�����1�����첽��������ʱӦ�õ���ngx_http_finalize_request���������ü�����1�����⣬
ngx_http_read_client_request_body�������Ǽӹ����ü����ģ���ngx_http_upstream_init������ȴû�мӹ����ü���������Nginx�������޸�
������⣩����HTTP�������ģ���У�����ngx_http_proxy_handler�������á�ngx_http_read- client_request_body(r��ngx_http_upstream_init);��
���ͬʱ�����������첽������ע�⣬���������ֻ����һ�����ü�����ִ����������ngx_http_proxy_handler��������ʱֻ����
ngx_http_finalize_request����һ�Σ�������ȷ�ġ�����mytestģ��Ҳһ�������Ҫ��֤�����ü��������Ӻͼ�������Խ��еġ�
    */
    rc = ngx_http_read_client_request_body(r, ngx_http_upstream_init);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }

    return NGX_DONE;
}


static ngx_int_t
ngx_http_proxy_eval(ngx_http_request_t *r, ngx_http_proxy_ctx_t *ctx,
    ngx_http_proxy_loc_conf_t *plcf)
{
    u_char               *p;
    size_t                add;
    u_short               port;
    ngx_str_t             proxy;
    ngx_url_t             url;
    ngx_http_upstream_t  *u;

    if (ngx_http_script_run(r, &proxy, plcf->proxy_lengths->elts, 0,
                            plcf->proxy_values->elts)
        == NULL)
    {
        return NGX_ERROR;
    }

    if (proxy.len > 7
        && ngx_strncasecmp(proxy.data, (u_char *) "http://", 7) == 0)
    {
        add = 7;
        port = 80;

#if (NGX_HTTP_SSL)

    } else if (proxy.len > 8
               && ngx_strncasecmp(proxy.data, (u_char *) "https://", 8) == 0)
    {
        add = 8;
        port = 443;
        r->upstream->ssl = 1;

#endif

    } else {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid URL prefix in \"%V\"", &proxy);
        return NGX_ERROR;
    }

    u = r->upstream;

    u->schema.len = add;
    u->schema.data = proxy.data;

    ngx_memzero(&url, sizeof(ngx_url_t));

    url.url.len = proxy.len - add;
    url.url.data = proxy.data + add;
    url.default_port = port;
    url.uri_part = 1;
    url.no_resolve = 1;

    if (ngx_parse_url(r->pool, &url) != NGX_OK) {
        if (url.err) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "%s in upstream \"%V\"", url.err, &url.url);
        }

        return NGX_ERROR;
    }

    if (url.uri.len) {
        if (url.uri.data[0] == '?') {
            p = ngx_pnalloc(r->pool, url.uri.len + 1);
            if (p == NULL) {
                return NGX_ERROR;
            }

            *p++ = '/';
            ngx_memcpy(p, url.uri.data, url.uri.len);

            url.uri.len++;
            url.uri.data = p - 1;
        }
    }

    ctx->vars.key_start = u->schema;

    ngx_http_proxy_set_vars(&url, &ctx->vars);

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
        u->resolved->port = (in_port_t) (url.no_port ? port : url.port);
        u->resolved->no_port = url.no_port;
    }

    return NGX_OK;
}


#if (NGX_HTTP_CACHE)

//����proxy_cache_key xxx ����ֵ��r->cache->keys
static ngx_int_t //ngx_http_upstream_cache��ִ��
ngx_http_proxy_create_key(ngx_http_request_t *r)
{
    size_t                      len, loc_len;
    u_char                     *p;
    uintptr_t                   escape;
    ngx_str_t                  *key;
    ngx_http_upstream_t        *u;
    ngx_http_proxy_ctx_t       *ctx;
    ngx_http_proxy_loc_conf_t  *plcf;

    u = r->upstream;

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_module);

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    key = ngx_array_push(&r->cache->keys);
    if (key == NULL) {
        return NGX_ERROR;
    }

    if (plcf->cache_key.value.data) { //���������proxy_cache_key

        if (ngx_http_complex_value(r, &plcf->cache_key, key) != NGX_OK) { //����plcf->cache_key�еı���valueֵ��r->cache->keys��
            return NGX_ERROR;
        }

        return NGX_OK;
    }

    //���û������proxy_cache_key����ʹ��Ĭ��Default:  proxy_cache_key $scheme$proxy_host$request_uri; 
    /* 
    
     Syntax:  proxy_cache_key string;
      
     Default:  proxy_cache_key $scheme$proxy_host$request_uri; 
     Context:  http, server, location
     */
    
    *key = ctx->vars.key_start;

    key = ngx_array_push(&r->cache->keys); //������Щ�����ǽ���Ĭ�ϵ� proxy_cache_key $scheme$proxy_host$request_uri; �浽keys����
    if (key == NULL) {
        return NGX_ERROR;
    }

    if (plcf->proxy_lengths && ctx->vars.uri.len) {

        *key = ctx->vars.uri;
        u->uri = ctx->vars.uri;

        return NGX_OK;

    } else if (ctx->vars.uri.len == 0 && r->valid_unparsed_uri && r == r->main)
    {
        *key = r->unparsed_uri;
        u->uri = r->unparsed_uri;

        return NGX_OK;
    }

    loc_len = (r->valid_location && ctx->vars.uri.len) ? plcf->location.len : 0;

    if (r->quoted_uri || r->internal) {
        escape = 2 * ngx_escape_uri(NULL, r->uri.data + loc_len,
                                    r->uri.len - loc_len, NGX_ESCAPE_URI);
    } else {
        escape = 0;
    }

    len = ctx->vars.uri.len + r->uri.len - loc_len + escape
          + sizeof("?") - 1 + r->args.len;

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    key->data = p;

    if (r->valid_location) {
        p = ngx_copy(p, ctx->vars.uri.data, ctx->vars.uri.len);
    }

    if (escape) {
        ngx_escape_uri(p, r->uri.data + loc_len,
                       r->uri.len - loc_len, NGX_ESCAPE_URI);
        p += r->uri.len - loc_len + escape;

    } else {
        p = ngx_copy(p, r->uri.data + loc_len, r->uri.len - loc_len);
    }

    if (r->args.len > 0) {
        *p++ = '?';
        p = ngx_copy(p, r->args.data, r->args.len);
    }

    key->len = p - key->data;
    u->uri = *key;

    return NGX_OK;
}

#endif

/*
  proxy_pass  http://10.10.0.103:8080/tttxxsss; 
  ���������:http://10.2.13.167/proxy1111/yangtest
  ʵ���ϵ���˵�uri���Ϊ/tttxxsss/yangtest
  plcf->location:/proxy1111, ctx->vars.uri:/tttxxsss,  r->uri:/proxy1111/yangtest, r->args:, urilen:19
*/

/*
�����FCGI�������齨��FCGI�ĸ���ͷ������������ʼͷ���������ͷ������STDINͷ�������u->request_bufs���ӱ����档
�����Proxyģ�飬ngx_http_proxy_create_request�����������ͷ��ɶ��,�ŵ�u->request_bufs����
FastCGI memcached  uwsgi  scgi proxy�����õ�upstreamģ��
 */
static ngx_int_t
ngx_http_proxy_create_request(ngx_http_request_t *r) //ngx_http_upstream_init_request��ִ��
{
    size_t                          len, 
                                    uri_len, 
                                    loc_len,  //��ǰlocation������ location xxx {} �е�xxx�ĳ���3
                                    body_len;
    uintptr_t                     escape;
    ngx_buf_t                    *b;
    ngx_str_t                     method;
    ngx_uint_t                    i, unparsed_uri;
    ngx_chain_t                  *cl, *body;
    ngx_list_part_t              *part;
    ngx_table_elt_t              *header;
    ngx_http_upstream_t          *u;
    ngx_http_proxy_ctx_t         *ctx;
    ngx_http_script_code_pt       code;
    ngx_http_proxy_headers_t     *headers;
    ngx_http_script_engine_t      e, le;
    ngx_http_proxy_loc_conf_t    *plcf;
    ngx_http_script_len_code_pt   lcode;

    u = r->upstream;//�õ���ngx_http_proxy_handlerǰ�洴�������νṹ

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_module); //��ȡproxyģ������������Ϣ

#if (NGX_HTTP_CACHE)
    headers = u->cacheable ? &plcf->headers_cache : &plcf->headers;
#else
    headers = &plcf->headers;
#endif

    if (u->method.len) {
        /* �ͻ���head����ᱻת��ΪGET����� HEAD was changed to GET to cache response */
        method = u->method;
        method.len++;

    } else if (plcf->method.len) {
        method = plcf->method;

    } else {
        method = r->method_name;
        method.len++;
    }

    //��ȡproxyģ��״̬����������Ϣ��
    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (method.len == 5
        && ngx_strncasecmp(method.data, (u_char *) "HEAD ", 5) == 0)
    {
        ctx->head = 1;
    }

    //���治�ϵĶ����ݳ��Ƚ����ۼӣ����Ҫ���͵���˵������ܹ��ж೤������len��������
    len = method.len + sizeof(ngx_http_proxy_version) - 1 + sizeof(CRLF) - 1; //ͷ���г���

    escape = 0;
    loc_len = 0;
    unparsed_uri = 0;

    //���洦��һ��uri�����Լ�location
    if (plcf->proxy_lengths && ctx->vars.uri.len) {//proxy_pass xxxx����
        uri_len = ctx->vars.uri.len;

    }  else if (ctx->vars.uri.len == 0 && r->valid_unparsed_uri && r == r->main) {
        unparsed_uri = 1;
        uri_len = r->unparsed_uri.len;

    } else {
        loc_len = (r->valid_location && ctx->vars.uri.len) ?
                      plcf->location.len : 0;  //��ǰlocation������ location xxx {} �е�xxx�ĳ���3

        if (r->quoted_uri || r->space_in_uri || r->internal) {
            escape = 2 * ngx_escape_uri(NULL, r->uri.data + loc_len,
                                        r->uri.len - loc_len, NGX_ESCAPE_URI);
        }

        /*
           proxy_pass  http://10.10.0.103:8080/tttxxsss; 
           ���������:http://10.2.13.167/proxy1111/yangtest
           ʵ���ϵ���˵�uri���Ϊ/tttxxsss/yangtest
           plcf->location:/proxy1111, ctx->vars.uri:/tttxxsss,  r->uri:/proxy1111/yangtest, r->args:, urilen:19
          */
        uri_len = ctx->vars.uri.len + r->uri.len - loc_len + escape
                  + sizeof("?") - 1 + r->args.len; //ע��sizeof("?")��2�ֽ�
    }

    if (uri_len == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "zero length URI to proxy");
        return NGX_ERROR;
    }

    len += uri_len;//�ۼӵ�һ�У���: GET /XXX/Y.html HTTP/1.0

    ngx_memzero(&le, sizeof(ngx_http_script_engine_t));

    ngx_http_script_flush_no_cacheable_variables(r, plcf->body_flushes);
    ngx_http_script_flush_no_cacheable_variables(r, headers->flushes);

    if (plcf->body_lengths) { //����proxy_set_body ���õ�body�ַ�������
        le.ip = plcf->body_lengths->elts;
        le.request = r;
        le.flushed = 1;
        body_len = 0;

        while (*(uintptr_t *) le.ip) {
            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            body_len += lcode(&le);
        }

        ctx->internal_body_length = body_len; //���������proxy_set_body����internal_body_lengthΪproxy_set_body���õİ��峤�ȣ�����Ϊ�ͻ���������峤��
        len += body_len;

    } else if (r->headers_in.chunked && r->reading_body) {
        ctx->internal_body_length = -1;
        ctx->internal_chunked = 1;

    } else {
        //���������proxy_set_body����internal_body_lengthΪproxy_set_body���õİ��峤�ȣ�����Ϊ�ͻ���������峤��
        ctx->internal_body_length = r->headers_in.content_length_n;
    }

    //ngx_http_proxy_headers��proxy_set_header���õ�ͷ����Ϣ���������key:valueֵ���Ⱥ�
    le.ip = headers->lengths->elts;
    le.request = r;
    le.flushed = 1;

    while (*(uintptr_t *) le.ip) {
        while (*(uintptr_t *) le.ip) {
            lcode = *(ngx_http_script_len_code_pt *) le.ip;
            len += lcode(&le);
        }
        le.ip += sizeof(uintptr_t);
    }

    //�ѿͻ��˷��͹�����ͷ����key:valueҲ���������ע������ǰ���ngx_http_proxy_headers��proxy_set_header��ӵ��ظ�
    if (plcf->upstream.pass_request_headers) {//�Ƿ�Ҫ��HTTP����ͷ����HEADER���͸���ˣ���HTTP_Ϊǰ׺
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

            /* ��ngx_http_proxy_loc_conf_t->headers�в����Ƿ���ڿͻ����������е�ͷ������Ϣr->headers_in.headers�� �ҵ�˵�����ظ���������len*/
            if (ngx_hash_find(&headers->hash, header[i].hash,
                              header[i].lowcase_key, header[i].key.len))
            {
                continue;
            }

            len += header[i].key.len + sizeof(": ") - 1
                + header[i].value.len + sizeof(CRLF) - 1; //����ngx_http_proxy_headers proxy_set_header���õ�ͷ�����Լ��ͻ��˷��͹�����ͷ���е�key:value�ܳ���
        }
    }

    b = ngx_create_temp_buf(r->pool, len);
    if (b == NULL) {
        return NGX_ERROR;
    }

    cl = ngx_alloc_chain_link(r->pool);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    cl->buf = b;//�ϰ취������黺��������ӱ�ͷ��


    /* �����ⲿ�ְ�GET /XXX/Y.html HTTP/1.0������buf�ڴ� */
    /* the request line */
    b->last = ngx_copy(b->last, method.data, method.len); //�Ȱ��������е�GET POST HEAD�ȷ���������b�ڴ�ռ�

    u->uri.data = b->last;

    if (plcf->proxy_lengths && ctx->vars.uri.len) {
        b->last = ngx_copy(b->last, ctx->vars.uri.data, ctx->vars.uri.len);

    } else if (unparsed_uri) {
        b->last = ngx_copy(b->last, r->unparsed_uri.data, r->unparsed_uri.len);

    } else {
        if (r->valid_location) {
            b->last = ngx_copy(b->last, ctx->vars.uri.data, ctx->vars.uri.len);
        }

        if (escape) { //���ո����ת�ƣ�
            ngx_escape_uri(b->last, r->uri.data + loc_len,
                           r->uri.len - loc_len, NGX_ESCAPE_URI);
            b->last += r->uri.len - loc_len + escape;

        } else {
            b->last = ngx_copy(b->last, r->uri.data + loc_len,
                               r->uri.len - loc_len);
        }

        if (r->args.len > 0) {
            *b->last++ = '?';
            b->last = ngx_copy(b->last, r->args.data, r->args.len);
        }
    }

    u->uri.len = b->last - u->uri.data;

    if (plcf->http_version == NGX_HTTP_VERSION_11) {
        b->last = ngx_cpymem(b->last, ngx_http_proxy_version_11,
                             sizeof(ngx_http_proxy_version_11) - 1);

    } else {
        b->last = ngx_cpymem(b->last, ngx_http_proxy_version,
                             sizeof(ngx_http_proxy_version) - 1);
    }


    /* ����ngx_http_proxy_headers  proxy_set_header�����õ�ͷ����Ϣkey:value��b�ڴ��� */
    ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

    e.ip = headers->values->elts;
    e.pos = b->last;
    e.request = r;
    e.flushed = 1;

    le.ip = headers->lengths->elts;

    while (*(uintptr_t *) le.ip) {
        lcode = *(ngx_http_script_len_code_pt *) le.ip;

        /* skip the header line name length */
        (void) lcode(&le);

        if (*(ngx_http_script_len_code_pt *) le.ip) {

            for (len = 0; *(uintptr_t *) le.ip; len += lcode(&le)) {
                lcode = *(ngx_http_script_len_code_pt *) le.ip;
            }

            e.skip = (len == sizeof(CRLF) - 1) ? 1 : 0;

        } else {
            e.skip = 0;
        }

        le.ip += sizeof(uintptr_t);

        while (*(uintptr_t *) e.ip) {
            code = *(ngx_http_script_code_pt *) e.ip;
            code((ngx_http_script_engine_t *) &e);
        }
        e.ip += sizeof(uintptr_t);
    }

    b->last = e.pos;


     /* �����ͻ���������ͷ������Ϣkey:value��b�ڴ��� */
    if (plcf->upstream.pass_request_headers) {
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

            //ngx_http_proxy_headers  proxy_set_header�����õ�ͷ����Ϣ�Ϳͻ��˷��͹������ظ����Ͳ���Ҫ������
            if (ngx_hash_find(&headers->hash, header[i].hash,
                              header[i].lowcase_key, header[i].key.len))
            {
                continue;
            }

            b->last = ngx_copy(b->last, header[i].key.data, header[i].key.len);

            *b->last++ = ':'; *b->last++ = ' ';

            b->last = ngx_copy(b->last, header[i].value.data,
                               header[i].value.len);

            *b->last++ = CR; *b->last++ = LF;

            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http proxy header: \"%V: %V\"",
                           &header[i].key, &header[i].value);
        }
    }


    /* add "\r\n" at the header end */
    *b->last++ = CR; *b->last++ = LF;//��һ�����н���

    //������ָproxy_set_body�������ӵ�body������ӵ�b�ڴ���
    if (plcf->body_values) {
        e.ip = plcf->body_values->elts;
        e.pos = b->last;
        e.skip = 0;

        while (*(uintptr_t *) e.ip) {
            code = *(ngx_http_script_code_pt *) e.ip;
            code((ngx_http_script_engine_t *) &e);
        }

        b->last = e.pos;
    }

    //��header��Ϣȫ����ӡ����
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http proxy header:%N\"%*s\"",
                   (size_t) (b->last - b->pos), b->pos);

    if (r->request_body_no_buffering) { //����Ҫ���棬��ֱ�Ӱ�ͷ���в��ַ��ͳ�ȥ

        u->request_bufs = cl;

        if (ctx->internal_chunked) {
            u->output.output_filter = ngx_http_proxy_body_output_filter;
            u->output.filter_ctx = r;
        }

    } else if (plcf->body_values == NULL && plcf->upstream.pass_request_body) { 
    //���û������proxy_set_body���壬������Ҫ���Ͱ��壬��ѿͻ��˰���Ҳ�������ڴ���

        body = u->request_bufs;
        u->request_bufs = cl;

        while (body) {
            b = ngx_alloc_buf(r->pool);
            if (b == NULL) {
                return NGX_ERROR;
            }

            ngx_memcpy(b, body->buf, sizeof(ngx_buf_t));

            cl->next = ngx_alloc_chain_link(r->pool);
            if (cl->next == NULL) {
                return NGX_ERROR;
            }

            cl = cl->next;
            cl->buf = b;

            body = body->next;
        }

    } else { //˵��ͨ��proxy_set_body�����˵����İ���
        u->request_bufs = cl;
    }

    b->flush = 1;
    cl->next = NULL;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_reinit_request(ngx_http_request_t *r)
{
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        return NGX_OK;
    }

    ctx->status.code = 0;
    ctx->status.count = 0;
    ctx->status.start = NULL;
    ctx->status.end = NULL;
    ctx->chunked.state = 0;

    r->upstream->process_header = ngx_http_proxy_process_status_line;
    r->upstream->pipe->input_filter = ngx_http_proxy_copy_filter;
    r->upstream->input_filter = ngx_http_proxy_non_buffered_copy_filter;
    r->state = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_body_output_filter(void *data, ngx_chain_t *in)
{
    ngx_http_request_t  *r = data;

    off_t                  size;
    u_char                *chunk;
    ngx_int_t              rc;
    ngx_buf_t             *b;
    ngx_chain_t           *out, *cl, *tl, **ll, **fl;
    ngx_http_proxy_ctx_t  *ctx;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "proxy output filter");

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (in == NULL) {
        out = in;
        goto out;
    }

    out = NULL;
    ll = &out;

    if (!ctx->header_sent) {
        /* first buffer contains headers, pass it unmodified */

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "proxy output header");

        ctx->header_sent = 1;

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

    size = 0;
    cl = in;
    fl = ll;

    for ( ;; ) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "proxy output chunk: %d", ngx_buf_size(cl->buf));

        size += ngx_buf_size(cl->buf);

        if (cl->buf->flush
            || cl->buf->sync
            || ngx_buf_in_memory(cl->buf)
            || cl->buf->in_file)
        {
            tl = ngx_alloc_chain_link(r->pool);
            if (tl == NULL) {
                return NGX_ERROR;
            }

            tl->buf = cl->buf;
            *ll = tl;
            ll = &tl->next;
        }

        if (cl->next == NULL) {
            break;
        }

        cl = cl->next;
    }

    if (size) {
        tl = ngx_chain_get_free_buf(r->pool, &ctx->free);
        if (tl == NULL) {
            return NGX_ERROR;
        }

        b = tl->buf;
        chunk = b->start;

        if (chunk == NULL) {
            /* the "0000000000000000" is 64-bit hexadecimal string */

            chunk = ngx_palloc(r->pool, sizeof("0000000000000000" CRLF) - 1);
            if (chunk == NULL) {
                return NGX_ERROR;
            }

            b->start = chunk;
            b->end = chunk + sizeof("0000000000000000" CRLF) - 1;
        }

        b->tag = (ngx_buf_tag_t) &ngx_http_proxy_body_output_filter;
        b->memory = 0;
        b->temporary = 1;
        b->pos = chunk;
        b->last = ngx_sprintf(chunk, "%xO" CRLF, size);

        tl->next = *fl;
        *fl = tl;
    }

    if (cl->buf->last_buf) {
        tl = ngx_chain_get_free_buf(r->pool, &ctx->free);
        if (tl == NULL) {
            return NGX_ERROR;
        }

        b = tl->buf;

        b->tag = (ngx_buf_tag_t) &ngx_http_proxy_body_output_filter;
        b->temporary = 0;
        b->memory = 1;
        b->last_buf = 1;
        b->pos = (u_char *) CRLF "0" CRLF CRLF;
        b->last = b->pos + 7;

        cl->buf->last_buf = 0;

        *ll = tl;

        if (size == 0) {
            b->pos += 2;
        }

    } else if (size > 0) {
        tl = ngx_chain_get_free_buf(r->pool, &ctx->free);
        if (tl == NULL) {
            return NGX_ERROR;
        }

        b = tl->buf;

        b->tag = (ngx_buf_tag_t) &ngx_http_proxy_body_output_filter;
        b->temporary = 0;
        b->memory = 1;
        b->pos = (u_char *) CRLF;
        b->last = b->pos + 2;

        *ll = tl;

    } else {
        *ll = NULL;
    }

out:

    rc = ngx_chain_writer(&r->upstream->writer, out);

    ngx_chain_update_chains(r->pool, &ctx->free, &ctx->busy, &out,
                            (ngx_buf_tag_t) &ngx_http_proxy_body_output_filter);

    return rc;
}


static ngx_int_t //ngx_http_upstream_process_header��ִ��
ngx_http_proxy_process_status_line(ngx_http_request_t *r)
{
    size_t                 len;
    ngx_int_t              rc;
    ngx_http_upstream_t   *u;
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    u = r->upstream;

    rc = ngx_http_parse_status_line(r, &u->buffer, &ctx->status);//����״̬��(��Ӧ��)��Ҳ���ǵ�һ��

    if (rc == NGX_AGAIN) {//������ݻ�û����ô��
        return rc;
    }

    if (rc == NGX_ERROR) {

#if (NGX_HTTP_CACHE)

        if (r->cache) {
            r->http_version = NGX_HTTP_VERSION_9;
            return NGX_OK;
        }

#endif

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "upstream sent no valid HTTP/1.0 header");

#if 0
        if (u->accel) {
            return NGX_HTTP_UPSTREAM_INVALID_HEADER;
        }
#endif

        r->http_version = NGX_HTTP_VERSION_9;
        u->state->status = NGX_HTTP_OK;
        u->headers_in.connection_close = 1;

        return NGX_OK;
    }

    if (u->state && u->state->status == 0) {//��״̬�ֶΣ��ͱ���״̬��
        u->state->status = ctx->status.code;
    }

    u->headers_in.status_n = ctx->status.code;

    len = ctx->status.end - ctx->status.start;
    u->headers_in.status_line.len = len;

    u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);
    if (u->headers_in.status_line.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(u->headers_in.status_line.data, ctx->status.start, len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http proxy status %ui \"%V\"",
                   u->headers_in.status_n, &u->headers_in.status_line);

    if (ctx->status.http_version < NGX_HTTP_VERSION_11) {
        u->headers_in.connection_close = 1;
    }

    u->process_header = ngx_http_proxy_process_header;

    return ngx_http_proxy_process_header(r);
}


/*
//���κ�˷�����ģ��Ŀɶ��¼��ص������ngx_http_upstream_process_header��Ȼ�����process_header����ͷ�����ݵĽ����ˡ�
//ע������ֻ��ȡͷ���ֶ�(��ȡͷ���ֶ��п��ܻ��ȡ��һ���ְ���)��û�ж�ȡbody���֣���body�����������ط���ȡ����,��:
//����buffering�Ƿ�򿪣���˷��͵�ͷ�����ᱻbuffer�����Ȼᷢ��header��Ȼ�����body�ķ��ͣ���body�ķ��;���Ҫ����bufferingѡ���ˡ�
*/
static ngx_int_t //ngx_http_upstream_process_header��ִ�иú���
ngx_http_proxy_process_header(ngx_http_request_t *r)
{
    ngx_int_t                       rc;
    ngx_table_elt_t                *h;
    ngx_http_upstream_t            *u;
    ngx_http_proxy_ctx_t           *ctx;
    ngx_http_upstream_header_t     *hh;
    ngx_http_upstream_main_conf_t  *umcf;

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

    for ( ;; ) {//����cache������£�ע����ʱ��buf������ʵ�����Ѿ���ngx_http_upstream_process_header�г�ȥ��Ϊ�������ļ���Ԥ����ͷ���ڴ�

        rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);

        if (rc == NGX_OK) {//���潫����µ�ͷ���ֶα������������浽headers_in.headers

            /* a header line has been parsed successfully */

            h = ngx_list_push(&r->upstream->headers_in.headers);
            if (h == NULL) {
                return NGX_ERROR;
            }

            h->hash = r->header_hash;

            h->key.len = r->header_name_end - r->header_name_start;
            h->value.len = r->header_end - r->header_start;

            h->key.data = ngx_pnalloc(r->pool,
                               h->key.len + 1 + h->value.len + 1 + h->key.len);
            if (h->key.data == NULL) {
                return NGX_ERROR;
            }

            h->value.data = h->key.data + h->key.len + 1;
            h->lowcase_key = h->key.data + h->key.len + 1 + h->value.len + 1; //key-value������key��Сд�ַ���

            ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
            h->key.data[h->key.len] = '\0';
            ngx_memcpy(h->value.data, r->header_start, h->value.len);
            h->value.data[h->value.len] = '\0';

            if (h->key.len == r->lowcase_index) {
                ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);

            } else {
                ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
            }

            hh = ngx_hash_find(&umcf->headers_in_hash, h->hash,
                               h->lowcase_key, h->key.len); //��ngx_http_upstream_headers_in�����Ƿ������еĳ�Աƥ��

            //ngx_http_upstream_headers_in����ƥ������ִ�ж�Ӧ��handler
            if (hh && hh->handler(r, h, hh->offset) != NGX_OK) {
                return NGX_ERROR;
            }

            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http proxy header: \"%V: %V\"",
                           &h->key, &h->value);

            continue;
        }

        if (rc == NGX_HTTP_PARSE_HEADER_DONE) {//ȫ�����Ѿ���������ˣ��϶�����������\R\N��

            /* a whole header has been parsed successfully */

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http proxy header done");

            /*
             * if no "Server" and "Date" in header line,
             * then add the special empty headers
             */
            /* ˵����ͻ��˷��͵�ͷ�����б������server:  date: ��������û�з����������ֶΣ���nginx����ӿ�value��������ͷ���� */
            if (r->upstream->headers_in.server == NULL) { //������û�з���server:xxxͷ���У���ֱ�����server: value����Ϊ0
                h = ngx_list_push(&r->upstream->headers_in.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }

                h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(
                                    ngx_hash('s', 'e'), 'r'), 'v'), 'e'), 'r');

                ngx_str_set(&h->key, "Server");
                ngx_str_null(&h->value);
                h->lowcase_key = (u_char *) "server";
            }

            if (r->upstream->headers_in.date == NULL) {// Date ����HTTP��Ϣ�����ڡ����磺Date: Mon,10PR 18:42:51 GMT 
                h = ngx_list_push(&r->upstream->headers_in.headers);
                if (h == NULL) {
                    return NGX_ERROR;
                }

                h->hash = ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'), 'e');

                ngx_str_set(&h->key, "Date");
                ngx_str_null(&h->value);
                h->lowcase_key = (u_char *) "date";
            }

            /* clear content length if response is chunked */

            u = r->upstream;

            if (u->headers_in.chunked) { //chunked���뷽ʽ�Ͳ���Ҫ����content-length: ͷ���У����峤����chunked���ĸ�ʽָ���������ݳ���
                u->headers_in.content_length_n = -1;
            }

            /*
             * set u->keepalive if response has no body; this allows to keep
             * connections alive in case of r->header_only or X-Accel-Redirect
             */

            ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

            if (u->headers_in.status_n == NGX_HTTP_NO_CONTENT
                || u->headers_in.status_n == NGX_HTTP_NOT_MODIFIED
                || ctx->head
                || (!u->headers_in.chunked
                    && u->headers_in.content_length_n == 0))
            {
                u->keepalive = !u->headers_in.connection_close;
            }

            if (u->headers_in.status_n == NGX_HTTP_SWITCHING_PROTOCOLS) {
                u->keepalive = 0;

                if (r->headers_in.upgrade) {//��˷���//HTTP/1.1 101��ʱ����1  
                    u->upgrade = 1;
                }
            }

            int keepalive_t = u->keepalive;
            ngx_log_debugall(r->connection->log, 0,
                      "upstream header recv ok, u->keepalive:%d", keepalive_t);

            //ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,  "yang test .... body:%*s", (size_t) (r->upstream->buffer.last - r->upstream->buffer.pos), r->upstream->buffer.pos);
            return NGX_OK;
        }

        if (rc == NGX_AGAIN) {
            return NGX_AGAIN;
        }

        /* there was error while a header line parsing */
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent invalid header");

        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
    }
}


static ngx_int_t
ngx_http_proxy_input_filter_init(void *data)
{
    ngx_http_request_t    *r = data;
    ngx_http_upstream_t   *u;
    ngx_http_proxy_ctx_t  *ctx;

    u = r->upstream;
    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http proxy filter init upstream status:%d is HEAD:%d chunked:%d content_length_n:%O",
                   u->headers_in.status_n, ctx->head, u->headers_in.chunked,
                   u->headers_in.content_length_n);

    /* as per RFC2616, 4.4 Message Length */

    if (u->headers_in.status_n == NGX_HTTP_NO_CONTENT
        || u->headers_in.status_n == NGX_HTTP_NOT_MODIFIED
        || ctx->head)
    {
        /* 1xx, 204, and 304 and replies to HEAD requests */
        /* no 1xx since we don't send Expect and Upgrade */

        u->pipe->length = 0;
        u->length = 0;
        u->keepalive = !u->headers_in.connection_close;

    } else if (u->headers_in.chunked) {
        /* chunked */

        u->pipe->input_filter = ngx_http_proxy_chunked_filter;
        u->pipe->length = 3; /* "0" LF LF */

        u->input_filter = ngx_http_proxy_non_buffered_chunked_filter;
        u->length = 1;

    } else if (u->headers_in.content_length_n == 0) {
        /* empty body: special case as filter won't be called */

        u->pipe->length = 0;
        u->length = 0;
        u->keepalive = !u->headers_in.connection_close;

    } else {
        /* content length or connection close */

        u->pipe->length = u->headers_in.content_length_n;
        u->length = u->headers_in.content_length_n;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_copy_filter(ngx_event_pipe_t *p, ngx_buf_t *buf)
{
    ngx_buf_t           *b;
    ngx_chain_t         *cl;
    ngx_http_request_t  *r;

    if (buf->pos == buf->last) {
        return NGX_OK;
    }

    cl = ngx_chain_get_free_buf(p->pool, &p->free);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    b = cl->buf;

    ngx_memcpy(b, buf, sizeof(ngx_buf_t));
    b->shadow = buf;
    b->tag = p->tag;
    b->last_shadow = 1;
    b->recycled = 1;
    buf->shadow = b;

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, p->log, 0, "input buf #%d", b->num);

    if (p->in) {
        *p->last_in = cl;
    } else {
        p->in = cl;
    }
    p->last_in = &cl->next;

    if (p->length == -1) {
        return NGX_OK;
    }

    p->length -= b->last - b->pos;

    if (p->length == 0) {
        r = p->input_ctx;
        p->upstream_done = 1;
        r->upstream->keepalive = !r->upstream->headers_in.connection_close;

    } else if (p->length < 0) {
        r = p->input_ctx;
        p->upstream_done = 1;

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "upstream sent more data than specified in "
                      "\"Content-Length\" header");
    }

    int upstream_done = p->upstream_done;
    if(upstream_done)
        ngx_log_debugall(p->log, 0, "proxy copy filter upstream_done:%d", upstream_done);
    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_chunked_filter(ngx_event_pipe_t *p, ngx_buf_t *buf)
{
    ngx_int_t              rc;
    ngx_buf_t             *b, **prev;
    ngx_chain_t           *cl;
    ngx_http_request_t    *r;
    ngx_http_proxy_ctx_t  *ctx;

    if (buf->pos == buf->last) {
        return NGX_OK;
    }

    r = p->input_ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    b = NULL;
    prev = &buf->shadow;

    for ( ;; ) {

        rc = ngx_http_parse_chunked(r, buf, &ctx->chunked);

        if (rc == NGX_OK) {

            /* a chunk has been parsed successfully */

            cl = ngx_chain_get_free_buf(p->pool, &p->free);
            if (cl == NULL) {
                return NGX_ERROR;
            }

            b = cl->buf;

            ngx_memzero(b, sizeof(ngx_buf_t));

            b->pos = buf->pos;
            b->start = buf->start;
            b->end = buf->end;
            b->tag = p->tag;
            b->temporary = 1;
            b->recycled = 1;

            *prev = b;
            prev = &b->shadow;

            if (p->in) {
                *p->last_in = cl;
            } else {
                p->in = cl;
            }
            p->last_in = &cl->next;

            /* STUB */ b->num = buf->num;

            ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                           "input buf #%d %p", b->num, b->pos);

            if (buf->last - buf->pos >= ctx->chunked.size) {

                buf->pos += (size_t) ctx->chunked.size;
                b->last = buf->pos;
                ctx->chunked.size = 0;

                continue;
            }

            ctx->chunked.size -= buf->last - buf->pos;
            buf->pos = buf->last;
            b->last = buf->last;

            continue;
        }

        if (rc == NGX_DONE) {

            /* a whole response has been parsed successfully */

            p->upstream_done = 1;
            r->upstream->keepalive = !r->upstream->headers_in.connection_close;

            break;
        }

        if (rc == NGX_AGAIN) {

            /* set p->length, minimal amount of data we want to see */

            p->length = ctx->chunked.length;

            break;
        }

        /* invalid response */

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "upstream sent invalid chunked response");

        return NGX_ERROR;
    }

    int upstream_done = p->upstream_done;
    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http proxy chunked state %d, length %d, p->upstream_done:%d",
                   ctx->chunked.state, p->length, upstream_done);

    if (b) {
        b->shadow = buf;
        b->last_shadow = 1;

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, p->log, 0,
                       "input buf %p %z", b->pos, b->last - b->pos);

        return NGX_OK;
    }

    /* there is no data record in the buf, add it to free chain */

    if (ngx_event_pipe_add_free_buf(p, buf) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_non_buffered_copy_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t   *r = data;

    ngx_buf_t            *b;
    ngx_chain_t          *cl, **ll;
    ngx_http_upstream_t  *u;

    u = r->upstream;

    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next) {
        ll = &cl->next;
    }

    cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);
    if (cl == NULL) {
        return NGX_ERROR;
    }

    *ll = cl;

    cl->buf->flush = 1;
    cl->buf->memory = 1;

    b = &u->buffer;

    cl->buf->pos = b->last;
    b->last += bytes;
    cl->buf->last = b->last;
    cl->buf->tag = u->output.tag;

    if (u->length == -1) {
        return NGX_OK;
    }

    u->length -= bytes;

    if (u->length == 0) {
        u->keepalive = !u->headers_in.connection_close;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_non_buffered_chunked_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t   *r = data;

    ngx_int_t              rc;
    ngx_buf_t             *b, *buf;
    ngx_chain_t           *cl, **ll;
    ngx_http_upstream_t   *u;
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        return NGX_ERROR;
    }

    u = r->upstream;
    buf = &u->buffer;

    buf->pos = buf->last;
    buf->last += bytes;

    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next) {
        ll = &cl->next;
    }

    for ( ;; ) {

        rc = ngx_http_parse_chunked(r, buf, &ctx->chunked);

        if (rc == NGX_OK) {

            /* a chunk has been parsed successfully */

            cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);
            if (cl == NULL) {
                return NGX_ERROR;
            }

            *ll = cl;
            ll = &cl->next;

            b = cl->buf;

            b->flush = 1;
            b->memory = 1;

            b->pos = buf->pos;
            b->tag = u->output.tag;

            if (buf->last - buf->pos >= ctx->chunked.size) {
                buf->pos += (size_t) ctx->chunked.size;
                b->last = buf->pos;
                ctx->chunked.size = 0;

            } else {
                ctx->chunked.size -= buf->last - buf->pos;
                buf->pos = buf->last;
                b->last = buf->last;
            }

            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http proxy out buf %p %z",
                           b->pos, b->last - b->pos);

            continue;
        }

        if (rc == NGX_DONE) {

            /* a whole response has been parsed successfully */

            u->keepalive = !u->headers_in.connection_close;
            u->length = 0;

            break;
        }

        if (rc == NGX_AGAIN) {
            break;
        }

        /* invalid response */

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "upstream sent invalid chunked response");

        return NGX_ERROR;
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
                           "http proxy in memory %p-%p %uz",
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


static void
ngx_http_proxy_abort_request(ngx_http_request_t *r)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "abort http proxy request");

    return;
}


static void
ngx_http_proxy_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "finalize http proxy request");

    return;
}


static ngx_int_t
ngx_http_proxy_host_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = ctx->vars.host_header.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = ctx->vars.host_header.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_port_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = ctx->vars.port.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = ctx->vars.port.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_add_x_forwarded_for_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t             len;
    u_char            *p;
    ngx_uint_t         i, n;
    ngx_table_elt_t  **h;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    n = r->headers_in.x_forwarded_for.nelts;
    h = r->headers_in.x_forwarded_for.elts;

    len = 0;

    for (i = 0; i < n; i++) {
        len += h[i]->value.len + sizeof(", ") - 1;
    }

    if (len == 0) {
        v->len = r->connection->addr_text.len;
        v->data = r->connection->addr_text.data;
        return NGX_OK;
    }

    len += r->connection->addr_text.len;

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = len;
    v->data = p;

    for (i = 0; i < n; i++) {
        p = ngx_copy(p, h[i]->value.data, h[i]->value.len);
        *p++ = ','; *p++ = ' ';
    }

    ngx_memcpy(p, r->connection->addr_text.data, r->connection->addr_text.len);

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_internal_body_length_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL || ctx->internal_body_length < 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);

    if (v->data == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(v->data, "%O", ctx->internal_body_length) - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_internal_chunked_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_proxy_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    if (ctx == NULL || !ctx->internal_chunked) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = (u_char *) "chunked";
    v->len = sizeof("chunked") - 1;

    return NGX_OK;
}

/*
location /proxy1/ {			
    proxy_pass  http://10.10.0.103:8080/; 		
}

���urlΪhttp://10.2.13.167/proxy1/����ngx_http_upstream_rewrite_location�����
��˷���Location: http://10.10.0.103:8080/secure/MyJiraHome.jspa
��ʵ�ʷ��͸�������ͻ��˵�headers_out.headers.locationΪhttp://10.2.13.167/proxy1/secure/MyJiraHome.jspa
*/
//ngx_http_upstream_rewrite_location->ngx_http_proxy_rewrite_redirect��ִ��
//��ȡ�µ��ض�����µ�uri��������������h->value(Ҳ����ngx_http_upstream_headers_in_t->location�У���)
static ngx_int_t
ngx_http_proxy_rewrite_redirect(ngx_http_request_t *r, ngx_table_elt_t *h,
    size_t prefix)
{
    size_t                      len;
    ngx_int_t                   rc;
    ngx_uint_t                  i;
    ngx_http_proxy_rewrite_t   *pr;
    ngx_http_proxy_loc_conf_t  *plcf;

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_module);

    pr = plcf->redirects->elts;

    if (pr == NULL) {
        return NGX_DECLINED;
    }

    len = h->value.len - prefix;

    for (i = 0; i < plcf->redirects->nelts; i++) {
        rc = pr[i].handler(r, h, prefix, len, &pr[i]); //ngx_http_proxy_rewrite_complex_handler

        if (rc != NGX_DECLINED) {
            return rc;
        }
    }

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_proxy_rewrite_cookie(ngx_http_request_t *r, ngx_table_elt_t *h)
{
    size_t                      prefix;
    u_char                     *p;
    ngx_int_t                   rc, rv;
    ngx_http_proxy_loc_conf_t  *plcf;

    p = (u_char *) ngx_strchr(h->value.data, ';');
    if (p == NULL) {
        return NGX_DECLINED;
    }

    prefix = p + 1 - h->value.data;

    rv = NGX_DECLINED;

    plcf = ngx_http_get_module_loc_conf(r, ngx_http_proxy_module);

    if (plcf->cookie_domains) {
        p = ngx_strcasestrn(h->value.data + prefix, "domain=", 7 - 1);

        if (p) {
            rc = ngx_http_proxy_rewrite_cookie_value(r, h, p + 7,
                                                     plcf->cookie_domains);
            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (rc != NGX_DECLINED) {
                rv = rc;
            }
        }
    }

    if (plcf->cookie_paths) {
        p = ngx_strcasestrn(h->value.data + prefix, "path=", 5 - 1);

        if (p) {
            rc = ngx_http_proxy_rewrite_cookie_value(r, h, p + 5,
                                                     plcf->cookie_paths);
            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (rc != NGX_DECLINED) {
                rv = rc;
            }
        }
    }

    return rv;
}


static ngx_int_t
ngx_http_proxy_rewrite_cookie_value(ngx_http_request_t *r, ngx_table_elt_t *h,
    u_char *value, ngx_array_t *rewrites)
{
    size_t                     len, prefix;
    u_char                    *p;
    ngx_int_t                  rc;
    ngx_uint_t                 i;
    ngx_http_proxy_rewrite_t  *pr;

    prefix = value - h->value.data;

    p = (u_char *) ngx_strchr(value, ';');

    len = p ? (size_t) (p - value) : (h->value.len - prefix);

    pr = rewrites->elts;

    for (i = 0; i < rewrites->nelts; i++) {
        rc = pr[i].handler(r, h, prefix, len, &pr[i]);

        if (rc != NGX_DECLINED) {
            return rc;
        }
    }

    return NGX_DECLINED;
}

/*
location /proxy1/ {			
    proxy_pass  http://10.10.0.103:8080/; 		
}

���urlΪhttp://10.2.13.167/proxy1/����ngx_http_upstream_rewrite_location�����
��˷���Location: http://10.10.0.103:8080/secure/MyJiraHome.jspa
��ʵ�ʷ��͸�������ͻ��˵�headers_out.headers.locationΪhttp://10.2.13.167/proxy1/secure/MyJiraHome.jspa
*/
//ngx_http_upstream_rewrite_location->ngx_http_proxy_rewrite_redirect��ִ��
//��ȡ�µ��ض�����µ�uri��������������h->value
static ngx_int_t
ngx_http_proxy_rewrite_complex_handler(ngx_http_request_t *r,
    ngx_table_elt_t *h, size_t prefix, size_t len, ngx_http_proxy_rewrite_t *pr)
{
    ngx_str_t  pattern, replacement;

    //����proxy_redirect /xxx1/$adfa /xxx2/$dgg�е�/xxx1/$adfaΪ�����ַ�����pattern
    if (ngx_http_complex_value(r, &pr->pattern.complex, &pattern) != NGX_OK) { 
        return NGX_ERROR;
    }

    if (pattern.len > len
        || ngx_rstrncmp(h->value.data + prefix, pattern.data,
                        pattern.len) != 0)
    {
        return NGX_DECLINED;
    }

    //����proxy_redirect /xxx1/$adfa /xxx2/$dgg�е�/xxx2/$dggΪ�����ַ�����replacement
    if (ngx_http_complex_value(r, &pr->replacement, &replacement) != NGX_OK) {
        return NGX_ERROR;
    }

    return ngx_http_proxy_rewrite(r, h, prefix, pattern.len, &replacement);
}


#if (NGX_PCRE)

static ngx_int_t
ngx_http_proxy_rewrite_regex_handler(ngx_http_request_t *r, ngx_table_elt_t *h,
    size_t prefix, size_t len, ngx_http_proxy_rewrite_t *pr)
{
    ngx_str_t  pattern, replacement;

    pattern.len = len;
    pattern.data = h->value.data + prefix;

    if (ngx_http_regex_exec(r, pr->pattern.regex, &pattern) != NGX_OK) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, &pr->replacement, &replacement) != NGX_OK) {
        return NGX_ERROR;
    }

    if (prefix == 0 && h->value.len == len) {
        h->value = replacement;
        return NGX_OK;
    }

    return ngx_http_proxy_rewrite(r, h, prefix, len, &replacement);
}

#endif


static ngx_int_t
ngx_http_proxy_rewrite_domain_handler(ngx_http_request_t *r,
    ngx_table_elt_t *h, size_t prefix, size_t len, ngx_http_proxy_rewrite_t *pr)
{
    u_char     *p;
    ngx_str_t   pattern, replacement;

    if (ngx_http_complex_value(r, &pr->pattern.complex, &pattern) != NGX_OK) {
        return NGX_ERROR;
    }

    p = h->value.data + prefix;

    if (p[0] == '.') {
        p++;
        prefix++;
        len--;
    }

    if (pattern.len != len || ngx_rstrncasecmp(pattern.data, p, len) != 0) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, &pr->replacement, &replacement) != NGX_OK) {
        return NGX_ERROR;
    }

    return ngx_http_proxy_rewrite(r, h, prefix, len, &replacement);
}

//����replacement
static ngx_int_t
ngx_http_proxy_rewrite(ngx_http_request_t *r, ngx_table_elt_t *h, size_t prefix,
    size_t len, ngx_str_t *replacement)
{
    u_char  *p, *data;
    size_t   new_len;

    new_len = replacement->len + h->value.len - len;

    if (replacement->len > len) {

        data = ngx_pnalloc(r->pool, new_len + 1);
        if (data == NULL) {
            return NGX_ERROR;
        }

        p = ngx_copy(data, h->value.data, prefix);
        p = ngx_copy(p, replacement->data, replacement->len);

        ngx_memcpy(p, h->value.data + prefix + len,
                   h->value.len - len - prefix + 1);

        h->value.data = data;

    } else {
        p = ngx_copy(h->value.data + prefix, replacement->data,
                     replacement->len);

        ngx_memmove(p, h->value.data + prefix + len,
                    h->value.len - len - prefix + 1);
    }

    h->value.len = new_len;

    return NGX_OK;
}


static ngx_int_t
ngx_http_proxy_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_proxy_vars; v->name.len; v++) {
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
ngx_http_proxy_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_proxy_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_proxy_main_conf_t));
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
ngx_http_proxy_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_proxy_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_proxy_loc_conf_t));
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
     *     conf->upstream.ssl_name = NULL;
     *
     *     conf->method = { 0, NULL };
     *     conf->headers_source = NULL;
     *     conf->headers.lengths = NULL;
     *     conf->headers.values = NULL;
     *     conf->headers.hash = { NULL, 0 };
     *     conf->headers_cache.lengths = NULL;
     *     conf->headers_cache.values = NULL;
     *     conf->headers_cache.hash = { NULL, 0 };
     *     conf->body_lengths = NULL;
     *     conf->body_values = NULL;
     *     conf->body_source = { 0, NULL };
     *     conf->redirects = NULL;
     *     conf->ssl = 0;
     *     conf->ssl_protocols = 0;
     *     conf->ssl_ciphers = { 0, NULL };
     *     conf->ssl_trusted_certificate = { 0, NULL };
     *     conf->ssl_crl = { 0, NULL };
     *     conf->ssl_certificate = { 0, NULL };
     *     conf->ssl_certificate_key = { 0, NULL };
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

#if (NGX_HTTP_SSL)
    conf->upstream.ssl_session_reuse = NGX_CONF_UNSET;
    conf->upstream.ssl_server_name = NGX_CONF_UNSET;
    conf->upstream.ssl_verify = NGX_CONF_UNSET;
    conf->ssl_verify_depth = NGX_CONF_UNSET_UINT;
    conf->ssl_passwords = NGX_CONF_UNSET_PTR;
#endif

    /* "proxy_cyclic_temp_file" is disabled */
    conf->upstream.cyclic_temp_file = 0;

    conf->redirect = NGX_CONF_UNSET;
    conf->upstream.change_buffering = 1;

    conf->cookie_domains = NGX_CONF_UNSET_PTR;
    conf->cookie_paths = NGX_CONF_UNSET_PTR;

    conf->http_version = NGX_CONF_UNSET_UINT;

    conf->headers_hash_max_size = NGX_CONF_UNSET_UINT;
    conf->headers_hash_bucket_size = NGX_CONF_UNSET_UINT;

    ngx_str_set(&conf->upstream.module, "proxy");

    return conf;
}


static char *
ngx_http_proxy_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_proxy_loc_conf_t *prev = parent;
    ngx_http_proxy_loc_conf_t *conf = child;

    u_char                     *p;
    size_t                      size;
    ngx_int_t                   rc;
    ngx_hash_init_t             hash;
    ngx_http_core_loc_conf_t   *clcf;
    ngx_http_proxy_rewrite_t   *pr;
    ngx_http_script_compile_t   sc;

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
                           "there must be at least 2 \"proxy_buffers\"");
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
             "\"proxy_busy_buffers_size\" must be equal to or greater than "
             "the maximum of the value of \"proxy_buffer_size\" and "
             "one of the \"proxy_buffers\"");

        return NGX_CONF_ERROR;
    }

    if (conf->upstream.busy_buffers_size
        > (conf->upstream.bufs.num - 1) * conf->upstream.bufs.size)
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
             "\"proxy_busy_buffers_size\" must be less than "
             "the size of all \"proxy_buffers\" minus one buffer");

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
             "\"proxy_temp_file_write_size\" must be equal to or greater "
             "than the maximum of the value of \"proxy_buffer_size\" and "
             "one of the \"proxy_buffers\"");

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
             "\"proxy_max_temp_file_size\" must be equal to zero to disable "
             "temporary files usage or must be equal to or greater than "
             "the maximum of the value of \"proxy_buffer_size\" and "
             "one of the \"proxy_buffers\"");

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
                              &ngx_http_proxy_temp_path)
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

    //proxy_cache abc�����proxy_cache_path xxx keys_zone=abc:10m;һ�𣬷�����ngx_http_proxy_merge_loc_conf��ʧ�ܣ���Ϊû��Ϊ��abc����ngx_http_file_cache_t
    if (conf->upstream.cache_zone && conf->upstream.cache_zone->data == NULL) {
        ngx_shm_zone_t  *shm_zone;

        shm_zone = conf->upstream.cache_zone;

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"proxy_cache\" zone \"%V\" is unknown",
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

    ngx_conf_merge_value(conf->upstream.cache_lock,
                              prev->upstream.cache_lock, 0);

    ngx_conf_merge_msec_value(conf->upstream.cache_lock_timeout,
                              prev->upstream.cache_lock_timeout, 5000);

    ngx_conf_merge_msec_value(conf->upstream.cache_lock_age,
                              prev->upstream.cache_lock_age, 5000);

    ngx_conf_merge_value(conf->upstream.cache_revalidate,
                              prev->upstream.cache_revalidate, 0);

#endif

    ngx_conf_merge_str_value(conf->method, prev->method, "");

    if (conf->method.len
        && conf->method.data[conf->method.len - 1] != ' ')
    {
        conf->method.data[conf->method.len] = ' ';
        conf->method.len++;
    }

    ngx_conf_merge_value(conf->upstream.pass_request_headers,
                              prev->upstream.pass_request_headers, 1);
    ngx_conf_merge_value(conf->upstream.pass_request_body,
                              prev->upstream.pass_request_body, 1);

    ngx_conf_merge_value(conf->upstream.intercept_errors,
                              prev->upstream.intercept_errors, 0);

#if (NGX_HTTP_SSL)

    ngx_conf_merge_value(conf->upstream.ssl_session_reuse,
                              prev->upstream.ssl_session_reuse, 1);

    ngx_conf_merge_bitmask_value(conf->ssl_protocols, prev->ssl_protocols,
                                 (NGX_CONF_BITMASK_SET|NGX_SSL_TLSv1
                                  |NGX_SSL_TLSv1_1|NGX_SSL_TLSv1_2));

    ngx_conf_merge_str_value(conf->ssl_ciphers, prev->ssl_ciphers,
                             "DEFAULT");

    if (conf->upstream.ssl_name == NULL) {
        conf->upstream.ssl_name = prev->upstream.ssl_name;
    }

    ngx_conf_merge_value(conf->upstream.ssl_server_name,
                              prev->upstream.ssl_server_name, 0);
    ngx_conf_merge_value(conf->upstream.ssl_verify,
                              prev->upstream.ssl_verify, 0);
    ngx_conf_merge_uint_value(conf->ssl_verify_depth,
                              prev->ssl_verify_depth, 1);
    ngx_conf_merge_str_value(conf->ssl_trusted_certificate,
                              prev->ssl_trusted_certificate, "");
    ngx_conf_merge_str_value(conf->ssl_crl, prev->ssl_crl, "");

    ngx_conf_merge_str_value(conf->ssl_certificate,
                              prev->ssl_certificate, "");
    ngx_conf_merge_str_value(conf->ssl_certificate_key,
                              prev->ssl_certificate_key, "");
    ngx_conf_merge_ptr_value(conf->ssl_passwords, prev->ssl_passwords, NULL);

    if (conf->ssl && ngx_http_proxy_set_ssl(cf, conf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

#endif

    ngx_conf_merge_value(conf->redirect, prev->redirect, 1);

    if (conf->redirect) {

        if (conf->redirects == NULL) {
            conf->redirects = prev->redirects;
        }

        //ngx_http_proxy_redirect��ngx_http_proxy_rewrite_redirect����Ķ�������һ��ȷ���ض����ĵ�ַ
        if (conf->redirects == NULL && conf->url.data) {
        //ngx_http_proxy_merge_loc_conf��if�������ݺ�ngx_http_proxy_redirect�����proxy_redirect defaultһ����Ҳ����˵proxy_redirectĬ��Ϊdefault
            conf->redirects = ngx_array_create(cf->pool, 1,
                                             sizeof(ngx_http_proxy_rewrite_t));
            if (conf->redirects == NULL) {
                return NGX_CONF_ERROR;
            }

            pr = ngx_array_push(conf->redirects);
            if (pr == NULL) {
                return NGX_CONF_ERROR;
            }

            ngx_memzero(&pr->pattern.complex,
                        sizeof(ngx_http_complex_value_t));

            ngx_memzero(&pr->replacement, sizeof(ngx_http_complex_value_t));

            pr->handler = ngx_http_proxy_rewrite_complex_handler;

            if (conf->vars.uri.len) {
                pr->pattern.complex.value = conf->url; 
                pr->replacement.value = conf->location;

            } else {
                pr->pattern.complex.value.len = conf->url.len
                                                + sizeof("/") - 1;

                p = ngx_pnalloc(cf->pool, pr->pattern.complex.value.len);
                if (p == NULL) {
                    return NGX_CONF_ERROR;
                }

                pr->pattern.complex.value.data = p;

                p = ngx_cpymem(p, conf->url.data, conf->url.len);
                *p = '/';

                ngx_str_set(&pr->replacement.value, "/");
            }
        }
    }

    ngx_conf_merge_ptr_value(conf->cookie_domains, prev->cookie_domains, NULL);

    ngx_conf_merge_ptr_value(conf->cookie_paths, prev->cookie_paths, NULL);

    ngx_conf_merge_uint_value(conf->http_version, prev->http_version,
                              NGX_HTTP_VERSION_10);

    ngx_conf_merge_uint_value(conf->headers_hash_max_size,
                              prev->headers_hash_max_size, 512);

    ngx_conf_merge_uint_value(conf->headers_hash_bucket_size,
                              prev->headers_hash_bucket_size, 64);

    conf->headers_hash_bucket_size = ngx_align(conf->headers_hash_bucket_size,
                                               ngx_cacheline_size);

    hash.max_size = conf->headers_hash_max_size;
    hash.bucket_size = conf->headers_hash_bucket_size;
    hash.name = "proxy_headers_hash";

    if (ngx_http_upstream_hide_headers_hash(cf, &conf->upstream,
            &prev->upstream, ngx_http_proxy_hide_headers, &hash)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    if (clcf->noname
        && conf->upstream.upstream == NULL && conf->proxy_lengths == NULL)
    {
        conf->upstream.upstream = prev->upstream.upstream;
        conf->location = prev->location;
        conf->vars = prev->vars;

        conf->proxy_lengths = prev->proxy_lengths;
        conf->proxy_values = prev->proxy_values;

#if (NGX_HTTP_SSL)
        conf->upstream.ssl = prev->upstream.ssl;
#endif
    }

    if (clcf->lmt_excpt && clcf->handler == NULL
        && (conf->upstream.upstream || conf->proxy_lengths))
    {
        clcf->handler = ngx_http_proxy_handler;
    }

    if (conf->body_source.data == NULL) {
        conf->body_flushes = prev->body_flushes;
        conf->body_source = prev->body_source;
        conf->body_lengths = prev->body_lengths;
        conf->body_values = prev->body_values;
    }

    if (conf->body_source.data && conf->body_lengths == NULL) {

        ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

        sc.cf = cf;
        sc.source = &conf->body_source;
        sc.flushes = &conf->body_flushes;
        sc.lengths = &conf->body_lengths;
        sc.values = &conf->body_values;
        sc.complete_lengths = 1;
        sc.complete_values = 1;

        if (ngx_http_script_compile(&sc) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    if (conf->headers_source == NULL) {
        conf->headers = prev->headers;
#if (NGX_HTTP_CACHE)
        conf->headers_cache = prev->headers_cache;
#endif
        conf->headers_source = prev->headers_source;
    }

    rc = ngx_http_proxy_init_headers(cf, conf, &conf->headers,
                                     ngx_http_proxy_headers);
    if (rc != NGX_OK) {
        return NGX_CONF_ERROR;
    }

#if (NGX_HTTP_CACHE)

    if (conf->upstream.cache) {
        rc = ngx_http_proxy_init_headers(cf, conf, &conf->headers_cache,
                                         ngx_http_proxy_cache_headers);
        if (rc != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

#endif

    return NGX_CONF_OK;
}

/* 
  ��proxy_set_header��ngx_http_proxy_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
  ��ngx_http_proxy_cache_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
*/
//1. ����洢����ngx_http_proxy_headers��proxy_set_header���õ�ͷ����Ϣͷ��ӵ���lengths��values ����ngx_http_proxy_loc_conf_t->headers
//2. ����洢����ngx_http_proxy_cache_headers���õ�ͷ����Ϣͷ��ӵ���lengths��values  ����ngx_http_proxy_loc_conf_t->headers_cache
static ngx_int_t
ngx_http_proxy_init_headers(ngx_conf_t *cf, ngx_http_proxy_loc_conf_t *conf,
    ngx_http_proxy_headers_t *headers, ngx_keyval_t *default_headers)
{
    u_char                       *p;
    size_t                        size;
    uintptr_t                    *code;
    ngx_uint_t                    i;
    ngx_array_t                   headers_names, headers_merged;
    ngx_keyval_t                 *src, *s, *h;
    ngx_hash_key_t               *hk;
    ngx_hash_init_t               hash;
    ngx_http_script_compile_t     sc;
    ngx_http_script_copy_code_t  *copy;

    if (headers->hash.buckets) { //�����Ϊ�գ����أ�����ǵ�һ�ν���ú�����������������̴����ռ��ʼ��
        return NGX_OK;
    }

    if (ngx_array_init(&headers_names, cf->temp_pool, 4, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&headers_merged, cf->temp_pool, 4, sizeof(ngx_keyval_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (conf->headers_source == NULL) {
        conf->headers_source = ngx_array_create(cf->pool, 4,
                                                sizeof(ngx_keyval_t));
        if (conf->headers_source == NULL) {
            return NGX_ERROR;
        }
    }

    headers->lengths = ngx_array_create(cf->pool, 64, 1); //64��1�ֽڵ�����
    if (headers->lengths == NULL) {
        return NGX_ERROR;
    }

    headers->values = ngx_array_create(cf->pool, 512, 1); //512��1�ֽڵ�����
    if (headers->values == NULL) {
        return NGX_ERROR;
    }

    src = conf->headers_source->elts;
    for (i = 0; i < conf->headers_source->nelts; i++) { //��proxy_set_header���õ�ͷ����Ϣ��ӵ�headers_merged������

        s = ngx_array_push(&headers_merged);
        if (s == NULL) {
            return NGX_ERROR;
        }

        *s = src[i];
    }

    h = default_headers; //ngx_http_proxy_headers����ngx_http_proxy_cache_headers

    while (h->key.len) { //��default_headers(ngx_http_proxy_headers����ngx_http_proxy_cache_headers)�в��ظ���Ԫ����ӵ�headers_merged��

        src = headers_merged.elts;
        for (i = 0; i < headers_merged.nelts; i++) {
            if (ngx_strcasecmp(h->key.data, src[i].key.data) == 0) {
                goto next;
            }
        }

        s = ngx_array_push(&headers_merged);
        if (s == NULL) {
            return NGX_ERROR;
        }

        *s = *h;

    next:

        h++;
    }


    /* 
    ��proxy_set_header��ngx_http_proxy_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
    ��ngx_http_proxy_cache_headers����һ��Ȼ������е�key:value�ַ����ͳ�����Ϣ��ӵ�headers->lengths��headers->values��
     */
    src = headers_merged.elts;
    for (i = 0; i < headers_merged.nelts; i++) {

        /* ����headers_merged�еĳ�Ա��Ϣ��headers_names������ */
        hk = ngx_array_push(&headers_names);
        if (hk == NULL) {
            return NGX_ERROR;
        }

        hk->key = src[i].key;
        hk->key_hash = ngx_hash_key_lc(src[i].key.data, src[i].key.len);
        hk->value = (void *) 1;

        if (src[i].value.len == 0) { //valueΪ�յģ�������ӵ�headers->lengths��
            continue;
        }

        if (ngx_http_script_variables_count(&src[i].value) == 0) {
            copy = ngx_array_push_n(headers->lengths,
                                    sizeof(ngx_http_script_copy_code_t));
            if (copy == NULL) {
                return NGX_ERROR;
            }

            //key:value�ַ�������code
            copy->code = (ngx_http_script_code_pt)
                                                 ngx_http_script_copy_len_code;
            copy->len = src[i].key.len + sizeof(": ") - 1
                        + src[i].value.len + sizeof(CRLF) - 1; //key:value�ַ�������   
                        //�����������ڽ���key:valueֵ��ʱ���ngx_http_script_copy_len_code��ȡ


            //key:value�ַ���code
            size = (sizeof(ngx_http_script_copy_code_t)
                       + src[i].key.len + sizeof(": ") - 1
                       + src[i].value.len + sizeof(CRLF) - 1
                       + sizeof(uintptr_t) - 1)
                    & ~(sizeof(uintptr_t) - 1); //4�ֽڶ���


            copy = ngx_array_push_n(headers->values, size);
            if (copy == NULL) {
                return NGX_ERROR;
            }

            copy->code = ngx_http_script_copy_code;
            copy->len = src[i].key.len + sizeof(": ") - 1
                        + src[i].value.len + sizeof(CRLF) - 1;

            p = (u_char *) copy + sizeof(ngx_http_script_copy_code_t); //ָ�򿪱ٿռ�ʵ�ʵĴ洢key:value�ĵط�

            p = ngx_cpymem(p, src[i].key.data, src[i].key.len);
            *p++ = ':'; *p++ = ' ';
            p = ngx_cpymem(p, src[i].value.data, src[i].value.len);
            *p++ = CR; *p = LF;

        } else {
            /*
                ngx_http_proxy_cache_headers��ngx_http_proxy_cache_headers���õ�ͷ��������ڱ���
               */
            copy = ngx_array_push_n(headers->lengths,
                                    sizeof(ngx_http_script_copy_code_t));
            if (copy == NULL) {
                return NGX_ERROR;
            }

            copy->code = (ngx_http_script_code_pt)
                                                 ngx_http_script_copy_len_code;
            copy->len = src[i].key.len + sizeof(": ") - 1;


            size = (sizeof(ngx_http_script_copy_code_t)
                    + src[i].key.len + sizeof(": ") - 1 + sizeof(uintptr_t) - 1)
                    & ~(sizeof(uintptr_t) - 1);

            copy = ngx_array_push_n(headers->values, size);
            if (copy == NULL) {
                return NGX_ERROR;
            }

            copy->code = ngx_http_script_copy_code;
            copy->len = src[i].key.len + sizeof(": ") - 1;

            p = (u_char *) copy + sizeof(ngx_http_script_copy_code_t);
            p = ngx_cpymem(p, src[i].key.data, src[i].key.len);
            *p++ = ':'; *p = ' ';


            ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

            sc.cf = cf;
            sc.source = &src[i].value;
            sc.flushes = &headers->flushes;
            sc.lengths = &headers->lengths;
            sc.values = &headers->values;

            if (ngx_http_script_compile(&sc) != NGX_OK) {
                return NGX_ERROR;
            }


            copy = ngx_array_push_n(headers->lengths,
                                    sizeof(ngx_http_script_copy_code_t));
            if (copy == NULL) {
                return NGX_ERROR;
            }

            copy->code = (ngx_http_script_code_pt)
                                                 ngx_http_script_copy_len_code;
            copy->len = sizeof(CRLF) - 1;


            size = (sizeof(ngx_http_script_copy_code_t)
                    + sizeof(CRLF) - 1 + sizeof(uintptr_t) - 1)
                    & ~(sizeof(uintptr_t) - 1);

            copy = ngx_array_push_n(headers->values, size);
            if (copy == NULL) {
                return NGX_ERROR;
            }

            copy->code = ngx_http_script_copy_code;
            copy->len = sizeof(CRLF) - 1;

            p = (u_char *) copy + sizeof(ngx_http_script_copy_code_t);
            *p++ = CR; *p = LF;
        }

        code = ngx_array_push_n(headers->lengths, sizeof(uintptr_t)); //headers->lengthsĩβ����ӿ�ָ���ʾ�������
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;

        code = ngx_array_push_n(headers->values, sizeof(uintptr_t));//headers->valuesĩβ����ӿ�ָ���ʾ�������
        if (code == NULL) {
            return NGX_ERROR;
        }

        *code = (uintptr_t) NULL;
    }

    code = ngx_array_push_n(headers->lengths, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_ERROR;
    }

    *code = (uintptr_t) NULL;


    //
    hash.hash = &headers->hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = conf->headers_hash_max_size;
    hash.bucket_size = conf->headers_hash_bucket_size;
    hash.name = "proxy_headers_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    return ngx_hash_init(&hash, headers_names.elts, headers_names.nelts);
}

/*
�﷨��proxy_pass URL 
Ĭ��ֵ��no 
ʹ���ֶΣ�location, location�е�if�ֶ� 
���ָ�����ñ�����������ĵ�ַ�ͱ�ӳ���URI����ַ����ʹ����������IP�Ӷ˿ںŵ���ʽ�����磺
proxy_pass http://localhost:8000/uri/;����һ��unix socket��
proxy_pass http://unix:/path/to/backend.socket:/uri/;·����unix�ؼ��ֵĺ���ָ����λ������ð��֮�䡣
ע�⣺HTTP Hostͷû��ת������������Ϊ����proxy_pass���������磬������ƶ���������example.com������һ̨������Ȼ��������������
������example.com��һ���µ�IP����ͬʱ�ھɻ������ֶ����µ�example.comIPд��/etc/hosts��ͬʱʹ��proxy_pass�ض���http://example.com, 
Ȼ���޸�DNS���µ�IP��
����������ʱ��Nginx��location��Ӧ��URI�����滻��proxy_passָ������ָ���Ĳ��֣����������������ʹ���޷�ȷ�����ȥ�滻��

��locationͨ��������ʽָ����
����ʹ�ô����location������rewriteָ��ı�URI��ʹ��������ÿ��Ը��Ӿ�ȷ�Ĵ�������break����

location  /name/ {
  rewrite      /name/([^/] +)  /users?name=$1  break;
  proxy_pass   http://127.0.0.1;
}��Щ�����URI��û�б�ӳ�䴫�ݡ�
���⣬��Ҫ����һЩ����Ա�URI���ԺͿͻ�����ͬ�ķ�����ʽת���������Ǵ��������ʽ�����䴦���ڼ䣺


���������ϵ�б�ܽ����滻Ϊһ���� ��//�� �C ��/��; 

��ɾ�����õĵ�ǰĿ¼����/./�� �C ��/��; 

��ɾ�����õ���ǰĿ¼����/dir /../�� �C ��/����
����ڷ������ϱ�����δ���κδ������ʽ����URI����ô��proxy_passָ���б���ʹ��δָ��URI�Ĳ��֣�

location  /some/path/ {
  proxy_pass   http://127.0.0.1;
}��ָ����ʹ�ñ�����һ�ֱȽ������������������URL����ʹ�ò����������ȫ�ֹ����URL��
����ζ�����е����ò��������㷽��Ľ���ĳ������Ҫ����������Ŀ¼���������ǽ���ת������ͬ��URL����һ��server�ֶε����ã���


location / {
  proxy_pass   http://127.0.0.1:8080/VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot;
}���������ʹ��rewrite��proxy_pass����ϣ�

location / {
  rewrite ^(.*)$ /VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot$1 break;
  proxy_pass   http://127.0.0.1:8080;
}��������������URL������д�� proxy_pass�е���βб�ܲ�û��ʵ�����塣
�����Ҫͨ��ssl�������ӵ�һ�����η������飬proxy_passǰ׺Ϊ https://������ͬʱָ��ssl�Ķ˿ڣ��磺


upstream backend-secure {
  server 10.0.0.20:443;
}
 
server {
  listen 10.0.0.1:443;
  location / {
    proxy_pass https://backend-secure;
  }
}
*/
//proxy_pass���� //�����������nginx����proxy_passָ���ʱ�򣬾͵��ã�Ȼ��������صĻص���������Ӧ��ģ����ȥ
static char *
ngx_http_proxy_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{//��NGX_HTTP_CONTENT_PHASE����������handler����ngx_http_proxy_handler��  
    ngx_http_proxy_loc_conf_t *plcf = conf;

    size_t                      add;
    u_short                     port;
    ngx_str_t                  *value, *url;
    ngx_url_t                   u;
    ngx_uint_t                  n;
    ngx_http_core_loc_conf_t   *clcf;
    ngx_http_script_compile_t   sc;

    if (plcf->upstream.upstream || plcf->proxy_lengths) {
        return "is duplicate";
    }

    //��ȡ��ǰ��location�������ĸ�location���õ�"proxy_pass"ָ��  
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    //����loc��handler�����clcf->handler����ngx_http_update_location_config()���渳��r->content_handler����
    // ����NGX_HTTP_CONTENT_PHASE����������handler����ngx_http_proxy_handler��  
    clcf->handler = ngx_http_proxy_handler;

    if (clcf->name.data[clcf->name.len - 1] == '/') {
        clcf->auto_redirect = 1;
    }

    value = cf->args->elts;

    url = &value[1];
    //
    n = ngx_http_script_variables_count(url);

    if (n) {//proxy_pass xxxx���Ƿ���б���������б������ͽ�����Ӧ�ı���ֵ

        ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

        sc.cf = cf;
        sc.source = url;
        sc.lengths = &plcf->proxy_lengths;
        sc.values = &plcf->proxy_values;
        sc.variables = n;
        sc.complete_lengths = 1;
        sc.complete_values = 1;

        if (ngx_http_script_compile(&sc) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

#if (NGX_HTTP_SSL)
        plcf->ssl = 1;
#endif

        return NGX_CONF_OK;
    }

    //proxypass����Ĳ���������http://����https://��ͷ
    if (ngx_strncasecmp(url->data, (u_char *) "http://", 7) == 0) {
        add = 7;
        port = 80;

    } else if (ngx_strncasecmp(url->data, (u_char *) "https://", 8) == 0) {

#if (NGX_HTTP_SSL)
        plcf->ssl = 1;

        add = 8;
        port = 443;
#else
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "https protocol requires SSL support");
        return NGX_CONF_ERROR;
#endif

    } else {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid URL prefix");
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url.len = url->len - add; //����url�ĳ��ȣ���ȥhttp://(https://)  
    u.url.data = url->data + add; //����url����ȥhttp://(https://)������ԭ����"http://backend1"�����ھ���"backend1"  
    u.default_port = port; //Ĭ��port   80http  443htps ǰ�渳ֵ
    u.uri_part = 1;
    u.no_resolve = 1; //��Ҫresolve���url������  

    //��������server��upstream���뵽upstream����,�� upstream {}���ƣ����൱��һ��upstream{}���ÿ�
    plcf->upstream.upstream = ngx_http_upstream_add(cf, &u, 0); //upstream�б������ȡ��http://ͷ������Ϣ
    if (plcf->upstream.upstream == NULL) { 
        return NGX_CONF_ERROR;
    }

    plcf->vars.schema.len = add;
    plcf->vars.schema.data = url->data;
    plcf->vars.key_start = plcf->vars.schema;

    ngx_http_proxy_set_vars(&u, &plcf->vars);

    plcf->location = clcf->name;

    if (clcf->named
#if (NGX_PCRE)
        || clcf->regex
#endif
        || clcf->noname)
    {
        if (plcf->vars.uri.len) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "\"proxy_pass\" cannot have URI part in "
                               "location given by regular expression, "
                               "or inside named location, "
                               "or inside \"if\" statement, "
                               "or inside \"limit_except\" block");
            return NGX_CONF_ERROR;
        }

        plcf->location.len = 0;
    }

    plcf->url = *url; 
    
    return NGX_CONF_OK;
}

/*
�﷨��proxy_redirect [ default|off|redirect replacement ];
Ĭ��ֵ��proxy_redirect default;
ʹ���ֶΣ�http, server, location
���ָ��Ϊ�����������Ӧ���б���ı��Ӧ��ͷ����Location���͡�Refresh������ֵ��
���Ǽ��豻����ķ��������ص�Ӧ��ͷ�ֶ�Ϊ��Location: http://localhost:8000/two/some/uri/��
ָ�


proxy_redirect http://localhost:8000/two/ http://frontend/one/;�Ὣ����дΪ��Location: http://frontend/one/some/uri/��
����д�ֶ�������Բ�ʹ�÷���������

proxy_redirect http://localhost:8000/two/ /;������Ĭ�ϵķ��������Ͷ˿ڽ������ã��˿�Ĭ��80��
Ĭ�ϵ���д����ʹ�ò���default����ʹ��location��proxy_pass��ֵ��
�������������ǵȼ۵ģ�


location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   default;
}
 
location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   http://upstream:port/two/   /one/;
}ͬ��������д�ֶ��п���ʹ�ñ�����


proxy_redirect   http://localhost:8000/    http://$host:$server_port/;���ָ������ظ�ʹ�ã�


proxy_redirect   default;
proxy_redirect   http://localhost:8000/    /;
proxy_redirect   http://www.example.com/   /;����off�ڱ����н������е�proxy_redirectָ�


proxy_redirect   off;
proxy_redirect   default;
proxy_redirect   http://localhost:8000/    /;
proxy_redirect   http://www.example.com/   /;���ָ����Ժ����׵Ľ�������������ķ���������дΪ����������ķ���������

proxy_redirect   /   /;
*/ 

//ngx_http_proxy_redirect��ngx_http_proxy_rewrite_redirect����Ķ�������һ��ȷ���ض����ĵ�ַ
static char *
ngx_http_proxy_redirect(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    u_char                            *p;
    ngx_str_t                         *value;
    ngx_http_proxy_rewrite_t          *pr;
    ngx_http_compile_complex_value_t   ccv;

    if (plcf->redirect == 0) {
        return NGX_CONF_OK;
    }

    plcf->redirect = 1;

    value = cf->args->elts;

    if (cf->args->nelts == 2) {
        if (ngx_strcmp(value[1].data, "off") == 0) {
            plcf->redirect = 0;
            plcf->redirects = NULL;
            return NGX_CONF_OK;
        }

        if (ngx_strcmp(value[1].data, "false") == 0) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "invalid parameter \"false\", use \"off\" instead");
            plcf->redirect = 0;
            plcf->redirects = NULL;
            return NGX_CONF_OK;
        }

        if (ngx_strcmp(value[1].data, "default") != 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid parameter \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }
    }

    if (plcf->redirects == NULL) {
        plcf->redirects = ngx_array_create(cf->pool, 1,
                                           sizeof(ngx_http_proxy_rewrite_t));
        if (plcf->redirects == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    pr = ngx_array_push(plcf->redirects);
    if (pr == NULL) {
        return NGX_CONF_ERROR;
    }

    if (ngx_strcmp(value[1].data, "default") == 0) {
        if (plcf->proxy_lengths) { //default����proxy_pass����Я������
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "\"proxy_redirect default\" cannot be used "
                               "with \"proxy_pass\" directive with variables");
            return NGX_CONF_ERROR;
        }

        if (plcf->url.data == NULL) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "\"proxy_redirect default\" should be placed "
                               "after the \"proxy_pass\" directive");
            return NGX_CONF_ERROR;
        }

        pr->handler = ngx_http_proxy_rewrite_complex_handler;

        ngx_memzero(&pr->pattern.complex, sizeof(ngx_http_complex_value_t));

        ngx_memzero(&pr->replacement, sizeof(ngx_http_complex_value_t));

        if (plcf->vars.uri.len) {
            pr->pattern.complex.value = plcf->url;
            pr->replacement.value = plcf->location;

        } else {
            pr->pattern.complex.value.len = plcf->url.len + sizeof("/") - 1;

            p = ngx_pnalloc(cf->pool, pr->pattern.complex.value.len);
            if (p == NULL) {
                return NGX_CONF_ERROR;
            }

            pr->pattern.complex.value.data = p;

            p = ngx_cpymem(p, plcf->url.data, plcf->url.len);
            *p = '/';

            ngx_str_set(&pr->replacement.value, "/");
        }

        return NGX_CONF_OK;
    }


    if (value[1].data[0] == '~') {
        value[1].len--;
        value[1].data++;

        if (value[1].data[0] == '*') {
            value[1].len--;
            value[1].data++;

            if (ngx_http_proxy_rewrite_regex(cf, pr, &value[1], 1) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

        } else {
            if (ngx_http_proxy_rewrite_regex(cf, pr, &value[1], 0) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }

    } else {

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &value[1];
        ccv.complex_value = &pr->pattern.complex;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        pr->handler = ngx_http_proxy_rewrite_complex_handler;
    }


    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &pr->replacement;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_proxy_cookie_domain(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t                         *value;
    ngx_http_proxy_rewrite_t          *pr;
    ngx_http_compile_complex_value_t   ccv;

    if (plcf->cookie_domains == NULL) {
        return NGX_CONF_OK;
    }

    value = cf->args->elts;

    if (cf->args->nelts == 2) {

        if (ngx_strcmp(value[1].data, "off") == 0) {
            plcf->cookie_domains = NULL;
            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    if (plcf->cookie_domains == NGX_CONF_UNSET_PTR) {
        plcf->cookie_domains = ngx_array_create(cf->pool, 1,
                                     sizeof(ngx_http_proxy_rewrite_t));
        if (plcf->cookie_domains == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    pr = ngx_array_push(plcf->cookie_domains);
    if (pr == NULL) {
        return NGX_CONF_ERROR;
    }

    if (value[1].data[0] == '~') {
        value[1].len--;
        value[1].data++;

        if (ngx_http_proxy_rewrite_regex(cf, pr, &value[1], 1) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

    } else {

        if (value[1].data[0] == '.') {
            value[1].len--;
            value[1].data++;
        }

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &value[1];
        ccv.complex_value = &pr->pattern.complex;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        pr->handler = ngx_http_proxy_rewrite_domain_handler;

        if (value[2].data[0] == '.') {
            value[2].len--;
            value[2].data++;
        }
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &pr->replacement;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_proxy_cookie_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t                         *value;
    ngx_http_proxy_rewrite_t          *pr;
    ngx_http_compile_complex_value_t   ccv;

    if (plcf->cookie_paths == NULL) {
        return NGX_CONF_OK;
    }

    value = cf->args->elts;

    if (cf->args->nelts == 2) {

        if (ngx_strcmp(value[1].data, "off") == 0) {
            plcf->cookie_paths = NULL;
            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    if (plcf->cookie_paths == NGX_CONF_UNSET_PTR) {
        plcf->cookie_paths = ngx_array_create(cf->pool, 1,
                                     sizeof(ngx_http_proxy_rewrite_t));
        if (plcf->cookie_paths == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    pr = ngx_array_push(plcf->cookie_paths);
    if (pr == NULL) {
        return NGX_CONF_ERROR;
    }

    if (value[1].data[0] == '~') {
        value[1].len--;
        value[1].data++;

        if (value[1].data[0] == '*') {
            value[1].len--;
            value[1].data++;

            if (ngx_http_proxy_rewrite_regex(cf, pr, &value[1], 1) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

        } else {
            if (ngx_http_proxy_rewrite_regex(cf, pr, &value[1], 0) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }

    } else {

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &value[1];
        ccv.complex_value = &pr->pattern.complex;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        pr->handler = ngx_http_proxy_rewrite_complex_handler;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &pr->replacement;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_proxy_rewrite_regex(ngx_conf_t *cf, ngx_http_proxy_rewrite_t *pr,
    ngx_str_t *regex, ngx_uint_t caseless)
{
#if (NGX_PCRE)
    u_char               errstr[NGX_MAX_CONF_ERRSTR];
    ngx_regex_compile_t  rc;

    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = *regex;
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    if (caseless) {
        rc.options = NGX_REGEX_CASELESS;
    }

    pr->pattern.regex = ngx_http_regex_compile(cf, &rc);
    if (pr->pattern.regex == NULL) {
        return NGX_ERROR;
    }

    pr->handler = ngx_http_proxy_rewrite_regex_handler;

    return NGX_OK;

#else

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "using regex \"%V\" requires PCRE library", regex);
    return NGX_ERROR;

#endif
}


static char *
ngx_http_proxy_store(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t                  *value;
    ngx_http_script_compile_t   sc;

    if (plcf->upstream.store != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        plcf->upstream.store = 0;
        return NGX_CONF_OK;
    }

#if (NGX_HTTP_CACHE)
    if (plcf->upstream.cache > 0) {
        return "is incompatible with \"proxy_cache\"";
    }
#endif

    plcf->upstream.store = 1;

    if (ngx_strcmp(value[1].data, "on") == 0) {
        return NGX_CONF_OK;
    }

    /* include the terminating '\0' into script */
    value[1].len++;

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = &value[1];
    sc.lengths = &plcf->upstream.store_lengths;
    sc.values = &plcf->upstream.store_values;
    sc.variables = ngx_http_script_variables_count(&value[1]);
    sc.complete_lengths = 1;
    sc.complete_values = 1;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


#if (NGX_HTTP_CACHE)
//proxy_cache abc�����proxy_cache_path xxx keys_zone=abc:10m;һ�𣬷�����ngx_http_proxy_merge_loc_conf��ʧ�ܣ���Ϊû��Ϊ��abc����ngx_http_file_cache_t
static char *
ngx_http_proxy_cache(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t                         *value;
    ngx_http_complex_value_t           cv;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (plcf->upstream.cache != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    if (ngx_strcmp(value[1].data, "off") == 0) {
        plcf->upstream.cache = 0;
        return NGX_CONF_OK;
    }

    if (plcf->upstream.store > 0) {
        return "is incompatible with \"proxy_store\"";
    }

    plcf->upstream.cache = 1;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &cv;

    //����������ʽ�����proxy_cache xxx$ss �е��ַ������������ı���length��value������cv��
    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cv.lengths != NULL) { //˵��proxy_cache�����д��ڱ���

        plcf->upstream.cache_value = ngx_palloc(cf->pool,
                                             sizeof(ngx_http_complex_value_t));
        if (plcf->upstream.cache_value == NULL) {
            return NGX_CONF_ERROR;
        }

        *plcf->upstream.cache_value = cv;

        return NGX_CONF_OK;
    }

    plcf->upstream.cache_zone = ngx_shared_memory_add(cf, &value[1], 0,
                                                      &ngx_http_proxy_module);
    if (plcf->upstream.cache_zone == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_proxy_cache_key(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t                         *value;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (plcf->cache_key.value.data) {
        return "is duplicate";
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &plcf->cache_key;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

#endif


#if (NGX_HTTP_SSL)

static char *
ngx_http_proxy_ssl_password_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_proxy_loc_conf_t *plcf = conf;

    ngx_str_t  *value;

    if (plcf->ssl_passwords != NGX_CONF_UNSET_PTR) {
        return "is duplicate";
    }

    value = cf->args->elts;

    plcf->ssl_passwords = ngx_ssl_read_password_file(cf, &value[1]);

    if (plcf->ssl_passwords == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

#endif


static char *
ngx_http_proxy_lowat_check(ngx_conf_t *cf, void *post, void *data)
{
#if (NGX_FREEBSD)
    ssize_t *np = data;

    if ((u_long) *np >= ngx_freebsd_net_inet_tcp_sendspace) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"proxy_send_lowat\" must be less than %d "
                           "(sysctl net.inet.tcp.sendspace)",
                           ngx_freebsd_net_inet_tcp_sendspace);

        return NGX_CONF_ERROR;
    }

#elif !(NGX_HAVE_SO_SNDLOWAT)
    ssize_t *np = data;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "\"proxy_send_lowat\" is not supported, ignored");

    *np = 0;

#endif

    return NGX_CONF_OK;
}


#if (NGX_HTTP_SSL)

static ngx_int_t
ngx_http_proxy_set_ssl(ngx_conf_t *cf, ngx_http_proxy_loc_conf_t *plcf)
{
    ngx_pool_cleanup_t  *cln;

    plcf->upstream.ssl = ngx_pcalloc(cf->pool, sizeof(ngx_ssl_t));
    if (plcf->upstream.ssl == NULL) {
        return NGX_ERROR;
    }

    plcf->upstream.ssl->log = cf->log;

    if (ngx_ssl_create(plcf->upstream.ssl, plcf->ssl_protocols, NULL)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NGX_ERROR;
    }

    cln->handler = ngx_ssl_cleanup_ctx;
    cln->data = plcf->upstream.ssl;

    if (plcf->ssl_certificate.len) {

        if (plcf->ssl_certificate_key.len == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                          "no \"proxy_ssl_certificate_key\" is defined "
                          "for certificate \"%V\"", &plcf->ssl_certificate);
            return NGX_ERROR;
        }

        if (ngx_ssl_certificate(cf, plcf->upstream.ssl, &plcf->ssl_certificate,
                                &plcf->ssl_certificate_key, plcf->ssl_passwords)
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    if (SSL_CTX_set_cipher_list(plcf->upstream.ssl->ctx,
                                (const char *) plcf->ssl_ciphers.data)
        == 0)
    {
        ngx_ssl_error(NGX_LOG_EMERG, cf->log, 0,
                      "SSL_CTX_set_cipher_list(\"%V\") failed",
                      &plcf->ssl_ciphers);
        return NGX_ERROR;
    }

    if (plcf->upstream.ssl_verify) {
        if (plcf->ssl_trusted_certificate.len == 0) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "no proxy_ssl_trusted_certificate for proxy_ssl_verify");
            return NGX_ERROR;
        }

        if (ngx_ssl_trusted_certificate(cf, plcf->upstream.ssl,
                                        &plcf->ssl_trusted_certificate,
                                        plcf->ssl_verify_depth)
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        if (ngx_ssl_crl(cf, plcf->upstream.ssl, &plcf->ssl_crl) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

#endif


static void
ngx_http_proxy_set_vars(ngx_url_t *u, ngx_http_proxy_vars_t *v)
{   //AF_UNIX�������׽���
    if (u->family != AF_UNIX) {//ngx_http_upstream_add->ngx_parse_inet_url��������ΪAF_INET 

        if (u->no_port || u->port == u->default_port) {

            v->host_header = u->host;

            if (u->default_port == 80) {
                ngx_str_set(&v->port, "80");

            } else {
                ngx_str_set(&v->port, "443"); //https�˿�443
            }

        } else {
            v->host_header.len = u->host.len + 1 + u->port_text.len;
            v->host_header.data = u->host.data;
            v->port = u->port_text;
        }

        v->key_start.len += v->host_header.len;  

        /*
            proxy_pass  http://10.10.0.103:8080/tttxx; 
            printf("yang test ....... no_port:%u, port:%u, default_port:%u, host:%s, key:%s\n", 
                u->no_port, u->port, u->default_port, u->host.data, v->key_start.data);
            yang test ....... no_port:0, port:8080, default_port:80, host:10.10.0.103:8080/tttxx, key:http://10.10.0.103:8080/tttxx
         */
    } else {
        ngx_str_set(&v->host_header, "localhost");
        ngx_str_null(&v->port);
        v->key_start.len += sizeof("unix:") - 1 + u->host.len + 1;
    }

    v->uri = u->uri;
}

