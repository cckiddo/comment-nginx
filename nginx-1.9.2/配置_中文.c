/*  http://shouce.jb51.net/nginx/left.html
��ģ�飨Main Module�� 


��ժҪ
����һЩNginx�Ļ������ƹ���
��ָ��
daemon
�﷨��daemon on | off
Ĭ��ֵ��on

daemon off;
���������в�Ҫʹ��"daemon"��"master_process"ָ���Щָ������ڿ������ԡ���Ȼ����ʹ��daemon off�����������У�������������û���κΰ�����������������������Զ��Ҫʹ��master_process off�� 
env
�﷨��env VAR|VAR=VALUE
Ĭ��ֵ��TZ
ʹ���ֶΣ�main
��������������޶�һЩ����������ֵ�������µ�����»ᴴ�����޸ı�����ֵ��

���ڲ�ͣ����������������ӻ�ɾ��һЩģ��ʱ�̳еı���
��ʹ��Ƕ��ʽperlģ��
��ʹ�ù����еĽ��̣������ס��ĳЩ����ϵͳ�����Ϊ������ڱ�����ʼ��ʱƵ����ʹ�ÿ��ļ�������Ȼ������֮ǰ�������������ã������ᵽ
����ͣ�������ļ���һ�����⣨�˾䲻֪��ô����ԭ�ģ�for use by working processes. However it is necessary to keep in mind, that 
management of behaviour of system libraries in a similar way probably not always as frequently libraries use variables only during 
initialization, that is still before they can be set by means of the given instruction. Exception to it is the above described updating 
an executed file with zero downtime. ��
���û����ȷ�Ķ���TZ��ֵ,Ĭ������������Ǽ̳еģ��������õ�Perlģ�����ǿ���ʹ��TZ��ֵ��
����

env  MALLOC_OPTIONS;
env  PERL5LIB=/data/site/modules;
env  OPENSSL_ALLOW_PROXY_CERTS=1;debug_points
�﷨��debug_points [stop | abort] 
Ĭ��ֵ��none���ޣ�

debug_points stop;��Nginx�ڲ��кܶ���ԣ����debug_points��ֵ��Ϊstopʱ����ô��������ʱ��ֹͣNginx�����ӵ����������debug_point��ֵ
��Ϊabort,��ô��������ʱ�������ں��ļ���
error_log
�﷨��error_log file [ debug | info | notice | warn | error | crit ] 
Ĭ��ֵ��${prefix}/logs/error.log
ָ��Nginx������FastCGI��������־�ļ�λ�á�
ÿ���ֶεĴ�����־�ȼ�Ĭ��ֵ��

1��main�ֶ� - error
2��HTTP�ֶ� - crit
3��server�ֶ� - crit

Nginx֧��Ϊÿ�������������ò�ͬ�Ĵ�����־�ļ�����һ��Ҫ����lighttpd����ϸΪÿ�������������ò�ͬ������־��������ο���
SeparateErrorLoggingPerVirtualHost��mailing list thread on separating error logging per virtual host
������ڱ��밲װNginxʱ������--with-debug�����������ʹ���������ã�

error_log LOGFILE [debug_core | debug_alloc | debug_mutex | debug_event | debug_http | debug_imap];ע��error_log off��
���ܹر���־��¼���ܣ����Ὣ��־�ļ�д��һ���ļ���Ϊoff���ļ��У��������رմ�����־��¼���ܣ�Ӧʹ���������ã�

error_log /dev/null crit;ͬ��ע��0.7.53�汾��nginx��ʹ�������ļ�ָ���Ĵ�����־·��ǰ��ʹ�ñ���ʱָ����Ĭ����־λ�ã����
����nginx���û��Ը�λ��û��д��Ȩ�ޣ�nginx��������´���

[alert]: could not open error log file: open() "/var/log/nginx/error.log" failed (13: Permission denied)log_not_found
�﷨��log_not_found on | off 
Ĭ��ֵ��on 
ʹ���ֶΣ�location 
�������ָ�����Ƿ��¼�ͻ��˵��������404�������־��ͨ�����ڲ����ڵ�robots.txt��favicon.ico�ļ������磺

location = /robots.txt {
  log_not_found  off;
}
include
�﷨��include file | * 
Ĭ��ֵ��none 
����԰���һЩ�����������ļ����������Ҫ�Ĺ��ܡ�
0.4.4�汾�Ժ�includeָ���Ѿ��ܹ�֧���ļ�ͨ�����

include vhosts/*.conf;ע�⣺ֱ��0.6.7�汾����������������ļ����·�������ڱ���ʱָ����--prefix=PATHĿ¼��������Ĭ����
/usr/local/nginx������㲻��ָ�����Ŀ¼�µ��ļ�����д����·����
0.6.7�汾�Ժ�ָ�����ļ����·������nginx.conf���ڵ�Ŀ¼��������������prefixĿ¼��·����

lock_file
�﷨��lock_file file 
Ĭ��ֵ������ʱָ�� 

lock_file  /var/log/lock_file;Nginxʹ�����ӻ���������˳���accept()ϵͳ���ã����Nginx��i386,amd64,sparc64,��ppc64������ʹ��
gcc,Intel C++,��SunPro C++���б��룬Nginx�������첽������з��ʿ��ƣ���������������ļ��ᱻʹ�á�

master_process
�﷨��master_process on | off 
Ĭ��ֵ��on 

master_process  off;���������в�Ҫʹ��"daemon"��"master_process"ָ���Щѡ������ڿ������ԡ�

pid
�﷨��pid file 
Ĭ��ֵ������ʱָ�� 

pid /var/log/nginx.pid;ָ��pid�ļ�������ʹ��kill��������������źţ���������������¶�ȡ�����ļ��������ʹ�ã�kill -HUP `cat /var/log/nginx.pid`

ssl_engine
�﷨��ssl_engine engine 
Ĭ��ֵ��������ϵͳ���� 
�������ָ������ʹ�õ�OpenSSL���棬�����ʹ����������ҳ��ĸ��ǿ��õģ�openssl engine -t

$ openssl engine -t
(cryptodev) BSD cryptodev engine
  [ ���� ] 
(dynamic) Dynamic engine loading support
  [ ������ ] timer_resolution
�﷨��timer_resolution t 
Ĭ��ֵ��none 

timer_resolution  100ms;���������������gettimeofday()ϵͳ���õ�ʱ�䣬Ĭ�������gettimeofday()�����ж�������ɺ�Żᱻ���ã�kevent(), epoll, /dev/poll, select(), poll()��
�������Ҫһ���Ƚ�׼ȷ��ʱ������¼$upstream_response_time����$msec����������ܻ��õ�timer_resolution

try_files 
�﷨��try_files path1 [ path2] uri 
Ĭ��ֵ��none 
���ð汾��0.7.27 
����˳������ڵ��ļ������ҷ����ҵ��ĵ�һ���ļ���б��ָĿ¼��$uri / �������û���ҵ��ļ�������£�������һ������Ϊlast���ڲ�
�ض������last���������롱��������������URL����������һ���ڲ�����
�ڴ���Mongrel��ʹ�ã�

location / {
  try_files /system/maintenance.html
  $uri $uri/index.html $uri.html @mongrel;
}
 
location @mongrel {
  proxy_pass http://mongrel;
}��Drupal / FastCGI�У� 
location / {
  try_files $uri $uri/ @drupal;
}
 
location ~ \.php$ {
  try_files $uri @drupal;
  fastcgi_pass 127.0.0.1:8888;
  fastcgi_param  SCRIPT_FILENAME  /path/to$fastcgi_script_name;
  # other fastcgi_param
}
 
location @drupal {
  fastcgi_pass 127.0.0.1:8888;
  fastcgi_param  SCRIPT_FILENAME  /path/to/index.php;
  fastcgi_param  QUERY_STRING     q=$request_uri;
  # other fastcgi_param
}����������У����try_filesָ�

location / {
  try_files $uri $uri/ @drupal;
}��ͬ���������ã�

location / {
  error_page     404 = @drupal;
  log_not_found  off;
}��Σ�

location ~ \.php$ {
  try_files $uri @drupal;
 
  fastcgi_pass 127.0.0.1:8888;
  fastcgi_param  SCRIPT_FILENAME  /path/to$fastcgi_script_name;
  # other fastcgi_param
}ָtry_files�ڽ������ύ��FastCGI����֮ǰ�����ڵ�php�ļ���
һ����Wordpress��Joomla�е����ӣ�

location / {
  try_files $uri $uri/ @wordpress;
}
 
location ~ \.php$ {
  try_files $uri @wordpress;
 
  fastcgi_pass 127.0.0.1:8888;
  fastcgi_param SCRIPT_FILENAME /path/to$fastcgi_script_name;
  # other fastcgi_param
}
 
location @wordpress {
    fastcgi_pass 127.0.0.1:8888;
    fastcgi_param SCRIPT_FILENAME /path/to/index.php;
    # other fastcgi_param
}user
�﷨��user user [group] 
Ĭ��ֵ��nobody nobody 
�����������root���У�Nginx�������setuid()/setgid()�������û�/�飬���û��ָ���飬��ô��ʹ�����û�����ͬ���飬Ĭ������»�ʹ��
nobody�û���nobody�飨����nogroup���������ڱ���ʱָ����--user=USER��--group=GROUP��ֵ��

user www users;worker_cpu_affinity
�﷨��worker_cpu_affinity cpumask [cpumask...] 
Ĭ��ֵ��none 
��֧��linuxϵͳ��
�������������������ָ����cpu��������sched_setaffinity()����

worker_processes     4;
worker_cpu_affinity 0001 0010 0100 1000;ָ��ÿ�����̵�һ��CPU��

worker_processes     2;
worker_cpu_affinity 0101 1010;ָ����һ�����̵�CPU0/CPU2��ָ���ڶ������̵�CPU1/CPU3������HTT��������˵��һ�������ѡ��

worker_priority
�﷨��worker_priority [-] number 
Ĭ��ֵ��on 
���ѡ����������������й������̵����ȼ�����linuxϵͳ�е�nice����������setpriority()��
worker_processes
�﷨��worker_processes number 
Ĭ��ֵ��1 

worker_processes 5;�������¼���ԭ��Nginx������Ҫ���в�ֹһ������

��ʹ����SMP���Գƶദ��������
�����������ڴ���I/O����ƿ��ʱΪ�˼�����Ӧʱ�䡣
����ʹ��select()/poll()������ÿ�����̵����������ʱ��

���¼�ģ����һ�������ǽ�ʹ��worker_processes��worker_connections���������������������max_clients����
max_clients = worker_processes * worker_connections 

worker_rlimit_core
�﷨��worker_rlimit_core size 
Ĭ��ֵ�� 
�����ÿ�����̺����ļ����ֵ��

worker_rlimit_nofile
�﷨��worker_rlimit_nofile limit 
Ĭ��ֵ�� 
�����ܹ��򿪵�����ļ���������

worker_rlimit_sigpending 
�﷨��worker_rlimit_sigpending limit 
Ĭ��ֵ�� 
linux�ں�2.6.8�Ժ����Ƶ��õĽ�������ʵ�û����п���ʹ�õ��ź�������

working_directory 
�﷨��working_directory path 
Ĭ��ֵ��--prefix 
����Ĺ���Ŀ¼��һ��ֻ����ָ�������ļ�λ�ã�Nginx��ʹ�þ���·���������������ļ��е����·����ת�Ƶ�--prefix==PATH

������
$pid
����ID��
$realpath_root
δ���
���ο��ĵ�
ǰ��->�¼�ģ�飨Events Module��













�¼�ģ�飨Events Module��


��ժҪ
����Nginx�������ӵķ�ʽ
��ָ��
accept_mutex
�﷨��accept_mutex [ on | off ] 
Ĭ��ֵ��on 
Nginxʹ�����ӻ���������˳���accept()ϵͳ����
accept_mutex_delay 
�﷨��accept_mutex_delay Nms; 
Ĭ��ֵ��500ms
���һ������û�л��������������������ֵ��ʱ��󱻻��գ�Ĭ����500ms

debug_connection
�﷨��debug_connection [ip | CIDR] 
Ĭ��ֵ��none 
0.3.54�汾���������֧��CIDR��ַ�ظ�ʽ��
�����������ָ��ֻ��¼��ĳ���ͻ���IP������debug��Ϣ��
��Ȼ��Ҳ����ָ�����������

error_log /var/log/nginx/errors;
events {
  debug_connection   192.168.1.1;
}devpoll_changes
devpoll_events 
kqueue_changes
kqueue_events 
epoll_events
�﷨��devpoll_changes 
Ĭ��ֵ��
��Щ����ָ���˰��չ涨��ʽ���ݵ����������ں˵��¼�����Ĭ��devpoll��ֵΪ32������Ϊ512��
multi_accept
�﷨��multi_accept [ on | off ] 
Ĭ��ֵ��off 
multi_accept��Nginx�ӵ�һ��������֪ͨ�����accept()�����ܾ����������
rtsig_signo
�﷨��rtsig_signo 
Ĭ��ֵ��
Nginx��rtsigģʽ���ú�ʹ�������źţ���ָ��ָ����һ���źű�ţ��ڶ����źű��Ϊ��һ����1
Ĭ��rtsig_signo��ֵΪSIGRTMIN+10 (40)��
rtsig_overflow_events
rtsig_overflow_test
rtsig_overflow_threshold

�﷨��rtsig_overflow_* 
Ĭ��ֵ��
��Щ����ָ����δ���rtsig��������������������nginx���rtsig����ʱ�����ǽ���������poll()�� rtsig.poll()������δ��ɵ��¼���
ֱ��rtsig���ſ��Է�ֹ�µ�����������������ϣ�nginx�ٴ�����rtsigģʽ��
rtsig_overflow_events specifiesָ������poll()���¼�����Ĭ��Ϊ16
rtsig_overflow_testָ��poll()��������¼���nginx���ſ�rtsig���У�Ĭ��ֵΪ32
rtsig_overflow_thresholdֻ��������Linux 2.4.x�ں��£����ſ�rtsig����ǰnginx����ں���ȷ��������������������
Ĭ��ֵΪ1/10����rtsig_overflow_threshold 3����Ϊ1/3��
use
�﷨��use [ kqueue | rtsig | epoll | /dev/poll | select | poll | eventport ] 
Ĭ��ֵ��
�������./configure��ʱ��ָ���˲�ֹһ���¼�ģ�ͣ������ͨ�������������nginx����ʹ����һ���¼�ģ�ͣ�Ĭ�������nginx�ڱ���ʱ���
�����ʺ���ϵͳ���¼�ģ�͡�
����������￴�����п��õ��¼�ģ�Ͳ��������./configureʱ�������ǡ�

worker_connections
�﷨��worker_connections 
Ĭ��ֵ��
worker_connections��worker_proceses������ģ�飩������������������������
��������� = worker_processes * worker_connections 
�ڷ���������£�
��������� = worker_processes * worker_connections/4
���������Ĭ�ϴ�2�����ӵ���������nginxʹ��������ͬ��ַ�ص�fds���ļ�����������ǰ���������










HTTP����ģ�飨HTTP Core��


��ժҪ
Nginx����HTTP�ĺ��Ĺ���ģ��
��ָ��
alias
�﷨��alias file-path|directory-path; 
Ĭ��ֵ��no
ʹ���ֶΣ�location
���ָ��ָ��һ��·��ʹ��ĳ��ĳ����ע��������������root������document rootû�иı䣬����ֻ��ʹ���˱���Ŀ¼���ļ���

location  /i/ {
  alias  /spool/w3/images/;
}
�ϸ������ܣ�����"/i/top.gif"����������ļ�: "/spool/w3/images/top.gif"��
Aliasͬ���������ڴ�������ʽ��location���磺

location ~ ^/download/(.*)$ {
  alias /home/website/files/$1;
}����"/download/book.pdf"������"/home/website/files/book.pdf"��
ͬ����Ҳ�����ڱ���Ŀ¼�ֶ���ʹ�ñ�����

client_body_in_file_only 
�﷨��client_body_in_file_only on|off 
Ĭ��ֵ��off 
ʹ���ֶΣ�http, server, location 
���ָ��ʼ�մ洢һ����������ʵ�嵽һ���ļ���ʹ��ֻ��0�ֽڡ�
ע�⣺������ָ��򿪣���ôһ������������ɺ����洢���ļ�������ɾ����
���ָ���������debug���Ժ�Ƕ��ʽPerlģ���е�$r->request_body_file��

client_body_in_single_buffer 
�﷨��client_body_in_single_buffer 
Ĭ��ֵ��off 
ʹ���ֶΣ�http, server, location 
���ָ��(0.7.58�汾)ָ���Ƿ񽫿ͻ����������������ķ���һ������������ʹ�ñ���$request_bodyʱ�Ƽ�ʹ�����ָ���Լ��ٸ��Ʋ�����
����޷���һ��������뵥�������������ᱻ������̡�
client_body_buffer_size
�﷨��client_body_buffer_size the_size 
Ĭ��ֵ��8k/16k 
ʹ���ֶΣ�http, server, location 
���ָ�����ָ����������ʵ��Ļ�������С��
����������󳬹�������ָ����ֵ����ô��Щ����ʵ�������򲿷ֽ�����д��һ����ʱ�ļ���
Ĭ��ֵΪ�����ڴ��ҳ��Сֵ������ƽ̨�Ĳ�ͬ��������8k��16k��
client_body_temp_path
�﷨��client_body_temp_path dir-path [ level1 [ level2 [ level3 ] 
Ĭ��ֵ��client_body_temp 
ʹ���ֶΣ�http, server, location 
ָ��ָ����������ʵ����ͼд�����ʱ�ļ�·����
����ָ������Ŀ¼�ṹ���磺

client_body_temp_path  /spool/nginx/client_temp 1 2;��ô����Ŀ¼�ṹ������������

/spool/nginx/client_temp/7/45/00000123457client_body_timeout
�﷨��client_body_timeout time
Ĭ��ֵ��60 
ʹ���ֶΣ�http, server, location 
ָ��ָ����ȡ����ʵ��ĳ�ʱʱ�䡣
����ĳ�ʱ��ָһ������ʵ��û�н����ȡ���裬������ӳ������ʱ����ͻ���û���κ���Ӧ��Nginx������һ��"Request time out" (408)����

client_header_buffer_size
�﷨��client_header_buffer_size size 
Ĭ��ֵ��1k 
ʹ���ֶΣ�http, server 
ָ��ָ���ͻ�������ͷ���Ļ�������С
������������һ������ͷ�������1k
���������������wap�ͻ��˵Ľϴ��cookie�����ܻ����1k��Nginx���������һ������Ļ����������ֵ������large_client_header_buffers�������á�

client_header_timeout 
�﷨��client_header_timeout time 
Ĭ��ֵ��60 
ʹ���ֶΣ�http, server 
ָ��ָ����ȡ�ͻ�������ͷ����ĳ�ʱʱ�䡣
����ĳ�ʱ��ָһ������ͷû�н����ȡ���裬������ӳ������ʱ����ͻ���û���κ���Ӧ��Nginx������һ��"Request time out" (408)����

client_max_body_size 
�﷨��client_max_body_size size 
Ĭ��ֵ��client_max_body_size 1m 
ʹ���ֶΣ�http, server, location 
ָ��ָ������ͻ������ӵ��������ʵ���С��������������ͷ����Content-Length�ֶΡ�
����������ָ����ֵ���ͻ��˽��յ�һ��"Request Entity Too Large" (413)����
��ס�����������֪��������ʾ�������

default_type
�﷨�� default_type MIME-type 
Ĭ��ֵ��default_type text/plain 
ʹ���ֶΣ�http, server, location 
ĳ���ļ��ڱ�׼MIME��ͼû��ָ��������µ�Ĭ��MIME���͡�
�ο�types��

location = /proxy.pac {
  default_type application/x-ns-proxy-autoconfig;
}
location = /wpad.dat {
  rewrite . /proxy.pac;
  default_type application/x-ns-proxy-autoconfig;
}
directio
�﷨��directio [size|off] 
Ĭ��ֵ��directio off 
ʹ���ֶΣ�http, server, location 
�������ָ���ڶ�ȡ�ļ���С����ָ��ֵ���ļ�ʱʹ��O_DIRECT��FreeBSD, Linux����F_NOCACHE��Mac OS X�����ߵ���directio()������Solaris����
��һ�������õ��������ʱ�����sendfile��ͨ������������ڴ��ļ���

directio  4m;error_page
�﷨��error_page code [ code... ] [ = | =answer-code ] uri | @named_location 
Ĭ��ֵ��no 
ʹ���ֶΣ�http, server, location, location �е�if�ֶ� 
�����������Ϊ�������ָ����Ӧ�Ĵ���ҳ��

error_page   404          /404.html;
error_page   502 503 504  /50x.html;
error_page   403          http://example.com/forbidden.html;
error_page   404          = @fetch;ͬ������Ҳ�����޸ķ��صĴ�����룺

error_page 404 =200 /.empty.gif;���һ���������Ӧ�����������FastCGI����������������������Է��ز�ͬ����Ӧ�룬��200, 302, 401��404��
��ô����ָ����Ӧ�뷵�أ�

error_page   404  =  /404.php;������ض���ʱ����Ҫ�ı�URI�����Խ�����ҳ���ض���һ��������location�ֶ��У�

location / (
    error_page 404 = @fallback;
)

location @fallback (
    proxy_pass http://backend;
)if_modified_since 
�﷨��if_modified_since [off|exact|before]
Ĭ��ֵ��if_modified_since exact 
ʹ���ֶΣ�http, server, location 
ָ�0.7.24��������ν��ļ�����޸�ʱ��������ͷ�е�"If-Modified-Since"ʱ����Ƚϡ�

��off �����������ͷ�е�"If-Modified-Since"��0.7.34����
��exact����ȷƥ��
��before���ļ��޸�ʱ��ӦС������ͷ�е�"If-Modified-Since"ʱ��

internal 
�﷨��internal 
Ĭ��ֵ��no 
ʹ���ֶΣ� location 
internalָ��ָ��ĳ��locationֻ�ܱ����ڲ��ġ�������ã��ⲿ�ĵ�������᷵��"Not found" (404)
���ڲ��ġ���ָ�������ͣ�

��ָ��error_page�ض��������
��ngx_http_ssi_moduleģ����ʹ��include virtualָ�����ĳЩ������
��ngx_http_rewrite_moduleģ����ʹ��rewriteָ���޸ĵ�����

һ����ֹ����ҳ�汻�û�ֱ�ӷ��ʵ����ӣ�

error_page 404 /404.html;
location  /404.html {
  internal;
}

keepalive_timeout 
�﷨��keepalive_timeout [ time ] [ time ]
Ĭ��ֵ��keepalive_timeout 75 
ʹ���ֶΣ�http, server, location 
�����ĵ�һ��ֵָ���˿ͻ���������������ӵĳ�ʱʱ�䣬�������ʱ�䣬���������ر����ӡ�
�����ĵڶ���ֵ����ѡ��ָ����Ӧ��ͷ��Keep-Alive: timeout=time��timeֵ�����ֵ����ʹһЩ�����֪��ʲôʱ��ر����ӣ��Ա������
�����ظ��رգ������ָ�����������nginx������Ӧ��ͷ�з���Keep-Alive��Ϣ�������Ⲣ����ָ������һ�����ӡ�Keep-Alive����
������������ֵ���Բ���ͬ
�����г���һЩ��������δ������Keep-Alive��Ӧ��ͷ��

��MSIE��Opera��Keep-Alive: timeout=Nͷ���ԡ�
��MSIE����һ�����Ӵ�Լ60-65�룬Ȼ����һ��TCP RST��
��Opera��һֱ����һ�����Ӵ��ڻ״̬��
��Mozilla��һ��������N�Ļ��������Ӵ�Լ1-10�롣
��Konqueror����һ�����Ӵ�ԼN�롣

keepalive_requests 
�﷨��keepalive_requests n 
Ĭ��ֵ��keepalive_requests 100 
ʹ���ֶΣ�http, server, location 
���������ֳ����ӵ���������

large_client_header_buffers 
�﷨��large_client_header_buffers number size 
Ĭ��ֵ��large_client_header_buffers 4 4k/8k 
ʹ���ֶΣ�http, server 
ָ���ͻ���һЩ�Ƚϴ������ͷʹ�õĻ����������ʹ�С��
�����ֶβ��ܴ���һ����������С������ͻ��˷���һ���Ƚϴ��ͷ��nginx������"Request URI too large" (414)
ͬ���������ͷ����ֶβ��ܴ���һ�������������������������"Bad request" (400)��
������ֻ������ʱ�ֿ���
Ĭ��һ����������СΪ����ϵͳ�з�ҳ�ļ���С��ͨ����4k��8k�����һ�������������ս�״̬ת��Ϊkeep-alive������ռ�õĻ����������ͷš�

limit_except 
�﷨��limit_except methods {...} 
Ĭ��ֵ��no 
ʹ���ֶΣ�location 
ָ�������location�ֶ�����һЩhttp���������ơ�
ngx_http_access_module��ngx_http_auth_basic_moduleģ���к�ǿ�ķ��ʿ��ƹ��ܡ�

limit_except  GET {
  allow  192.168.1.0/32;
  deny   all;
}limit_rate 
�﷨��limit_rate speed 
Ĭ��ֵ��no 
ʹ���ֶΣ�http, server, location, location�е�if�ֶ� 
���ƽ�Ӧ���͵��ͻ��˵��ٶȣ���λΪ�ֽ�/�룬���ƽ���һ��������Ч�������һ���ͻ��˴�2�����ӣ��������ٶ������ֵ���Զ���
����һЩ��ͬ��״��������Ҫ��server�ֶ������Ʋ������ӵ��ٶȣ���ô��������������ã����������ѡ������$limit_rate������ֵ���ﵽĿ�ģ�

server {
  if ($slow) {
    set $limit_rate  4k;
  }
}ͬ������ͨ������X-Accel-Limit-Rateͷ��NginxXSendfile��������proxy_pass���ص�Ӧ�𡣲��Ҳ�����X-Accel-Redirectͷ����ɡ�

limit_rate_after 
�﷨��limit_rate_after time 
Ĭ��ֵ��limit_rate_after 1m 
ʹ���ֶΣ�http, server, location, location�е�if�ֶ� 
��Ӧ��һ���ֱ����ݺ������ٶȣ�

limit_rate_after 1m;
limit_rate 100k;listen
�﷨(0.7.x)��listen address:port [ default [ backlog=num | rcvbuf=size | sndbuf=size | accept_filter=filter | deferred | bind | ssl ] ] 
�﷨(0.8.x)��listen address:port [ default_server [ backlog=num | rcvbuf=size | sndbuf=size | accept_filter=filter | deferred | bind | ssl ] ] 
Ĭ��ֵ��listen 80 
ʹ���ֶΣ�server 
listenָ��ָ����server{...}�ֶ��п��Ա����ʵ���ip��ַ���˿ںţ�����ָֻ��һ��ip��һ���˿ڣ�����һ���ɽ����ķ���������

listen 127.0.0.1:8000;
listen 127.0.0.1;
listen 8000;
listen *:8000;
listen localhost:8000;ipv6��ַ��ʽ��0.7.36����һ����������ָ����

listen [::]:8000;
listen [fe80::1];��linux�������FreeBSD����IPv6[::]����ô��ͬ��������Ӧ��IPv4��ַ�������һЩ��ipv6����������Ȼ�������ã���
���ʧ�ܣ���Ȼ����ʹ�������ĵ�ַ������[::]���ⷢ�����⣬Ҳ����ʹ��"default ipv6only=on" ѡ�����ƶ����listen�ֶν���ipv6��ַ��
ע�����ѡ���������listen��Ч������Ӱ��server��������listen�ֶ�ָ����ipv4��ַ��

listen [2a02:750:5::123]:80;
listen [::]:80 default ipv6only=on;���ֻ��ip��ַָ������Ĭ�϶˿�Ϊ80��
���ָ����default��������ô���server�齫��ͨ������ַ:�˿ڡ������з��ʵ�Ĭ�Ϸ����������������Ϊ��Щ��ƥ��server_nameָ���е�����
��ָ��Ĭ��server������������������������ǳ����ã����û��ָ�����default��������ôĬ�Ϸ�������ʹ�õ�һ��server�顣
listen����һЩ��ͬ�Ĳ�������ϵͳ����listen(2)��bind(2)��ָ���Ĳ�������Щ������������default����֮��
backlog=num -- ָ������listen(2)ʱbacklog��ֵ��Ĭ��Ϊ-1��
rcvbuf=size -- Ϊ���ڼ����Ķ˿�ָ��SO_RCVBUF��
sndbuf=size -- Ϊ���ڼ����Ķ˿�ָ��SO_SNDBUF��
accept_filter=filter -- ָ��accept-filter��

��������FreeBSD��������������������dataready��httpready���������հ汾��FreeBSD��FreeBSD: 6.0, 5.4-STABLE��4.11-STABLE���ϣ�Ϊ
���Ƿ���-HUP�źſ��ܻ�ı�accept-filter��
deferred -- ��linuxϵͳ���ӳ�accept(2)���ò�ʹ��һ�������Ĳ����� TCP_DEFER_ACCEPT��
bind -- ��bind(2)�ֿ����á�
����Ҫָ����ġ���ַ:�˿ڡ���ʵ������������˲�ͬ��ָ�����ͬһ���˿ڣ�����ÿ����ͬ�ĵ�ַ��ĳ��ָ�������Ϊ����˿ڵ����е�ַ
��*:port������ônginxֻ��bind(2)������*:port�����������ͨ��ϵͳ����getsockname()ȷ���ĸ���ַ�������ӵ���������ʹ����
parameters backlog, rcvbuf, sndbuf, accept_filter��deferred��Щ��������ô�����ǽ��������ַ:�˿ڡ��ֿ����á�
ssl -- ������0.7.14������listen(2)��bind(2)ϵͳ���ù�����
����ָ�����������listen������������SSLģʽ���⽫���������ͬʱ������HTTP��HTTPS����Э���£����磺
listen  80;
listen  443 default ssl;һ��ʹ����Щ�������������ӣ� 
listen  127.0.0.1 default accept_filter=dataready backlog=1024;0.8.21�汾�Ժ�nginx���Լ���unix�׽ӿڣ� 
listen unix:/tmp/nginx1.sock;location 
�﷨��location [=|~|~*|^~|@] /uri/ { ... } 
Ĭ��ֵ��no 
ʹ���ֶΣ�server 
�����������URI�Ĳ�ͬ�������������ã�����ʹ���ַ�����������ʽƥ�䣬���Ҫʹ��������ʽ�������ָ������ǰ׺��
1��~* �����ִ�Сд��
2��~ ���ִ�Сд��
Ҫȷ����ָ��ƥ���ض��Ĳ�ѯ���������ȶ��ַ�������ƥ�䣬�ַ���ƥ�佫��Ϊ��ѯ�Ŀ�ʼ����ȷ�е�ƥ�佫��ʹ�á�Ȼ��������ʽ��
ƥ���ѯ��ʼ��ƥ���ѯ�ĵ�һ��������ʽ�ҵ����ֹͣ���������û���ҵ�������ʽ����ʹ���ַ��������������
��һЩ����ϵͳ����Mac OS X��Cygwin���ַ�����ͨ�������ִ�Сд�ķ�ʽ���ƥ�䣨0.7.7�������ǣ��ȽϽ����ڵ��ֽڵ����Ի�����
������ʽ���԰�������0.7.40��������������ָ���С�
����ʹ�á�^~����ǽ�ֹ���ַ���ƥ�����������ʽ�������ȷ�е�ƥ��location�������ǣ���ô������ʽ���ᱻ��顣
ʹ�á�=����ǿ�����URI��location֮�䶨�徫ȷ��ƥ�䣬�ھ�ȷƥ����ɺ󲢲����ж��������������������/�������������ʹ��
��location = /���������������
��ʹû�С�=���͡�^~����ǣ���ȷ��ƥ��location���ҵ���ͬ����ֹͣ��ѯ��
�����Ǹ��ֲ�ѯ��ʽ���ܽ᣺
1��ǰ׺��=����ʾ��ȷƥ���ѯ������ҵ�������ֹͣ��ѯ��
2��ָ����Ȼʹ�ñ�׼�ַ��������ƥ��ʹ�á�^~��ǰ׺��ֹͣ��ѯ��
3��������ʽ���������������ļ��ж����˳��
4���������������һ��ƥ�䣬���ƥ�佫��ʹ�ã�����ʹ�õڶ�����ƥ�䡣
����

location  = / {
  # ֻƥ�� / �Ĳ�ѯ.
  [ configuration A ]
}
location  / {
  # ƥ���κ��� / ��ʼ�Ĳ�ѯ������������ʽ��һЩ�ϳ����ַ�����������ƥ�䡣
  [ configuration B ]
}
location ^~ /images/ {
  # ƥ���κ��� /images/ ��ʼ�Ĳ�ѯ����ֹͣ�����������������ʽ��
  [ configuration C ]
}
location ~* \.(gif|jpg|jpeg)$ {
  # ƥ���κ���gif, jpg, or jpeg��β���ļ����������� /images/ Ŀ¼��������Configuration C�д���
  [ configuration D ]
}������Ĵ�����������
��/ -> configuration A 
��/documents/document.html -> configuration B 
��/images/1.gif -> configuration C 
��/documents/1.jpg -> configuration D 
ע����������κ�˳������4�����ò���ƥ��������ͬ�ģ�����ʹ��Ƕ�׵�location�ṹʱ���ܻὫ�����ļ���ĸ��Ӳ��Ҳ���һЩ�Ƚ�����Ľ����
��ǡ�@��ָ��һ��������location������location������������������ִ�У����ǽ�ʹ�����ڲ��ض��������С����鿴error_page��try_files��
log_not_found 
�﷨��log_not_found [on|off] 
Ĭ��ֵ��log_not_found on 
ʹ���ֶΣ�http, server, location 
ָ��ָ���Ƿ�һЩ�ļ�û���ҵ��Ĵ�����Ϣд��error_logָ�����ļ��С�

log_subrequest 
�﷨��log_subrequest [on|off]
Ĭ��ֵ��log_subrequest off 
ʹ���ֶΣ�http, server, location 
ָ��ָ���Ƿ�һЩ����rewrite rules��/��SSI requests����������־д��access_logָ�����ļ��С�

msie_padding 
�﷨��msie_padding [on|off] 
Ĭ��ֵ��msie_padding on 
ʹ���ֶΣ�http, server, location 
ָ��ָ��������ر�MSIE�������chrome�������0.8.25+����msie_padding��������������ܿ�����nginx��Ϊ��Ӧʵ�������С512�ֽڣ���
����Ӧ���ڻ����400��״̬���롣
ָ��Ԥ����MSIE��chrome������м���Ѻõġ�HTTP����ҳ�棬�Ա㲻�ڷ����������ظ���Ĵ�����Ϣ��

msie_refresh 
�﷨�� msie_refresh [on|off] 
Ĭ��ֵ��msie_refresh off 
ʹ���ֶΣ�http, server, location 
ָ�������ܾ�ΪMSIE����һ��refresh��������һ��redirect

open_file_cache 
�﷨��open_file_cache max = N [inactive = time] | off 
Ĭ��ֵ��open_file_cache off 
ʹ���ֶΣ�http, server, location 
���ָ��ָ�������Ƿ����ã�������ã�����¼�ļ�������Ϣ��
���򿪵��ļ�����������С��Ϣ���޸�ʱ�䡣
�����ڵ�Ŀ¼��Ϣ��
���������ļ������еĴ�����Ϣ -- û������ļ����޷���ȷ��ȡ���ο�open_file_cache_errors
ָ��ѡ�

��max - ָ������������Ŀ���������������ʹ�ù����ļ���LRU�������Ƴ���
��inactive - ָ�������ļ����Ƴ���ʱ�䣬��������ʱ�����ļ�û�����أ�Ĭ��Ϊ60�롣
��off - ��ֹ���档

��: 
 open_file_cache max=1000 inactive=20s;
 open_file_cache_valid    30s;
 open_file_cache_min_uses 2;
 open_file_cache_errors   on;open_file_cache_errors 
�﷨��open_file_cache_errors on | off 
Ĭ��ֵ��open_file_cache_errors off 
ʹ���ֶΣ�http, server, location 
���ָ��ָ���Ƿ�������һ���ļ��Ǽ�¼cache����
open_file_cache_min_uses 
�﷨��open_file_cache_min_uses number 
Ĭ��ֵ��open_file_cache_min_uses 1 
ʹ���ֶΣ�http, server, location 
���ָ��ָ������open_file_cacheָ����Ч�Ĳ�����һ����ʱ�䷶Χ�ڿ���ʹ�õ���С�ļ��������ʹ�ø����ֵ���ļ���������cache�����Ǵ�״̬��
open_file_cache_valid 
�﷨��open_file_cache_valid time 
Ĭ��ֵ��open_file_cache_valid 60 
ʹ���ֶΣ�http, server, location 
���ָ��ָ���˺�ʱ��Ҫ���open_file_cache�л�����Ŀ����Ч��Ϣ��
optimize_server_names 
�﷨��optimize_server_names [ on|off ] 
Ĭ��ֵ��optimize_server_names on 
ʹ���ֶΣ�http, server 
���ָ��ָ���Ƿ��ڻ������������������п������Ż�����������顣
�����Ǽ��Ӱ�쵽ʹ�����������ض�������������Ż�����ô���л�����������������������һ������ַ���˿ڶԡ�������ͬ�����ã�����
������ִ�е�ʱ�򲢲������ٴμ�飬�ض����ʹ�õ�һ��server name��
����ض������ʹ�������������ڿͻ��˼��ͨ������ô���������������Ϊoff��
ע�⣺���������������nginx 0.7.x�汾��ʹ�ã���ʹ��server_name_in_redirect��
port_in_redirect 
�﷨��port_in_redirect [ on|off ] 
Ĭ��ֵ��port_in_redirect on 
ʹ���ֶΣ�http, server, location 
���ָ��ָ���Ƿ�����nginx���ض�������жԶ˿ڽ��в�����
������ָ������Ϊoff�����ض����������nginx������url����Ӷ˿ڡ�
recursive_error_pages 
�﷨��recursive_error_pages [on|off] 
Ĭ��ֵ��recursive_error_pages off 
ʹ���ֶΣ�http, server, location 
recursive_error_pagesָ�����ó���һ��error_pageָ������������error_page��
resolver 
�﷨��resolver address 
Ĭ��ֵ��no 
ʹ���ֶΣ�http, server, location 
ָ��DNS��������ַ���磺 
resolver 127.0.0.1;resolver_timeout 
�﷨��resolver_timeout time 
Ĭ��ֵ��30s 
ʹ���ֶΣ�http, server, location 
������ʱʱ�䡣�磺 
resolver_timeout 5s;root
�﷨��root path 
Ĭ��ֵ��root html 
ʹ���ֶΣ�http, server, location ,location�е�if�ֶ�
���󵽴����ļ���Ŀ¼��
�����У�

location  /i/ {
  root  /spool/w3;
}�������"/i/top.gif"�ļ���nginx��ת��"/spool/w3/i/top.gif"�ļ���������ڲ�����ʹ�ñ�����
ע�⣺��������root��������location������ֵ���棬��"/i/top.gif"����������"/spool/w3/top.gif"�ļ������Ҫʵ������������
apache alias�Ĺ��ܣ�����ʹ��aliasָ�
satisfy_any
�﷨��satisfy_any [ on|off ] 
Ĭ��ֵ��satisfy_any off 
ʹ���ֶΣ�location 
ָ����Լ������һ���ɹ�������Ч�飬����NginxHttpAccessModule��NginxHttpAuthBasicModule������ģ����ִ�С�

location / {
  satisfy_any  on;
  allow  192.168.1.0/32;
  deny   all;
  auth_basic            "closed site";
  auth_basic_user_file  conf/htpasswd;
}send_timeout 
�﷨��send_timeout the time 
Ĭ��ֵ��send_timeout 60 
ʹ���ֶΣ�http, server, location 
ָ��ָ���˷��͸��ͻ���Ӧ���ĳ�ʱʱ�䣬Timeout��ָû�н�������established״̬��ֻ������������֣�����������ʱ��ͻ���û��
�κ���Ӧ��nginx���ر����ӡ�

sendfile 
�﷨��sendfile [ on|off ] 
Ĭ��ֵ��sendfile off 
ʹ���ֶΣ�http, server, location 
�Ƿ�����sendfile()������

server 
�﷨��server {...} 
Ĭ��ֵ��no 
ʹ���ֶΣ�http 
server�ֶΰ����������������á�
û����ȷ�Ļ������ֿ����������������е�����ͷ���ͻ���IP������������
����ͨ��listenָ����ָ���������ӵ����server������е�ַ�Ͷ˿ڣ�������server_nameָ���п���ָ�����е�������

server_name
�﷨��server_name name [... ] 
Ĭ��ֵ��server_name hostname 
ʹ���ֶΣ�server 
���ָ�����������ã�
����HTTP���������ͷ����nginx�����ļ��е�server{...}�ֶ���ָ���Ĳ�������ƥ�䣬�����ҳ���һ��ƥ�������������ζ�����������
�ķ�����������ѭ�������ȼ�����
1������ƥ������ơ�
2�����ƿ�ʼ��һ���ļ�ͨ�����*.example.com��
3�����ƽ�����һ���ļ�ͨ�����www.example.*��
4��ʹ��������ʽ�����ơ�
���û��ƥ��Ľ����nginx�����ļ�����װ�������ȼ�ʹ��[#server server { ... }]�ֶΣ�
1��listenָ����Ϊdefault��server�ֶΡ�
2����һ������listen������Ĭ�ϵ�listen 80����server�ֶΡ�
�����server_name_in_redirect�����ã����ָ���������HTTP�ض���ķ���������
����

server {
  server_name   example.com  www.example.com;
}��һ������Ϊ�������Ļ������ƣ�Ĭ������Ϊ������hostname��
��Ȼ������ʹ���ļ�ͨ�����

server {
  server_name   example.com  *.example.com  www.example.*;
}���������е�ǰ�������ƿ��Ժϲ�Ϊһ����

server {
  server_name  .example.com;
}ͬ������ʹ��������ʽ������ǰ��ӡ�~����

server {
  server_name   www.example.com   ~^www\d+\.example\.com$;
}����ͻ���������û������ͷ����û��ƥ��server_name������ͷ���������������ƽ�������һ��HTTP�ض��������ֻʹ�á�*����ǿ��nginx
��HTTP�ض�����ʹ��Hostͷ��ע��*�������ڵ�һ�����ƣ������������һ����ɵ�Ƶ����ƴ��棬�硰_����

server {
  server_name example.com *;
}
server {
  server_name _ *;
}��nginx0.6.x��������ı䣺

server {
  server_name _;
}0.7.12�汾�Ժ��Ѿ�����֧�ֿշ����������Դ�����Щû������ͷ������

server {
  server_name "";
}

server_name_in_redirect
�﷨��server_name_in_redirect on|off 
Ĭ��ֵ��server_name_in_redirect on 
ʹ���ֶΣ�http, server, location 
������ָ��򿪣�nginx��ʹ��server_nameָ���Ļ�������������Ϊ�ض����ַ������رգ�nginx��ʹ�������е�����ͷ��
server_names_hash_max_size 
�﷨��server_names_hash_max_size number 
Ĭ��ֵ��server_names_hash_max_size 512 
ʹ���ֶΣ�http 
���������ƹ�ϣ������ֵ��������Ϣ��ο�nginx Optimizations��
server_names_hash_bucket_size 
�﷨��server_names_hash_bucket_size number 
Ĭ��ֵ��server_names_hash_bucket_size 32/64/128 
ʹ���ֶΣ�http 
���������ƹ�ϣ��ÿ��ҳ��Ĵ�С�����ָ���Ĭ��ֵ������cpu���档������Ϣ��ο�nginx Optimizations��
server_tokens 
�﷨��server_tokens on|off 
Ĭ��ֵ��server_tokens on 
ʹ���ֶΣ�http, server, location 
�Ƿ��ڴ���ҳ��ͷ�����ͷ�����nginx�汾��Ϣ��
tcp_nodelay 
�﷨��tcp_nodelay [on|off] 
Ĭ��ֵ��tcp_nodelay on 
ʹ���ֶΣ�http, server, location 
���ָ��ָ���Ƿ�ʹ��socket��TCP_NODELAYѡ����ѡ��ֻ��keep-alive������Ч��
��������˽�������TCP_NODELAYѡ�����Ϣ��
tcp_nopush 
�﷨��tcp_nopush [on|off] 
Ĭ��ֵ��tcp_nopush off 
ʹ���ֶΣ�http, server, location 
���ָ��ָ���Ƿ�ʹ��socket��TCP_NOPUSH��FreeBSD����TCP_CORK��linux��ѡ����ѡ��ֻ��ʹ��sendfileʱ��Ч��
�������ѡ��Ľ�����nginx��ͼ������HTTPӦ��ͷ��װ��һ�����С�
�������鿴����TCP_NOPUSH��TCP_CORKѡ��ĸ�����Ϣ��
try_files 
�﷨��try_files file1 [file2 ... filen] fallback 
Ĭ��ֵ��none 
ʹ���ֶΣ�location
���ָ�����nginx�����Բ�����ÿ���ļ��Ĵ��ڣ�����URI��ʹ�õ�һ���ҵ����ļ������û���ҵ��ļ�����������Ϊfallback������ʹ�κ����ƣ�
��location�ֶΣ�fallback��һ������Ĳ�������������һ��������location���߿ɿ���URI��
����

location / {
  try_files index.html index.htm @fallback;
}

location @fallback {
  root /var/www/error;
  index index.html;
}types 
�﷨��types {...} 
ʹ���ֶΣ�http, server, location 
����ֶ�ָ��һЩ��չ���ļ���Ӧ��ʽ��Ӧ���MIME���ͣ�һ��MIME���Ϳ�����һЩ�������Ƶ���չ��Ĭ��ʹ�������ļ���Ӧ��ʽ��

types {
  text/html    html;
  image/gif    gif;
  image/jpeg   jpg;
}�����Ķ�Ӧ��ͼ�ļ�Ϊconf/mime.types�����ҽ���������
���������ĳЩ�ض���location�Ĵ���ʽʹ��MIME���ͣ�application/octet-stream������ʹ���������ã�

location /download/ {
  types         { }
  default_type  application/octet-stream;
}������
core module ֧��һЩ���õı�������apacheʹ�õı�����һ�¡�
���ȣ�һЩ���������˿ͻ�������ͷ����һЩ�ֶΣ��磺$http_user_agent, $http_cookie�ȵȡ�ע�⣬������Щ�������������ж��壬����
�����޷���֤�����Ǵ��ڵĻ���˵���Զ��嵽һЩ��ĵط���������ѭһ���Ĺ淶����
����֮�⣬������һЩ����������
$arg_PARAMETER 
������������ڲ�ѯ�ַ���ʱGET����PARAMETER��ֵ��
$args
������������������еĲ�����
$binary_remote_addr 
����������ʽ�Ŀͻ��˵�ַ��
$body_bytes_sent 
δ֪��
$content_length 
����ͷ�е�Content-length�ֶΡ�
$content_type
����ͷ�е�Content-Type�ֶΡ�
$cookie_COOKIE 
cookie COOKIE��ֵ��
$document_root 
��ǰ������rootָ����ָ����ֵ��
$document_uri 
��$uri��ͬ��
$host
�����е�����ͷ�ֶΣ���������е�����ͷ�����ã���Ϊ��������������ķ��������ơ�
$is_args 
���$args���ã�ֵΪ"?"������Ϊ""��
$limit_rate 
����������������������ʡ�
$nginx_version 
��ǰ���е�nginx�汾�š�
$query_string 
��$args��ͬ��
$remote_addr 
�ͻ��˵�IP��ַ��
$remote_port 
�ͻ��˵Ķ˿ڡ�
$remote_user 
�Ѿ�����Auth Basic Module��֤���û�����
$request_filename 
��ǰ����������ļ�·������root��aliasָ����URI�������ɡ�
$request_body 
���������0.7.58+�������������Ҫ��Ϣ����ʹ��proxy_pass��fastcgi_passָ���location�бȽ������塣
$request_body_file 
�ͻ�������������Ϣ����ʱ�ļ�����
$request_completion 
δ֪��
$request_method 
��������ǿͻ�������Ķ�����ͨ��ΪGET��POST��
����0.8.20��֮ǰ�İ汾�У����������Ϊmain request�еĶ����������ǰ������һ�������󣬲���ʹ�������ǰ����Ķ�����
$request_uri 
����������ڰ���һЩ�ͻ������������ԭʼURI�����޷��޸ģ���鿴$uri���Ļ���дURI��
$scheme 
HTTP��������http��https��������ʹ�ã�����

rewrite  ^(.+)$  $scheme://example.com$1  redirect;$server_addr 
��������ַ�������һ��ϵͳ���ú����ȷ�����ֵ�����Ҫ�ƿ�ϵͳ���ã��������listen��ָ����ַ����ʹ��bind������
$server_name 
���������ơ�
$server_port 
���󵽴�������Ķ˿ںš�
$server_protocol 
����ʹ�õ�Э�飬ͨ����HTTP/1.0��HTTP/1.1��
$uri 
�����еĵ�ǰURI(�����������������λ��$args)�����Բ�ͬ����������ݵ�$request_uri��ֵ��������ͨ���ڲ��ض��򣬻���ʹ��indexָ������޸ġ�
���ο��ĵ�
Original Documentation
Nginx Http Core Module
ǰ��->HTTP���ؾ���ģ�飨HTTP Upstream��




URL��дģ�飨Rewrite��

��ժҪ
���ģ������ʹ��������ʽ��дURI����PCRE�⣩�����ҿ��Ը�����ر����ض����ѡ��ͬ�����á�
������ָ����server�ֶ���ָ������ô���ڱ������locationȷ��֮ǰִ�У������ָ��ִ�к���ѡ���location������������д������ô����Ҳ��ִ�С������location��ִ�����ָ��������µ�URI����ôlocation��һ��ȷ�����µ�URI��
������ѭ���������ִ��10�Σ������Ժ�nginx������500����
��ָ��
break 
�﷨��break
Ĭ��ֵ��none
ʹ���ֶΣ�server, location, if 
��ɵ�ǰ���õĹ���ִֹͣ����������дָ�
ʾ����
if ($slow) {
  limit_rate  10k;
  break;
}if 
�﷨��if (condition) { ... } 
Ĭ��ֵ��none
ʹ���ֶΣ�server, location 
�ж�һ��������������������������Ĵ������ڵ���佫ִ�У�������ô��ϼ��̳С�
�������ж������ָ������ֵ��

��һ�����������ƣ���������ֵΪ�����ַ���""����һЩ�á�0����ʼ���ַ�����
��һ��ʹ��=����!=������ıȽ���䡣
��ʹ�÷���~*��~ģʽƥ���������ʽ��
��~Ϊ���ִ�Сд��ƥ�䡣
��~*�����ִ�Сд��ƥ�䣨firefoxƥ��FireFox����
��!~��!~*��Ϊ����ƥ��ġ���
��ʹ��-f��!-f���һ���ļ��Ƿ���ڡ�
��ʹ��-d��!-d���һ��Ŀ¼�Ƿ���ڡ�
��ʹ��-e��!-e���һ���ļ���Ŀ¼�����������Ƿ���ڡ� 
��ʹ��-x��!-x���һ���ļ��Ƿ�Ϊ��ִ���ļ��� 

������ʽ��һ���ֿ�����Բ���ţ�����֮����˳����$1-$9�����á�
ʾ�����ã�
if ($http_user_agent ~ MSIE) {
  rewrite  ^(.*)$  /msie/$1  break;
}
 
if ($http_cookie ~* "id=([^;] +)(?:;|$)" ) {
  set  $id  $1;
}
 
if ($request_method = POST ) {
  return 405;
}
 
if (!-f $request_filename) {
  break;
  proxy_pass  http://127.0.0.1;
}
 
if ($slow) {
  limit_rate  10k;
}
 
if ($invalid_referer) {
  return   403;
}
 
if ($args ~ post=140){
  rewrite ^ http://example.com/ permanent;
}���ñ���$invalid_referer��ָ��valid_referersָ����
return
�﷨��return code 
Ĭ��ֵ��none
ʹ���ֶΣ�server, location, if 
���ָ�����ִ��������䲢Ϊ�ͻ��˷���״̬���룬����ʹ�����е�ֵ��204��400��402-406��408��410, 411, 413, 416��500-504�����⣬�Ǳ�׼����444���ر����Ӳ��Ҳ������κε�ͷ����
rewrite 
�﷨��rewrite regex replacement flag 
Ĭ��ֵ��none
ʹ���ֶΣ�server, location, if 
������ص�������ʽ���ַ����޸�URI��ָ����������ļ��г��ֵ�˳��ִ�С�
ע����д����ֻƥ�����·�������Ǿ��Ե�URL�������ƥ�������������Լ�һ��if�жϣ��磺

if ($host ~* www\.(.*)) {
  set $host_without_www $1;
  rewrite ^(.*)$ http://$host_without_www$1 permanent; # $1Ϊ'/foo'��������'www.mydomain.com/foo'
}��������дָ�������ӱ�ǡ�
����滻���ַ�����http://��ͷ�����󽫱��ض��򣬲��Ҳ���ִ�ж����rewriteָ�
��ǿ��������µ�ֵ�� 
��last - �����дָ�֮��������Ӧ��URI��location��
��break - �����дָ�
��redirect - ����302��ʱ�ض�������滻�ֶ���http://��ͷ��ʹ�á�
��permanent - ����301�����ض���
ע�����һ���ض�������Եģ�û�����������֣���nginx�����ض���Ĺ�����ʹ��ƥ��server_nameָ��ġ�Host��ͷ����server_nameָ��ָ���ĵ�һ�����ƣ����ͷ��ƥ��򲻴��ڣ����û������server_name����ʹ�ñ������������������������nginxʹ�á�Host��ͷ��������server_nameʹ�á�*��ͨ������鿴http����ģ���е�server_name�������磺 
rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  last;
rewrite  ^(/download/.*)/audio/(.*)\..*$  $1/mp3/$2.ra   last;
return   403;����������ǽ������һ����Ϊ/download/��location�У�����Ҫ��last��Ǹ�Ϊbreak������nginx��ִ��10��ѭ��������500���� 
location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  break;
  rewrite  ^(/download/.*)/audio/(.*)\..*$  $1/mp3/$2.ra   break;
  return   403;
}����滻�ֶ��а�����������ô�����������������ӵ����棬Ϊ�˷�ֹ���ӣ����������һ���ַ������һ���ʺţ�

rewrite  ^/users/(.*)$  /show?user=$1?  last;ע�⣺�����ţ�{��}��������ͬʱ����������ʽ�����ÿ��У�Ϊ�˷�ֹ��ͻ��������ʽʹ�ô�������Ҫ��˫���ţ����ߵ����ţ�������Ҫ��д���µ�URL��

/photos/123456 Ϊ: 
/path/to/photos/12/1234/123456.png ��ʹ������������ʽ��ע�����ţ��� 
rewrite  "/photos/([0-9] {2})([0-9] {2})([0-9] {2})" /path/to/photos/$1/$1$2/$1$2$3.png;ͬ������дֻ��·�����в����������ǲ��������Ҫ��дһ����������URL������ʹ�����´��棺 
if ($args ^~ post=100){
  rewrite ^ http://example.com/new-address.html? permanent;
}ע��$args�������ᱻ���룬��location�����е�URI��ͬ���ο�http����ģ���е�location����
set
�﷨��set variable value 
Ĭ��ֵ��none
ʹ���ֶΣ�server, location, if 
ָ������һ��������Ϊ�丳ֵ����ֵ�������ı������������ǵ���ϡ�
�����ʹ��set����һ���µı��������ǲ���ʹ��set����$http_xxxͷ��������ֵ��������Բ鿴�������
uninitialized_variable_warn 
�﷨��uninitialized_variable_warn on|off 
Ĭ��ֵ��uninitialized_variable_warn on 
ʹ���ֶΣ�http, server, location, if 
������ر���δ��ʼ�������м�¼������־��
��ʵ�ϣ�rewriteָ���������ļ�����ʱ�Ѿ����뵽�ڲ������У��ڽ�������������ʱʹ�á�
�����������һ���򵥵Ķ�ջ�������������ָ� 
location /download/ {
  if ($forbidden) {
    return   403;
  }
  if ($slow) {
    limit_rate  10k;
  }
  rewrite  ^/(download/.*)/media/(.*)\..*$  /$1/mp3/$2.mp3  break;
�������������˳�� 
  variable $forbidden
  checking to zero
  recovery 403
  completion of entire code
  variable $slow
  checking to zero
  checkings of regular expression
  copying "/"
  copying $1
  copying "/mp3/"
  copying $2
  copying "..mpe"
  completion of regular expression
  completion of entire sequence
ע�Ⲣû�й���limit_rate�Ĵ��룬��Ϊ��û���ἰngx_http_rewrite_moduleģ�飬��if�����������"location"ָ���������ļ�����ͬ����ͬʱ���ڡ�
���$slowΪ�棬��Ӧ��if�齫��Ч�������������limit_rate��ֵΪ10k��
ָ� 
rewrite  ^/(download/.*)/media/(.*)\..*$  /$1/mp3/$2.mp3  break;������ǽ���һ��б������Բ���ţ�����Լ���ִ��˳��

rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  break;֮���˳���������£�

  checking regular expression
  copying $1
  copying "/mp3/"
  copying $2
  copying "..mpe"
  completion of regular expression
  completion of entire code
���ο��ĵ�
Original Documentation
Nginx Http Rewrite Module
ǰ��->SSIģ�飨SSI��


HTTP���ؾ���ģ�飨HTTP Upstream��


��ժҪ
���ģ��Ϊ��˵ķ������ṩ�򵥵ĸ��ؾ��⣨��ѯ��round-robin��������IP��client IP����
��������

upstream backend  {
  server backend1.example.com weight=5;
  server backend2.example.com:8080;
  server unix:/tmp/backend3;
}
 
server {
  location / {
    proxy_pass  http://backend;
  }
}��ָ��
ip_hash 
�﷨��ip_hash 
Ĭ��ֵ��none 
ʹ���ֶΣ�upstream 
���ָ����ڿͻ������ӵ�IP��ַ���ַ�����
��ϣ�Ĺؼ����ǿͻ��˵�C�������ַ��������ܽ���֤����ͻ����������Ǳ�ת����һ̨�������ϣ����������̨�����������ã���ô����
ת��������ķ������ϣ��⽫��֤ĳ���ͻ����кܴ�����������ӵ�һ̨��������
�޷���Ȩ�أ�weight����ip_hash����ʹ�����ַ����ӡ������ĳ̨�����������ã����������Ϊ��down����������:

upstream backend {
  ip_hash;
  server   backend1.example.com;
  server   backend2.example.com;
  server   backend3.example.com  down;
  server   backend4.example.com;
}server 
�﷨��server name [parameters] 
Ĭ��ֵ��none 
ʹ���ֶΣ�upstream 
ָ����˷����������ƺ�һЩ����������ʹ��������IP���˿ڣ�����unix socket�����ָ��Ϊ�����������Ƚ������ΪIP��
��weight = NUMBER - ���÷�����Ȩ�أ�Ĭ��Ϊ1��
��max_fails = NUMBER - ��һ��ʱ���ڣ����ʱ����fail_timeout���������ã��������������Ƿ����ʱ���������ʧ����������Ĭ��Ϊ1����
������Ϊ0���Թرռ�飬��Щ������proxy_next_upstream��fastcgi_next_upstream��404���󲻻�ʹmax_fails���ӣ��ж��塣
��fail_timeout = TIME - �����ʱ���ڲ�����max_fails�����ô�С��ʧ�ܳ������������������������ܲ����ã�ͬ����ָ���˷����������õ�
ʱ�䣨����һ�γ�������������֮ǰ����Ĭ��Ϊ10�룬fail_timeout��ǰ����Ӧʱ��û��ֱ�ӹ�ϵ����������ʹ��proxy_connect_timeout��
proxy_read_timeout�����ơ�
��down - ��Ƿ�������������״̬��ͨ����ip_hashһ��ʹ�á�
��backup - (0.6.7�����)������еķǱ��ݷ�������崻���æ����ʹ�ñ����������޷���ip_hashָ�����ʹ�ã���
ʾ������

upstream  backend  {
  server   backend1.example.com    weight=5;
  server   127.0.0.1:8080          max_fails=3  fail_timeout=30s;
  server   unix:/tmp/backend3;
}ע�⣺�����ֻʹ��һ̨���η�������nginx������һ�����ñ���Ϊ1����max_fails��fail_timeout�������ᱻ����
��������nginx�������ӵ����Σ����󽫶�ʧ��
�����ʹ�ö�̨���η�������
upstream 
�﷨��upstream name { ... } 
Ĭ��ֵ��none 
ʹ���ֶΣ�http 
����ֶ�����һȺ�����������Խ�����ֶη���proxy_pass��fastcgi_passָ������Ϊһ��������ʵ�壬���ǿ��Կ����Ǽ�����ͬ�˿ڵķ���������
��Ҳ������ͬʱ����TCP��Unix socket�ķ�������
����������ָ����ͬ��Ȩ�أ�Ĭ��Ϊ1��
ʾ������

upstream backend {
  server backend1.example.com weight=5;
  server 127.0.0.1:8080       max_fails=3  fail_timeout=30s;
  server unix:/tmp/backend3;
}���󽫰�����ѯ�ķ�ʽ�ַ�����˷���������ͬʱҲ�ῼ��Ȩ�ء�
����������������ÿ�η���7������5�����󽫱����͵�backend1.example.com��������̨���ֱ�õ�һ�����������һ̨�����������ã���ô
���󽫱�ת������һ̨��������ֱ�����еķ�������鶼ͨ����������еķ��������޷�ͨ����飬��ô�����ظ��ͻ������һ̨�����ķ����������Ľ����

������
�汾0.5.18�Ժ󣬿���ͨ��log_module�еı�������¼��־��

log_format timing '$remote_addr - $remote_user [$time_local]  $request '
  'upstream_response_time $upstream_response_time '
  'msec $msec request_time $request_time';
 
log_format up_head '$remote_addr - $remote_user [$time_local]  $request '
  'upstream_http_content_type $upstream_http_content_type';$upstream_addr 
ǰ�˷�������������ķ�������ַ
$upstream_cache_status 
0.8.3�汾����ֵ����Ϊ��
��MISS 
��EXPIRED - expired�����󱻴��͵���ˡ�
��UPDATING - expired������proxy/fastcgi_cache_use_stale���ڸ��£���ʹ�þɵ�Ӧ��
��STALE - expired������proxy/fastcgi_cache_use_stale����˽��õ����ڵ�Ӧ��
��HIT
$upstream_status 
ǰ�˷���������Ӧ״̬��
$upstream_response_time 
ǰ�˷�������Ӧ��ʱ�䣬��ȷ�����룬��ͬ��Ӧ���Զ��ź�ð�ŷֿ���
$upstream_http_$HEADER 
�����HTTPЭ��ͷ���磺

$upstream_http_host���ο��ĵ�
Original Documentation
Nginx Http Upstream Module


HTTP����ģ�飨HTTP Proxy��

��ժҪ
���ģ�����ת�����������ķ�������
HTTP/1.0�޷�ʹ��keepalive����˷�������Ϊÿ�����󴴽�����ɾ�����ӣ���nginxΪ���������HTTP/1.1��Ϊ��˷���������HTTP/1.0������
������Ϳ���Ϊ���������keepalive��
��������

location / {
  proxy_pass        http://localhost:8000;
  proxy_set_header  X-Real-IP  $remote_addr;
}ע�⵱ʹ��http proxyģ�飨����FastCGI�������е����������ڷ��͵���˷�����֮ǰnginx���������ǣ���ˣ��ڲ����Ӻ�˴��͵�����ʱ��
���Ľ�����ʾ���ܲ���ȷ��

��ָ��
proxy_buffer_size 
�﷨��proxy_buffer_size the_size 
Ĭ��ֵ��proxy_buffer_size 4k/8k 
ʹ���ֶΣ�http, server, location 
���ôӱ������������ȡ�ĵ�һ����Ӧ��Ļ�������С��
ͨ��������ⲿ��Ӧ���а���һ��С��Ӧ��ͷ��
Ĭ����������ֵ�Ĵ�СΪָ��proxy_buffers��ָ����һ���������Ĵ�С���������Խ�������Ϊ��С��
proxy_buffering 
�﷨��proxy_buffering on|off 
Ĭ��ֵ��proxy_buffering on 
ʹ���ֶΣ�http, server, location 
Ϊ��˵ķ���������Ӧ�𻺳塣
������û��壬nginx���豻����������ܹ��ǳ���Ĵ���Ӧ�𣬲�������뻺����������ʹ�� proxy_buffer_size��proxy_buffers������ز�����
�����Ӧ�޷�ȫ�������ڴ棬����д��Ӳ�̡�
������û��壬�Ӻ�˴�����Ӧ�����������͵��ͻ��ˡ�
nginx���Ա������������Ӧ����Ŀ������Ӧ��Ĵ�С������proxy_buffer_size��ָ����ֵ��
���ڻ��ڳ���ѯ��CometӦ����Ҫ�ر����ָ������첽��Ӧ�𽫱����岢��Comet�޷�����������
proxy_buffers 
�﷨��proxy_buffers the_number is_size; 
Ĭ��ֵ��proxy_buffers 8 4k/8k; 
ʹ���ֶΣ�http, server, location 
�������ڶ�ȡӦ�����Ա�������������Ļ�������Ŀ�ʹ�С��Ĭ�����ҲΪ��ҳ��С�����ݲ���ϵͳ�Ĳ�ͬ������4k����8k��
proxy_busy_buffers_size 
�﷨��proxy_busy_buffers_size size; 
Ĭ��ֵ��proxy_busy_buffers_size ["#proxy buffer size"] * 2; 
ʹ���ֶΣ�http, server, location, if 
δ֪��
proxy_cache 
�﷨��proxy_cache zone_name; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
����һ��������������ƣ�һ����ͬ����������ڲ�ͬ�ĵط�ʹ�á�
��0.7.48�󣬻�����ѭ��˵�"Expires", "Cache-Control: no-cache", "Cache-Control: max-age=XXX"ͷ���ֶΣ�0.7.66�汾�Ժ�
"Cache-Control:"private"��"no-store"ͷͬ������ѭ��nginx�ڻ�������в��ᴦ��"Vary"ͷ��Ϊ��ȷ��һЩ˽�����ݲ������е��û�������
��˱������� "no-cache"����"max-age=0"ͷ������proxy_cache_key�����û�ָ����������$cookie_xxx��ʹ��cookie��ֵ��Ϊproxy_cache_key
��һ���ֿ��Է�ֹ����˽�����ݣ����Կ����ڲ�ͬ��location�зֱ�ָ��proxy_cache_key��ֵ�Ա�ֿ�˽�����ݺ͹������ݡ�
����ָ��������������(buffers)�����proxy_buffers����Ϊoff�����治����Ч��
proxy_cache_key 
�﷨��proxy_cache_key line; 
Ĭ��ֵ��$scheme$proxy_host$request_uri; 
ʹ���ֶΣ�http, server, location 
ָ��ָ���˰����ڻ����еĻ���ؼ��֡�

proxy_cache_key "$host$request_uri$cookie_user";ע��Ĭ������·���������������û�а���������ؼ����У������Ϊ���վ���ڲ�ͬ��
location��ʹ�ö������������Ҫ�ڻ���ؼ����а�����������

proxy_cache_key "$scheme$host$request_uri";proxy_cache_path 
�﷨��proxy_cache_path path [levels=number] keys_zone=zone_name:zone_size [inactive=time] [max_size=size]; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http 
ָ��ָ�������·����һЩ������������������ݴ洢���ļ��У�����ʹ�ô���url�Ĺ�ϣֵ��Ϊ�ؼ������ļ�����levels����ָ���������Ŀ¼�������磺 
proxy_cache_path  /data/nginx/cache  levels=1:2   keys_zone=one:10m;�ļ��������ڣ�

/data/nginx/cache/c/29/b7f54b2df7773722d382f4809d65029c 
����ʹ�������1λ��2λ������ΪĿ¼�ṹ���� X, X:X,��X:X:X e.g.: "2", "2:2", "1:1:2"���������ֻ��������Ŀ¼��
���л��key��Ԫ���ݴ洢�ڹ�����ڴ���У����������keys_zone����ָ����
ע��ÿһ��������ڴ�ر����ǲ��ظ���·�������磺

proxy_cache_path  /data/nginx/cache/one    levels=1      keys_zone=one:10m;
proxy_cache_path  /data/nginx/cache/two    levels=2:2    keys_zone=two:100m;
proxy_cache_path  /data/nginx/cache/three  levels=1:1:2  keys_zone=three:1000m;�����inactive����ָ����ʱ���ڻ��������û
�б�������ɾ����Ĭ��inactiveΪ10���ӡ�
һ����Ϊcache manager�Ľ��̿��ƴ��̵Ļ����С����������ɾ������Ļ���Ϳ��ƻ����С����Щ����max_size�����ж��壬��Ŀǰ��
���ֵ����max_sizeָ����ֵ֮�󣬳������С������ʹ�����ݣ�LRU�滻�㷨������ɾ����
�ڴ�صĴ�С���ջ���ҳ�����ı����������ã�һ��ҳ�棨�ļ�����Ԫ���ݴ�С���ղ���ϵͳ������FreeBSD/i386��Ϊ64�ֽڣ�FreeBSD/amd64��Ϊ128�ֽڡ�
proxy_cache_path��proxy_temp_pathӦ��ʹ������ͬ���ļ�ϵͳ�ϡ�
proxy_cache_methods 
�﷨��proxy_cache_methods [GET HEAD POST]; 
Ĭ��ֵ��proxy_cache_methods GET HEAD; 
ʹ���ֶΣ�http, server, location 
GET/HEAD����װ����䣬�����޷�����GET/HEAD��ʹ��ֻʹ������������ã� 
proxy_cache_methods POST;proxy_cache_min_uses 
�﷨��proxy_cache_min_uses the_number; 
Ĭ��ֵ��proxy_cache_min_uses 1; 
ʹ���ֶΣ�http, server, location 
���ٴεĲ�ѯ��Ӧ�𽫱����棬Ĭ��1��
proxy_cache_valid 
�﷨��proxy_cache_valid reply_code [reply_code ...] time; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
Ϊ��ͬ��Ӧ�����ò�ͬ�Ļ���ʱ�䣬���磺 
  proxy_cache_valid  200 302  10m;
  proxy_cache_valid  404      1m;ΪӦ�����Ϊ200��302�����û���ʱ��Ϊ10���ӣ�404���뻺��1���ӡ�
���ֻ����ʱ�䣺
 proxy_cache_valid 5m;��ôֻ�Դ���Ϊ200, 301��302��Ӧ����л��档
ͬ������ʹ��any�����κ�Ӧ�� 
  proxy_cache_valid  200 302 10m;
  proxy_cache_valid  301 1h;
  proxy_cache_valid  any 1m;proxy_cache_use_stale 
�﷨��proxy_cache_use_stale [error|timeout|updating|invalid_header|http_500|http_502|http_503|http_504|http_404|off] [...]; 
Ĭ��ֵ��proxy_cache_use_stale off; 
ʹ���ֶΣ�http, server, location 
���ָ�����nginx��ʱ�Ӵ��������ṩһ�����ڵ���Ӧ������������proxy_next_upstreamָ�
Ϊ�˷�ֹ����ʧЧ���ڶ���߳�ͬʱ���±��ػ���ʱ���������ָ��'updating'������������ֻ֤��һ���߳�ȥ���»��棬����������̸߳���
����Ĺ������������߳�ֻ����Ӧ��ǰ�����еĹ��ڰ汾��
proxy_connect_timeout 
�﷨��proxy_connect_timeout timeout_in_seconds 

Ĭ��ֵ��proxy_connect_timeout 60 
ʹ���ֶΣ�http, server, location 
ָ��һ�����ӵ�����������ĳ�ʱʱ�䣬��Ҫע��������ʱ����ò�Ҫ����75�롣
���ʱ�䲢����ָ����������ҳ���ʱ�䣨���ʱ����proxy_read_timeout��������������ǰ�˴�����������������еģ���������һЩ״��
������û���㹻���߳�ȥ�����������󽫱�����һ�����ӳ����ӳٴ�������ô������������ڷ�����ȥ�������ӡ�
proxy_headers_hash_bucket_size 
�﷨��proxy_headers_hash_bucket_size size; 
Ĭ��ֵ��proxy_headers_hash_bucket_size 64; 
ʹ���ֶΣ�http, server, location, if 
���ù�ϣ���д洢��ÿ�����ݴ�С���ο����ͣ���
proxy_headers_hash_max_size 
�﷨��proxy_headers_hash_max_size size; 
Ĭ��ֵ��proxy_headers_hash_max_size 512; 
ʹ���ֶΣ�http, server, location, if 
���ù�ϣ������ֵ���ο����ͣ���
proxy_hide_header 
�﷨��proxy_hide_header the_header 
ʹ���ֶΣ�http, server, location 
nginx���Դӱ����������������"Date", "Server", "X-Pad"��"X-Accel-..."Ӧ�����ת�������������������һЩ������ͷ���ֶΣ�����
��������ᵽ��ͷ���ֶα��뱻ת��������ʹ��proxy_pass_headerָ����磺��Ҫ����MS-OfficeWebserver��AspNet-Version����ʹ���������ã� 
location / {
  proxy_hide_header X-AspNet-Version;
  proxy_hide_header MicrosoftOfficeWebServer;
}��ʹ��X-Accel-Redirectʱ���ָ��ǳ����á����磬�����Ҫ�ں��Ӧ�÷�������һ����Ҫ���ص��ļ�����һ������ͷ������X-Accel-Redirect��
�μ�Ϊ����ļ���ͬʱҪ��ǡ����Content-Type�����ǣ��ض����URL��ָ���������ļ����ļ�����������������������������Լ���Content-Type��
�����Ⲣ������ȷ�ģ������ͺ����˺��Ӧ�÷��������ݵ�Content-Type��Ϊ�˱���������������ʹ�����ָ� 
location / {
  proxy_pass http://backend_servers;
}
 
location /files/ {
  proxy_pass http://fileserver;
  proxy_hide_header Content-Type;
proxy_ignore_client_abort 
�﷨��proxy_ignore_client_abort [ on|off ] 
Ĭ��ֵ��proxy_ignore_client_abort off 
ʹ���ֶΣ�http, server, location 
��ֹ�ڿͻ����Լ��ն������������жϴ�������
proxy_ignore_headers 
�﷨��proxy_ignore_headers name [name ...] 
Ĭ��ֵ��none 
ʹ���ֶΣ�http, server, location 
���ָ��(0.7.54+) ��ֹ�������Դ����������Ӧ��
����ָ�����ֶ�Ϊ"X-Accel-Redirect", "X-Accel-Expires", "Expires"��"Cache-Control"�� 
proxy_intercept_errors 
�﷨��proxy_intercept_errors [ on|off ] 
Ĭ��ֵ��proxy_intercept_errors off 
ʹ���ֶΣ�http, server, location 
ʹnginx��ֹHTTPӦ�����Ϊ400���߸��ߵ�Ӧ��
Ĭ������±����������������Ӧ�𶼽������ݡ� 
�����������Ϊon��nginx�Ὣ��ֹ���ⲿ�ִ�����һ��error_pageָ�����������error_page��û��ƥ��Ĵ��������򱻴��������
���ݵĴ���Ӧ��ᰴԭ�����ݡ�
proxy_max_temp_file_size 
�﷨��proxy_max_temp_file_size size; 
Ĭ��ֵ��proxy_max_temp_file_size 1G; 
ʹ���ֶΣ�http, server, location, if 
��������������ʱʹ��һ����ʱ�ļ������ֵ������ļ��������ֵ����ͬ�������������д����̽��л��档
������ֵ����Ϊ�㣬���ֹʹ����ʱ�ļ���
proxy_method 
�﷨��proxy_method [method] 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
Ϊ��˷���������HTTP������ʽ�������㽫���ָ��ָ��ΪPOST����ô����ת������˵����󶼽�ʹ��POST����ʽ��
ʾ�����ã�
  proxy_method POST;proxy_next_upstream 
�﷨�� proxy_next_upstream [error|timeout|invalid_header|http_500|http_502|http_503|http_504|http_404|off] 
Ĭ��ֵ��proxy_next_upstream error timeout 
ʹ���ֶΣ�http, server, location 
ȷ���ں������������ת������һ����������

��error - �����ӵ�һ��������������һ�����󣬻��߶�ȡӦ��ʱ��������
��timeout - �����ӵ���������ת��������߶�ȡӦ��ʱ������ʱ��
��invalid_header - ���������ؿյĻ��ߴ����Ӧ��
��http_500 - ����������500���롣
��http_502 - ����������502���롣
��http_503 - ����������503���롣
��http_504 - ����������504���롣
��http_404 - ����������404���롣
��off - ��ֹת��������һ̨��������

ת������ֻ������û�����ݴ��ݵ��ͻ��˵Ĺ����С�
proxy_no_cache 
�﷨��proxy_no_cache variable1 variable2 ...; 
Ĭ��ֵ��None 
ʹ���ֶΣ�http, server, location 
ȷ���ں�������»����Ӧ�𽫲���ʹ�ã�ʾ����

proxy_no_cache $cookie_nocache  $arg_nocache$arg_comment;
proxy_no_cache $http_pragma     $http_authorization;���Ϊ���ַ������ߵ���0�����ʽ��ֵ����false�����磬�����������У������
������������cookie "nocache"���������Ǵ�������������͵���ˡ�
ע�⣺���Ժ�˵�Ӧ����Ȼ�п��ܷ��ϻ�����������һ�ַ������Կ��ٵĸ��»����е����ݣ��Ǿ��Ƿ���һ��ӵ�����Լ����������ͷ���ֶ�
���������磺My-Secret-Header����ô��proxy_no_cacheָ���п����������壺

proxy_no_cache $http_my_secret_header;proxy_pass 
�﷨��proxy_pass URL 
Ĭ��ֵ��no 
ʹ���ֶΣ�location, location�е�if�ֶ� 
���ָ�����ñ�����������ĵ�ַ�ͱ�ӳ���URI����ַ����ʹ����������IP�Ӷ˿ںŵ���ʽ�����磺

proxy_pass http://unix:/path/to/backend.socket:/uri/;·����unix�ؼ��ֵĺ���ָ����λ������ð��֮�䡣
����������ʱ��Nginx��location��Ӧ��URI�����滻��proxy_passָ������ָ���Ĳ��֣����������������ʹ���޷�ȷ�����ȥ�滻��
��locationͨ��������ʽָ����
����ʹ�ô����location������rewriteָ��ı�URI��ʹ��������ÿ��Ը��Ӿ�ȷ�Ĵ�������break����

location  /name/ {
  rewrite      /name/([^/] +)  /users?name=$1  break;
  proxy_pass   http://127.0.0.1;
}��Щ�����URI��û�б�ӳ�䴫�ݡ�
���⣬��Ҫ����һЩ����Ա�URI���ԺͿͻ�����ͬ�ķ�����ʽת���������Ǵ��������ʽ�����䴦���ڼ䣺

���������ϵ�б�ܽ����滻Ϊһ���� "//" -- "/"; 
��ɾ�����õĵ�ǰĿ¼��"/./" -- "/"; 
��ɾ�����õ���ǰĿ¼��"/dir /../" -- "/"��
����ڷ������ϱ�����δ���κδ������ʽ����URI����ô��proxy_passָ���б���ʹ��δָ��URI�Ĳ��֣� 
location  /some/path/ {
  proxy_pass   http://127.0.0.1;
}��ָ����ʹ�ñ�����һ�ֱȽ������������������URL����ʹ�ò����������ȫ�ֹ����URL��
����ζ�����е����ò��������㷽��Ľ���ĳ������Ҫ����������Ŀ¼���������ǽ���ת������ͬ��URL����һ��server�ֶε����ã���

location / {
  proxy_pass   http://127.0.0.1:8080/VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot;
}���������ʹ��rewrite��proxy_pass����ϣ�

location / {
  rewrite ^(.*)$ /VirtualHostBase/https/$server_name:443/some/path/VirtualHostRoot$1 break;
  proxy_pass   http://127.0.0.1:8080;
}��������������URL������д�� proxy_pass�е���βб�ܲ�û��ʵ�����塣
proxy_pass_header 
�﷨��proxy_pass_header the_name 
ʹ���ֶΣ�http, server, location 
���ָ������ΪӦ��ת��һЩ���ص�ͷ���ֶΡ�
�磺

location / {
  proxy_pass_header X-Accel-Redirect;
}proxy_pass_request_body 
�﷨��proxy_pass_request_body [ on | off ] ; 
Ĭ��ֵ��proxy_pass_request_body on; 
ʹ���ֶΣ�http, server, location 
���ð汾��0.1.29

proxy_pass_request_headers 
�﷨��proxy_pass_request_headers [ on | off ] ; 
Ĭ��ֵ��proxy_pass_request_headers on; 
ʹ���ֶΣ�http, server, location 
���ð汾��0.1.29

proxy_redirect 
�﷨��proxy_redirect [ default|off|redirect replacement ] 
Ĭ��ֵ��proxy_redirect default 
ʹ���ֶΣ�http, server, location 
�����Ҫ�޸Ĵӱ����������������Ӧ��ͷ�е�"Location"��"Refresh"�ֶΣ����������ָ�����á�
���豻�������������Location�ֶ�Ϊ�� http://localhost:8000/two/some/uri/
���ָ� 
proxy_redirect http://localhost:8000/two/ http://frontend/one/;��Location�ֶ���дΪhttp://frontend/one/some/uri/��
�ڴ�����ֶ��п��Բ�д����������

proxy_redirect http://localhost:8000/two/ /;������ʹ�÷������Ļ������ƺͶ˿ڣ���ʹ�����Է�80�˿ڡ�
���ʹ�á�default��������������location��proxy_pass������������������
���������������õ�Ч��

location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   default;
}
 
location /one/ {
  proxy_pass       http://upstream:port/two/;
  proxy_redirect   http://upstream:port/two/   /one/;
}��ָ���п���ʹ��һЩ������

proxy_redirect   http://localhost:8000/    http://$host:$server_port/;���ָ����ʱ�����ظ���

  proxy_redirect   default;
  proxy_redirect   http://localhost:8000/    /;
  proxy_redirect   http://www.example.com/   /;����off��������ֶ��н�ֹ���е�proxy_redirectָ�

  proxy_redirect   off;
  proxy_redirect   default;
  proxy_redirect   http://localhost:8000/    /;
  proxy_redirect   http://www.example.com/   /;�������ָ�����Ϊ���������������������ض��������������� 
proxy_redirect   /   /;proxy_read_timeout 
�﷨��proxy_read_timeout time 
Ĭ��ֵ��proxy_read_timeout 60 
ʹ���ֶΣ�http, server, location 
������ȡ��˷�����Ӧ��ĳ�ʱʱ�䣬������nginx���ȴ����ʱ����ȡ��һ�������Ӧ�𡣳�ʱʱ����ָ������������ֺ���״̬Ϊestablished�ĳ�ʱʱ�䡣
�����proxy_connect_timeout�����ʱ�������׽��һ̨��������ӷ������ӳ��ӳٴ�����û�����ݴ��͵ķ�������ע�ⲻҪ����ֵ����̫�ͣ�ĳЩ
����´�������������ܳ���ʱ�������ҳ��Ӧ�������統����һ����Ҫ�ܶ����ı���ʱ������Ȼ������ڲ�ͬ��location�������ò�ͬ��ֵ��

proxy_redirect_errors 
���Ƽ�ʹ�ã���ʹ�� proxy_intercept_errors��
proxy_send_lowat 
�﷨��proxy_send_lowat [ on | off ] 
Ĭ��ֵ��proxy_send_lowat off; 
ʹ���ֶΣ�http, server, location, if 
����SO_SNDLOWAT�����ָ�������FreeBSD��
proxy_send_timeout 
�﷨��proxy_send_timeout seconds 
Ĭ��ֵ��proxy_send_timeout 60 
ʹ���ֶΣ�http, server, location 
���ô��������ת������ĳ�ʱʱ�䣬ͬ��ָ����������ֺ��ʱ�䣬����������ʱ����������û������ת�����������������nginx���ر����ӡ�
proxy_set_body 
�﷨��proxy_set_body [ on | off ] 
Ĭ��ֵ��proxy_set_body off; 
ʹ���ֶΣ�http, server, location, if 
���ð汾��0.3.10��
proxy_set_header
�﷨��proxy_set_header header value 
Ĭ��ֵ�� Host and Connection
ʹ���ֶΣ�http, server, location 
���ָ���������͵������������������ͷ���¶����������һЩ�ֶΡ�
���ֵ������һ���ı��������������ǵ���ϡ�
proxy_set_header��ָ�����ֶ���û�ж���ʱ��������ϼ��ֶμ̳С�
Ĭ��ֻ�������ֶο������¶��壺

proxy_set_header Host $proxy_host;
proxy_set_header Connection Close;δ�޸ĵ�����ͷ��Host�����������·�ʽ���ͣ�

proxy_set_header Host $http_host;�����������ֶ��ڿͻ��˵�����ͷ�в����ڣ���ô���������ݵ��������������
������������ʹ��$Host����������ֵ��������ͷ�е�"Host"�ֶλ����������

proxy_set_header Host $host;���⣬���Խ�������Ķ˿������������һ�𴫵ݣ�

proxy_set_header Host $host:$proxy_port;�������Ϊ���ַ������򲻻ᴫ��ͷ������ˣ������������ý���ֹ���ʹ��gzipѹ���� 
proxy_set_header  Accept-Encoding  "";proxy_store 
�﷨��proxy_store [on | off | path] 
Ĭ��ֵ��proxy_store off 
ʹ���ֶΣ�http, server, location 
���ָ��������Щ�������ļ������洢������"on"�����ļ���alias��rootָ��ָ����Ŀ¼һ�£�����"off"���رմ洢��·�����п���ʹ�ñ�����

proxy_store   /data/www$original_uri;Ӧ��ͷ�е�"Last-Modified"�ֶ��������ļ�����޸�ʱ�䣬Ϊ���ļ��İ�ȫ������ʹ��proxy_temp_pathָ
��һ����ʱ�ļ�Ŀ¼��
���ָ��Ϊ��Щ���Ǿ���ʹ�õ��ļ���һ�ݱ��ؿ������Ӷ����ٱ�������������ء�

location /images/ {
  root                 /data/www;
  error_page           404 = /fetch$uri;
}
 
location /fetch {
  internal;
  proxy_pass           http://backend;
  proxy_store          on;
  proxy_store_access   user:rw  group:rw  all:r;
  proxy_temp_path      /data/temp;
  alias                /data/www;
}����ͨ�����ַ�ʽ��

location /images/ {
  root                 /data/www;
  error_page           404 = @fetch;
}
 
location @fetch {
  internal;
 
  proxy_pass           http://backend;
  proxy_store          on;
  proxy_store_access   user:rw  group:rw  all:r;
  proxy_temp_path      /data/temp;
 
  root                 /data/www;
}ע��proxy_store����һ�����棬��������һ������
proxy_store_access 
�﷨��proxy_store_access users:permissions [users:permission ...] 
Ĭ��ֵ��proxy_store_access user:rw 
ʹ���ֶΣ�http, server, location 
ָ�������ļ���Ŀ¼�����Ȩ�ޣ��磺

proxy_store_access  user:rw  group:rw  all:r;�����ȷָ����������е�Ȩ�ޣ���û�б�Ҫȥָ���û���Ȩ�ޣ�

proxy_store_access  group:rw  all:r;proxy_temp_file_write_size 
�﷨��proxy_temp_file_write_size size; 
Ĭ��ֵ��proxy_temp_file_write_size ["#proxy buffer size"] * 2; 
ʹ���ֶΣ�http, server, location, if 
������д��proxy_temp_pathʱ���ݵĴ�С����Ԥ��һ�����������ڴ����ļ�ʱ����̫����
proxy_temp_path
�﷨��proxy_temp_path dir-path [ level1 [ level2 [ level3 ] ; 
Ĭ��ֵ����configureʱ��--http-proxy-temp-pathָ�� 
ʹ���ֶΣ�http, server, location 
������http����ģ���е�client_body_temp_pathָ�ָ��һ����ַ������Ƚϴ�ı���������
proxy_upstream_fail_timeout
0.5.0�汾���Ƽ�ʹ�ã���ʹ��http���ؾ���ģ����serverָ���fail_timeout������
proxy_upstream_fail_timeout
0.5.0�汾���Ƽ�ʹ�ã���ʹ��http���ؾ���ģ����serverָ���max_fails������
������
��ģ���а���һЩ���ñ�������������proxy_set_headerָ�����Դ���ͷ����
$proxy_host 
���������������������˿ںš�
$proxy_host 
������������Ķ˿ںš�
$proxy_add_x_forwarded_for 
�����ͻ�������ͷ�е�"X-Forwarded-For"����$remote_addr�ö��ŷֿ������û��"X-Forwarded-For"����ͷ����$proxy_add_x_forwarded_for��
��$remote_addr��
���ο��ĵ�
Original Documentation
Nginx Http Proxy Module
ǰ��->URL��дģ�飨Rewrite��


�������������÷�����վ�ο�:http://www.pythonfan.org/docs/webserver/nginx/manual/cn/docs/
*/
