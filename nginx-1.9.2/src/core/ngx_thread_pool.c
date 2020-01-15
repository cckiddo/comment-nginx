
/*
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Valentin V. Bartenev
 * Copyright (C) Ruslan Ermilov
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_thread_pool.h>


typedef struct {
    //�����е�ÿ����Ա��ngx_thread_pool_init�г�ʼ����
    //�����Ա�ṹʽngx_thread_pool_t
    ngx_array_t               pools; //���е�thread_pool name������Ϣngx_thread_pool_t����ڸ������У���ngx_thread_pool_add
} ngx_thread_pool_conf_t;//�����ռ���ngx_thread_pool_create_conf


typedef struct { //��ngx_thread_pool_done
    ngx_thread_task_t        *first;

    /*
     *ngx_thread_pool_t->queue.last = task;  ����ӵ�����ͨ��last������һ��
     ngx_thread_pool_t->queue.last = &task->next;  �´���������������task->nextָ����������
     */
    ngx_thread_task_t       **last;
} ngx_thread_pool_queue_t; //�̳߳ض���  ��ʼ����ngx_thread_pool_queue_init

#define ngx_thread_pool_queue_init(q)                                         \
    (q)->first = NULL;                                                        \
    (q)->last = &(q)->first

//һ���ýṹ��Ӧһ��threads_pool����
struct ngx_thread_pool_s {//�ýṹʽ�����ngx_thread_pool_conf_t->pool�����еģ���ngx_thread_pool_init_worker
    ngx_thread_mutex_t        mtx; //�߳���  ngx_thread_pool_init�г�ʼ��
    //ngx_thread_task_post����ӵ�������ӵ��ö�����
    ngx_thread_pool_queue_t   queue;//ngx_thread_pool_init  ngx_thread_pool_queue_init�г�ʼ��
    //�ڸ��̳߳�poll��ÿ���һ���̣߳�waiting�Ӽ������߳�ȫ������ִ�������waiting��ָ���0
    //��������̶߳��Ѿ���ִ������(Ҳ����waiting>-0)��������������ô�����ֻ�ܵȴ�������waiting��ʾ�ȴ���ִ�е�������
    ngx_int_t                 waiting;//�ȴ���������   ngx_thread_task_post��1   ngx_thread_pool_cycle��1
    ngx_thread_cond_t         cond;//��������  ngx_thread_pool_init�г�ʼ��

    ngx_log_t                *log;//ngx_thread_pool_init�г�ʼ��

    ngx_str_t                 name;//thread_pool name threads=number [max_queue=number];�е�name  ngx_thread_pool
    //���û�����ã���ngx_thread_pool_init_confĬ�ϸ�ֵΪ32
    ngx_uint_t                threads;//thread_pool name threads=number [max_queue=number];�е�number  ngx_thread_pool
    //���û�����ã���ngx_thread_pool_init_confĬ�ϸ�ֵΪ65535  
    //ָ�����߳��Ѿ�ȫ�����������£���������Ӷ��ٸ����񵽵ȴ�����
    ngx_int_t                 max_queue;//thread_pool name threads=number [max_queue=number];�е�max_queue  ngx_thread_pool

    u_char                   *file;//�����ļ���
    ngx_uint_t                line;//thread_pool�����������ļ��е��к�
};


static ngx_int_t ngx_thread_pool_init(ngx_thread_pool_t *tp, ngx_log_t *log,
    ngx_pool_t *pool);
static void ngx_thread_pool_destroy(ngx_thread_pool_t *tp);
static void ngx_thread_pool_exit_handler(void *data, ngx_log_t *log);

static void *ngx_thread_pool_cycle(void *data);
static void ngx_thread_pool_handler(ngx_event_t *ev);

static char *ngx_thread_pool(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_thread_pool_create_conf(ngx_cycle_t *cycle);
static char *ngx_thread_pool_init_conf(ngx_cycle_t *cycle, void *conf);

static ngx_int_t ngx_thread_pool_init_worker(ngx_cycle_t *cycle);
static void ngx_thread_pool_exit_worker(ngx_cycle_t *cycle);


static ngx_command_t  ngx_thread_pool_commands[] = {
    /*
    Syntax: thread_pool name threads=number [max_queue=number];
    Default: thread_pool default threads=32 max_queue=65536; threads����Ϊ��pool���̸߳�����max_queue��ʾ�ȴ����̵߳��ȵ�������
    
    Defines named thread pools used for multi-threaded reading and sending of files without blocking worker processes. 
    The threads parameter defines the number of threads in the pool. 
    In the event that all threads in the pool are busy, a new task will wait in the queue. The max_queue parameter limits the 
    number of tasks allowed to be waiting in the queue. By default, up to 65536 tasks can wait in the queue. When the queue 
    overflows, the task is completed with an error. 
    */
    { ngx_string("thread_pool"),
      NGX_MAIN_CONF|NGX_DIRECT_CONF|NGX_CONF_TAKE23,
      ngx_thread_pool,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_core_module_t  ngx_thread_pool_module_ctx = {
    ngx_string("thread_pool"),
    ngx_thread_pool_create_conf,
    ngx_thread_pool_init_conf
};


ngx_module_t  ngx_thread_pool_module = {
    NGX_MODULE_V1,
    &ngx_thread_pool_module_ctx,           /* module context */
    ngx_thread_pool_commands,              /* module directives */
    NGX_CORE_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    ngx_thread_pool_init_worker,           /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_thread_pool_exit_worker,           /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_str_t  ngx_thread_pool_default = ngx_string("default");

static ngx_uint_t               ngx_thread_pool_task_id;
static ngx_atomic_t             ngx_thread_pool_done_lock;//Ϊ0�����Ի�ȡ������Ϊ0�����ܻ�ȡ������
static ngx_thread_pool_queue_t  ngx_thread_pool_done; //���е�

//����thread_pool name threads=number [max_queue=number];�е�number��������ô����߳�
static ngx_int_t
ngx_thread_pool_init(ngx_thread_pool_t *tp, ngx_log_t *log, ngx_pool_t *pool)
{
    int             err;
    pthread_t       tid;
    ngx_uint_t      n;
    pthread_attr_t  attr;

    if (ngx_notify == NULL) {
        ngx_log_error(NGX_LOG_ALERT, log, 0,
               "the configured event method cannot be used with thread pools");
        return NGX_ERROR;
    }

    ngx_thread_pool_queue_init(&tp->queue);

    if (ngx_thread_mutex_create(&tp->mtx, log) != NGX_OK) {
        return NGX_ERROR;
    }

    if (ngx_thread_cond_create(&tp->cond, log) != NGX_OK) {
        (void) ngx_thread_mutex_destroy(&tp->mtx, log);
        return NGX_ERROR;
    }

    tp->log = log;

    err = pthread_attr_init(&attr);
    if (err) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_attr_init() failed");
        return NGX_ERROR;
    }

#if 0
    err = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (err) {
        ngx_log_error(NGX_LOG_ALERT, log, err,
                      "pthread_attr_setstacksize() failed");
        return NGX_ERROR;
    }
#endif

    for (n = 0; n < tp->threads; n++) {
        /*
        �߳�ԭ�pthread_create()��pthread_self()��pthread_exit(),pthread_join(),pthread_cancel(),pthread_detach( .
        �õ��߳�����ȫ�ο�(��ͼ�����ӣ��ܺ�):http://blog.csdn.net/tototuzuoquan/article/details/39553427
         */
        err = pthread_create(&tid, &attr, ngx_thread_pool_cycle, tp);
        if (err) {
            ngx_log_error(NGX_LOG_ALERT, log, err,
                          "pthread_create() failed");
            return NGX_ERROR;
        }
    }

    (void) pthread_attr_destroy(&attr);

    return NGX_OK;
}


static void
ngx_thread_pool_destroy(ngx_thread_pool_t *tp)
{
    ngx_uint_t           n;
    ngx_thread_task_t    task;
    volatile ngx_uint_t  lock;

    ngx_memzero(&task, sizeof(ngx_thread_task_t));

    task.handler = ngx_thread_pool_exit_handler;//�����˳�ִ�к���
    task.ctx = (void *) &lock;//ָ����Ĳ���  

    // û�и�ֵtask->event.handler  task->event.data   (void) ngx_notify(ngx_thread_pool_handler); �л᲻��δ���??? /
    for (n = 0; n < tp->threads; n++) {  //tp�����е��̳߳���Ӹ�����  
        lock = 1;

        if (ngx_thread_task_post(tp, &task) != NGX_OK) {
            return;
        }

        while (lock) { //�������ж����lockû�иı䣬����CPU�������߳�ִ�У��Դ˵ȴ����൱��pthread_join  
            ngx_sched_yield();
        }

        //ֻ���̳߳��е�һ���߳�ִ����exit_handler����ܻ����forѭ��

        task.event.active = 0;
    }

    //��ʱ����ߣ����е��̶߳��Ѿ��˳�   //������������   ������

    (void) ngx_thread_cond_destroy(&tp->cond, tp->log);

    (void) ngx_thread_mutex_destroy(&tp->mtx, tp->log);
}


static void
ngx_thread_pool_exit_handler(void *data, ngx_log_t *log)
{
    ngx_uint_t *lock = data;

    *lock = 0;

    pthread_exit(0);
}


ngx_thread_task_t *
ngx_thread_task_alloc(ngx_pool_t *pool, size_t size)
{
    ngx_thread_task_t  *task;

    task = ngx_pcalloc(pool, sizeof(ngx_thread_task_t) + size);
    if (task == NULL) {
        return NULL;
    }

    task->ctx = task + 1;

    return task;
}

/*
һ�����߳�ͨ��ngx_thread_task_post��������̶߳����е��߳�ִ���������̺߳źͽ��̺�һ�����̶߳����е��̺߳ͽ��̺Ų�һ��������
2016/01/07 12:38:01[               ngx_thread_task_post,   280][yangya  [debug] 20090#20090: ngx add task to thread, task id:183
20090#20090ǰ���ǽ��̺ţ����������̺߳ţ�������ͬ
*/
//������ӵ���Ӧ���̳߳����������
ngx_int_t //ngx_thread_pool_cycle��ngx_thread_task_post����Ķ�
ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task)
{
    if (task->event.active) {
        ngx_log_error(NGX_LOG_ALERT, tp->log, 0,
                      "task #%ui already active", task->id);
        return NGX_ERROR;
    }
    
    if (ngx_thread_mutex_lock(&tp->mtx, tp->log) != NGX_OK) {
        return NGX_ERROR;
    }

    if (tp->waiting >= tp->max_queue) {
        (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);

        ngx_log_error(NGX_LOG_ERR, tp->log, 0,
                      "thread pool \"%V\" queue overflow: %i tasks waiting",
                      &tp->name, tp->waiting);
        return NGX_ERROR;
    }

    task->event.active = 1;

    task->id = ngx_thread_pool_task_id++;
    task->next = NULL;
    ngx_log_debugall(tp->log, 0, "ngx add task to thread, task id:%ui", task->id);

    if (ngx_thread_cond_signal(&tp->cond, tp->log) != NGX_OK) {
        (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);
        return NGX_ERROR;
    }

    //��ӵ��������
    *tp->queue.last = task;
    tp->queue.last = &task->next;

    tp->waiting++;

    (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);

    ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
                   "task #%ui added to thread pool name: \"%V\" complete",
                   task->id, &tp->name);

    return NGX_OK;
}

//ngx_thread_pool_cycle��ngx_thread_task_post����Ķ�
static void *
ngx_thread_pool_cycle(void *data)
{
    ngx_thread_pool_t *tp = data; //һ���ýṹ��Ӧһ��threads_pool����

    int                 err;
    sigset_t            set;
    ngx_thread_task_t  *task;

#if 0
    ngx_time_update();
#endif

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, tp->log, 0,
                   "thread in pool \"%V\" started", &tp->name);

    sigfillset(&set);

    sigdelset(&set, SIGILL);
    sigdelset(&set, SIGFPE);
    sigdelset(&set, SIGSEGV);
    sigdelset(&set, SIGBUS);

    err = pthread_sigmask(SIG_BLOCK, &set, NULL); //
    if (err) {
        ngx_log_error(NGX_LOG_ALERT, tp->log, err, "pthread_sigmask() failed");
        return NULL;
    }

    /*
    һ�����߳�ͨ��ngx_thread_task_post��������̶߳����е��߳�ִ���������̺߳źͽ��̺�һ�����̶߳����е��̺߳ͽ��̺Ų�һ��������
    2016/01/07 12:38:01[               ngx_thread_task_post,   280][yangya  [debug] 20090#20090: ngx add task to thread, task id:183
    20090#20090ǰ���ǽ��̺ţ����������̺߳ţ�������ͬ
    */
    for ( ;; ) {//һ������ִ������ֻ��ߵ����ѭ��
        if (ngx_thread_mutex_lock(&tp->mtx, tp->log) != NGX_OK) {
            return NULL;
        }

        /* the number may become negative */
        tp->waiting--;

        //�����������������ֱ��ִ�����񣬲�����while�еȴ�conf signal
        while (tp->queue.first == NULL) { //��ʱ�������Ϊ�գ������������ϵȴ�  ���ngx_thread_task_post�Ķ�
            //����������ʱ����ngx_thread_task_post -> ngx_thread_cond_signal
            if (ngx_thread_cond_wait(&tp->cond, &tp->mtx, tp->log) //�ȴ�ngx_thread_cond_signal��Ż᷵��
                != NGX_OK)
            {
                (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);
                return NULL;
            }
        }

        //ȡ����������Ȼ��ִ��
        task = tp->queue.first;
        tp->queue.first = task->next;

        if (tp->queue.first == NULL) {
            tp->queue.last = &tp->queue.first;
        }

        //��һ�μ�������Ϊ�̳߳��ǹ�����Դ������̶߳��Ӷ�����ȡ�̣߳��������̻߳�������񵽶�����
        if (ngx_thread_mutex_unlock(&tp->mtx, tp->log) != NGX_OK) {
            return NULL;
        }

#if 0
        ngx_time_update();
#endif

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
                       "run task #%ui in thread pool name:\"%V\"",
                       task->id, &tp->name);

        task->handler(task->ctx, tp->log); //ÿ�������и��Ե�ctx,������ﲻ��Ҫ����

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, tp->log, 0,
                       "complete task #%ui in thread pool name: \"%V\"",
                       task->id, &tp->name);

        task->next = NULL;

        ngx_spinlock(&ngx_thread_pool_done_lock, 1, 2048);

        //task��ӵ�����β����ͬʱ���Ա�֤�����ӵ�ʱ������task����ǰ��task�γ�һ������firstִ�е�һ��task��lastָ�����һ��task
        *ngx_thread_pool_done.last = task;
        ngx_thread_pool_done.last = &task->next;

        ngx_unlock(&ngx_thread_pool_done_lock);

//ngx_notifyͨ�����̣߳�����������ϣ�ngx_thread_pool_handler�����߳�ִ�У�Ҳ���ǽ���cycle{}ͨ��epoll_wait����ִ�У����������̳߳��е��߳�ִ��
        (void) ngx_notify(ngx_thread_pool_handler); 
    }
}

//���������epoll��֪ͨ���¼�����øú��� 
////ngx_notifyͨ�����̣߳�����������ϣ�ngx_thread_pool_handler�����߳�ִ�У�Ҳ���ǽ���cycle{}ͨ��epoll_wait����ִ�У����������̳߳��е��߳�ִ��
static void
ngx_thread_pool_handler(ngx_event_t *ev)
{
    ngx_event_t        *event;
    ngx_thread_task_t  *task;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ev->log, 0, "thread pool handler");

    ngx_spinlock(&ngx_thread_pool_done_lock, 1, 2048);

    /* �����ǲ���������?
        ����̳߳��е��߳�ִ������ȽϿ죬����������ִ��epoll_wait�������е���������ô�ͼ�ⲻ��ngx_notify�е�epoll�¼����п����´μ�⵽���¼���ʱ��
        ngx_thread_pool_done���Ѿ������˺ܶ�ִ����������¼�����ngx_thread_pool_cycle��
        ���������ֻȡ�˶����ײ�������?????? ���������������???????????���԰�

        ���ǣ����������е������������while{}�еõ���ִ��
     */

    task = ngx_thread_pool_done.first;
    ngx_thread_pool_done.first = NULL;
    //β��ָ��ͷ������ͷ�Ѿ���Ϊ�գ�����ִ������  
    ngx_thread_pool_done.last = &ngx_thread_pool_done.first;

    ngx_unlock(&ngx_thread_pool_done_lock);

    while (task) {//����ִ��ǰ�����ngx_thread_pool_done�е�ÿһ������  
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0,
                       "run completion handler for task #%ui", task->id);

        event = &task->event;
        task = task->next;

        event->complete = 1;
        event->active = 0;

      /*�����С�ļ�����һ�ο��Զ��꣬����ָ����Բο�ngx_http_cache_thread_handler  ngx_http_copy_thread_handler  ngx_thread_read

        ����Ǵ��ļ����أ����һ�������ﺯ��ʽ����ļ�����������������һ������ȡ32768�ֽڣ������Ҫ��ζ�ȡ�ļ���������һ��treadִ���������
        ����ngx_notifyͨ��epoll��Ȼ���ߵ���������� 
        */
        event->handler(event);//�����Ƿ�Ӧ�ü��event->handler�Ƿ�Ϊ�գ�����ο�ngx_thread_pool_destroy
    }
}


static void *
ngx_thread_pool_create_conf(ngx_cycle_t *cycle)
{
    ngx_thread_pool_conf_t  *tcf;

    tcf = ngx_pcalloc(cycle->pool, sizeof(ngx_thread_pool_conf_t));
    if (tcf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&tcf->pools, cycle->pool, 4,
                       sizeof(ngx_thread_pool_t *))
        != NGX_OK)
    {
        return NULL;
    }

    return tcf;
}

//�������thread_poll default����ָ��Ĭ�ϵ�threads��max_queue
static char *
ngx_thread_pool_init_conf(ngx_cycle_t *cycle, void *conf)
{
    ngx_thread_pool_conf_t *tcf = conf;

    ngx_uint_t           i;
    ngx_thread_pool_t  **tpp;

    tpp = tcf->pools.elts;

    for (i = 0; i < tcf->pools.nelts; i++) {

        if (tpp[i]->threads) {
            continue;
        }

        if (tpp[i]->name.len == ngx_thread_pool_default.len
            && ngx_strncmp(tpp[i]->name.data, ngx_thread_pool_default.data,
                           ngx_thread_pool_default.len)
               == 0)
        {
            tpp[i]->threads = 32;
            tpp[i]->max_queue = 65536;
            continue;
        }

        ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
                      "unknown thread pool \"%V\" in %s:%ui",
                      &tpp[i]->name, tpp[i]->file, tpp[i]->line);

        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

/*
Syntax: thread_pool name threads=number [max_queue=number];
Default: thread_pool default threads=32 max_queue=65536; threads����Ϊ��pool���̸߳�����max_queue��ʾ�ȴ����̵߳��ȵ�������
*/
static char *
ngx_thread_pool(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t          *value;
    ngx_uint_t          i;
    ngx_thread_pool_t  *tp;

    value = cf->args->elts;

    tp = ngx_thread_pool_add(cf, &value[1]);

    if (tp == NULL) {
        return NGX_CONF_ERROR;
    }

    if (tp->threads) { //˵��������ͬ����thread_pool name threads xx���ظ������threads����ͬ�����ǿ������������ģ������õĻḲ��ǰ�����õ�
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "duplicate thread pool \"%V\"", &tp->name);
        return NGX_CONF_ERROR;
    }

    tp->max_queue = 65536;

    for (i = 2; i < cf->args->nelts; i++) {

        if (ngx_strncmp(value[i].data, "threads=", 8) == 0) {

            tp->threads = ngx_atoi(value[i].data + 8, value[i].len - 8);

            if (tp->threads == (ngx_uint_t) NGX_ERROR || tp->threads == 0) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid threads value \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "max_queue=", 10) == 0) {

            tp->max_queue = ngx_atoi(value[i].data + 10, value[i].len - 10);

            if (tp->max_queue == NGX_ERROR) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid max_queue value \"%V\"", &value[i]);
                return NGX_CONF_ERROR;
            }

            continue;
        }
    }

    if (tp->threads == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%V\" must have \"threads\" parameter",
                           &cmd->name);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

//����Ƿ��Ѿ��и�name��ngx_thread_pool_t������ֱ�ӷ��أ�û���򴴽�����ӵ�ngx_thread_pool_conf_t��
ngx_thread_pool_t *
ngx_thread_pool_add(ngx_conf_t *cf, ngx_str_t *name)
{
    ngx_thread_pool_t       *tp, **tpp;
    ngx_thread_pool_conf_t  *tcf;

    if (name == NULL) {
        name = &ngx_thread_pool_default;
    }
    
    //�������ֵ��̳߳��Ƿ��Ѿ����ڣ�������ֱ�ӷ�����ǰ���̳߳�ngx_thread_pool_t��û�з���NULL
    tp = ngx_thread_pool_get(cf->cycle, name);

    if (tp) {
        return tp;
    }

    //�����µ�ngx_thread_pool_t
    tp = ngx_pcalloc(cf->pool, sizeof(ngx_thread_pool_t));
    if (tp == NULL) {
        return NULL;
    }

    tp->name = *name;
    tp->file = cf->conf_file->file.name.data;
    tp->line = cf->conf_file->line;

    tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cf->cycle->conf_ctx,
                                                  ngx_thread_pool_module);

    tpp = ngx_array_push(&tcf->pools);
    if (tpp == NULL) {
        return NULL;
    }

    *tpp = tp;

    return tp;
}

//�������ֵ��̳߳��Ƿ��Ѿ����ڣ�������ֱ�ӷ�����ǰ���̳߳�ngx_thread_pool_t��û�з���NULL
ngx_thread_pool_t *
ngx_thread_pool_get(ngx_cycle_t *cycle, ngx_str_t *name)
{
    ngx_uint_t                i;
    ngx_thread_pool_t       **tpp;
    ngx_thread_pool_conf_t   *tcf;

    tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                  ngx_thread_pool_module);

    tpp = tcf->pools.elts;

    for (i = 0; i < tcf->pools.nelts; i++) {

        if (tpp[i]->name.len == name->len
            && ngx_strncmp(tpp[i]->name.data, name->data, name->len) == 0)
        {
            return tpp[i];
        }
    }

    return NULL;
}

//��ngx_thread_pool_init_worker�� ngx_thread_pool_exit_worker�ֱ�ᴴ��ÿһ���̳߳غ�����ÿһ���̳߳أ�
static ngx_int_t
ngx_thread_pool_init_worker(ngx_cycle_t *cycle)
{
    ngx_uint_t                i;
    ngx_thread_pool_t       **tpp;
    ngx_thread_pool_conf_t   *tcf;

    if (ngx_process != NGX_PROCESS_WORKER
        && ngx_process != NGX_PROCESS_SINGLE)
    {
        return NGX_OK;
    }

    tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                  ngx_thread_pool_module);

    if (tcf == NULL) {
        return NGX_OK;
    }

    ngx_thread_pool_queue_init(&ngx_thread_pool_done);

    tpp = tcf->pools.elts;

    for (i = 0; i < tcf->pools.nelts; i++) { //�������е��̳߳�
        if (ngx_thread_pool_init(tpp[i], cycle->log, cycle->pool) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

//��ngx_thread_pool_init_worker�� ngx_thread_pool_exit_worker�ֱ�ᴴ��ÿһ���̳߳غ�����ÿһ���̳߳أ�
static void
ngx_thread_pool_exit_worker(ngx_cycle_t *cycle)
{
    ngx_uint_t                i;
    ngx_thread_pool_t       **tpp;
    ngx_thread_pool_conf_t   *tcf;

    if (ngx_process != NGX_PROCESS_WORKER
        && ngx_process != NGX_PROCESS_SINGLE)
    {
        return;
    }

    tcf = (ngx_thread_pool_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                  ngx_thread_pool_module);

    if (tcf == NULL) {
        return;
    }

    tpp = tcf->pools.elts;

    for (i = 0; i < tcf->pools.nelts; i++) {
        ngx_thread_pool_destroy(tpp[i]);
    }
}
