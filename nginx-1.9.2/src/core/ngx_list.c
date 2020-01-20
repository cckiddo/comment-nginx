
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

//n size�ֱ��Ӧngx_list_t�е�size��nalloc
ngx_list_t *
ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size) //ʵ���Ͼ���Ϊnginx_list_t��part��Ա����ָ����n*size�ռ䣬���Ҵ����˿ռ�sizeof(ngx_list_t)
{
    ngx_list_t  *list;

    list = ngx_palloc(pool, sizeof(ngx_list_t));
    if (list == NULL) {
        return NULL;
    }

    if (ngx_list_init(list, pool, n, size) != NGX_OK) {
        return NULL;
    }

    return list;
}

/*
ngx_list_tҲ��һ��˳����������ʵ�����൱�ڶ�̬�����뵥������Ľ�
���壬ֻ�����������ȶ�̬����򵥵ö࣬������һ��������1�����顣
*/
//���l�е�last��elts�����ˣ�����l�������´���һ��ngx_list_part_t����ʵ�����ݲ��ֿռ��СΪl->nalloc * l->size��
//���l��last����ʣ�࣬�򷵻�last��δ�õĿռ�
void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last;

    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */

        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}

