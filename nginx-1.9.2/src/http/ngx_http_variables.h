
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_VARIABLES_H_INCLUDED_
#define _NGX_HTTP_VARIABLES_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef ngx_variable_value_t  ngx_http_variable_value_t;

#define ngx_http_variable(v)     { sizeof(v) - 1, 1, 0, 0, 0, (u_char *) v }

typedef struct ngx_http_variable_s  ngx_http_variable_t;
//�ο�<��������nginx-����>
typedef void (*ngx_http_set_variable_pt) (ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
typedef ngx_int_t (*ngx_http_get_variable_pt) (ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


/*
NGX_HTTP_VAR_CHANGEABLE�������ظ����壻
NGX_HTTP_VAR_NOCACHEABLE������ֵ�����Ա����棬ÿ��ʹ�ñ�����㣻
NGX_HTTP_VAR_INDEXED��ָʾ����ֵ�洢�������У���ʹ��ngx_http_get_variable������ȡ����ʱ����ÿ�ζ�Ϊ��������ֵ�ռ䣻
NGX_HTTP_VAR_NOHASH�����ý������Ժ󣬱���������hash��������������ʱ�������ñ��������ʱ�����


NGX_HTTP_VAR_CHANGEABLE��ʾ�����ǿ��Ը��ĵġ�������set�����ı������ǿɸı�ģ�ͬʱҲ�в��ɸı�ġ�����ǿɸı����
Ҳ����˵��������ǲ��ܻ����(NGX_HTTP_VAR_NOCACHEABLE)��
*/ //ngx_http_add_variable������flag����

/*
����Ƭ��8.2-6���ļ�����nginx��conf
50��    set $file t_a;
51��    set $file t_b;
    ��ʱ��setָ����ظ���ӱ���$file����ʵ����51�в�������������$file����Ϊ�������Ĺ����з����Ѿ��иñ����ˣ�������NGX_HTTP_VAR_CHANGEABLE
�ģ����Ծͷ��ظñ���ʹ�գ�������������ֵ��Ϊt_b���������һ������NGX_HTTP_VAR_CHANGEABLE�ı���$t_var����ôNginx����ʾthe duplicate��t_var��variable���˳�ִ�С�
*/
#define NGX_HTTP_VAR_CHANGEABLE   1

/*
�������ΪNGX_HTTP_VAR_NOCACHEABLE����ʾ�ñ������ɻ��档���Ƕ�֪����������Щ�����������Ǹ���ͻ��������ÿ�����Ӷ���ģ�����
����$http_user_agent�����ſͻ���ʹ��������Ĳ�ͬ����ͬ����ֻ�ڿͻ��˵�ͬһ���������������϶����ᷢ���ı䣬��������һ������
ǰ�����IE�������������Opera�������������������ǿɻ���ģ�ס��������ͻ������ӵ����������У�����$http_user_agentֵ����һ�ξ����ˣ�����ʹ
�ÿ�ֱ��ʹ���仺�档Ȼ������һЩ��������ΪNginx������ڲ�����ᷢ���ı䣬�������$uri����Ȼ�ͻ��˷���������������URI��/thread-3760675-2-l.html��
��ͨ��rewriteת��ȴ�����/thread.php?id=3760675&page=2&flOOFI��Ҳ���Ǳ���$uri�����˸ı䣬���Զ��ڱ���$uri��ÿ��ʹ�ö���������������㣨������
�ص�get_handler0���������ñ��Ӱ����߼���Ҫ�Ǳ���ȡֵ����ngx_http_get_flushed_variable����Ȼ�����������ȷ֪����ǰ��ϸ���������ʱ�������Ͽ��ǣ�
Ҳ��һ���ͷ�Ҫ���¼����ȡֵ������ո�ͨ�����������ȡ�˱���$uri��ֵ������������ȥ��ȡ����$uri��ֵ�����������Ȼ�У�����������$uri������ֵ��
ֵ������������ͬ����������ʱ��ʹ������һ��ȡֵ����ngx_http_get_indexed_variable��ֱ��ȡֵ���������Ƿ�ɻ����ǡ�
*/
#define NGX_HTTP_VAR_NOCACHEABLE  2

/*
NGX HTTP_VAR_INDEXED��NGXHTTP_VARNOHASH������cmcf->variables_hash�Լ�ȡֵ����ngx_http_get_variable�ȣ����Ƕ���ΪSSIģ��ʵ�ֶ���Ƶ�
*/
#define NGX_HTTP_VAR_INDEXED      4
#define NGX_HTTP_VAR_NOHASH       8

/* 
�����Ķ�������ģ�������ɵ� (�ڽ�����)������������ָ����� (�û�����)������д�����ļ�ʱ��ֻ��ʹ���Ѿ�������ı�����
�����Ķ������ṩ������ģ������û�����ָ���� (�� set, geo ��) ��ɣ�

Ȼ����Nginx ����ģ�鶨��ı�������ֻ�в��ֳ��ֵ��������ļ� (nginx.conf) �У�Ҳ����˵���ڶ������ļ��е� http {} ��������н���
�Ĺ����У��������������еı��������� Nginx ���й����б�ʹ�õ��ı�����Nginx ���� ngx_http_get_variable_index ������ cmcf->variables 
�и��������г��ֵı������η��� ngx_http_variable_t ���͵Ŀռ䣬������Ӧ�������������ظ����ô˱�����������Ľ����ص�������

����ģ�� ngx_http_core_module �ṩ�ı���ʹ�� ngx_http_core_variables �������� preconfiguration �ص����� ngx_http_variables_add_core_vars ���ж��壺
�Ǻ���ģ����� ngx_http_fastcgi_module �ṩ�ı���ʹ�� ngx_http_fastcgi_vars �������� preconfiguration �ص����� ngx_http_fastcgi_add_variables ���ж��壺
�� ngx_http_rewrite_module �ṩ�� set ָ�����Զ�������������ý������� ngx_http_rewrite_set ���ж��壺


��ȡ������ֵ
�ű�����
ʲô�����棿�ӻ�е������˵���ǰ�����ת���ɻ�е�˶����豸���ڼ����ѧ��������һ����ָ����������ת����������ʽ����ʽ����������ģ�顣

    Nginx ���������������������ת����һϵ�нű������ں��ʵ�ʱ����ͨ���ű�����������Щ�ű����õ�����������ֵ�����仰˵���ű����渺�𽫲�
���еı����ں��ʵ�ʱ��ȡֵ���Ͳ����еĹ̶��ַ���ƴ�ӳ������ַ�����

    �Բ����Ľű���������Ҳ�������������������ɡ�Ϊ�˱����壬ѡ�������һ�����������з�������������ʹ Nginx ���õ������Ӧ�� Host 
��Ϣ�󣬱��磬"example.com"�����ű����洦��ƴ�ӳ� "example.com/access.log" ���ú��� access log ��Ŀ���ļ�����
    access_log  ${host}/access.log;            ������ȡ���ű�������̿��Բο�ngx_http_script_compile_t������������̼�ngx_http_variable_s����½�
*/ //�ο�:http://ialloc.org/posts/2013/10/20/ngx-notes-http-variables/

/*
ngx_http_core_main_conf_t->variables�����Ա�Ľṹʽngx_http_variable_s�� ngx_http_request_s->variables�����Ա�ṹ��
ngx_variable_value_t�������ṹ�Ĺ�ϵ�����У�һ����ν������һ����ν����ֵ

r->variables���������cmcf->variables��һһ��Ӧ�ģ��γ�var_ name��var_value�ԣ����������������ͬһ���±�λ��Ԫ�ظպþ���
�໥��Ӧ�ı������ͱ���ֵ����������ʹ��ĳ������ʱ�ܻ���ͨ������ngx_http_get_variable_index������ڱ������������index�±꣬Ҳ���Ǳ�
�������index�ֶ�ֵ��Ȼ���������index�±����ȥ����ֵ������ȡ��Ӧ��ֵ
*/ //�ο�<��������nginx-����>
struct ngx_http_variable_s { //ngx_http_add_variable  ngx_http_get_variable_index�д����ռ䲢��ֵ 
//variables_hash���variables�����еĳ�Ա��ngx_http_variables_init_vars�д�variables_keys�л�ȡ��Ӧֵ����Щֵ����Դ����ʱ��variables_keys

    ngx_str_t                     name;   /* must be first to build the hash */

    /*
    get_handler���ֵ���lazy_handle�Ĳ��ԣ�ֻ��ʹ�õ��������Ż�������ֵ��
    set_handler���ֵ���active_handle�Ĳ��ԣ�ÿִ��һ�����󣬶���������ֵ��

        set_handlerr()�ص�Ŀǰֻ��ʹ��set����ָ���ű�����ʱ�Ż��õ���������ֱ��ʹ��cmcf->variables_keys���Ӧ�����ĸ��ֶΣ���
    ��һ�������ļ�������ϣ�set_handlerr()�ص�Ҳ���ò�����

    set_handler0������ص�Ŀǰֻ��ʹ����setָ�����ɽű������һ�����裬�ṩ���û��������ļ�������޸����ñ�����ֵ������set_handler0�ӿڵı����ǳ��٣�
�����$args��$limitrate�����������һ�������NGX_HTTP_VAR_CHANGEABLE��ǣ���������ӿں������壬��Ϊ��Ȼ�����޸ģ��α��ṩ�޸Ľӿڣ�Ҳ�����NGX_HTTP
VAR_NOCACHEABLE��ǣ���Ϊ��Ȼ�ᱻ�޸ģ���ȻҲ�ǲ��ɻ����
     */ //
    ngx_http_set_variable_pt      set_handler; //
    //��ngx_http_variables_init_vars�л��"http_"  "send_http_"������Ĭ�ϵ�get_handler��data
    /*
    gethandler()�ص��ֶΣ�����ֶ���Ҫʵ�ֻ�ȡ����ֵ�Ĺ��ܡ�ǰ�潲��Nginx���ñ�����ֵ������Ĭ����Դ�ģ�����Ǽ򵥵�ֱ�Ӵ����ĳ����
�������潲���ڲ�����$args���������ô��Ҫ���get_handler()�ص������������ԣ�ͨ��data�ֶ�ָ��ĵ�ַ��ȡ����������Ƚϸ��ӣ���Ȼ֪��
���ֵ������Ķ�������ȴ��Ҫ�Ƚϸ��ӵ��߼���ȡ�����潲���ڲ�����$remote_port���������ʱ�ͱ��뿿�ص�����get_handler()��ִ���ⲿ���߼���
��֮�����ܼ򵥻��ӣ��ص�����get_handler0������ȥ�ں��ʵĵط�ͨ�����ʵķ�ʽ����ȡ�����ڲ�������ֵ����Ҳ��Ϊʲô���ǲ�û�и�Nginx�ڲ�������ֵ��
ȴ���ܶ���ֵ����Ϊ������ص������Ĵ��ڡ�������������ʾ��������data�ֶ���get_handler()�ص��ֶ������
     */ //gethandler()�ص��ֶΣ�����ֶ���Ҫʵ�ֻ�ȡ����ֵ�Ĺ���   ����ngx_http_core_variables
     //ngx_http_rewrite_set������Ϊngx_http_rewrite_var  args��get_handler��ngx_http_variable_request ngx_http_variables_init_vars������"http_"�ȵ�get_handler
    ngx_http_get_variable_pt      get_handler; 

    /*
    �ٸ����ӣ�Nginx�ڲ�����$args��ʾ���ǿͻ���GET����ʱuri��Ĳ������ṹ��ngxhttp_request_t��һ��ngx_str_t�����ֶ�Ϊ
args���ڴ�ŵľ���GET��������������ڲ�����$args�����data�ֶξ���ָ�����r���args�ֶΣ���ʾ��������֮�������ֱ�ӵ��������ô��ӵ�
����أ���Nginx�ڲ�����$remoteport�����������ʾ�ͻ��˶˿ںţ����ֵ�ڽṹ��ngxhttprequest_t��û��ֱ�ӵ��ֶζ�Ӧ������϶�ͬ��Ҳ������
ngxhttp_request_t����r���ôȥ��ȡ�Ϳ�gethandler������ʵ�֣���ʱdata�����ֶ�ûʲô���ã�ֵΪ0��
     */
    uintptr_t                     data;//����:��args��data��ֵ��offsetof(ngx_http_request_t, args)���������args��ngx_http_request_t�е�ƫ�ơ�
    
    ngx_uint_t                    flags; //��ֵΪNGX_HTTP_VAR_CHANGEABLE��
    //��variables�����е�λ�ã���ngx_http_get_variable_index��ʱ���Ѿ�ȷ������variables�����е�λ��
    ngx_uint_t                    index;  //ͨ������ngx_http_get_variable_index���
};


ngx_http_variable_t *ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name,
    ngx_uint_t flags);
ngx_int_t ngx_http_get_variable_index(ngx_conf_t *cf, ngx_str_t *name);
ngx_http_variable_value_t *ngx_http_get_indexed_variable(ngx_http_request_t *r,
    ngx_uint_t index);
ngx_http_variable_value_t *ngx_http_get_flushed_variable(ngx_http_request_t *r,
    ngx_uint_t index);

ngx_http_variable_value_t *ngx_http_get_variable(ngx_http_request_t *r,
    ngx_str_t *name, ngx_uint_t key);

ngx_int_t ngx_http_variable_unknown_header(ngx_http_variable_value_t *v,
    ngx_str_t *var, ngx_list_part_t *part, size_t prefix);


#if (NGX_PCRE)

//ngx_http_regex_compile�з���ռ�,�������   ngx_http_regex_exec��ȡ��Ӧ��ֵ
typedef struct {
    ngx_uint_t                    capture;
    ngx_int_t                     index;
} ngx_http_regex_variable_t; //ngx_http_regex_t�а����ó�Ա�ṹ

//�ýṹ������ngx_http_script_regex_code_t��
typedef struct {//ngx_http_regex_compile������ʹ�ã���ֵ��ngx_http_regex_compile
    ngx_regex_t                  *regex; //ngx_regex_compile_t->regexһ��
    ngx_uint_t                    ncaptures;//ngx_regex_compile_t->capturesһ�� ������ģʽ�ͷ�������ģʽ���ܸ���
    ngx_http_regex_variable_t    *variables; //������ģʽ��Ӧ�ı�������ֵ��ngx_http_regex_compile
    ngx_uint_t                    nvariables; //�ж��ٸ�������ģʽ����ֵ��ngx_http_regex_compile�� ngx_http_regex_exec��ʹ��
    ngx_str_t                     name;//ngx_regex_compile_t->patternһ��
} ngx_http_regex_t;


typedef struct {
    ngx_http_regex_t             *regex;
    void                         *value;
} ngx_http_map_regex_t;


ngx_http_regex_t *ngx_http_regex_compile(ngx_conf_t *cf,
    ngx_regex_compile_t *rc);
ngx_int_t ngx_http_regex_exec(ngx_http_request_t *r, ngx_http_regex_t *re,
    ngx_str_t *s);

#endif


typedef struct {
    ngx_hash_combined_t           hash;
#if (NGX_PCRE)
    ngx_http_map_regex_t         *regex;
    ngx_uint_t                    nregex;
#endif
} ngx_http_map_t;


void *ngx_http_map_find(ngx_http_request_t *r, ngx_http_map_t *map,
    ngx_str_t *match);


ngx_int_t ngx_http_variables_add_core_vars(ngx_conf_t *cf);
ngx_int_t ngx_http_variables_init_vars(ngx_conf_t *cf);


extern ngx_http_variable_value_t  ngx_http_variable_null_value;
extern ngx_http_variable_value_t  ngx_http_variable_true_value;


#endif /* _NGX_HTTP_VARIABLES_H_INCLUDED_ */
