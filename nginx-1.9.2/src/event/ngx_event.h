
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_INVALID_INDEX  0xd0d0d0d0


#if (NGX_HAVE_IOCP)

typedef struct {
    WSAOVERLAPPED    ovlp;
    ngx_event_t     *event;
    int              error;
} ngx_event_ovlp_t;

#endif

//cycle->read_events��cycle->write_events�����������ŵ���ngx_event_s,�����Ƕ�Ӧ�ģ���ngx_event_process_init  ngx_event_t�¼���ngx_connection_t������һһ��Ӧ��
//ngx_event_t�¼���ngx_connection_t�����Ǵ���TCP���ӵĻ������ݽṹ, ��Nginx�У�ÿһ���¼�����ngx_event_t�ṹ������ʾ

/*
1.ngx_event_s��������ͨ��epoll��д�¼�(�ο�ngx_event_connect_peer->ngx_add_conn����ngx_add_event)��ͨ����д�¼�����

2.Ҳ��������ͨ��ʱ���¼�(�ο�ngx_cache_manager_process_handler->ngx_add_timer(ngx_event_add_timer))��ͨ��ngx_process_events_and_timers�е�
epoll_wait���أ������Ƕ�д�¼��������أ�Ҳ��������Ϊû��ȡ�����������Ӷ��ȴ�0.5s�������»�ȡ���������¼���ִ�г�ʱ�¼��������¼������ж϶�
ʱ�������еĳ�ʱ�¼�����ʱ��ִ�дӶ�ָ��event��handler��Ȼ���һ��ָ���Ӧr����u��->write_event_handler  read_event_handler

3.Ҳ���������ö�ʱ��expirtʵ�ֵĶ�д�¼�(�ο�ngx_http_set_write_handler->ngx_add_timer(ngx_event_add_timer)),�������̼�2��ֻ����handler�в���ִ��write_event_handler  read_event_handler
*/

/*һ��ngx_connection_s��Ӧһ��ngx_event_s read��һ��ngx_event_s write,�����¼���fd�Ǵ�ngx_connection_s->fd��ȡ������
��ngx_worker_process_init->ngx_event_process_init�й������� */
struct ngx_event_s {
    /*
    �¼���صĶ���ͨ��data����ָ��ngx_connection_t���Ӷ���,��ngx_get_connection�������ļ��첽I/Oʱ�������ܻ�ָ��ngx_event_aio_t(ngx_file_aio_init)�ṹ��
     */
    void            *data;  //��ֵ��ngx_get_connection

    //��־λ��Ϊ1ʱ��ʾ�¼��ǿ�д�ġ�ͨ������£�����ʾ��Ӧ��TCP����Ŀǰ״̬�ǿ�д�ģ�Ҳ�������Ӵ��ڿ��Է����������״̬
    unsigned         write:1; //��ngx_get_connection����д�¼�evĬ��Ϊ1  ��ev�¼�Ӧ��Ĭ�ϻ���0

    //��־λ��Ϊ1ʱ��ʾΪ���¼����Խ����µ����ӡ�ͨ������£���ngx_cycle_t�е�listening��̬�����У�ÿһ����������ngx_listening_t��
    //Ӧ�Ķ��¼��е�accept��־λ�Ż���l  ngx_event_process_init����1
    unsigned         accept:1;

    /*
    �����־λ�������ֵ�ǰ�¼��Ƿ��ǹ��ڵģ��������Ǹ��¼�����ģ��ʹ�õģ����¼�����ģ��ɲ��ù��ġ�Ϊʲô��Ҫ�����־λ�أ�
    ����ʼ����һ���¼�ʱ������ǰ����¼����ܻ�ر�һЩ���ӣ�����Щ�����п���Ӱ�������¼��л�δ�����ĺ�����¼�����ʱ��
    ��ͨ��instance��־λ�����⴦�������Ѿ����ڵ��¼�������ϸ����ngx_epoll_module�����ʹ��instance��־λ����
    �����¼��ģ�����һ���������Ʒ���

        instance��־λΪʲô�����ж��¼��Ƿ���ڣ�instance��־λ��ʹ����ʵ�ܼ򵥣���������ָ������һλһ��
    ��0��һ���ԡ���Ȼ���һλʼ�ն���0����ô����������ʾinstance����������ʹ��ngx_epoll_add_event������epoll������¼�ʱ���Ͱ�epoll_event��
    ���ϳ�Աdata��ptr��Աָ��ngx_connection_t���ӵĵ�ַ��ͬʱ�����һλ��Ϊ����¼���instance��־������ngx_epoll_process_events������ȡ��ָ�����ӵ�
    ptr��ַʱ���Ȱ����һλinstanceȡ�������ٰ�ptr��ԭ�������ĵ�ַ����ngx_connection_t���ӡ�������instance�������ںδ�������Ҳ�ͽ���ˡ�
    ��ô�������¼�������ô�����أ��ٸ����ӣ�����epoll_wait -�η���3���¼����ڵ�
        1���¼��Ĵ�������У�����ҵ�����Ҫ�����Թر���һ�����ӣ����������ǡ�ö�Ӧ��3���¼��������Ļ����ڴ�����3���¼�ʱ������¼���
    �Ѿ��ǹ��ڹ����ˣ�һ�������Ȼ������Ȼ��ˣ��ѹرյ�������ӵ�fd�׽�����Ϊһ1�ܽ�������𣿴��ǲ��ܴ������������
        ����������������ò�Ʋ����ܷ����ĳ�����������ô�����ģ������3���¼���Ӧ��ngx_connection_t�����е�fd�׽���ԭ����50�������1���¼�
    ʱ��������ӵ��׽��ֹر��ˣ�ͬʱ��Ϊһ1�����ҵ���ngx_free_connection�������ӹ黹�����ӳء���ngx_epoll_process_events������ѭ���п�ʼ��
    ���2���¼���ǡ�õ�2���¼��ǽ����������¼�������ngx_get_connection�����ӳ���ȡ�������ӷǳ����ܾ��Ǹո��ͷŵĵ�3���¼���Ӧ�����ӡ�������
    ����50�ոձ��ͷţ�Linux�ں˷ǳ��п��ܰѸո��ͷŵ��׽���50�ַ�����½��������ӡ���ˣ���ѭ���д����3���¼�ʱ������¼����ǹ��ڵ��ˣ�����Ӧ
    ���¼��ǹرյ����ӣ��������½��������ӡ�
        ��ν��������⣿����instance��־λ��������ngx_get_connection�����ӳ��л�ȡһ��������ʱ��instance��־λ�ͻ��÷�
     */
    /* used to detect the stale events in kqueue and epoll */
    unsigned         instance:1; //ngx_get_connection�����ӳ��л�ȡһ��������ʱ��instance��־λ�ͻ��÷�  //��ngx_get_connection

    /*
     * the event was passed or would be passed to a kernel;
     * in aio mode - operation was posted.
     */
    /*
    ��־λ��Ϊ1ʱ��ʾ��ǰ�¼��ǻ�Ծ�ģ�Ϊ0ʱ��ʾ�¼��ǲ���Ծ�ġ����״̬��Ӧ���¼�����ģ�鴦��ʽ�Ĳ�ͬ�����磬������¼���
    ɾ���¼��ʹ����¼�ʱ��active��־λ�Ĳ�ͬ�����Ӧ�Ų�ͬ�Ĵ���ʽ����ʹ���¼�ʱ��һ�㲻��ֱ�Ӹı�active��־λ
     */  //ngx_epoll_add_event��Ҳ����1  �ڵ��øú����󣬸�ֵһֱΪ1�����ǵ���ngx_epoll_del_event
    unsigned         active:1; //����Ƿ��Ѿ���ӵ��¼������У������ظ����  ��server��accept�ɹ���
    //������client��connect��ʱ���active��1����ngx_epoll_add_connection����һ�����epoll_ctlΪEPOLL_CTL_ADD,����ٴ���ӷ�
    //��activeΪ1,��epoll_ctlΪEPOLL_CTL_MOD

    /*
    ��־λ��Ϊ1ʱ��ʾ�����¼�������kqueue����rtsig�¼�����ģ������Ч��������epoll�¼�����ģ���������壬���ﲻ������
     */
    unsigned         disabled:1;

    /* the ready event; in aio mode 0 means that no operation can be posted */
    /*
    ��־λ��Ϊ1ʱ��ʾ��ǰ�¼��Ѿ�����������Ҳ����˵����������¼�������ģ�鴦������¼�����
    HTTP����У����������¼���ready��־λ��ȷ���Ƿ���Խ���������߷�����Ӧ
    ready��־λ�����Ϊ1�����ʾ����ͻ��˵�TCP�����Ͽ��Է������ݣ����Ϊ0�����ʾ�ݲ��ɷ������ݡ�
     */ //������ԶԶ˵������ں˻�����û������(����NGX_EAGAIN)���������ӶϿ���0����ngx_unix_recv
     //�ڷ������ݵ�ʱ��ngx_unix_send�е�ʱ�����ϣ������1000�ֽڣ�����ʵ����sendֻ������500�ֽ�(˵���ں�Э��ջ������������Ҫͨ��epoll�ٴδٷ�write��ʱ�����д)�����������쳣�����ready��0
    unsigned         ready:1; //��ngx_epoll_process_events����1,���¼���������ȡ���ݺ�ngx_unix_recv����0

    /*
    �ñ�־λ����kqueue��eventport��ģ�������壬������Linux�ϵ�epoll�¼�����ģ�����������ģ�����ƪ����������ϸ˵��
     */
    unsigned         oneshot:1;

    /* aio operation is complete */
    //aio on | thread_pool��ʽ�£������ȡ������ɣ�����ngx_epoll_eventfd_handler(aio on)����ngx_thread_pool_handler(aio thread_pool)����1
    unsigned         complete:1; //��ʾ��ȡ������ɣ�ͨ��epoll���Ʒ��ػ�ȡ ����ngx_epoll_eventfd_handler

    //��־λ��Ϊ1ʱ��ʾ��ǰ������ַ����Ѿ�����  �����ں˻�����û�����ݣ���ȥ������᷵��0
    unsigned         eof:1; //��ngx_unix_recv
    //��־λ��Ϊ1ʱ��ʾ�¼��ڴ�������г��ִ���
    unsigned         error:1;

    //��־λ��ΪIʱ��ʾ����¼��Ѿ���ʱ��������ʾ�¼�������ģ������ʱ����
    /*���ͻ������ӵ����ݣ���ngx_http_init_connection(ngx_connection_t *c)�е�ngx_add_timer(rev, c->listening->post_accept_timeout)�Ѷ��¼���ӵ���ʱ���У������ʱ����1
      ÿ��ngx_unix_recv���ں����ݶ�ȡ��Ϻ�����������add epoll���ȴ��µ����ݵ�����ͬʱ��������ʱ��ngx_add_timer(rev, c->listening->post_accept_timeout);
      �����post_accept_timeout��ô���¼���û�����ݵ�����ʱ����ʼ����ر�TCP����*/

    /*
    ����ʱ��ָ�Ķ�ȡ�Զ����ݵĳ�ʱʱ�䣬д��ʱָ���ǵ����ݰ��ܴ��ʱ��write����NGX_AGAIN��������write��ʱ�����Ӷ��ж��Ƿ�ʱ���������
    �Զ����ݳ���С����һ��writeֱ�ӷ��سɹ����򲻻����write��ʱ��ʱ����Ҳ�Ͳ�����write��ʱ��д��ʱ���ο�����ngx_http_upstream_send_request
     */
    unsigned         timedout:1; //��ʱ����ʱ��ǣ���ngx_event_expire_timers
    //��־λ��Ϊ1ʱ��ʾ����¼������ڶ�ʱ����
    unsigned         timer_set:1; //ngx_event_add_timer ngx_add_timer ����1   ngx_event_expire_timers��0

    //��־λ��delayedΪ1ʱ��ʾ��Ҫ�ӳٴ�������¼��������������ٹ��� 
    unsigned         delayed:1; //���ټ�ngx_http_write_filter  

    /*
     ��־λ��Ϊ1ʱ��ʾ�ӳٽ���TCP���ӣ�Ҳ����˵������TCP�������ֺ󲢲��������ӣ�����Ҫ�ȵ������յ����ݰ���ŻὨ��TCP����
     */
    unsigned         deferred_accept:1; //ͨ��listen��ʱ����� deferred ������ȷ��

    /* the pending eof reported by kqueue, epoll or in aio chain operation */
    //��־λ��Ϊ1ʱ��ʾ�ȴ��ַ�����������ֻ��kqueue��aio�¼����������й�
    //һ���ڴ���EPOLLRDHUP(���Զ��Ѿ��رգ�����д���ݣ���������¼�)��ʱ�򣬻���1����ngx_epoll_process_events
    unsigned         pending_eof:1; 

    /*
    if (c->read->posted) { //ɾ��post���е�ʱ����Ҫ���
        ngx_delete_posted_event(c->read);
    }
     */
    unsigned         posted:1; //��ʾ�ӳٴ�����¼�����ngx_epoll_process_events -> ngx_post_event  ����Ƿ����ӳٶ�������
    //��־λ��Ϊ1ʱ��ʾ��ǰ�¼��Ѿ��رգ�epollģ��û��ʹ����
    unsigned         closed:1; //ngx_close_connection����1

    /* to test on worker exit */
    //�������ñ�־λĿǰ��ʵ������
    unsigned         channel:1;
    unsigned         resolver:1;

    unsigned         cancelable:1;

#if (NGX_WIN32)
    /* setsockopt(SO_UPDATE_ACCEPT_CONTEXT) was successful */
    unsigned         accept_context_updated:1;
#endif

#if (NGX_HAVE_KQUEUE)
    unsigned         kq_vnode:1;

    /* the pending errno reported by kqueue */
    int              kq_errno;
#endif

    /*
     * kqueue only:
     *   accept:     number of sockets that wait to be accepted
     *   read:       bytes to read when event is ready
     *               or lowat when event is set with NGX_LOWAT_EVENT flag
     *   write:      available space in buffer when event is ready
     *               or lowat when event is set with NGX_LOWAT_EVENT flag
     *
     * iocp: TODO
     *
     * otherwise:
     *   accept:     1 if accept many, 0 otherwise
     */

//��־ס����epoll�¼����������±�ʾһ�ξ����ܶ�ؽ���TCP���ӣ�����multi_accept�������Ӧ��ʵ��ԭ�����9.8.1��
#if (NGX_HAVE_KQUEUE) || (NGX_HAVE_IOCP)
    int              available;
#else
    unsigned         available:1; //ngx_event_accept��  ev->available = ecf->multi_accept;  
#endif
    /*
    ÿһ���¼�����ĵĲ�����handler�ص�������������ÿһ���¼�����ģ��ʵ�֣��Դ˾�������¼�������Ρ����ѡ�
     */

    /*
    1.event��������ͨ��epoll��д�¼�(�ο�ngx_event_connect_peer->ngx_add_conn����ngx_add_event)��ͨ����д�¼�����
    
    2.Ҳ��������ͨ��ʱ���¼�(�ο�ngx_cache_manager_process_handler->ngx_add_timer(ngx_event_add_timer))��ͨ��ngx_process_events_and_timers�е�
    epoll_wait���أ������Ƕ�д�¼��������أ�Ҳ��������Ϊû��ȡ�����������Ӷ��ȴ�0.5s�������»�ȡ���������¼���ִ�г�ʱ�¼��������¼������ж϶�
    ʱ�������еĳ�ʱ�¼�����ʱ��ִ�дӶ�ָ��event��handler��Ȼ���һ��ָ���Ӧr����u��->write_event_handler  read_event_handler
    
    3.Ҳ���������ö�ʱ��expirtʵ�ֵĶ�д�¼�(�ο�ngx_http_set_write_handler->ngx_add_timer(ngx_event_add_timer)),�������̼�2��ֻ����handler�в���ִ��write_event_handler  read_event_handler
    */
     
    //����¼�����ʱ�Ĵ�������ÿ���¼�����ģ�鶼������ʵ����
    //ngx_epoll_process_events��ִ��accept
    /*
     ��ֵΪngx_http_process_request_line     ngx_event_process_init�г�ʼ��Ϊngx_event_accept  ������ļ��첽i/o����ֵΪngx_epoll_eventfd_handler
     //��accept�ͻ������Ӻ�ngx_http_init_connection�и�ֵΪngx_http_wait_request_handler����ȡ�ͻ�������  
     �ڽ�����ͻ��˷�����������������к�ͷ���к�����handlerΪngx_http_request_handler
     */ //һ����ͻ��˵����ݶ�д�� ngx_http_request_handler;  ���˷�������дΪngx_http_upstream_handler(��fastcgi proxy memcache gwgi��)
    
    /* ngx_event_accept��ngx_http_ssl_handshake ngx_ssl_handshake_handler ngx_http_v2_write_handler ngx_http_v2_read_handler 
    ngx_http_wait_request_handler  ngx_http_request_handler,ngx_http_upstream_handler ngx_file_aio_event_handler */
    ngx_event_handler_pt  handler; //��epoll��д�¼���ngx_epoll_process_events����
   

#if (NGX_HAVE_IOCP)
    ngx_event_ovlp_t ovlp;
#endif
    //����epoll�¼�������ʽ��ʹ��index���������ﲻ��˵��
    ngx_uint_t       index;
    //�����ڼ�¼error_log��־��ngx_log_t����
    ngx_log_t       *log;  //���Լ�¼��־��ngx_log_t���� ��ʵ����ngx_listening_t�л�ȡ��log //��ֵ��ngx_event_accept
    //��ʱ���ڵ㣬���ڶ�ʱ���������
    ngx_rbtree_node_t   timer; //��ngx_event_timer_rbtree

    /* the posted queue */
    /*
    post�¼����ṹ��һ��������ͳһ�������������next��prev��Ϊ����ָ�룬�Դ˹���һ�����׵�˫����������nextָ���һ���¼��ĵ�ַ��
    prevָ��ǰһ���¼��ĵ�ַ
     */
    ngx_queue_t      queue;

#if 0

    /* the threads support */

    /*
     * the event thread context, we store it here
     * if $(CC) does not understand __thread declaration
     * and pthread_getspecific() is too costly
     */

    void            *thr_ctx;

#if (NGX_EVENT_T_PADDING)

    /* event should not cross cache line in SMP */

    uint32_t         padding[NGX_EVENT_T_PADDING];
#endif
#endif
};


#if (NGX_HAVE_FILE_AIO)

struct ngx_event_aio_s { //ngx_file_aio_init�г�ʼ��,�����ռ�͸�ֵ
    void                      *data; //ָ��ngx_http_request_t  ��ֵ��ngx_http_copy_aio_handler

    //ִ�еط���ngx_file_aio_event_handler����ֵ�ط���ngx_http_copy_aio_handler
    ngx_event_handler_pt       handler;//����������ҵ��ģ��ʵ�ֵķ��������첽I/O�¼���ɺ󱻵���
    ngx_file_t                *file;//fileΪҪ��ȡ��file�ļ���Ϣ

#if (NGX_HAVE_AIO_SENDFILE)
    ssize_t                  (*preload_handler)(ngx_buf_t *file);
#endif

    ngx_fd_t                   fd;//fileΪҪ��ȡ��file�ļ���Ϣ��Ӧ���ļ�������

#if (NGX_HAVE_EVENTFD)
    int64_t                    res; //��ֵ��ngx_epoll_eventfd_handler
#endif

#if !(NGX_HAVE_EVENTFD) || (NGX_TEST_BUILD_EPOLL)
    ngx_err_t                  err;
    size_t                     nbytes;
#endif

    ngx_aiocb_t                aiocb;
    //������ļ��첽i/o�е�ngx_event_aio_t����������ngx_event_aio_t->ngx_event_t(ֻ�ж�),����������¼��е�event,��Ϊngx_connection_s�е�event(��������д)
    ngx_event_t                event; //ֻ���첽i/o���¼�
};

#endif

//ngx_event_module_t�е�actions��Ա�Ƕ����¼�����ģ��ĺ��ķ����������ص㿴һ��actions�е���10�����󷽷�
typedef struct {
    /* 
    ����¼����������������1������Ȥ���¼���ӵ�����ϵͳ�ṩ���¼��������ƣ���epoll��
    kqueue�ȣ��У����������¼������󣬽������ڵ��������process_eventsʱ��ȡ����¼� 
    */
    ngx_int_t  (*add)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags); //ngx_add_event��ִ��

    /*
    ɾ���¼�������������1���Ѿ��������¼����������е��¼��Ƴ��������Ժ�ʹ����¼�����������
   process_events����ʱҲ�޷��ٻ�ȡ����¼�
    */
    ngx_int_t  (*del)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags); //ngx_del_event��ִ��

    /*
     ����1���¼���Ŀǰ�¼���ܲ����������������󲿷��¼�����ģ����ڸ÷�����ʵ�ֶ����������add������ȫһ�µ�
     */
    ngx_int_t  (*enable)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);

    //����1���¼���Ŀǰ�¼���ܲ����������������󲿷��¼�����ģ����ڸ÷�����ʵ�ֶ����������del������ȫһ�µ�
    ngx_int_t  (*disable)(ngx_event_t *ev, ngx_int_t event, ngx_uint_t flags);

    //���¼��������������һ���µ����ӣ�����ζ�������ϵĶ�д�¼�����ӵ��¼�������������
    ngx_int_t  (*add_conn)(ngx_connection_t *c); //ngx_add_conn��ִ��
    //���¼������������Ƴ�һ�����ӵĶ�д�¼�
    ngx_int_t  (*del_conn)(ngx_connection_t *c, ngx_uint_t flags); //ngx_del_conn��ִ��

    ngx_int_t  (*notify)(ngx_event_handler_pt handler); //ngx_notify��ִ��

    //�������Ĺ���ѭ���У���ͨ������process_event�����������¼���
    ngx_int_t  (*process_events)(ngx_cycle_t *cycle, ngx_msec_t timer,
                   ngx_uint_t flags); //���ü�ngx_process_events

    //��ʼ���¼�����ģ��ķ���
    ngx_int_t  (*init)(ngx_cycle_t *cycle, ngx_msec_t timer); //ngx_event_process_init��ִ��
    //�˳��¼�����ģ��ǰ���õķ���
    void       (*done)(ngx_cycle_t *cycle);  //ngx_done_events��ִ��
} ngx_event_actions_t;


extern ngx_event_actions_t   ngx_event_actions;


/*
 * The event filter requires to read/write the whole data:
 * select, poll, /dev/poll, kqueue, epoll.
 */ //#define NGX_LEVEL_EVENT    0  ��
#define NGX_USE_LEVEL_EVENT      0x00000001 //epoll��LTģʽ   ��ngx_handle_read_event  nginxĬ��ʹ��NGX_USE_CLEAR_EVENT���ش�����ʽ

/*
 * The event filter is deleted after a notification without an additional
 * syscall: kqueue, epoll.
 */
#define NGX_USE_ONESHOT_EVENT    0x00000002

/*
 * The event filter notifies only the changes and an initial level:
 * kqueue, epoll.
 */ //#define NGX_CLEAR_EVENT    EPOLLET  ��ngx_handle_read_event
 //Ĭ��ʹ�����,��ngx_epoll_init 
#define NGX_USE_CLEAR_EVENT      0x00000004 ////Ĭ���ǲ���LTģʽ��ʹ��epoll�ģ�NGX USE CLEAR EVENT��ʵ���Ͼ����ڸ���Nginxʹ��ETģʽ

/*
 * The event filter has kqueue features: the eof flag, errno,
 * available data, etc.
 */
#define NGX_USE_KQUEUE_EVENT     0x00000008

/*
 * The event filter supports low water mark: kqueue's NOTE_LOWAT.
 * kqueue in FreeBSD 4.1-4.2 has no NOTE_LOWAT so we need a separate flag.
 */
#define NGX_USE_LOWAT_EVENT      0x00000010

/*
 * The event filter requires to do i/o operation until EAGAIN: epoll.
 */
#define NGX_USE_GREEDY_EVENT     0x00000020  //epoll����Ӹñ��

/*
 * The event filter is epoll.
 */
#define NGX_USE_EPOLL_EVENT      0x00000040

/*
 * Obsolete.
 */
#define NGX_USE_RTSIG_EVENT      0x00000080

/*
 * Obsolete.
 */
#define NGX_USE_AIO_EVENT        0x00000100

/*
 * Need to add socket or handle only once: i/o completion port.
 */
#define NGX_USE_IOCP_EVENT       0x00000200

/*
 * The event filter has no opaque data and requires file descriptors table:
 * poll, /dev/poll.
 */
#define NGX_USE_FD_EVENT         0x00000400

/*
 * The event module handles periodic or absolute timer event by itself:
 * kqueue in FreeBSD 4.4, NetBSD 2.0, and MacOSX 10.4, Solaris 10's event ports.
 */
#define NGX_USE_TIMER_EVENT      0x00000800

/*
 * All event filters on file descriptor are deleted after a notification:
 * Solaris 10's event ports.
 */
#define NGX_USE_EVENTPORT_EVENT  0x00001000

/*
 * The event filter support vnode notifications: kqueue.
 */
#define NGX_USE_VNODE_EVENT      0x00002000


/*
 * The event filter is deleted just before the closing file.
 * Has no meaning for select and poll.
 * kqueue, epoll, eventport:         allows to avoid explicit delete,
 *                                   because filter automatically is deleted
 *                                   on file close,
 *
 * /dev/poll:                        we need to flush POLLREMOVE event
 *                                   before closing file.
 */
#define NGX_CLOSE_EVENT    1

/*
 * disable temporarily event filter, this may avoid locks
 * in kernel malloc()/free(): kqueue.
 */
#define NGX_DISABLE_EVENT  2

/*
 * event must be passed to kernel right now, do not wait until batch processing.
 */
#define NGX_FLUSH_EVENT    4


/* these flags have a meaning only for kqueue */
#define NGX_LOWAT_EVENT    0
#define NGX_VNODE_EVENT    0


#if (NGX_HAVE_EPOLL) && !(NGX_HAVE_EPOLLRDHUP)
#define EPOLLRDHUP         0
#endif


#if (NGX_HAVE_KQUEUE)

#define NGX_READ_EVENT     EVFILT_READ
#define NGX_WRITE_EVENT    EVFILT_WRITE

#undef  NGX_VNODE_EVENT
#define NGX_VNODE_EVENT    EVFILT_VNODE

/*
 * NGX_CLOSE_EVENT, NGX_LOWAT_EVENT, and NGX_FLUSH_EVENT are the module flags
 * and they must not go into a kernel so we need to choose the value
 * that must not interfere with any existent and future kqueue flags.
 * kqueue has such values - EV_FLAG1, EV_EOF, and EV_ERROR:
 * they are reserved and cleared on a kernel entrance.
 */
#undef  NGX_CLOSE_EVENT
#define NGX_CLOSE_EVENT    EV_EOF

#undef  NGX_LOWAT_EVENT
#define NGX_LOWAT_EVENT    EV_FLAG1

#undef  NGX_FLUSH_EVENT
#define NGX_FLUSH_EVENT    EV_ERROR

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  EV_ONESHOT
#define NGX_CLEAR_EVENT    EV_CLEAR

#undef  NGX_DISABLE_EVENT
#define NGX_DISABLE_EVENT  EV_DISABLE


#elif (NGX_HAVE_DEVPOLL || NGX_HAVE_EVENTPORT)

#define NGX_READ_EVENT     POLLIN
#define NGX_WRITE_EVENT    POLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1


#elif (NGX_HAVE_EPOLL)

#define NGX_READ_EVENT     (EPOLLIN|EPOLLRDHUP)
#define NGX_WRITE_EVENT    EPOLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_CLEAR_EVENT    EPOLLET
#define NGX_ONESHOT_EVENT  0x70000000
#if 0
#define NGX_ONESHOT_EVENT  EPOLLONESHOT
#endif


#elif (NGX_HAVE_POLL)

#define NGX_READ_EVENT     POLLIN
#define NGX_WRITE_EVENT    POLLOUT

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1


#else /* select */

#define NGX_READ_EVENT     0
#define NGX_WRITE_EVENT    1

#define NGX_LEVEL_EVENT    0
#define NGX_ONESHOT_EVENT  1

#endif /* NGX_HAVE_KQUEUE */


#if (NGX_HAVE_IOCP)
#define NGX_IOCP_ACCEPT      0
#define NGX_IOCP_IO          1
#define NGX_IOCP_CONNECT     2
#endif


#ifndef NGX_CLEAR_EVENT
#define NGX_CLEAR_EVENT    0    /* dummy declaration */
#endif

#define NGX_FUNC_LINE __FUNCTION__, __LINE__

/*
ngx_event_actions = ngx_epoll_module_ctx.actions; ngx_event_actionsΪ�����ģ��action����ngx_event_actions = ngx_poll_module_ctx.actions;
ngx_event_actions = ngx_select_module_ctx.actions;
*/
#define ngx_process_events   ngx_event_actions.process_events //ngx_process_events_and_timers��ִ��
#define ngx_done_events      ngx_event_actions.done

#define ngx_add_event        ngx_event_actions.add // ngx_epoll_add_event����øú���
#define ngx_del_event        ngx_event_actions.del

#define ngx_add_conn         ngx_event_actions.add_conn //connect��accept���ص�ʱ���õ�  �Ѿ�channel����ʱ����
#define ngx_del_conn         ngx_event_actions.del_conn

//ngx_notifyΪ�¼�ģ���֪ͨ��������Ҫ��ʹ�ø�֪ͨ��������ĳ�����Ѿ�ִ����ɣ�
//ngx_notifyͨ�����̣߳�����������ϣ�ngx_thread_pool_handler�����߳�ִ�У�Ҳ���ǽ���cycle{}ͨ��epoll_wait����ִ�У����������̳߳��е��߳�ִ��
#define ngx_notify           ngx_event_actions.notify

#define ngx_add_timer        ngx_event_add_timer //��ngx_process_events_and_timers�У������¼�ʹepoll_wait���أ����ִ�г�ʱ�Ķ�ʱ��
#define ngx_del_timer        ngx_event_del_timer


extern ngx_os_io_t  ngx_io;

#define ngx_recv             ngx_io.recv
#define ngx_recv_chain       ngx_io.recv_chain
#define ngx_udp_recv         ngx_io.udp_recv
#define ngx_send             ngx_io.send
#define ngx_send_chain       ngx_io.send_chain //epoll��ʽngx_io = ngx_linux_io;


//���еĺ���ģ��NGX_CORE_MODULE��Ӧ��������ctxΪngx_core_module_t����ģ�飬����http{} NGX_HTTP_MODULEģ���Ӧ��Ϊ������Ϊngx_http_module_t
//events{} NGX_EVENT_MODULEģ���Ӧ��Ϊ������Ϊngx_event_module_t
#define NGX_EVENT_MODULE      0x544E5645  /* "EVNT" */
/*
NGX_MAIN_CONF����������Գ�����ȫ�������У����������κ�{}���ÿ顣
NGX_EVET_CONF����������Գ�����events{}���ڡ�
NGX_HTTP_MAIN_CONF�� ��������Գ�����http{}���ڡ�
NGX_HTTP_SRV_CONF:����������Գ�����server{}���ڣ���server���������http{}�顣
NGX_HTTP_LOC_CONF����������Գ�����location{}���ڣ���location���������server{}�顣
NGX_HTTP_UPS_CONF�� ��������Գ�����upstream{}���ڣ���location���������http{}�顣
NGX_HTTP_SIF_CONF����������Գ�����server{}���ڵ�if{}���С���if���������http{}�顣
NGX_HTTP_LIF_CONF: ��������Գ�����location{}���ڵ�if{}���С���if���������http{}�顣
NGX_HTTP_LMT_CONF: ��������Գ�����limit_except{}����,��limit_except���������http{}�顣
*/
#define NGX_EVENT_CONF        0x02000000


//���ڴ洢��ngx_event_core_commands���������н������ĸ��ֲ���
typedef struct {
    ngx_uint_t    connections; //���ӳصĴ�С
    //ͨ��"use"ѡ��IO���÷�ʽ select epoll�ȣ�Ȼ��ͨ��������ֵ ��ngx_event_use
    //Ĭ�ϸ�ֵ��ngx_event_core_init_conf��ngx_event_core_module��ĵ�һ��NGX_EVENT_MODULEҲ����ngx_epoll_moduleĬ����Ϊ��һ��eventģ��
    ngx_uint_t    use; //ѡ�õ��¼�ģ���������¼�ģ���е���ţ�Ҳ����ctx_index��Ա ��ֵ��ngx_event_use

    /*
    �¼���available��־λ��Ӧ��multi_accept�������availableΪlʱ������Nginx -���Ծ�����ؽ��������ӣ�����ʵ��ԭ��Ҳ��������
     */ //Ĭ��0
    ngx_flag_t    multi_accept; //��־λ�����Ϊ1�����ʾ�ڽ��յ�һ���������¼�ʱ��һ���Խ��������ܶ������

    /*
     ���ccf->worker_processes > 1 && ecf->accept_mutex�����ڴ������̺󣬵���ngx_event_process_init��accept��ӵ�epoll�¼������У�
     ������ngx_process_events_and_timers->ngx_trylock_accept_mutex�а�accept��ӵ�epoll�¼�������
     */
    ngx_flag_t    accept_mutex;//��־λ��Ϊ1ʱ��ʾ���ø��ؾ�����

    /*
    ���ؾ�������ʹ��Щworker�������ò�����ʱ�ӳٽ��������ӣ�accept_mutex_delay��������ӳ�ʱ��ĳ��ȡ����������Ӱ�츺�ؾ��������
     */ //Ĭ��500ms��Ҳ����0.5s
    ngx_msec_t    accept_mutex_delay; //��λms  ���û��ȡ��mutex�������ӳ���ô��������»�ȡ

    u_char       *name;//��ѡ���¼�ģ������֣�����use��Ա��ƥ���  epoll select

/*
��-with-debug����ģʽ�£����Խ����ĳЩ�ͻ��˽���������������Լ������־����debug_connection�������ڱ�����Щ�ͻ��˵ĵ�ַ��Ϣ
*/
#if (NGX_DEBUG)
    ngx_array_t   debug_connection;
#endif
} ngx_event_conf_t;

//���еĺ���ģ��NGX_CORE_MODULE��Ӧ��������ctxΪngx_core_module_t����ģ�飬����http{} NGX_HTTP_MODULEģ���Ӧ��Ϊ������Ϊngx_http_module_t
//events{} NGX_EVENT_MODULEģ���Ӧ��Ϊ������Ϊngx_event_module_t

//������ĺ����ڽ�����event{}����ngx_events_block��ִ��,���е�NGX_EVENT_MODULEģ�鶼��ngx_events_block��ִ��
typedef struct {//ngx_epoll_module_ctx  ngx_select_module_ctx  
    ngx_str_t              *name; //�¼�ģ�������

    //�ڽ���������ǰ������ص��������ڴ����洢����������Ľṹ��
    void                 *(*create_conf)(ngx_cycle_t *cycle);

    //�ڽ�����������ɺ�initһconf�����ᱻ���ã������ۺϴ���ǰ�¼�ģ�����Ȥ��ȫ��������
    char                 *(*init_conf)(ngx_cycle_t *cycle, void *conf);

    //�����¼��������ƣ�ÿ�����ģ����Ҫʵ�ֵ�10�����󷽷�
    ngx_event_actions_t     actions;
} ngx_event_module_t;


extern ngx_atomic_t          *ngx_connection_counter;

extern ngx_atomic_t          *ngx_accept_mutex_ptr;
extern ngx_shmtx_t            ngx_accept_mutex;
extern ngx_uint_t             ngx_use_accept_mutex;
extern ngx_uint_t             ngx_accept_events;
extern ngx_uint_t             ngx_accept_mutex_held;
extern ngx_msec_t             ngx_accept_mutex_delay;
extern ngx_int_t              ngx_accept_disabled;


#if (NGX_STAT_STUB)

extern ngx_atomic_t  *ngx_stat_accepted;
extern ngx_atomic_t  *ngx_stat_handled;
extern ngx_atomic_t  *ngx_stat_requests;
extern ngx_atomic_t  *ngx_stat_active;
extern ngx_atomic_t  *ngx_stat_reading;
extern ngx_atomic_t  *ngx_stat_writing;
extern ngx_atomic_t  *ngx_stat_waiting;

#endif


#define NGX_UPDATE_TIME         1

/*
�õ����Ļ�����flagΪNGX_POST_EVENTS������ζ��ngx_process_events�����У��κ��¼������Ӻ������accept�¼����ŵ�
ngx_posted_accept_events�����У�epollin|epollout�¼����ŵ�ngx_posted_events������ 
*/ //��ngx_process_events_and_timers����λ��λ  ���flag��Ϊ��λ����ngx_epoll_process_events���Ӻ���epoll�¼�ngx_post_event
#define NGX_POST_EVENTS         2 //NGX_POST_EVENTS������ζ��ngx_process_events�����У��κ��¼������Ӻ���


extern sig_atomic_t           ngx_event_timer_alarm;
extern ngx_uint_t             ngx_event_flags;
extern ngx_module_t           ngx_events_module;
extern ngx_module_t           ngx_event_core_module;


#define ngx_event_get_conf(conf_ctx, module)                                  \
             (*(ngx_get_conf(conf_ctx, ngx_events_module))) [module.ctx_index];



void ngx_event_accept(ngx_event_t *ev);
ngx_int_t ngx_trylock_accept_mutex(ngx_cycle_t *cycle);
u_char *ngx_accept_log_error(ngx_log_t *log, u_char *buf, size_t len);


void ngx_process_events_and_timers(ngx_cycle_t *cycle);
//ngx_int_t ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags);
ngx_int_t ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags, const char* func, int line);

ngx_int_t ngx_handle_write_event(ngx_event_t *wev, size_t lowat, const char* func, int line);


#if (NGX_WIN32)
void ngx_event_acceptex(ngx_event_t *ev);
ngx_int_t ngx_event_post_acceptex(ngx_listening_t *ls, ngx_uint_t n);
u_char *ngx_acceptex_log_error(ngx_log_t *log, u_char *buf, size_t len);
#endif


ngx_int_t ngx_send_lowat(ngx_connection_t *c, size_t lowat);


/* used in ngx_log_debugX() */
#define ngx_event_ident(p)  ((ngx_connection_t *) (p))->fd


#include <ngx_event_timer.h>
#include <ngx_event_posted.h>

#if (NGX_WIN32)
#include <ngx_iocp_module.h>
#endif


#endif /* _NGX_EVENT_H_INCLUDED_ */
