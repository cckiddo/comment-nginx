
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_


#include <ngx_setaffinity.h>
#include <ngx_setproctitle.h>


typedef pid_t       ngx_pid_t;

#define NGX_INVALID_PID  -1

/*
Worker���̵Ĺ���ѭ��ngx_worker_process_cycle����Ҳ������ngx_spawn_proc_pt�������
cacheManage���̻���cache_loader���̵Ĺ���ѭ��ngx_cache_manager_process_cycle����Ҳ�����
*/
//ngx_spawn_process�����е���
typedef void (*ngx_spawn_proc_pt) (ngx_cycle_t *cycle, void *data);

/*
�ڽ���master��������ǰ������Ҫ��master���̹����ӽ��̵����ݽṹ�и������˽⡣���涨����pgx_processesȫ�����飬��Ȼ�ӽ�����Ҳ��
��ngx_processes���飬�������������Ǹ�master����ʹ�õ�

master�����������ӽ�����ص�״̬��Ϣ��������ngx_processes�����С�������һ������Ԫ�ص�����ngx_process_t�ṹ��Ķ��壬�������¡�
*/
typedef struct {
    ngx_pid_t           pid; //����ID
    int                 status; //�ӽ����˳��󣬸������յ�SIGCHLD�󣬿�ʼwaitpid����������waitpidϵͳ���û�ȡ���Ľ���״̬����ngx_process_get_status

    /*
    ������socketpairϵͳ���ò����������ڽ��̼�ͨ�ŵ�socket�������һ��socket������Ի���ͨ�ţ�Ŀǰ����master��������worker�ӽ����ʵ�ͨ�ţ����14.4��
    */
    ngx_socket_t        channel[2];//socketpairʵ������ͨ���ܵ���װʵ�ֵ� ngx_spawn_process�и�ֵ

    //�ӽ��̵�ѭ��ִ�з������������̵���ngx_spawn_proces�����ӽ���ʱʹ��
    ngx_spawn_proc_pt   proc;

    /*
    �����ngx_spawn_proc_pt�����е�2��������Ҫ����1��ָ�룬���ǿ�ѡ�ġ����磬worker�ӽ��̾Ͳ���Ҫ����cache manage����
    ����Ҫngx_cache_manager_ctx�����ĳ�Ա����ʱ��dataһ����ngx_spawn_proc_pt�����е�2�������ǵȼ۵�
    */
    void               *data;
    char               *name;//�������ơ�����ϵͳ����ʾ�Ľ���������name��ͬ

    //һ����Щǰ���������ngx_spawn_process�и�ֵ
    unsigned            respawn:1; //��־λ��Ϊ1ʱ��ʾ�����������ӽ���
    unsigned            just_spawn:1; //��־λ��Ϊ1ʱ��ʾ���������ӽ���
    unsigned            detached:1; //��־λ��Ϊ1ʱ��ʾ�ڽ��и����ӽ��̷���
    unsigned            exiting:1;//��־λ��Ϊ1ʱ��ʾ���������˳�
    unsigned            exited:1;//��־λ��Ϊ1ʱ��ʾ�����Ѿ��˳�  ���ӽ����˳��󣬸������յ�SIGCHLD�󣬿�ʼwaitpid,��ngx_process_get_status
} ngx_process_t;

typedef struct {//��ֵ��ngx_exec_new_binary
    char         *path; 
    char         *name;
    char *const  *argv;
    char *const  *envp;
} ngx_exec_ctx_t;

//����1024��Ԫ�ص�ngx_processes���飬Ҳ�������ֻ����1024���ӽ���
#define NGX_MAX_PROCESSES         1024

/*
�ڷ���ngx_spawn_process()�����½���ʱ�����˽��½������ԡ�ͨ�׵�˵���ǽ��̹����費��Ҫ������
��Դ���У�nginx_process.h�У������¼������Ա�ʶ��

NGX_PROCESS_NORESPAWN    ���ӽ����˳�ʱ,�����̲����ٴ�����
NGX_PROCESS_JUST_SPAWN   ��--
NGX_PROCESS_RESPAWN      ���ӽ����쳣�˳�ʱ,��������Ҫ����
NGX_PROCESS_JUST_RESPAWN ��--
NGX_PROCESS_DETACHED     ���ȴ����滻����ʱ�����������ڲ�����Nginx������½����������

NGX_PROCESS_JUST_RESPAWN��ʶ���ջ���ngx_spawn_process()����worker����ʱ����ngx_processes[s].just_spawn = 1���Դ���Ϊ����ɵ�worker���̵ı�ǡ�
*/
#define NGX_PROCESS_NORESPAWN     -1  //�ӽ����˳�ʱ,�����̲����ٴ�����
#define NGX_PROCESS_JUST_SPAWN    -2
#define NGX_PROCESS_RESPAWN       -3  //�ӽ����쳣�˳�ʱ,��������Ҫ����
#define NGX_PROCESS_JUST_RESPAWN  -4   
#define NGX_PROCESS_DETACHED      -5  //�ȴ����滻����ʱ�����������ڲ�����Nginx������½����������


#define ngx_getpid   getpid

#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid 
//ngx_log_pid, ngx_log_tid����ID���߳�ID(���̺߳źͽ��̺���ͬ���ڿ����̳߳ص�ʱ���߳�ID�ͽ���ID��ͬ),��־�ļ��л��¼
#endif


ngx_pid_t ngx_spawn_process(ngx_cycle_t *cycle,
    ngx_spawn_proc_pt proc, void *data, char *name, ngx_int_t respawn);
ngx_pid_t ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx);
ngx_int_t ngx_init_signals(ngx_log_t *log);
void ngx_debug_point(void);


#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif


extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

extern ngx_pid_t      ngx_pid;
extern ngx_socket_t   ngx_channel;
extern ngx_int_t      ngx_process_slot;
extern ngx_int_t      ngx_last_process;
extern ngx_process_t  ngx_processes[NGX_MAX_PROCESSES];


#endif /* _NGX_PROCESS_H_INCLUDED_ */
