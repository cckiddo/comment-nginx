
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t; //ngx_list_part_tֻ���������һ��Ԫ��

//����ͼ�λ���<<�������nginx>> 3.2.3��ngx_list_t���ݽṹ
//�ڴ����ο�ngx_list_push
struct ngx_list_part_s { //ngx_list_part_tֻ���������һ��Ԫ��   ���ݲ����ܵĿռ��СΪsize * nalloc�ֽ�
    void             *elts; //ָ���������ʼ��ַ��
    ngx_uint_t        nelts; //���鵱ǰ��ʹ���˶�������  ��ʾ�������Ѿ�ʹ���˶��ٸ�Ԫ�ء���Ȼ��nelts����С��ngx_list_t �ṹ���е�nalloc��
    ngx_list_part_t  *next; //��һ������Ԫ��ngx_list_part_t�ĵ�ַ��
};

/* ngx_list_t��ngx_queue_t��ȴ������:ngx_list_t��Ҫ���������ڳ�Ա�ڵ��ڴ���䣬��ngx_queue_t����Ҫ */
//�÷�����������������Բο�//ngx_http_request_s->headers_in.headers��������Բο�����ngx_http_fastcgi_create_request
typedef struct { //ngx_list_t������������
    ngx_list_part_t  *last; //ָ����������һ������Ԫ�ء�
    ngx_list_part_t   part; //������׸�����Ԫ�ء� part����ָ�������飬ͨ��part->next��ָ��ǰ�������ڵ���һ�������ͷ��
    /*
    �����е�ÿ��ngx_list_part_tԪ�ض���һ�����顣��Ϊ����洢����ĳ�����͵����ݽṹ����ngx_list_t �Ƿǳ��������ݽṹ���������������ƴ洢
    ʲô�������ݣ�ֻ��ͨ��size����ÿһ������Ԫ�ص�ռ�õĿռ��С��Ҳ�����û�Ҫ�洢��һ��������ռ�õ��ֽ�������С�ڻ����size��
    */ //size��������Ԫ���е�ÿ����Ԫ�صĴ�С�����ô��
    size_t            size;  //����list��ʱ����ngx_list_create->ngx_list_init����Ҫ�ƶ�n��size��С
    ngx_uint_t        nalloc; //���������Ԫ��һ��������ǲ��ɸ��ĵġ�nalloc��ʾÿ��ngx_list_part_t����������������ɴ洢���ٸ����ݡ�
    ngx_pool_t       *pool; //�����й����ڴ������ڴ�ض����û�Ҫ��ŵ�����ռ�õ��ڴ涼����pool����ģ������л���ϸ���ܡ�
} ngx_list_t;

//n size�ֱ��Ӧngx_list_t�е�size��nalloc
ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size) //n size�ֱ��Ӧngx_list_t�е�size��nalloc
{
    list->part.elts = ngx_palloc(pool, n * size); 
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/* ����list����
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
