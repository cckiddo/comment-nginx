
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_str_t                name;
    ngx_array_t             *lengths;
    ngx_array_t             *values;
} ngx_http_index_t;


typedef struct { //�ռ䴴���͸�ֵ��ngx_http_index_set_index
    //��index  11.html 22.xx�е�11.html��22.xx�ַ������浽indices����
    ngx_array_t             *indices;    /* array of ngx_http_index_t */    //indices��Ĭ����һ��NGX_HTTP_DEFAULT_INDEX
    size_t                   max_index_len; //��ֵΪindices���������ַ������ĳ���
} ngx_http_index_loc_conf_t;


#define NGX_HTTP_DEFAULT_INDEX   "index.html"


static ngx_int_t ngx_http_index_test_dir(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, u_char *path, u_char *last);
static ngx_int_t ngx_http_index_error(ngx_http_request_t *r,
    ngx_http_core_loc_conf_t *clcf, u_char *file, ngx_err_t err);

static ngx_int_t ngx_http_index_init(ngx_conf_t *cf);
static void *ngx_http_index_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_index_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static char *ngx_http_index_set_index(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

/*
������ҳ
�﷨��index file ...;
Ĭ�ϣ�index index.html;
���ÿ飺http��server��location
��ʱ������վ��ʱ��URI��/����ʱһ���Ƿ�����վ����ҳ��������root��alias����ͬ��������ngx_http_index_moduleģ���ṩ��index����ʵ�֡�index����Ը�
����ļ�������Nginx���ᰴ��˳����������Щ�ļ������磺
location / {
    root   path;
    index /index.html /html/index.php /index.php;
}
���յ������Nginx���Ȼ᳢�Է���path/index.php�ļ���������Է��ʣ���ֱ�ӷ����ļ����ݽ������󣬷�������ͼ����path/html/index.php�ļ������ݣ��������ơ�
*/ //ע��:���URI��б�߽�β���ļ�����׷�ӵ�URI���棬�ο�ngx_http_index_handler
static ngx_command_t  ngx_http_index_commands[] = {

    { ngx_string("index"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_index_set_index,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_index_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_index_init,                   /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_index_create_loc_conf,        /* create location configuration */
    ngx_http_index_merge_loc_conf          /* merge location configuration */
};

/*
location / {			
    index index11.html	#���뱣֤��uri����Ŀ¼���ڲ��Ҹ�Ŀ¼����û��index11.html��autoindex��Ӧ��ngx_http_autoindex_handler�Ż���Ч		
    autoindex on;		
}
ֻ����index11.html�ļ������ڵ�ʱ��Ż�ִ��autoindex�����û������index��Ĭ�ϴ�index.html�����뱣֤index.html��uriĿ¼���ڣ���������ڣ���һ�������ڵ�Ŀ¼Ҳ����ִ��autoindex

Nginx һ����� content �׶ΰ������������ľ�̬��Դ����ģ��:ngx_index ģ�飬 ngx_autoindex ģ�顢ngx_static ģ��
ngx_index �� ngx_autoindex ģ�鶼ֻ����������Щ URI �� / ��β�������������� GET /cats/�������ڲ��� / ��β���������ֱ�Ӻ��ԣ�
ͬʱ�Ѵ���Ȩ�ƽ��� content �׶ε���һ��ģ�顣�� ngx_static ģ����պ��෴��ֱ�Ӻ�����Щ URI �� / ��β������ 
*/
ngx_module_t  ngx_http_index_module = {
    NGX_MODULE_V1,
    &ngx_http_index_module_ctx,            /* module context */
    ngx_http_index_commands,               /* module directives */
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
 * Try to open/test the first index file before the test of directory
 * existence because valid requests should prevail over invalid ones.
 * If open()/stat() of a file will fail then stat() of a directory
 * should be faster because kernel may have already cached some data.
 * Besides, Win32 may return ERROR_PATH_NOT_FOUND (NGX_ENOTDIR) at once.
 * Unix has ENOTDIR error; however, it's less helpful than Win32's one:
 * it only indicates that path points to a regular file, not a directory.
 */
/*  ����index index.html index_large.html  gmime-gmime-cipher-context.html;
2025/02/14 08:24:04[   ngx_http_process_request_headers,  1412]  [debug] 2955#2955: *2 http header done
2025/02/14 08:24:04[                ngx_event_del_timer,    39]  [debug] 2955#2955: *2 < ngx_http_process_request,  2013>  event timer del: 3: 30909486
2025/02/14 08:24:04[        ngx_http_core_rewrite_phase,  1810]  [debug] 2955#2955: *2 rewrite phase: 0 (NGX_HTTP_SERVER_REWRITE_PHASE)
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1868]  [debug] 2955#2955: *2 find config phase: 1 (NGX_HTTP_FIND_CONFIG_PHASE), uri:/
2025/02/14 08:24:04[ ngx_http_core_find_static_location,  2753]  [debug] 2955#2955: *2 static_locations test location: "/", client uri:/
2025/02/14 08:24:04[        ngx_http_core_find_location,  2693]  [debug] 2955#2955: *2 ngx pcre test location: ~ "/1mytest"
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1888]  [debug] 2955#2955: *2 using configuration "/"
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1895]  [debug] 2955#2955: *2 http cl:-1 max:1048576
2025/02/14 08:24:04[        ngx_http_core_rewrite_phase,  1810]  [debug] 2955#2955: *2 rewrite phase: 2 (NGX_HTTP_REWRITE_PHASE)
2025/02/14 08:24:04[   ngx_http_core_post_rewrite_phase,  1963]  [debug] 2955#2955: *2 post rewrite phase: 3 (NGX_HTTP_POST_REWRITE_PHASE)
2025/02/14 08:24:04[        ngx_http_core_generic_phase,  1746]  [debug] 2955#2955: *2 generic phase: 4 (NGX_HTTP_PREACCESS_PHASE)
2025/02/14 08:24:04[        ngx_http_core_generic_phase,  1746]  [debug] 2955#2955: *2 generic phase: 5 (NGX_HTTP_PREACCESS_PHASE)
2025/02/14 08:24:04[         ngx_http_core_access_phase,  2061]  [debug] 2955#2955: *2 access phase: 6 (NGX_HTTP_ACCESS_PHASE)
2025/02/14 08:24:04[         ngx_http_core_access_phase,  2061]  [debug] 2955#2955: *2 access phase: 7 (NGX_HTTP_ACCESS_PHASE)
2025/02/14 08:24:04[    ngx_http_core_post_access_phase,  2163]  [debug] 2955#2955: *2 post access phase: 8 (NGX_HTTP_POST_ACCESS_PHASE)
2025/02/14 08:24:04[        ngx_http_core_content_phase,  2491]  [debug] 2955#2955: *2 content phase: 9 (NGX_HTTP_CONTENT_PHASE)
2025/02/14 08:24:04[             ngx_http_index_handler,   144]  [debug] 2955#2955: *2 yang test ... index-count:3
2025/02/14 08:24:04[             ngx_http_index_handler,   216]  [debug] 2955#2955: *2 open index "/usr/local/nginx/html/index.html"
2025/02/14 08:24:04[         ngx_http_internal_redirect,  3853]  [debug] 2955#2955: *2 internal redirect: "/index.html?"
2025/02/14 08:24:04[        ngx_http_core_rewrite_phase,  1810]  [debug] 2955#2955: *2 rewrite phase: 0 (NGX_HTTP_SERVER_REWRITE_PHASE)
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1868]  [debug] 2955#2955: *2 find config phase: 1 (NGX_HTTP_FIND_CONFIG_PHASE), uri:/index.html
2025/02/14 08:24:04[ ngx_http_core_find_static_location,  2753]  [debug] 2955#2955: *2 static_locations test location: "/", client uri:/index.html
2025/02/14 08:24:04[ ngx_http_core_find_static_location,  2753]  [debug] 2955#2955: *2 static_locations test location: "proxy1", client uri:/index.html
2025/02/14 08:24:04[ ngx_http_core_find_static_location,  2753]  [debug] 2955#2955: *2 static_locations test location: "mytest", client uri:/index.html
2025/02/14 08:24:04[        ngx_http_core_find_location,  2693]  [debug] 2955#2955: *2 ngx pcre test location: ~ "/1mytest"
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1888]  [debug] 2955#2955: *2 using configuration "/"
2025/02/14 08:24:04[    ngx_http_core_find_config_phase,  1895]  [debug] 2955#2955: *2 http cl:-1 max:1048576
2025/02/14 08:24:04[        ngx_http_core_rewrite_phase,  1810]  [debug] 2955#2955: *2 rewrite phase: 2 (NGX_HTTP_REWRITE_PHASE)
2025/02/14 08:24:04[   ngx_http_core_post_rewrite_phase,  1963]  [debug] 2955#2955: *2 post rewrite phase: 3 (NGX_HTTP_POST_REWRITE_PHASE)
2025/02/14 08:24:04[        ngx_http_core_generic_phase,  1746]  [debug] 2955#2955: *2 generic phase: 4 (NGX_HTTP_PREACCESS_PHASE)
2025/02/14 08:24:04[        ngx_http_core_generic_phase,  1746]  [debug] 2955#2955: *2 generic phase: 5 (NGX_HTTP_PREACCESS_PHASE)
2025/02/14 08:24:04[         ngx_http_core_access_phase,  2061]  [debug] 2955#2955: *2 access phase: 6 (NGX_HTTP_ACCESS_PHASE)
2025/02/14 08:24:04[         ngx_http_core_access_phase,  2061]  [debug] 2955#2955: *2 access phase: 7 (NGX_HTTP_ACCESS_PHASE)
2025/02/14 08:24:04[    ngx_http_core_post_access_phase,  2163]  [debug] 2955#2955: *2 post access phase: 8 (NGX_HTTP_POST_ACCESS_PHASE)
2025/02/14 08:24:04[        ngx_http_core_content_phase,  2491]  [debug] 2955#2955: *2 content phase: 9 (NGX_HTTP_CONTENT_PHASE)
2025/02/14 08:24:04[        ngx_http_core_content_phase,  2491]  [debug] 2955#2955: *2 content phase: 10 (NGX_HTTP_CONTENT_PHASE)
2025/02/14 08:24:04[        ngx_http_core_content_phase,  2491]  [debug] 2955#2955: *2 content phase: 11 (NGX_HTTP_CONTENT_PHASE)
2025/02/14 08:24:04[            ngx_http_static_handler,    85]  [debug] 2955#2955: *2 http filename: "/usr/local/nginx/html/index.html"
2025/02/14 08:24:04[            ngx_http_static_handler,   145]  [debug] 2955#2955: *2 http static fd: 11
2025/02/14 08:24:04[      ngx_http_discard_request_body,   734]  [debug] 2955#2955: *2 http set discard body
*/
static ngx_int_t //��Ҫ�����Ǽ��uri�е��ļ��Ƿ���ڣ�������ֱ�ӹر����ӣ����������ڲ��ض����ض�����������ļ�·�������ĩβû��/���ߵ��ú���ֱ���˳���Ȼ����static-module�л�ȡ�ļ�����
ngx_http_index_handler(ngx_http_request_t *r)
{//ע��:ngx_http_static_handler���uri������/��β���أ�ngx_http_index_handler����/��β����
//ѭ������index index.html index_large.html  gmime-gmime-cipher-context.html;���õ��ļ��������򷵻أ��ҵ�һ�����ڱ���������ļ�
//ngx_http_static_handler ngx_http_index_handlerÿ�ζ�Ҫ��ȡ������Ϣstat��Ϣ�����ÿ�λ�ȡ�ܿ�������һ��statִ�е�ʱ���ȡ����Ϣ�����ǻ������

    u_char                       *p, *name;
    size_t                        len, root, reserve, allocated;
    ngx_int_t                     rc;
    ngx_str_t                     path, uri;
    ngx_uint_t                    i, dir_tested;
    ngx_http_index_t             *index;
    ngx_open_file_info_t          of;
    ngx_http_script_code_pt       code;
    ngx_http_script_engine_t      e;
    ngx_http_core_loc_conf_t     *clcf;
    ngx_http_index_loc_conf_t    *ilcf;
    ngx_http_script_len_code_pt   lcode;

    /*
      һ��ƥ�䵽location / {
    
      }��ʱ�򣬲Ż�ִ�������index��Ȼ������ڲ���ת
     */

    /*
    ������������:http://10.135.10.167/ABC/,��Ҳ������Ҫ��uri���Ϊ/ABC/index.html,��ӡ����
     2015/10/16 12:08:03[                ngx_event_del_timer,    39]  [debug] 12610#12610: *2 < ngx_http_process_request,  2013>  event timer del: 3: 1859492499
     2015/10/16 12:08:03[        ngx_http_core_rewrite_phase,  1810]  [debug] 12610#12610: *2 rewrite phase: 0 (NGX_HTTP_SERVER_REWRITE_PHASE)
     2015/10/16 12:08:03[    ngx_http_core_find_config_phase,  1868]  [debug] 12610#12610: *2 find config phase: 1 (NGX_HTTP_FIND_CONFIG_PHASE), uri:/ABC/
     2015/10/16 12:08:03[ ngx_http_core_find_static_location,  2753]  [debug] 12610#12610: *2 static_locations test location: "/", client uri:/ABC/
     2015/10/16 12:08:03[ ngx_http_core_find_static_location,  2753]  [debug] 12610#12610: *2 static_locations test location: "proxy1", client uri:/ABC/
     2015/10/16 12:08:03[ ngx_http_core_find_static_location,  2753]  [debug] 12610#12610: *2 static_locations test location: "mytest", client uri:/ABC/
     2015/10/16 12:08:03[        ngx_http_core_find_location,  2693]  [debug] 12610#12610: *2 ngx pcre test location: ~ "\.php$"
     2015/10/16 12:08:03[        ngx_http_core_find_location,  2693]  [debug] 12610#12610: *2 ngx pcre test location: ~ "/1mytest"
     2015/10/16 12:08:03[    ngx_http_core_find_config_phase,  1888]  [debug] 12610#12610: *2 using configuration "/"
     2015/10/16 12:08:03[    ngx_http_core_find_config_phase,  1895]  [debug] 12610#12610: *2 http cl:-1 max:1048576
     2015/10/16 12:08:03[        ngx_http_core_rewrite_phase,  1810]  [debug] 12610#12610: *2 rewrite phase: 2 (NGX_HTTP_REWRITE_PHASE)
     2015/10/16 12:08:03[   ngx_http_core_post_rewrite_phase,  1963]  [debug] 12610#12610: *2 post rewrite phase: 3 (NGX_HTTP_POST_REWRITE_PHASE)
     2015/10/16 12:08:03[        ngx_http_core_generic_phase,  1746]  [debug] 12610#12610: *2 generic phase: 4 (NGX_HTTP_PREACCESS_PHASE)
     2015/10/16 12:08:03[        ngx_http_core_generic_phase,  1746]  [debug] 12610#12610: *2 generic phase: 5 (NGX_HTTP_PREACCESS_PHASE)
     2015/10/16 12:08:03[         ngx_http_core_access_phase,  2061]  [debug] 12610#12610: *2 access phase: 6 (NGX_HTTP_ACCESS_PHASE)
     2015/10/16 12:08:03[         ngx_http_core_access_phase,  2061]  [debug] 12610#12610: *2 access phase: 7 (NGX_HTTP_ACCESS_PHASE)
     2015/10/16 12:08:03[    ngx_http_core_post_access_phase,  2163]  [debug] 12610#12610: *2 post access phase: 8 (NGX_HTTP_POST_ACCESS_PHASE)
     2015/10/16 12:08:03[        ngx_http_core_content_phase,  2491]  [debug] 12610#12610: *2 content phase: 9 (NGX_HTTP_CONTENT_PHASE)
     2015/10/16 12:08:03[             ngx_http_index_handler,   191]  [debug] 12610#12610: *2 yang test ... index-count:3
     2015/10/16 12:08:03[             ngx_http_index_handler,   263]  [debug] 12610#12610: *2 open index "/var/yyz/www/ABC/index.html"
     2015/10/16 12:08:03[             ngx_http_index_handler,   283]  [debug] 12610#12610: *2 stat() "/var/yyz/www/ABC/index.html" failed (2: No such file or directory)
     2015/10/16 12:08:03[            ngx_http_index_test_dir,   364]  [debug] 12610#12610: *2 http index check dir: "/var/yyz/www/ABC"

     */ //Ĭ��http://10.2.13.167��ʱ�����������ת��Ϊhttp://10.2.13.167/���͵�nginx������
    if (r->uri.data[r->uri.len - 1] != '/') { //ĩβ����/��ֱ����ת����һ�׶�
        return NGX_DECLINED;
    }

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD|NGX_HTTP_POST))) {
        return NGX_DECLINED;
    }

    ilcf = ngx_http_get_module_loc_conf(r, ngx_http_index_module);
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    allocated = 0;
    root = 0;
    dir_tested = 0;
    name = NULL;
    /* suppress MSVC warning */
    path.data = NULL;

    index = ilcf->indices->elts;
    //indices��Ĭ����һ��NGX_HTTP_DEFAULT_INDEX
    for (i = 0; i < ilcf->indices->nelts; i++) {//ѭ������index���õ��ļ�������и��ļ���������ڲ��ض��򣬴�����NGX_HTTP_SERVER_REWRITE_PHASE

        if (index[i].lengths == NULL) {

            if (index[i].name.data[0] == '/') {
                return ngx_http_internal_redirect(r, &index[i].name, &r->args);
            }

            reserve = ilcf->max_index_len;
            len = index[i].name.len;

        } else {
            ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

            e.ip = index[i].lengths->elts;
            e.request = r;
            e.flushed = 1;

            /* 1 is for terminating '\0' as in static names */
            len = 1;

            while (*(uintptr_t *) e.ip) {
                lcode = *(ngx_http_script_len_code_pt *) e.ip;
                len += lcode(&e);
            }

            /* 16 bytes are preallocation */

            reserve = len + 16;
        }

        if (reserve > allocated) {

            name = ngx_http_map_uri_to_path(r, &path, &root, reserve);
            if (name == NULL) {
                return NGX_ERROR;
            }

            allocated = path.data + path.len - name;
        }

        if (index[i].values == NULL) {

            /* index[i].name.len includes the terminating '\0' */

            ngx_memcpy(name, index[i].name.data, index[i].name.len);

            path.len = (name + index[i].name.len - 1) - path.data;

        } else {
            e.ip = index[i].values->elts;
            e.pos = name;

            while (*(uintptr_t *) e.ip) {
                code = *(ngx_http_script_code_pt *) e.ip;
                code((ngx_http_script_engine_t *) &e);
            }

            if (*name == '/') {
                uri.len = len - 1;
                uri.data = name;
                return ngx_http_internal_redirect(r, &uri, &r->args);
            }

            path.len = e.pos - path.data;

            *e.pos = '\0';
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "open index \"%V\"", &path);

        ngx_memzero(&of, sizeof(ngx_open_file_info_t));

        of.read_ahead = clcf->read_ahead;
        of.directio = clcf->directio;
        of.valid = clcf->open_file_cache_valid;
        of.min_uses = clcf->open_file_cache_min_uses;
        of.test_only = 1;
        of.errors = clcf->open_file_cache_errors;
        of.events = clcf->open_file_cache_events;

        if (ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool)
            != NGX_OK)
        {
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, of.err,
                           "%s \"%s\" failed", of.failed, path.data);

            if (of.err == 0) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

#if (NGX_HAVE_OPENAT)
            if (of.err == NGX_EMLINK
                || of.err == NGX_ELOOP)
            {
                return NGX_HTTP_FORBIDDEN;
            }
#endif

            if (of.err == NGX_ENOTDIR
                || of.err == NGX_ENAMETOOLONG
                || of.err == NGX_EACCES)
            {
                return ngx_http_index_error(r, clcf, path.data, of.err);
            }

            if (!dir_tested) {
                rc = ngx_http_index_test_dir(r, clcf, path.data, name - 1);

                if (rc != NGX_OK) {
                    return rc;
                }

                dir_tested = 1;
            }

            if (of.err == NGX_ENOENT) {
                continue; //stat��ȡ�Ĳ���file_nameָ�����ļ�������
            }

            ngx_log_error(NGX_LOG_CRIT, r->connection->log, of.err,
                          "%s \"%s\" failed", of.failed, path.data);

            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        uri.len = r->uri.len + len - 1;

        if (!clcf->alias) {
            uri.data = path.data + root;

        } else {
            uri.data = ngx_pnalloc(r->pool, uri.len);
            if (uri.data == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            p = ngx_copy(uri.data, r->uri.data, r->uri.len);
            ngx_memcpy(p, name, len - 1);
        }

        return ngx_http_internal_redirect(r, &uri, &r->args); //�ڲ��ض���
    }

    return NGX_DECLINED;
}

static ngx_int_t
ngx_http_index_test_dir(ngx_http_request_t *r, ngx_http_core_loc_conf_t *clcf,
    u_char *path, u_char *last)
{
    u_char                c;
    ngx_str_t             dir;
    ngx_open_file_info_t  of;

    c = *last;
    if (c != '/' || path == last) {
        /* "alias" without trailing slash */
        c = *(++last);
    }
    *last = '\0';

    dir.len = last - path;
    dir.data = path;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http index check dir: \"%V\"", &dir);

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.test_dir = 1;
    of.test_only = 1;
    of.valid = clcf->open_file_cache_valid;
    of.errors = clcf->open_file_cache_errors;

    if (ngx_http_set_disable_symlinks(r, clcf, &dir, &of) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ngx_open_cached_file(clcf->open_file_cache, &dir, &of, r->pool)
        != NGX_OK)
    {
        if (of.err) {

#if (NGX_HAVE_OPENAT)
            if (of.err == NGX_EMLINK
                || of.err == NGX_ELOOP)
            {
                return NGX_HTTP_FORBIDDEN;
            }
#endif

            if (of.err == NGX_ENOENT) {
                *last = c;
                return ngx_http_index_error(r, clcf, dir.data, NGX_ENOENT);
            }

            if (of.err == NGX_EACCES) {

                *last = c;

                /*
                 * ngx_http_index_test_dir() is called after the first index
                 * file testing has returned an error distinct from NGX_EACCES.
                 * This means that directory searching is allowed.
                 */

                return NGX_OK;
            }

            ngx_log_error(NGX_LOG_CRIT, r->connection->log, of.err,
                          "%s \"%s\" failed", of.failed, dir.data);
        }

        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    *last = c;

    if (of.is_dir) {
        return NGX_OK;
    }

    ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                  "\"%s\" is not a directory", dir.data);

    return NGX_HTTP_INTERNAL_SERVER_ERROR;
}


static ngx_int_t
ngx_http_index_error(ngx_http_request_t *r, ngx_http_core_loc_conf_t  *clcf,
    u_char *file, ngx_err_t err)
{
    if (err == NGX_EACCES) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, err,
                      "\"%s\" is forbidden", file);

        return NGX_HTTP_FORBIDDEN;
    }

    if (clcf->log_not_found) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, err,
                      "\"%s\" is not found", file);
    }

    return NGX_HTTP_NOT_FOUND;
}


static void *
ngx_http_index_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_index_loc_conf_t  *conf;

    conf = ngx_palloc(cf->pool, sizeof(ngx_http_index_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->indices = NULL;
    conf->max_index_len = 0;

    return conf;
}


static char *
ngx_http_index_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_index_loc_conf_t  *prev = parent;
    ngx_http_index_loc_conf_t  *conf = child;

    ngx_http_index_t  *index;

    if (conf->indices == NULL) {
        conf->indices = prev->indices;
        conf->max_index_len = prev->max_index_len;
    }

    if (conf->indices == NULL) {
        conf->indices = ngx_array_create(cf->pool, 1, sizeof(ngx_http_index_t));
        if (conf->indices == NULL) {
            return NGX_CONF_ERROR;
        }

        index = ngx_array_push(conf->indices);
        if (index == NULL) {
            return NGX_CONF_ERROR;
        }

        index->name.len = sizeof(NGX_HTTP_DEFAULT_INDEX);
        index->name.data = (u_char *) NGX_HTTP_DEFAULT_INDEX;
        index->lengths = NULL;
        index->values = NULL;

        conf->max_index_len = sizeof(NGX_HTTP_DEFAULT_INDEX);

        return NGX_CONF_OK;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_index_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_index_handler;

    return NGX_OK;
}


/* TODO: warn about duplicate indices */
//index /index.html /html/index.php /index.php;
static char * //��index���õ��ַ�����ӵ�ilcf->indices������
ngx_http_index_set_index(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_index_loc_conf_t *ilcf = conf;

    ngx_str_t                  *value;
    ngx_uint_t                  i, n;
    ngx_http_index_t           *index;
    ngx_http_script_compile_t   sc;

    if (ilcf->indices == NULL) {
        ilcf->indices = ngx_array_create(cf->pool, 2, sizeof(ngx_http_index_t));
        if (ilcf->indices == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {

        if (value[i].data[0] == '/' && i != cf->args->nelts - 1) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "only the last index in \"index\" directive "
                               "should be absolute");
        }

        if (value[i].len == 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "index \"%V\" in \"index\" directive is invalid",
                               &value[1]);
            return NGX_CONF_ERROR;
        }

        index = ngx_array_push(ilcf->indices);
        if (index == NULL) {
            return NGX_CONF_ERROR;
        }

        index->name.len = value[i].len;
        index->name.data = value[i].data;
        index->lengths = NULL;
        index->values = NULL;

        n = ngx_http_script_variables_count(&value[i]);

        if (n == 0) {
            if (ilcf->max_index_len < index->name.len) {
                ilcf->max_index_len = index->name.len;
            }

            if (index->name.data[0] == '/') {
                continue;
            }

            /* include the terminating '\0' to the length to use ngx_memcpy() */
            index->name.len++;

            continue;
        }

        ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

        sc.cf = cf;
        sc.source = &value[i];
        sc.lengths = &index->lengths;
        sc.values = &index->values;
        sc.variables = n;
        sc.complete_lengths = 1;
        sc.complete_values = 1;

        if (ngx_http_script_compile(&sc) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}
