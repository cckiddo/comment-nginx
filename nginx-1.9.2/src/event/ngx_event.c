
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_log.h>

#define DEFAULT_CONNECTIONS  512


extern ngx_module_t ngx_kqueue_module;
extern ngx_module_t ngx_eventport_module;
extern ngx_module_t ngx_devpoll_module;
extern ngx_module_t ngx_epoll_module;
extern ngx_module_t ngx_select_module;


static char *ngx_event_init_conf(ngx_cycle_t *cycle, void *conf);
static ngx_int_t ngx_event_module_init(ngx_cycle_t *cycle);
static ngx_int_t ngx_event_process_init(ngx_cycle_t *cycle);
static char *ngx_events_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *ngx_event_connections(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_event_use(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_event_debug_connection(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static void *ngx_event_core_create_conf(ngx_cycle_t *cycle);
static char *ngx_event_core_init_conf(ngx_cycle_t *cycle, void *conf);


/* 
nginx�ṩ����timer_resolution�����û���ʱ����µļ����
���ø����nginx��ʹ���жϻ��ƣ�����ʹ�ö�ʱ��������е���Сʱ��Ϊepoll_wait�ĳ�ʱʱ�䣬����ʱ��ʱ�������ڱ��жϡ�
timer_resolutionָ���ʹ�ý�������epoll_wait��ʱʱ��Ϊ-1�����ʾepoll_wait����Զ����ֱ����д�¼��������ź��жϡ� 

��������ļ���ʹ����timer_ resolution�����Ҳ����ngx_timer_resolutionֵ����0����˵���ã���ϣ��������ʱ�侫ȷ��Ϊngx_timer_resolution����

*/ //ngx_timer_signal_handler��ʱ����ʱͨ����ֵ����       ����������������epoll_wait�ķ������ɶ�ʱ���ж�����
//��ʱ��������ngx_event_process_init����ʱ����Ч��ngx_process_events_and_timers -> ngx_process_events
//��timer_resolutionȫ�������н������Ĳ���,��ʾ����msִ�ж�ʱ���жϣ�Ȼ��epoll_wail�᷵�ظ����ڴ�ʱ��
static ngx_uint_t     ngx_timer_resolution; //������timer_resolution��������ֵΪ0   �ο�ngx_process_events_and_timers  ��λ��ms
sig_atomic_t          ngx_event_timer_alarm; //ngx_event_timer_alarmֻ�Ǹ�ȫ�ֱ�����������Ϊlʱ����ʾ��Ҫ����ʱ�䡣

static ngx_uint_t     ngx_event_max_module;//gx_event_max_module�Ǳ����Nginx�������¼�ģ����ܸ�����

ngx_uint_t            ngx_event_flags; //λͼ��ʾ����NGX_USE_FD_EVENT��  ��ʼ����ngx_epoll_init
ngx_event_actions_t   ngx_event_actions; //ngx_event_actions = ngx_epoll_module_ctx.actions;


static ngx_atomic_t   connection_counter = 1;

//ԭ�ӱ������͵�ngx_connection_counter��ͳ�����н�������������������������������ӣ� ���ܵ�������������ĳ�����̵ģ������н��̵ģ���Ϊ�����ǹ����ڴ��
ngx_atomic_t         *ngx_connection_counter = &connection_counter;
ngx_atomic_t         *ngx_accept_mutex_ptr; //ָ������ڴ�ռ䣬��ngx_event_module_init
//ngx_accept_mutexΪ�����ڴ滥����  //��ȡ�������Ľ��̲Ż���ܿͻ��˵�accept����
ngx_shmtx_t           ngx_accept_mutex; //�����ڴ�Ŀռ�  �ڽ����ӵ�ʱ��Ϊ�˱��⾪Ⱥ����accept��ʱ��ֻ�л�ȡ����ԭ�������Ű�accept��ӵ�epoll�¼��У���ngx_trylock_accept_mutex


/*
ע��:�����ngx_accept_mutex_ptr�����������ȫ�ֱ���ngx_use_accept_mutex�ȵ�����?
ngx_accept_mutex_ptr�ǹ����ڴ�ռ䣬���н��̹����������ngx_use_accept_mutex��ȫ�ֱ����Ǳ����̿ɼ��ģ��������̲���ʹ�øÿռ䣬���ɼ���
*/



//ngx_use_accept_mutex��ʾ�Ƿ���Ҫͨ����accept�����������Ⱥ���⡣��nginx worker������>1ʱ�������ļ��д�accept_mutexʱ�������־��Ϊ1   
//����ʵ��:�ڴ������̵߳�ʱ����ִ��ngx_event_process_initʱ��û����ӵ�epoll���¼��У�worker����accept��������ٷ���epoll
//ccf->master && ccf->worker_processes > 1 && ecf->accept_mutex ��������Ż��øñ��Ϊ1
ngx_uint_t            ngx_use_accept_mutex; //��ô���ngx_use_accept_mutex��Ϊ1�����Ա��⾪Ⱥ����ֵ��ngx_event_process_init 
ngx_uint_t            ngx_accept_events; //ֻ��eventport���õ��ñ���
/* ngx_accept_mutex_held�ǵ�ǰ���̵�һ��ȫ�ֱ��������Ϊl�����ʾ��������Ѿ���ȡ����ngx_accept_mutex�������Ϊ0�����ʾû�л�ȡ���� */
//��ngx_process_events_and_timers����λ��λ  ���flag��Ϊ��λ����ngx_epoll_process_events���Ӻ���epoll�¼�ngx_post_event
//���˸ñ�ǣ���ʾ�ý����Ѿ���accept�¼���ӵ�epoll�¼������ˣ������ظ�ִ�к����ngx_enable_accept_events���ú�������ϵͳ���ù��̣�Ӱ������
ngx_uint_t            ngx_accept_mutex_held; //1��ʾ��ǰ��ȡ��ngx_accept_mutex��   0��ʾ��ǰ��û�л�ȡ��ngx_accept_mutex��   
//Ĭ��0.5s��������accept_mutex_delay��������
ngx_msec_t            ngx_accept_mutex_delay; //���û��ȡ��mutex�������ӳ���ô��������»�ȡ��accept_mutex_delay���ã���λ500ms

/*
ngx_accept_disabled��ʾ��ʱ�����ɣ�û��Ҫ�ٴ����������ˣ�������nginx.conf����������ÿһ��nginx worker�����ܹ�����������������
���ﵽ�������7/8ʱ��ngx_accept_disabledΪ����˵����nginx worker���̷ǳ���æ��������ȥ���������ӣ���Ҳ�Ǹ��򵥵ĸ��ؾ���
*/
ngx_int_t             ngx_accept_disabled; //��ֵ�ط���ngx_event_accept

/*
   ��ΪWeb��������Nginx����ͳ��������������HTTP����״���Ĺ��ܣ�����ĳһ��Nginx worker���̵�״������������worker��������״�����ܺͣ���
���磬��������ͳ��ĳһʱ����Nginx�Ⱦ������������״�������涨���6��ԭ�ӱ�����������ͳ��ngx_http_stub_status_moduleģ������״���ģ�������ʾ��

NGX_STAT_STUBѡ��ͨ������ı������ʹ��:
��ģ���� auto/options�ļ��У�ͨ�������configѡ���ģ����뵽nginx
    HTTP_STUB_STATUS=NO
    --with-http_stub_status_module)  HTTP_STUB_STATUS=YES       ;; 

*/
#if (NGX_STAT_STUB)

//�Ѿ������ɹ�����TCP������
ngx_atomic_t   ngx_stat_accepted0;
ngx_atomic_t  *ngx_stat_accepted = &ngx_stat_accepted0;

/*
���ӽ����ɹ��һ�ȡ��ngx_connection t�ṹ����Ѿ�������ڴ�أ������ڱ�ʾ��ʼ���˶�/д�¼����������
*/
ngx_atomic_t   ngx_stat_handled0;
ngx_atomic_t  *ngx_stat_handled = &ngx_stat_handled0;

//�Ѿ���HTTPģ�鴦�����������
ngx_atomic_t   ngx_stat_requests0;
ngx_atomic_t  *ngx_stat_requests = &ngx_stat_requests0;

/*
�Ѿ���ngx_cycle_t���Ľṹ���free_connections���ӳ��л�ȡ��ngx_connection_t����Ļ�Ծ������
*/
ngx_atomic_t   ngx_stat_active0;
ngx_atomic_t  *ngx_stat_active = &ngx_stat_active0;

//���ڽ���TCP����������
ngx_atomic_t   ngx_stat_reading0;
ngx_atomic_t  *ngx_stat_reading = &ngx_stat_reading0;

//���ڷ���TCP����������
ngx_atomic_t   ngx_stat_writing0;
ngx_atomic_t  *ngx_stat_writing = &ngx_stat_writing0;

ngx_atomic_t   ngx_stat_waiting0;
ngx_atomic_t  *ngx_stat_waiting = &ngx_stat_waiting0;

#endif



static ngx_command_t  ngx_events_commands[] = {

    { ngx_string("events"),
      NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
      ngx_events_block,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_core_module_t  ngx_events_module_ctx = {
    ngx_string("events"),
    NULL,
    ngx_event_init_conf
};

/*
���Կ�����ngx_events_module_ctxʵ�ֵĽӿ�ֻ�Ƕ�����ģ�����ֶ��ѣ�ngx_core_module_t�ӿ��ж����create_onf����û��ʵ�֣�NULL��ָ�뼴Ϊ��ʵ�֣���
Ϊʲô�أ�������Ϊngx_events_moduleģ�鲢�������������Ĳ�����ֻ���ڳ���events����������ø��¼�ģ��ȥ����eventso()���ڵ������
��Ȼ�Ͳ���Ҫʵ��create_conf�����������洢����������Ľṹ��. 
*/
//һ����nginx.conf�����ļ����ҵ�ngx_events_module����Ȥ�ġ�events{}��ngx_events_moduleģ��Ϳ�ʼ������
//���˶�events������Ľ����⣬��ģ��û���������κ�����
ngx_module_t  ngx_events_module = {
    NGX_MODULE_V1,
    &ngx_events_module_ctx,                /* module context */
    ngx_events_commands,                   /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_str_t  event_core_name = ngx_string("event_core");

//������ü�ngx_event_core_commands ngx_http_core_commands ngx_stream_commands ngx_http_core_commands ngx_core_commands  ngx_mail_commands
static ngx_command_t  ngx_event_core_commands[] = {
    //ÿ��worker���̿���ͬʱ��������������
    //���ӳصĴ�С��Ҳ����ÿ��worker������֧�ֵ�TCP��������������������connections��������������ظ��ģ��ɲ���9.3.3��������ӳصĸ���
    { ngx_string("worker_connections"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_connections,
      0,
      0,
      NULL },

    //�����¼�ģ�͡� use [kqueue | rtsig | epoll | dev/poll | select | poll | eventport] linuxϵͳ��ֻ֧��select poll epoll���� 
    //freebsd���kqueue,LINUX��û��
    //ȷ��ѡ����һ���¼�ģ����Ϊ�¼���������
    { ngx_string("use"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_use,
      0,
      0,
      NULL },
    //���¼�ģ��֪ͨ��TCP����ʱ���������ڱ��ε����ж����еĿͻ���TCP�������󶼽�������
    //��Ӧ���¼������available�ֶΡ�����epoll�¼�����ģʽ��˵����ζ���ڽ��յ�һ���������¼�ʱ������accept�Ծ����ܶ�ؽ�������
    { ngx_string("multi_accept"),
      NGX_EVENT_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      0,
      offsetof(ngx_event_conf_t, multi_accept),
      NULL },

    //accept_mutex on|off�Ƿ��accept����������Ϊ��ʵ��worker���̽������ӵĸ��ؾ��⡢�򿪺��ö��worker�������������кŵĽ���TCP����
    //Ĭ���Ǵ򿪵ģ�����رյĻ�TCP���ӻ���죬��worker������Ӳ�����ô���ȡ�
    { ngx_string("accept_mutex"),
      NGX_EVENT_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      0,
      offsetof(ngx_event_conf_t, accept_mutex),
      NULL },
    //accept_mutex_delay time���������Ϊaccpt_mutex on����workerͬһʱ��ֻ��һ�������ܸ���ȡaccept�������accept�����������ģ����Ȣ������
    //�������أ�Ȼ��ȴ�timeʱ�����»�ȡ��
    //����accept_mutex���ؾ��������ӳ�accept_mutex_delay���������ͼ�����������¼�
    { ngx_string("accept_mutex_delay"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      0,
      offsetof(ngx_event_conf_t, accept_mutex_delay),
      NULL },
    //debug_connection 1.2.2.2�����յ���IP��ַ�����ʱ��ʹ��debug�����ӡ�������Ļ�������error_log�е�����
    //��Ҫ������ָ��IP��TCP���Ӵ�ӡdebug����ĵ�����־
    { ngx_string("debug_connection"),
      NGX_EVENT_CONF|NGX_CONF_TAKE1,
      ngx_event_debug_connection,
      0,
      0,
      NULL },

      ngx_null_command
};

//ngx_event_core_moduleģ�����ʵ����create_conf������init_conf������������Ϊ��������������TCP�����¼���������
//���Բ���ʵ��ngx_event_actions_t�еķ���
ngx_event_module_t  ngx_event_core_module_ctx = {
    &event_core_name,
    ngx_event_core_create_conf,            /* create configuration */
    ngx_event_core_init_conf,              /* init configuration */

    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*
Nginx������һϵ�У�ĿǰΪ9���������ڲ�ͬ����ϵͳ����ͬ�ں˰汾�ϵ��¼�����ģ�飬������ngx_epoll_module��ngx_kqueue_module��
ngx_poll_module��ngx_select_module��ngx_devpoll_module��ngx_eventport_module��ngx_aio_module��ngx_rtsig_module
�ͻ���Windows��ngx_select_moduleģ�顣��ngx_event_core_moduleģ��ĳ�ʼ�������У����������9��ģ����ѡȡ1����ΪNginx���̵��¼�����ģ�顣
*/
ngx_module_t  ngx_event_core_module = {
    NGX_MODULE_V1,
    &ngx_event_core_module_ctx,            /* module context */
    ngx_event_core_commands,               /* module directives */
    NGX_EVENT_MODULE,                      /* module type */
    NULL,                                  /* init master */
    ngx_event_module_init,                 /* init module */ //�����������ļ���ִ��
    ngx_event_process_init,                /* init process */ //�ڴ����ӽ��̵�����ִ��  ngx_worker_process_init
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


//��һ�δ���ͻ�����������󣬻��ngx_http_process_request_line��ӵ���ʱ���У������client_header_timeout��û���ŵ��������ݹ�����
//����ߵ�ngx_http_read_request_header�е�ngx_http_finalize_request(r, NGX_HTTP_BAD_REQUEST);�Ӷ��ر�����

/*
��˵nginxǰ����������ʲô�ǡ���Ⱥ������˵�������߳�/����̣�linux���߳̽���Ҳû������𣩵ȴ�ͬһ��socket�¼���������¼�����ʱ��
��Щ�߳�/���̱�ͬʱ���ѣ����Ǿ�Ⱥ�����������Ч�ʺܵ��£������̱��ں����µ��Ȼ��ѣ�ͬʱȥ��Ӧ��һ���¼�����Ȼֻ��һ�������ܴ���
�¼��ɹ��������Ľ����ڴ�����¼�ʧ�ܺ��������ߣ�Ҳ������ѡ�񣩡����������˷�������Ǿ�Ⱥ��

nginx����������master���̼����˿ںţ�����80�������е�nginx worker���̿�ʼ��epoll_wait���������¼���linux�£�����������κα�����һ��
����������ʱ�����ж��worker������epoll_wait�󱻻��ѣ�Ȼ�����Լ�acceptʧ�ܡ����ڣ����ǿ��Կ���nginx����ô���������Ⱥ�����ˡ�
*/

void
ngx_process_events_and_timers(ngx_cycle_t *cycle)  
{
    ngx_uint_t  flags;
    ngx_msec_t  timer, delta;
    
    /*nginx�ṩ����timer_resolution�����û���ʱ����µļ����
    ���ø����nginx��ʹ���жϻ��ƣ�����ʹ�ö�ʱ��������е���Сʱ��Ϊepoll_wait�ĳ�ʱʱ�䣬����ʱ��ʱ�������ڱ��жϡ�
    timer_resolutionָ���ʹ�ý�������epoll_wait��ʱʱ��Ϊ-1�����ʾepoll_wait����Զ����ֱ����д�¼��������ź��жϡ�
    
    1.����timer_resolutionʱ��flags=0��ֻ�е�ngx_event_timer_alarm=1ʱepoll_wait()����ʱ��ִ��ngx_time_update�����º���ngx_event_timer_alarm���㣩
    2.û������timer_resolution��flags = NGX_UPDATE_TIME��timerΪ��ʱ�����������С��ʱʱ�䣬����Ϊepoll_wait�ĳ�ʱʱ��(timeout) */
    
    if (ngx_timer_resolution) {
        timer = NGX_TIMER_INFINITE; //���������timer_resolution������timerΪ-1,Ҳ����epoll_waitֻ��ͨ���¼��������أ���ʱ����ʱ����epoll_wait����
        flags = 0;

    } else { //
        //���û������timer_resolution��ʱ������ÿ��epoll_wait�����ʱ�䣬����ÿ��timer_resolution���ø���һ��ʱ�䣬��ngx_epoll_process_events
        //��ȡ����������ĳ�ʱ��ʱ��ʱ��
        timer = ngx_event_find_timer();//�������һ��accept��ʱ��ʧ�ܣ�����ngx_event_accept�л��ngx_event_conf_t->accept_mutex_delay���뵽�������ʱ����
        flags = NGX_UPDATE_TIME; 
        
#if (NGX_WIN32)

        /* handle signals from master in case of network inactivity */

        if (timer == NGX_TIMER_INFINITE || timer > 500) {
            timer = 500;
        }

#endif
    }

    ngx_use_accept_mutex = 1;
   //ngx_use_accept_mutex��ʾ�Ƿ���Ҫͨ����accept�����������Ⱥ���⡣��nginx worker������>1ʱ�������ļ��д�accept_mutexʱ�������־��Ϊ1   
    if (ngx_use_accept_mutex) {
        /*
              ngx_accept_disabled��ʾ��ʱ�����ɣ�û��Ҫ�ٴ����������ˣ�������nginx.conf����������ÿһ��nginx worker�����ܹ�����������������
          ���ﵽ�������7/8ʱ��ngx_accept_disabledΪ����˵����nginx worker���̷ǳ���æ��������ȥ���������ӣ���Ҳ�Ǹ��򵥵ĸ��ؾ���
              �ڵ�ǰʹ�õ����ӵ�������������7/8ʱ���Ͳ����ٴ����������ˣ�ͬʱ����ÿ�ε���process_eventsʱ���Ὣngx_accept_disabled��1��
          ֱ��ngx_accept_disabled��������������7/8����ʱ���Ż����ngx_trylock_accept_mutex��ͼȥ�����������¼���
          */
        if (ngx_accept_disabled > 0) { //Ϊ��˵�������������˳����˷�֮��,���������Ľ����������else����accept
            ngx_accept_disabled--;

        } else {
            /*
                 ���ngx_trylock_accept_mutex����û�л�ȡ�����������������¼�����ģ���process_events����ʱֻ�ܴ������е������ϵ��¼���
                 �����ȡ������������process_events����ʱ�ͻ�ȴ������������ϵ��¼���Ҳ���������ӵ��¼���
              
                ������������⾪Ⱥ?
                   ������accept mutex��ֻ�гɹ���ȡ���Ľ��̣��ŻὫlisten  
                   �׽��ַ���epoll�С���ˣ���ͱ�֤��ֻ��һ������ӵ��  
                   �����׽ӿڣ������н���������epoll_waitʱ��������־�Ⱥ����  
                   �����ngx_trylock_accept_mutex�����У����˳���Ļ�ȡ��������ô���Ὣ�����˿�ע�ᵽ��ǰworker���̵�epoll����   

               ���accept�������worker����һ�����Եõ������������������������̣��������̷��أ���ȡ�ɹ��Ļ�ngx_accept_mutex_held����Ϊ1��
               �õ�������ζ�ż���������ŵ������̵�epoll���ˣ����û���õ��������������ᱻ��epoll��ȡ���� 
              */
        /*
           ���ngx_use_accept_mutexΪ0Ҳ����δ����accept_mutex��������ngx_worker_process_init->ngx_event_process_init �а�accept���Ӷ��¼�ͳ�Ƶ�epoll��
           ������ngx_process_events_and_timers->ngx_process_events_and_timers->ngx_trylock_accept_mutex�а�accept���Ӷ��¼�ͳ�Ƶ�epoll��
           */
            if (ngx_trylock_accept_mutex(cycle) == NGX_ERROR) { //�����ǻ�ȡ��������û��ȡ�������Ƿ���NGX_OK
                return;
            }

            /*
                �õ����Ļ�����flagΪNGX_POST_EVENTS������ζ��ngx_process_events�����У��κ��¼������Ӻ������accept�¼����ŵ�
                ngx_posted_accept_events�����У�epollin|epollout�¼����ŵ�ngx_posted_events������ 
               */
            if (ngx_accept_mutex_held) {
                flags |= NGX_POST_EVENTS;

            } else {
                /*
                    �ò�������Ҳ�Ͳ��ᴦ������ľ�������timerʵ���Ǵ���epoll_wait�ĳ�ʱʱ�䣬�޸�Ϊ���ngx_accept_mutex_delay��ζ
                    ��epoll_wait���̵ĳ�ʱ���أ����������ӳ�ʱ��û�еõ�����   
                    */
                if (timer == NGX_TIMER_INFINITE
                    || timer > ngx_accept_mutex_delay)
                {   //���û��ȡ���������ӳ���ô��ms���»�ȡ˵������ѭ����Ҳ���Ǽ��������������̻�ã������������epoll_wait��˯��0.5s,Ȼ�󷵻�
                    timer = ngx_accept_mutex_delay; //��֤��ô��ʱ�䳬ʱ��ʱ�����epoll_wait���أ��Ӷ����Ը����ڴ�ʱ��
                }
            }
        }
    }

    delta = ngx_current_msec;

    /*
    1.������̻����������ȡ��������ý�����epoll�¼�������ᴥ�����أ�Ȼ��õ���Ӧ���¼�handler�������ӳٶ����У�Ȼ���ͷ�����Ȼ
    ����ִ�ж�Ӧhandler��ͬʱ����ʱ�䣬�жϸý��̶�Ӧ�ĺ�������Ƿ��ж�ʱ����ʱ��
    2.���û�л�ȡ��������Ĭ�ϴ���epoll_wait�ĳ�ʱʱ����0.5s����ʾ��0.5s������ȡ����0.5s��ʱ�󣬻���µ�ǰʱ�䣬ͬʱ�ж��Ƿ��й��ڵ�
      ��ʱ��������ָ���Ӧ�Ķ�ʱ������
    */

/*
1.ngx_event_s��������ͨ��epoll��д�¼�(�ο�ngx_event_connect_peer->ngx_add_conn����ngx_add_event)��ͨ����д�¼�����

2.Ҳ��������ͨ��ʱ���¼�(�ο�ngx_cache_manager_process_handler->ngx_add_timer(ngx_event_add_timer))��ͨ��ngx_process_events_and_timers�е�
epoll_wait���أ������Ƕ�д�¼��������أ�Ҳ��������Ϊû��ȡ�����������Ӷ��ȴ�0.5s�������»�ȡ���������¼���ִ�г�ʱ�¼��������¼������ж϶�
ʱ�������еĳ�ʱ�¼�����ʱ��ִ�дӶ�ָ��event��handler��Ȼ���һ��ָ���Ӧr����u��->write_event_handler  read_event_handler

3.Ҳ���������ö�ʱ��expirtʵ�ֵĶ�д�¼�(�ο�ngx_http_set_write_handler->ngx_add_timer(ngx_event_add_timer)),�������̼�2��ֻ����handler�в���ִ��write_event_handler  read_event_handler
*/
    
    //linux�£���ͨ�����׽��ֵ���ngx_epoll_process_events������ʼ�����첽�ļ�i/o�����¼��Ļص�����Ϊngx_epoll_eventfd_handler
    (void) ngx_process_events(cycle, timer, flags);

    delta = ngx_current_msec - delta; //(void) ngx_process_events(cycle, timer, flags)��epoll�ȴ��¼��������̻��ѵ�ʱ��

    ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "epoll_wait timer range(delta): %M", delta);
             
    //����Ӧ�������ڿͻ��˵�accept�¼���epoll_wait���غ���뵽post���У�ִ��������accpet�����¼��������ͷ�ngx_accept_mutex���������������̾Ϳ�����������accept�ͻ�������
    ngx_event_process_posted(cycle, &ngx_posted_accept_events); //һ��ִ��ngx_event_accept
    
    //�ͷ������ٴ��������EPOLLIN EPOLLOUT����   
    if (ngx_accept_mutex_held) {
        ngx_shmtx_unlock(&ngx_accept_mutex);
    }

    if (delta) {
        ngx_event_expire_timers(); //�������������еĳ�ʱ�¼�handler
    }
    
    /*
     Ȼ���ٴ������������ݶ�д������Ϊ��Щ�����ʱ�ã�������ngx_process_events��NGX_POST_EVENTS��־���¼�������ngx_posted_events
     �����У��ӳٵ����ͷ����ٴ��� 
     */
    ngx_event_process_posted(cycle, &ngx_posted_events); //��ͨ��д�¼������ͷ�ngx_accept_mutex����ִ�У���߿ͻ���accept����
}

/*
ET��Edge Triggered����LT��Level Triggered������Ҫ������Դ���������ӿ��� 
eg�� 
1�� ��ʾ�ܵ����ߵ��ļ����ע�ᵽepoll�У� 
2�� �ܵ�д����ܵ���д��2KB�����ݣ� 
3�� ����epoll_wait���Ի�ùܵ�����Ϊ�Ѿ������ļ������ 
4�� �ܵ����߶�ȡ1KB������ 
5�� һ��epoll_wait������� 
�����ETģʽ���ܵ���ʣ���1KB�������ٴε���epoll_wait���ò����ܵ����ߵ��ļ�������������µ�����д��ܵ��������LTģʽ��
ֻҪ�ܵ��������ݿɶ���ÿ�ε���epoll_wait���ᴥ����


epoll������ģʽLT��ET
���ߵĲ�������level-triggerģʽ��ֻҪĳ��socket����readable/writable״̬������ʲôʱ�����epoll_wait���᷵�ظ�socket��
��edge-triggerģʽ��ֻ��ĳ��socket��unreadable��Ϊreadable���unwritable��Ϊwritableʱ��epoll_wait�Ż᷵�ظ�socket��

���ԣ���epoll��ETģʽ�£���ȷ�Ķ�д��ʽΪ:
����ֻҪ�ɶ�����һֱ����ֱ������0������ errno = EAGAIN
д:ֻҪ��д����һֱд��ֱ�����ݷ����꣬���� errno = EAGAIN



ngx_handle_read_event�����Ὣ���¼���ӵ��¼�����ģ���У��������¼���Ӧ��TCP������һ�����ֿɶ��¼�������յ�TCP������һ
�˷��������ַ������ͻ�ص����¼���handler������
    ���濴һ��ngx_handle_read_event�Ĳ����ͷ���ֵ������rev��Ҫ�������¼���flags����ָ���¼���������ʽ�����ڲ�ͬ���¼�����
ģ�飬flags��ȡֵ��Χ����ͬ��������Linux�µ�epollΪ��������ngx_epoll_module��˵��flags��ȡֵ��Χ������0����NGX_CLOSE_EVENT
��NGX_CLOSE_EVENT����epoll��LTˮƽ����ģʽ����Ч����Nginx��Ҫ������ETģʽ�£�һ����Ժ���flags����������÷�������NGX_0K��ʾ�ɹ�������
NGX��ERRO����ʾʧ�ܡ�

����Linux Epoll ETģʽEPOLLOUT��EPOLLIN����ʱ�� ETģʽ��Ϊ��Ե����ģʽ������˼�壬������Ե��������������ᴥ���ġ�

EPOLLOUT�¼���
EPOLLOUT�¼�ֻ��������ʱ����һ�Σ���ʾ��д������ʱ����Ҫ����������Ҫ��׼��������������
1.ĳ��write��д���˷��ͻ����������ش�����ΪEAGAIN��
2.�Զ˶�ȡ��һЩ���ݣ������¿�д�ˣ���ʱ�ᴥ��EPOLLOUT��
�򵥵�˵��EPOLLOUT�¼�ֻ���ڲ���д����д��ת��ʱ�̣��Żᴥ��һ�Σ����Խб�Ե��������з�û��ģ�

��ʵ������������ǿ�ƴ���һ�Σ�Ҳ���а취�ģ�ֱ�ӵ���epoll_ctl��������һ��event�Ϳ����ˣ�event��ԭ��������һģһ�����У����������EPOLLOUT�����ؼ����������ã��ͻ����ϴ���һ��EPOLLOUT�¼���

EPOLLIN�¼���
EPOLLIN�¼���ֻ�е��Զ�������д��ʱ�Żᴥ�������Դ���һ�κ���Ҫ���϶�ȡ��������ֱ������EAGAINΪֹ������ʣ�µ�����ֻ�����´ζԶ���д��ʱ����һ��ȡ�����ˡ�



����epoll��Ե����ģʽ��ET���µ�EPOLLOUT������

ETģʽ�£�EPOLLOUT���������У�
1.��������-->������������
2.ͬʱ����EPOLLOUT��EPOLLIN�¼�ʱ������IN �¼�����������˳��һ��OUT�¼���
3.һ���ͻ���connect������accept�ɹ���ᴥ��һ��OUT�¼���

����2�����˷ѽ⣬�ں˴��������ע�ͣ�˵��һ����IN ʱ����OUT����˳��һ��������˸��¼����� (15.11.30��֤���£�ȷʵ������)


���ϣ���ֻ����IN�¼����������ݺ��Ϊ����OUT�¼�����ʱ��ᷢ�ִ���OUT�¼��������㣬��Ҫǿ�ƴ�����������������һ��Ҫ������events������EPOLLOUT���ɡ�

��
һ����Ѷ��̨������������
ʹ��Linux epollģ�ͣ�ˮƽ����ģʽ����socket��дʱ���᲻ͣ�Ĵ���socket
��д���¼�����δ���


��һ�����ձ�ķ�ʽ��
��Ҫ��socketд���ݵ�ʱ��Ű�socket����epoll���ȴ���д�¼������ܵ���д�¼��󣬵���write����send�������ݡ����������ݶ�д��󣬰�socket�Ƴ�epoll��
���ַ�ʽ��ȱ���ǣ���ʹ���ͺ��ٵ����ݣ�ҲҪ��socket����epoll��д������Ƴ�epoll����һ���������ۡ�


һ�ָĽ��ķ�ʽ��
��ʼ����socket����epoll����Ҫ��socketд���ݵ�ʱ��ֱ�ӵ���write����send�������ݡ��������EAGAIN����socket����epoll����epoll
��������д���ݣ�ȫ�����ݷ�����Ϻ����Ƴ�epoll��
���ַ�ʽ���ŵ��ǣ����ݲ����ʱ����Ա���epoll���¼��������Ч�ʡ�
*/
ngx_int_t
ngx_handle_read_event(ngx_event_t *rev, ngx_uint_t flags, const char* func, int line) //recv��ȡ����NGX_AGAIN����Ҫ�ٴ�ngx_handle_read_event������fd��epoll����Ķ��¼�
{
    char tmpbuf[128];
    
    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) { //epoll���ش���etģʽ

        /* kqueue, epoll */

        if (!rev->active && !rev->ready) {
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_CLEAR_EVENT(et) read add", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, rev->log, 0, tmpbuf);
                       
            if (ngx_add_event(rev, NGX_READ_EVENT, NGX_CLEAR_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }
        }

        return NGX_OK;

    } else if (ngx_event_flags & NGX_USE_LEVEL_EVENT) { //epollˮƽ�ٷ�ģʽ

        /* select, poll, /dev/poll */

        if (!rev->active && !rev->ready) {
        
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_LEVEL_EVENT read add", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, rev->log, 0, tmpbuf);
            
            if (ngx_add_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (rev->active && (rev->ready || (flags & NGX_CLOSE_EVENT))) {
        
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_LEVEL_EVENT read del", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, rev->log, 0, tmpbuf);
            
            if (ngx_del_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT | flags)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

    } else if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {

        /* event ports */

        if (!rev->active && !rev->ready) {
            if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (rev->oneshot && !rev->ready) {
            if (ngx_del_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }
    }

    /* iocp */

    return NGX_OK;
}

/*
ngx_handle��write��event�����Ὣд������ӵ��¼�����ģ���С�wev��Ҫ�������¼�����lowat���ʾֻ�е����Ӷ�Ӧ���׽��ֻ������б�����
lowat��С�Ŀ��ÿռ�ʱ���¼��ռ�������select����epoll_wait���ã����ܴ��������д�¼���lowat����Ϊ0ʱ��ʾ������
��д�������Ĵ�С�����÷�������NGXһOK��ʾ�ɹ�������NGX ERROR��ʾʧ�ܡ�

EOIKK EPOLLOUT�¼�ֻ��������ʱ����һ�Σ���ʾ��д������ʱ����Ҫ����������Ҫ��׼��������������
1.ĳ��write��д���˷��ͻ����������ش�����ΪEAGAIN��
2.�Զ˶�ȡ��һЩ���ݣ������¿�д�ˣ���ʱ�ᴥ��EPOLLOUT��

����Linux Epoll ETģʽEPOLLOUT��EPOLLIN����ʱ�� ETģʽ��Ϊ��Ե����ģʽ������˼�壬������Ե��������������ᴥ���ġ�

EPOLLOUT�¼���
EPOLLOUT�¼�ֻ��������ʱ����һ�Σ���ʾ��д������ʱ����Ҫ����������Ҫ��׼��������������
1.ĳ��write��д���˷��ͻ����������ش�����ΪEAGAIN��
2.�Զ˶�ȡ��һЩ���ݣ������¿�д�ˣ���ʱ�ᴥ��EPOLLOUT��
�򵥵�˵��EPOLLOUT�¼�ֻ���ڲ���д����д��ת��ʱ�̣��Żᴥ��һ�Σ����Խб�Ե��������з�û��ģ�

��ʵ������������ǿ�ƴ���һ�Σ�Ҳ���а취�ģ�ֱ�ӵ���epoll_ctl��������һ��event�Ϳ����ˣ�event��ԭ��������һģһ�����У����������EPOLLOUT�����ؼ����������ã��ͻ����ϴ���һ��EPOLLOUT�¼���

EPOLLIN�¼���
EPOLLIN�¼���ֻ�е��Զ�������д��ʱ�Żᴥ�������Դ���һ�κ���Ҫ���϶�ȡ��������ֱ������EAGAINΪֹ������ʣ�µ�����ֻ�����´ζԶ���д��ʱ����һ��ȡ�����ˡ�


*/ ////write��ȡ����NGX_AGAIN����Ҫ�ٴ�ngx_handle_write_event������fd��epoll����Ķ��¼�
ngx_int_t
ngx_handle_write_event(ngx_event_t *wev, size_t lowat, const char* func, int line) 
{
    ngx_connection_t  *c;
    char tmpbuf[256];
    
    if (lowat) {
        c = wev->data;

        if (ngx_send_lowat(c, lowat) == NGX_ERROR) {
            return NGX_ERROR;
        }
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {

        /* kqueue, epoll */

        if (!wev->active && !wev->ready) {
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_CLEAR_EVENT write add", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, wev->log, 0, tmpbuf);
            
            if (ngx_add_event(wev, NGX_WRITE_EVENT,
                              NGX_CLEAR_EVENT | (lowat ? NGX_LOWAT_EVENT : 0))
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }
        }

        return NGX_OK;

    } else if (ngx_event_flags & NGX_USE_LEVEL_EVENT) {

        /* select, poll, /dev/poll */

        if (!wev->active && !wev->ready) {

            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_LEVEL_EVENT write add", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, wev->log, 0, tmpbuf);
            
            if (ngx_add_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (wev->active && wev->ready) {

            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_USE_LEVEL_EVENT write del", func, line);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, wev->log, 0, tmpbuf);
            
            if (ngx_del_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

    } else if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {

        /* event ports */

        if (!wev->active && !wev->ready) {
            if (ngx_add_event(wev, NGX_WRITE_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }

        if (wev->oneshot && wev->ready) {
            if (ngx_del_event(wev, NGX_WRITE_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }

            return NGX_OK;
        }
    }

    /* iocp */

    return NGX_OK;
}


static char *
ngx_event_init_conf(ngx_cycle_t *cycle, void *conf)
{
    if (ngx_get_conf(cycle->conf_ctx, ngx_events_module) == NULL) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "no \"events\" section in configuration");
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

/*
ngx_event_module_init������ʵ�ܼ򵥣�����Ҫ��ʼ����һЩ������������ngx_http_stub_status_moduleͳ��ģ��ʹ�õ�һЩԭ���Ե�ͳ�Ʊ���
*/
static ngx_int_t
ngx_event_module_init(ngx_cycle_t *cycle)
{
    void              ***cf;
    u_char              *shared;
    size_t               size, cl;
    ngx_shm_t            shm;
    ngx_time_t          *tp;
    ngx_core_conf_t     *ccf;
    ngx_event_conf_t    *ecf;

    cf = ngx_get_conf(cycle->conf_ctx, ngx_events_module);
    ecf = (*cf)[ngx_event_core_module.ctx_index];

    if (!ngx_test_config && ngx_process <= NGX_PROCESS_MASTER) {
        ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0,
                      "using the \"%s\" event method", ecf->name);
    }

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    ngx_timer_resolution = ccf->timer_resolution;

#if !(NGX_WIN32)
    {
        ngx_int_t      limit;
        struct rlimit  rlmt;

        if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) { // ÿ�������ܴ򿪵�����ļ����� 
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "getrlimit(RLIMIT_NOFILE) failed, ignored");

        } else {
            if (ecf->connections > (ngx_uint_t) rlmt.rlim_cur
                && (ccf->rlimit_nofile == NGX_CONF_UNSET
                    || ecf->connections > (ngx_uint_t) ccf->rlimit_nofile))
            {
                limit = (ccf->rlimit_nofile == NGX_CONF_UNSET) ?
                             (ngx_int_t) rlmt.rlim_cur : ccf->rlimit_nofile;

                ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                              "%ui worker_connections exceed "
                              "open file resource limit: %i",
                              ecf->connections, limit);
            }
        }
    }
#endif /* !(NGX_WIN32) */


    if (ccf->master == 0) {
        return NGX_OK;
    }

    if (ngx_accept_mutex_ptr) {
        return NGX_OK;
    }


    /* cl should be equal to or greater than cache line size */

/*
�������Ҫʹ�õĹ����ڴ�Ĵ�С��Ϊʲôÿ��ͳ�Ƴ�Ա��Ҫʹ��128�ֽ��أ����ƺ�̫���ˣ�����ȥ��ÿ��ngx_atomic_tԭ�ӱ��������Ҫ8��
�ڶ��ѡ���ʵ����ΪNginx��ֿ�����CPU�Ķ������档��Ŀǰ���CPU�ܹ��»����еĴ�С����128�ֽڣ���������Ҫͳ�Ƶı������Ƿ��ʷǳ�Ƶ
���ĳ�Ա��ͬʱ����ռ�õ��ڴ��ַǳ��٣����Բ�����ÿ����Ա��ʹ��128�ֽڴ�ŵ���ʽ�������ٶȸ���
*/
    cl = 128;

    size = cl            /* ngx_accept_mutex */
           + cl          /* ngx_connection_counter */
           + cl;         /* ngx_temp_number */

#if (NGX_STAT_STUB)

    size += cl           /* ngx_stat_accepted */
           + cl          /* ngx_stat_handled */
           + cl          /* ngx_stat_requests */
           + cl          /* ngx_stat_active */
           + cl          /* ngx_stat_reading */
           + cl          /* ngx_stat_writing */
           + cl;         /* ngx_stat_waiting */

#endif

    shm.size = size;
    shm.name.len = sizeof("nginx_shared_zone") - 1;
    shm.name.data = (u_char *) "nginx_shared_zone";
    shm.log = cycle->log;

    //����һ�鹲���ڴ棬�����ڴ�Ĵ�СΪshm.size
    if (ngx_shm_alloc(&shm) != NGX_OK) {
        return NGX_ERROR;
    }

    //�����ڴ���׵�ַ����shm.addr��Ա��
    shared = shm.addr;

    //ԭ�ӱ������͵�accept��ʹ����128�ֽڵĹ����ڴ�
    ngx_accept_mutex_ptr = (ngx_atomic_t *) shared;

    /*
     ngx_accept_mutex���Ǹ��ؾ�������spinֵΪ-1���Ǹ���Nginx�����������ʹ���̽���˯��״̬
     */
    ngx_accept_mutex.spin = (ngx_uint_t) -1;

    if (ngx_shmtx_create(&ngx_accept_mutex, (ngx_shmtx_sh_t *) shared,
                         cycle->lock_file.data)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    //ԭ��������͵�ngx_connection counter��ͳ�����н�������������������������������ӣ�
    ngx_connection_counter = (ngx_atomic_t *) (shared + 1 * cl);

    (void) ngx_atomic_cmp_set(ngx_connection_counter, 0, 1);

    ngx_log_debug2(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                   "counter: %p, %d",
                   ngx_connection_counter, *ngx_connection_counter);

    ngx_temp_number = (ngx_atomic_t *) (shared + 2 * cl);

    tp = ngx_timeofday();

    ngx_random_number = (tp->msec << 16) + ngx_pid;

#if (NGX_STAT_STUB)
    //���γ�ʼ����Ҫͳ�Ƶ�6��ԭ�ӱ�����Ҳ����ʹ�ù����ڴ���Ϊԭ�ӱ���
    ngx_stat_accepted = (ngx_atomic_t *) (shared + 3 * cl);
    ngx_stat_handled = (ngx_atomic_t *) (shared + 4 * cl);
    ngx_stat_requests = (ngx_atomic_t *) (shared + 5 * cl);
    ngx_stat_active = (ngx_atomic_t *) (shared + 6 * cl);
    ngx_stat_reading = (ngx_atomic_t *) (shared + 7 * cl);
    ngx_stat_writing = (ngx_atomic_t *) (shared + 8 * cl);
    ngx_stat_waiting = (ngx_atomic_t *) (shared + 9 * cl);

#endif

    return NGX_OK;
}


#if !(NGX_WIN32)

//ngx_event_timer_alarmֻ�Ǹ�ȫ�ֱ�����������Ϊlʱ����ʾ��Ҫ����ʱ�䡣
/*
��ngx_event_ actions t��process_events�����У�ÿһ���¼�����ģ�鶼��Ҫ��ngx_event_timer_alarmΪ1ʱ��
��ngx_time_update������������ϵͳʱ�䣬�ڸ���ϵͳ��������Ҫ��ngx_event_timer_alarm��Ϊ0��
*/ //��ʱ����ʱ����epoll_wait���أ����ش����Ż�ִ��timer��ʱhandler  ngx_timer_signal_handler
static void
ngx_timer_signal_handler(int signo)
{
    ngx_event_timer_alarm = 1;
#if 1
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, "timer signal");
#endif
}

#endif
//�ڴ����ӽ��̵�����ִ��  ngx_worker_process_init��
static ngx_int_t
ngx_event_process_init(ngx_cycle_t *cycle)
{
    ngx_uint_t           m, i;
    ngx_event_t         *rev, *wev;
    ngx_listening_t     *ls;
    ngx_connection_t    *c, *next, *old;
    ngx_core_conf_t     *ccf;
    ngx_event_conf_t    *ecf;
    ngx_event_module_t  *module;

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);
    ecf = ngx_event_get_conf(cycle->conf_ctx, ngx_event_core_module);

    /*
         ����accept_mutex���ؾ�������ͬʱʹ����masterģʽ����worker�ų���������1ʱ������ʽȷ���˽��̽�ʹ��accept_mutex���ؾ�������
     ��ˣ���ʹ�����������ļ���ָ����accept_mutex�������û��ʹ��masterģʽ����worker������������1������������ʱ���ǲ���ʹ��
     ���ؾ���������Ȼ�����ڶ������ȥ��һ�������˿��ϵ����ӵ��������ô��Ȼ����Ҫ������worker���̵ĸ��أ���
         ��ʱ�Ὣngx_use_accept_mutexȫ�ֱ�����Ϊ1��ngx_accept_mutex_held��־��Ϊ0��ngx_accept_mutex_delay����Ϊ�������ļ���ָ��������ӳ�ʱ�䡣
     */
    if (ccf->master && ccf->worker_processes > 1 && ecf->accept_mutex) {
        ngx_use_accept_mutex = 1;
        ngx_accept_mutex_held = 0;
        ngx_accept_mutex_delay = ecf->accept_mutex_delay;

    } else {
        ngx_use_accept_mutex = 0;
    }

#if (NGX_WIN32)

    /*
     * disable accept mutex on win32 as it may cause deadlock if
     * grabbed by a process which can't accept connections
     */

    ngx_use_accept_mutex = 0;

#endif

    ngx_queue_init(&ngx_posted_accept_events);
    ngx_queue_init(&ngx_posted_events);

    //��ʼ�������ʵ�ֵĶ�ʱ����
    if (ngx_event_timer_init(cycle->log) == NGX_ERROR) {
        return NGX_ERROR;
    }

    //�ڵ���use������ָ�����¼�ģ���У���ngx_event_module_t�ӿ��£�ngx_event_actions_t�е�init������������¼�ģ��ĳ�ʼ��������
    for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != NGX_EVENT_MODULE) {
            continue;
        }

        if (ngx_modules[m]->ctx_index != ecf->use) { //�ҵ�epoll����select��moduleģ��
            continue;
        }

        module = ngx_modules[m]->ctx;

        if (module->actions.init(cycle, ngx_timer_resolution) != NGX_OK) { //ִ��epoll module�е�ngx_epoll_init
            /* fatal */
            exit(2);
        }

        break; /*����ѭ����ֻ����ʹ��һ��������¼�ģ��*/  
    }

#if !(NGX_WIN32)
    /*
    ���nginx.conf�����ļ���������timer_resolution�������������Ҫ����ʱ�侫�ȣ���ʱ�����setitimer����������ʱ����
    Ϊtimer_resolution�������ص�ngx_timer_signal_handler����
     */
    if (ngx_timer_resolution && !(ngx_event_flags & NGX_USE_TIMER_EVENT)) {
        struct sigaction  sa;
        struct itimerval  itv;
        
        //���ö�ʱ��
        /*
            ��ngx_event_ actions t��process_events�����У�ÿһ���¼�����ģ�鶼��Ҫ��ngx_event_timer_alarmΪ1ʱ��
            ��ngx_time_update������������ϵͳʱ�䣬�ڸ���ϵͳ��������Ҫ��ngx_event_timer_alarm��Ϊ0��
          */
        ngx_memzero(&sa, sizeof(struct sigaction)); //ÿ��ngx_timer_resolution ms�ᳬʱִ��handle
        sa.sa_handler = ngx_timer_signal_handler;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGALRM, &sa, NULL) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "sigaction(SIGALRM) failed");
            return NGX_ERROR;
        }

        itv.it_interval.tv_sec = ngx_timer_resolution / 1000;
        itv.it_interval.tv_usec = (ngx_timer_resolution % 1000) * 1000;
        itv.it_value.tv_sec = ngx_timer_resolution / 1000;
        itv.it_value.tv_usec = (ngx_timer_resolution % 1000 ) * 1000;

        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setitimer() failed");
        }
    }

    /* 
     ���ʹ����epoll�¼�����ģʽ����ô��Ϊngx_cycle_t�ṹ���е�files��ԱԤ����Ѯ����
     */
    if (ngx_event_flags & NGX_USE_FD_EVENT) {
        struct rlimit  rlmt;

        if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "getrlimit(RLIMIT_NOFILE) failed");
            return NGX_ERROR;
        }

        cycle->files_n = (ngx_uint_t) rlmt.rlim_cur; //ÿ�������ܹ��򿪵�����ļ���

        cycle->files = ngx_calloc(sizeof(ngx_connection_t *) * cycle->files_n,
                                  cycle->log);
        if (cycle->files == NULL) {
            return NGX_ERROR;
        }
    }

#endif

    cycle->connections =
        ngx_alloc(sizeof(ngx_connection_t) * cycle->connection_n, cycle->log);
    if (cycle->connections == NULL) {
        return NGX_ERROR;
    }

    c = cycle->connections;

    cycle->read_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
                                   cycle->log);
    if (cycle->read_events == NULL) {
        return NGX_ERROR;
    }

    rev = cycle->read_events;
    for (i = 0; i < cycle->connection_n; i++) {
        rev[i].closed = 1;
        rev[i].instance = 1;
    }

    cycle->write_events = ngx_alloc(sizeof(ngx_event_t) * cycle->connection_n,
                                    cycle->log);
    if (cycle->write_events == NULL) {
        return NGX_ERROR;
    }

    wev = cycle->write_events;
    for (i = 0; i < cycle->connection_n; i++) {
        wev[i].closed = 1;
    }

    i = cycle->connection_n;
    next = NULL;

    /*
    ������ţ�������3��������Ӧ�Ķ�/д�¼����õ�ÿһ��ngx_connection_t���Ӷ����У�ͬʱ����Щ������ngx_connection_t�е�data��Ա
    ��Ϊnextָ�봮��������Ϊ��һ�����ÿ���������������׼��
     */
    do {
        i--;

        c[i].data = next;
        c[i].read = &cycle->read_events[i];
        c[i].write = &cycle->write_events[i];
        c[i].fd = (ngx_socket_t) -1;

        next = &c[i];
    } while (i);

    /*
    ��ngx_cycle_t�ṹ���еĿ�����������free_connectionsָ��connections��������1��Ԫ�أ�Ҳ���ǵ�10������ngx_connection_t��
    ��ͨ��data��Ա��ɵĵ�������ײ���
     */
    cycle->free_connections = next;
    cycle->free_connection_n = cycle->connection_n;

    /* for each listening socket */
    /*
     �ڸոս����õ����ӳ��У�Ϊ����ngx_listening_t���������е�connection��Ա�������ӣ�ͬʱ�Լ����˿ڵĶ��¼����ô�����
     Ϊngx_event_accept��Ҳ����˵�����������¼�ʱ������ngx_event_accept�������������ӣ�����
     */
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) { 

#if (NGX_HAVE_REUSEPORT)
        //master����ִ��ngx_clone_listening����������˶�worker������80�˿ڻ���worker��listen��ֵ��master������ngx_open_listening_sockets
        //�л����80�˿�worker�Σ���ô�ӽ��̴��������󣬲���ÿ���ֽ��̶���ע��worker��� listen�¼�����?Ϊ�˱���������⣬nginxͨ��
        //���ӽ�������ngx_event_process_init������ʱ��ͨ��ngx_add_event�������ӽ��̹�ע��listen������ʵ��ֻ��עmaster�����д�����һ��listen�¼�
        if (ls[i].reuseport && ls[i].worker != ngx_worker) {
            continue;
        }
#endif
        
        c = ngx_get_connection(ls[i].fd, cycle->log); //�����ӳ��л�ȡһ��ngx_connection_t

        if (c == NULL) {
            return NGX_ERROR;
        }

        c->log = &ls[i].log;

        c->listening = &ls[i]; //�ѽ�����listen��������Ϣ��ֵ��ngx_connection_s�е�listening��
        ls[i].connection = c;

        rev = c->read;

        rev->log = c->log;
        rev->accept = 1;

#if (NGX_HAVE_DEFERRED_ACCEPT)
        rev->deferred_accept = ls[i].deferred_accept;
#endif

        if (!(ngx_event_flags & NGX_USE_IOCP_EVENT)) {
            if (ls[i].previous) {

                /*
                 * delete the old accept events that were bound to
                 * the old cycle read events array
                 */

                old = ls[i].previous->connection;

                if (ngx_del_event(old->read, NGX_READ_EVENT, NGX_CLOSE_EVENT)
                    == NGX_ERROR)
                {
                    return NGX_ERROR;
                }

                old->fd = (ngx_socket_t) -1;
            }
        }

#if (NGX_WIN32)

        if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
            ngx_iocp_conf_t  *iocpcf;
            rev->handler = ngx_event_acceptex;

            if (ngx_use_accept_mutex) {
                continue;
            }

            if (ngx_add_event(rev, 0, NGX_IOCP_ACCEPT) == NGX_ERROR) {
                return NGX_ERROR;
            }

            ls[i].log.handler = ngx_acceptex_log_error;

            iocpcf = ngx_event_get_conf(cycle->conf_ctx, ngx_iocp_module);
            if (ngx_event_post_acceptex(&ls[i], iocpcf->post_acceptex)
                == NGX_ERROR)
            {
                return NGX_ERROR;
            }

        } else {
            rev->handler = ngx_event_accept;

            if (ngx_use_accept_mutex) {
                continue;
            }

            if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return NGX_ERROR;
            }
        }

#else
        /*
        �Լ����˿ڵĶ��¼����ô�����
        Ϊngx_event_accept��Ҳ����˵�����������¼�ʱ������ngx_event_accept��������������
          */
        rev->handler = ngx_event_accept; 

        /* 
          ʹ����accept_mutex����ʱ���������׽��ַ���epoll��, ���ǵȵ�worker����accept��������ٷ���epoll�����⾪Ⱥ�ķ����� 
          */ //�ڽ����ӵ�ʱ��Ϊ�˱��⾪Ⱥ����accept��ʱ��ֻ�л�ȡ����ԭ�������Ű�accept��ӵ�epoll�¼��У���ngx_process_events_and_timers->ngx_trylock_accept_mutex
        if (ngx_use_accept_mutex
#if (NGX_HAVE_REUSEPORT)
            && !ls[i].reuseport
#endif
           ) //����ǵ����̷�ʽ
        {
            continue;
        }

        /*
          �������������ӵĶ��¼���ӵ��¼�����ģ���У�������epoll���¼�ģ��Ϳ�ʼ���������񣬲���ʼ���û��ṩ�����ˡ�
          */ //���ngx_use_accept_mutexΪ0Ҳ����δ����accept_mutex��������ngx_worker_process_init->ngx_event_process_init �а�accept���Ӷ��¼�ͳ�Ƶ�epoll��
          //������ngx_process_events_and_timers->ngx_process_events_and_timers->ngx_trylock_accept_mutex�а�accept���Ӷ��¼�ͳ�Ƶ�epoll��

        char tmpbuf[256];
        
        snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_READ_EVENT(et) read add", NGX_FUNC_LINE);
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, tmpbuf);
        if (ngx_add_event(rev, NGX_READ_EVENT, 0) == NGX_ERROR) { //�����epoll��Ϊngx_epoll_add_event
            return NGX_ERROR;
        }

#endif  

    }

    return NGX_OK;
}


ngx_int_t
ngx_send_lowat(ngx_connection_t *c, size_t lowat)
{
    int  sndlowat;

#if (NGX_HAVE_LOWAT_EVENT)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {
        c->write->available = lowat;
        return NGX_OK;
    }

#endif

    if (lowat == 0 || c->sndlowat) {
        return NGX_OK;
    }

    sndlowat = (int) lowat;
    /*
     SO_RCVLOWAT SO_SNDLOWAT 
     ÿ���׽ӿڶ���һ�����յͳ��޶Ⱥ�һ�����͵ͳ��޶ȡ�
     ���յͳ��޶ȣ�����TCP�׽ӿڶ��ԣ����ջ������е����ݱ���ﵽ�涨�������ں˲�֪ͨ���̡��ɶ��������紥��select����epoll�����ء��׽ӿڿɶ�����
     ���͵ͳ��޶ȣ�����TCP�׽ӿڶ��ԣ��ͽ��յͳ��޶�һ������
     */
    if (setsockopt(c->fd, SOL_SOCKET, SO_SNDLOWAT,
                   (const void *) &sndlowat, sizeof(int))
        == -1)
    {
        ngx_connection_error(c, ngx_socket_errno,
                             "setsockopt(SO_SNDLOWAT) failed");
        return NGX_ERROR;
    }

    c->sndlowat = 1;

    return NGX_OK;
}

static char *
ngx_events_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char                 *rv;
    void               ***ctx;
    ngx_uint_t            i;
    ngx_conf_t            pcf;
    ngx_event_module_t   *m;

    if (*(void **) conf) {
        return "is duplicate";
    }

    /* count the number of the event modules and set up their indices */

    ngx_event_max_module = 0;
    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_EVENT_MODULE) {
            continue;
        }

        ngx_modules[i]->ctx_index = ngx_event_max_module++;
    }

    ctx = ngx_pcalloc(cf->pool, sizeof(void *));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    *ctx = ngx_pcalloc(cf->pool, ngx_event_max_module * sizeof(void *));
    if (*ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    //confΪngx_conf_handler�е�conf = confp[ngx_modules[i]->ctx_index];Ҳ����confָ�����ngx_cycle_s->conf_ctx[]��
    //���Զ�conf��ֵ���Ƕ�ngx_cycle_s�е�conf_ctx��ֵ,���վ���ngx_cycle_s�е�conf_ctx[ngx_events_module=>index]ָ����ctx
    *(void **) conf = ctx;

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_EVENT_MODULE) {
            continue;
        }

        m = ngx_modules[i]->ctx;

        if (m->create_conf) {
            (*ctx)[ngx_modules[i]->ctx_index] = m->create_conf(cf->cycle);
            if ((*ctx)[ngx_modules[i]->ctx_index] == NULL) {
                return NGX_CONF_ERROR;
            }
        }
    }

    //��ʱ����֮ǰ��cf,�����������event{}���ú��ڻָ�
    pcf = *cf;
    cf->ctx = ctx;
    cf->module_type = NGX_EVENT_MODULE;
    cf->cmd_type = NGX_EVENT_CONF;

    rv = ngx_conf_parse(cf, NULL);//��ʱ��cf�����������ctxΪNGX_EVENT_MODULEģ��create_conf�����ڴ洢event{}�Ŀռ�

    *cf = pcf; //������event{}���ú󣬻ָ�

    if (rv != NGX_CONF_OK) {
        return rv;
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_EVENT_MODULE) {
            continue;
        }

        m = ngx_modules[i]->ctx;

        if (m->init_conf) {
            rv = m->init_conf(cf->cycle, (*ctx)[ngx_modules[i]->ctx_index]);
            if (rv != NGX_CONF_OK) {
                return rv;
            }
        }
    }

    return NGX_CONF_OK;
}


static char *
ngx_event_connections(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

    ngx_str_t  *value;

    if (ecf->connections != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;
    ecf->connections = ngx_atoi(value[1].data, value[1].len);
    if (ecf->connections == (ngx_uint_t) NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid number \"%V\"", &value[1]);

        return NGX_CONF_ERROR;
    }

    cf->cycle->connection_n = ecf->connections;

    return NGX_CONF_OK;
}


static char *
ngx_event_use(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

    ngx_int_t             m;
    ngx_str_t            *value;
    ngx_event_conf_t     *old_ecf;
    ngx_event_module_t   *module;

    if (ecf->use != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (cf->cycle->old_cycle->conf_ctx) {
        old_ecf = ngx_event_get_conf(cf->cycle->old_cycle->conf_ctx,
                                     ngx_event_core_module);
    } else {
        old_ecf = NULL;
    }


    for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != NGX_EVENT_MODULE) {
            continue;
        }

        module = ngx_modules[m]->ctx;
        if (module->name->len == value[1].len) {
            if (ngx_strcmp(module->name->data, value[1].data) == 0) {
                ecf->use = ngx_modules[m]->ctx_index;
                ecf->name = module->name->data;

                if (ngx_process == NGX_PROCESS_SINGLE
                    && old_ecf
                    && old_ecf->use != ecf->use)
                {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "when the server runs without a master process "
                               "the \"%V\" event type must be the same as "
                               "in previous configuration - \"%s\" "
                               "and it cannot be changed on the fly, "
                               "to change it you need to stop server "
                               "and start it again",
                               &value[1], old_ecf->name);

                    return NGX_CONF_ERROR;
                }

                return NGX_CONF_OK;
            }
        }
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid event type \"%V\"", &value[1]);

    return NGX_CONF_ERROR;
}


static char *
ngx_event_debug_connection(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
#if (NGX_DEBUG)
    ngx_event_conf_t  *ecf = conf;

    ngx_int_t             rc;
    ngx_str_t            *value;
    ngx_url_t             u;
    ngx_cidr_t            c, *cidr;
    ngx_uint_t            i;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    value = cf->args->elts;

#if (NGX_HAVE_UNIX_DOMAIN)

    if (ngx_strcmp(value[1].data, "unix:") == 0) {
        cidr = ngx_array_push(&ecf->debug_connection);
        if (cidr == NULL) {
            return NGX_CONF_ERROR;
        }

        cidr->family = AF_UNIX;
        return NGX_CONF_OK;
    }

#endif

    rc = ngx_ptocidr(&value[1], &c);

    if (rc != NGX_ERROR) {
        if (rc == NGX_DONE) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "low address bits of %V are meaningless",
                               &value[1]);
        }

        cidr = ngx_array_push(&ecf->debug_connection);
        if (cidr == NULL) {
            return NGX_CONF_ERROR;
        }

        *cidr = c;

        return NGX_CONF_OK;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));
    u.host = value[1];

    if (ngx_inet_resolve_host(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "%s in debug_connection \"%V\"",
                               u.err, &u.host);
        }

        return NGX_CONF_ERROR;
    }

    cidr = ngx_array_push_n(&ecf->debug_connection, u.naddrs);
    if (cidr == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(cidr, u.naddrs * sizeof(ngx_cidr_t));

    for (i = 0; i < u.naddrs; i++) {
        cidr[i].family = u.addrs[i].sockaddr->sa_family;

        switch (cidr[i].family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            sin6 = (struct sockaddr_in6 *) u.addrs[i].sockaddr;
            cidr[i].u.in6.addr = sin6->sin6_addr;
            ngx_memset(cidr[i].u.in6.mask.s6_addr, 0xff, 16);
            break;
#endif

        default: /* AF_INET */
            sin = (struct sockaddr_in *) u.addrs[i].sockaddr;
            cidr[i].u.in.addr = sin->sin_addr.s_addr;
            cidr[i].u.in.mask = 0xffffffff;
            break;
        }
    }

#else

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "\"debug_connection\" is ignored, you need to rebuild "
                       "nginx using --with-debug option to enable it");

#endif

    return NGX_CONF_OK;
}


static void *
ngx_event_core_create_conf(ngx_cycle_t *cycle)
{
    ngx_event_conf_t  *ecf;

    ecf = ngx_palloc(cycle->pool, sizeof(ngx_event_conf_t));
    if (ecf == NULL) {
        return NULL;
    }

    ecf->connections = NGX_CONF_UNSET_UINT;
    ecf->use = NGX_CONF_UNSET_UINT;
    ecf->multi_accept = NGX_CONF_UNSET;
    ecf->accept_mutex = NGX_CONF_UNSET;
    ecf->accept_mutex_delay = NGX_CONF_UNSET_MSEC;
    ecf->name = (void *) NGX_CONF_UNSET;

#if (NGX_DEBUG)

    if (ngx_array_init(&ecf->debug_connection, cycle->pool, 4,
                       sizeof(ngx_cidr_t)) == NGX_ERROR)
    {
        return NULL;
    }

#endif

    return ecf;
}


static char *
ngx_event_core_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_event_conf_t  *ecf = conf;

#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)
    int                  fd;
#endif
    ngx_int_t            i;
    ngx_module_t        *module;
    ngx_event_module_t  *event_module;

    module = NULL;

#if (NGX_HAVE_EPOLL) && !(NGX_TEST_BUILD_EPOLL)

    fd = epoll_create(100);

    if (fd != -1) {
        (void) close(fd);
        module = &ngx_epoll_module;

    } else if (ngx_errno != NGX_ENOSYS) {
        module = &ngx_epoll_module;
    }

#endif

#if (NGX_HAVE_DEVPOLL)

    module = &ngx_devpoll_module;

#endif

#if (NGX_HAVE_KQUEUE)

    module = &ngx_kqueue_module;

#endif

#if (NGX_HAVE_SELECT)

    if (module == NULL) {
        module = &ngx_select_module;
    }

#endif

    //ngx_event_core_module��ĵ�һ��NGX_EVENT_MODULEҲ����ngx_epoll_moduleĬ����Ϊ��һ��eventģ��
    if (module == NULL) {
        for (i = 0; ngx_modules[i]; i++) {

            if (ngx_modules[i]->type != NGX_EVENT_MODULE) {
                continue;
            }

            event_module = ngx_modules[i]->ctx;

            if (ngx_strcmp(event_module->name->data, event_core_name.data) == 0) //����Ϊngx_event_core_module
            {
                continue;
            }

            module = ngx_modules[i];
            break;
        }
    }

    if (module == NULL) {
        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0, "no events module found");
        return NGX_CONF_ERROR;
    }

    ngx_conf_init_uint_value(ecf->connections, DEFAULT_CONNECTIONS);
    cycle->connection_n = ecf->connections;

    ngx_conf_init_uint_value(ecf->use, module->ctx_index);

    event_module = module->ctx;
    ngx_conf_init_ptr_value(ecf->name, event_module->name->data);

    ngx_conf_init_value(ecf->multi_accept, 0);
    ngx_conf_init_value(ecf->accept_mutex, 1);
    ngx_conf_init_msec_value(ecf->accept_mutex_delay, 500);

    return NGX_CONF_OK;
}
