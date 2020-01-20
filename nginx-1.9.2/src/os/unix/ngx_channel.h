
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CHANNEL_H_INCLUDED_
#define _NGX_CHANNEL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>

//��װ�˸��ӽ���֮�䴫�ݵ���Ϣ��  
//�����̴������ӽ��̺�ͨ������Ѹ��ӽ��̵������Ϣȫ�����ݸ��������н��̣����������ӽ��̾�֪���ý��̵�channel��Ϣ�ˡ�
//��ΪNginx�������Ƶ��ͬ��master������worker���̼��״̬
//��ngx_start_worker_processes->ngx_pass_open_channel
typedef struct { 
     ngx_uint_t  command; //�Զ˽�Ҫ��������  ȡֵΪNGX_CMD_OPEN_CHANNEL��
     ngx_pid_t   pid;  //��ǰ���ӽ���id   ����ID��һ���Ƿ�������Ľ���ID
     ngx_int_t   slot; //��ȫ�ֽ��̱��е�λ��    ��ʾ���������ngx_processes�������������
     ngx_fd_t    fd; //���ݵ�fd   ͨ�ŵ��׽��־��
} ngx_channel_t;


ngx_int_t ngx_write_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_add_channel_event(ngx_cycle_t *cycle, ngx_fd_t fd,
    ngx_int_t event, ngx_event_handler_pt handler);
void ngx_close_channel(ngx_fd_t *fd, ngx_log_t *log);

#endif /* _NGX_CHANNEL_H_INCLUDED_ */

