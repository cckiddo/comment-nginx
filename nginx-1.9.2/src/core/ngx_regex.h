
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_REGEX_H_INCLUDED_
#define _NGX_REGEX_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

#include <pcre.h>


#define NGX_REGEX_NO_MATCHED  PCRE_ERROR_NOMATCH   /* -1 */

#define NGX_REGEX_CASELESS    PCRE_CASELESS


typedef struct {
    pcre        *code;
    pcre_extra  *extra;
} ngx_regex_t;

//ngx_http_rewrite�У�rewrite aaa bbb break;�����У�aaa����ʹ��ngx_regex_compile_t��bbb����ʹ��ngx_http_script_compile_t
//��ֵ��ngx_regex_compile
typedef struct { //��س�Ա��ngx_http_regex_t�е�һ��
    ngx_str_t     pattern; //pcre_compile��ȡ����
    ngx_pool_t   *pool;
    ngx_int_t     options;

    ngx_regex_t  *regex;
    /*
    ����rewrite   ^(?<name1>/download/.*)/media/(?<name2>.*)/(abc)\..*$     $name1/mp3/$name2.mp3  break;
    prce����^(?<name1>/download/.*)/media/(?<name2>.*)/(abc)\..*$��ʱ��Ľ����capturesΪ3(1����������ģʽ������2��������ģʽ����)��named_capturesΪ2
     */
    int           captures; //�õ�����������ģʽ�ĸ���,����������ģʽ(?<name>exp) �ͷ�������ģʽ(exp)����ֵ��ngx_regex_compile
    int           named_captures; //�õ�����������ģʽ�ĸ���,��������������ģʽ�ĸ���;��ֵ��ngx_regex_compile

    /* ��������������ȡ����ģʽ�ַ����õ� */    
    int           name_size;
    u_char       *names;

    ngx_str_t     err;
} ngx_regex_compile_t;


typedef struct {
    ngx_regex_t  *regex; //������ʽ����ngx_regex_compileת�����regex��Ϣ
    u_char       *name; //������ʽԭ�ַ���
} ngx_regex_elt_t;


void ngx_regex_init(void);
ngx_int_t ngx_regex_compile(ngx_regex_compile_t *rc);

/*
����������ʽ��ָ�����ַ����н��в��Һ�ƥ��,�����ƥ��Ľ��.
ʹ�ñ���õ�ģʽ����ƥ�䣬������Perl���Ƶ��㷨������ƥ�䴮��ƫ��λ�á���

����������ʽ���re.name= ^(/download/.*)/media/(.*)/tt/(.*)$��  s=/download/aa/media/bdb/tt/ad,�����ǻ�ƥ�䣬ͬʱƥ��ı�������3�����򷵻�ֵΪ3+1=4,�����ƥ���򷵻�-1
*/
#define ngx_regex_exec(re, s, captures, size)                                \
    pcre_exec(re->code, re->extra, (const char *) (s)->data, (s)->len, 0, 0, \
              captures, size)
#define ngx_regex_exec_n      "pcre_exec()"

ngx_int_t ngx_regex_exec_array(ngx_array_t *a, ngx_str_t *s, ngx_log_t *log);


#endif /* _NGX_REGEX_H_INCLUDED_ */
