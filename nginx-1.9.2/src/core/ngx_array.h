
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_ARRAY_H_INCLUDED_
#define _NGX_ARRAY_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

/*cookies����ngx_array_t����洢�ģ������Ȳ�����������ݽṹ������Ȥ�Ļ�����ֱ������7.3���˽�ngx_array_t������÷�*/
//���ĳ����������nginx.conf�ļ��п��ܳ��ֶ�Σ�������������ж�̬�洢���ο�ngx_conf_set_str_array_slot
typedef struct { //����ͨ��ngx_array_create���������ռ䣬����ʼ��������Ա
    void        *elts; //������ngx_keyval_t  ngx_str_t  ngx_bufs_t ngx_hash_key_t��
    ngx_uint_t   nelts; //�Ѿ�ʹ���˶��ٸ�
    size_t       size; //ÿ��elts�Ŀռ��С��
    ngx_uint_t   nalloc; //����ж��ٸ�eltsԪ��
    ngx_pool_t  *pool; //��ֵ��ngx_init_cycle��Ϊcycle��ʱ������pool�ռ�
} ngx_array_t;

ngx_array_t *ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size);
void ngx_array_destroy(ngx_array_t *a);
void *ngx_array_push(ngx_array_t *a);
void *ngx_array_push_n(ngx_array_t *a, ngx_uint_t n);


static ngx_inline ngx_int_t
ngx_array_init(ngx_array_t *array, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = ngx_palloc(pool, n * size);
    if (array->elts == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

#endif /* _NGX_ARRAY_H_INCLUDED_ */

