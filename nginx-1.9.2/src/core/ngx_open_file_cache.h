
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef _NGX_OPEN_FILE_CACHE_H_INCLUDED_
#define _NGX_OPEN_FILE_CACHE_H_INCLUDED_


#define NGX_OPEN_FILE_DIRECTIO_OFF  NGX_MAX_OFF_T_VALUE

//����ͨ��ngx_open_and_stat_file��ȡ�ļ������������Ϣ
typedef struct {
    ngx_fd_t                 fd;
    ngx_file_uniq_t          uniq;//�ļ�inode�ڵ�� ͬһ���豸�е�ÿ���ļ������ֵ���ǲ�ͬ��
    time_t                   mtime; //�ļ�����޸ĵ�ʱ��
    off_t                    size;
    off_t                    fs_size;
    //ȡֵ�Ǵ�ngx_http_core_loc_conf_s->directio  //�ڻ�ȡ�����ļ����ݵ�ʱ��ֻ���ļ���С�������directio��ʱ��Ż���Чngx_directio_on
    ////Ĭ��NGX_OPEN_FILE_DIRECTIO_OFF�Ǹ��������ֵ���൱�ڲ�ʹ��
    off_t                    directio; //��Ч��ngx_open_and_stat_file  if (of->directio <= ngx_file_size(&fi)) { ngx_directio_on }
    size_t                   read_ahead;  /* read_ahead���ã�Ĭ��0 */

    /*
    ��ngx_file_info_wrapper�л�ȡ�ļ�stat������Ϣ��ʱ������ļ������ڻ���openʧ�ܣ�����statʧ�ܣ�����Ѵ�������������ֶ�
    of->err = ngx_errno;
    of->failed = ngx_fd_info_n;
     */
    ngx_err_t                err;
    char                    *failed;
    //open_file_cache_valid 60S  
    //��ʾ60s�����ĵ�һ������Ҫ���ļ�stat��Ϣ��һ�μ�飬����Ƿ��ͱ仯��������ͱ仯����»�ȡ�ļ�stat��Ϣ���ߴ��´����ý׶Σ�
    //��Ч��ngx_open_cached_file�е�(&& now - file->created < of->valid )
    time_t                   valid;//of.valid = ngx_http_core_loc_conf_t->open_file_cache_valid;  

    ngx_uint_t               min_uses;//ngx_http_core_loc_conf_t->open_file_cache_min_uses;

#if (NGX_HAVE_OPENAT)
    //disable_symlinks on | if_not_owner [from=part];��fromЯ���Ĳ���part��Ӧ�ַ������ֽ���
    size_t                   disable_symlinks_from;
    //�Ƿ����ļ�·���Ƿ��з������ӣ���ngx_http_set_disable_symlinks  disable_symlinks�������ã�Ĭ��off;
    unsigned                 disable_symlinks:2;
#endif

    unsigned                 test_dir:1;
/*
Ngx_http_core_module.c (src\http):        of.test_only = 1;
Ngx_http_index_module.c (src\http\modules):        of.test_only = 1;
Ngx_http_index_module.c (src\http\modules):    of.test_only = 1;
Ngx_http_log_module.c (src\http\modules):        of.test_only = 1;
Ngx_http_script.c (src\http):    of.test_only = 1;
*/
    unsigned                 test_only:1;
    unsigned                 log:1;
    unsigned                 errors:1;
    unsigned                 events:1;//ngx_http_core_loc_conf_t->open_file_cache_min_uses;

    unsigned                 is_dir:1;
    unsigned                 is_file:1;
    unsigned                 is_link:1;
    unsigned                 is_exec:1;
    //ע����������ļ���С����direction���ã�����1�������ʹ��direct I/O��ʽ,��Ч��ngx_directio_on
    unsigned                 is_directio:1; //���ļ���С����directio xxx���е�����ʱngx_open_and_stat_file�л���1
} ngx_open_file_info_t;


typedef struct ngx_cached_open_file_s  ngx_cached_open_file_t;

//ngx_cached_open_file_s��ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   
/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s(ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   )��ngx_expire_old_cached_files����ʧЧ�ж�, 
�����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)ngx_http_file_cache_node_t(ngx_http_file_cache_s->sh�еĳ�Ա)��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/ //Ϊʲô��Ҫ�ڴ��б����ļ�stat��Ϣ�ڵ�?��Ϊ��������Ա����ļ���fd�Ѿ��ļ���С����Ϣ���Ͳ���ÿ���ظ����ļ����һ�ȡ�ļ���С��Ϣ������ֱ�Ӷ�fd�������������Ч��
struct ngx_cached_open_file_s {//ngx_open_cached_file�д����ڵ�   ��Ҫ�洢�����ļ���fstat��Ϣ����ngx_open_and_stat_file
    //node.key���ļ������� hash = ngx_crc32_long(name->data, name->len);//�ļ�����hash��ӵ�ngx_open_file_cache_t->rbtree�������
    ngx_rbtree_node_t        node;
    ngx_queue_t              queue;

    u_char                  *name;
    time_t                   created; //��ֵ��ngx_open_cached_file  �û����ļ���Ӧ�Ĵ���ʱ��
    time_t                   accessed; //�ý׶����һ�η���ʱ�� ngx_open_cached_file�и���

    ngx_fd_t                 fd;
    ngx_file_uniq_t          uniq; //��ֵ��ngx_open_cached_file��������Դ��ngx_open_and_stat_file
    time_t                   mtime;//��ֵ��ngx_open_cached_file��������Դ��ngx_open_and_stat_file
    off_t                    size;//��ֵ��ngx_open_cached_file��������Դ��ngx_open_and_stat_file
    ngx_err_t                err;
    //ngx_open_cached_file->ngx_open_file_lookupÿ�β��ҵ��и��ļ���������1
    uint32_t                 uses;//ngx_open_cached_file�д����ڵ�ṹ��ʱ��Ĭ����1

#if (NGX_HAVE_OPENAT)
    size_t                   disable_symlinks_from;
    //�Ƿ����ļ�·���Ƿ��з������ӣ���ngx_http_set_disable_symlinks  disable_symlinks�������ã�Ĭ��off;
    unsigned                 disable_symlinks:2;
#endif
    //������ļ�������ngx_open_cached_file�м�1��ngx_open_file_cleanup�м�1��Ҳ���Ǳ�ʾ�ж��ٸ��ͻ���������ʹ�ø�node�ڵ�ngx_cached_open_file_s
    //ֻҪ��һ���ͻ���r��ʹ�øýڵ�node�������ͷŸ�node�ڵ㣬��ngx_close_cached_file
    unsigned                 count:24;//ngx_open_cached_file�д����ڵ�ṹ��ʱ��Ĭ����0  ��ʾ�����ø�node�ڵ�Ŀͻ��˸���
    unsigned                 close:1;//��ngx_expire_old_cached_files�дӺ�������Ƴ��ڵ�󣬻�ر��ļ���ͬʱ��close��1
    unsigned                 use_event:1;//ngx_open_cached_file�д����ڵ�ṹ��ʱ��Ĭ����0

    //����ı�Ǹ�ֵ��ngx_open_cached_file��������Դ��ngx_open_and_stat_file
    unsigned                 is_dir:1;
    unsigned                 is_file:1;
    unsigned                 is_link:1;
    unsigned                 is_exec:1;
    unsigned                 is_directio:1;

    //ֻ��kqueue����
    ngx_event_t             *event;//ngx_open_cached_file�д����ڵ�ṹ��ʱ��Ĭ����NULL
};

/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s(ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   )��ngx_expire_old_cached_files����ʧЧ�ж�, 
�����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)ngx_http_file_cache_node_t(ngx_http_file_cache_s->sh�еĳ�Ա)��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/

/*
nginx������ָ���ǹ������ļ���������:һ�����Ǳ�����˵����ngx_http_log_moduleģ���open_file_log_cache����;�洢��ngx_http_log_loc_conf_t->open_file_cache 
��һ����ngx_http_core_moduleģ��� open_file_cache���ã��洢��ngx_http_core_loc_conf_t->open_file_cache;ǰ����ֻ��������access������־�ļ���
������������ľͶ��ˣ�������static��index��tryfiles��gzip��mp4��flv�����Ǿ�̬�ļ�Ŷ!
������ָ���handler�������˺��� ngx_open_file_cache_init ������������������ļ��������ĵ�һ������ʼ��
*/


//Ϊʲô��Ҫ�ڴ��б����ļ�stat��Ϣ�ڵ�?��Ϊ��������Ա����ļ���fd�Ѿ��ļ���С����Ϣ���Ͳ���ÿ���ظ����ļ����һ�ȡ�ļ���С��Ϣ������ֱ�Ӷ�fd�������������Ч��
typedef struct { //ngx_open_file_cache_init�д����ռ�͸�ֵ����ngx_open_file_cache_cleanup���ͷ���Դ
    //�ú������expire_queue���еĽڵ��Ա��ngx_cached_open_file_s
    ngx_rbtree_t             rbtree;//rbtree�������expire_queue�����а�������ͬ����Ԫ��
    ngx_rbtree_node_t        sentinel;
    //��������ڹ��ڿ����ж��õģ�һ����β�������¹��ڣ�ǰ��ĺ����rbtreeһ�������ڱ������ҵ�
    ngx_queue_t              expire_queue; //�������ڻ�ȡ��һ��������е�Ԫ�غ����һ�����˶��е�Ԫ�أ�ǰ���rbtree��������ڱ�������

    //��ʼ��Ϊ0����ngx_open_cached_file�д����½ڵ��+1����ngx_expire_old_cached_files����ngx_open_file_cache_remove�м�1
    ngx_uint_t               current; //�������expire_queue�����г�Աnode������
    /*
    ��ngx_open_cached_file�д����½ڵ���������ж�
    if (cache->current >= cache->max) { //������нڵ���������ˣ�ɾ�����ϵ�node�ڵ�
        ngx_expire_old_cached_files(cache, 0, pool->log);
    }
     */
    ngx_uint_t               max; //open_file_cache max=1000 inactive=20s;�е�max
    time_t                   inactive; //open_file_cache max=1000 inactive=20s;�е�20s  ngx_expire_old_cached_files����Ч
} ngx_open_file_cache_t; //ע��ngx_http_file_cache_sh_t��ngx_open_file_cache_t������


typedef struct {
    ngx_open_file_cache_t   *cache;
    ngx_cached_open_file_t  *file;
 //file->uses >= min_uses��ʾֻҪ��ngx_cached_open_file_s file�ڵ㱻�������Ĵ����ﵽmin_uses�Σ�����Զ����ر��ļ������Ǹ�cache nodeʧЧ����ngx_open_file_cleanup  ngx_close_cached_file
    ngx_uint_t               min_uses; //���ֵ����open_file_cache_min_uses 30S���õ�ʱ��
    ngx_log_t               *log;
} ngx_open_file_cache_cleanup_t;


typedef struct {

    /* ngx_connection_t stub to allow use c->fd as event ident */
    void                    *data;
    ngx_event_t             *read;
    ngx_event_t             *write;
    ngx_fd_t                 fd;

    ngx_cached_open_file_t  *file;
    ngx_open_file_cache_t   *cache;
} ngx_open_file_cache_event_t;


ngx_open_file_cache_t *ngx_open_file_cache_init(ngx_pool_t *pool,
    ngx_uint_t max, time_t inactive);
ngx_int_t ngx_open_cached_file(ngx_open_file_cache_t *cache, ngx_str_t *name,
    ngx_open_file_info_t *of, ngx_pool_t *pool);


#endif /* _NGX_OPEN_FILE_CACHE_H_INCLUDED_ */
