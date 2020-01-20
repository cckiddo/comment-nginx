
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
#include <ngx_channel.h>


//�źŷ��ͼ�ngx_os_signal_process
typedef struct { //signals
    int     signo;   //��Ҫ������ź�
    char   *signame; //�źŶ�Ӧ���ַ�������
    char   *name;    //����źŶ�Ӧ�ŵ�Nginx����
    void  (*handler)(int signo); //�յ�signo�źź�ͻ�ص�handler����
} ngx_signal_t;

static void ngx_execute_proc(ngx_cycle_t *cycle, void *data);
static void ngx_signal_handler(int signo);
static void ngx_process_get_status(void);
static void ngx_unlock_mutexes(ngx_pid_t pid);

int              ngx_argc;
char           **ngx_argv; //���ִ��nginxʱ�������Ĳ����� ��ngx_save_argv
char           **ngx_os_argv; //ָ��nginx����ʱ�������Ĳ�������ngx_save_argv

//��ǰ�����Ľ�����ngx_processes�����е��±�
ngx_int_t        ngx_process_slot;
ngx_socket_t     ngx_channel;  //�洢�����ӽ��̵�����  ngx_spawn_process�и�ֵ  ngx_channel = ngx_processes[s].channel[1]

//ngx_processes�������������ngx_process_tԪ���������±�
ngx_int_t        ngx_last_process;

/*
�ڽ���master��������ǰ������Ҫ��master���̹����ӽ��̵����ݽṹ�и������˽⡣���涨����pgx_processesȫ�����飬��Ȼ�ӽ�����Ҳ��
��ngx_processes���飬�������������Ǹ�master����ʹ�õ�
*/
ngx_process_t    ngx_processes[NGX_MAX_PROCESSES]; //�洢�����ӽ��̵�����  ngx_spawn_process�и�ֵ

//�źŷ��ͼ�ngx_os_signal_process �źŴ�����ngx_signal_handler
ngx_signal_t  signals[] = {
    { ngx_signal_value(NGX_RECONFIGURE_SIGNAL),
      "SIG" ngx_value(NGX_RECONFIGURE_SIGNAL),
      "reload",
      /* reloadʵ������ִ��reload��nginx������ԭmaster+worker�е�master���̷���reload�źţ�Դmaster�յ��������µ�worker���̣�ͬʱ��Դworker
         ���̷���quit�źţ������Ǵ��������е�������Ϣ���˳���������ֻ���µ�worker�������С�
      */
      ngx_signal_handler }, 

    { ngx_signal_value(NGX_REOPEN_SIGNAL),
      "SIG" ngx_value(NGX_REOPEN_SIGNAL),
      "reopen",
      ngx_signal_handler },

    { ngx_signal_value(NGX_NOACCEPT_SIGNAL),
      "SIG" ngx_value(NGX_NOACCEPT_SIGNAL),
      "",
      ngx_signal_handler },

    { ngx_signal_value(NGX_TERMINATE_SIGNAL),
      "SIG" ngx_value(NGX_TERMINATE_SIGNAL),
      "stop",
      ngx_signal_handler },

    { ngx_signal_value(NGX_SHUTDOWN_SIGNAL),
      "SIG" ngx_value(NGX_SHUTDOWN_SIGNAL),
      "quit",
      ngx_signal_handler },

    { ngx_signal_value(NGX_CHANGEBIN_SIGNAL),
      "SIG" ngx_value(NGX_CHANGEBIN_SIGNAL),
      "",
      ngx_signal_handler },

    { SIGALRM, "SIGALRM", "", ngx_signal_handler },

    { SIGINT, "SIGINT", "", ngx_signal_handler },

    { SIGIO, "SIGIO", "", ngx_signal_handler },

    { SIGCHLD, "SIGCHLD", "", ngx_signal_handler },

    { SIGSYS, "SIGSYS, SIG_IGN", "", SIG_IGN },

    { SIGPIPE, "SIGPIPE, SIG_IGN", "", SIG_IGN },

    { 0, NULL, "", NULL }
};

/*
master������������һ���ӽ����أ���ʵ�ܼ򵥣�forkϵͳ���ü�������ɡ�ngx_spawn_process������װ��forkϵͳ���ã�
���һ��ngx_processes������ѡ��һ����δʹ�õ�ngx_process_tԪ�ش洢����ӽ��̵������Ϣ���������1024����ŦԪ�����Ѿ�û�п�
���Ԫ�أ�Ҳ����˵���ӽ��̸������������ֵ1024����ô���᷵��NGX_INVALID_PID����ˣ�ngx_processes������Ԫ�صĳ�ʼ������ngx_spawn_process�����н��С�
*/
//��һ��������ȫ�ֵ����ã��ڶ����������ӽ�����Ҫִ�еĺ�����������������proc�Ĳ��������ĸ����͡�  name���ӽ��̵�����
ngx_pid_t ngx_spawn_process(ngx_cycle_t *cycle, ngx_spawn_proc_pt proc, void *data,
    char *name, ngx_int_t respawn) //respawnȡֵΪNGX_PROCESS_RESPAWN�ȣ�����Ϊ������ngx_processes[]�е����
{
    u_long     on;
    ngx_pid_t  pid;
    ngx_int_t  s; //��Ҫ�������ӽ����ڽ��̱��е�λ��   

    // ���respawn��С��0������Ϊ��ǰ�����Ѿ��˳�����Ҫ����  
    if (respawn >= 0) {  //�滻����ngx_processes[respawn],�ɰ�ȫ���øý��̱���  
        s = respawn; 

    } else {
        for (s = 0; s < ngx_last_process; s++) { 
            if (ngx_processes[s].pid == -1) { //���ҵ�һ�������յĽ��̱���   
                break;
            }
        }

        if (s == NGX_MAX_PROCESSES) { //���ֻ�ܴ���1024���ӽ���
            ngx_log_error(NGX_LOG_ALERT, cycle->log, 0,
                          "no more than %d processes can be spawned",
                          NGX_MAX_PROCESSES);
            return NGX_INVALID_PID;
        }
    }


    if (respawn != NGX_PROCESS_DETACHED) { //���Ƿ�����ӽ���      /* �����ȴ����滻 */

        /* Solaris 9 still has no AF_LOCAL */
       
        /* 
          �����൱��Master���̵���socketpair()Ϊ�µ�worker���̴���һ��ȫ˫����socket 
            
          ʵ����socketpair ������pipe ���������Ƶģ�Ҳֻ����ͬ�������Ͼ�����Ե��ϵ�Ľ��̼�ͨ�ţ���pipe �����������ܵ��ǰ�˫���ģ�
          ��socketpair ������Ϊ�Ǵ���һ��ȫ˫���Ĺܵ���

          int socketpair(int domain, int type, int protocol, int sv[2]);
          ����������Դ���һ�Թ������׽���sv[2]���������ν�������4������������d��ʾ����Linux��ͨ��ȡֵΪAF UNIX��typeȡֵΪSOCK��
          STREAM����SOCK��DGRAM������ʾ���׽�����ʹ�õ���TCP����UDP; protocol���봫��0��sv[2]��һ����������Ԫ�ص��������飬ʵ���Ͼ�
          �������׽��֡���socketpair����0ʱ��sv[2]�������׽��ִ����ɹ�������socketpair����һ1��ʾʧ�ܡ�
             ��socketpairִ�гɹ�ʱ��sv[2]�������׽��־߱����й�ϵ����sv[0]�׽���д�����ݣ������Դ�sv[l]�׽����ж�ȡ����д������ݣ�
          ͬ������sv[l]�׽���д�����ݣ�Ҳ���Դ�sv[0]�ж�ȡ��д������ݡ�ͨ�����ڸ����ӽ���ͨ��ǰ�����ȵ���socketpair������������һ��
          �׽��֣��ڵ���fork�����������ӽ��̺󣬽����ڸ������йر�sv[l]�׽��֣���ʹ��sv[0]�׽����������ӽ��̷��������Լ������ӽ��̷�
          ���������ݣ������ӽ�������ر�sv[0]�׽��֣���ʹ��sv[l]�׽��ּȿ��Խ��ո����̷��������ݣ�Ҳ�����򸸽��̷������ݡ�
          ע��socketpair��Э����ΪAF_UNIX UNXI��
          */  
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, ngx_processes[s].channel) == -1) //��ngx_worker_process_init����ӵ��¼���
        {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "socketpair() failed while spawning \"%s\"", name);
            return NGX_INVALID_PID;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, cycle->log, 0,
                       "channel %d:%d",
                       ngx_processes[s].channel[0],
                       ngx_processes[s].channel[1]);

        /* ����master��channel[0](��д�˿�)��channel[1](�����˿�)��Ϊ��������ʽ */  
        if (ngx_nonblocking(ngx_processes[s].channel[0]) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          ngx_nonblocking_n " failed while spawning \"%s\"",
                          name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        if (ngx_nonblocking(ngx_processes[s].channel[1]) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          ngx_nonblocking_n " failed while spawning \"%s\"",
                          name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
        
        /* 
            �����첽ģʽ�� ������Կ��¡������̾�һ����ioctl������fcntl���� or ���ϲ�ѯ 
          */  
        on = 1; // ���λ��ioctl���������0�������ã���0������  

        /* 
          ����channel[0]���ź������첽I/O��־ 
          FIOASYNC����״̬��־�����Ƿ���ȡ���socket���첽I/O�źţ�SIGIO�� 
          ����O_ASYNC�ļ�״̬��־��Ч����ͨ��fcntl��F_SETFL��������or��� 
         */ 
        if (ioctl(ngx_processes[s].channel[0], FIOASYNC, &on) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "ioctl(FIOASYNC) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        /* F_SETOWN������ָ������SIGIO��SIGURG�źŵ�socket����������ID�������ID�� 
          * ������˼��ָ��Master���̽���SIGIO��SIGURG�ź� 
          * SIGIO�źű�������socket����Ϊ�ź������첽I/O���ܲ���������һ������ 
          * SIGURG�ź������µĴ������ݵ���socketʱ������ 
         */ 
        if (fcntl(ngx_processes[s].channel[0], F_SETOWN, ngx_pid) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(F_SETOWN) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
            
        
        /* FD_CLOEXEC�����������ļ���close-on-exec״̬��׼ 
          *             ��exec()���ú�close-on-exec��־Ϊ0������£����ļ������رգ���������exec()�󱻹ر� 
          *             Ĭ��close-on-exec״̬Ϊ0����Ҫͨ��FD_CLOEXEC���� 
          *     ������˼�ǵ�Master������ִ����exec()���ú󣬹ر�socket        
          */
        if (fcntl(ngx_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        
        /* ͬ�ϣ�������˼�ǵ�Worker�ӽ���ִ����exec()���ú󣬹ر�socket */  
        if (fcntl(ngx_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
        
       /* 
         ���ü��������ӽ��̵�channel ������ں�����õ����ں��洴�����ӽ��̵�cycleѭ��ִ�к����л��õ�������ngx_worker_process_init -> ngx_add_channel_event   
         �Ӷ����ӽ��̵�channel[1]������ӵ�epool�У����ڶ�ȡ�����̷��͵�ngx_channel_t��Ϣ
        */  
        ngx_channel = ngx_processes[s].channel[1];

    } else {
        ngx_processes[s].channel[0] = -1;
        ngx_processes[s].channel[1] = -1;
    }

    ngx_process_slot = s; // ��һ������ngx_pass_open_channel()���õ������������±꣬����Ѱ�ұ��δ������ӽ���  

    pid = fork();

    switch (pid) {

    case -1:
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "fork() failed while spawning \"%s\"", name);
        ngx_close_channel(ngx_processes[s].channel, cycle->log);
        return NGX_INVALID_PID;

    case 0: //�ӽ���
        ngx_pid = ngx_getpid();  // �����ӽ���ID  
        //printf(" .....slave......pid:%u, %u\n", pid, ngx_pid); slave......pid:0, 14127
        proc(cycle, data); // ����proc�ص���������ngx_worker_process_cycle��֮��worker�ӽ��̴����￪ʼִ��  
        break;

    default: //�����̣�������ʱ���ӡ��pidΪ�ӽ���ID
        //printf(" ......master.....pid:%u, %u\n", pid, ngx_pid); master.....pid:14127, 14126
        break;
    }

    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "start %s %P", name, pid);

    /* ��һ������������ngx_process_t�ĳ�Ա���� */  
    ngx_processes[s].pid = pid;
    ngx_processes[s].exited = 0;

    if (respawn >= 0) { /* �������0,��˵�����������ӽ��̣��������ĳ�ʼ���������ظ��� */  
        return pid;
    }

    ngx_processes[s].proc = proc;
    ngx_processes[s].data = data;
    ngx_processes[s].name = name;
    ngx_processes[s].exiting = 0;

    switch (respawn) {/* OK��Ҳ����˵�ˣ���������״̬��Ϣ */  

    case NGX_PROCESS_NORESPAWN:
        ngx_processes[s].respawn = 0;
        ngx_processes[s].just_spawn = 0;
        ngx_processes[s].detached = 0;
        break;

    case NGX_PROCESS_JUST_SPAWN:
        ngx_processes[s].respawn = 0;
        ngx_processes[s].just_spawn = 1;
        ngx_processes[s].detached = 0;
        break;

    case NGX_PROCESS_RESPAWN:
        ngx_processes[s].respawn = 1;
        ngx_processes[s].just_spawn = 0;
        ngx_processes[s].detached = 0;
        break;

    case NGX_PROCESS_JUST_RESPAWN:
        ngx_processes[s].respawn = 1;
        ngx_processes[s].just_spawn = 1;
        ngx_processes[s].detached = 0;
        break;

    case NGX_PROCESS_DETACHED:
        ngx_processes[s].respawn = 0;
        ngx_processes[s].just_spawn = 0;
        ngx_processes[s].detached = 1;
        break;
    }

    if (s == ngx_last_process) {
        ngx_last_process++;
    }

    return pid;
}


ngx_pid_t
ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx)
{
    return ngx_spawn_process(cycle, ngx_execute_proc, ctx, ctx->name,
                             NGX_PROCESS_DETACHED);
}


static void
ngx_execute_proc(ngx_cycle_t *cycle, void *data)
{
    ngx_exec_ctx_t  *ctx = data;

    /*
    execve()����ִ�в���filename�ַ�����������ļ�·�����ڶ�������������ָ�����������ݸ�ִ���ļ���������
    Ҫ�Կ�ָ��(NULL)���������һ��������Ϊ���ݸ�ִ���ļ����»����������顣
    */
    if (execve(ctx->path, ctx->argv, ctx->envp) == -1) { //�Ѿ�master����bind������fdд�뵽��������NGINX_VAR��
        ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                      "execve() failed while executing %s \"%s\"",
                      ctx->name, ctx->path);
    }

    exit(1);
}

/*
һ���źż��ź���Դ
�źű���

�ź������������϶��жϻ��Ƶ�һ��ģ�⣬��ԭ���ϣ�һ�������յ�һ���ź��봦�����յ�һ���ж��������˵��һ���ġ��ź����첽�ģ�һ�����̲���ͨ���κβ������ȴ��źŵĵ����ʵ�ϣ�����Ҳ��֪���źŵ���ʲôʱ�򵽴

�ź��ǽ��̼�ͨ�Ż�����Ψһ���첽ͨ�Ż��ƣ����Կ������첽֪ͨ��֪ͨ�����źŵĽ�������Щ���鷢���ˡ��źŻ��ƾ���POSIXʵʱ��չ�󣬹��ܸ���ǿ�󣬳��˻���֪ͨ�����⣬�����Դ��ݸ�����Ϣ��

�ź���Դ

�ź��¼��ķ�����������Դ��Ӳ����Դ(�������ǰ����˼��̻�������Ӳ������)�������Դ����÷����źŵ�ϵͳ������kill, raise, alarm��setitimer�Լ�sigqueue�����������Դ������һЩ�Ƿ�����Ȳ�����


--------------------------------------------------------------------------------
��ҳ��
�����źŵ�����
���Դ�������ͬ�ķ���Ƕȶ��źŽ��з��ࣺ��1���ɿ��Է��棺�ɿ��ź��벻�ɿ��źţ���2����ʱ��Ĺ�ϵ�ϣ�ʵʱ�ź����ʵʱ�źš��ڡ�Linux�������̼�ͨ�ţ�һ�����ܵ��������ܵ����ĸ�1���г���ϵͳ��֧�ֵ������źš�

1���ɿ��ź��벻�ɿ��ź�
"���ɿ��ź�"

Linux�źŻ��ƻ������Ǵ�Unixϵͳ�м̳й����ġ�����Unixϵͳ�е��źŻ��ƱȽϼ򵥺�ԭʼ��������ʵ���б�¶��һЩ���⣬��ˣ�����Щ���������ڻ����ϵ��źŽ���"���ɿ��ź�"���ź�ֵС��SIGRTMIN(Red hat 7.2�У�SIGRTMIN=32��SIGRTMAX=63)���źŶ��ǲ��ɿ��źš������"���ɿ��ź�"����Դ��������Ҫ�����ǣ�

����ÿ�δ����źź󣬾ͽ����źŵ���Ӧ����ΪĬ�϶�������ĳЩ����£������¶��źŵĴ�������ˣ��û������ϣ�������Ĳ�������ô��Ҫ���źŴ�������β��һ�ε���signal()�����°�װ���źš�
�źſ��ܶ�ʧ�����潫�Դ���ϸ������ 
��ˣ�����unix�µĲ��ɿ��ź���Ҫָ���ǽ��̿��ܶ��ź���������ķ�Ӧ�Լ��źſ��ܶ�ʧ�� Linux֧�ֲ��ɿ��źţ����ǶԲ��ɿ��źŻ������˸Ľ����ڵ������źŴ������󣬲������µ��ø��źŵİ�װ�������źŰ�װ�������ڿɿ������ϵ�ʵ�֣�����ˣ�Linux�µĲ��ɿ��ź�������Ҫָ�����źſ��ܶ�ʧ��

"�ɿ��ź�"

����ʱ��ķ�չ��ʵ��֤�����б�Ҫ���źŵ�ԭʼ���Ƽ��ԸĽ������䡣���ԣ��������ֵĸ���Unix�汾�ֱ����ⷽ��������о�����ͼʵ��"�ɿ��ź�"������ԭ��������ź��������Ӧ�ã����������Ķ�������ֻ������������һЩ�źţ�����һ��ʼ�Ͱ����Ƕ���Ϊ�ɿ��źţ���Щ�ź�֧���Ŷӣ����ᶪʧ��ͬʱ���źŵķ��ͺͰ�װҲ�������°汾���źŷ��ͺ���sigqueue()���źŰ�װ����sigaction()��POSIX.4�Կɿ��źŻ������˱�׼�������ǣ�POSIXֻ�Կɿ��źŻ���Ӧ���еĹ����Լ��źŻ��ƵĶ���ӿ����˱�׼�������źŻ��Ƶ�ʵ��û��������Ĺ涨��

�ź�ֵλ��SIGRTMIN��SIGRTMAX֮����źŶ��ǿɿ��źţ��ɿ��źſ˷����źſ��ܶ�ʧ�����⡣Linux��֧���°汾���źŰ�װ����sigation�����Լ��źŷ��ͺ���sigqueue()��ͬʱ����Ȼ֧�����ڵ�signal�����źŰ�װ������֧���źŷ��ͺ���kill()��

ע����Ҫ����������⣺��sigqueue()���͡�sigaction��װ���źž��ǿɿ��ġ���ʵ�ϣ��ɿ��ź���ָ������ӵ����źţ��ź�ֵλ��SIGRTMIN��SIGRTMAX֮�䣩�����ɿ��ź����ź�ֵС��SIGRTMIN���źš��źŵĿɿ��벻�ɿ�ֻ���ź�ֵ�йأ����źŵķ��ͼ���װ�����޹ء�Ŀǰlinux�е�signal()��ͨ��sigation()����ʵ�ֵģ���ˣ���ʹͨ��signal������װ���źţ����źŴ������Ľ�βҲ�����ٵ���һ���źŰ�װ������ͬʱ����signal()��װ��ʵʱ�ź�֧���Ŷӣ�ͬ�����ᶪʧ��

����Ŀǰlinux�������źŰ�װ����:signal()��sigaction()��˵�����Ƕ����ܰ�SIGRTMIN��ǰ���źű�ɿɿ��źţ�����֧���Ŷӣ����п��ܶ�ʧ����Ȼ�ǲ��ɿ��źţ������Ҷ�SIGRTMIN�Ժ���źŶ�֧���Ŷӡ�����������������������ڣ�����sigaction��װ���źŶ��ܴ�����Ϣ���źŴ��������������ź���һ�㶼��������������signal��װ���ź�ȴ�������źŴ�����������Ϣ�������źŷ��ͺ�����˵Ҳ��һ���ġ�

2��ʵʱ�ź����ʵʱ�ź�
����Unixϵͳֻ������32���źţ�Ret hat7.2֧��64���źţ����0-63(SIGRTMIN=31��SIGRTMAX=63)���������ܽ�һ�����ӣ�����Ҫ�õ��ں˵�֧�֡�ǰ32���ź��Ѿ�����Ԥ����ֵ��ÿ���ź�����ȷ������;�����壬����ÿ���źŶ��и��Ե�ȱʡ�������簴���̵�CTRL ^Cʱ�������SIGINT�źţ��Ը��źŵ�Ĭ�Ϸ�Ӧ���ǽ�����ֹ����32���źű�ʾʵʱ�źţ���ͬ��ǰ������Ŀɿ��źš��Ᵽ֤�˷��͵Ķ��ʵʱ�źŶ������ա�ʵʱ�ź���POSIX��׼��һ���֣�������Ӧ�ý��̡�

��ʵʱ�źŶ���֧���Ŷӣ����ǲ��ɿ��źţ�ʵʱ�źŶ�֧���Ŷӣ����ǿɿ��źš�


--------------------------------------------------------------------------------
��ҳ��
�������̶��źŵ���Ӧ
���̿���ͨ�����ַ�ʽ����Ӧһ���źţ���1�������źţ������źŲ����κδ������У��������źŲ��ܺ��ԣ�SIGKILL��SIGSTOP����2����׽�źš������źŴ����������źŷ���ʱ��ִ����Ӧ�Ĵ���������3��ִ��ȱʡ������Linux��ÿ���źŶ��涨��Ĭ�ϲ�������ϸ�����ο�[2]�Լ��������ϡ�ע�⣬���̶�ʵʱ�źŵ�ȱʡ��Ӧ�ǽ�����ֹ��

Linux���������������ַ�ʽ����һ������Ӧ�źţ�ȡ���ڴ��ݸ���ӦAPI�����Ĳ�����


--------------------------------------------------------------------------------
��ҳ��
�ġ��źŵķ���
�����źŵ���Ҫ�����У�kill()��raise()�� sigqueue()��alarm()��setitimer()�Լ�abort()��

1��kill() 
#include <sys/types.h> 
#include <signal.h> 
int kill(pid_t pid,int signo) 

����pid��ֵ �źŵĽ��ս��� 
pid>0 ����IDΪpid�Ľ��� 
pid=0 ͬһ��������Ľ��� 
pid<0 pid!=-1 ������IDΪ -pid�����н��� 
pid=-1 �����ͽ��������⣬���н���ID����1�Ľ��� 

Sinno���ź�ֵ����Ϊ0ʱ�������źţ���ʵ�ʲ������κ��źţ����ճ����д����飬��ˣ������ڼ��Ŀ������Ƿ���ڣ��Լ���ǰ�����Ƿ������Ŀ�귢���źŵ�Ȩ�ޣ�rootȨ�޵Ľ��̿������κν��̷����źţ���rootȨ�޵Ľ���ֻ��������ͬһ��session����ͬһ���û��Ľ��̷����źţ���

Kill()�����pid>0ʱ���źŷ��ͣ����óɹ����� 0�� ���򣬷��� -1��ע������pid<0ʱ�������������Щ���̽������źţ����ְ汾˵����һ����ʵ�ܼ򵥣������ں�Դ��kernal/signal.c���ɣ��ϱ��еĹ����ǲο�red hat 7.2��

2��raise���� 
#include <signal.h> 
int raise(int signo) 
����̱������źţ�����Ϊ�������͵��ź�ֵ�����óɹ����� 0�����򣬷��� -1�� 

3��sigqueue���� 
#include <sys/types.h> 
#include <signal.h> 
int sigqueue(pid_t pid, int sig, const union sigval val) 
���óɹ����� 0�����򣬷��� -1�� 

sigqueue()�ǱȽ��µķ����ź�ϵͳ���ã���Ҫ�����ʵʱ�ź�����ģ���ȻҲ֧��ǰ32�֣���֧���źŴ��в������뺯��sigaction()���ʹ�á�

sigqueue�ĵ�һ��������ָ�������źŵĽ���ID���ڶ�������ȷ���������͵��źţ�������������һ���������ݽṹunion sigval��ָ�����źŴ��ݵĲ�������ͨ����˵��4�ֽ�ֵ��

 	typedef union sigval {
 		int  sival_int;
 		void *sival_ptr;
 	}sigval_t;sigqueue()��kill()�����˸���ĸ�����Ϣ����sigqueue()ֻ����һ�����̷����źţ������ܷ����źŸ�һ�������顣���signo=0������ִ�д����飬��ʵ���ϲ������κ��źţ�0ֵ�źſ����ڼ��pid����Ч���Լ���ǰ�����Ƿ���Ȩ����Ŀ����̷����źš�

�ڵ���sigqueueʱ��sigval_tָ������Ϣ�´����3�����źŴ�������3�����źŴ�����ָ�����źŴ�������sigaction��װ�����趨��sa_sigactionָ�룬�Ժ󽫲�������siginfo_t�ṹ�У������źŴ������Ϳ��Դ�����Щ��Ϣ�ˡ�����sigqueueϵͳ����֧�ַ��ʹ������źţ����Ա�kill()ϵͳ���õĹ���Ҫ����ǿ��öࡣ

ע��sigqueue�������ͷ�ʵʱ�ź�ʱ��������������������Ϣ��Ȼ�ܹ����ݸ��źŴ������� sigqueue�������ͷ�ʵʱ�ź�ʱ����Ȼ��֧���Ŷӣ������źŴ�����ִ�й����е�����������ͬ�źţ������ϲ�Ϊһ���źš�

4��alarm���� 
#include <unistd.h> 
unsigned int alarm(unsigned int seconds) 
ר��ΪSIGALRM�źŶ��裬��ָ����ʱ��seconds��󣬽�����̱�����SIGALRM�źţ��ֳ�Ϊ����ʱ�䡣���̵���alarm���κ���ǰ��alarm()���ö�����Ч���������secondsΪ�㣬��ô�����ڽ����ٰ����κ�����ʱ�䡣 
����ֵ���������alarm����ǰ���������Ѿ�����������ʱ�䣬�򷵻���һ������ʱ���ʣ��ʱ�䣬���򷵻�0�� 

5��setitimer���� 
#include <sys/time.h> 
int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)); 
setitimer()��alarm����ǿ��֧��3�����͵Ķ�ʱ���� 

ITIMER_REAL�� �趨����ʱ�䣻����ָ����ʱ����ں˽�����SIGALRM�źŸ������̣�
ITIMER_VIRTUAL �趨����ִ��ʱ�䣻����ָ����ʱ����ں˽�����SIGVTALRM�źŸ������̣�
ITIMER_PROF �趨����ִ���Լ��ں��򱾽��̶����ĵ�ʱ��ͣ�����ָ����ʱ����ں˽�����ITIMER_VIRTUAL�źŸ������̣�
Setitimer()��һ������whichָ����ʱ�����ͣ���������֮һ�����ڶ��������ǽṹitimerval��һ��ʵ�����ṹitimerval��ʽ����¼1�������������ɲ�������

Setitimer()���óɹ�����0�����򷵻�-1��

6��abort() 
#include <stdlib.h> 
void abort(void); 

����̷���SIGABORT�źţ�Ĭ������½��̻��쳣�˳�����Ȼ�ɶ����Լ����źŴ���������ʹSIGABORT����������Ϊ�����źţ�����abort()��SIGABORT��Ȼ�ܱ����̽��ա��ú����޷���ֵ��


--------------------------------------------------------------------------------
��ҳ��
�塢�źŵİ�װ�������źŹ���������
�������Ҫ����ĳһ�źţ���ô��Ҫ�ڽ����а�װ���źš���װ�ź���Ҫ����ȷ���ź�ֵ��������Ը��ź�ֵ�Ķ���֮���ӳ���ϵ�������̽�Ҫ�����ĸ��źţ����źű����ݸ�����ʱ����ִ�к��ֲ�����

linux��Ҫ����������ʵ���źŵİ�װ��signal()��sigaction()������signal()�ڿɿ��ź�ϵͳ���õĻ�����ʵ��, �ǿ⺯������ֻ��������������֧���źŴ�����Ϣ����Ҫ������ǰ32�ַ�ʵʱ�źŵİ�װ����sigaction()�ǽ��µĺ�����������ϵͳ����ʵ�֣�sys_signal�Լ�sys_rt_sigaction����������������֧���źŴ�����Ϣ����Ҫ������ sigqueue() ϵͳ�������ʹ�ã���Ȼ��sigaction()ͬ��֧�ַ�ʵʱ�źŵİ�װ��sigaction()����signal()��Ҫ������֧���źŴ��в�����

1��signal() 
#include <signal.h> 
void (*signal(int signum, void (*handler))(int)))(int); 
����ú���ԭ�Ͳ��������Ļ������Բο�����ķֽⷽʽ����⣺ 
typedef void (*sighandler_t)(int)�� 
sighandler_t signal(int signum, sighandler_t handler)); 
��һ������ָ���źŵ�ֵ���ڶ�������ָ�����ǰ���ź�ֵ�Ĵ������Ժ��Ը��źţ�������ΪSIG_IGN�������Բ���ϵͳĬ�Ϸ�ʽ�����ź�(������ΪSIG_DFL)��Ҳ�����Լ�ʵ�ִ���ʽ(����ָ��һ��������ַ)�� 
���signal()���óɹ����������һ��Ϊ��װ�ź�signum������signal()ʱ��handlerֵ��ʧ���򷵻�SIG_ERR�� 

2��sigaction() 
#include <signal.h> 
int sigaction(int signum,const struct sigaction *act,struct sigaction *oldact)); 

sigaction�������ڸı���̽��յ��ض��źź����Ϊ���ú����ĵ�һ������Ϊ�źŵ�ֵ������Ϊ��SIGKILL��SIGSTOP����κ�һ���ض���Ч���źţ�Ϊ�������źŶ����Լ��Ĵ��������������źŰ�װ���󣩡��ڶ���������ָ��ṹsigaction��һ��ʵ����ָ�룬�ڽṹsigaction��ʵ���У�ָ���˶��ض��źŵĴ�������Ϊ�գ����̻���ȱʡ��ʽ���źŴ�������������oldactָ��Ķ�����������ԭ������Ӧ�źŵĴ�����ָ��oldactΪNULL������ѵڶ�����������������ΪNULL����ô�ú��������ڼ���źŵ���Ч�ԡ�

�ڶ���������Ϊ��Ҫ�����а����˶�ָ���źŵĴ����ź������ݵ���Ϣ���źŴ�����ִ�й�����Ӧ���ε���Щ�����ȵȡ�

sigaction�ṹ�������£�

 struct sigaction {
          union{
            __sighandler_t _sa_handler;
            void (*_sa_sigaction)(int,struct siginfo *, void *)��
            }_u
                     sigset_t sa_mask��
                    unsigned long sa_flags�� 
                  void (*sa_restorer)(void)��
                  }���У�sa_restorer���ѹ�ʱ��POSIX��֧��������Ӧ�ٱ�ʹ�á�

1���������ݽṹ�е�����Ԫ��_sa_handler�Լ�*_sa_sigactionָ���źŹ������������û�ָ�����źŴ����������˿������û��Զ���Ĵ������⣬������ΪSIG_DFL(����ȱʡ�Ĵ���ʽ)��Ҳ����ΪSIG_IGN�������źţ���

2����_sa_handlerָ���Ĵ�����ֻ��һ�����������ź�ֵ�������źŲ��ܴ��ݳ��ź�ֵ֮����κ���Ϣ����_sa_sigaction��ָ�����źŴ���������������������Ϊʵʱ�źŶ���ģ���Ȼͬ��֧�ַ�ʵʱ�źţ�����ָ��һ��3�����źŴ���������һ������Ϊ�ź�ֵ������������û��ʹ�ã�posixû�й淶ʹ�øò����ı�׼�����ڶ���������ָ��siginfo_t�ṹ��ָ�룬�ṹ�а����ź�Я��������ֵ��������ָ��Ľṹ���£�

 siginfo_t {
                  int      si_signo;   �ź�ֵ���������ź�������
                  int      si_errno;   errnoֵ���������ź�������
                  int      si_code;    �źŲ�����ԭ�򣬶�������������
        union{          �������ݽṹ����ͬ��Ա��Ӧ��ͬ�ź�
          //ȷ�������㹻��Ĵ洢�ռ�
          int _pad[SI_PAD_SIZE];
          //��SIGKILL������Ľṹ
          struct{
              ...
              }...
            ... ...
            ... ...          
          //��SIGILL, SIGFPE, SIGSEGV, SIGBUS������Ľṹ
              struct{
              ...
              }...
            ... ...
            }
      }ע��Ϊ�˸������Ķ�����˵������ʱ���Ѹýṹ��ʾΪ��¼2����ʾ����ʽ��

siginfo_t�ṹ�е��������ݳ�Աȷ���ýṹ��Ӧ���е��źţ��������ʵʱ�ź���˵����ʵ�ʲ�������Ľṹ��ʽ��

	typedef struct {
		int si_signo;
		int si_errno;			
		int si_code;			
		union sigval si_value;	
		} siginfo_t;�ṹ�ĵ��ĸ���ͬ��Ϊһ���������ݽṹ��

	union sigval {
		int sival_int;		
		void *sival_ptr;	
		}�����������ݽṹ��˵��siginfo_t�ṹ�е�si_valueҪô����һ��4�ֽڵ�����ֵ��Ҫô����һ��ָ�룬��͹��������ź���ص����ݡ����źŵĴ������У������������ź��������ָ�룬��û�й涨������ζ���Щ���ݽ��в�������������Ӧ���ɳ��򿪷���Ա���ݾ�����������Լ����

ǰ��������ϵͳ����sigqueue�����ź�ʱ��sigqueue�ĵ�������������sigval�������ݽṹ��������sigqueueʱ�������ݽṹ�е����ݾͽ��������źŴ������ĵڶ��������С��������ڷ����ź�ͬʱ���Ϳ������źŴ���һЩ������Ϣ���źſ��Դ�����Ϣ�Գ��򿪷��Ƿǳ�������ġ�

�źŲ����Ĵ��ݹ��̿�ͼʾ���£�


3��sa_maskָ�����źŴ������ִ�й����У���Щ�ź�Ӧ����������ȱʡ����µ�ǰ�źű�����������ֹ�źŵ�Ƕ�׷��ͣ�����ָ��SA_NODEFER����SA_NOMASK��־λ��

ע����ע��sa_maskָ�����ź�������ǰ��������������sigaction������װ�źŵĴ�����ִ�й�������sa_maskָ�����źŲű�������

4��sa_flags�а���������־λ�������ո��ᵽ��SA_NODEFER��SA_NOMASK��־λ����һ���Ƚ���Ҫ�ı�־λ��SA_SIGINFO�����趨�˸ñ�־λʱ����ʾ�źŸ����Ĳ������Ա����ݵ��źŴ������У���ˣ�Ӧ��Ϊsigaction�ṹ�е�sa_sigactionָ��������������Ӧ��Ϊsa_handlerָ���źŴ��������������øñ�־��ú������塣��ʹΪsa_sigactionָ�����źŴ����������������SA_SIGINFO���źŴ�����ͬ�����ܵõ��źŴ��ݹ��������ݣ����źŴ������ж���Щ��Ϣ�ķ��ʶ������¶δ���Segmentation fault����

ע���ܶ������ڲ����ñ�־λʱ����Ϊ����������˸ñ�־λ���ͱ��붨���������źŴ�������ʵ�ʲ��������ģ���֤�����ܼ򵥣��Լ�ʵ��һ����һ�����źŴ����������ڳ��������øñ�־λ�����Բ쿴��������н����ʵ���ϣ����԰Ѹñ�־λ�����ź��Ƿ񴫵ݲ����Ŀ��أ�������ø�λ���򴫵ݲ��������򣬲����ݲ�����


--------------------------------------------------------------------------------
��ҳ��
�����źż����źż�����������
�źż�������Ϊһ���������ͣ�

	typedef struct {
			unsigned long sig[_NSIG_WORDS]��
			} sigset_t�źż����������źŵļ��ϣ�linux��֧�ֵ������źſ���ȫ���򲿷ֵĳ������źż��У���Ҫ���ź�������غ������ʹ�á�������Ϊ�źż������������غ�����

	#include <signal.h>
int sigemptyset(sigset_t *set)��
int sigfillset(sigset_t *set)��
int sigaddset(sigset_t *set, int signum)
int sigdelset(sigset_t *set, int signum)��
int sigismember(const sigset_t *set, int signum)��
sigemptyset(sigset_t *set)��ʼ����setָ�����źż����źż�����������źű���գ�
sigfillset(sigset_t *set)���øú�����setָ����źż��н�����linux֧�ֵ�64���źţ�
sigaddset(sigset_t *set, int signum)��setָ����źż��м���signum�źţ�
sigdelset(sigset_t *set, int signum)��setָ����źż���ɾ��signum�źţ�
sigismember(const sigset_t *set, int signum)�ж��ź�signum�Ƿ���setָ����źż��С�
--------------------------------------------------------------------------------
��ҳ��
�ߡ��ź��������ź�δ��:
ÿ�����̶���һ������������Щ�źŵ��͵�����ʱ�����������źż������źż��е������ź��ڵ��͵����̺󶼽������������������ź�������صļ���������

#include <signal.h>
int  sigprocmask(int  how,  const  sigset_t *set, sigset_t *oldset))��
int sigpending(sigset_t *set));
int sigsuspend(const sigset_t *mask))��sigprocmask()�����ܹ����ݲ���how��ʵ�ֶ��źż��Ĳ�����������Ҫ�����֣�

����how ���̵�ǰ�źż� 
SIG_BLOCK �ڽ��̵�ǰ�����źż������setָ���źż��е��ź� 
SIG_UNBLOCK ������������źż��а���setָ���źż��е��źţ������Ը��źŵ����� 
SIG_SETMASK ���½��������źż�Ϊsetָ����źż� 

sigpending(sigset_t *set))��õ�ǰ�ѵ��͵����̣�ȴ�������������źţ���setָ����źż��з��ؽ����

sigsuspend(const sigset_t *mask))�����ڽ��յ�ĳ���ź�֮ǰ, ��ʱ��mask�滻���̵��ź�����, ����ͣ����ִ�У�ֱ���յ��ź�Ϊֹ��sigsuspend ���غ󽫻ָ�����֮ǰ���ź����롣�źŴ�������ɺ󣬽��̽�����ִ�С���ϵͳ����ʼ�շ���-1������errno����ΪEINTR��

��¼1���ṹitimerval��

            struct itimerval {
                struct timeval it_interval;  next value 
                struct timeval it_value;     current value 
            };
            struct timeval {
                long tv_sec;                 seconds 
                long tv_usec;                microseconds 
            };��¼2���������źŴ������еڶ���������˵����������

siginfo_t {
int      si_signo;   �ź�ֵ���������ź�������
int      si_errno;   errnoֵ���������ź�������
int      si_code;    �źŲ�����ԭ�򣬶������ź�������
pid_t    si_pid;     �����źŵĽ���ID,��kill(2),ʵʱ�ź��Լ�SIGCHLD������ 
uid_t    si_uid;    �����źŽ��̵���ʵ�û�ID����kill(2),ʵʱ�ź��Լ�SIGCHLD������ 
int      si_status;  �˳�״̬����SIGCHLD������
clock_t  si_utime;   �û����ĵ�ʱ�䣬��SIGCHLD������ 
clock_t  si_stime;   �ں����ĵ�ʱ�䣬��SIGCHLD������ 
sigval_t si_value;   �ź�ֵ��������ʵʱ�����壬��һ���������ݽṹ��
                          ����Ϊһ����������si_int��ʾ��Ҳ����Ϊһ��ָ�룬��si_ptr��ʾ��
	
void *   si_addr;    ����fault���ڴ��ַ����SIGILL,SIGFPE,SIGSEGV,SIGBUS �ź�������
int      si_band;   SIGPOLL�ź������� 
int      si_fd;     SIGPOLL�ź������� 
}ʵ���ϣ�����ǰ����Ԫ���⣬����Ԫ����֯��һ�����Ͻṹ�У����������ݽṹ�У��ָ��ݲ�ͬ���ź���֯�ɲ�ͬ�Ľṹ��ע�����ᵽ�Ķ�ĳ���ź�������ָ���ǣ��ڸ��źŵĴ������п��Է�����Щ����������ź���ص����������Ϣ��ֻ�����ض��ź�ֻ���ض���Ϣ����Ȥ���ѡ�

*/ //�źŴ�����ngx_signal_handler
ngx_int_t
ngx_init_signals(ngx_log_t *log)
{
    ngx_signal_t      *sig;
    struct sigaction   sa;

    for (sig = signals; sig->signo != 0; sig++) {
        ngx_memzero(&sa, sizeof(struct sigaction));
        sa.sa_handler = sig->handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
#if (NGX_VALGRIND)
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          "sigaction(%s) failed, ignored", sig->signame);
#else
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                          "sigaction(%s) failed", sig->signame);
            return NGX_ERROR;
#endif
        }
    }

    return NGX_OK;
}

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
//ע���º���ngx_init_signals
void
ngx_signal_handler(int signo)
{
    char            *action;
    ngx_int_t        ignore;
    ngx_err_t        err;
    ngx_signal_t    *sig;

    ignore = 0;

    err = ngx_errno;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    ngx_time_sigsafe_update();

    action = "";

    switch (ngx_process) {

    case NGX_PROCESS_MASTER:
    case NGX_PROCESS_SINGLE:
        switch (signo) {
        //�����յ�QUIT�ź�ʱ��ngx_quit��־λ����Ϊ1�������ڸ���worker������Ҫ���ŵعرս���
        case ngx_signal_value(NGX_SHUTDOWN_SIGNAL):
            ngx_quit = 1;
            action = ", shutting down";
            break;

        //�����յ�TERM�ź�ʱ��ngx_terminate��־λ����Ϊ1�������ڸ���worker������Ҫǿ�ƹرս���
        case ngx_signal_value(NGX_TERMINATE_SIGNAL):
        case SIGINT:
            ngx_terminate = 1;
            action = ", exiting";
            break;

        case ngx_signal_value(NGX_NOACCEPT_SIGNAL):
            if (ngx_daemonized) {
                ngx_noaccept = 1;
                action = ", stop accepting connections";
            }
            break;

        case ngx_signal_value(NGX_RECONFIGURE_SIGNAL): //reload�źţ�����master����
            ngx_reconfigure = 1;
            action = ", reconfiguring";
            break;

        //�����յ�USRI�ź�ʱ��ngx_reopen��־λ����Ϊ1�������ڸ���Nginx��Ҫ���´��ļ������л���־�ļ�ʱ��
        case ngx_signal_value(NGX_REOPEN_SIGNAL):
            ngx_reopen = 1;
            action = ", reopening logs";
            break;

        case ngx_signal_value(NGX_CHANGEBIN_SIGNAL):
            //if (getppid() > 1 || ngx_new_binary > 0) { 
            if (ngx_new_binary > 0) {  //yang add change��Ϊ�˵��ԣ�����ע��
           //nginx������ͨ�����͸��ź�,������뱣֤�����̴���1��������С�ڵ���1�Ļ���˵���Ѿ��ɾ�master�����˱�master����Ͳ���������
           //�������ͨ��crt��¼����nginx�Ļ������Կ�����PPID����1,���Բ���������

                /*
                 * Ignore the signal in the new binary if its parent is
                 * not the init process, i.e. the old binary's process
                 * is still running.  Or ignore the signal in the old binary's
                 * process if the new binary's process is already running.
                 */

                action = ", ignoring";
                ignore = 1;
                break;
            }

            ngx_change_binary = 1;
            action = ", changing binary";
            break;

        case SIGALRM: 
            ngx_sigalrm = 1; //�ӽ��̻��������ö�ʱ���źţ���ngx_timer_signal_handler
            break;

        case SIGIO:
            ngx_sigio = 1;
            break;

        case SIGCHLD: //�ӽ�����ֹ, ��ʱ���ں�ͬʱ�򸸽��̷��͸�sigchld�ź�.�ȴ�������waitpid���գ����⽩������
            ngx_reap = 1;
            break;
        }

        break;

    case NGX_PROCESS_WORKER:
    case NGX_PROCESS_HELPER:
        switch (signo) {

        case ngx_signal_value(NGX_NOACCEPT_SIGNAL):
            if (!ngx_daemonized) {
                break;
            }
            ngx_debug_quit = 1;
        case ngx_signal_value(NGX_SHUTDOWN_SIGNAL): //���������յ�quit�ź�
            ngx_quit = 1;
            action = ", shutting down";
            break;

        case ngx_signal_value(NGX_TERMINATE_SIGNAL):
        case SIGINT:
            ngx_terminate = 1;
            action = ", exiting";
            break;

        case ngx_signal_value(NGX_REOPEN_SIGNAL):
            ngx_reopen = 1;
            action = ", reopening logs";
            break;

        case ngx_signal_value(NGX_RECONFIGURE_SIGNAL):
        case ngx_signal_value(NGX_CHANGEBIN_SIGNAL):
        case SIGIO:
            action = ", ignoring";
            break;
        }

        break;
    }

    ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0,
                  "signal %d (%s) received%s", signo, sig->signame, action);

    if (ignore) {
        ngx_log_error(NGX_LOG_CRIT, ngx_cycle->log, 0,
                      "the changing binary signal is ignored: "
                      "you should shutdown or terminate "
                      "before either old or new binary's process");
    }

    if (signo == SIGCHLD) { //�����ӽ�����Դwaitpid
        ngx_process_get_status();
    }

    ngx_set_errno(err);
}


static void ngx_process_get_status(void)
{
    int              status;
    char            *process;
    ngx_pid_t        pid;
    ngx_err_t        err;
    ngx_int_t        i;
    ngx_uint_t       one;

    one = 0;

    for ( ;; ) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) {
            return;
        }

        if (pid == -1) {
            err = ngx_errno;

            if (err == NGX_EINTR) {
                continue;
            }

            if (err == NGX_ECHILD && one) {
                return;
            }

            /*
             * Solaris always calls the signal handler for each exited process
             * despite waitpid() may be already called for this process.
             *
             * When several processes exit at the same time FreeBSD may
             * erroneously call the signal handler for exited process
             * despite waitpid() may be already called for this process.
             */

            if (err == NGX_ECHILD) {
                ngx_log_error(NGX_LOG_INFO, ngx_cycle->log, err,
                              "waitpid() failed");
                return;
            }

            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, err,
                          "waitpid() failed");
            return;
        }


        one = 1;
        process = "unknown process";

        for (i = 0; i < ngx_last_process; i++) {
            if (ngx_processes[i].pid == pid) {
                ngx_processes[i].status = status;
                ngx_processes[i].exited = 1;
                process = ngx_processes[i].name;
                break;
            }
        }

        if (WTERMSIG(status)) {
#ifdef WCOREDUMP
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                          "%s %P exited on signal %d%s",
                          process, pid, WTERMSIG(status),
                          WCOREDUMP(status) ? " (core dumped)" : "");
#else
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                          "%s %P exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

        } else {
            ngx_log_error(NGX_LOG_NOTICE, ngx_cycle->log, 0,
                          "%s %P exited with code %d",
                          process, pid, WEXITSTATUS(status));
        }

        if (WEXITSTATUS(status) == 2 && ngx_processes[i].respawn) {
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                          "%s %P exited with fatal code %d "
                          "and cannot be respawned",
                          process, pid, WEXITSTATUS(status));
            ngx_processes[i].respawn = 0;
        }

        ngx_unlock_mutexes(pid);
    }
}


static void
ngx_unlock_mutexes(ngx_pid_t pid)
{
    ngx_uint_t        i;
    ngx_shm_zone_t   *shm_zone;
    ngx_list_part_t  *part;
    ngx_slab_pool_t  *sp;

    /*
     * unlock the accept mutex if the abnormally exited process
     * held it
     */

    if (ngx_accept_mutex_ptr) {
        (void) ngx_shmtx_force_unlock(&ngx_accept_mutex, pid);
    }

    /*
     * unlock shared memory mutexes if held by the abnormally exited
     * process
     */

    part = (ngx_list_part_t *) &ngx_cycle->shared_memory.part;
    shm_zone = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            shm_zone = part->elts;
            i = 0;
        }

        sp = (ngx_slab_pool_t *) shm_zone[i].shm.addr;

        if (ngx_shmtx_force_unlock(&sp->mutex, pid)) {
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0,
                          "shared memory zone \"%V\" was locked by %P",
                          &shm_zone[i].shm.name, pid);
        }
    }
}


void
ngx_debug_point(void)//���Լ�ֹͣ��֪ͨ������
{
    ngx_core_conf_t  *ccf;

    ccf = (ngx_core_conf_t *) ngx_get_conf(ngx_cycle->conf_ctx,
                                           ngx_core_module);

    switch (ccf->debug_points) {

    case NGX_DEBUG_POINTS_STOP:
        raise(SIGSTOP);
        break;

    case NGX_DEBUG_POINTS_ABORT:
        ngx_abort();
    }
}

/*
ngx_os_signal_process()��������
����signals���飬���ݸ����ź�name���ҵ���Ӧsigno��
����kill���pid����signo���źţ�
*/
ngx_int_t 
ngx_os_signal_process(ngx_cycle_t *cycle, char *name, ngx_int_t pid)
{
    ngx_signal_t  *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        if (ngx_strcmp(name, sig->name) == 0) {
            if (kill(pid, sig->signo) != -1) { //���﷢��signal���źŽ��մ�����signals->handler
                return 0;
            }

            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "kill(%P, %d) failed", pid, sig->signo);
        }
    }

    return 1;
}

