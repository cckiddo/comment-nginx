
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_channel.h>


static void ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n,
    ngx_int_t type);
static void ngx_start_cache_manager_processes(ngx_cycle_t *cycle,
    ngx_uint_t respawn);
static void ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch);
static void ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo);
static ngx_uint_t ngx_reap_children(ngx_cycle_t *cycle);
static void ngx_master_process_exit(ngx_cycle_t *cycle);
static void ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker);
static void ngx_worker_process_exit(ngx_cycle_t *cycle);
static void ngx_channel_handler(ngx_event_t *ev);
static void ngx_cache_manager_process_cycle(ngx_cycle_t *cycle, void *data);
static void ngx_cache_manager_process_handler(ngx_event_t *ev);
static void ngx_cache_loader_process_handler(ngx_event_t *ev);

//����ǵ�һ�μ��أ�������ngx_is_init_cycle�������reload����������ԭ����nginx���̵�ngx_process == NGX_PROCESS_MASTER
ngx_uint_t    ngx_process;//������ֵ��Ĭ��0��Ҳ����NGX_PROCESS_SINGLE
ngx_uint_t    ngx_worker;
ngx_pid_t     ngx_pid;//ngx_pid = ngx_getpid(); ���ӽ�����Ϊ�ӽ���pid����master��Ϊmaster��pid

/*
��Linux�У���Ҫʹ��������������Nginx��������������ֹͣ�����������ļ����ع���־�ļ���ƽ����������Ϊ��Ĭ������£�Nginx����װ��Ŀ¼
/usr/local/nginx/�У���������ļ�·��Ϊ/usr/local/nginc/sbin/nginx�������ļ�·��Ϊ/usr/local/nginx/conf/nginx.conf����Ȼ��
��configureִ��ʱ�ǿ���ָ�������ǰ�װ�ڲ�ͬĿ¼�ġ�Ϊ�˼����������ֻ˵��Ĭ�ϰ�װ����µ������е�ʹ�������������߰�װ��
Ŀ¼�����˱仯����ô�滻һ�¼��ɡ�

��1��Ĭ�Ϸ�ʽ����

ֱ��ִ��Nginx�����Ƴ������磺

/usr/local/nginx/sbin/nginx

��ʱ�����ȡĬ��·���µ������ļ���/usr/local/nginx/conf/nginx.conf��

ʵ���ϣ���û����ʽָ��nginx.conf�����ļ�·��ʱ��������configure����ִ��ʱʹ��--conf-path=PATHָ����nginx.conf�ļ����μ�1.5.1�ڣ���

��2������ָ�������ļ���������ʽ

ʹ��-c����ָ�������ļ������磺

/usr/local/nginx/sbin/nginx -c /tmp/nginx.conf

��ʱ�����ȡ-c������ָ����nginx.conf�����ļ�������Nginx��

��3������ָ����װĿ¼��������ʽ

ʹ��-p����ָ��Nginx�İ�װĿ¼�����磺

/usr/local/nginx/sbin/nginx -p /usr/local/nginx/

��4������ָ��ȫ���������������ʽ

����ͨ��-g������ʱָ��һЩȫ���������ʹ�µ���������Ч�����磺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;"

��������������ζ�Ż��pid�ļ�д��/var/nginx/test.pid�С�

-g������Լ��������ָ�������������Ĭ��·���µ�nginx.conf�е����������ͻ�������޷������������������������������������pid logs/nginx.pid��
�ǲ��ܴ�����Ĭ�ϵ�nginx.conf�еġ�

��һ��Լ�������ǣ���-g��ʽ������Nginx����ִ������������ʱ����Ҫ��-g����Ҳ���ϣ�������ܳ��������ƥ������Ρ����磬���ҪֹͣNginx����
��ô��Ҫִ��������룺

/usr/local/nginx/sbin/nginx -g "pid /var/nginx/test.pid;" -s stop

���������-g "pid /var/nginx/test.pid;"����ô�Ҳ���pid�ļ���Ҳ������޷�ֹͣ����������

��5������������Ϣ�Ƿ��д���

�ڲ�����Nginx������£�ʹ��-t���������������ļ��Ƿ��д������磺

/usr/local/nginx/sbin/nginx -t

ִ�н������ʾ�����Ƿ���ȷ��

��6���ڲ������ý׶β������Ϣ

��������ѡ��ʱ��ʹ��-q�������Բ���error�������µ���Ϣ�������Ļ�����磺

/usr/local/nginx/sbin/nginx -t -q

��7����ʾ�汾��Ϣ

ʹ��-v������ʾNginx�İ汾��Ϣ�����磺

/usr/local/nginx/sbin/nginx -v

��8����ʾ����׶εĲ���

ʹ��-V�������˿�����ʾNginx�İ汾��Ϣ�⣬��������ʾ���ñ���׶ε���Ϣ����GCC�������İ汾������ϵͳ�İ汾��ִ��configureʱ�Ĳ����ȡ����磺

/usr/local/nginx/sbin/nginx -V

��9�����ٵ�ֹͣ����

ʹ��-s stop����ǿ��ֹͣNginx����-s������ʵ�Ǹ���Nginx�������������е�Nginx�������ź�����Nginx����ͨ��nginx.pid�ļ��еõ�master���̵Ľ���ID��
���������е�master���̷���TERM�ź������ٵعر�Nginx�������磺

/usr/local/nginx/sbin/nginx -s stop

ʵ���ϣ����ͨ��kill����ֱ����nginx master���̷���TERM����INT�źţ�Ч����һ���ġ����磬��ͨ��ps�������鿴nginx master�Ľ���ID��

:ahf5wapi001:root > ps -ef | grep nginx

root     10800     1  0 02:27 ?        00:00:00 nginx: master process ./nginx

root     10801 10800  0 02:27 ?        00:00:00 nginx: worker process

������ֱ��ͨ��kill�����������źţ�

kill -s SIGTERM 10800

���ߣ�

kill -s SIGINT 10800

�������������Ч����ִ��/usr/local/nginx/sbin/nginx -s stop����ȫһ���ġ�

��10�������š���ֹͣ����

���ϣ��Nginx������������ش����굱ǰ����������ֹͣ������ô����ʹ��-s quit������ֹͣ�������磺

/usr/local/nginx/sbin/nginx -s quit

�����������ֹͣNginx������������ġ�������ֹͣ����ʱ��worker������master�������յ��źź����������ѭ�����˳����̡��������š���ֹͣ����ʱ��
���Ȼ�رռ����˿ڣ�ֹͣ�����µ����ӣ�Ȼ��ѵ�ǰ���ڴ��������ȫ�������꣬������˳����̡�

�����ֹͣ�������ƣ�����ֱ�ӷ���QUIT�źŸ�master������ֹͣ������Ч����ִ��-s quit������һ���ġ����磺

kill -s SIGQUIT <nginx master pid>

���ϣ�������š���ֹͣĳ��worker���̣���ô����ͨ����ý��̷���WINCH�ź���ֹͣ�������磺

kill -s SIGWINCH <nginx worker pid>

��11��ʹ�����е�Nginx�ض��������Ч

ʹ��-s reload��������ʹ�����е�Nginx�������¼���nginx.conf�ļ������磺

/usr/local/nginx/sbin/nginx -s reload

��ʵ�ϣ�Nginx���ȼ���µ��������Ƿ��������ȫ����ȷ���ԡ����š��ķ�ʽ�رգ�����������Nginx��ʵ�����Ŀ�ġ����Ƶģ�-s�Ƿ����źţ�
��Ȼ������kill�����HUP�ź����ﵽ��ͬ��Ч����

kill -s SIGHUP <nginx master pid>

��12����־�ļ��ع�

ʹ��-s reopen�����������´���־�ļ������������Ȱѵ�ǰ��־�ļ�������ת�Ƶ�����Ŀ¼�н��б��ݣ������´�ʱ�ͻ������µ���־�ļ���
�������ʹ����־�ļ������ڹ������磺

/usr/local/nginx/sbin/nginx -s reopen

��Ȼ������ʹ��kill�����USR1�ź�Ч����ͬ��

kill -s SIGUSR1 <nginx master pid>

��13��ƽ������Nginx

��Nginx�����������µİ汾ʱ������Ҫ���ɵĶ������ļ�Nginx�滻����ͨ�������������Ҫ��������ģ���Nginx֧�ֲ���������������°汾��ƽ��������

����ʱ�������²��裺

1��֪ͨ�������еľɰ汾Nginx׼��������ͨ����master���̷���USR2�źſɴﵽĿ�ġ����磺

kill -s SIGUSR2 <nginx master pid>

��ʱ�������е�Nginx�Ὣpid�ļ����������罫/usr/local/nginx/logs/nginx.pid������Ϊ/usr/local/nginx/logs/nginx.pid.oldbin�������µ�Nginx���п��������ɹ���

2�������°汾��Nginx������ʹ�����Ͻ��ܹ�������һ��������������ʱͨ��ps������Է����¾ɰ汾��Nginx��ͬʱ���С�

3��ͨ��kill������ɰ汾��master���̷���SIGQUIT�źţ��ԡ����š��ķ�ʽ�رվɰ汾��Nginx�����ֻ���°汾��Nginx�������У���ʱƽ��������ϡ�

��14����ʾ�����а���

ʹ��-h����-?��������ʾ֧�ֵ����������в�����

*/
sig_atomic_t  ngx_reap;
sig_atomic_t  ngx_sigio;
sig_atomic_t  ngx_sigalrm;
sig_atomic_t  ngx_terminate; //�����յ�TERM�ź�ʱ��ngx_terminate��־λ����Ϊ1�������ڸ���worker������Ҫǿ�ƹرս��̣�
sig_atomic_t  ngx_quit; //�����յ�QUIT�ź�ʱ��ngx_quit��־λ����Ϊ1�������ڸ���worker������Ҫ���ŵعرս��̣�
sig_atomic_t  ngx_debug_quit;
ngx_uint_t    ngx_exiting; //ngx_exiting��־λ����ngx_worker_process_cycle�������˳�ʱ��Ϊ��־λʹ��
sig_atomic_t  ngx_reconfigure;//nginx -s reload�ᴥ�����º�
sig_atomic_t  ngx_reopen; //�����յ�USRI�ź�ʱ��ngx_reopen��־λ����Ϊ1�������ڸ���Nginx��Ҫ���´��ļ������л���־�ļ�ʱ��

sig_atomic_t  ngx_change_binary; //ƽ���������°汾��Nginx����������
ngx_pid_t     ngx_new_binary;//�����ȴ����滻�������ǵ���execve��ִ���µĴ��롣 �������ngx_change_binary�Ļ����ϻ�ȡֵ
ngx_uint_t    ngx_inherited;
ngx_uint_t    ngx_daemonized;

sig_atomic_t  ngx_noaccept;
ngx_uint_t    ngx_noaccepting;
ngx_uint_t    ngx_restart;


static u_char  master_process[] = "master process";

/*
��Nginx�У����������proxy(fastcgi) cache���ܣ�master process����������ʱ������������������ӽ���(�����ڴ���������ӽ���)��������
��ʹ��̵Ļ�����塣��һ�����̵Ĺ����Ƕ��ڼ�黺�棬�������ڵĻ���ɾ�����ڶ������̵���������������ʱ�򽫴������Ѿ�����ĸ�
��ӳ�䵽�ڴ���(ĿǰNginx�趨Ϊ�����Ժ�60��)��Ȼ���˳���

����ģ������������̵�ngx_process_events_and_timers()�����У������ngx_event_expire_timers()��Nginx��ngx_event_timer_rbtree(�����)��
�水��ִ�е�ʱ����Ⱥ�����һϵ�е��¼���ÿ��ȡִ��ʱ��������¼��������ǰʱ���Ѿ�����Ӧ��ִ�и��¼����ͻ�����¼���handler������
���̵�handler�ֱ���ngx_cache_manager_process_handler��ngx_cache_loader_process_handler

Ҳ����˵manger �� loader�Ķ�ʱ����ֱ����ngx_cache_manager_process_handler��ngx_cache_loader_process_handler���������Կ���manager�Ķ�
ʱ����ʼʱ����0����loader��60000���롣Ҳ����˵��manager��nginxһ����ʱ�������ˣ����ǣ�loader����nginx������1���Ӻ�Ż�������
*/
static ngx_cache_manager_ctx_t  ngx_cache_manager_ctx = {
    ngx_cache_manager_process_handler, "cache manager process", 0
};
static ngx_cache_manager_ctx_t  ngx_cache_loader_ctx = {
    ngx_cache_loader_process_handler, "cache loader process", 60000  //���̴�����60000m��ִ��ngx_cache_loader_process_handler,��ngx_cache_manager_process_cycle����ӵĶ�ʱ��
};


static ngx_cycle_t      ngx_exit_cycle;
static ngx_log_t        ngx_exit_log;
static ngx_open_file_t  ngx_exit_log_file;
/*

ngx_master_process_cycle �� �� ngx_start_worker_processes���ɶ�������ӽ��̣�ngx_start_worker_processes �� �� ngx_worker_process_cycle
�����������ݣ���������ж�����̣߳�����Ҳ���ʼ���̺߳ʹ����̹߳������ݣ���ʼ�����֮��ngx_worker_process_cycle 
����봦��ѭ�������� ngx_process_events_and_timers �� �� �� �� �� �� ngx_process_events�����¼���
�����¼�Ͷ�ݵ��¼�����ngx_posted_events �� �� �� �� �� �� ngx_event_thread_process_posted�д����¼���
*/
/*
master���̲���Ҫ���������¼�����������ҵ���ִ�У�ֻ��ͨ������worker���ӽ�
����ʵ����������ƽ��������������־�ļ��������ļ�ʵʱ��Ч�ȹ���
*/

//����Ƕ���̷�ʽ�������ͻ����ngx_master_process_cycle��������������� 
void
ngx_master_process_cycle(ngx_cycle_t *cycle)
{
    char              *title;
    u_char            *p;
    size_t             size;
    ngx_int_t          i;
    ngx_uint_t         n, sigio;
    sigset_t           set;
    struct itimerval   itv;
    ngx_uint_t         live;
    ngx_msec_t         delay;
    ngx_listening_t   *ls;
    ngx_core_conf_t   *ccf;


    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, ngx_signal_value(NGX_RECONFIGURE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_REOPEN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_NOACCEPT_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_TERMINATE_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
    sigaddset(&set, ngx_signal_value(NGX_CHANGEBIN_SIGNAL));

    /*
     ÿ��������һ���ź�����(signal mask)���򵥵�˵���ź�������һ����λͼ��������ÿһλ����Ӧ��һ���ź�
     ���λͼ�е�ĳһλΪ1���ͱ�ʾ��ִ�е�ǰ�źŵĴ�������ڼ���Ӧ���ź���ʱ�������Ρ���ʹ����ִ�еĹ����в���Ƕ�׵���Ӧ�����źš�
     
     Ϊʲô��ĳһ�źŽ��������أ���������һ�¶�CTRL_C�Ĵ������֪������һ��������������ʱ���ڼ����ϰ�һ��CTRL_C���ں˾ͻ�����Ӧ�Ľ���
     ����һ��SIGINT �źţ���������źŵ�Ĭ�ϲ�������ͨ��do_exit()�����ý��̵����С����ǣ���ЩӦ�ó�����ܶ�CTRL_C���Լ��Ĵ������Ծ�Ҫ
     ΪSIGINT��������һ���������ʹ��ָ��Ӧ�ó����е�һ�����������Ǹ������ж�CTRL_C����¼�������Ӧ�����ǣ���ʵ����ȴ���֣�����CTRL_C
     �¼����������ܼ�����ʱ��ոս����һ���źŵĴ�����򣬵ڶ���SIGINT�źž͵����ˣ����ڶ����źŵ�Ĭ�ϲ�����ɱ�����̣���������һ���ź�
     �Ĵ���������û��ִ���ꡣΪ�˱�����������ĳ��֣�����ִ��һ���źŴ������Ĺ����н������ź��Զ����ε�����ν�����Ρ����뽫�źź���
     �ǲ�ͬ�ģ���ֻ�ǽ��ź���ʱ���ڸǡ�һ�£�һ������ȥ�����ѵ�����ź��ּ����õ�����
     
     ��ν����, �����ǽ�ֹ�����ź�, ������ʱ�����źŵĵ���,
     ������κ�, �źŽ�������, ���ᶪʧ
     */ // ������Щ�źŶ�������������sigpending���òŸ���������Щ�¼�
     
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {   //�ο������sigsuspend     
    //���ӽ��̵ļ̳й�ϵ���Բο�:http://blog.chinaunix.net/uid-20011314-id-1987626.html
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "sigprocmask() failed");
    }

    sigemptyset(&set); 


    size = sizeof(master_process);

    for (i = 0; i < ngx_argc; i++) {
        size += ngx_strlen(ngx_argv[i]) + 1;
    }

    title = ngx_pnalloc(cycle->pool, size);
    if (title == NULL) {
        /* fatal */
        exit(2);
    }

    /* ��master process + ����һ�������������� */
    p = ngx_cpymem(title, master_process, sizeof(master_process) - 1);
    for (i = 0; i < ngx_argc; i++) {
        *p++ = ' ';
        p = ngx_cpystrn(p, (u_char *) ngx_argv[i], size);
    }

    ngx_setproctitle(title); //�޸Ľ�����Ϊtitle

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    ngx_start_worker_processes(cycle, ccf->worker_processes,
                               NGX_PROCESS_RESPAWN); //����worker����
    ngx_start_cache_manager_processes(cycle, 0); //����cache manager�� cache loader����

    ngx_new_binary = 0;
    delay = 0;
    sigio = 0;
    live = 1;

/*
    ngx_signal_handler��������ݽ��յ����ź�����ngx_reap. ngx_quit. ngx_terminate.
ngx_reconfigure. ngx_reopen. ngx_change_binary. ngx_noaccept��Щ��־λ������8-40
��8-4�����н��յ����źŶ�Nginx��ܵ�����
�����������������ש������������������������������ש�������������������������������������������������
��    ��  ��    ��  ��Ӧ�����е�ȫ�ֱ�־λ����  ��    ����                                        ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��  QUIT        ��    ngx_quit                  ��  ���ŵعر���������                            ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��  TERM����INT ��ngx_terminate                 ��  ǿ�ƹر���������                              ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��  USR1        ��    ngx reopen                ��  ���´򿪹����е������ļ�                      ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��              ��                              ��  �����ӽ��̲��ٽ��ܴ����µ����ӣ�ʵ���൱�ڶ�  ��
��  WINCH       ��ngx_noaccept                  ��                                                ��
��              ��                              �����е�����̷���QUIT�ź���                      ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��  USR2        ��ngx_change_binary             ��  ƽ���������°汾��Nginx����                   ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��  HUP         ��ngx_reconfigure               ��  �ض������ļ���ʹ��������侰����Ч            ��
�ǩ��������������贈�����������������������������贈������������������������������������������������
��              ��                              ��  ���ӽ��������������ʱ��Ҫ������е��ӽ��̣�  ��
��  CHLD        ��    ngx_reap                  ��Ҳ����ngx_reap_children���������Ĺ���           ��
�����������������ߩ������������������������������ߩ�������������������������������������������������
    ��8-4�г���master���������е�7��ȫ�ֱ�־λ����������֮�⣬����һ����־λҲ
���õ�������������master������������Ϊ��־λʹ�õģ����ź��޹ء�

ʵ���ϣ���������8����־λ��ngx_reap��ngx_terminate��ngx_quit��ngx_reconfigure��ngx_restart��ngx_reopen��ngx_change_binary��
ngx_noaccept������ִ�в�ͬ�ķ�֧���̣���ѭ��ִ�У�ע�⣬ÿ��һ��ѭ��ִ����Ϻ���̻ᱻ����ֱ�����µ��źŲŻἤ�����ִ�У���
*/
    for ( ;; ) {
        /*
        delay�����ȴ��ӽ����˳���ʱ�䣬�������ǽ��ܵ�SIGINT�źź�������Ҫ�ȷ����źŸ��ӽ��̣����ӽ��̵��˳���Ҫһ����ʱ�䣬
        ��ʱʱ����ӽ������˳������Ǹ����̾�ֱ���˳���������sigkill�źŸ��ӽ���(ǿ���˳�),Ȼ�����˳���
          */
        if (delay) 
        {
            if (ngx_sigalrm) {
                sigio = 0;
                delay *= 2;
                ngx_sigalrm = 0;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                           "termination cycle: %d", delay);
            //delay = 2000;
            itv.it_interval.tv_sec = 0;
            itv.it_interval.tv_usec = 0;
            itv.it_value.tv_sec = delay / 1000;
            itv.it_value.tv_usec = (delay % 1000 ) * 1000;

            /*
            setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)); 
            setitimer()��alarm����ǿ��֧��3�����͵Ķ�ʱ���� 
            
            ITIMER_REAL�� �趨����ʱ�䣻����ָ����ʱ����ں˽�����SIGALRM�źŸ������̣�
            ITIMER_VIRTUAL �趨����ִ��ʱ�䣻����ָ����ʱ����ں˽�����SIGVTALRM�źŸ������̣�
            ITIMER_PROF �趨����ִ���Լ��ں��򱾽��̶����ĵ�ʱ��ͣ�����ָ����ʱ����ں˽�����ITIMER_VIRTUAL�źŸ������̣�
            
            */ //���ö�ʱ������ϵͳ��ʵʱ�������㣬�ͳ�SIGALRM�ź�,����źŷ�����������ngx_sigalrmΪ1������delay�ͻ᲻�Ϸ�����
            if (setitimer(ITIMER_REAL, &itv, NULL) == -1) { //ÿ��itvʱ�䷢��һ��SIGALRM�ź�
                ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                              "setitimer() failed");
            }
        }

        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "sigsuspend");

        /*
          sigsuspend(const sigset_t *mask))�����ڽ��յ�ĳ���ź�֮ǰ, ��ʱ��mask�滻���̵��ź�����, ����ͣ����ִ�У�ֱ���յ��ź�Ϊֹ��
          sigsuspend ���غ󽫻ָ�����֮ǰ���ź����롣�źŴ�������ɺ󣬽��̽�����ִ�С���ϵͳ����ʼ�շ���-1������errno����ΪEINTR��

         
          ��ʵsigsuspend��һ��ԭ�Ӳ���������4�����裺
          (1) �����µ�mask������ǰ���̣�
          (2) �յ��źţ��ָ�ԭ��mask��
          (3) ���øý������õ��źŴ�������
          (4) ���źŴ��������غ�sigsuspend���ء�
          
          */ 
        /*
        �ȴ��źŷ���,ǰ��sigprocmask��������sigemptyset(&set);���������ȴ����������źţ�ֻҪ���źŵ����򷵻ء����綨ʱ�ź�  ngx_reap ngx_terminate���ź�
        �������(2)������Կ����ڴ�������ִ���ź��жϺ����ĺٺ٣�������ʱ���Ѿ��ָ���ԭ����mask(Ҳ��������sigprocmask���õ����뼯)
        �������źŴ������в����ٴ���������źţ�ֻ���ڸ�while()ѭ���ٴ��ߵ�sigsuspend��ʱ�������ź��жϣ��Ӷ�������ͬһʱ�̶���ж�ͬһ�ź�
        */
        //nginx sigsuspend�������ο�http://weakyon.com/2015/05/14/learning-of-sigsuspend.html
        sigsuspend(&set); //�ȴ���ʱ����ʱ��ͨ��ngx_init_signalsִ��ngx_signal_handler�е�SIGALRM�źţ��źŴ��������غ󣬼����ú�������Ĳ���

        ngx_time_update();

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "wake up, sigio %i", sigio);

        if (ngx_reap) { //�������յ�һ���ӽ����˳����źţ���ngx_signal_handler
            ngx_reap = 0;
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "reap children");

            ///������洦���˳����ӽ���(�е�worker�쳣�˳�����ʱ���Ǿ���Ҫ�������worker )����������ӽ��̶��˳���᷵��0. 
            live = ngx_reap_children(cycle); // ���ӽ��������������ʱ��Ҫ������е��ӽ��̣�Ҳ����ngx_reap_children���������Ĺ���
        }

        //���û�д����ӽ��̣������յ���ngx_terminate����ngx_quit�źţ���master�˳��� 
        if (!live && (ngx_terminate || ngx_quit)) {
            ngx_master_process_exit(cycle);
        }

        /*
         ���ngx_terminate��־λΪl�����������ӽ��̷����ź�TERM��֪ͨ�ӽ���ǿ���˳����̣�������ֱ��������1����������̣��ȴ��źż�����̡�
         */
        if (ngx_terminate) { //�յ���sigint�źš�
            if (delay == 0) {///������ʱ��
                delay = 50;
            }

            if (sigio) {
                sigio--;
                continue;
            }

            sigio = ccf->worker_processes + 2 /* cache processes */;

            if (delay > 1000) { //�����ʱ����ǿ��ɱ��worker  
                ngx_signal_worker_processes(cycle, SIGKILL);
            } else { //������sigint��worker�������˳���  
                ngx_signal_worker_processes(cycle,
                                       ngx_signal_value(NGX_TERMINATE_SIGNAL));
            }

            continue;
        }

        /*
          ����'ngx_quitΪ1�ķ�֧���̡��ر����еļ����˿ڣ�������ֱ��������1��������master���̣��ȴ��źż�����̡�
          */
        if (ngx_quit) {///�յ�quit�źš� 
            
            //���͸�worker quit�ź�  
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));

            ls = cycle->listening.elts;
            for (n = 0; n < cycle->listening.nelts; n++) {
                if (ngx_close_socket(ls[n].fd) == -1) {
                    ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_socket_errno,
                                  ngx_close_socket_n " %V failed",
                                  &ls[n].addr_text);
                }
            }
            cycle->listening.nelts = 0;

            continue;
        }

        //�յ���Ҫreconfig���ź�  
        /*
         ���ngx_reconfigure��־λΪ0����������13�����ngx_restart��־λ�����ngx_reconfigureΪl�����ʾ��Ҫ���¶�ȡ�����ļ���
         Nginx��������ԭ�ȵ�worker���ӽ��������¶�ȡ�����ļ������Ĳ��������³�ʼ��ngx_cycle_t�ṹ�壬��������ȡ�µ������ļ���
         �������µ�worker���̣����پɵ�worker���̡������н������ngx_init_cycle�������³�ʼ��ngx_cycle_t�ṹ�塣
          */
        if (ngx_reconfigure) { //�ض������ļ���ʹ��������侰����Ч 
            ngx_reconfigure = 0;

            if (ngx_new_binary) { //�ж��Ƿ��ȴ����滻����µĴ��뻹��������(Ҳ���ǻ�û�˳���ǰ��master)��������������У�����Ҫ���³�ʼ��config��  
                ngx_start_worker_processes(cycle, ccf->worker_processes,
                                           NGX_PROCESS_RESPAWN);
                ngx_start_cache_manager_processes(cycle, 0);
                ngx_noaccepting = 0;

                continue;
            }

            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reconfiguring");

            cycle = ngx_init_cycle(cycle); //���³�ʼ��config�������������µ�worker  
            if (cycle == NULL) {
                cycle = (ngx_cycle_t *) ngx_cycle;
                continue;
            }

            ngx_cycle = cycle;
            ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                   ngx_core_module);

            //����ngx_start_worker_processes����������һ��worker���̣���Щworker���̽�ʹ����ngx_cycle_t�����塣
            ngx_start_worker_processes(cycle, ccf->worker_processes,
                                       NGX_PROCESS_JUST_RESPAWN);
                                       
            //����ngx_start_cache_ _manager_processes���������ջ���ģ��ļ�����������Ƿ�����cache manage����cache loader���̡�
            //���������������ú󣬿϶��Ǵ����ӽ����ˣ���ʱ���live��־λ��Ϊ1
            ngx_start_cache_manager_processes(cycle, 1);

            /* allow new processes to start */
            ngx_msleep(100);

            live = 1;

            //��ԭ�ȵģ����Ǹո�����ģ������ӽ��̷���QUIT�źţ�Ҫ���������ŵ��˳��Լ��Ľ��̡�
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
        }

        if (ngx_restart) {
            ngx_restart = 0;
            ngx_start_worker_processes(cycle, ccf->worker_processes,
                                       NGX_PROCESS_RESPAWN);
            ngx_start_cache_manager_processes(cycle, 0);
            live = 1;
        }

        /*
         ʹ��-s reopen�����������´���־�ļ������������Ȱѵ�ǰ��־�ļ�������ת�Ƶ�����Ŀ¼�н��б��ݣ������´�ʱ�ͻ������µ���־�ļ���
    �������ʹ����־�ļ������ڹ��󡣵�Ȼ������ʹ��kill�����USR1�ź�Ч����ͬ��
        */
        if (ngx_reopen) {
            /*
                ���ngx_reopenΪ1�������ngx_reopen_files����´������ļ���ͬʱ��ngx_reopen��־λ��Ϊ0��
                �������ӽ��̷���USRI�źţ�Ҫ���ӽ��̶������´������ļ���
               */
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, ccf->user);
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_REOPEN_SIGNAL));
        }

        /*
         ���ngx_change_binary��־λ�����ngx_change_binaryΪ1�����ʾ��Ҫƽ������Nginx����ʱ������ngx_exec_new_binary�������µ���
         ���������°汾��Nginx���� ͬʱ��ngx_change_binary��־λ��Ϊ0��
          */
        if (ngx_change_binary) { //ƽ���������°汾��Nginx����
            ngx_change_binary = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "changing binary");
            ngx_new_binary = ngx_exec_new_binary(cycle, ngx_argv);///�����ȴ����滻�������ǵ���execve��ִ���µĴ��롣 
        }

        
        ///���ܵ�ֹͣaccept���ӣ���ʵҲ����worker�˳�(��������ǣ�����master����Ҫ�˳�).��  
        /*
          ���ngx_noaccept��־λ�����ngx_noacceptΪ0���������1��������һ��ѭ�������ngx_noaciceptΪ1���������е��ӽ��̷���QUIT�źţ�
          Ҫ���������ŵعرշ���ͬʱ��ngx_noaccept��Ϊ0������ngx_noaccepting��Ϊ1����ʾ����ֹͣ�����µ����ӡ�
          */
        if (ngx_noaccept) {//�����ӽ��̲��ٽ��ܴ����µ����ӣ�ʵ���൱�ڶ����е�����̷���QUIT�ź���
            ngx_noaccept = 0;
            ngx_noaccepting = 1;

            //��worker�����źš�  
            ngx_signal_worker_processes(cycle,
                                        ngx_signal_value(NGX_SHUTDOWN_SIGNAL));
        }
    }
}

/*
���hginx.conf������Ϊ�����̹���ģʽ����ʱ�������ngx_single_process_cycle�������뵥�ų̹���ģʽ��
*/
void
ngx_single_process_cycle(ngx_cycle_t *cycle)
{
    ngx_uint_t  i;

    if (ngx_set_environment(cycle, NULL) == NULL) {
        /* fatal */
        exit(2);
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->init_process) {
            if (ngx_modules[i]->init_process(cycle) == NGX_ERROR) {
                /* fatal */
                exit(2);
            }
        }
    }

    for ( ;; ) {
        ngx_log_debug0(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker cycle");

        ngx_process_events_and_timers(cycle);

        if (ngx_terminate || ngx_quit) {

            for (i = 0; ngx_modules[i]; i++) {
                if (ngx_modules[i]->exit_process) {
                    ngx_modules[i]->exit_process(cycle);
                }
            }

            ngx_master_process_exit(cycle);
        }

        if (ngx_reconfigure) {
            ngx_reconfigure = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reconfiguring");

            cycle = ngx_init_cycle(cycle);
            if (cycle == NULL) {
                cycle = (ngx_cycle_t *) ngx_cycle;
                continue;
            }

            ngx_cycle = cycle;
        }

        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, (ngx_uid_t) -1);
        }
    }
}

static void
ngx_start_worker_processes(ngx_cycle_t *cycle, ngx_int_t n, ngx_int_t type)
{
    ngx_int_t      i;
    ngx_channel_t  ch;

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "start worker processes");

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    //���ݸ�����worker�ӽ��̵������ͨ�Źܵ�   
    ch.command = NGX_CMD_OPEN_CHANNEL;

    /*
    ��master���̰��������ļ���worker���̵���Ŀ��������Щ�ӽ��̣�Ҳ���ǵ��ñ�8-2�е�ngx_start_worker_processes��������
    */
    for (i = 0; i < n; i++) { //nΪnginx.conf worker_processes�����õĽ�����
/*
                                 |----------(ngx_worker_process_cycle->ngx_worker_process_init)
    ngx_start_worker_processes---| ngx_processes[]��صĲ�����ֵ����
                                 |----------ngx_pass_open_channel
*/
        ngx_spawn_process(cycle, ngx_worker_process_cycle,
                          (void *) (intptr_t) i, "worker process", type);

        //���Ѿ�������worker���̹㲥��ǰ����worker������Ϣ������   
        ch.pid = ngx_processes[ngx_process_slot].pid;
        ch.slot = ngx_process_slot;
        ch.fd = ngx_processes[ngx_process_slot].channel[0]; //ngx_spawn_process�и�ֵ

        /*  
           ����ÿ���ӽ��̺͸�����֮��ʹ�õ���socketpairϵͳ���ý���������ȫ˫����socket  
           channel[]�ڸ��ӽ����и���һ�ף�channel[0]Ϊд�ˣ�channel[1]Ϊ����  

            
           �����̹ر�socket[0],�ӽ��̹ر�socket[1]�������̴�sockets[1]�ж�д���ӽ��̴�sockets[0]�ж�д������ȫ˫����̬���ο�http://www.xuebuyuan.com/1691574.html
           �Ѹ��ӽ��̵����channel��Ϣ���ݸ��Ѿ������õ����������ӽ���
         */
        ngx_pass_open_channel(cycle, &ch); 
    }
}

/*
��Nginx�У����������proxy(fastcgi) cache���ܣ�master process����������ʱ������������������ӽ���(�����ڴ���������ӽ���)��������
��ʹ��̵Ļ�����塣��һ�����̵Ĺ����Ƕ��ڼ�黺�棬�������ڵĻ���ɾ�����ڶ������̵���������������ʱ�򽫴������Ѿ�����ĸ�
��ӳ�䵽�ڴ���(ĿǰNginx�趨Ϊ�����Ժ�60��)��Ȼ���˳���

����ģ������������̵�ngx_process_events_and_timers()�����У������ngx_event_expire_timers()��Nginx��ngx_event_timer_rbtree(�����)��
�水��ִ�е�ʱ����Ⱥ�����һϵ�е��¼���ÿ��ȡִ��ʱ��������¼��������ǰʱ���Ѿ�����Ӧ��ִ�и��¼����ͻ�����¼���handler������
���̵�handler�ֱ���ngx_cache_manager_process_handler��ngx_cache_loader_process_handler
*/
static void
ngx_start_cache_manager_processes(ngx_cycle_t *cycle, ngx_uint_t respawn)
{
    ngx_uint_t       i, manager, loader;
    ngx_path_t     **path;
    ngx_channel_t    ch;

    manager = 0;
    loader = 0;

    path = ngx_cycle->paths.elts;
    for (i = 0; i < ngx_cycle->paths.nelts; i++) {

        if (path[i]->manager) {
            manager = 1;
        }

        if (path[i]->loader) {
            loader = 1;
        }
    }

/*
����һ�����У���master���̸���֮ǰ��ģ��ĳ�ʼ������������Ƿ�����cachemanage�ӽ��̣�Ҳ���Ǹ���ngx_cycle_t�д洢·���Ķ�̬����
pathes���Ƿ���ĳ��·����manage��־λ���������Ƿ�����cache manage�ӽ��̡�������κ�1��·����manage��־λΪ1��������cache manage�ӽ��̡�
*/
    if (manager == 0) { //ֻ���������˻�����Ϣ�Ż���1���������û�����û��治������cache manage��load����
        return;
    }

    /*
    ��Nginx�У����������proxy(fastcgi) cache���ܣ�master process����������ʱ������������������ӽ���(�����ڴ���������ӽ���)��������
    ��ʹ��̵Ļ�����塣��һ�����̵Ĺ����Ƕ��ڼ�黺�棬�������ڵĻ���ɾ�����ڶ������̵���������������ʱ�򽫴������Ѿ�����ĸ�
    ��ӳ�䵽�ڴ���(ĿǰNginx�趨Ϊ�����Ժ�60��)��Ȼ���˳���
    
    ����ģ������������̵�ngx_process_events_and_timers()�����У������ngx_event_expire_timers()��Nginx��ngx_event_timer_rbtree(�����)��
    �水��ִ�е�ʱ����Ⱥ�����һϵ�е��¼���ÿ��ȡִ��ʱ��������¼��������ǰʱ���Ѿ�����Ӧ��ִ�и��¼����ͻ�����¼���handler������
    ���̵�handler�ֱ���ngx_cache_manager_process_handler��ngx_cache_loader_process_handler
    */
    ngx_spawn_process(cycle, ngx_cache_manager_process_cycle,
                      &ngx_cache_manager_ctx, "cache manager process",
                      respawn ? NGX_PROCESS_JUST_RESPAWN : NGX_PROCESS_RESPAWN);

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_OPEN_CHANNEL;
    ch.pid = ngx_processes[ngx_process_slot].pid;
    ch.slot = ngx_process_slot;
    ch.fd = ngx_processes[ngx_process_slot].channel[0];

    ngx_pass_open_channel(cycle, &ch);

    /*
    ������κ�1��·����loader��־λΪ1��������cache loader�ӽ���, ���ļ�����ģ���������
    */
    if (loader == 0) {
        return;
    }

    
    ngx_spawn_process(cycle, ngx_cache_manager_process_cycle,
                      &ngx_cache_loader_ctx, "cache loader process",
                      respawn ? NGX_PROCESS_JUST_SPAWN : NGX_PROCESS_NORESPAWN);

    ch.command = NGX_CMD_OPEN_CHANNEL;
    ch.pid = ngx_processes[ngx_process_slot].pid;
    ch.slot = ngx_process_slot;
    ch.fd = ngx_processes[ngx_process_slot].channel[0];

    ngx_pass_open_channel(cycle, &ch);
}

/*
�ӽ��̴�����ʱ�򣬸����̵Ķ������ᱻ�ӽ��̼̳У����Ժ��洴�����ӽ����ܹ��õ�ǰ�洴�����ӽ��̵�channel��Ϣ��ֱ�ӿ��Ժ�����ͨ�ţ�
��ôǰ�洴���Ľ������֪������Ľ�����Ϣ�أ� �ܼ򵥣���Ȼǰ�洴���Ľ����ܹ�������Ϣ����ô�Ҿͷ�����Ϣ����������Ľ���
��channel,������Ϣ������channel[0]�У������Ϳ����໥ͨ���ˡ�
*/
/*
                                 |----------(ngx_worker_process_cycle->ngx_worker_process_init)
    ngx_start_worker_processes---| ngx_processes[]��صĲ�����ֵ����
                                 |----------ngx_pass_open_channel
*/
static void 
ngx_pass_open_channel(ngx_cycle_t *cycle, ngx_channel_t *ch)
{ //�ú������Խ��������̺����������ӽ��̵�ͨ����ϵ���͸����̵�ͨ����ϵ��ֱ�Ӽ̳й����ģ����Ա����̿���ͨ��ch->fd�����е�
    ngx_int_t  i;

    for (i = 0; i < ngx_last_process; i++) { /* ngx_last_processȫ�ֱ�����ͬ����ngx_spawn_process()�б���ֵ����Ϊ�����Ľ��� */  

        
        // �����մ�����worker�ӽ��� || �����ڵ��ӽ��� || �丸����socket�رյ��ӽ���  
        //���Ժ�ngx_worker_process_init�е�channel�رղ�������Ķ�
        if (i == ngx_process_slot
            || ngx_processes[i].pid == -1
            || ngx_processes[i].channel[0] == -1) //�����Լ����쳣��worker     
        {
            continue;
        } //

        ngx_log_debug6(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                      "pass channel s:%d pid:%P fd:%d to s:%i pid:%P fd:%d",
                      ch->slot, ch->pid, ch->fd,
                      i, ngx_processes[i].pid,
                      ngx_processes[i].channel[0]);

        /* TODO: NGX_AGAIN */
        //������Ϣ��������worker   
        /* ��ÿ�����̵ĸ����̷��͸մ���worker���̵���Ϣ��IPC��ʽ�Ժ��ٸ� */  
        
        //���Ժ�ngx_worker_process_init�е�channel�رղ�������Ķ�
        //�� ÿ������ channel[0]������Ϣ    
        
        //���ڸ����̶��ԣ���֪�����н��̵�channel[0]�� ֱ�ӿ������ӽ��̷������ 
        
        ngx_write_channel(ngx_processes[i].channel[0],
                          ch, sizeof(ngx_channel_t), cycle->log); //chΪ��������Ϣ��ngx_processes[i].channel[0]Ϊ����������Ϣ
    }
}

/*
NGX_PROCESS_JUST_RESPAWN��ʶ���ջ���ngx_spawn_process()����worker����ʱ����ngx_processes[s].just_spawn = 1���Դ���Ϊ����ɵ�worker���̵ı�ǡ�
֮��ִ�У�
ngx_signal_worker_processes(cycle, ngx_signal_value(NGX_SHUTDOWN_SIGNAL));  
�Դ˹رվɵ�worker���̡�����ú�������ᷢ����Ҳ��ѭ��������worker���̷����źţ����������ȰѾ�worker���̹رգ�Ȼ���ٹ����µ�worker���̡�
*/
static void     //ngx_reap_children��ngx_signal_worker_processes��Ӧ
ngx_signal_worker_processes(ngx_cycle_t *cycle, int signo) //����̷���signo�ź�
{
    ngx_int_t      i;
    ngx_err_t      err;
    ngx_channel_t  ch;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

#if (NGX_BROKEN_SCM_RIGHTS)

    ch.command = 0;

#else

    switch (signo) {

    case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
        ch.command = NGX_CMD_QUIT;
        break;

    case ngx_signal_value(NGX_TERMINATE_SIGNAL):
        ch.command = NGX_CMD_TERMINATE;
        break;

    case ngx_signal_value(NGX_REOPEN_SIGNAL):
        ch.command = NGX_CMD_REOPEN;
        break;

    default:
        ch.command = 0;
    }

#endif

    ch.fd = -1;


    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %d %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].detached || ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].just_spawn) {
            ngx_processes[i].just_spawn = 0;
            continue;
        }

        if (ngx_processes[i].exiting
            && signo == ngx_signal_value(NGX_SHUTDOWN_SIGNAL))
        {
            continue;
        }

        if (ch.command) {
            if (ngx_write_channel(ngx_processes[i].channel[0],
                                  &ch, sizeof(ngx_channel_t), cycle->log)
                == NGX_OK)
            {
                if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
                    ngx_processes[i].exiting = 1;
                }

                continue;
            }
        }

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                       "kill (%P, %d)", ngx_processes[i].pid, signo);

        if (kill(ngx_processes[i].pid, signo) == -1) { //�رվɵĽ��̣�  
            err = ngx_errno;
            ngx_log_error(NGX_LOG_ALERT, cycle->log, err,
                          "kill(%P, %d) failed", ngx_processes[i].pid, signo);

            if (err == NGX_ESRCH) {
                ngx_processes[i].exited = 1;
                ngx_processes[i].exiting = 0;
                ngx_reap = 1;
            }

            continue;
        }

        if (signo != ngx_signal_value(NGX_REOPEN_SIGNAL)) {
            ngx_processes[i].exiting = 1;
        }
    }
}

///������洦���˳����ӽ���(�е�worker�쳣�˳�����ʱ���Ǿ���Ҫ�������worker )����������ӽ��̶��˳���᷵��0. 
static ngx_uint_t
ngx_reap_children(ngx_cycle_t *cycle) //ngx_reap_children��ngx_signal_worker_processes��Ӧ
{
    ngx_int_t         i, n;
    ngx_uint_t        live;
    ngx_channel_t     ch;
    ngx_core_conf_t  *ccf;

    ngx_memzero(&ch, sizeof(ngx_channel_t));

    ch.command = NGX_CMD_CLOSE_CHANNEL;
    ch.fd = -1;

    live = 0;
    for (i = 0; i < ngx_last_process; i++) {

        ngx_log_debug7(NGX_LOG_DEBUG_EVENT, cycle->log, 0,
                       "child: %d %P e:%d t:%d d:%d r:%d j:%d",
                       i,
                       ngx_processes[i].pid,
                       ngx_processes[i].exiting,
                       ngx_processes[i].exited,
                       ngx_processes[i].detached,
                       ngx_processes[i].respawn,
                       ngx_processes[i].just_spawn);

        if (ngx_processes[i].pid == -1) {
            continue;
        }

        if (ngx_processes[i].exited) {

            if (!ngx_processes[i].detached) {
                ngx_close_channel(ngx_processes[i].channel, cycle->log);

                ngx_processes[i].channel[0] = -1;
                ngx_processes[i].channel[1] = -1;

                ch.pid = ngx_processes[i].pid;
                ch.slot = i;

                for (n = 0; n < ngx_last_process; n++) {
                    if (ngx_processes[n].exited
                        || ngx_processes[n].pid == -1
                        || ngx_processes[n].channel[0] == -1)
                    {
                        continue;
                    }

                    ngx_log_debug3(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                                   "pass close channel s:%i pid:%P to:%P",
                                   ch.slot, ch.pid, ngx_processes[n].pid);

                    /* TODO: NGX_AGAIN */

                    ngx_write_channel(ngx_processes[n].channel[0],
                                      &ch, sizeof(ngx_channel_t), cycle->log);
                }
            }

            if (ngx_processes[i].respawn
                && !ngx_processes[i].exiting
                && !ngx_terminate
                && !ngx_quit)
            {
                if (ngx_spawn_process(cycle, ngx_processes[i].proc,
                                      ngx_processes[i].data,
                                      ngx_processes[i].name, i)
                    == NGX_INVALID_PID)
                {
                    ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                                  "could not respawn %s",
                                  ngx_processes[i].name);
                    continue;
                }


                ch.command = NGX_CMD_OPEN_CHANNEL;
                ch.pid = ngx_processes[ngx_process_slot].pid;
                ch.slot = ngx_process_slot;
                ch.fd = ngx_processes[ngx_process_slot].channel[0];

                ngx_pass_open_channel(cycle, &ch);

                live = 1;

                continue;
            }

            if (ngx_processes[i].pid == ngx_new_binary) {

                ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx,
                                                       ngx_core_module);

                if (ngx_rename_file((char *) ccf->oldpid.data,
                                    (char *) ccf->pid.data)
                    == NGX_FILE_ERROR)
                {
                    ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                                  ngx_rename_file_n " %s back to %s failed "
                                  "after the new binary process \"%s\" exited",
                                  ccf->oldpid.data, ccf->pid.data, ngx_argv[0]);
                }

                ngx_new_binary = 0;
                if (ngx_noaccepting) {
                    ngx_restart = 1;
                    ngx_noaccepting = 0;
                }
            }

            if (i == ngx_last_process - 1) {
                ngx_last_process--;

            } else {
                ngx_processes[i].pid = -1;
            }

        } else if (ngx_processes[i].exiting || !ngx_processes[i].detached) {
            live = 1;
        }
    }

    return live;
}


static void
ngx_master_process_exit(ngx_cycle_t *cycle)
{
    ngx_uint_t  i;

    ngx_delete_pidfile(cycle);

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exit");

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->exit_master) {
            ngx_modules[i]->exit_master(cycle);
        }
    }

    ngx_close_listening_sockets(cycle);

    /*
     * Copy ngx_cycle->log related data to the special static exit cycle,
     * log, and log file structures enough to allow a signal handler to log.
     * The handler may be called when standard ngx_cycle->log allocated from
     * ngx_cycle->pool is already destroyed.
     */


    ngx_exit_log = *ngx_log_get_file_log(ngx_cycle->log);

    ngx_exit_log_file.fd = ngx_exit_log.file->fd;
    ngx_exit_log.file = &ngx_exit_log_file;
    ngx_exit_log.next = NULL;
    ngx_exit_log.writer = NULL;

    ngx_exit_cycle.log = &ngx_exit_log;
    ngx_exit_cycle.files = ngx_cycle->files;
    ngx_exit_cycle.files_n = ngx_cycle->files_n;
    ngx_cycle = &ngx_exit_cycle;

    ngx_destroy_pool(cycle->pool);

    exit(0);
}

/*
                                 |----------(ngx_worker_process_cycle->ngx_worker_process_init)
    ngx_start_worker_processes---| ngx_processes[]��صĲ�����ֵ����
                                 |----------ngx_pass_open_channel
*/
//��Nginx��ѭ�����������ѭ����ngx_worker_process_cycle�������У��ᶨ�ڵص����¼�ģ�飬�Լ���Ƿ��������¼�������
static void
ngx_worker_process_cycle(ngx_cycle_t *cycle, void *data) //data��ʾ���ǵڼ���worker����
{
    ngx_int_t worker = (intptr_t) data; //worker��ʾ�󶨵��ڼ���cpu��

    ngx_uint_t         i;
    ngx_connection_t  *c;

    ngx_process = NGX_PROCESS_WORKER;
    ngx_worker = worker;

    ngx_worker_process_init(cycle, worker); //��Ҫ�����ǰ�CPU�ͽ��̰�

    ngx_setproctitle("worker process");

    /*
    ��ngx_worker_process_cycle�з��У�ͨ�����ngx_exiting��ngx_terminate��ngx_quit��ngx_reopen��4����־λ����������������
    */
    for ( ;; ) {

        if (ngx_exiting) {

            c = cycle->connections;

            for (i = 0; i < cycle->connection_n; i++) {

                /* THREAD: lock */

                if (c[i].fd != -1 && c[i].idle) {
                    c[i].close = 1;
                    c[i].read->handler(c[i].read);
                }
            }

            ngx_event_cancel_timers();

            if (ngx_event_timer_rbtree.root == ngx_event_timer_rbtree.sentinel)
            {
                ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");

                ngx_worker_process_exit(cycle); 
            }
        }

        //ngx_log_debug1(NGX_LOG_DEBUG_EVENT, cycle->log, 0, "worker(%P) cycle again", ngx_pid);

        ngx_process_events_and_timers(cycle);

        if (ngx_terminate) { //û�йر��׽��֣�Ҳû�д���Ϊ��������¼�������ֱ��exit
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");

            ngx_worker_process_exit(cycle);
        }

        if (ngx_quit) {
            ngx_quit = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0,
                          "gracefully shutting down");
            ngx_setproctitle("worker process is shutting down");

            if (!ngx_exiting) {
                ngx_close_listening_sockets(cycle);
          /*
                ���ngx_exitingΪ1����ʼ׼���ر�worker���̡����ȣ����ݵ�ǰngx_cycle_t���������ڴ�������ӣ��������Ƕ�Ӧ�Ĺر����Ӵ�����
            �����ǽ������е�close��־λ��Ϊ1���ٵ��ö��¼��Ĵ��������ڵ�9���л���ϸ����Nginx���ӣ����������л���ӵĶ��¼���������
            �����ӹر��¼��󣬽����ngx_event timer_ rbtree����������������¼��Ķ�ʱ�����ڵ�9���л���������Ƿ�Ϊ�գ������Ϊ�գ���ʾ��
            ���¼���Ҫ��������������ִ�У�����ngx_process��events��and��timers���������¼������Ϊ�գ���ʾ�Ѿ����������е��¼�����ʱ����
            ������ģ���exit_process��������������ڴ�أ��˳�����worker���̡�
                ע��ngx_exiting��־λֻ��Ψһһ�δ������������Ҳ����������յ�QUIT�źš�ngx_quitֻ�У����״�����Ϊ1ʱ���ŻὫngx_exiting��Ϊ1��
            */
                ngx_exiting = 1;//��ʼquit��������Դ�ͷŲ������������if(ngx_exting)
            }
        }

       /*
         ʹ��-s reopen�����������´���־�ļ������������Ȱѵ�ǰ��־�ļ�������ת�Ƶ�����Ŀ¼�н��б��ݣ������´�ʱ�ͻ������µ���־�ļ���
         �������ʹ����־�ļ������ڹ��󡣵�Ȼ������ʹ��kill�����USR1�ź�Ч����ͬ��
        */
        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, -1);
        }
    }
}

/*
                                 |----------(ngx_worker_process_cycle->ngx_worker_process_init)
    ngx_start_worker_processes---| ngx_processes[]��صĲ�����ֵ����
                                 |----------ngx_pass_open_channel
*/
static void
ngx_worker_process_init(ngx_cycle_t *cycle, ngx_int_t worker)
{ //��Ҫ�����ǰ�CPU�ͽ��̰�  ����epoll_crate��
    sigset_t          set;
    uint64_t          cpu_affinity;
    ngx_int_t         n;
    ngx_uint_t        i;
    struct rlimit     rlmt;
    ngx_core_conf_t  *ccf;
    ngx_listening_t  *ls;

    if (ngx_set_environment(cycle, NULL) == NULL) {
        /* fatal */
        exit(2);
    }

    ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->conf_ctx, ngx_core_module);

    if (worker >= 0 && ccf->priority != 0) { /*�������ȼ�*/
        if (setpriority(PRIO_PROCESS, 0, ccf->priority) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setpriority(%d) failed", ccf->priority);
        }
    }

    if (ccf->rlimit_nofile != NGX_CONF_UNSET) {
        rlmt.rlim_cur = (rlim_t) ccf->rlimit_nofile;
        rlmt.rlim_max = (rlim_t) ccf->rlimit_nofile;

        //RLIMIT_NOFILEָ���˽��̿ɴ򿪵�����ļ������ʴ�һ��ֵ��������ֵ���������EMFILE����
        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setrlimit(RLIMIT_NOFILE, %i) failed",
                          ccf->rlimit_nofile);
        }
    }

    if (ccf->rlimit_core != NGX_CONF_UNSET) {
        rlmt.rlim_cur = (rlim_t) ccf->rlimit_core;
        rlmt.rlim_max = (rlim_t) ccf->rlimit_core;
        //�޸Ĺ������̵�core�ļ��ߴ�����ֵ����(RLIMIT_CORE)�������ڲ����������̵��������������ơ�
        if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "setrlimit(RLIMIT_CORE, %O) failed",
                          ccf->rlimit_core);
        }
        ngx_log_debugall(cycle->log, 0, "setrlimit(RLIMIT_CORE, &rlmt) OK,rlimit_core:%O",ccf->rlimit_core);
    }

    if (geteuid() == 0) {
        if (setgid(ccf->group) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "setgid(%d) failed", ccf->group);
            /* fatal */
            exit(2);
        }

        if (initgroups(ccf->username, ccf->group) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "initgroups(%s, %d) failed",
                          ccf->username, ccf->group);
        }

        if (setuid(ccf->user) == -1) {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          "setuid(%d) failed", ccf->user);
            /* fatal */
            exit(2);
        }
    }

    if (worker >= 0) {
        cpu_affinity = ngx_get_cpu_affinity(worker);

        if (cpu_affinity) {
            ngx_setaffinity(cpu_affinity, cycle->log);
        }
    }

#if (NGX_HAVE_PR_SET_DUMPABLE)

    /* allow coredump after setuid() in Linux 2.4.x */

    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "prctl(PR_SET_DUMPABLE) failed");
    }

#endif

    if (ccf->working_directory.len) { //·��������ڣ����򷵻ش���
        if (chdir((char *) ccf->working_directory.data) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "chdir(\"%s\") failed", ccf->working_directory.data);
            /* fatal */
            exit(2);
        }
        ngx_log_debugall(cycle->log, 0, "chdir %V OK", &ccf->working_directory);
    }

    sigemptyset(&set);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "sigprocmask() failed");
    }

    srandom((ngx_pid << 16) ^ ngx_time());

    /*
     * disable deleting previous events for the listening sockets because
     * in the worker processes there are no events at all at this point
     */
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
        ls[i].previous = NULL;
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->init_process) {
            if (ngx_modules[i]->init_process(cycle) == NGX_ERROR) { //ngx_event_process_init��
                /* fatal */
                exit(2);
            }
        }
    }

    /*
    
    ��socketpair��������sock[0]��sock[1]���ڸ����̺��ӽ��̵�ͨ�ţ���������ʹ������һ��socketʱ��ΪʲôҪ����close���ر��ӽ��̵�sock[1]���������£�
          int r = socketpair( AF_UNIX, SOCK_STREAM, 0, fd );
          if ( fork() ) {
              Parent process: echo client 
              int val = 0;
              close( fd[1] );          
               while ( 1 ) {
                sleep( 1 );
                ++val;
                printf( "Sending data: %d\n", val );
                write( fd[0], &val, sizeof(val) );
                read( fd[0], &val, sizeof(val) );
                printf( "Data received: %d\n", val );
              }
            }
            else {
               Child process: echo server 
              int val;
              close( fd[0] );
              while ( 1 ) {
                read( fd[1], &val, sizeof(val) );
                ++val;
                write( fd[1], &val, sizeof(val) );
              }
            }
          }
    ��������CSDN���ͣ�ת�������������http://blog.csdn.net/sunnyboychina/archive/2007/11/14/1884076.aspx 
    ------Solutions------
    ����socketpair����������socket���Ǵ򿪵ģ�fork���ӽ��̻�̳��������򿪵�socket��Ϊ��ʵ�ָ��ӽ���ͨ��socket pair�������ڹܵ���ͨ�ţ����뱣֤���ӽ��̷ֱ�openĳһ��socket�� 
    ------Solutions------
    ������ô��⣺���ӽ���һ��������socketд���ݣ�һ�����ж�ȡ���ݣ���д��ʱ��Ȼ���ܶ��ˣ�ͬ��������ʱ��Ͳ���д�ˡ��Ͳ���ϵͳ�е��ٽ���Դ��ࡣ 
    ------Solutions------
    лл���ǵĽ���Ҷ��ˡ��Ǻ�
    
    
    channel[0] ������������Ϣ�ģ�channel[1]������������Ϣ�ġ���ô���Լ����ԣ�����Ҫ���������̷�����Ϣ����Ҫ�����������̵�channel[0], 
    �ر�channel[1]; ���Լ����ԣ�����Ҫ�ر�channel[0]�� ����ngx_channel�ŵ�epoll�У��ӵ�һ�����еĽ������ǿ���֪�������ngx_channel
    ʵ�ʾ����Լ��� channel[1]����������Ϣ������ʱ��Ϳ���֪ͨ���ˡ�
    */
    //�ر����������ӽ��̶�Ӧ�� channel[1] �� �Լ��� channel[0]���Ӷ�ʵ���ӽ��̵�channel[1]�������̵�channel[0]ͨ��
    for (n = 0; n < ngx_last_process; n++) {

        if (ngx_processes[n].pid == -1) {
            continue;
        }

        if (n == ngx_process_slot) {
            continue;
        }

        if (ngx_processes[n].channel[1] == -1) {
            continue;
        }

        if (close(ngx_processes[n].channel[1]) == -1) { //�رճ�������������������н��̵Ķ���
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "close() channel failed");
        }
    }

    if (close(ngx_processes[ngx_process_slot].channel[0]) == -1) { //�رձ����̵�д�� ��ʣ�µ�һ��ͨ������ȫ˫����
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "close() channel failed");
    }

#if 0
    ngx_last_process = 0;
#endif

    //����epoll add ��ngx_chanel ����epoll ��  
    if (ngx_add_channel_event(cycle, ngx_channel, NGX_READ_EVENT,
                              ngx_channel_handler) //��ngx_spawn_process�и�ֵ
        == NGX_ERROR)
    {
        /* fatal */
        exit(2);
    }
}


static void
ngx_worker_process_exit(ngx_cycle_t *cycle)
{
    ngx_uint_t         i;
    ngx_connection_t  *c;

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->exit_process) {
            ngx_modules[i]->exit_process(cycle);
        }
    }

    if (ngx_exiting) {
        c = cycle->connections;
        for (i = 0; i < cycle->connection_n; i++) {
            if (c[i].fd != -1
                && c[i].read
                && !c[i].read->accept
                && !c[i].read->channel
                && !c[i].read->resolver)
            {
                ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                              "*%uA open socket #%d left in connection %ui",
                              c[i].number, c[i].fd, i);
                ngx_debug_quit = 1;
            }
        }

        if (ngx_debug_quit) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0, "aborting");
            ngx_debug_point();
        }
    }

    /*
     * Copy ngx_cycle->log related data to the special static exit cycle,
     * log, and log file structures enough to allow a signal handler to log.
     * The handler may be called when standard ngx_cycle->log allocated from
     * ngx_cycle->pool is already destroyed.
     */

    ngx_exit_log = *ngx_log_get_file_log(ngx_cycle->log);

    ngx_exit_log_file.fd = ngx_exit_log.file->fd;
    ngx_exit_log.file = &ngx_exit_log_file;
    ngx_exit_log.next = NULL;
    ngx_exit_log.writer = NULL;

    ngx_exit_cycle.log = &ngx_exit_log;
    ngx_exit_cycle.files = ngx_cycle->files;
    ngx_exit_cycle.files_n = ngx_cycle->files_n;
    ngx_cycle = &ngx_exit_cycle;

    ngx_destroy_pool(cycle->pool);

    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0, "exit");

    exit(0);
}

/*
�����ӽ���������δ�����أ��ӽ��̵Ĺܵ��ɶ��¼���׽������ngx_channel_handler(ngx_event_t *ev)������������У����ȡmseeage��
Ȼ������������ݲ�ͬ����������ͬ�Ĵ����������Ĵ���Ƭ�ϣ� 
*/ //��ngx_write_channel��Ӧ
static void
ngx_channel_handler(ngx_event_t *ev)
{
    ngx_int_t          n;
    ngx_channel_t      ch;
    ngx_connection_t  *c;

    if (ev->timedout) {
        ev->timedout = 0;
        return;
    }

    c = ev->data;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel handler");

    for ( ;; ) {

        n = ngx_read_channel(c->fd, &ch, sizeof(ngx_channel_t), ev->log);

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0, "channel: %i", n);

        if (n == NGX_ERROR) {

            if (ngx_event_flags & NGX_USE_EPOLL_EVENT) {
                ngx_del_conn(c, 0);
            }

            ngx_close_connection(c);
            return;
        }

        if (ngx_event_flags & NGX_USE_EVENTPORT_EVENT) {
            char tmpbuf[256];
        
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_READ_EVENT(et) read add", NGX_FUNC_LINE);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0, tmpbuf);
            if (ngx_add_event(ev, NGX_READ_EVENT, 0) == NGX_ERROR) {
                return;
            }
        }

        if (n == NGX_AGAIN) {
            return;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ev->log, 0,
                       "channel command: %d", ch.command);

        switch (ch.command) {

        case NGX_CMD_QUIT:
            ngx_quit = 1;
            break;

        case NGX_CMD_TERMINATE:
            ngx_terminate = 1;
            break;

        case NGX_CMD_REOPEN:
            ngx_reopen = 1;
            break;

        case NGX_CMD_OPEN_CHANNEL:

            ngx_log_debug3(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "get channel s:%i pid:%P fd:%d",
                           ch.slot, ch.pid, ch.fd);

            //��ngx_processesȫ�ֽ��̱���и�ֵ��  
            ngx_processes[ch.slot].pid = ch.pid;
            ngx_processes[ch.slot].channel[0] = ch.fd;
            break;

        case NGX_CMD_CLOSE_CHANNEL:

            ngx_log_debug4(NGX_LOG_DEBUG_CORE, ev->log, 0,
                           "close channel s:%i pid:%P our:%P fd:%d",
                           ch.slot, ch.pid, ngx_processes[ch.slot].pid,
                           ngx_processes[ch.slot].channel[0]);

            if (close(ngx_processes[ch.slot].channel[0]) == -1) {
                ngx_log_error(NGX_LOG_ALERT, ev->log, ngx_errno,
                              "close() channel failed");
            }

            ngx_processes[ch.slot].channel[0] = -1;
            break;
        }
    }
}

/*
���˳䵱�����������nginx������ʹ����varnish/squid�Ļ���ְ�𣬼����ͻ��˵��������ݻ�����Nginx���������´�ͬ������������nginxֱ�ӷ��أ�
�����˱������������ѹ����cacheʹ��һ�鹫���ڴ����򣨹����ڴ棩����Ż�����������ݣ�Nginx����ʱcache loader���̽����̻���Ķ����ļ�
(cycle->pathes���Ժ������֯)���ص��ڴ��У�������Ϻ��Զ��˳���ֻ�п�����proxy buffer����ʹ��proxy cache��

ע1������������������ص�httpͷ����no-store/no-cache/private/max-age=0����expires������������ʱ�������Ӧ���ݲ���nginx���棻
*/ //���Ӧ��������ngx_http_upstream_process_request->ngx_http_file_cache_update�н��л���
static void  
ngx_cache_manager_process_cycle(ngx_cycle_t *cycle, void *data)
{ //nginx: cache loader process���̺�nginx: cache manager process����ִ�иú���
    ngx_cache_manager_ctx_t *ctx = data;

    void         *ident[4];
    ngx_event_t   ev;

    /*
     * Set correct process type since closing listening Unix domain socket
     * in a master process also removes the Unix domain socket file.
     */
    ngx_process = NGX_PROCESS_HELPER;

    ngx_close_listening_sockets(cycle);

    /* Set a moderate number of connections for a helper process. */
    cycle->connection_n = 512;

    ngx_worker_process_init(cycle, -1);

    ngx_memzero(&ev, sizeof(ngx_event_t));
    ev.handler = ctx->handler; //ngx_cache_manager_process_handler  ngx_cache_loader_process_handler
    ev.data = ident;
    ev.log = cycle->log;
    ident[3] = (void *) -1;

    ngx_use_accept_mutex = 0;

    ngx_setproctitle(ctx->name);

    ngx_add_timer(&ev, ctx->delay, NGX_FUNC_LINE); //ctx->dealy��ִ��ctx->handler;
    
    for ( ;; ) {

        if (ngx_terminate || ngx_quit) {
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "exiting");
            exit(0);
        }

        if (ngx_reopen) {
            ngx_reopen = 0;
            ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "reopening logs");
            ngx_reopen_files(cycle, -1);
        }

        ngx_process_events_and_timers(cycle);
    }
}

/*
��Nginx�У����������proxy(fastcgi) cache���ܣ�master process����������ʱ������������������ӽ���(�����ڴ���������ӽ���)��������
��ʹ��̵Ļ�����塣��һ�����̵Ĺ����Ƕ��ڼ�黺�棬�������ڵĻ���ɾ�����ڶ������̵���������������ʱ�򽫴������Ѿ�����ĸ�
��ӳ�䵽�ڴ���(ĿǰNginx�趨Ϊ�����Ժ�60��)��Ȼ���˳���

����ģ������������̵�ngx_process_events_and_timers()�����У������ngx_event_expire_timers()��Nginx��ngx_event_timer_rbtree(�����)��
�水��ִ�е�ʱ����Ⱥ�����һϵ�е��¼���ÿ��ȡִ��ʱ��������¼��������ǰʱ���Ѿ�����Ӧ��ִ�и��¼����ͻ�����¼���handler������
���̵�handler�ֱ���ngx_cache_manager_process_handler��ngx_cache_loader_process_handler //���Ӧ��������ngx_http_upstream_process_request->ngx_http_file_cache_update�н��л���
*/

//�������������ÿ�����̻���·����Ӧ��manager()��������ngx_http_file_cache_manager()��������������ܼ򵥣����Ǽ�黺����У���
//�����������Ϣ��û�й��ڣ�������ڣ��Ͱѻ�����ļ��Ӵ���ɾ���������������ڴ����ͷš�
static void   //ngx_cache_manager_ctx
//cache manager ����ά�������ļ�������������ڵĻ����� Ŀ��ͬʱ����Ҳ���黺��Ŀ¼�ܴ�С����������������ƵĻ���ǿ����������ϵĻ��� ��Ŀ�� 
ngx_cache_manager_process_handler(ngx_event_t *ev)
{
    time_t        next, n;
    ngx_uint_t    i;
    ngx_path_t  **path;

    next = 60 * 60; //60S

    path = ngx_cycle->paths.elts;
    for (i = 0; i < ngx_cycle->paths.nelts; i++) { //�������е�cacheĿ¼
        //����manger�ص�
        if (path[i]->manager) {
            n = path[i]->manager(path[i]->data);//manager �ص�������Ҫ���������������Ҫ���ڵĻ�����Ŀ�൱ǰʱ���ļ��ʱ����
            //ȡ����һ�εĶ�ʱ����ʱ�䣬���Կ�����ȡn��next����Сֵ
            next = (n <= next) ? n : next; //��һ��������ڵ�ʱ�䣬ͨ�������ngx_add_timer�Ӷ�������nextʱ�䵽���������

            ngx_time_update();
        }
    }

    //cache manager ���̼�黺����Ŀ��Ч�Եļ���Ϊ 1 ��Сʱ�����ң�Ϊ�˱���ռ�ù��� CPU�� cache manger ��̼������֤Ϊ 1 �롣 
    if (next == 0) {
        next = 1;
    }

    ngx_add_timer(ev, next * 1000, NGX_FUNC_LINE); //��ʱִ��ngx_cache_manager_process_handler->ngx_http_file_cache_manager
}

/*
��Nginx�У����������proxy(fastcgi) cache���ܣ�master process����������ʱ������������������ӽ���(�����ڴ���������ӽ���)��������
��ʹ��̵Ļ�����塣��һ�����̵Ĺ����Ƕ��ڼ�黺�棬�������ڵĻ���ɾ�����ڶ������̵���������������ʱ�򽫴������Ѿ�����ĸ�
��ӳ�䵽�ڴ���(ĿǰNginx�趨Ϊ�����Ժ�60��)��Ȼ���˳���

Nginx ��������ʱ������ͼ�ӻ����Ӧ���ļ�ϵͳ·���µ��ļ���ȡ��Ҫ���ݣ�Ȼ���ؽ� ������ڴ�ṹ����������� cache loader ������ɡ� 

ͬʱ����פ Nginx �ӽ��� cache manager ����ά�������ļ�������������ڵĻ����� Ŀ��ͬʱ����Ҳ���黺��Ŀ¼�ܴ�С����������������ƵĻ���
ǿ����������ϵĻ��� ��Ŀ�� 

����ģ������������̵�ngx_process_events_and_timers()�����У������ngx_event_expire_timers()��Nginx��ngx_event_timer_rbtree(�����)��
�水��ִ�е�ʱ����Ⱥ�����һϵ�е��¼���ÿ��ȡִ��ʱ��������¼��������ǰʱ���Ѿ�����Ӧ��ִ�и��¼����ͻ�����¼���handler������
���̵�handler�ֱ���ngx_cache_manager_process_handler��ngx_cache_loader_process_handler 
//���Ӧ��������ngx_http_upstream_process_request->ngx_http_file_cache_update�н��л���
*/
static void //ngx_cache_loader_ctx
ngx_cache_loader_process_handler(ngx_event_t *ev) //�ӻ����Ӧ���ļ�ϵͳ·���µ��ļ���ȡ��Ҫ���ݣ�Ȼ���ؽ� ������ڴ�ṹ
{//ngx_cache_manager_process_cycle��ִ�У���������60s��ſ�ʼָ��ú�����ͨ����ngx_cache_manager_process_cycle���60s��ʱ����Ȼ�����ִ�иú���
    ngx_uint_t     i;
    ngx_path_t   **path;
    ngx_cycle_t   *cycle;

    cycle = (ngx_cycle_t *) ngx_cycle;

    path = cycle->paths.elts;
    for (i = 0; i < cycle->paths.nelts; i++) {

        if (ngx_terminate || ngx_quit) {
            break;
        }

        if (path[i]->loader) {
            path[i]->loader(path[i]->data); //ngx_http_file_cache_loader
            ngx_time_update();
        }
    }

    exit(0);
}
