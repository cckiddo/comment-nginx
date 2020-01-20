
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_md5.h>

/*
��֤Ȩ�������ַ�ʽ��һ����secure_link_secret����һ����secure_link+secure_link_md5,���ǲ��ܹ��棬��ngx_http_secure_link_merge_conf

�����secure_link+secure_link_md5��ʽ����һ��ͻ�������uri��Я��secure_link $md5_str, time�е�md5_str�ַ�����nginx�յ��󣬽���64base decode
������ַ����󣬰Ѹ��ַ�����secure_link_md5�ַ�������MD5���㣬ͬʱ�ж�secure_link $md5_str, time�е�time�͵�ǰʱ���Ƿ���ڣ�����ַ���
�Ƚ���ͬ����û�й��ڣ����ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'����ngx_http_secure_link_variable��ͬʱ��secure_link $md5_str, time
�е�time�������$secure_link_expires

�����secure_link_secret��ʽ�����ǰѿͻ�������uri�е�/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx�е�5e814704a28d9bc1914ff19fa0c4a00a�ַ�����
secure_link_secret xxx�������MD5ֵ���бȽϣ���ͬ����Ȩ�ޣ��ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'���÷�ʽ���漰time����
*/

//�ο�ngx_http_secure_link_commands
typedef struct {
    ngx_http_complex_value_t  *variable; //secure_link md5_str, 120���ã� ��ngx_http_set_complex_value_slot
//��������echo -n 'secure_link_md5���õ��ַ���' |     openssl md5 -binary | openssl base64 | tr +/ -_ | tr -d = ��ȡ�ַ�����Ӧ��MD5ֵ
    ngx_http_complex_value_t  *md5;//secure_link_md5 $the_uri_you_want_to_hashed_by_md5����
//��������echo -n 'secure_link_secret���õ��ַ���' | openssl md5 -hex  ��ȡsecure_link_secret�������õ�MD5��Ϣ
    ngx_str_t                  secret;//secure_link_secret����
} ngx_http_secure_link_conf_t;


/*
��֤Ȩ�������ַ�ʽ��һ����secure_link_secret����һ����secure_link+secure_link_md5,���ǲ��ܹ��棬��ngx_http_secure_link_merge_conf

�����secure_link+secure_link_md5��ʽ����һ��ͻ�������uri��Я��secure_link $md5_str, time�е�md5_str�ַ�����nginx�յ��󣬽���64base decode
������ַ����󣬰Ѹ��ַ�����secure_link_md5�ַ�������MD5���㣬ͬʱ�ж�secure_link $md5_str, time�е�time�͵�ǰʱ���Ƿ���ڣ�����ַ���
�Ƚ���ͬ����û�й��ڣ����ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'����ngx_http_secure_link_variable��ͬʱ��secure_link $md5_str, time
�е�time�������$secure_link_expires

�����secure_link_secret��ʽ�����ǰѿͻ�������uri�е�/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx�е�5e814704a28d9bc1914ff19fa0c4a00a�ַ�����
secure_link_secret xxx�������MD5ֵ���бȽϣ���ͬ����Ȩ�ޣ��ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'���÷�ʽ���漰time����

��˿���secure_linkһ����Ҫ�ͻ�����������ʹ�ã�һ����Ҫ�������֧��
*/
typedef struct {
    ngx_str_t                  expires;//secure_link md5_str, time�����е�time�ַ�������ngx_http_secure_link_variable
} ngx_http_secure_link_ctx_t;


static ngx_int_t ngx_http_secure_link_old_variable(ngx_http_request_t *r,
    ngx_http_secure_link_conf_t *conf, ngx_http_variable_value_t *v,
    uintptr_t data);
static ngx_int_t ngx_http_secure_link_expires_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void *ngx_http_secure_link_create_conf(ngx_conf_t *cf);
static char *ngx_http_secure_link_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static ngx_int_t ngx_http_secure_link_add_variables(ngx_conf_t *cf);

/*
����nginx
server {

    listen       80;
    server_name  s1.down.ttlsa.com;
    access_log  /data/logs/nginx/s1.down.ttlsa.com.access.log  main;

    index index.html index.<a href="http://www.ttlsa.com/php/" title="php"target="_blank">php</a> index.html;
    root /data/site/s1.down.ttlsa.com;

    location / {
        secure_link $arg_st,$arg_e;
        secure_link_md5 ttlsa.com$uri$arg_e;

        if ($secure_link = "") {
            return 403;
        }

        if ($secure_link = "0") {
            return 403;
        }
    }
}

php����ҳ��

<?php
 # ���ã�����nginx secure link����
 # վ�㣺www.ttlsa.com
 # ���ߣ����׿�
 # ʱ�䣺2013-09-11
$secret = 'ttlsa.com'; # ��Կ
 $path = '/web/nginx-1.4.2.tar.gz'; # �����ļ�
 # ���ص���ʱ��,time�ǵ�ǰʱ��,300��ʾ300��,Ҳ����˵�����ڵ�300��֮���ļ�������
 $expire = time()+300;
# ���ļ�·������Կ������ʱ�����ɼ��ܴ�
 $md5 = base64_encode(md5($secret . $path . $expire, true));
 $md5 = strtr($md5, '+/', '-_');
 $md5 = str_replace('=', '', $md5);
# ���ܺ�����ص�ַ
 echo '<a href=http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st='.$md5.'&e='.$expire.'>nginx-1.4.2</a>';
 echo '<br>http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st='.$md5.'&e='.$expire;
 ?>


����nginx������
��http://test.ttlsa.com/down.php����������������
 ���ص�ַ���£�
http://s1.down.ttlsa.com/web/nginx-1.4.2.tar.gz?st=LSVzmZllg68AJaBmeK3E8Q&e=1378881984
ҳ�治Ҫˢ�£��ȵ�5���Ӻ�������һ�Σ���ᷢ�ֵ�����ػ���ת��403ҳ�档

secure link ������ԭ��
?�û�����down.php
?down.php����secret��Կ������ʱ�䡢�ļ�uri���ɼ��ܴ�
?�����ܴ������ʱ����Ϊ���������ļ����ص�ַ�ĺ���
?nginx���ط��������յ��˹���ʱ�䣬Ҳʹ�ù���ʱ�䡢��������Կ���ļ�uri���ɼ��ܴ�
?���û��������ļ��ܴ����Լ����ɵļ��ܴ����жԱȣ�һ���������أ���һ��403

��������ʵ���Ϻܼ򵥣��������û�������֤. ��Ϊע���һ���Ǵ��һ����Ҫй¶���Լ�����Կ��������˾Ϳ��Ե����ˣ�����й¶֮������ܾ���������Կ.

*/  

//ngx_http_secure_link_module���ڿ��Դ���ngx_http_accesskey_module�����ǹ�������   ngx_http_secure_link_module Nginx�İ�ȫģ��,��ñ�����webserverȨ�ޡ�
//ngx_http_referer_module������ͨ����������


/*
The ngx_http_secure_link_module module (0.7.18) is used to check authenticity of requested links, protect resources 
from unauthorized access, and limit link lifetime. 
��ģ���ǿ��ƿͻ��˵ķ���Ȩ�޵ģ����ĳ�η�����Ȩ�ޣ����Է��ʣ�Ҳ���Կ�������ڶ೤ʱ����ڿ��Է��ʸ��ļ�������Դ
*/

/*
��֤Ȩ�������ַ�ʽ��һ����secure_link_secret����һ����secure_link+secure_link_md5,���ǲ��ܹ��棬��ngx_http_secure_link_merge_conf

�����secure_link+secure_link_md5��ʽ����һ��ͻ�������uri��Я��secure_link $md5_str, time�е�md5_str�ַ�����nginx�յ��󣬽���64base decode
������ַ����󣬰Ѹ��ַ�����secure_link_md5�ַ�������MD5���㣬ͬʱ�ж�secure_link $md5_str, time�е�time�͵�ǰʱ���Ƿ���ڣ�����ַ���
�Ƚ���ͬ����û�й��ڣ����ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'����ngx_http_secure_link_variable��ͬʱ��secure_link $md5_str, time
�е�time�������$secure_link_expires

�����secure_link_secret��ʽ�����ǰѿͻ�������uri�е�/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx�е�5e814704a28d9bc1914ff19fa0c4a00a�ַ�����
secure_link_secret xxx�������MD5ֵ���бȽϣ���ͬ����Ȩ�ޣ��ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'���÷�ʽ���漰time����
*/
static ngx_command_t  ngx_http_secure_link_commands[] = {
    /*
    �﷨��secure_link $md5_hash[,$expire_time] 
    Ĭ��ֵ��none 
    ʹ���ֶΣ�location
    ���ָ��ָ���������URL��MD5��ϣֵ�͹���ʱ�䣬$md5_hashΪ����URL��64λ���룬$expired_timeΪ��1970-01-01 00:00:00 UTCʱ��������������
    ����㲻����$expire_time����ô���URL��Զ������ڡ�
    */ //�ͻ�����Ȩ�޷��ʸ���Դ���೤ʱ���ڶ����Է��ʣ��������ʱ��ξͲ��ܷ����ˣ�ֻ�����»�ȡȨ��

    /*
       secure_link $md5_str, time�е�md5_strһ���Ǵӿͻ��������е�uri��ȡ����ȡ�������64λdecode��Ȼ����secure_link_md5���õ��ַ�������MD5
       ֵ�����Ľ���Ƚϣ�ͬʱ��time�������¼��Ƚϣ����Ƿ���ڣ��ȽϽ����ͬ����û�й�������ΪȨ��ͨ��������$secure_linkΪ'1'������ͨ��
       ����Ϊ��0���ο�ngx_http_secure_link_variable

       ��ʱ������°�$secure_link����Ϊ��0������ʾ���ڡ����ƥ��MD5_strʧ�ܣ���ʾ��Ч��$secure_link����Ϊ���ַ�����

       ע��md5_str�ַ������Ȳ��ܳ���24�ֽڣ��ο�ngx_http_secure_link_variable,MD5ֵ�Ƚ�Ҳ�ο�ngx_http_secure_link_variable
     */ 
    //���������secure_link_secret�������� secure_link secure_link_md5
     //secure_link��Ч�Ƚϼ�ngx_http_secure_link_variable����ngx_http_secure_link_merge_conf

    /*
��֤Ȩ�������ַ�ʽ��һ����secure_link_secret����һ����secure_link+secure_link_md5,���ǲ��ܹ��棬��ngx_http_secure_link_merge_conf

�����secure_link+secure_link_md5��ʽ����һ��ͻ�������uri��Я��secure_link $md5_str, time�е�md5_str�ַ�����nginx�յ��󣬽���64base decode
������ַ����󣬰Ѹ��ַ�����secure_link_md5�ַ�������MD5���㣬ͬʱ�ж�secure_link $md5_str, time�е�time�͵�ǰʱ���Ƿ���ڣ�����ַ���
�Ƚ���ͬ����û�й��ڣ����ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'����ngx_http_secure_link_variable��ͬʱ��secure_link $md5_str, time
�е�time�������$secure_link_expires

�����secure_link_secret��ʽ�����ǰѿͻ�������uri�е�/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx�е�5e814704a28d9bc1914ff19fa0c4a00a�ַ�����
secure_link_secret xxx�������MD5ֵ���бȽϣ���ͬ����Ȩ�ޣ��ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'���÷�ʽ���漰time����
*/ //secure_link $MD5_STR���Դ�uri�л�ȡ�����Բο�ngx_http_arg
     
    { ngx_string("secure_link"),   //ע�⻹���ڱ���$secure_link_expires�ͱ���$secure_link
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_secure_link_conf_t, variable),
      NULL },

/*
�﷨��secure_link_md5 $the_uri_you_want_to_hashed_by_md5
Ĭ��ֵ��none 
ʹ���ֶΣ�location
���ָ��ָ������Ҫͨ��MD5��ϣ���ַ������ַ������԰�����������ϣֵ���͡�secure_link�����õ�$md5_hash�������бȽϣ���������ͬ��
$secure_link����ֵΪ1������Ϊ���ַ�����
*/ //secure_link_md5��Ч�Ƚϼ�ngx_http_secure_link_variable 
//���������secure_link_secret�������� secure_link secure_link_md5����ngx_http_secure_link_merge_conf
    { ngx_string("secure_link_md5"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_secure_link_conf_t, md5),
      NULL },

    /*
    �﷨��secure_link_secret secret_word 
    Ĭ��ֵ��none 
    ʹ���ֶΣ�location
    ָ��Ϊ�������ָ��һ�������ֶΣ�һ�����������ӵ�������ַ���£�
    /prefix/MD5 hash/reference
    ��secretָ����ָ����secret_word��������Ϊһ��MD5ֵ���������ܱ����������У����磺�ܱ������ļ�top_secret_file.pdfλ��Ŀ¼p��nginx����Ϊ��
    location /p/ {
        secure_link_secret segredo;
     
        if ($secure_link = "") {
            return 403;
        }
     
        rewrite ^ /p/$secure_link break;
    }
    
    �����ͨ��openssl����������MD5ֵ��
    echo -n 'top_secret_file.pdfsegredo' | openssl dgst -md5
    
    �õ���ֵΪ0849e9c72988f118896724a0502b92a8������ͨ�������ױ��������ӷ���:
    http://example.com/p/0849e9c72988f118896724a0502b92a8/top_secret_file.pdf
    
    ע�⣬����ʹ�ø�·������/�����磺
    location / {
   #����ʹ��/
       secure_link_secret segredo;
       [...]
    }
    */ //���������secure_link_secret�������� secure_link secure_link_md5����ngx_http_secure_link_merge_conf

        /*
    ��֤Ȩ�������ַ�ʽ��һ����secure_link_secret����һ����secure_link+secure_link_md5,���ǲ��ܹ��棬��ngx_http_secure_link_merge_conf
    
    �����secure_link+secure_link_md5��ʽ����һ��ͻ�������uri��Я��secure_link $md5_str, time�е�md5_str�ַ�����nginx�յ��󣬽���64base decode
    ������ַ����󣬰Ѹ��ַ�����secure_link_md5�ַ�������MD5���㣬ͬʱ�ж�secure_link $md5_str, time�е�time�͵�ǰʱ���Ƿ���ڣ�����ַ���
    �Ƚ���ͬ����û�й��ڣ����ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'����ngx_http_secure_link_variable��ͬʱ��secure_link $md5_str, time
    �е�time�������$secure_link_expires
    
    �����secure_link_secret��ʽ�����ǰѿͻ�������uri�е�/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx�е�5e814704a28d9bc1914ff19fa0c4a00a�ַ�����
    secure_link_secret xxx�������MD5ֵ���бȽϣ���ͬ����Ȩ�ޣ��ñ���$secure_linkΪ'1'�������ӱ���Ϊ��0'���÷�ʽ���漰time����
    */

    { ngx_string("secure_link_secret"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_secure_link_conf_t, secret),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_secure_link_module_ctx = {
    ngx_http_secure_link_add_variables,    /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_secure_link_create_conf,      /* create location configuration */
    ngx_http_secure_link_merge_conf        /* merge location configuration */
};

//ngx_http_secure_link_module���ڿ��Դ���ngx_http_accesskey_module�����ǹ�������   ngx_http_secure_link_module Nginx�İ�ȫģ��,��ñ�����webserverȨ�ޡ�
//ngx_http_referer_module������ͨ����������

//��ģ��һ����Ҫר�ŵĿͻ���֧��
ngx_module_t  ngx_http_secure_link_module = { 
//����Ȩ�޿������ģ��:nginx���з������Ƶ���ngx_http_access_moduleģ��� ngx_http_auth_basic_moduleģ��   ngx_http_secure_link_module
    NGX_MODULE_V1,
    &ngx_http_secure_link_module_ctx,      /* module context */
    ngx_http_secure_link_commands,         /* module directives */
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


/*
����

$secure_link
�������Ƿ�ʹ�á�secure_link_secret�������ֵ��������ͬ�����壺
���ʹ�á�secure_link_secret����������֤��URLͨ����֤�����ֵΪtrue������Ϊ���ַ�����
���ʹ�á�secure_link���͡�secure_link_md5����������֤��URLͨ����֤$secure_linkΪ'1'���������ʱ�䳬��$expire_time, $secure_linkֵΪ'0'�����򣬽�Ϊ���ַ�����

$secure_link_expires
���ڱ���$expire_time��ֵ��

The status of these checks is made available in the $secure_link variable. 
//�Ƿ���Ȩ�޷��ʵ��жϽ���洢��secure_link�У�Ϊ1���Է��ʣ�Ϊ0���ܷ���

*/ 

//$secure_link������ֵ��ngx_http_secure_link_variable
static ngx_str_t  ngx_http_secure_link_name = ngx_string("secure_link");
//$secure_link_expires������ֵ��ngx_http_secure_link_expires_variable,��ʾsecure_link $md5_str, time�е�time��ÿ�οͻ����������
//��Ҫ�жϸ�ʱ�䣬������$secure_linkΪ0��û������1����ngx_http_secure_link_variable
static ngx_str_t  ngx_http_secure_link_expires_name =
    ngx_string("secure_link_expires");

//��ȡsecure_link���õ�ֵ��ͬʱ��secure_link_md5�����ֵ���бȽϣ���ͬ����$secure_link����Ϊ1����ͬ����0
static ngx_int_t
ngx_http_secure_link_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                       *p, *last;
    ngx_str_t                     val, hash;
    time_t                        expires;
    ngx_md5_t                     md5;
    ngx_http_secure_link_ctx_t   *ctx;
    ngx_http_secure_link_conf_t  *conf;
    u_char                        hash_buf[16], md5_buf[16];

    conf = ngx_http_get_module_loc_conf(r, ngx_http_secure_link_module);

    if (conf->secret.data) {
        return ngx_http_secure_link_old_variable(r, conf, v, data);
    }

    if (conf->variable == NULL || conf->md5 == NULL) {
        goto not_found;
    }

    if (ngx_http_complex_value(r, conf->variable, &val) != NGX_OK) {
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "secure link: \"%V\"", &val);

    last = val.data + val.len;

    p = ngx_strlchr(val.data, last, ','); ////secure_link md5_str, 120���ã�
    expires = 0;

    if (p) {
        val.len = p++ - val.data; //��ʱ��val.len=md5_str���ַ��������ˣ������������,120�ַ���

        expires = ngx_atotm(p, last - p); //��ȡ//secure_link md5_str, 120�����е�120
        if (expires <= 0) {
            goto not_found;
        }

        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_secure_link_ctx_t));
        if (ctx == NULL) {
            return NGX_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_secure_link_module);

        ctx->expires.len = last - p;
        ctx->expires.data = p;
    }

    //��ʱ��val.len=md5_str���ַ��������ˣ������������,120�ַ���
    if (val.len > 24) {//secure_link���ý��������ַ������ܳ���24���ַ� ?????? Ϊʲô��
        goto not_found;
    }

    hash.len = 16;
    hash.data = hash_buf;

    //��secure_link md5_str, 120�����е�md5_str�ַ�������base64����decode������ֵ�浽������    
    if (ngx_decode_base64url(&hash, &val) != NGX_OK) {
        goto not_found;
    }

    if (hash.len != 16) {
        goto not_found;
    }

    //����secure_link_md5 $the_uri_you_want_to_hashed_by_md5���õ��ַ���
    if (ngx_http_complex_value(r, conf->md5, &val) != NGX_OK) {
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "secure link md5: \"%V\"", &val);

    //echo -n 'ttlsa.com/download/nginx-1.9.2.rar1452130593' |     openssl md5 -binary | openssl base64 | tr +/ -_ | tr -d =
    ngx_md5_init(&md5);
    ngx_md5_update(&md5, val.data, val.len);
    ngx_md5_final(md5_buf, &md5);

    if (ngx_memcmp(hash_buf, md5_buf, 16) != 0) { //�Ƚ��ж��Ƿ�һ��
        goto not_found;
    }

    //����v������Ӧ��ֵΪ0����1
    v->data = (u_char *) ((expires && expires < ngx_time()) ? "0" : "1");
    v->len = 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;

not_found:

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_secure_link_old_variable(ngx_http_request_t *r,
    ngx_http_secure_link_conf_t *conf, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    u_char      *p, *start, *end, *last;
    size_t       len;
    ngx_int_t    n;
    ngx_uint_t   i;
    ngx_md5_t    md5;
    u_char       hash[16];

    p = &r->unparsed_uri.data[1];
    last = r->unparsed_uri.data + r->unparsed_uri.len;

    // ��ȡ/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx������5e814704a28d9bc1914ff19fa0c4a00a�ַ���
    while (p < last) {
        if (*p++ == '/') {
            start = p;
            goto md5_start;
        }
    }

    goto not_found;

md5_start:
    // ��ȡ/p/5e814704a28d9bc1914ff19fa0c4a00a/link/xx������/link/xx�ַ�����Ϊ�µ�uri
    while (p < last) {
        if (*p++ == '/') {
            end = p - 1;
            goto url_start;
        }
    }

    goto not_found;

url_start:

    len = last - p;

    //5e814704a28d9bc1914ff19fa0c4a00a��������32�ֽڣ���ΪMD5��������32�ֽ�
    if (end - start != 32 || len == 0) {
        goto not_found;
    }

    //����secure_link_secret���õ��ַ�����MD5У��ֵ
    ngx_md5_init(&md5);
    ngx_md5_update(&md5, p, len);
    ngx_md5_update(&md5, conf->secret.data, conf->secret.len);
    ngx_md5_final(hash, &md5);

    for (i = 0; i < 16; i++) {
        n = ngx_hextoi(&start[2 * i], 2);
        if (n == NGX_ERROR || n != hash[i]) {
            goto not_found;
        }
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;

not_found:

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_secure_link_expires_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_secure_link_ctx_t  *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_secure_link_module);

    if (ctx) {
        v->len = ctx->expires.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = ctx->expires.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static void *
ngx_http_secure_link_create_conf(ngx_conf_t *cf)
{
    ngx_http_secure_link_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_secure_link_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->variable = NULL;
     *     conf->md5 = NULL;
     *     conf->secret = { 0, NULL };
     */

    return conf;
}


static char *
ngx_http_secure_link_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_secure_link_conf_t *prev = parent;
    ngx_http_secure_link_conf_t *conf = child;

    if (conf->secret.data) { //���������secure_link_secret�������� secure_link secure_link_md5
        if (conf->variable || conf->md5) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "\"secure_link_secret\" cannot be mixed with "
                               "\"secure_link\" and \"secure_link_md5\"");
            return NGX_CONF_ERROR;
        }

        return NGX_CONF_OK;
    }

    if (conf->variable == NULL) {
        conf->variable = prev->variable;
    }

    if (conf->md5 == NULL) {
        conf->md5 = prev->md5;
    }

    if (conf->variable == NULL && conf->md5 == NULL) {
        conf->secret = prev->secret;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_secure_link_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_secure_link_name, 0);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_secure_link_variable;

    var = ngx_http_add_variable(cf, &ngx_http_secure_link_expires_name, 0);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_secure_link_expires_variable;

    return NGX_OK;
}
