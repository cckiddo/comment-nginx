
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_POSTED_H_INCLUDED_
#define _NGX_EVENT_POSTED_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

/*
post�¼����еĲ�������
�����������������������������������������������������ש������������������������������ש�������������������������������������
��    ������                                        ��    ��������                  ��    ִ������                        ��
�ǩ��������������������������������������������������贈�����������������������������贈������������������������������������
��  ngx_locked_post_event(ev,                       ��  ev��Ҫ��ӵ�post�¼�����    ��  ��queue�¼�����������¼�ev��ע   ��
��queue)                                            �����¼���queue��post�¼�����   ���⣬ev�����뵽�¼����е��ײ�        ��
�ǩ��������������������������������������������������贈�����������������������������贈������������������������������������
��                                                  ��                              ��  �̰߳�ȫ����queue�¼����������   ��
��                                                  ��    ev��Ҫ��ӵ�post���е���  ���¼�ev����Ŀǰ��ʹ�ö��̵߳����    ��
��ngx_post_event(ev, queue)                         ��                              ��                                    ��
��                                                  ������queue��post�¼�����       ���£�����ngx_locked_post_event�Ĺ��� ��
��                                                  ��                              ������ͬ��                            ��
�ǩ��������������������������������������������������贈�����������������������������贈������������������������������������
��                                                  ��    ev��Ҫ��ĳ��post�¼�����  ��  ���¼�ev����������post�¼�����    ��
��ngx_delete_posted_event(ev)                       ��                              ��                                    ��
��                                                  ���Ƴ����¼�                    ����ɾ��                              ��
�ǩ��������������������������������������������������贈�����������������������������贈������������������������������������
��                                                  ��  cycle�ǽ��̵ĺ��Ľṹ��     ��                                    ��
��   void ngx_event_process_posted                  ��ngx_cycle_t��ָ�룮posted��Ҫ ��  ����posted�¼������������¼�      ��
��(ngx_cycle_t *cycle,ngx_thread_                   ��������post�¼����У�����ȡֵ  ����handler�ص�������ÿ���¼�������   ��
��volatile ngx_event_t **posted);                   ��Ŀǰ������Ϊngx_posted_events ��handler�����󣬾ͻ��posted�¼����� ��
��                                               I  ��                              ����ɾ��                              ��
��                                                  ������ngx_posted_accept_events  ��                                    ��
�����������������������������������������������������ߩ������������������������������ߩ�������������������������������������
*/
#define ngx_post_event(ev, q)                                                 \
                                                                              \
    if (!(ev)->posted) {                                                      \
        (ev)->posted = 1;                                                     \
        ngx_queue_insert_tail(q, &(ev)->queue);                               \
                                                                              \
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0, "post event %p", ev);\
                                                                              \
    } else  {                                                                 \
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0,                      \
                       "update posted event %p", ev);                         \
    }


#define ngx_delete_posted_event(ev)                                           \
                                                                              \
    (ev)->posted = 0;                                                         \
    ngx_queue_remove(&(ev)->queue);                                           \
                                                                              \
    ngx_log_debug1(NGX_LOG_DEBUG_CORE, (ev)->log, 0,                          \
                   "delete posted event %p", ev);



void ngx_event_process_posted(ngx_cycle_t *cycle, ngx_queue_t *posted);


extern ngx_queue_t  ngx_posted_accept_events;
extern ngx_queue_t  ngx_posted_events;


#endif /* _NGX_EVENT_POSTED_H_INCLUDED_ */
