
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

/*
    ʵ���ϣ���������Ľ���벻��Nginx��post�¼�������ơ����post�¼���ʲô��˼�أ�����ʾ�����¼��Ӻ�ִ�С�Nginx���������post���У�һ
�����ɱ������ļ������ӵĶ��¼����ɵ�ngx_posted_accept_events���У���һ��������ͨ����д�¼����ɵ�ngx_posted_events���С�������post��
���������û����ʲô���Ĺ����أ�
   ��epoll_wait������һ���¼����ֵ������������У��ô�����������¼���ngx_posted_accept_events��������ִ�У������ͨ�¼���ngx_posted_events��
�����ִ�У����ǽ������Ⱥ���͸��ؾ�����������Ĺؼ�������ڴ���һ���¼��Ĺ����в�������һ���¼���������ϣ������¼����ִ�У���������ִ�У���
�Ϳ��԰����ŵ�post�����С�
*/
ngx_queue_t  ngx_posted_accept_events; //�Ӻ�����½�����accept�¼�
ngx_queue_t  ngx_posted_events; //��ͨ�Ӻ����ӽ����ɹ���Ķ�д�¼�

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
//��posted������ȴ������ev��ִ�и����¼���handler
void
ngx_event_process_posted(ngx_cycle_t *cycle, ngx_queue_t *posted)
{
    ngx_queue_t  *q;
    ngx_event_t  *ev; 

    while (!ngx_queue_empty(posted)) {

        q = ngx_queue_head(posted);
        ev = ngx_queue_data(q, ngx_event_t, queue);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                      "begin to run befor posted event %p", ev);

        ngx_delete_posted_event(ev);

        ev->handler(ev);
    }
}
