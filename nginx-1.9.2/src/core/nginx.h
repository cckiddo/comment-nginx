
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGINX_H_INCLUDED_
#define _NGINX_H_INCLUDED_


#define nginx_version      1009002
#define NGINX_VERSION      "1.9.2"
#define NGINX_VER          "nginx/" NGINX_VERSION

#ifdef NGX_BUILD
#define NGINX_VER_BUILD    NGINX_VER " (" NGX_BUILD ")"
#else
#define NGINX_VER_BUILD    NGINX_VER
#endif

/*
��ִ�в�������������Nginx�Ĳ���ʱ���ϵ�Nginx���̻�ͨ������������NGINX����������Ҫ�򿪵ļ����˿ڣ�
�µ�Nginx���̻�ͨ��ngx_add_inherited_sockets������ʹ���Ѿ��򿪵�TCP�����˿�   
*/
#define NGINX_VAR          "NGINX" //��ngx_exec_new_binary ͨ���û����������浱ǰ��һЩ���������µ�nginx������ʱ�򣬾ʹӻ�������NGINX_VAR�л�ȡ����
#define NGX_OLDPID_EXT     ".oldbin" //������nginx��ִ���ļ���ʱ���޸�nginx.pidΪnginx.pid.oldbin


#endif /* _NGINX_H_INCLUDED_ */
