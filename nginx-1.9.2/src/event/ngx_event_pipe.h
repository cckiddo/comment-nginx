
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_PIPE_H_INCLUDED_
#define _NGX_EVENT_PIPE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


typedef struct ngx_event_pipe_s  ngx_event_pipe_t;

//������������εİ���Ļص�����ԭ��
typedef ngx_int_t (*ngx_event_pipe_input_filter_pt)(ngx_event_pipe_t *p,
                                                    ngx_buf_t *buf);
//�����η�����Ӧ�Ļص�����ԭ��                                                    
typedef ngx_int_t (*ngx_event_pipe_output_filter_pt)(void *data,
                                                     ngx_chain_t *chain);

//��Ա��ʼ������ֵ��ngx_http_upstream_send_response�и�ֵ
struct ngx_event_pipe_s { //ngx_http_XXX_handler(ngx_http_fastcgi_handler)�д���
    ngx_connection_t  *upstream;// Nginx�����η�����֮������� p->upstream = u->peer.connection;
    ngx_connection_t  *downstream;//Nginx�����οͻ���֮������� p->downstream = c
    
    /* ֱ�ӽ��������η������Ļ����������������δ���κδ�������ݡ��������������ģ�����ܵ���Ӧ��������ͷ�� */
    /*
       1.���ngx_event_pipe_read_upstream��ngx_readv_chain����NGX_AGAIN,���´��ٴζ���ʱ�򣬾�ֱ�����ϴ�û�õ�chain
    
       2.�������������ngx_event_pipe_read_upstream����buf�ռ䣬����ȴ����buf��û�ж�ȡ��ʵ�ʵ���ҳ��������(�����յ�һ��fastcgi END)�������Ҫ�Ѹ�bufָ���ڴ�
         ����free_raw_bufs�����У��Ա����´ζ�ȡ��˰����ʱ��ֱ�Ӵ�����ȡ,��ngx_http_fastcgi_input_filter->ngx_event_pipe_add_free_buf
       3.������������������ȡ��˵�����û������һ��bufָ����ڴ棬�����ʱ��ӵ�free_raw_bufs��ͷ������ngx_event_pipe_read_upstream
     */
    ngx_chain_t       *free_raw_bufs; //��ֵ��ngx_event_pipe_read_upstream
    // ��ʾ���յ���������Ӧ���������������Ǿ���input_filter�����
    //ngx_event_pipe_read_upstream��ȡ���ݺ�ͨ��ngx_http_fastcgi_input_filter�Ѷ�ȡ�������ݼ��뵽p->in����
    //ngx_http_write_filter��in�е�����ƴ�ӵ�out���棬Ȼ�����writev���ͣ�û�з��ͳ�ȥ������buf���ǻ�����out������
    ngx_chain_t       *in;//ÿ�ζ�ȡ���ݺ󣬵���input_filter��Э���ʽ���н����������������ݲ��ַŵ�in�����γ�һ�������ο�ngx_http_fastcgi_input_filter
    /*����p->in��shadow��inָ��һ��chain����ÿ������ָ��һ��ʵʵ���ڵ�fcgi DATA���ݣ����������php�ȴ���鹲��һ������FCGI���ݿ飻
    ����ĳ�������FCGI���ݿ�����һ�����ݽڵ��last_shadow��ԱΪ1����ʾ���������FCGI���ݿ�����һ���������ҵ�shadowָ��ָ�������FCGI���ݿ��bufָ��
	�ͷ���Щ�����ݿ��ʱ�򣬿��Բο�ngx_event_pipe_drain_chains�����ͷš�
    */ // ָ����յ���һ��������
    ngx_chain_t      **last_in; //ִ��ngx_event_pipe_t->in�����һ��chain�ڵ�  �ο�ngx_http_fastcgi_input_filter

    //ngx_event_pipe_read_upstream��ȡ���ݺ�ͨ��ngx_http_fastcgi_input_filter�Ѷ�ȡ�������ݼ��뵽p->in����
    //ngx_http_write_filter��p->in�е�����ƴ�ӵ�ngx_http_request_t->out���棬Ȼ�����writev���ͣ�û�з��ͳ�ȥ������buf���ǻ�����ngx_http_request_t->out������
    
    //�����Ž�Ҫ�����ͻ��˵Ļ�����������д����ʱ�ļ��ɹ�ʱ�����in�еĻ�������ӵ�out��
    //buf��tempfile�����ݻ�ŵ�out���棬�����µ�chainָ����ngx_event_pipe_write_chain_to_temp_file�����������õġ�ngx_event_pipe_write_chain_to_temp_file

/*
bufferin��ʽ�������xxx_buffers  XXX_buffer_sizeָ���Ŀռ䶼�����ˣ����ѻ����е�����д����ʱ�ļ���Ȼ�������������ngx_event_pipe_write_chain_to_temp_file
��д����ʱ�ļ���ֱ��read����NGX_AGAIN,Ȼ����ngx_event_pipe_write_to_downstream->ngx_output_chain->ngx_output_chain_copy_buf�ж�ȡ��ʱ�ļ�����
���͵���ˣ������ݼ���������ͨ��epoll read����ѭ��������
*/
    ngx_chain_t       *out;  //���������ݴ�����ʱ�ļ�������outָ�� �ο�ngx_event_pipe_write_chain_to_temp_file
    // �ȴ��ͷŵĻ�����
    ngx_chain_t       *free;
    // ��ʾ�ϴε���ngx_http_output_filter����������Ӧʱû�з�����Ļ���������
    //����out�и����ڵ�ָ���buf����ռ�Ĵ�С�Ƿ�Ϊ0��������out�и����ڵ�buf�������Ƿ��Ѿ����ͳ�ȥ�����out�и���buf�Ѿ�������ϣ����ƶ���
    //free�У��������ʣ������ӵ�busy�С���ngx_chain_update_chains

    /*ʵ����p->busy����ָ�����ngx_http_write_filter��δ�������r->out�б�������ݣ��ⲿ������ʼ����r->out����ǰ�棬�����ڶ������ݺ���
    ngx_http_write_filter�л�����������ݼӵ�r->out���棬Ҳ����δ���͵�������r->outǰ���������������棬����ʵ��write��֮ǰδ���͵��ȷ��ͳ�ȥ*/
    ngx_chain_t       *busy;  //�ñ������ڵ�ԭ�����Ҫ���ڻ��ɺ�����ݵ���󳤶ȣ��ο�ngx_event_pipe_write_to_downstream(if (bsize >= (size_t) p->busy_size) {)
    //������еĸ���bufָ���ָ����ʵ��ngx_http_request_t->out�еĸ���bufָ���ָ�붼ָ��ͬ�����ڴ�飬���ǵ���ngx_http_output_filter��û�з��ͳ�ȥ������


    /*
     * the input filter i.e. that moves HTTP/1.1 chunks
     * from the raw bufs to an incoming chain
     */
     //buffering��ʽ�����Ӧ����ʹ��ngx_event_pipe_t->input_filter  ��buffering��ʽ��Ӧ��˰���ʹ��ngx_http_upstream_s->input_filter����ngx_http_upstream_send_response�ֲ�
    // ������յ��ġ��������η����������� //FCGIΪngx_http_fastcgi_input_filter������Ϊngx_event_pipe_copy_input_filter �����������ض���ʽ����
    //��ngx_event_pipe_read_upstream��ִ��
    ngx_event_pipe_input_filter_pt    input_filter;//��ngx_http_fastcgi_handler(���ngx_http_xxx_handler) ngx_http_proxy_copy_filter ngx_http_fastcgi_input_filter
    // ����input_filter�ĵĲ�����һ����ngx_http_request_t�ĵ�ַ
    void                             *input_ctx; //ָ���Ӧ�Ŀͻ�������ngx_http_request_t����ngx_http_fastcgi_handler ngx_http_proxy_handler

    // �����η�����Ӧ�ĺ��� ע�����һ�ε���ngx_http_output_filterû�з�����ϣ���ʣ������ݻ���ӵ�ngx_http_request_t->out��
    ngx_event_pipe_output_filter_pt   output_filter;//ngx_http_output_filter
    // output_filter�Ĳ�����ָ��ngx_http_request_t                      
    void                             *output_ctx; //ngx_http_upstream_send_response�и�ֵ

    // 1����ʾ��ǰ�Ѷ�ȡ�����ε���Ӧ  Ҳ�����ж�����˷������İ���
    unsigned           read:1; //ֻҪ��n = p->upstream->recv_chain()�ж������ݣ�Ҳ����n����0����read=1;
    unsigned           cacheable:1; // 1�������ļ����� p->cacheable = u->cacheable || u->store;
    unsigned           single_buf:1;  // 1����ʾ����������Ӧʱ��һ��ֻ�ܽ���һ��ngx_buf_t������
    unsigned           free_bufs:1; // 1��һ�����ٽ������ΰ��壬�������ܵ��ͷŻ�����
    //��ҳ�����Ѿ�����    proxy��ȡ���˰���ο�ngx_http_proxy_copy_filter
    unsigned           upstream_done:1; // 1����ʾNginx�����ν����Ѿ����� ���˶����ӵ�������յ�NGX_HTTP_FASTCGI_END_REQUEST��ʶ������1
    unsigned           upstream_error:1;// 1��Nginx�����η����������ӳ��ִ���
    //p->upstream->recv_chain(p->upstream, chain, limit);����0��ʱ����1
    unsigned           upstream_eof:1;//p->upstream->recv_chain(p->upstream, chain, limit);����0��ʱ����1
    /* 1����ʾ��ʱ������ȡ������Ӧ�ĵ����̡���ʱ���ȵ���ngx_event_pipe_write_to_downstream
    �������ͻ������е����ݸ����Σ��Ӷ��ڳ��������ռ䣬�ٵ���ngx_event_pipe_read_upstream
    ������ȡ������Ϣ */ ////��cachable��ʽ�£�ָ������Ŀռ��������1
    unsigned           upstream_blocked:1; //��cachable��ʽ�£�����ָ���Ŀռ�(����fastcgi_buffers  5 3K  )�Ѿ����꣬�����Ҫ����һ�£��ȷ���һ�������ݵ��ͻ���������󣬾Ϳ��Լ�������
    unsigned           downstream_done:1;  // 1�������εĽ����ѽ���
    unsigned           downstream_error:1; // 1�������ε����ӳ��ִ���
    //Ĭ��0����//fastcgi_cyclic_temp_file  XXX_cyclic_temp_file����
    unsigned           cyclic_temp_file:1; // 1��������ʱ�ļ���������ngx_http_upstream_conf_t�е�ͬ����Ա��ֵ��

    ngx_int_t          allocated;    // �ѷ���Ļ���������
    // ��¼�˽���������Ӧ���ڴ滺������С��bufs.size��ʾÿ���ڴ滺������С��bufs.num��ʾ��������num�������� 
    //�Ի�����Ӧ�ķ�ʽת�����η������İ���ʱ��ʹ�õ��ڴ��С 
    //��ngx_event_pipe_read_upstream�д����ռ�  Ĭ��fastcgi_buffers 8  ngx_pagesize  ֻ���buffing��ʽ��Ч
    ngx_bufs_t         bufs;//����fastcgi_buffers  5 3K   ��Ա��ʼ������ֵ��ngx_http_upstream_send_response�и�ֵ
    // �������á��Ƚϻ����������е�ngx_buf_t�ṹ���tag��־λ
    ngx_buf_tag_t      tag; //p->tag = u->output.tag //��Ա��ʼ������ֵ��ngx_http_upstream_send_response�и�ֵ

    /* busy�������д�������Ӧ���ȵ����ֵ��������busy_sizeʱ������ȴ�busy�������������㹻�����ݣ����ܼ�������out��in�е����� */
    //��������Ч�ط���ngx_event_pipe_write_to_downstream
    ssize_t            busy_size; // p->busy_size = u->conf->busy_buffers_size; //����buffering��־λΪ1������������ת����Ӧʱ��Ч��

    // �Ѿ����յ�����������Ӧ����ĳ���
    off_t              read_length;
    //��ʼֵ-1  flcf->keep_conn //fastcgi_keep_conn  on | off �ͺ�˳����ӵ�ʱ��Żᷢ����ֵ
    //�����chunk�Ĵ��ͷ��ͣ���һֱΪ-1  fastcgi���岿�ֳ��ȸ�ֵ��ngx_http_fastcgi_input_filter  
    //proxy���峤�ȸ�ֵ��ngx_http_proxy_input_filter_init  ��ȡ���ְ�����ngx_http_proxy_copy_filter��ȥ��ȡ���ⲿ�֣���ʾ����Ҫ�����ٲ��ܶ���
    /* fastcgi����£���˶����ӵ�������յ�NGX_HTTP_FASTCGI_END_REQUEST��ʶ������upstream_doneΪ1����ʾ������ݶ�ȡ��ϣ���˶�fastcgi��˵lengthһֱδ-1
        ���Ϊproxy����£���ȡ���ְ�����ngx_http_proxy_copy_filter��p->length��ȥ��ȡ���ⲿ�֣���ʾ����Ҫ�����ٲ��ܶ���
      */
    off_t              length; //��ʾ����Ҫ��ȡ���ٰ���ű�ʾ������ҳ�������  ���Բο�ngx_http_fastcgi_input_filter

    /*
��buffering��־λΪ1ʱ����������ٶȿ��������ٶȣ����п��ܰ��������ε���Ӧ�洢����ʱ�ļ��У���max_temp_file_sizeָ������ʱ�ļ���
��󳤶ȡ�ʵ���ϣ���������ngx_event_pipe_t�ṹ���е�temp_file     fastcgi_max_temp_file_size����
*/
    // ��ʾ��ʱ�ļ�����󳤶� ��������ٶȿ��������ٶȣ����п��ܰ��������ε���Ӧ�洢����ʱ�ļ��У���max_temp_file_sizeָ������ʱ�ļ�����󳤶ȡ�
    off_t              max_temp_file_size; //p->max_temp_file_size = u->conf->max_temp_file_size;
    // ��ʾһ��д���ļ�ʱ���ݵ���󳤶� //fastcgi_temp_file_write_size���ñ�ʾ���������е���Ӧд����ʱ�ļ�ʱһ��д���ַ�������󳤶�
    ssize_t            temp_file_write_size; //p->temp_file_write_size = u->conf->temp_file_write_size;

    // ��ȡ������Ӧ�ĳ�ʱʱ��
    ngx_msec_t         read_timeout;
    // �����η�����Ӧ�ĳ�ʱʱ��
    ngx_msec_t         send_timeout;
    // �����η�����Ӧʱ��TCP���������õ�send_lowat��ˮλ�� �ں˻���������ֻ�дﵽ��ֵ���ܴӻ��������ͳ�ȥ
    ssize_t            send_lowat;

    ngx_pool_t        *pool;
    ngx_log_t         *log;

    // ��ʾ�ڽ������η�������Ӧͷ���׶Σ��Ѿ���ȡ����Ӧ����
    ngx_chain_t       *preread_bufs; //ngx_http_upstream_send_response�д����ռ�͸�ֵ
    // ��ʾ�ڽ������η�������Ӧͷ���׶Σ��Ѿ���ȡ����Ӧ���峤��
    size_t             preread_size;
    
    // ���ڻ����ļ� if(u->cacheable == 1) ngx_http_upstream_send_response�д����ռ�͸�ֵ
    //ָ�����Ϊ��ȡ���ͷ���е�ʱ�����ĵ�һ����������ͷ���в��֣�buf��С��xxx_buffer_size(fastcgi_buffer_size proxy_buffer_size memcached_buffer_size)ָ��
  /*
    ������ֻ�洢��ͷ����buffer��ͷ���е����ݲ��֣���Ϊ����д��ʱ�ļ���ʱ����Ҫ�Ѻ��ͷ����Ҳд����������ǰ���ȡͷ���к�ָ���Ѿ�ָ�������ݲ���
    �����Ҫ��ʱ��buf_to_file->startָ��ͷ���в��ֿ�ʼ��posָ�����ݲ��ֿ�ʼ��Ҳ����ͷ���в��ֽ�β
    ngx_event_pipe_write_chain_to_temp_fileд����ʱ�ļ���buf_to_file�ᱻ��ΪNULL
  */
    ngx_buf_t         *buf_to_file; //���ͷ���в���д����ʱ�ļ�����ngx_event_pipe_write_chain_to_temp_file�л��buf_to_file��ΪNULL

    size_t             limit_rate;////Ĭ��ֵ0 fastcgi_limit_rate ����proxy memcached�Ƚ�����������  ���Ƶ�����ͻ�����������ٶȣ��������˵��ٶ�
    time_t             start_sec;

    // ���������Ӧ����ʱ�ļ�  Ĭ��ֵngx_http_fastcgi_temp_path
/*
Ĭ�������p->temp_file->path = u->conf->temp_path; Ҳ������ngx_http_fastcgi_temp_pathָ��·������������ǻ��淽ʽ(p->cacheable=1)��������
proxy_cache_path(fastcgi_cache_path) /a/b��ʱ�����use_temp_path=off(��ʾ��ʹ��ngx_http_fastcgi_temp_path���õ�path)��
��p->temp_file->path = r->cache->file_cache->temp_path; Ҳ������ʱ�ļ�/a/b/temp��use_temp_path=off��ʾ��ʹ��ngx_http_fastcgi_temp_path
���õ�·������ʹ��ָ������ʱ·��/a/b/temp   ��ngx_http_upstream_send_response  
ngx_event_pipe_write_chain_to_temp_file->ngx_write_chain_to_temp_file�д�����д����ʱ�ļ�
*/    
    /* ��ǰfastcgi_buffers ��fastcgi_buffer_size���õĿռ䶼�Ѿ������ˣ�����Ҫ������д����ʱ�ļ���ȥ���ο�ngx_event_pipe_read_upstream */
    //д��ʱ�ļ���max_temp_file_size  temp_file_write_sizeҲ�й�ϵ
/*
�������xxx_buffers  XXX_buffer_sizeָ���Ŀռ䶼�����ˣ����ѻ����е�����д����ʱ�ļ���Ȼ�������������ngx_event_pipe_write_chain_to_temp_file
��д����ʱ�ļ���ֱ��read����NGX_AGAIN,Ȼ����ngx_event_pipe_write_to_downstream->ngx_output_chain->ngx_output_chain_copy_buf�ж�ȡ��ʱ�ļ�����
���͵���ˣ������ݼ���������ͨ��epoll read����ѭ��������
*/ //temp_fileĬ�����ļ����ݷ��͵��ͻ��˺󣬻�ɾ���ļ�����ngx_create_temp_file->ngx_pool_delete_file
//��ngx_http_file_cache_update���Կ��������������д����ʱ�ļ�����д��xxx_cache_path�У���ngx_http_file_cache_update

    /*ngx_http_upstream_init_request->ngx_http_upstream_cache �ͻ��˻�ȡ���� ���Ӧ��������ݺ���ngx_http_upstream_send_response->ngx_http_file_cache_create
    �д�����ʱ�ļ���Ȼ����ngx_event_pipe_write_chain_to_temp_file�Ѷ�ȡ�ĺ������д����ʱ�ļ��������
    ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update�а���ʱ�ļ�����rename(�൱��mv)��proxy_cache_pathָ��
    ��cacheĿ¼����
    */
    /*������ݶ�ȡ��ϣ�����ȫ��д����ʱ�ļ���Ż�ִ��rename���̣�Ϊʲô��Ҫ��ʱ�ļ���ԭ����:����֮ǰ�Ļ�������ˣ������и��������ڴӺ��
    ��ȡ����д����ʱ�ļ��������ֱ��д�뻺���ļ������ڻ�ȡ������ݹ����У��������һ���ͻ��������������proxy_cache_use_stale updating����
    ������������ֱ�ӻ�ȡ֮ǰ�ϾɵĹ��ڻ��棬�Ӷ����Ա����ͻ(ǰ�������д�ļ�������������ȡ�ļ�����) 
    */
    ngx_temp_file_t   *temp_file;  //tempfile������ngx_create_temp_file  ���ջ�ͨ��ngx_create_hashed_filename��path��level=N:N��֯��һ��


    // ��ʹ�õ�ngx_buf_t��������Ŀ/* STUB */ 
    int     num; //��ǰ��ȡ���˵ڼ����ڴ�
};


ngx_int_t ngx_event_pipe(ngx_event_pipe_t *p, ngx_int_t do_write);
ngx_int_t ngx_event_pipe_copy_input_filter(ngx_event_pipe_t *p, ngx_buf_t *buf);
ngx_int_t ngx_event_pipe_add_free_buf(ngx_event_pipe_t *p, ngx_buf_t *b);


#endif /* _NGX_EVENT_PIPE_H_INCLUDED_ */
