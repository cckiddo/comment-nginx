
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_TIMER_H_INCLUDED_
#define _NGX_EVENT_TIMER_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


#define NGX_TIMER_INFINITE  (ngx_msec_t) -1

#define NGX_TIMER_LAZY_DELAY  300


ngx_int_t ngx_event_timer_init(ngx_log_t *log);
ngx_msec_t ngx_event_find_timer(void);
void ngx_event_expire_timers(void);
void ngx_event_cancel_timers(void);


extern ngx_rbtree_t  ngx_event_timer_rbtree;


static ngx_inline void
ngx_event_del_timer(ngx_event_t *ev, const char *func, unsigned int line)
{//ngx_del_timer
    char tmpbuf[128];

    snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
    ngx_log_debug3(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "%s event timer del: %d: %M", tmpbuf,
                    ngx_event_ident(ev->data), ev->timer.key);
                    
    ngx_rbtree_delete(&ngx_event_timer_rbtree, &ev->timer);

#if (NGX_DEBUG)
    ev->timer.left = NULL;
    ev->timer.right = NULL;
    ev->timer.parent = NULL;
#endif

    ev->timer_set = 0;
}

/*
1.ngx_event_s��������ͨ��epoll��д�¼�(�ο�ngx_event_connect_peer->ngx_add_conn����ngx_add_event)��ͨ����д�¼�����

2.Ҳ��������ͨ��ʱ���¼�(�ο�ngx_cache_manager_process_handler->ngx_add_timer(ngx_event_add_timer))��ͨ��ngx_process_events_and_timers�е�
epoll_wait���أ������Ƕ�д�¼��������أ�Ҳ��������Ϊû��ȡ�����������Ӷ��ȴ�0.5s�������»�ȡ���������¼���ִ�г�ʱ�¼��������¼������ж϶�
ʱ�������еĳ�ʱ�¼�����ʱ��ִ�дӶ�ָ��event��handler��Ȼ���һ��ָ���Ӧr����u��->write_event_handler  read_event_handler

3.Ҳ���������ö�ʱ��expirtʵ�ֵĶ�д�¼�(�ο�ngx_http_set_write_handler->ngx_add_timer(ngx_event_add_timer)),�������̼�2��ֻ����handler�в���ִ��write_event_handler  read_event_handler
*/


//ngx_event_expire_timers��ִ��ev->handler
//��ngx_process_events_and_timers�У������¼�ʹepoll_wait���أ����ִ�г�ʱ�Ķ�ʱ��
//ע�ⶨʱ���ĳ�ʱ������һ������timerʱ�䳬ʱ����ʱ������Ϊtimer_resolution�����û������timer_resolution��ʱ��������Զ����ʱ����Ϊepoll_wait�����أ��޷�����ʱ��
static ngx_inline void
ngx_event_add_timer(ngx_event_t *ev, ngx_msec_t timer, const char *func, unsigned int line)
{//ngx_add_timer
    ngx_msec_t      key;
    ngx_msec_int_t  diff;
    char tmpbuf[128];

    key = ngx_current_msec + timer;
    
    if (ev->timer_set) { //���֮ǰ��ev�Ѿ���ӹ������Ȱ�֮ǰ��ev��ʱ��del����Ȼ�����������

        /*
         * Use a previous timer value if difference between it and a new
         * value is less than NGX_TIMER_LAZY_DELAY milliseconds: this allows
         * to minimize the rbtree operations for fast connections.
         */

        diff = (ngx_msec_int_t) (key - ev->timer.key);

        if (ngx_abs(diff) < NGX_TIMER_LAZY_DELAY) {
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
            ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                           "%s event timer: %d, old: %M, new: %M, ", tmpbuf,
                            ngx_event_ident(ev->data), ev->timer.key, key);
            return;
        }

        ngx_del_timer(ev, NGX_FUNC_LINE);
    }

    ev->timer.key = key;
    snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> ", func, line);
    ngx_log_debug4(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "%s event timer add fd:%d, expire-time:%M s, timer.key:%M", tmpbuf,
                    ngx_event_ident(ev->data), timer / 1000, ev->timer.key);

    ngx_rbtree_insert(&ngx_event_timer_rbtree, &ev->timer);

    ev->timer_set = 1;
}


#endif /* _NGX_EVENT_TIMER_H_INCLUDED_ */
