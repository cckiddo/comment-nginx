
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

/*
ngx_pool_cleanup_t��ngx_http_cleanup_pt�ǲ�ͬ�ģ�ngx_pool_cleanup_t�������õ��ڴ������ʱ�Żᱻ������������Դ������ʱ�ͷ���
Դ������ʹ�õ��ڴ�ض�������ngx_http_cleanup_pt����ngx_http_request_t�ṹ���ͷ�ʱ���������ͷ���Դ�ġ�


���������Ҫ����Լ��Ļص�����������Ҫ����ngx_pool_cleanup_add���õ�һ��ngx_pool_cleanup_t��Ȼ������handlerΪ���ǵ���������
������dataΪ����Ҫ��������ݡ�������ngx_destroy_pool�л�ѭ������handler�������ݣ�

���磺���ǿ��Խ�һ��������ļ���������Ϊ��Դ���ص��ڴ���ϣ�ͬʱ�ṩһ���ر��ļ������ĺ���ע�ᵽhandler�ϣ���ô�ڴ�����ͷ�
��ʱ�򣬾ͻ���������ṩ�Ĺر��ļ������������ļ���������Դ��
*/
//�ڴ��pool���������ݵ��õģ���ngx_pool_s  ngx_destroy_pool
struct ngx_pool_cleanup_s { //�������ӵ�ngx_pool_s�е�cleanup�ϵģ���ngx_pool_cleanup_add
    ngx_pool_cleanup_pt   handler;// ��ǰ cleanup ���ݵĻص�����  ngx_destroy_pool��ִ��    ���������ļ����ngx_pool_cleanup_file��
    void                 *data;// �ڴ��������ַ     �ص�ʱ���������ݴ���ص�������  ngx_pool_cleanup_add�п��ٿռ�
    
    ngx_pool_cleanup_t   *next;// ָ����һ�� cleanup �ڴ��ָ��
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

/*
�ڴ��           ---  ngx_pool_s;
�ڴ������       ---  ngx_pool_data_t;
���ڴ��         --- ngx_pool_large_s; 
*///����ڴ�ṹ��,����ṹ  
struct ngx_pool_large_s { //ngx_pool_s�еĴ���ڴ��Ա
    ngx_pool_large_t     *next;
    void                 *alloc;//������ڴ���ַ   
};

/*
�ڴ��           ---  ngx_pool_s;
�ڴ������       ---  ngx_pool_data_t;
���ڴ��         --- ngx_pool_large_s; 
*/ //�ڴ�����������   
typedef struct {
    u_char               *last;//��������ڴ��β��ַ,��������׵�ַ    pool->d.last ~ pool->d.end �е��ڴ������ǿ�����������
    u_char               *end;//��ǰ�ڴ�ؽڵ����������ڴ������λ��  
    ngx_pool_t           *next;//��һ���ڴ�ؽڵ�ngx_pool_t,��ngx_palloc_block
    ngx_uint_t            failed;//��ǰ�ڵ������ڴ�ʧ�ܵĴ���,   ������ִӵ�ǰpool�з����ڴ�ʧ���ĴΣ���ʹ����һ��pool,��ngx_palloc_block 
} ngx_pool_data_t;

/*
Ϊ�˼����ڴ���Ƭ����������ͨ��ͳһ���������ٴ����г����ڴ�й©�Ŀ����ԣ�Nginx�����ngx_pool_t�ڴ�����ݽṹ��
*/
/*
�ڴ��           ---  ngx_pool_s;
�ڴ������       ---  ngx_pool_data_t;
���ڴ��         --- ngx_pool_large_s; 
*/
//�ڴ�����ݽṹ,������ʽ�洢   ͼ�λ����ο�Nginx �ڴ�أ�pool������ http://www.linuxidc.com/Linux/2011-08/41860.htm
struct ngx_pool_s {
    ngx_pool_data_t       d;//�ڵ�����    // ���� pool ��������ָ��Ľṹ�� pool->d.last ~ pool->d.end �е��ڴ������ǿ�����������
    size_t                max;//��ǰ�ڴ�ڵ�������������ڴ�ռ� // һ������pool�п��ٵ����ռ�
    //ÿ�δ�pool�з����ڴ��ʱ���Ǵ�curren��ʼ����pool�ڵ��ȡ�ڴ��
    ngx_pool_t           *current;//�ڴ���п��������ڴ�ĵ�һ���ڵ�      pool ��ǰ����ʹ�õ�pool��ָ�� current ��Զָ���pool�Ŀ�ʼ��ַ��current����˼�ǵ�ǰ��pool��ַ

/*
pool �е� chain ָ��һ�� ngx_chain_t ���ݣ���ֵ���ɺ� ngx_free_chain ���и���ģ�ָ��֮ǰ�����˵ģ�
�����ͷŵ�ngx_chain_t���ݡ��ɺ���ngx_alloc_chain_link����ʹ�á�
*/
    ngx_chain_t          *chain;// pool ��ǰ���õ� ngx_chain_t ���ݣ�ע�⣺�� ngx_free_chain ��ֵ   ngx_alloc_chain_link
    ngx_pool_large_t     *large;//�ڵ��д��ڴ��ָ��   // pool ��ָ������ݿ��ָ�루�����ݿ���ָ size > max �����ݿ飩
    ngx_pool_cleanup_t   *cleanup;// pool ��ָ�� ngx_pool_cleanup_t ���ݿ��ָ�� //cleanup��ngx_pool_cleanup_add��ֵ
    ngx_log_t            *log; // pool ��ָ�� ngx_log_t ��ָ�룬����д��־��  ngx_event_accept�ḳֵ
};

typedef struct {//ngx_open_cached_file�д����ռ�͸�ֵ
    ngx_fd_t              fd;//�ļ����
    u_char               *name; //�ļ�����
    ngx_log_t            *log;//��־����
} ngx_pool_cleanup_file_t;


void *ngx_alloc(size_t size, ngx_log_t *log);
void *ngx_calloc(size_t size, ngx_log_t *log);

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
