
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_CYCLE_H_INCLUDED_
#define _NGX_PROCESS_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

//��Ƶ����ʹ��Ƶ�����ַ�ʽͨ��ǰ���뷢�͵�����
#define NGX_CMD_OPEN_CHANNEL   1
//�ر��Ѿ��򿪵�Ƶ����ʵ����Ҳ���ǹر��׽���
#define NGX_CMD_CLOSE_CHANNEL  2
//Ҫ����շ��������˳�����
#define NGX_CMD_QUIT           3
//Ҫ����շ�ǿ�Ƶؽ�������
#define NGX_CMD_TERMINATE      4
//Ҫ����շ����´򿪽����Ѿ��򿪹����ļ�
#define NGX_CMD_REOPEN         5


#define NGX_PROCESS_SINGLE     0 //�����̷�ʽ  //������õ��ǵ����̹���ģʽ  
#define NGX_PROCESS_MASTER     1 //�������е�master+��worker����ģʽ�е��������Ǹ�ģʽ
#define NGX_PROCESS_SIGNALLER  2 //��nginx -s�����źŵĽ���
#define NGX_PROCESS_WORKER     3 //�������е�master+��worker����ģʽ�е�worker�ӽ����Ǹ�ģʽ
#define NGX_PROCESS_HELPER     4 //�����ڻ����ļ������cache_manager���ڸ�ģʽ

/*
static ngx_cache_manager_ctx_t  ngx_cache_manager_ctx = {
    ngx_cache_manager_process_handler, "cache manager process", 0
};
static ngx_cache_manager_ctx_t  ngx_cache_loader_ctx = {
    ngx_cache_loader_process_handler, "cache loader process", 60000  //���̴�����60000m��ִ��ngx_cache_loader_process_handler,��ngx_cache_manager_process_cycle����ӵĶ�ʱ��
};
*/
typedef struct { //ngx_cache_manager_process_cycle
    ngx_event_handler_pt       handler; //ngx_cache_manager_process_handler  ngx_cache_loader_process_handler
    char                      *name; //������
    ngx_msec_t                 delay; //�ӳٶ೤ʱ��ִ�������handler��ͨ����ʱ��ʵ�֣���ngx_cache_manager_process_cycle
} ngx_cache_manager_ctx_t;


void ngx_master_process_cycle(ngx_cycle_t *cycle);
void ngx_single_process_cycle(ngx_cycle_t *cycle);


extern ngx_uint_t      ngx_process;
extern ngx_uint_t      ngx_worker;
extern ngx_pid_t       ngx_pid;
extern ngx_pid_t       ngx_new_binary;
extern ngx_uint_t      ngx_inherited;
extern ngx_uint_t      ngx_daemonized;
extern ngx_uint_t      ngx_exiting;

extern sig_atomic_t    ngx_reap;
extern sig_atomic_t    ngx_sigio;
extern sig_atomic_t    ngx_sigalrm;
extern sig_atomic_t    ngx_quit;
extern sig_atomic_t    ngx_debug_quit;
extern sig_atomic_t    ngx_terminate;
extern sig_atomic_t    ngx_noaccept;
extern sig_atomic_t    ngx_reconfigure;
extern sig_atomic_t    ngx_reopen;
extern sig_atomic_t    ngx_change_binary;


#endif /* _NGX_PROCESS_CYCLE_H_INCLUDED_ */
