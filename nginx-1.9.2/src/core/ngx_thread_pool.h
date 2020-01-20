
/*
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Valentin V. Bartenev
 */


#ifndef _NGX_THREAD_POOL_H_INCLUDED_
#define _NGX_THREAD_POOL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

//������ӵ���ngx_thread_pool_s->queue�����У�Ҳ������ӵ�ngx_thread_pool_s��Ӧ���̳߳ض�����
struct ngx_thread_task_s {
    ngx_thread_task_t   *next; //ָ����һ���ύ������  
    ngx_uint_t           id; //����id  û���һ������������ӣ���ngx_thread_pool_task_id
    void                *ctx; //ִ�лص������Ĳ���  
    //ngx_thread_pool_cycle��ִ��
    void               (*handler)(void *data, ngx_log_t *log); //�ص�����   ִ����handler���ͨ��ngx_notifyִ��event->handler 
    //ִ����handler���ͨ��ngx_notifyִ��event->handler 
    ngx_event_t          event; //һ�������һ���¼���Ӧ  �¼���ͨ��ngx_notify��ngx_thread_pool_handler��ִ��
};


typedef struct ngx_thread_pool_s  ngx_thread_pool_t;//һ���ýṹ��Ӧһ��threads_pool�̳߳�����

ngx_thread_pool_t *ngx_thread_pool_add(ngx_conf_t *cf, ngx_str_t *name);
ngx_thread_pool_t *ngx_thread_pool_get(ngx_cycle_t *cycle, ngx_str_t *name);

ngx_thread_task_t *ngx_thread_task_alloc(ngx_pool_t *pool, size_t size);
ngx_int_t ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task);


#endif /* _NGX_THREAD_POOL_H_INCLUDED_ */
