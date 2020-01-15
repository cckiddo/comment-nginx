
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_RADIX_TREE_H_INCLUDED_
#define _NGX_RADIX_TREE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_RADIX_NO_VALUE   (uintptr_t) -1

typedef struct ngx_radix_node_s  ngx_radix_node_t;

//�ο�<�������nginx> ͼ7-9
struct ngx_radix_node_s {
    ngx_radix_node_t  *right; //ָ�������������û������������ֵΪnull��ָ��
    ngx_radix_node_t  *left; //ָ�������������û������������ֵΪnull��ָ��
    ngx_radix_node_t  *parent; //ָ�򸸽ڵ㣬���û�и��ڵ㣬������ڵ㣩ֵΪnull��ָ��
    uintptr_t          value; //value�洢����ָ���ֵ����ָ���û���������ݽṹ���������ڵ㻹δʹ�ã�value��ֵ����NGX_RADIX_NO_VALUE
};

/*
ÿ��ɾ��1���ڵ�ʱ��ngx_radix_treej�������������ͷ�����ڵ�ռ�õ��ڴ棬���ǰ�����ӵ�free�������С�������������µĽڵ�ʱ������
�Ȳ鿴free���Ƿ��нڵ㣬���free����δʹ�õĽڵ㣬�������ʹ�ã����û�У��ͻ��ٴ�pool�ڴ���з������ڴ�洢�ڵ㡣

����ngx_radix_tree-t�ṹ����˵������ʹ�õĽǶ����������ǲ���Ҫ�˽�pool��free��start��size��Щ��Ա�����壬���˽����ʹ��root���ڵ㼴�ɡ�
*/
typedef struct {
    ngx_radix_node_t  *root;  //ָ����ڵ�
    ngx_pool_t        *pool;  //�ڴ�أ���������������Ľڵ�����ڴ�
    ngx_radix_node_t  *free;  //�����Ѿ����䵫��ʱδʹ�ã��������У��Ľڵ㣬freeʵ���������в������нڵ�ĵ�����
    char              *start; //�ѷ����ڴ��л�δʹ���ڴ���׵�ַ
    size_t             size;  //�ѷ����ڴ��л�δʹ�õ��ڴ��С
} ngx_radix_tree_t;


ngx_radix_tree_t *ngx_radix_tree_create(ngx_pool_t *pool,
    ngx_int_t preallocate);

ngx_int_t ngx_radix32tree_insert(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask, uintptr_t value);
ngx_int_t ngx_radix32tree_delete(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask);
uintptr_t ngx_radix32tree_find(ngx_radix_tree_t *tree, uint32_t key);

#if (NGX_HAVE_INET6)
ngx_int_t ngx_radix128tree_insert(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask, uintptr_t value);
ngx_int_t ngx_radix128tree_delete(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask);
uintptr_t ngx_radix128tree_find(ngx_radix_tree_t *tree, u_char *key);
#endif


#endif /* _NGX_RADIX_TREE_H_INCLUDED_ */
