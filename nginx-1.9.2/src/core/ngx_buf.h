
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;
/*
������ngx_buf_t��Nginx��������ݵĹؼ����ݽṹ������Ӧ�����ڴ�����ҲӦ���ڴ�������
*/
typedef struct ngx_buf_s  ngx_buf_t; //���ڴ���з���ngx_buf_t�ռ䣬ͨ��ngx_chain_s��pool����������ʼ�����еĸ���������ֵ����Ϊngx_create_temp_buf
/*
ngx_buf_t��һ�ֻ������ݽṹ�����������ṩ�Ľ�����һЩָ���Ա�ͱ�־λ������HTTPģ����˵����Ҫע��HTTP��ܡ��¼�������������
��ʹ��pos��last��ָ���Լ���δ�����Щ��־λ�ģ�����˵��ֻ��������÷�������������Զ���һ��ngx_buf_t�ṹ�壬��Ӧ����������
���÷�����Ӧ�ø���ҵ���������ж��塣������һ��ngx_buf_t������ת��������TCP��ʱ��pos��ָ��Ҫ���͵����ε�TCP����
ʼ��ַ����last��ָ��Ԥ����������TCP���Ļ�������ʼ��ַ����
*/
/*
ʵ���ϣ�Nginx����װ��һ������ngx_buf_t�ļ�㷽��������ȫ�ȼ��������6����䣬������ʾ��
ngx_buf_t *b = ngx_create_temp_buf(r->pool, 128);

�������ڴ�󣬿���������ڴ�д�����ݡ���д�����ݺ�Ҫ��b->lastָ��ָ�����ݵ�ĩβ�����b->last��b->pos��ȣ���ôHTTP����ǲ��ᷢ��һ���ֽڵİ���ġ�

��󣬰������ngx_buf_t *b��ngx_chain_t����ngx_http_output_filter�����Ϳ��Է���HTTP��Ӧ�İ��������ˡ����磺
ngx_chain_t out;
out.buf = b;
out.next = NULL;
return ngx_http_output_filter(r, &out);
*/ //�ο�http://blog.chinaunix.net/uid-26335251-id-3483044.html    
struct ngx_buf_s { //���Բο�ngx_create_temp_buf         �����ռ���ngx_create_temp_buf��������ָ��ָ����Щ�ռ�
/*posͨ������������ʹ���߱���Ӧ�ô�pos���λ�ÿ�ʼ�����ڴ��е����ݣ�������������Ϊͬһ��ngx_buf_t���ܱ���η�������
��Ȼ��pos�ĺ�������ʹ������ģ�鶨���*/
    //����pos��Ա��last��Աָ��ĵ�ַ֮����ڴ���ǽ��յ��Ļ�δ�������ַ���
    u_char          *pos; //posָ��ָ����ڴ���������ڴ档 posΪ��ɨ����ڴ���У���δ�������ڴ��β����
    u_char          *last;/*lastͨ����ʾ��Ч�����ݵ���Ϊֹ��ע�⣬pos��last֮����ڴ���ϣ��nginx���������*/

    /* �����ļ�ʱ��file_pos��file_last�ĺ����봦���ڴ�ʱ��pos��last��ͬ��file_pos��ʾ��Ҫ������ļ�λ�ã�file_last��ʾ��ֹ���ļ�λ�� */
    off_t            file_pos;  //���Խ��ngx_output_chain_copy_buf�Ķ��������    ������ݵĴ�ӡ������ngx_http_write_filter�в鿴������Ϣ
    //Ҳ����ʵ�ʴ浽��ʱ�ļ��е��ֽ���,��ngx_event_pipe_write_chain_to_temp_file
    off_t            file_last; //д���ļ����ݵ���β���ĳ��ȸ�ֵ��ngx_http_read_client_request_body     ������ݵĴ�ӡ������ngx_http_write_filter�в鿴������Ϣ

    //���ngx_buf_t�����������ڴ棬��ôstartָ������ڴ����ʼ��ַ
    u_char          *start;         /* start of buffer */ //�����ռ��ngx_http_upstream_process_header
    u_char          *end;           /* end of buffer */ //��start��Ա��Ӧ��ָ�򻺳����ڴ��ĩβ
    //ngx_http_request_body_length_filter�и�ֵΪngx_http_read_client_request_body
    //ʵ������һ��void*���͵�ָ�룬ʹ���߿��Թ�������Ķ�����ȥ��ֻҪ��ʹ���������塣
    ngx_buf_tag_t    tag;/*��ʾ��ǰ�����������ͣ��������ĸ�ģ��ʹ�þ�ָ�����ģ��ngx_module_t�����ĵ�ַ*/
    ngx_file_t      *file;//���õ��ļ�  ���ڴ洢���յ����а���󣬰Ѱ�������д�뵽file�ļ��У���ֵ��ngx_http_read_client_request_body
    
 /*��ǰ��������Ӱ�ӻ��������ó�Ա�����õ���������ʹ�û�����ת�����η���������Ӧʱ��ʹ����shadow��Ա��������ΪNginx̫��
 Լ�ڴ��ˣ�����һ���ڴ沢ʹ��ngx_buf_t��ʾ���յ������η�������Ӧ���������οͻ���ת��ʱ���ܻ������ڴ�洢���ļ��У�Ҳ
 ����ֱ�������η��ͣ���ʱNginx���������¸���һ���ڴ������µ�Ŀ�ģ������ٴν���һ��ngx_buf_t�ṹ��ָ��ԭ�ڴ棬�������
 ngx_buf_t�ṹ��ָ����ͬһ���ڴ棬����֮��Ĺ�ϵ��ͨ��shadow��Ա�����á�������ƹ��ڸ��ӣ�ͨ��������ʹ��

 �����buf����copy������һ��buf�������ֶε�ʱ����ô������bufָ���ʵ������ͬһ���ڴ棬������ͬһ���ļ���ͬһ���֣���
 ʱ������buf��shadow�ֶζ���ָ��Է��ġ���ô��������������buf�����ͷŵ�ʱ�򣬾���Ҫʹ�����ر�С�ģ��������������ͷţ�Ҫ
 ��ǰ���Ǻã���������Դ�Ķ���ͷţ����ܻ���ɳ��������
 */
    ngx_buf_t       *shadow; //�ο�ngx_http_fastcgi_input_filter


    /* the buf's content could be changed */
    //Ϊ1ʱ��ʾ��buf����������������һ���û��������ڴ���У����ҿ��Ա���filter����Ĺ����н��б�����������������
    unsigned         temporary:1; //��ʱ�ڴ��־λ��Ϊ1ʱ��ʾ�������ڴ���������ڴ�����޸�

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */ //Ϊ1ʱ��ʾ��buf�����������������ڴ��У�������Щ����ȷ���ܱ����д����filter���б����
    unsigned         memory:1;//��־λ��Ϊ1ʱ��ʾ�������ڴ���������ڴ治���Ա��޸�

    /* the buf's content is mmap()ed and must not be changed */
    //Ϊ1ʱ��ʾ��buf�����������������ڴ���, ��ͨ��mmapʹ���ڴ�ӳ����ļ���ӳ�䵽�ڴ��еģ���Щ����ȷ���ܱ����д����filter���б����
    unsigned         mmap:1;//��־λ��Ϊ1ʱ��ʾ����ڴ�����mmapϵͳ����ӳ������ģ������Ա��޸�

    //���Ի��յġ�Ҳ�������buf�ǿ��Ա��ͷŵġ�����ֶ�ͨ�������shadow�ֶ�һ��ʹ�õģ�����ʹ��ngx_create_temp_buf ����������buf��
    //����������һ��buf��shadow����ô����ʹ������ֶ�����ʾ���buf�ǿ��Ա��ͷŵġ�
    //��1�˱�ʾ��buf��Ҫ���Ϸ��ͳ�ȥ���ο�ngx_http_write_filter -> if (!last && !flush && in && size < (off_t) clcf->postpone_output) {
    unsigned         recycled:1; //��־λ��Ϊ1ʱ��ʾ�ɻ������ã�����buf���µ�bufָ��ָ���ʱ�򣬾���1����ngx_http_upstream_send_response

    /*
    ngx_buf_t��һ����־λin_file����in_file��Ϊ1�ͱ�ʾ���ngx_buf_t���������͵����ļ��������ڴ档
����ngx_http_output_filter����Nginx��⵽in_fileΪ1�������ngx_buf_t�������е�file��Ա����ȡʵ�ʵ��ļ���file��������ngx_file_t
    */ //Ϊ1ʱ��ʾ��buf�����������������ļ��С�   ������ݵĴ�ӡ������ngx_http_write_filter�в鿴������Ϣ
    unsigned         in_file:1;//��־λ��Ϊ1ʱ��ʾ��λ�������������ļ��������ڴ棬˵������ȫ�������ļ��У���Ҫ����"client_body_in_file_only" on | clean 

    //������flush�ֶα�����Ϊ1�ĵ�buf��chain�����chain�����ݼ��㲻�������������ݣ�last_buf�����ã���־����Ҫ��������ݶ����ˣ���
    //Ҳ����������������postpone_output���õ����ƣ����ǻ��ܵ��������ʵ��������������ơ�
    ////��1�˱�ʾ��buf��Ҫ���Ϸ��ͳ�ȥ���ο�ngx_http_write_filter -> if (!last && !flush && in && size < (off_t) clcf->postpone_output) {
    unsigned         flush:1;//��־λ��Ϊ1ʱ��ʾ��Ҫִ��flush����  ��ʾ��Ҫ�������ͻ�����������ݣ�
    /*��־λ�����ڲ�����黺����ʱ�Ƿ�ʹ��ͬ����ʽ����������ǣ�����ܻ�����Nginx���̣�Nginx�����в������������첽�ģ�����
    ��֧�ָ߲����Ĺؼ�����Щ��ܴ�����syncΪ1ʱ���ܻ��������ķ�ʽ����I/O����������������ʹ������Nginxģ�����*/
    unsigned         sync:1;
    /*��־λ����ʾ�Ƿ������һ�黺��������Ϊngx_buf_t������ngx_chain_t��������������ˣ���last_bufΪ1ʱ����ʾ��ǰ�����һ�������Ļ�����*/
    /*
    ������ܰ��������ɣ���洢���һ���������ݵ�buf��last_buf��1����ngx_http_request_body_length_filter  
    ������Ͱ����ʱ��ֻ��ͷ�����������1����ngx_http_header_filter
    �������ģ���ڷ��Ͱ������ݵ�ʱ���������ngx_http_write_filter������in����chain���е�ĳ��bufΪ�ð�������һ�����ݣ����buf�е�last_buf����1
     */ // ngx_http_send_special�л���1  ��ngx_http_write_filter -> if (!last && !flush && in && size < (off_t) clcf->postpone_output) {
    //��1�˱�ʾ��buf��Ҫ���Ϸ��ͳ�ȥ���ο�ngx_http_write_filter -> if (!last && !flush && in && size < (off_t) clcf->postpone_output) {
    unsigned         last_buf:1;  //���ݱ��Զ��chain���ݸ��˹����������ֶ�Ϊ1�����������һ��chain��
    //�ڵ�ǰ��chain���棬��buf�����һ�����ر�Ҫע�����last_in_chain��buf��һ����last_buf������last_buf��bufһ����last_in_chain�ġ�������Ϊ���ݻᱻ�Զ��chain���ݸ�ĳ��filterģ�顣
    unsigned         last_in_chain:1;//��־λ����ʾ�Ƿ���ngx_chain_t�е����һ�黺����

    //�ڴ���һ��buf��shadow��ʱ��ͨ�����´�����һ��buf��last_shadow��Ϊ1��  
    //�ο�ngx_http_fastcgi_input_filter
    //�����ݱ����ͳ�ȥ�󣬸ñ�־λ1��ngx_buf_t���ջ���ӵ�free_raw_bufs��
    //��ֵһ�㶼Ϊ1��
    unsigned         last_shadow:1; /*��־λ����ʾ�Ƿ������һ��Ӱ�ӻ���������shadow�����ʹ�á�ͨ��������ʹ����*/
    unsigned         temp_file:1;//��־λ����ʾ��ǰ�������Ƿ�������ʱ�ļ�

    /* STUB */ int   num;  //��Ϊ��ȡ��˷������������ĵڼ���buf ����ngx_event_pipe_read_upstream  ��ʾ��������chain�еĵڼ���buf
};

/*
ngx_chain_t����ngx_buf_t���ʹ�õ��������ݽṹ�����濴һ�����Ķ��壺
typedef struct ngx_chain_s       ngx_chain_t;
struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};

bufָ��ǰ��ngx_buf_t��������next������ָ����һ��ngx_chain_t������������һ��ngx_chain_t������Ҫ��next��ΪNULL��

�����û�����HTTP ����ʱ����Ҫ����ngx_chain_t�������ע�⣬��������һ��ngx_chain_t����ô���뽫next��ΪNULL��
������Զ���ᷢ�ͳɹ��������������һֱ���������Nginx��ܵ�Ҫ�󣩡�
*/
struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};

//��ʾnum��size�ռ��С  �� 4 8K����ʾ4��8K�ռ䣬���Բο�ngx_conf_set_bufs_slot
typedef struct {//ͨ��output_buffers��������
    ngx_int_t    num; //ͨ��output_buffers��������outpu����ĸ���
    size_t       size;
} ngx_bufs_t; //proxy_buffers  fastcgi_buffers 4 4K��ֵ��ngx_event_pipe_read_upstream


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

#if (NGX_HAVE_FILE_AIO)
typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);
#endif

struct ngx_output_chain_ctx_s {//ngx_http_copy_filter�д����ռ�͸�ֵ
    /*ngx_output_chain_copy_bufc��tx->in�е��ڴ����ݻ��߻����ļ����ݻ´����dst�У�Ҳ����ctx->buf,Ȼ����ngx_output_chain_copy_buf����
    �������°�ctx->buf��ֵ���µ�chain��Ȼ��write��ȥ*/

    /* ������ʱ��buf */ //ʵ��bufָ����ڴ�ռ���ngx_output_chain_align_file_buf����ngx_output_chain_get_buf ���ٵ�
    ngx_buf_t                   *buf;

/*ngx_output_chain_copy_bufc��tx->in�е��ڴ����ݻ��߻����ļ����ݻ´����dst�У�Ҳ����ctx->buf,Ȼ����ngx_output_chain_copy_buf����
�������°�ctx->buf��ֵ���µ�chain��Ȼ��write��ȥ*/ //һ������in�п���65536�ֽڣ���ngx_output_chain_copy_buf
    
    /* �����˽�Ҫ���͵�chain */  //ʵ��in����ngx_output_chain->ngx_output_chain_add_copy(ctx->pool, &ctx->in, in)��ctx->in���������in��ֱ�ӿ�����ֵ
    //�����aio thread��ʽ������ngx_output_chain_copy_buf->ngx_thread_read->ngx_thread_read_handler�ж�ȡ��in->buf��
    ngx_chain_t                 *in;//in�Ǵ����͵����ݣ�busy���Ѿ�����ngx_chain_writer����û�з�����ϡ�
    ngx_chain_t                 *free;/* �������Ѿ�������ϵ�chain���Ա����ظ����� */  
    ngx_chain_t                 *busy; /* �����˻�δ���͵�chain */  

    /*
       �ͺ�˵�ngx_connection_t��ngx_event_connect_peer������Ϊ1������ngx_http_upstream_connect��c->sendfile &= r->connection->sendfile;��
       �Ϳͻ����������ngx_connextion_t��sendfile��Ҫ��ngx_http_update_location_config���жϣ�������������Ƿ���configure��ʱ���Ƿ��м�
       sendfileѡ������������1������0
    */
    unsigned                     sendfile:1;/* sendfile��� */  
//directʵ�����м���������ƣ���ʹ��ϵͳ����һ���棬��һ������ʹ��dmaֱ����dma���ƴ��ڴ����뵽�û��ռ��buffer�в�����cpu��mov������������cpu��
//��ʾ�ڻ�ȡ�ռ��ʱ���Ƿ���Ҫctx->alignment�ֽڶ��룬��ngx_output_chain_get_buf
//��ngx_output_chain_copy_buf�л��sendfile��Ӱ��  //��ֵ��1��ǰ����ngx_output_chain_as_is����0
    /*
         Ngx_http_echo_subrequest.c (src\echo-nginx-module-master\src):        b->file->directio = of.is_directio;
         Ngx_http_flv_module.c (src\http\modules):    b->file->directio = of.is_directio;
         Ngx_http_gzip_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
         Ngx_http_mp4_module.c (src\http\modules):    b->file->directio = of.is_directio;
         Ngx_http_static_module.c (src\http\modules):    b->file->directio = of.is_directio;
        ֻ���������⼸��ģ���в��п�����1����Ϊof.is_directioֻ�����ļ���С����directio 512���õĴ�Сʱ�Ż���1����ngx_open_and_stat_file�л���1
        
     */ 
     /* �������ļ����棬���ҳ������ߵ��� b->file->directio = of.is_directio(����of.is_directioҪΪ1)�⼸��ģ�飬
        �����ļ���С����directio xxx�еĴ�С�ſ�����1����ngx_output_chain_align_file_buf  ngx_output_chain_as_is */
    unsigned                     directio:1;/* directio��� */ //���û������direction������£�ngx_output_chain_align_file_buf ����1
#if (NGX_HAVE_ALIGNED_DIRECTIO)
    /* �������ļ����棬���ҳ������ߵ��� b->file->directio = of.is_directio;�⼸��ģ�飬
        �����ļ���С����directio xxx�еĴ�С */
    unsigned                     unaligned:1;//���û������direction������£�����ngx_output_chain_align_file_buf����1
#endif
/* �Ƿ���Ҫ���ڴ��б���һ��(ʹ��sendfile�Ļ����ڴ���û���ļ��Ŀ����ģ���������ʱ��Ҫ�����ļ�����ʱ����Ҫ����������) */ 
    unsigned                     need_in_memory:1;
    
    /* �Ƿ���Ҫ���ڴ������¸���һ�ݣ�����buf�����ڴ滹���ļ�, �����Ļ�������ģ�����ֱ���޸�����ڴ� */  
    unsigned                     need_in_temp:1;
#if (NGX_HAVE_FILE_AIO || NGX_THREADS)
/*
ngx_http_copy_aio_handler handler ngx_http_copy_aio_event_handlerִ�к󣬻��ûص�0   
ngx_http_copy_thread_handler ngx_http_copy_thread_event_handler��0
ngx_http_cache_thread_handler��1�� ngx_http_cache_thread_event_handler��0
ngx_http_file_cache_aio_read����1��
*/
    //ngx_http_copy_aio_handler����1   ngx_http_copy_filter��ctx->aio = ngx_http_request_t->aio
    //��Ч�ط���ngx_output_chain
    unsigned                     aio:1; //����aio on����aio thread poll��ʱ����1
#endif

#if (NGX_HAVE_FILE_AIO)
//ngx_http_copy_filter��ֵΪctx->aio_handler = ngx_http_copy_aio_handler;ִ����ngx_output_chain_copy_buf
    ngx_output_chain_aio_pt      aio_handler;
#if (NGX_HAVE_AIO_SENDFILE)
    ssize_t                    (*aio_preload)(ngx_buf_t *file);
#endif
#endif

#if (NGX_THREADS)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif
    //��ֵ��ngx_http_upstream_init_request  Ĭ��512   ��ngx_output_chain_get_buf��Ч����ʾ����ռ��ʱ�򣬿ռ���ʼ��ַ��Ҫ�������ֵ����
    off_t                        alignment;//directio_alignment 512;  ����directio���ʹ�ã�ָ����directio��ʽ��ȡ�ļ�ʱ�Ķ��뷽ʽ

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;//�Ѿ������buf����  ngx_output_chain_get_buf����һ��ngx_buf_t��ʱ�򣬻�����
    //ͨ��output_buffers�������ã���ֵ��ngx_http_copy_filter   Ĭ��ֵoutput_buffers 1 32768  
    //ngx_http_copy_filter�и�ֵΪngx_http_copy_filter_conf_t->bufs
    //�����ж���Ч��ngx_output_chain
    ngx_bufs_t                   bufs;//��ֵ��ngx_http_upstream_init_request  ��Ӧloc conf�����õ�bufs
    ngx_buf_tag_t                tag; //��ʶ�Լ�������ģ�飬����ο�ngx_http_fastcgi_handler  ģ���ǣ���Ҫ����buf����

/*
�����fastcgi_pass�����Ҳ���Ҫ����ͻ��˰��壬��output_filter=ngx_http_fastcgi_body_output_filter
�����proxy_pass�����Ҳ���Ҫ����ͻ��˰���,����internal_chunked ==1����output_filter=ngx_http_proxy_body_output_filter
�������Ĭ����ngx_http_upstream_init_request����Ϊngx_chain_writer
*/ //copy_filterģ���outpu_filter=ngx_http_next_body_filter
    ngx_output_chain_filter_pt   output_filter; //ngx_output_chain ��ֵ��ngx_http_upstream_init_request һ����ngx_http_next_filter,Ҳ���Ǽ�������filter��
    //һ��ִ�пͻ�������r
    void                        *filter_ctx;// ��ֵ��ngx_http_upstream_init_request /* ��ǰfilter�������ģ�����������upstreamҲ�����output_chain */  
};


typedef struct {
    //ngx_http_upstream_connect�г�ʼ����ֵu->writer.last = &u->writer.out; lastָ��ָ��out������ngx_chain_writer��lastָ��洢��out��cl�����һ���ڵ��NEXT��
    ngx_chain_t                 *out;//��û�з��ͳ�ȥ�Ĵ��������ݵ�ͷ��
    //ngx_http_upstream_connect�г�ʼ����ֵu->writer.last = &u->writer.out; lastָ��ָ��out������ngx_chain_writer��lastָ��洢��out��cl�����һ���ڵ��NEXT��
    ngx_chain_t                **last;//��Զָ������һ��ngx_chain_t��next�ֶεĵ�ַ����������ͨ�������ַ���ϵ��ں�������Ԫ�ء�
    ngx_connection_t            *connection; //�������������Ӧ������
    ngx_pool_t                  *pool; //����request��Ӧ��pool����ngx_http_upstream_init_request
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR

//�������buf����������Ƿ����ڴ��
#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)

//�������buf����������Ƿ�������ڴ������û�����ļ��
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

//�����special��buf��Ӧ���Ǵ�ngx_http_send_special������
//���ظ�buf�Ƿ���һ�������buf��ֻ��������ı�־��û�а������������ݡ�
#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

//���ظ�buf�Ƿ���һ��ֻ����sync��־���������������ݵ�����buf��
#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

//���ظ�buf�������ݵĴ�С������������������ļ��ﻹ�����ڴ��
#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);

/*
pool �е� chain ָ��һ�� ngx_chain_t ���ݣ���ֵ���ɺ� ngx_free_chain ���и���ģ�ָ��֮ǰ�����˵ģ�
�����ͷŵ�ngx_chain_t���ݡ��ɺ���ngx_alloc_chain_link����ʹ�á�
*/

#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
