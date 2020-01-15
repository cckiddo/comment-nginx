
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

//��ʱ����ͨ��һ�ú����ʵ�ֵġ�ngx_event_timer_rbtree�������ж�ʱ���¼���ɵĺ��������ngx_event_timer_sentinel������ú�������ڱ��ڵ�
/*
��ú�����е�ÿ���ڵ㶼��ngx_event_t�¼��е�timer��Ա����ngx_rbtree_node-t�ڵ�Ĺؼ��־����¼��ĳ�ʱʱ�䣬�������ʱʱ��Ĵ�С���
�˶���������ngx_event_timer rbtree�������������Ҫ�ҳ����п��ܳ�ʱ���¼�����ô��ngx_event timer- rbtree��������ߵĽڵ�ȡ�������ɡ�
ֻҪ�õ�ǰʱ��ȥ�Ƚ��������߽ڵ�ĳ�ʱʱ�䣬�ͻ�֪������¼���û�д�����ʱ�������û�д�����ʱ����ô��֪�����ٻ�Ҫ�������ٺ������㳬
ʱ������������ʱ���ȿ�һ�¶�ʱ���Ĳ�������������9-5����9-5��ʱ���Ĳ�������
�������������������������������������������������������ש��������������������������������ש�������������������������������������
��    ������                                          ��    ��������                    ��    ִ������                        ��
�ǩ����������������������������������������������������贈�������������������������������贈������������������������������������
��ngx_int_t ngx_event_timer_init                      ��  log����Լ�¼��־��ngx_log_t  ��  ��ʼ����ʱ��                      ��
��(ngx_log_t *log);                                   ������                            ��                                    ��
�ǩ����������������������������������������������������贈�������������������������������贈������������������������������������
��ngx_msec_t ngx_event_find_timer(void);              ��  ��                            ��  �ҳ������������ߵĽڵ㣬���    ��
��                                                    ��                                �����ĳ�ʱʱ����ڵ�ǰʱ�䣬Ҳ�ͱ���  ��
��                                                    ��                                ��Ŀǰ�Ķ�ʱ����û��һ���¼����㴥��  ��
��                                                    ��                                ����������ʱ���������ʱ�뵱ǰʱ���  ��
��                                     			    ��                                ����ֵ��Ҳ������Ҫ�������ٺ��������  ��
��                                                  I ��                                ������ʱ������������ĳ�ʱʱ��С�ڻ�  ��
��                                                    ��                                �����ڵ�ǰʱ�䣬�򷵻�0����ʾ��ʱ��   ��
��                                                    ��                                �����Ѿ����ڳ�ʱ��Ҫ�������¼�        ��
�ǩ����������������������������������������������������贈�������������������������������贈������������������������������������
��                                                    ��                                ��  ��鶨ʱ���е������¼������պ�    ��
��                                                    ��                                �������ؼ�����С�����˳�����ε���    ��
��ngx_event_expire_timers                             ��  ��                            ��                                    ��
��                                                    ��                                ���Ѿ����㳬ʱ������Ҫ�������¼���    ��
��                                                    ��                                ��handler�ص�����                     ��
�������������������������������������������������������ߩ��������������������������������ߩ�������������������������������������
�����������������������������������ש��������������������������������ש�����������������������������������
��    ������                      ��    ��������                    ��    ִ������                      ��
�ǩ��������������������������������贈�������������������������������贈����������������������������������
��static ngx_inline void          ��                                ��                                  ��
��ngx_event_del_timer             ��  ev����Ҫ�������¼�            ��  �Ӷ�ʱ�����Ƴ�һ���¼�          ��
��(ngx_event_t ev)                ��                                ��                                  ��
�ǩ��������������������������������贈�������������������������������贈����������������������������������
��static ngx_inline void          ��  ev����Ҫ�������¼���timer��   ��                                  ��
��ngx_event_add_timer(ngx_        ����λ�Ǻ��룬�����߶�ʱ���¼�ev  ��  ���һ����ʱ���¼�����ʱʱ��Ϊ  ��
��                                ��ϣ��timer�����ʱ��ͬʱ��Ҫ   ��timer����                         ��
��event_t *ev, ngx_msec_t timer)  ��                                ��                                  ��
��                                ���ص�ev��handler����             ��                                  ��
�����������������������������������ߩ��������������������������������ߩ�����������������������������������
    ��ʵ�ϣ�������������ngx_event add��timer������ngx_event del timer�������÷�����ȫһ���ģ�������ʾ��
#define ngx_add_timer ngx_event_add_timer
#define ngx_del_timer ngx_event_del timer
*/
ngx_rbtree_t              ngx_event_timer_rbtree;
static ngx_rbtree_node_t  ngx_event_timer_sentinel;
//�ڱ��ڵ����������²��Ҷ�ӽڵ㶼ָ��һ��NULL�սڵ㣬ͼ�λ��ο�:http://blog.csdn.net/xzongyuan/article/details/22389185

/*
 * the event timer rbtree may contain the duplicate keys, however,
 * it should not be a problem, because we use the rbtree to find
 * a minimum timer value only
 */
//��ʼ�������ʵ�ֵĶ�ʱ����
ngx_int_t
ngx_event_timer_init(ngx_log_t *log)
{
    ngx_rbtree_init(&ngx_event_timer_rbtree, &ngx_event_timer_sentinel,
                    ngx_rbtree_insert_timer_value);

    return NGX_OK;
}

//��ȡ����������ĳ�ʱ��ʱ��ʱ��
ngx_msec_t
ngx_event_find_timer(void)
{
    ngx_msec_int_t      timer;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    if (ngx_event_timer_rbtree.root == &ngx_event_timer_sentinel) {
        return NGX_TIMER_INFINITE;
    }

    root = ngx_event_timer_rbtree.root;
    sentinel = ngx_event_timer_rbtree.sentinel;

    node = ngx_rbtree_min(root, sentinel);

    timer = (ngx_msec_int_t) (node->key - ngx_current_msec);

    return (ngx_msec_t) (timer > 0 ? timer : 0);
}

/*
1.ngx_event_s��������ͨ��epoll��д�¼�(�ο�ngx_event_connect_peer->ngx_add_conn����ngx_add_event)��ͨ����д�¼�����

2.Ҳ��������ͨ��ʱ���¼�(�ο�ngx_cache_manager_process_handler->ngx_add_timer(ngx_event_add_timer))��ͨ��ngx_process_events_and_timers�е�
epoll_wait���أ������Ƕ�д�¼��������أ�Ҳ��������Ϊû��ȡ�����������Ӷ��ȴ�0.5s�������»�ȡ���������¼���ִ�г�ʱ�¼��������¼������ж϶�
ʱ�������еĳ�ʱ�¼�����ʱ��ִ�дӶ�ָ��event��handler��Ȼ���һ��ָ���Ӧr����u��->write_event_handler  read_event_handler

3.Ҳ���������ö�ʱ��expirtʵ�ֵĶ�д�¼�(�ο�ngx_http_set_write_handler->ngx_add_timer(ngx_event_add_timer)),�������̼�2��ֻ����handler�в���ִ��write_event_handler  read_event_handler
*/

void
ngx_event_expire_timers(void)
{
    ngx_event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = ngx_event_timer_rbtree.sentinel;

    for ( ;; ) {
        root = ngx_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = ngx_rbtree_min(root, sentinel);

        /* node->key > ngx_current_time */

        if ((ngx_msec_int_t) (node->key - ngx_current_msec) > 0) {
            return;
        }

        ev = (ngx_event_t *) ((char *) node - offsetof(ngx_event_t, timer));

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "event timer del: %d: %M",
                       ngx_event_ident(ev->data), ev->timer.key);

        ngx_rbtree_delete(&ngx_event_timer_rbtree, &ev->timer);

#if (NGX_DEBUG)
        ev->timer.left = NULL;
        ev->timer.right = NULL;
        ev->timer.parent = NULL;
#endif

        ev->timer_set = 0;

        ev->timedout = 1;

        ev->handler(ev); //��ʱ��ʱ�������д�¼��ص��������Ӷ��������ж�timedout��־λ
    }
}


void
ngx_event_cancel_timers(void)
{
    ngx_event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = ngx_event_timer_rbtree.sentinel;

    for ( ;; ) {
        root = ngx_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = ngx_rbtree_min(root, sentinel);

        ev = (ngx_event_t *) ((char *) node - offsetof(ngx_event_t, timer));

        if (!ev->cancelable) {
            return;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                       "event timer cancel: %d: %M",
                       ngx_event_ident(ev->data), ev->timer.key);

        ngx_rbtree_delete(&ngx_event_timer_rbtree, &ev->timer);

#if (NGX_DEBUG)
        ev->timer.left = NULL;
        ev->timer.right = NULL;
        ev->timer.parent = NULL;
#endif

        ev->timer_set = 0;

        ev->handler(ev);
    }
}
