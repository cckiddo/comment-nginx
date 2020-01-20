
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CONFIG_H_INCLUDED_
#define _NGX_HTTP_CONFIG_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/*
Nginx�����ļ����

#�����û� 
user nobody nobody; 
#�������� 
worker_processes 2; 
#ȫ�ִ�����־��PID�ļ� 
error_log logs/error.log notice; 
pid logs/nginx.pid; 
#����ģʽ������������ 
events { 
use epoll; 
worker_connections 1024; 
} 
#�趨http���������������ķ���������ṩ���ؾ���֧�� 
http { 
#�趨mime���� 
include conf/mime.types; 
default_type application/octet-stream; 
#�趨��־��ʽ 
log_format main ��$remote_addr �C $remote_user [$time_local] �� 
����$request�� $status $bytes_sent �� 
����$http_referer�� ��$http_user_agent�� �� 
����$gzip_ratio����; 
log_format download ��$remote_addr �C $remote_user [$time_local] �� 
����$request�� $status $bytes_sent �� 
����$http_referer�� ��$http_user_agent�� �� 
����$http_range�� ��$sent_http_content_range����; 
#�趨���󻺳� 
client_header_buffer_size 1k; 
large_client_header_buffers 4 4k;

#����gzipģ�� 
gzip on; 
gzip_min_length 1100; 
gzip_buffers 4 8k; 
gzip_types text/plain; 
output_buffers 1 32k; 
postpone_output 1460; 
#�趨access log 
access_log logs/access.log main; 
client_header_timeout 3m; 
client_body_timeout 3m; 
send_timeout 3m; 
sendfile on; 
tcp_nopush on; 
tcp_nodelay on; 
keepalive_timeout 65; 
#�趨���ؾ���ķ������б� 
upstream mysvr { 
#weigth������ʾȨֵ��ȨֵԽ�߱����䵽�ļ���Խ�� 
#�����ϵ�Squid����3128�˿� 
server 192.168.8.1:3128 weight=5; 
server 192.168.8.2:80 weight=1; 
server 192.168.8.3:80 weight=6; 
} 

#�趨�������� 
server { 
listen 80; 
server_name 192.168.8.1 www.hahaer.com; 
charset gb2312; 
#�趨�����������ķ�����־ 
access_log logs/www.hahaer.com.access.log main; 
#������� /img/ *, /js/ *, /css/ * ��Դ����ֱ��ȡ�����ļ�����ͨ��squid 
#�����Щ�ļ��϶࣬���Ƽ����ַ�ʽ����Ϊͨ��squid�Ļ���Ч������ 
location ~ ^/(img|js|css)/ { 
root /data3/Html; 
expires 24h; 
}

#�� ��/�� ���ø��ؾ��� 
location / { 
proxy_pass http://mysvr; 
proxy_redirect off; 
proxy_set_header Host $host; 
proxy_set_header X-Real-IP $remote_addr; 
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for; 
client_max_body_size 10m; 
client_body_buffer_size 128k; 
proxy_connect_timeout 90; 
proxy_send_timeout 90; 
proxy_read_timeout 90; 
proxy_buffer_size 4k; 
proxy_buffers 4 32k; 
proxy_busy_buffers_size 64k; 
proxy_temp_file_write_size 64k; 
}

#�趨�鿴Nginx״̬�ĵ�ַ 
location /NginxStatus { 
stub_status on; 
access_log on; 
auth_basic ��NginxStatus��; 
auth_basic_user_file conf/htpasswd; 
} 
} 
} 

��ע��conf/htpasswd �ļ��������� apache �ṩ�� htpasswd �������������ɡ�

3.) �鿴 Nginx ����״̬

�����ַ http://192.168.8.1/NginxStatus/��������֤�ʺ����룬���ɿ��������������ݣ�

 

Active connections: 328 
server accepts handled requests 
9309 8982 28890 
Reading: 1 Writing: 3 Waiting: 324 
*/

/*
��ʱ��HTTP��ܻ�Ϊ���е�HTTPģ�齨��3�����飬�ֱ�������HTTPģ���ngx_http_module_t�е�
create_main_conf��create_srv_conf��create_loc_conf�������صĵ�ַָ�루�����µ�����
��mytestģ����create_loc_conf��������ngx_http_mytest_conf_t�ṹ������create_lo_conf��������ʱ��ָ�봫�ݸ�HTTP��ܣ���
��Ȼ�����HTTPģ��������������Ȥ��
��û��ʵ��create_main_conf��create_srv_conf��create_loc_conf�ȷ�������ô��������Ӧλ
�ô洢��ָ����NULL��ngx_http_conf_ctx_t��3����Աmain_conf��srv_conf��loc_conf��
��ָ����3�����顣���濴һ�μ򻯵Ĵ��룬�˽��������create_loc_conf���صĵ�ַ��
ngx_http_conf_ctx_t *ctx;
//HTTP���������1��ngx_ht tp_conf_ctxt�ṹ
ctx=ngx_pcalloc (cf->pool,  sizeof (ngx_http_conf_ctx_t))j
if (ctx==NULL){
    return NGX_CONF_ERROR;
)
��������1������洢���е�HTTPģ��create_loc_conf�������صĵ�ַ
ctx->loc_conf=ngx_pcalloc (cf->pool,  sizeof (void*)  * ngx_http_max_module);
if (ctx->loc conf==NULL)  {
    return NGX_CONF_ERROR;
)
*/
/*��Nginx��⵽http{������������ؼ�������ʱ��HTTP����ģ�;������ˣ���ʱ�����Ƚ���1��ngx_http_conf_ ctx_t�ṹ��
��http{�����������о�ͨ��1��ngx_http_conf_ctx t�ṹ����������HTTPģ�����
�����ݽṹ����ڡ��Ժ������κ�server{�������������location{����������ʱ��Ҳ�Ὠ��ngx_http_
conf_ctx_t�ṹ������ͬ������������������HTTPģ��ͨ��create_srv_ conf  create_loc_
conf�ȷ������ص�ָ���ַ��ngx_http_conf_ctx_t���˽�http���ÿ�Ļ���
*/ //�ýṹ�еı���ֱ��ָ��ngx_http_module_t�е�����create_main_conf  create_srv_conf  create_loc_conf
/*
  http {
       xxxx
       server {
            location /xxx {
            }
       }
  }
  ��������������ļ�����ִ�е�http��ʱ�򿪱�ngx_http_conf_ctx_t��ֱ����һ��main srv loc_creat��ִ�е�serverʱ����ngx_http_conf_ctx_t�����srv_creat loc_creat, ִ�е�locationʱ����ngx_http_conf_ctx_t�����һ��loc_creat
  ����������������1��main_creat 2��srv_creat 3��loc_creat��

  http {
       xxxx
       server {
            location /xxx {
            }
       }

       server {
            location /yyy {
            }
       }
  }
  ��������������ļ�����ִ�е�http��ʱ�򿪱�ngx_http_conf_ctx_t��ֱ����һ��main srv loc_creat��ִ�е�serverʱ����ngx_http_conf_ctx_t�����srv_creat loc_creat, ִ�е�locationʱ����ngx_http_conf_ctx_t�����һ��loc_creat
  ����������������1��main_creat 1+2��srv_creat 1+2+2��loc_creat��
*/

/*
http{}�л����main_conf srv_conf loc_conf����ռ䣬��ngx_http_block��server{}�л����srv_conf loc_conf��
���ռ�,��ngx_http_core_server�� location{}�лᴴ��loc_conf�ռ�,��ngx_http_core_location
ͼ�λ��ο�:�������NGINX�е�ͼ9-2(P302)  ͼ10-1(P353) ͼ10-1(P356) ͼ10-1(P359)  ͼ4-2(P145)

ngx_http_conf_ctx_t��ngx_http_core_main_conf_t��ngx_http_core_srv_conf_t��ngx_http_core_loc_conf_s��ngx_cycle_s->conf_ctx�Ĺ�ϵ��:
Nginx��http���ýṹ�����֯�ṹ:http://tech.uc.cn/?p=300
*/ 
typedef struct { //��ؿռ䴴���͸�ֵ��ngx_http_block, �ýṹ��ngx_conf_t->ctx��Ա�����е����������ڴ��Դͷ��ngx_cycle_t->conf_ctx,��ngx_init_cycle
/* ָ��һ��ָ�����飬�����е�ÿ����Ա����������HTTPģ���create_main_conf���������Ĵ��ȫ��������Ľṹ�壬���Ǵ���Ž���ֱ��http{}���ڵ�main�������������� */
    void        **main_conf;  /* ָ�����飬�����е�ÿ��Ԫ��ָ������HTTPģ��ngx_http_module_t->create_main_conf���������Ľṹ�� */

/* ָ��һ��ָ�����飬�����е�ÿ����Ա����������HTTPģ���create_srv_conf������������server��صĽṹ�壬���ǻ���main���������
����srv�����server��������뵱ǰ��ngx_http_conf_ctx_t���ڽ���http{}����server{}��ʱ�������й� */
    void        **srv_conf;/* ָ�����飬�����е�ÿ��Ԫ��ָ������HTTPģ��ngx_http_module_t->create->srv->conf���������Ľṹ�� */

/*
ָ��һ��ָ�����飬�����е�ÿ����Ա����������HTTPģ���create_loc_conf������������location��صĽṹ�壬���ǿ��ܴ����main��srv��loc��
�����������뵱ǰ��ngx_http_conf_ctx_t���ڽ���http{}��server{}����location{}��ʱ�������йش��location{}������
*/
    void        **loc_conf;/* ָ�����飬�����е�ÿ��Ԫ��ָ������HTTPģ��ngx_http_module_t->create->loc->conf���������Ľṹ�� */
} ngx_http_conf_ctx_t; //ctx��content�ļ�ƣ���ʾ������  
//�ο�:http://tech.uc.cn/?p=300   ngx_http_conf_ctx_t������ָ��ctx�洢��ngx_cycle_t��conf_ctx��ָ���ָ�����飬��ngx_http_module��indexΪ�±������Ԫ��
//http://tech.uc.cn/?p=300��������������ݽṹ�ο�

/*
Nginx��װ��Ϻ󣬻�����Ӧ�İ�װĿ¼����װĿ¼��nginx.confΪnginx���������ļ���ginx�������ļ���Ϊ4���֣�main��ȫ�����ã���server���������ã���upstream�����ؾ���������裩��location��URLƥ���ض�λ�õ����ã��������߹�ϵΪ��server�̳�main��location�̳�server��upstream�Ȳ���̳���������Ҳ���ᱻ�̳С�

һ��Nginx��main��ȫ�����ã��ļ�

[root@rhel6u3-7 server]# vim /usr/local/nginx/conf/nginx.conf 
user nginx nginx; //ָ��nginx���е��û����û���Ϊnginx��Ĭ��Ϊnobody 
worker_processes 2�� //�����Ľ�������һ����߼�cpu����һ�� 
error_log logs/error.log notice; //����ȫ�ִ�����־�ļ���������notice��ʾ������debug��info��warn��error��critģʽ��debug�����࣬crit������٣�����ʵ�ʻ��������� 
pid logs/nginx.pid; //ָ������id�Ĵ洢�ļ�λ�� 
worker_rlimit_nofile 65535; //ָ��һ��nginx���̴򿪵�����ļ���������Ŀ����ϵͳ���̵������ļ��������� 
events { 
use epoll; ���ù���ģʽΪepoll������֮�⻹��select��poll��kqueue��rtsig��/dev/pollģʽ 
worker_connections 65535; //����ÿ�����̵���������� ��ϵͳ���̵������ļ��������� 
} 
����.

[root@rhel6u3-7 server]# cat /proc/cpuinfo | grep "processor" | wc �Cl //�鿴�߼�CPU���� 
[root@rhel6u3-7 server]# ulimit -n 65535 //����ϵͳ���̵������ļ�����

����Nginx��HTTP���������ã�Gzip���á�

http { 
*****************************������http������ȫ������********************************* 
include mime.types; //��ģ��ָ�ʵ�ֶ������ļ����������ļ����趨�����Լ����������ļ��ĸ��Ӷȣ�DNS�������ļ��е�zonerfc1912��acl�����϶����õ�include��� 
default_type application/octet-stream; //����ģ��ָ�����Ĭ������Ϊ����������Ҳ���ǵ��ļ�����δ����ʱʹ�����ַ�ʽ 
//�������Ϊ��־��ʽ���趨��mainΪ��־��ʽ�����ƣ����������ã��������á� 
log_format main '$remote_addr - $remote_user [$time_local] "$request" ' 
'$status $body_bytes_sent "$http_referer" ' 
'"$http_user_agent" "$http_x_forwarded_for"'; 
access_log logs/access.log main; //������־main 
client_max_body_size 20m; //��������ͻ�����������ĵ����ļ��ֽ��� 
client_header_buffer_size 32k; //ָ�����Կͻ�������ͷ��headebuffer��С 
client_body_temp_path /dev/shm/client_body_temp; //ָ������������ͼд�뻺���ļ���Ŀ¼·�� 
large_client_header_buffers 4 32k; //ָ���ͻ��������нϴ����Ϣͷ�Ļ�����������ʹ�С��Ŀǰ����Ϊ4��32KB 
sendfile on; //������Ч�ļ�����ģʽ 
tcp_nopush on; //������ֹ�������� 
tcp_nodelay on; //������ֹ�������� 
keepalive_timeout 65; //���ÿͻ������ӱ����ĳ�ʱʱ�� 
client_header_timeout 10; //�������ÿͻ��������ȡ��ʱʱ�� 
client_body_timeout 10; //�������ÿͻ������������ȡ��ʱʱ�� 
send_timeout 10; //����������Ӧ�ͻ��˵ĳ�ʱʱ�� 
//������httpGzipģ������ 
#httpGzip modules 
gzip on; //����gzipѹ�� 
gzip_min_length 1k; //��������ѹ����ҳ����С�ֽ��� 
gzip_buffers 4 16k; //����4����λΪ16K���ڴ���Ϊѹ����������� 
gzip_http_version 1.1; //����ʶ��httpЭ��İ汾,Ĭ����1.1 
gzip_comp_level 2; //ָ��gzipѹ����,1-9 ����ԽС,ѹ����ԽС,�ٶ�Խ��. 
gzip_types text/plain application/x-javascript text/css application/xml; //ָ��ѹ�������� 
gzip_vary on; //��ǰ�˵Ļ���������澭��gzipѹ����ҳ��

����nginx��server������������

���ַ�ʽһ����ֱ�����������ļ�������server�ֶ�������������������һ����ʹ��include�ֶ����������������������Լ����������ļ��ĸ����ԡ�

*****************************������server��������********************************* 
server { 
listen 80; //�����˿�Ϊ80 
server_name www.88181.com; //������������ 
charset gb2312; //���÷��ʵ����Ա��� 
access_log logs/www.88181.com.access.log main; //������������������־�Ĵ��·������־�ĸ�ʽΪmain 
location / { //�������������Ļ�����Ϣ 
root sites/www; //����������������վ��Ŀ¼ 
index index.html index.htm; //������������Ĭ�Ϸ��ʵ���ҳ 
} 
location /status { // �鿴nginx��ǰ��״̬���,��Ҫģ�� ��--with-http_stub_status_module��֧�� 
stub_status on; 
access_log /usr/local/nginx/logs/status.log; 
auth_basic "NginxStatus"; } 
} 
include /usr/local/nginx/server/www1.88181.com; //ʹ��include�ֶ�����server,�������� 
[root@rhel6u3-7 ~]# cat /usr/local/nginx/server/www1.88181.com 
server { 
listen 80; 
server_name www1.88181.com; 
location / { 
root sites/www1; 
index index.html index.htm; 
} 
}
��ƪ������Դ�� Linux������վ(www.linuxidc.com)  ԭ�����ӣ�http://www.linuxidc.com/Linux/2013-02/80069p2.htm
*/
/*
HTTP����ڶ�ȡ�����������ļ�ʱ��������ngx_http_module_t�ӿ�������8���׶Σ�HTTP��������������л���ÿ���׶��е���ngx_http_module_t
����Ӧ�ķ�������Ȼ�����ngx_http_module_t�е�ĳ���ص�������ΪNULL��ָ�룬��ôHTTP����ǲ���������ġ�
*/
/*
��������8���׶εĵ���˳�������������˳���ǲ�ͬ�ġ���Nginx���������У�HTTP��ܵ�����Щ�ص�������ʵ��˳���п����������ģ���nginx.conf�������йأ���
1��create_main_conf
2��create_srv_conf
3��create_loc_conf
4��preconfiguration
5��init_main_conf
6��merge_srv_conf
7��merge_loc_conf
8��postconfiguration
������http{}���ÿ�ʱ��HTTP��ܻ��������HTTPģ�����ʵ�ֵ�create main conf��create_srv_conf��
create_loc_conf�������ɴ洢main�������ò����Ľṹ�壻������servero{}��ʱ���ٴε�������HTTPģ
���create_srv conf��create loc_conf�ص��������ɴ洢srv�������ò����Ľṹ�壻������location{}ʱ
����ٴε���create_loc_conf�ص��������ɴ洢loc�������ò����Ľṹ�塣
������������:
{
  http {
     server{
        location xx {
            mytest;
        }
     }
  }
}
���ڽ�����http{}��ʱ������һ��
location{}�У����ڽ�����http{}��ʱ�����һ��create_loc_conf��������server{}��ʱ������һ��create_loc_conf
������location{}��ʱ�򻹻����һ��create_loc_conf

��ʵ�ϣ�NginxԤ���������ϲ�������10�������ǵ���Ϊ��������ngx_conf_merge_
str- value�����Ƶġ�Nginx�Ѿ�ʵ�ֺõ�10���򵥵�������ϲ��꣬���ǵ�
����������ngx_conf_merge_str_ value -�£����ҳ���ngx_conf_merge_bufs value�⣬����
��������3���������ֱ��ʾ�����ÿ�����������ÿ������Ĭ��ֵ��
  NginxԤ���10��������ϲ���
�����������������������������ש���������������������������������������������������������������������������
��    ������ϲ���          ��    ����                                                                  ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ�����ʹ�õȺ�(=)ֱ�Ӹ�ֵ�ı��������Ҹñ�����create loc conf�ȷ�      ��
��ngx_conf_merge_value      ���䷽���г�ʼ��ΪNGX CONF UNSET���������͵ĳ�Ա����ʹ��ngx_conf_           ��
��                          ��merge_value�ϲ���                                                         ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ�ָ�����͵ı��������Ҹñ�����create loc conf�ȷ��䷽���г�ʼ��Ϊ     ��
��ngx_conf_merge_ptr_value  ��NGX CONF UNSET PTR���������͵ĳ�Ա����ʹ��ngx_conf_merge_ptr_value        ��
��                          ���ϲ���                                                                    ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ��������͵ı��������Ҹñ�����create loc conf�ȷ��䷽���г�ʼ��Ϊ     ��
��ngx_conf_merge_uint_value ��NGX CONF UNSET UINT���������͵ĳ�Ա����ʹ��ngx_conf_merge_uint_           ��
��                          �� value�ϲ���                                                              ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ���ʾ�����ngx_msec_t���͵ı��������Ҹñ�����create loc conf�ȷ�     ��
��ngx_conf_merge_msec_value ���䷽���г�ʼ��ΪNGX CONF UNSET MSEC���������͵ĳ�Ա����ʹ��ngx_           ��
��                          ��conf_merge_msec_value�ϲ���                                               ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �Ტ��ʾ���timej���͵ı��������Ҹñ�����create loc conf�ȷ��䷽����    ��
��ngx_conf_merge_sec_value  ����ʼ��ΪNGX CONF UNSET���������͵ĳ�Ա����ʹ��ngx_conf_merge_sec_         ��
��                          ��value�ϲ���                                                               ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ�size-t�ȱ�ʾ�ռ䳤�ȵı��������Ҹñ�����create- loc_ conf�ȷ��䷽   ��
��ngx_conf_merge_size_value �����г�ʼ��ΪNGX��CONF UNSET SIZE���������͵ĳ�Ա����ʹ��ngx_conf_         ��
��                          ��merge_size_value�ϲ���                                                    ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  �ϲ�off�ȱ�ʾƫ�����ı��������Ҹñ�����create loc conf�ȷ��䷽����      ��
��ngx_conf_merge_off_value  ����ʼ��ΪNGX CONF UNSET���������͵ĳ�Ա����ʹ��ngx_conf_merge_off_         ��
��                          ��value�ϲ���                                                               ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  ngx_str_t���͵ĳ�Ա����ʹ��ngx_conf_merge_str_value�ϲ�����ʱ���˵�     ��
��ngx_conf_merge_str_value  ��                                                                          ��
��                          ��default����������һ��charˮ�ַ���                                         ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��                          ��  ngx_bufs t���͵ĳ�Ա����ʹ��ngx_conf merge_str_value�Ტ�꣬��ʱ���˵�  ��
��ngx_conf_merge_bufs_value ��                                                                          ��
��                          ��default��������������Ϊngx_bufsj������������Ա��������Ҫ��������Ĭ��ֵ    ��
�ǩ��������������������������贈��������������������������������������������������������������������������
��ngx_conf_merge_bitmask_   ��  �Զ�����λ����ʾ��־λ�����ͳ�Ա������ʹ��ngx_conf_merge_bitmask_       ��
��value                     ��value�ϲ���                                                               ��
�����������������������������ߩ���������������������������������������������������������������������������
*/ 

/*
������http{}���ÿ�ʱ��HTTP��ܻ��������HTTPģ�����ʵ�ֵ�create main conf��create_srv_conf��
create_loc_conf�������ɴ洢main�������ò����Ľṹ�壻������servero{}��ʱ���ٴε�������HTTPģ
���create_srv conf��create loc_conf�ص��������ɴ洢srv�������ò����Ľṹ�壻������location{}ʱ
����ٴε���create_loc_conf�ص��������ɴ洢loc�������ò����Ľṹ�塣
������������:
{
  http {
     server{
        location xx { //location��Ҫ����uri����ƥ��
            mytest;
        }
     }
  }
}
���ڽ�����http{}��ʱ������һ��
location{}�У����ڽ�����http{}��ʱ�����һ��create_loc_conf��������server{}��ʱ������һ��create_loc_conf
������location{}��ʱ�򻹻����һ��create_loc_conf

��ngx_http_mytest_config_moduleΪ��:
HTTP����ڽ���nginx.conf�ļ�ʱֻҪ����http{}��server{}��location{}���ÿ�ͻ����̷���һ��
http_mytest_conf_t�ṹ�塣
*/

//���еĺ���ģ��NGX_CORE_MODULE��Ӧ��������ctxΪngx_core_module_t����ģ�飬����http{} NGX_HTTP_MODULEģ���Ӧ��Ϊ������Ϊngx_http_module_t
//events{} NGX_EVENT_MODULEģ���Ӧ��Ϊ������Ϊngx_event_module_t

//ginx�������ļ���Ϊ4���֣�main��ȫ�����ã���server���������ã���upstream�����ؾ���������裩��location��URLƥ���ض�λ�õ����ã��������߹�ϵΪ��server�̳�main��location�̳�server��upstream�Ȳ���̳���������Ҳ���ᱻ�̳С�
//��Ա�е�createһ���ڽ���ǰִ�к�����merge�ں�����ִ��
typedef struct { //ע���ngx_http_conf_ctx_t�ṹ���        ��ʼ����ִֵ�У����Ϊ"http{}"�е����ã���ngx_http_block��, ,���е�NGX_HTTP_MODULEģ�鶼��ngx_http_block��ִ��
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf); //���������ļ�ǰ����
    //һ�������Ѷ�Ӧ��ģ����뵽11���׶ζ�Ӧ�Ľ׶�ȥngx_http_phases,����ngx_http_realip_module��ngx_http_realip_init
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf); //��������ļ��Ľ��������  

/*����Ҫ�������ݽṹ���ڴ洢main����ֱ����http{...}����������ȫ��������ʱ������ͨ��create_main_conf�ص����������洢ȫ��������Ľṹ��*/
    void       *(*create_main_conf)(ngx_conf_t *cf);
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);//�����ڳ�ʼ��main����������

/*����Ҫ�������ݽṹ���ڴ洢srv����ֱ������������server{...}����������������ʱ������ͨ��ʵ��create_srv_conf�ص����������洢srv����������Ľṹ��*/
    void       *(*create_srv_conf)(ngx_conf_t *cf);
// merge_srv_conf�ص�������Ҫ���ںϲ�main�����srv�����µ�ͬ��������
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

    /*����Ҫ�������ݽṹ���ڴ洢loc����ֱ����location{...}����������������ʱ������ʵ��create_loc_conf�ص�����*/
    void       *(*create_loc_conf)(ngx_conf_t *cf);

    /*
    typedef struct {
           void * (*create_loc_conf) (ngx_conf_t *cf) ;
          char*(*merge_loc_conf) (ngx_conf_t *cf, void *prev,
    }ngx_http_module_t
        ������δ��붨����create loc_conf��������ζ��HTTP��ܻὨ��loc��������á�
    ʲô��˼�أ�����˵�����û��ʵ��merge_loc_conf������Ҳ�����ڹ���ngx_http_module_t
    ʱ��merge_loc_conf��ΪNULL�ˣ���ô��4.1�ڵ�������server�����http���ڳ��ֵ�
    �����������Ч���������ϣ����server�����http���ڵ�������Ҳ��Ч����ô����ͨ��
    merge_loc_conf������ʵ�֡�merge_loc_conf������������ÿ���������������ÿ��ͬ��
    ������ϲ�����Ȼ����κϲ�ȡ���ھ����merge_loc_confʵ�֡�
        merge_loc_conf��3����������1��������Ȼ��ngx_conf_t *cf���ṩһЩ����������
    �ṹ�����ڴ�ء���־�ȡ�������Ҫ��ע���ǵ�2����3�����������е�2������void *prev
    ��ָ���������ÿ�ʱ���ɵĽṹ�壬����3������void:-leconf��ָ�����Ǳ��������ÿ�Ľ�
    ���塣
    */
    // merge_loc_conf�ص�������Ҫ���ںϲ�srv�����loc�����µ�ͬ��������
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf); //nginx�ṩ��10��Ԥ��ϲ��꣬������
} ngx_http_module_t; //

//���еĺ���ģ��NGX_CORE_MODULE��Ӧ��������ctxΪngx_core_module_t����ģ�飬����http{} NGX_HTTP_MODULEģ���Ӧ��Ϊ������Ϊngx_http_module_t
//events{} NGX_EVENT_MODULEģ���Ӧ��Ϊ������Ϊngx_event_module_t
//http{}�ڲ����������Զ�����NGX_HTTP_MODULE
#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

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
#define NGX_HTTP_MAIN_CONF        0x02000000
#define NGX_HTTP_SRV_CONF         0x04000000
#define NGX_HTTP_LOC_CONF         0x08000000
#define NGX_HTTP_UPS_CONF         0x10000000
#define NGX_HTTP_SIF_CONF         0x20000000
#define NGX_HTTP_LIF_CONF         0x40000000 
#define NGX_HTTP_LMT_CONF         0x80000000


#define NGX_HTTP_MAIN_CONF_OFFSET  offsetof(ngx_http_conf_ctx_t, main_conf)
#define NGX_HTTP_SRV_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, srv_conf)
#define NGX_HTTP_LOC_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, loc_conf)


//ע��ngx_http_get_module_main_conf ngx_http_get_module_loc_conf��ngx_http_get_module_ctx������

//��ȡ����request��Ӧ���Լ�������main,����http{}�Ķ�Ӧģ���������Ϣ
#define ngx_http_get_module_main_conf(r, module)                             \
    (r)->main_conf[module.ctx_index]
//��ȡ����request��Ӧ���Լ�������srv������server{} upstream{}�Ķ�Ӧģ���������Ϣ
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]

//ngx_http_get_module_ctx�洢���й����еĸ���״̬(�����ȡ������ݣ�������Ҫ��ζ�ȡ)  ngx_http_get_module_loc_conf��ȡ��ģ����local{}�е�������Ϣ
//��ȡ����request��Ӧ���Լ�������loc������location{} �Ķ�Ӧģ���������Ϣ
#define ngx_http_get_module_loc_conf(r, module)  (r)->loc_conf[module.ctx_index]


#define ngx_http_conf_get_module_main_conf(cf, module)                        \
    ((ngx_http_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_http_conf_get_module_srv_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]
#define ngx_http_conf_get_module_loc_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]

#define ngx_http_cycle_get_module_main_conf(cycle, module)                    \
    (cycle->conf_ctx[ngx_http_module.index] ?                                 \
        ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index])      \
            ->main_conf[module.ctx_index]:                                    \
        NULL)


#endif /* _NGX_HTTP_CONFIG_H_INCLUDED_ */
