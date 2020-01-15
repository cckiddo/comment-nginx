
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


 
#ifndef _NGX_HTTP_CACHE_H_INCLUDED_
#define _NGX_HTTP_CACHE_H_INCLUDED_

/*
�����漰���:
?http://blog.csdn.net/brainkick/article/details/8535242 
?http://blog.csdn.net/brainkick/article/details/8570698 
?http://blog.csdn.net/brainkick/article/details/8583335 
?http://blog.csdn.net/brainkick/article/details/8592027 
?http://blog.csdn.net/brainkick/article/details/39966271 
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define NGX_HTTP_CACHE_MISS          1
//xxx_cache_bypass ����ָ�����ʹ����ȶ������������ƹ��������ݣ�������Щ�������Ӧ������Ȼ���Ա� upstream ģ�黺�档 
#define NGX_HTTP_CACHE_BYPASS        2 //˵�����ܴӻ����ȡ��Ӧ�ôӺ�˻�ȡ ͨ��xxx_cache_bypass�����ж�ngx_http_test_predicates(r, u->conf->cache_bypass)
#define NGX_HTTP_CACHE_EXPIRED       3  //�������ݹ��ڣ���ǰ������Ҫ���������µ���Ӧ���ݡ�
//��ngx_http_file_cache_open->ngx_http_file_cache_read��Ȼ����ngx_http_upstream_cache���u->cache_status = NGX_HTTP_CACHE_EXPIRED;
#define NGX_HTTP_CACHE_STALE         4 //��ʾ�Լ��ǵ�һ�����ָû�����ڵĿͻ�����������Լ���Ҫ�Ӻ�˴��»�ȡ
#define NGX_HTTP_CACHE_UPDATING      5 //�������ݹ��ڣ�ͬʱ����ͬ��ʹ�øû���ڵ���������������������µ���Ӧ���ݡ������������ȡ�������ݺ��Լ��ڶ������ļ���ͻ��˷���
#define NGX_HTTP_CACHE_REVALIDATED   6
#define NGX_HTTP_CACHE_HIT           7 //������������
/*
�򻺴�ڵ㱻��ѯ������δ�� `min_uses`���Դ�������û�����Ƽ������������ǲ��ٻ�������Ӧ���� (`u->cacheable = 0`)��
case NGX_HTTP_CACHE_SCARCE: (����ngx_http_upstream_cache)
        u->cacheable = 0;

*/
#define NGX_HTTP_CACHE_SCARCE        8

#define NGX_HTTP_CACHE_KEY_LEN       16
#define NGX_HTTP_CACHE_ETAG_LEN      42
#define NGX_HTTP_CACHE_VARY_LEN      42

#define NGX_HTTP_CACHE_VERSION       3


typedef struct { //�����ռ�͸�ֵ��ngx_http_file_cache_valid_set_slot
    ngx_uint_t                       status; //2XX 3XX 4XX 5XX�ȣ����Ϊ0��ʾproxy_cache_valid any 3m;
    time_t                           valid; //proxy_cache_valid xxx 4m;�е�4m
} ngx_http_cache_valid_t;

//�ṹ�� ngx_http_file_cache_node_t ������̻����ļ����ڴ��е�������Ϣ 
//һ��cache�ļ���Ӧһ��node�����node����Ҫ������cache ��key��uniq�� uniq��Ҫ�ǹ����ļ�����key�����ں������

/*
Ϊ���Ӧ����������ݴ��������ļ��øú�����ȡ�����ļ������ͻ������������Ҳ�ǲ��øú�����ȡ�����ļ�����ֻҪ
proxy_cache_key $scheme$proxy_host$request_uri�����еı�����Ӧ��ֵһ�������ȡ�����ļ����϶���һ���ģ���ʹ�ǲ�ͬ�Ŀͻ���r���ο�ngx_http_file_cache_name
��Ϊ��ͬ�ͻ��˵�proxy_cache_key���õĶ�Ӧ����valueһ���������Ǽ��������ngx_http_cache_s->key[]Ҳ��һ�������ǵ��ں������queue�����е�
node�ڵ�Ҳ����ͬһ�����ο�ngx_http_file_cache_lookup  
*/

/*
   ͬһ���ͻ�������rֻӵ��һ��r->ngx_http_cache_t��r->ngx_http_cache_t->ngx_http_file_cache_t�ṹ��ͬһ���ͻ��˿��ܻ������˵Ķ��uri��
   �������˷�������ǰ����ngx_http_file_cache_open->ngx_http_file_cache_exists�лᰴ��proxy_cache_key $scheme$proxy_host$request_uri���������
   MD5��������Ӧ�ĺ�����ڵ㣬Ȼ����ӵ�ngx_http_file_cache_t->sh->rbtree������С�
*/

/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s(ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   )��ngx_expire_old_cached_files����ʧЧ�ж�, 
�����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)ngx_http_file_cache_node_t(ngx_http_file_cache_s->sh�еĳ�Ա)��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/

//�ýṹΪʲô�ܴ���һ�������ļ�? ��Ϊngx_http_file_cache_node_t�е�node+key[]����һ����Ӧ�Ļ����ļ���Ŀ¼f/27/46492fbf0d9d35d3753c66851e81627f�е�46492fbf0d9d35d3753c66851e81627f��ע��f/27������β�����ֽ�
//�ýṹʽ������ڵ㣬����ӵ�ngx_http_file_cache_t->sh->rbtree��������Լ�ngx_http_file_cache_t->sh->queue������
typedef struct { //ngx_http_file_cache_add�д��� //ngx_http_file_cache_exists�д����ռ�͸�ֵ    
    ngx_rbtree_node_t                node; /* �����ѯ���Ľڵ� */ //node���Ǳ�ngx_http_file_cache_node_t�ṹ��ǰ��ngx_rbtree_node_t���ֽ�
    ngx_queue_t                      queue; /* LRUҳ���û��㷨 �����еĽڵ� */
    
    //�ο�ngx_http_file_cache_exists���洢����ngx_http_cache_t->key�еĺ���һЩ�ֽ�
    u_char                           key[NGX_HTTP_CACHE_KEY_LEN
                                         - sizeof(ngx_rbtree_key_t)]; 

    //ngx_http_file_cache_exists�е�һ�δ�����ʱ��Ĭ��Ϊ1  ngx_http_file_cache_update���1��
    //ngx_http_upstream_finalize_request->ngx_http_file_cache_freeҲ���1  ngx_http_file_cache_exists�м�1����ʾ�ж��ٸ��ͻ��������ڻ�ȡ�û���
    unsigned                         count:20;    /* ���ü��� */ //�ο�
    //ֻ����������������ngx_http_file_cache_exists�м�1����ʾ�ܹ��ж��ٸ��ͻ�������û��棬��ʹ�͸ÿͻ������ӶϿ�Ҳ��������1����
    unsigned                         uses:10;    /* �������ѯ���Ĵ��� */     //����������ʹ��  ngx_http_file_cache_existsû���ҵ�һ������һ��
/*
valid_sec , valid_msec �C �������ݵĹ���ʱ�䣬�������ݹ��ں󱻲�ѯ ʱ���� ngx_http_file_cache_read ���� NGX_HTTP_CACHE_STALE ��
Ȼ����fastcgi_cache_use_stale����ָ������Ƿ񼰺��������ʹ�ù������ݡ� 
*/
    unsigned                         valid_msec:10;
    /*
    �������Ӧ�� >= NGX_HTTP_SPECIAL_RESPONSE , ���Ҵ���fastcgi_intercept_errors ���ã�ͬʱ fastcgi_cache_valid ����ָ��� 
error_page ����ָ��Ҳ�Ը���Ӧ�������趨������£����ֶμ�¼��Ӧ�룬 ���е�valid_sec�ֶμ�¼����Ӧ��ĳ���ʱ�䡣����error�ڵ㲢���� 
Ӧʵ�ʵĻ����ļ���
     */
    unsigned                         error:10;
/*
�û���ڵ��Ƿ��ж�Ӧ�Ļ����ļ����´����Ļ���ڵ���߹��ڵ�error�ڵ� (�μ�error�ֶΣ���error������0ʱ��Nginx ���Ҳ�� 
���ٹ��ĸýڵ��exists�ֶ�ֵ) ���ֶ�ֵΪ0���������ڵ�(error����0) ��existsΪ0ʱ������cache lock ģʽ�� 
*/ //ֻ�пͻ��������uri�����ﵽProxy_cache_min_uses 3�е�3�βŻ���1����ngx_http_file_cache_exists����Ϊ���û�дﵽ3�Σ���u->cachable = 0
    //��ʾ�û����ļ��Ƿ���ڣ�Proxy_cache_min_uses 3�����3�κ�ʼ��ȡ������ݣ���ȡ��Ϻ���ngx_http_file_cache_update����1
    unsigned                         exists:1;//�Ƿ���ڶ�Ӧ��cache�ļ���
//updating �C �������ݹ��ڣ�ĳ���������ڻ�ȡ��Ч�ĺ����Ӧ�����´˻���ڵ㡣�μ� ngx_http_cache_t::updating �� 
    unsigned                         updating:1; //�ͻ�������nginx�󣬷��ֻ�����ڣ�������´Ӻ�˻�ȡ���ݣ�updating��1����ngx_http_file_cache_read
    //�ο�ngx_http_file_cache_delete
    unsigned                         deleting:1;     /* ���ڱ������� */     //�Ƿ�����ɾ��
                                     /* 11 unused bits */
    //�ļ�inode�ڵ�ţ�
    ngx_file_uniq_t                  uniq;//�ļ���uniq  ��ֵ��ngx_http_file_cache_update
    //expires �C ����ڵ�Ŀɻ���ʱ�� (������������)�� 
    time_t                           expire;//cacheʧЧʱ��  ��nodeʧЧʱ�䣬�ο�ngx_http_file_cache_exists
    time_t                           valid_sec; //����cache control�е�max-age
    size_t                           body_start; //��ʵӦ����body��С  ��ֵ��ngx_http_file_cache_update
    off_t                            fs_size; //�ļ���С   ��ֵ��ngx_http_file_cache_update
    ngx_msec_t                       lock_time;
} ngx_http_file_cache_node_t;

//�ο�: nginx proxy cache����  http://blog.csdn.net/xiaolang85/article/details/38260041
//�ο�:nginx proxy cache��ʵ��ԭ�� http://blog.itpub.net/15480802/viewspace-1421409/
/*
�����Ӧ�Ļ�����Ŀ��������Ϣ (����ʹ�õĻ��� file_cache ��������Ŀ��Ӧ�Ļ���ڵ���Ϣ node �������ļ� file ��key ֵ������� crc32 �ȵ�) 
����ʱ������ngx_http_cache_t(r->cache) �ṹ���У�����ṹ���е���Ϣ���������൱��ngx_http_file_cache_header_t �� ngx_http_file_cache_node_t���ܺͣ� 
*/ //ngx_http_upstream_cache->ngx_http_file_cache_new�д����ռ�

/*
   ͬһ���ͻ�������rֻӵ��һ��r->ngx_http_cache_t��r->ngx_http_cache_t->ngx_http_file_cache_t�ṹ��ͬһ���ͻ��˿��ܻ������˵Ķ��uri��
   �������˷�������ǰ����ngx_http_file_cache_open->ngx_http_file_cache_exists�лᰴ��proxy_cache_key $scheme$proxy_host$request_uri���������
   MD5��������Ӧ�ĺ�����ڵ㣬Ȼ����ӵ�ngx_http_file_cache_t->sh->rbtree������С����Բ�ͬ�Ŀͻ���uri���в�ͬ��node�ڵ�����ں������
*/

/*ngx_http_upstream_init_request->ngx_http_upstream_cache �ͻ��˻�ȡ���� ���Ӧ��������ݺ���ngx_http_upstream_send_response->ngx_http_file_cache_create
�д�����ʱ�ļ���Ȼ����ngx_event_pipe_write_chain_to_temp_file�Ѷ�ȡ�ĺ������д����ʱ�ļ��������
ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update�а���ʱ�ļ�����rename(�൱��mv)��proxy_cache_pathָ��
��cacheĿ¼����
*/
struct ngx_http_cache_s {//������ngx_http_request_s->cache
    //cache file: "/var/yyz/cache_xxx/c/c1/13cc494353644acaed96a080cac13c1c"ngx_http_file_cache_name�а�path+level+key
    ngx_file_t                       file;    /* �����ļ������ṹ�� */ //cache file�Ĵ�����ngx_http_file_cache_name
    //��ʼ����ngx_http_file_cache_new  proxy_cache_key $scheme$proxy_host$request_uri
    //ngx_http_xxx_create_key��xxx_cache_key (proxy_cache_key  fastcgi_cache_key)�е����õ�value�����������浽kyes�����У�һ��ֻ����Чһ��xxx_cache_key��
    ngx_array_t                      keys; //������洢��ֵ�Ǵ�ngx_http_xxx_create_key����(����ngx_http_fastcgi_create_key)
    uint32_t                         crc32; //ngx_http_file_cache_create_key,���������keys�����е�����xxx_cache_key�����е��ַ�������crc32У��ֵ
    //proxy_cache_key  fastcgi_cache_key���õĲ����ַ����б����MD5����Ľ������ngx_http_file_cache_create_key
    //����proxy_cache_key $scheme$proxy_host$request_uri��Ϊ������������Ӧ��value����MD5����Ľ��
    u_char                           key[NGX_HTTP_CACHE_KEY_LEN];//xxx_cache_key�����ַ�������MD5�����ֵ ngx_http_file_cache_create_key
    //ngx_memcpy(c->main, c->key, NGX_HTTP_CACHE_KEY_LEN);
    u_char                           main[NGX_HTTP_CACHE_KEY_LEN];//��ȡxxx_cache_key�����ַ�������MD5�����ֵ

    //�ļ�inode�ڵ��
    ngx_file_uniq_t                  uniq; //��ֵ��ngx_http_file_cache_exists����������Դ��//�ļ���uniq  ��ֵ��ngx_http_file_cache_update
    //�ο�ngx_http_upstream_process_cache_control  ngx_http_upstream_process_accel_expires �����ɺ��Ӧ��ͷ���о���
    //������û��Я��������������ͷ����Я����ʱ�䣬��ͨ��ngx_http_file_cache_valid(fastcgi_cache_valid proxy_cache_valid xxx 4m;)��ȡʱ��
    //��ngx_http_file_cache_read�ж�ȡ�����ʱ�򣬻�ͨ��if (c->valid_sec < now) { }�жϻ����Ƿ���ڣ�
    time_t                           valid_sec; //��ֵ��ngx_http_upstream_send_response(Ҳ����fastcgi_cache_valid���õ���Чʱ��)  //ÿ��nginx�˳���ʱ������kill nginx�����ֻ����ļ���������ڣ����ɾ��
    //���Я����ͷ����"Last-Modified:XXX"��ֵ����ngx_http_upstream_process_last_modified
    time_t                           last_modified;
    
    time_t                           date;

    /*
     Etagȷ����������棺 Etag��ԭ���ǽ��ļ���Դ���һ��etagֵ��Response�������ߣ��������ٴ�����ʱ���������Etagֵ��������������
     ���ļ���Etag�Աȣ������ͬ�˾����·��ͼ��أ������ͬ���򷵻�304. HTTP/1.1304 Not Modified
     */ //etag���ü�ngx_http_set_etag
    ngx_str_t                        etag; //��˷���ͷ���� "etab:xxxx"
    ngx_str_t                        vary;//��˷��ص�ͷ���д���vary:xxx  ��ngx_http_upstream_process_vary
    u_char                           variant[NGX_HTTP_CACHE_KEY_LEN];

    /*
     ////[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header][body]
    c->header_start = sizeof(ngx_http_file_cache_header_t)
                      + sizeof(ngx_http_file_cache_key) + len + 1; //+1����Ϊkey�������и�'\N'
     */ //ʵ���ڽ��պ�˵�һ��ͷ���������Ϣ��ʱ�򣬻�Ԥ��u->buffer.pos += r->cache->header_start;�ֽڣ���ngx_http_upstream_process_header
     //��ʾ�����е�header�������ݳ���[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header]����
     //д��������װ���̲ο�:ngx_http_file_cache_set_header        ע��������body_start������
    size_t                           header_start; //��ngx_http_file_cache_create_key
    //r->cache->body_start = (u_short) (u->buffer.pos - u->buffer.start); ��ngx_http_upstream_send_response//����epoll�Ӻ�˶�ȡ���ݵĳ���,��׼������
    //c->body_start = u->conf->buffer_size; //xxx_buffer_size(fastcgi_buffer_size proxy_buffer_size memcached_buffer_size)
    //������[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header][body]��[body]ǰ�沿�ֵ��ֽڳ���
    //��˷��ص���ҳ���岿����buffer�еĴ洢λ��

    //ngx_http_upstream_send_response�е�(r->cache->body_start = (u_short) (u->buffer.pos - u->buffer.start); 
    //��˷��ص���ҳ���岿����buffer�еĴ洢λ�ã�Ҳ���ǳ�ȥ���ͷ���в��ֺ�Ŀ�ʼ��,˵����body_start���ǻ����ļ���ͷ������ز��ֵĳ���
    /*
    root@root:/var/yyz# cat cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f   ������̼�ngx_http_file_cache_set_header
     3hwhdBw
     KEY: /test2.php
     
     X-Powered-By: PHP/5.2.13
     Content-type: text/html
    //body_start����������һ���ڴ����ݳ���
    
     <Html> 
     <title>file update</title>
     <body> 
     <form method="post" action="" enctype="multipart/form-data">
     <input type="file" name="file" /> 
     <input type="submit" value="submit" /> 
     </form> 
     </body> 
     </html>

     ע�������������ʵ��8�ֽڵ�fastcgi��ʾͷ���ṹngx_http_fastcgi_header_t��ͨ��vi cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f���Կ���

 offset    0  1  2  3   4  5  6  7   8  9  a  b   c  d  e  f  0123456789abcdef
00000000 <03>00 00 00  ab 53 83 56  ff ff ff ff  2b 02 82 56  ....�S.V+..V
00000010  64 42 77 17  00 00 91 00  ce 00 00 00  00 00 00 00  dBw...........
00000020  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000030  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000040  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000050  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000060  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000070  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00000080  0a 4b 45 59  3a 20 2f 74  65 73 74 32  2e 70 68 70  .KEY: /test2.php
00000090  0a 01 06 00  01 01 0c 04  00 58 2d 50  6f 77 65 72  .........X-Power
000000a0  65 64 2d 42  79 3a 20 50  48 50 2f 35  2e 32 2e 31  ed-By: PHP/5.2.1
000000b0  33 0d 0a 43  6f 6e 74 65  6e 74 2d 74  79 70 65 3a  3..Content-type:
000000c0  20 74 65 78  74 2f 68 74  6d 6c 0d 0a  0d 0a 3c 48   text/html....<H
000000d0  74 6d 6c 3e  20 0d 0a 3c  74 69 74 6c  65 3e 66 69  tml> ..<title>fi
000000e0  6c 65 20 75  70 64 61 74  65 3c 2f 74  69 74 6c 65  le update</title
000000f0  3e 0d 0a 3c  62 6f 64 79  3e 20 0d 0a  3c 66 6f 72  >..<body> ..<for

 offset    0  1  2  3   4  5  6  7   8  9  a  b   c  d  e  f  0123456789abcdef
00000100  6d 20 6d 65  74 68 6f 64  3d 22 70 6f  73 74 22 20  m method="post"
00000110  61 63 74 69  6f 6e 3d 22  22 20 65 6e  63 74 79 70  action="" enctyp
00000120  65 3d 22 6d  75 6c 74 69  70 61 72 74  2f 66 6f 72  e="multipart/for
00000130  6d 2d 64 61  74 61 22 3e  0d 0a 3c 69  6e 70 75 74  m-data">..<input
00000140  20 74 79 70  65 3d 22 66  69 6c 65 22  20 6e 61 6d   type="file" nam
00000150  65 3d 22 66  69 6c 65 22  20 2f 3e 20  0d 0a 3c 69  e="file" /> ..<i
00000160  6e 70 75 74  20 74 31 31  31 31 31 31  31 31 31 31  nput t1111111111
00000170  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
00000180  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
00000190  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
000001a0  31 31 31 31  31 31 31 31  31 31 31 31  31 79 70 65  1111111111111ype
000001b0  3d 22 73 75  62 6d 69 74  22 20 76 61  6c 75 65 3d  ="submit" value=
000001c0  22 73 75 62  6d 69 74 22  20 2f 3e 20  0d 0a 3c 2f  "submit" /> ..</
000001d0  66 6f 72 6d  3e 20 0d 0a  3c 2f 62 6f  64 79 3e 20  form> ..</body>
000001e0  0d 0a 3c 2f  68 74 6d 6c  3e 20 0d 0a               ..</html> ..


header_start: [ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"] Ҳ��������ĵ�һ�к͵ڶ���
body_start: [ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header]Ҳ��������ĵ�һ������������
���:body_start = header_start + [header]����(����fastcgi���ص�ͷ���б�ʶ����)
     */ 
     //body_start���������ͷ����ز��ֵĳ��ȣ�������[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header][body]��[body]ǰ�沿�ֵ��ֽڳ���
    //�ͻ��˻�ȡ�����ļ���ǰ��[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header]�����ں���ngx_http_file_cache_open��

    size_t                           body_start;//д������ͷ�����ݷ�װ���̲ο�:ngx_http_file_cache_set_header
    
    off_t                            length;//�����ļ��Ĵ�С����ngx_http_file_cache_open
    off_t                            fs_size;//�ļ�ngx_http_file_cache_t->bsize�ֽڶ��룬��ngx_http_file_cache_open
//c->min_uses = u->conf->cache_min_uses; //Proxy_cache_min_uses number Ĭ��Ϊ1�����ͻ��˷�����ͬ����ﵽ�涨������nginx�Ŷ���Ӧ���ݽ��л��棻
    ngx_uint_t                       min_uses;
    ngx_uint_t                       error;
    ngx_uint_t                       valid_msec;

    ngx_buf_t                       *buf;/* �洢�����ļ�ͷ */ //ngx_http_file_cache_open�д����ռ�
    

    ngx_http_file_cache_t           *file_cache;//ͨ��ngx_http_upstream_cache->ngx_http_upstream_cache_get��ȡ
    //ngx_http_file_cache_node_t  �����ȡ����(�´������߱�����ѯ�õ���)ngx_http_file_cache_node_t����ngx_http_file_cache_exists
    //�ڻ�ȡ�������ǰ�����Ȼ����һ����Ƿ��л�����������ݣ����û�У������ngx_http_file_cache_open�д���node,Ȼ�����ȥ��˻�ȡ����
    ngx_http_file_cache_node_t      *node; //ngx_http_file_cache_exists�д����ռ�͸�ֵ

#if (NGX_THREADS)
//ngx_http_file_cache_aio_read->ngx_thread_read�д����ռ�͸�ֵ
    ngx_thread_task_t               *thread_task;
#endif

    ngx_msec_t                       lock_timeout; //�ο������lock  //proxy_cache_lock_timeout ���ã�Ĭ��5S
    ngx_msec_t                       lock_age; //XXX_cache_lock_age  proxy_cache_lock_age
    ngx_msec_t                       lock_time;
    ngx_msec_t                       wait_time;

    ngx_event_t                      wait_event;

/*
�����Ҫ���һ������: //proxy_cache_lock Ĭ��off 0  //proxy_cache_lock_timeout ���ã�Ĭ��5S
���������������ͻ��ˣ�һ���ͻ������ڻ�ȡ������ݣ����Һ�˷�����һ���֣���nginx�Ỻ����һ���֣����ҵȴ����к�����ݷ��ؼ������档
�����ڻ���Ĺ���������ͻ���2ҳ������ȥͬ��������uri�ȶ�һ�������ȥ���ͻ��˻���һ������ݣ���ʱ��Ϳ���ͨ�������������������⣬
Ҳ���ǿͻ���1��û������ȫ�����ݵĹ����пͻ���2ֻ�еȿͻ���1��ȡ��ȫ��������ݣ����߻�ȡ��proxy_cache_lock_timeout��ʱ����ͻ���2ֻ�дӺ�˻�ȡ����
*/
    unsigned                         lock:1;//c->lock = u->conf->cache_lock;
    /* 
     �������ݼ����ڣ���ǰ�������ȴ�����������´˻���ڵ㡣 ע������ͬһ���ͻ���r����ͬһ���ͻ����ڻ�ȡ������ݵĹ�����(������ݻ�û����)���ַ���һ��get����
     */
     /* ngx_http_file_cache_open�������NGX_AGAIN������ں�����ִ������Ĵ��룬Ҳ���ǵȴ�ǰ��������˷��غ��ٴδ������������ִ��ngx_http_upstream_init_request����
        ��ʱ��ǰ��Ӻ�˻�ȡ�����ݿ϶��Ѿ��õ�����
        r->write_event_handler = ngx_http_upstream_init_request;  //��ô������write handler��?��Ϊǰ��������ȡ��������ݺ��ڴ���epoll_in��ͬʱ
        Ҳ�ᴥ��epoll_out���Ӷ���ִ�иú���
        return;  
     */
    unsigned                         waiting:1; //ngx_http_file_cache_lock����1

    unsigned                         updated:1;
/* updating �C �������ݼ����ڣ����ҵ�ǰ�������ڻ�ȡ��Ч�ĺ����Ӧ�����´˻���ڵ㡣�μ� ngx_http_file_cache_node:updating �� 
���һ���ͻ����ڻ�ȡ������ݣ��п�����Ҫ�ͺ�˶��epoll read���ܻ�ȡ�꣬���ڻ�ȡ�����е�һ������δ��������ʱ��
һ����Ϊupdating�ĳ�Ա����1��ͬʱexists��Ա��0
*/  //�ͻ�������nginx�󣬷��ֻ�����ڣ�������´Ӻ�˻�ȡ���ݣ�updating��1����ngx_http_file_cache_read
    unsigned                         updating:1;
//������Ļ����Ѿ����ڣ����ҶԸû������������ﵽ�����Ҫ�����min_uses,��exists����ngx_http_file_cache_exists����1
//ֻ�пͻ��������uri�����ﵽProxy_cache_min_uses 3�е�3�βŻ���1����ngx_http_file_cache_exists����Ϊ���û�дﵽ3�Σ���u->cachable = 0

//��ʾ�û����ļ��Ƿ���ڣ�Proxy_cache_min_uses 3�����3�κ�ʼ��ȡ������ݣ���ȡ��Ϻ���ngx_http_file_cache_update����1������ֻ���ڵ�4�������ʱ��Ż���ngx_http_file_cache_exists��ֵΪ1
    unsigned                         exists:1; 
   
    unsigned                         temp_file:1;
    //ֻ�д�file aio�Ż���ngx_http_file_cache_aio_read����1����ʾ�Ѿ�֪ͨ�ں˽��ж�������ֻ�����ڻ�û�ж�ȡ��ϣ��ں���ɶ�ȡ���ͨ��epoll�¼�֪ͨ
    // ע������ͬһ���ͻ���r����ͬһ���ͻ����ڻ�ȡ������ݵĹ�����(������ݻ�û����)���ַ���һ��get����ע��ֻ��aio���и����
    unsigned                         reading:1; 
    unsigned                         secondary:1;
};

/*
ÿ���ļ�ϵͳ�еĻ����ļ����й̶��Ĵ洢��ʽ������ ngx_http_file_cache_header_tΪ��ͷ�ṹ���洢�����ļ��������Ϣ
(�޸�ʱ�䡢���� key �� crc32 ֵ��������ָ�� HTTP ��Ӧ��ͷ�Ͱ����ڻ����ļ���ƫ��λ�õ��ֶε�)��

root@root:/var/yyz/cache_xxx# cat b/7d/bf6813c2bc0becb369a8d8367b6b77db 
��oV��oVZ"  
KEY: /test.php
IX-Powered-By: PHP/5.2.13
Content-type: text/html

//��������������ļ�����
<Html> 
<Head> 
<title>Your page Subject and domain name</title>

<Meta NAME="" CONTENT="">
"" your others meta tagB
"" your others meta tag
"" your others meta tag
"" your others meta tag
"" your others meta tag
"" your others meta tag
"" your others meta tag
*/ 
//ʵ���ڽ��պ�˵�һ��ͷ���������Ϣ��ʱ�򣬻�Ԥ��u->buffer.pos += r->cache->header_start;�ֽڣ���ngx_http_upstream_process_header
//[ngx_http_file_cache_header_t]["\nKEY: "][orig_key]["\n"][header][body] ����ngx_http_file_cache_create_key
typedef struct { //д���ļ�ǰ��ֵ�ȼ�ngx_http_file_cache_set_header����ȡ�ļ��еĸ�ͷ���ṹ��ngx_http_file_cache_read
    ngx_uint_t                       version;
    time_t                           valid_sec;
    time_t                           last_modified;
    time_t                           date;
    uint32_t                         crc32;
    u_short                          valid_msec;
    /*
    ////[ngx_http_file_cache_header_t]["\nKEY: "][orig_key]["\n"][header][body]
    c->header_start = sizeof(ngx_http_file_cache_header_t)
                      + sizeof(ngx_http_file_cache_key) + len + 1; 
     */
    u_short                          header_start; //ָ���
     /*
    root@root:/var/yyz# cat cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f   ������̼�ngx_http_file_cache_set_header
     3hwhdBw
     KEY: /test2.php
     
     X-Powered-By: PHP/5.2.13
     Content-type: text/html
    //body_start����������һ���ڴ����ݳ���
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
     <Html> 
     <title>file update</title>
     <body> 
     <form method="post" action="" enctype="multipart/form-data">
     <input type="file" name="file" /> 
     <input type="submit" value="submit" /> 
     </form> 
     </body> 
     </html>
     */ 
    //������Ż����ļ���ǰ��[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key�е�KEY]["\n"][header]���ֵ����ݳ��ȿռ�,Ҳ����
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ǰ�������
    u_short                          body_start;
    u_char                           etag_len;
    /*
     Etagȷ����������棺 Etag��ԭ���ǽ��ļ���Դ���һ��etagֵ��Response�������ߣ��������ٴ�����ʱ���������Etagֵ��������������
     ���ļ���Etag�Աȣ������ͬ�˾����·��ͼ��أ������ͬ���򷵻�304. HTTP/1.1304 Not Modified
     */ //etag���ü�ngx_http_set_etag
    u_char                           etag[NGX_HTTP_CACHE_ETAG_LEN];
    u_char                           vary_len;
    u_char                           vary[NGX_HTTP_CACHE_VARY_LEN];
    u_char                           variant[NGX_HTTP_CACHE_KEY_LEN];
} ngx_http_file_cache_header_t;

//ngx_http_file_cache_init�����ռ�ͳ�ʼ��    
//ngx_http_file_cache_s->sh��Ա���Ǹýṹ

/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s��ngx_expire_old_cached_files����ʧЧ�ж�, �����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)
ngx_http_file_cache_node_t��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/

/*���е�ngx_http_file_cache_node_t������ӵ������rbtree������⣬������ӵ�����queue�У���������ڰ���key�����Ҷ�Ӧ��node�ڵ㣬�ο�
    ngx_http_file_cache_lookup��queue���ڿ��ٻ�ȡ������ӵ�queue���˺�������queue���˵�node�ڵ�����ɾ�����µȣ��ο�ngx_http_file_cache_expire*/
typedef struct { //���ڱ��滺��ڵ� �� ����ĵ�ǰ״̬ (�Ƿ����ڴӴ��̼��ء���ǰ�����С��)��
    //��ngx_http_cache_t->key�ַ����е���ǰ��4�ֽ�Ϊkey���ں�����б�������ngx_http_file_cache_lookup
    ngx_rbtree_t                     rbtree; //�������ʼ����ngx_http_file_cache_init
    //�������ʼ����ngx_http_file_cache_init
    ngx_rbtree_node_t                sentinel; //sentinel�ڱ������ⲿ�ڵ㣬���е�Ҷ���Լ������ĸ��ڵ㣬��ָ�����Ψһ���ڱ�nil���ڱ�����ɫΪ��ɫ
    /*���е�ngx_http_file_cache_node_t������ӵ������rbtree������⣬������ӵ�����queue�У���������ڰ���key�����Ҷ�Ӧ��node�ڵ㣬�ο�
    ngx_http_file_cache_lookup��queue���ڿ��ٻ�ȡ������ӵ�queue���˺�������queue���˵�node�ڵ�����ɾ�����µȣ��ο�ngx_http_file_cache_expire*/
    ngx_queue_t                      queue;//���г�ʼ����ngx_http_file_cache_init��
    //cold��ʾ���cache�Ƿ��Ѿ���loader����load����(ngx_cache_loader_process_handler->ngx_http_file_cache_loader)  
    //ngx_http_file_cache_loader��0����ʾ�����ļ��Ѿ�������ϣ�ngx_http_file_cache_initĬ�ϳ�ʼ������1��
    //test=0,��ʾ���������󻺴��ļ��Ѿ�������ϣ�Ϊ1��ʾ���̸�������û�м��ػ����ļ���Ĭ��ֵ1
    ngx_atomic_t                     cold;  /* �����Ƿ���� (�������) */ //����������һ��60s��ʼ���ػ���Ŀ¼����
    ngx_atomic_t                     loading;  /* �Ƿ����ڱ� loader ���̼��� */ //����load���cache  loader����pid����ngx_http_file_cache_loader
    //�����ļ��ܴ�С�����ļ��ϻ�ɾ����size���ȥɾ���ⲿ�ִ�С����ngx_http_file_cache_delete
    off_t                            size;    /* ��ʼ��Ϊ 0 */ //ռ���˻���ռ���ܴ�С����ֵ��ngx_http_file_cache_update  
} ngx_http_file_cache_sh_t; //ע��ngx_http_file_cache_sh_t��ngx_open_file_cache_t������
//��������²ο�:����������漰��ʵ��(һ  ��  ��) http://blog.csdn.net/brainkick/article/details/8535242

/*ngx_http_upstream_init_request->ngx_http_upstream_cache �ͻ��˻�ȡ���� ���Ӧ��������ݺ���ngx_http_upstream_send_response->ngx_http_file_cache_create
�д�����ʱ�ļ���Ȼ����ngx_event_pipe_write_chain_to_temp_file�Ѷ�ȡ�ĺ������д����ʱ�ļ��������
ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update�а���ʱ�ļ�����rename(�൱��mv)��proxy_cache_pathָ��
��cacheĿ¼����
*/

/*
�����ļ�stat״̬��Ϣngx_cached_open_file_s(ngx_open_file_cache_t->rbtree(expire_queue)�ĳ�Ա   )��ngx_expire_old_cached_files����ʧЧ�ж�, 
�����ļ�������Ϣ(ʵʵ���ڵ��ļ���Ϣ)ngx_http_file_cache_node_t(ngx_http_file_cache_s->sh�еĳ�Ա)��ngx_http_file_cache_expire����ʧЧ�жϡ�
*/

/*
   ͬһ���ͻ�������rֻӵ��һ��r->ngx_http_cache_t��r->ngx_http_cache_t->ngx_http_file_cache_t�ṹ��ͬһ���ͻ��˿��ܻ������˵Ķ��uri��
   �������˷�������ǰ����ngx_http_file_cache_open->ngx_http_file_cache_exists�лᰴ��proxy_cache_key $scheme$proxy_host$request_uri���������
   MD5��������Ӧ�ĺ�����ڵ㣬Ȼ����ӵ�ngx_http_file_cache_t->sh->rbtree������С����Բ�ͬ�Ŀͻ���uri���в�ͬ��node�ڵ�����ں������
*/

//��ȡ�ýṹngx_http_upstream_cache_get��ʵ������ͨ��proxy_cache xxx����fastcgi_cache xxx����ȡ�����ڴ�����ģ���˱�������proxy_cache����fastcgi_cache
struct ngx_http_file_cache_s { //ngx_http_file_cache_set_slot�д����ռ䣬������ngx_http_proxy_main_conf_t->caches������
    //sh ά�� LRU �ṹ���ڱ��滺��ڵ� �� ����ĵ�ǰ״̬ (�Ƿ����ڴӴ��̼��ء���ǰ�����С��)��
    ngx_http_file_cache_sh_t        *sh; //ngx_http_file_cache_init�з���ռ�
    //shpool�����ڹ������ڴ�� slab allocator �����л���ڵ�ռ�ÿռ䶼�������з���
    ngx_slab_pool_t                 *shpool;

    ngx_path_t                      *path;//ngx_http_file_cache_set_slot�д���ngx_path_t�ռ�  
    /*
Ĭ�������p->temp_file->path = u->conf->temp_path; Ҳ������ngx_http_fastcgi_temp_pathָ��·������������ǻ��淽ʽ(p->cacheable=1)��������
proxy_cache_path(fastcgi_cache_path) /a/b��ʱ�����use_temp_path=off(��ʾ��ʹ��ngx_http_fastcgi_temp_path���õ�path)��
��p->temp_file->path = r->cache->file_cache->temp_path; Ҳ������ʱ�ļ�/a/b/temp��use_temp_path=off��ʾ��ʹ��ngx_http_fastcgi_temp_path
���õ�·������ʹ��ָ������ʱ·��/a/b/temp   ��ngx_http_upstream_send_response 
*/
    //���proxy_cache_path /aa/bb use_temp_path=off ����temp_pathĬ��Ϊ/aa/bb/temp
    ngx_path_t                      *temp_path; //proxy_cache_path����use_temp_path=on   ��ֵ�ο�ngx_http_file_cache_set_slot

    off_t                            max_size;//����ռ��Ӳ�̴�С�����ô�� proxy_cache_path max_size=128m �������128M
    size_t                           bsize; //�ļ�ϵͳ��block size  ngx_http_file_cache_init�и�ֵ

    time_t                           inactive; //���û���פ��ʱ��   //proxy_cache_path inactive=30m ��ʾ��proxy_cache30 ��ʧЧ

    ngx_uint_t                       files;//��ǰ�ж��ٸ�cache�ļ�(����loader_files֮��ᱻ��0)
    //loader_files���ֵҲ����һ����ֵ����load���ļ������������ֵ֮��load���̻���ݵ�����(ʱ��λloader_sleep)
    ngx_uint_t                       loader_files;//proxy_cache_path����loader_files= ��ʾ����ж��ٸ�cache�ļ�
    ngx_msec_t                       last;//���manage����loader���ʵ�ʱ��
    //loader_files���ֵҲ����һ����ֵ����load���ļ������������ֵ֮��load���̻���ݵ�����(ʱ��λloader_sleep)
    //loader_sleep�������loader_files���ʹ�ã����ļ���������loader_files���ͻ�����
    //loader_threshold��������last��Ҳ����loader���������߼����
    
    //loader_sleep�������loader_files���ʹ�ã����ļ���������loader_files���ͻ�����
    ngx_msec_t                       loader_sleep;//proxy_cache_path����loader_sleep=
    //loader_threshold��������last��Ҳ����loader���������߼����
    ngx_msec_t                       loader_threshold;//proxy_cache_path����loader_threshold=

    //fastcgi_cache_path keys_zone=fcgi:10m;�е�keys_zone=fcgi:10mָ�������ڴ������Ѿ������ڴ�ռ��С
    ngx_shm_zone_t                  *shm_zone;
};


ngx_int_t ngx_http_file_cache_new(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_create(ngx_http_request_t *r);
void ngx_http_file_cache_create_key(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_open(ngx_http_request_t *r);
ngx_int_t ngx_http_file_cache_set_header(ngx_http_request_t *r, u_char *buf);
void ngx_http_file_cache_update(ngx_http_request_t *r, ngx_temp_file_t *tf);
void ngx_http_file_cache_update_header(ngx_http_request_t *r);
ngx_int_t ngx_http_cache_send(ngx_http_request_t *);
void ngx_http_file_cache_free(ngx_http_cache_t *c, ngx_temp_file_t *tf);
time_t ngx_http_file_cache_valid(ngx_array_t *cache_valid, ngx_uint_t status);

char *ngx_http_file_cache_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
char *ngx_http_file_cache_valid_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


extern ngx_str_t  ngx_http_cache_status[];


#endif /* _NGX_HTTP_CACHE_H_INCLUDED_ */
