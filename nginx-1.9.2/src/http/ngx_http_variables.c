
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>


static ngx_int_t ngx_http_variable_request(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#if 0
static void ngx_http_variable_request_set(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#endif
static ngx_int_t ngx_http_variable_request_get_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void ngx_http_variable_request_set_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_header(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_headers(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep);

static ngx_int_t ngx_http_variable_unknown_header_in(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_unknown_header_out(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_cookie(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_argument(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#if (NGX_HAVE_TCP_INFO)
static ngx_int_t ngx_http_variable_tcpinfo(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#endif

static ngx_int_t ngx_http_variable_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_host(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_binary_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_proxy_protocol_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_scheme(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_https(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void ngx_http_variable_set_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_is_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_document_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_realpath_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_filename(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_server_name(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_remote_user(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_body_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_pipe(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_completion(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_body(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_body_file(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_request_time(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_sent_content_type(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_location(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_keep_alive(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_sent_transfer_encoding(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_connection_requests(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t ngx_http_variable_nginx_version(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_hostname(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_pid(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_time_iso8601(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_time_local(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

/*
 * TODO:
 *     Apache CGI: AUTH_TYPE, PATH_INFO (null), PATH_TRANSLATED
 *                 REMOTE_HOST (null), REMOTE_IDENT (null),
 *                 SERVER_SOFTWARE
 *
 *     Apache SSI: DOCUMENT_NAME, LAST_MODIFIED, USER_NAME (file owner)
 */

/*
 * the $http_host, $http_user_agent, $http_referer, and $http_via
 * variables may be handled by generic
 * ngx_http_variable_unknown_header_in(), but for performance reasons
 * they are handled using dedicated entries
 */

 /*
һ����־����
        Nginx��־��Ҫ��Ϊ���֣�������־�ʹ�����־����־������Nginx�����ļ���/etc/nginx/nginx.conf�������ã�������־������ѡ���Թرգ�Ĭ�϶��Ǵ򿪵ġ�
1��������־
        ������־��Ҫ��¼�ͻ��˷���Nginx��ÿһ�����󣬸�ʽ�����Զ��塣ͨ��������־������Եõ��û�������Դ����ת��Դ��ʹ���նˡ�ĳ��URL�������������Ϣ��Nginx�з�����־���ָ����Ҫ��������
        ��1��log_format
        log_format����������־��ʽ��Ҳ������־�ļ���ÿ����־�ĸ�ʽ���������£�
        log_format name(��ʽ����) type(��ʽ��ʽ)
        ����˵�����£�
        log_format  main  '$server_name $remote_addr - $remote_user [$time_local] "$request" '
                        '$status $uptream_status $body_bytes_sent "$http_referer" '
                        '"$http_user_agent" "$http_x_forwarded_for" '
                        '$ssl_protocol $ssl_cipher $upstream_addr $request_time $upstream_response_time';
        �����ɫ����ΪNginxĬ��ָ���ĸ�ʽ��ʽ��ÿ����ʽ�ĺ������£�
        $server_name�������������ơ�
        $remote_addr��Զ�̿ͻ��˵�IP��ַ��
        -���հף���һ����-��ռλ���������ʷԭ���»����ڡ�
        $remote_user��Զ�̿ͻ����û����ƣ����ڼ�¼����߽��������֤ʱ�ṩ�����֣����¼�ٶȵ��û���scq2099yt�����û�е�¼���ǿհס�
        [$time_local]�����ʵ�ʱ����ʱ��������18/Jul/2012:17:00:01 +0800��ʱ����Ϣ����"+0800"��ʾ����������ʱ��λ��UTC֮���8Сʱ��
        $request�������URI��HTTPЭ�飬��������PV��־��¼�������õ���Ϣ����¼�������յ�һ��ʲô��������
        $status����¼���󷵻ص�http״̬�룬����ɹ���200��
        $uptream_status��upstream״̬������ɹ���200.
        $body_bytes_sent�����͸��ͻ��˵��ļ��������ݵĴ�С������899�����Խ���־ÿ����¼�е����ֵ�ۼ������Դ��Թ��Ʒ�������������
        $http_referer����¼���ĸ�ҳ�����ӷ��ʹ����ġ� 
        $http_user_agent���ͻ����������Ϣ
        $http_x_forwarded_for���ͻ��˵���ʵip��ͨ��web���������ڷ������ĺ��棬�����Ͳ��ܻ�ȡ���ͻ���IP��ַ�ˣ�ͨ��$remote_add�õ���IP��ַ�Ƿ�������������iP��ַ����������������ת�������httpͷ��Ϣ�У���������x_forwarded_for��Ϣ�����Լ�¼ԭ�пͻ��˵�IP��ַ��ԭ���ͻ��˵�����ķ�������ַ��
        $ssl_protocol��SSLЭ��汾������TLSv1��
        $ssl_cipher�����������е��㷨������RC4-SHA�� 
        $upstream_addr��upstream�ĵ�ַ���������ṩ�����������ַ�� 
        $request_time�������������ʱ�䡣 
        $upstream_response_time����������У�upstream����Ӧʱ�䡣
        ������־��һ�����͵ļ�¼���£�
        192.168.1.102 - scq2099yt [18/Mar/2013:23:30:42 +0800] "GET /stats/awstats.pl?config=scq2099yt HTTP/1.1" 200 899 "http://192.168.1.1/pv/" "Mozilla/4.0 (compatible; MSIE 6.0; Windows XXX; Maxthon)"
        ��Ҫע����ǣ�log_format���ñ������http�ڣ������������¾�����Ϣ��
        nginx: [warn] the "log_format" directive may be used only on "http" level in /etc/nginx/nginx.conf:97
        ��2��access_log
        access_logָ������ָ����־�ļ��Ĵ��·����������־�ļ���������ʽ�ͻ����С���������£�
        access_log path(���·��) [format(�Զ�����־��ʽ����) [buffer=size | off]]
        ����˵�����£�
        access_log  logs/access.log  main;
        �����ر���־���������£�
        access_log off;
        �ܹ�ʹ��access_logָ����ֶΰ�����http��server��location��
        ��Ҫע����ǣ�Nginx�������õ��û�����������־·���д����ļ���Ȩ�ޣ����򣬻ᱨ��
        С���ɣ������Ҫ��access_log�м�¼post����Ĳ��������Բο����
        Nginx֧��Ϊÿ��locationָ��ǿ�����־��¼��ͬ�������ӿ�����ͬһʱ���������ֹһ������־�У�������Ϣ��鿴�������������
2��������־
        ������־��Ҫ��¼�ͻ��˷���Nginx����ʱ����־����ʽ��֧���Զ��塣ͨ��������־������Եõ�ϵͳĳ�������server������ƿ���ȡ���ˣ�����־�ú����ã�����Եõ��ܶ��м�ֵ����Ϣ��������־��ָ��error_log��ָ���������ʽ���£�
        error_log path(���·��) level(��־�ȼ�)
        path����ͬaccess_log��level��ʾ��־�ȼ����������£�
        [ debug | info | notice | warn | error | crit ]
        �������ң���־��ϸ�̶��𼶵ݼ�����debug����ϸ��crit���١�
        ����˵�����£�
        error_log  logs/error.log  info;
        ��Ҫע����ǣ�error_log off�����ܹرմ�����־�����ǻὫ������־��¼��һ���ļ���Ϊoff���ļ��С�
        ��ȷ�Ĺرմ�����־��¼���ܵķ������£�
        error_log /dev/null;
        �����ʾ���洢��־��·������Ϊ������Ͱ����

*/ //��־���԰�����зָ���Բο�nginx��־�����������- Awstats 
/*
��ô������Ҫ�˽�һ��nginx�б�������֯�ṹ��nginx�������Թ�Ϊ���ࣺ


����1��nginx�ڲ�����
    nginx���ı�����ngx_http_core_variables ������
    nginx��ģ�����Լ��ı���������ngx_http_browser_module ģ��� ngx_http_browsers ������
����2��nginx�������ļ��У����ǻ��õ��ܶ������
    ���磺log_format  main  '$remote_addr';  �е�$remote_addr'
    ���磺set $name hello;�е�$name 
    ���磺limit_conn_zone $binary_remote_addr zone=ip_zone:10m; �е�$binary_remote_addr
    ���磺set $memcached_key test; �е�$memcached_key


�ڼ�¼access_log������־�ļ�ʱ������ʹ��ngx_http_core_moduleģ�鴦������ʱ�������ķḻ�ı�������Ȼ����Щ������������������HTTPģ�顣
���磬��URI�е�ĳ�����������趨������ʱ����ЩHTTPģ������������ʹ������$arg_PARAMETER�����ı��������磬�����ÿ�������е�������
Ϣ��¼��access��־�ļ��У������ʹ��$limit_rate������

��2-1�г���ngx_http_core_moduleģ���ṩ����Щ������

��2-1��ngx_http_core_moduleģ���ṩ�ı���
�� �� �� �⡡��
$arg_PARAMETER HTTP������ĳ��������ֵ����/index.html?size=100��������$arg_sizeȡ��100���ֵ
$args HTTP�����е��������������磬������/index.html?_w=120&_h=120�У�$args��ʾ�ַ���_w=120&_h=120
$binary_remote_addr �����Ƹ�ʽ�Ŀͻ��˵�ַ�����磺\x0A\xE0B\x0E
$body_bytes_sent ��ʾ����ͻ��˷��͵�http��Ӧ�У����岿�ֵ��ֽ���
$content_length ��ʾ�ͻ�������ͷ���е�Content-Length�ֶ�
$content_type ��ʾ�ͻ�������ͷ���е�Content-Type�ֶ�
$cookie_COOKIE ��ʾ�ڿͻ�������ͷ���е�cookie�ֶ�
$document_root ��ʾ��ǰ������ʹ�õ�root�������ֵ
$uri ��ʾ��ǰ�����URI�������κβ���
$document_uri ��$uri������ͬ
$request_uri ��ʾ�ͻ��˷�����ԭʼ����URI���������Ĳ�����$uri��$document_uriδ�����û���ԭʼ�������ڲ��ض����������ض�����URI����$request_uri��Զ����ı䣬ʼ���ǿͻ��˵�ԭʼURI
$host ��ʾ�ͻ�������ͷ���е�Host�ֶΡ����Host�ֶβ����ڣ�����ʵ�ʴ����server���������������ƴ��档���Host�ֶ��д��ж˿ڣ���IP:PORT����ô$host��ȥ���˿ڵģ�����ֵΪIP��$host��ȫСд�ġ���Щ������http_HEADER�е�http_host��ͬ��http_hostֻ�ǡ���ʵ����ȡ��Hostͷ����Ӧ��ֵ
$hostname ��ʾNginx���ڻ��������ƣ���gethostbyname���÷��ص�ֵ��ͬ
$http_HEADER ��ʾ��ǰHTTP��������Ӧͷ����ֵ��HEADER����ȫСд�����磬��$http_host��ʾ������Hostͷ����Ӧ��ֵ
$sent_http_HEADER ��ʾ���ؿͻ��˵�HTTP��Ӧ����Ӧͷ����ֵ��HEADER����ȫСд�����磬��$sent_http_content_type��ʾ��Ӧ��Content-Typeͷ����Ӧ��ֵ
$is_args ��ʾ�����е�URI�Ƿ�������������������$is_argsֵΪ?������������������ǿ��ַ���
$limit_rate ��ʾ��ǰ���ӵ������Ƕ��٣�0��ʾ������
$nginx_version ��ʾ��ǰNginx�İ汾�ţ���1.0.14
$query_string ����URI�еĲ�������$args��ͬ��Ȼ��$query_string��ֻ���Ĳ���ı�
$remote_addr ��ʾ�ͻ��˵ĵ�ַ
$remote_port ��ʾ�ͻ�������ʹ�õĶ˿�
$remote_user ��ʾʹ��Auth Basic Moduleʱ������û���
$request_filename ��ʾ�û������е�URI����root��aliasת������ļ�·��
$request_body ��ʾHTTP�����еİ��壬�ò���ֻ��proxy_pass��fastcgi_pass��������
$request_body_file ��ʾHTTP�����еİ���洢����ʱ�ļ���
$request_completion �������Ѿ�ȫ�����ʱ����ֵΪ��ok������û����ɣ���Ҫ���ؿͻ��ˣ�����ֵΪ���ַ����������ڶϵ������������ʹ��HTTP range���ʵĲ������ļ������һ�飬��ô��ֵҲ�ǿ��ַ���
$request_method ��ʾHTTP����ķ���������GET��PUT��POST��
$scheme ��ʾHTTP scheme����������https://nginx.com/�б�ʾhttps
$server_addr ��ʾ��������ַ
$server_name ��ʾ����������
$server_port ��ʾ�������˿�
$server_protocol ��ʾ��������ͻ��˷�����Ӧ��Э�飬��HTTP/1.1��HTTP/1.0
*/ //��ش���������Բο�:http://blog.csdn.net/fangru/article/details/9163553
//Nginx�ںܶ�ģ���ж������õı��������õ����ñ�����HTT(www.111cn.net)P����ģ���У���Щ����������ʹ��������ʽ����ƥ�䡣
/*
����ģ��ngx_http_core_module�ṩ�ı���ʹ��ngx_http_core_variables ��������preconfiguration�ص����� ngx_http_variables_add_core_vars���ж��壺
*/ //http://ialloc.org/posts/2013/10/20/ngx-notes-http-variables/
static ngx_http_variable_t  ngx_http_core_variables[] = { //�ο�<��������nginx-����>  ������Щ������ֵ����Դ��ngx_http_headers_in�е�handler����

    { ngx_string("http_host"), NULL, ngx_http_variable_header, //ngx_http_get_variable��ִ��ngx_http_variable_header���Ӷ���headers_in.host��ȡֵ
      offsetof(ngx_http_request_t, headers_in.host), 0, 0 }, 

    { ngx_string("http_user_agent"), NULL, ngx_http_variable_header,
      offsetof(ngx_http_request_t, headers_in.user_agent), 0, 0 },

    { ngx_string("http_referer"), NULL, ngx_http_variable_header,
      offsetof(ngx_http_request_t, headers_in.referer), 0, 0 },

#if (NGX_HTTP_GZIP)
    { ngx_string("http_via"), NULL, ngx_http_variable_header,
      offsetof(ngx_http_request_t, headers_in.via), 0, 0 },
#endif

#if (NGX_HTTP_X_FORWARDED_FOR)
    { ngx_string("http_x_forwarded_for"), NULL, ngx_http_variable_headers,
      offsetof(ngx_http_request_t, headers_in.x_forwarded_for), 0, 0 },
#endif

    { ngx_string("http_cookie"), NULL, ngx_http_variable_cookies,
      offsetof(ngx_http_request_t, headers_in.cookies), 0, 0 },

    { ngx_string("content_length"), NULL, ngx_http_variable_content_length,
      0, 0, 0 },

    { ngx_string("content_type"), NULL, ngx_http_variable_header,
      offsetof(ngx_http_request_t, headers_in.content_type), 0, 0 },

    { ngx_string("host"), NULL, ngx_http_variable_host, 0, 0, 0 },

    { ngx_string("binary_remote_addr"), NULL,
      ngx_http_variable_binary_remote_addr, 0, 0, 0 },

    { ngx_string("remote_addr"), NULL, ngx_http_variable_remote_addr, 0, 0, 0 },

    { ngx_string("remote_port"), NULL, ngx_http_variable_remote_port, 0, 0, 0 },

    { ngx_string("proxy_protocol_addr"), NULL,
      ngx_http_variable_proxy_protocol_addr, 0, 0, 0 },

    { ngx_string("server_addr"), NULL, ngx_http_variable_server_addr, 0, 0, 0 },

    { ngx_string("server_port"), NULL, ngx_http_variable_server_port, 0, 0, 0 },

    { ngx_string("server_protocol"), NULL, ngx_http_variable_request,
      offsetof(ngx_http_request_t, http_protocol), 0, 0 },

    { ngx_string("scheme"), NULL, ngx_http_variable_scheme, 0, 0, 0 },

    { ngx_string("https"), NULL, ngx_http_variable_https, 0, 0, 0 },

    { ngx_string("request_uri"), NULL, ngx_http_variable_request,
      offsetof(ngx_http_request_t, unparsed_uri), 0, 0 },

    //$uri �����еĵ�ǰURI(�����������������λ��$args)�����Բ�ͬ����������ݵ�$request_uri��ֵ��
    //������ͨ���ڲ��ض��򣬻���ʹ��indexָ������޸ġ�
    { ngx_string("uri"), NULL, ngx_http_variable_request,
      offsetof(ngx_http_request_t, uri),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("document_uri"), NULL, ngx_http_variable_request,
      offsetof(ngx_http_request_t, uri),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request"), NULL, ngx_http_variable_request_line, 0, 0, 0 },

    { ngx_string("document_root"), NULL,
      ngx_http_variable_document_root, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("realpath_root"), NULL,
      ngx_http_variable_realpath_root, 0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("query_string"), NULL, ngx_http_variable_request,
      offsetof(ngx_http_request_t, args),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("args"),
      ngx_http_variable_set_args,
      ngx_http_variable_request,
      offsetof(ngx_http_request_t, args),
      NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("is_args"), NULL, ngx_http_variable_is_args,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request_filename"), NULL,
      ngx_http_variable_request_filename, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("server_name"), NULL, ngx_http_variable_server_name, 0, 0, 0 },

    { ngx_string("request_method"), NULL,
      ngx_http_variable_request_method, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("remote_user"), NULL, ngx_http_variable_remote_user, 0, 0, 0 },

    { ngx_string("bytes_sent"), NULL, ngx_http_variable_bytes_sent,
      0, 0, 0 },

    { ngx_string("body_bytes_sent"), NULL, ngx_http_variable_body_bytes_sent,
      0, 0, 0 },

    { ngx_string("pipe"), NULL, ngx_http_variable_pipe,
      0, 0, 0 },

    { ngx_string("request_completion"), NULL,
      ngx_http_variable_request_completion,
      0, 0, 0 },

    { ngx_string("request_body"), NULL,
      ngx_http_variable_request_body,
      0, 0, 0 },

    { ngx_string("request_body_file"), NULL,
      ngx_http_variable_request_body_file,
      0, 0, 0 },

    { ngx_string("request_length"), NULL, ngx_http_variable_request_length,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request_time"), NULL, ngx_http_variable_request_time,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("status"), NULL,
      ngx_http_variable_status, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sent_http_content_type"), NULL,
      ngx_http_variable_sent_content_type, 0, 0, 0 },

    { ngx_string("sent_http_content_length"), NULL,
      ngx_http_variable_sent_content_length, 0, 0, 0 },

    { ngx_string("sent_http_location"), NULL,
      ngx_http_variable_sent_location, 0, 0, 0 },

    { ngx_string("sent_http_last_modified"), NULL,
      ngx_http_variable_sent_last_modified, 0, 0, 0 },

    { ngx_string("sent_http_connection"), NULL,
      ngx_http_variable_sent_connection, 0, 0, 0 },

    { ngx_string("sent_http_keep_alive"), NULL,
      ngx_http_variable_sent_keep_alive, 0, 0, 0 },

    { ngx_string("sent_http_transfer_encoding"), NULL,
      ngx_http_variable_sent_transfer_encoding, 0, 0, 0 },

    { ngx_string("sent_http_cache_control"), NULL, ngx_http_variable_headers,
      offsetof(ngx_http_request_t, headers_out.cache_control), 0, 0 },

    { ngx_string("limit_rate"), ngx_http_variable_request_set_size,
      ngx_http_variable_request_get_size,
      offsetof(ngx_http_request_t, limit_rate),
      NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("connection"), NULL,
      ngx_http_variable_connection, 0, 0, 0 },

    { ngx_string("connection_requests"), NULL,
      ngx_http_variable_connection_requests, 0, 0, 0 },

    { ngx_string("nginx_version"), NULL, ngx_http_variable_nginx_version,
      0, 0, 0 },

    { ngx_string("hostname"), NULL, ngx_http_variable_hostname,
      0, 0, 0 },

    { ngx_string("pid"), NULL, ngx_http_variable_pid,
      0, 0, 0 },

    { ngx_string("msec"), NULL, ngx_http_variable_msec,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("time_iso8601"), NULL, ngx_http_variable_time_iso8601,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("time_local"), NULL, ngx_http_variable_time_local,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

#if (NGX_HAVE_TCP_INFO)
    { ngx_string("tcpinfo_rtt"), NULL, ngx_http_variable_tcpinfo,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_rttvar"), NULL, ngx_http_variable_tcpinfo,
      1, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_snd_cwnd"), NULL, ngx_http_variable_tcpinfo,
      2, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("tcpinfo_rcv_space"), NULL, ngx_http_variable_tcpinfo,
      3, NGX_HTTP_VAR_NOCACHEABLE, 0 },
#endif

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


ngx_http_variable_value_t  ngx_http_variable_null_value =
    ngx_http_variable("");
ngx_http_variable_value_t  ngx_http_variable_true_value =
    ngx_http_variable("1");


/*
����name�Ǳ���������
flags�Ǳ����ı�־��
NGX_HTTP_VAR_CHANGEABLE�������ظ����壻
NGX_HTTP_VAR_NOCACHEABLE������ֵ�����Ա����棬ÿ��ʹ�ñ�����㣻
NGX_HTTP_VAR_INDEXED��ָʾ����ֵ�洢�������У���ʹ��ngx_http_get_variable������ȡ����ʱ����ÿ�ζ�Ϊ��������ֵ�ռ䣻
NGX_HTTP_VAR_NOHASH�����ý������Ժ󣬱���������hash��������������ʱ�������ñ��������ʱ�����

�����˱����Ժ���Ҫ���ñ�����get_handler��set_handler���Լ�data������map���������õģ�
    var->get_handler = ngx_http_map_variable;
    var->data = (uintptr_t) map;
 
������get_handler��set_handler����������ʹ�ò��ԣ�
get_handler���ֵ���lazy_handle�Ĳ��ԣ�ֻ��ʹ�õ��������Ż�������ֵ��
set_handler���ֵ���active_handle�Ĳ��ԣ�ÿִ��һ�����󣬶���������ֵ��
ͬʱ����get_handler��set_handler�ص�������û������ģ�������ݱ�����ʹ���ص㣬ȷ��ʹ������ĳһ�ֻص�������һ����˵��get_handler��ͨ��һЩ��
���������õ�data������Ϊ��������get_handler����set_handler�Ĳ�����
*/ 

/*
ģ�����Լ��ı�����preconfiguration��ʱ��һ������ngx_http_add_variable���뵽variables_keys�� �������ngx_http_fastcgi_vars ngx_http_browsers 
�����ļ���set���õı���Ҳ��ͨ��ngx_http_add_variable����
//ע��:ngx_http_core_moduleģ��һ��Ҫ�������漰������ģ��ǰ���壬��Ϊvariables_keys�ռ�����ngx_http_core_moduleģ���ngx_http_variables_add_core_vars�д���
*/

//����variables_keys hash����������Ѿ�����name����ֱ�ӷ���hash�ж�Ӧ��ngx_http_variable_t��Ϣ�����򴴽�ngx_http_variable_t hash�ڵ�
//ע��:ngx_http_core_moduleģ��һ��Ҫ�������漰������ģ��ǰ���壬��Ϊvariables_keys�ռ�����ngx_http_core_moduleģ���ngx_http_variables_add_core_vars�д���
 ngx_http_variable_t * 
ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name, ngx_uint_t flags)
{
    ngx_int_t                   rc;
    ngx_uint_t                  i;
    ngx_hash_key_t             *key;
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    if (name->len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"$\"");
        return NULL;
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    key = cmcf->variables_keys->keys.elts;
    for (i = 0; i < cmcf->variables_keys->keys.nelts; i++) {
        if (name->len != key[i].key.len
            || ngx_strncasecmp(name->data, key[i].key.data, name->len) != 0) //û��ƥ�䵽name�ַ���
        {
            continue;
        }

        v = key[i].value; //ngx_http_core_variables�е�ÿһ��ngx_http_variable_t��Ա

        if (!(v->flags & NGX_HTTP_VAR_CHANGEABLE)) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "the duplicate \"%V\" variable", name);
            return NULL;
        }

        return v; //����Ѿ����ڸ�name�ַ���������ֱ�ӷ���ngx_http_core_variables�ж�Ӧ��ngx_http_variable_t��Ա
    }

    v = ngx_palloc(cf->pool, sizeof(ngx_http_variable_t));
    if (v == NULL) {
        return NULL;
    }

    v->name.len = name->len;
    v->name.data = ngx_pnalloc(cf->pool, name->len);
    if (v->name.data == NULL) {
        return NULL;
    }

    ngx_strlow(v->name.data, name->data, name->len);

    v->set_handler = NULL;
    v->get_handler = NULL;
    v->data = 0;
    v->flags = flags;
    v->index = 0;

    rc = ngx_hash_add_key(cmcf->variables_keys, &v->name, v, 0);

    if (rc == NGX_ERROR) {
        return NULL;
    }

    if (rc == NGX_BUSY) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "conflicting variable name \"%V\"", name);
        return NULL;
    }

    return v;
}

/*
�ǿ����ж��߿��ܻ��������ˡ����ĳ������������NGX_HTTP_VAR_NOHASH�ı�־������ʹ�ñ��������ʣ���Ӧ����ô�����أ�


ngx_http_get_variable_index��ѱ�������һ��array�����ر�����array�е��±꣬����ģ�鱣��������±꣬��ͨ��
ngx_http_get_indexed_variableֱ�ӵõ�����ֵ
*/ 

/*�ڽ��������ļ��е�ʱ������set���ñ�����ʱ�򣬻�Ѷ�Ӧ�ı�����ӵ�variables���飬�����ڽ��������ļ������л��߽��������ļ�ǰ����û�а�ģ��
���Դ��ı�������variables���飬��variables_keys����洢��ģ�����Զ�����Լ�set���������ļ��н������ı���
*/
ngx_int_t //��cmcf->variables�в����Ƿ��ж�Ӧ��name������ȡ��variables�����е��±�
ngx_http_get_variable_index(ngx_conf_t *cf, ngx_str_t *name)
{
    ngx_uint_t                  i;
    ngx_http_variable_t        *v;         
    ngx_http_core_main_conf_t  *cmcf;

    if (name->len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"$\"");
        return NGX_ERROR;
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    v = cmcf->variables.elts;

    if (v == NULL) {
        if (ngx_array_init(&cmcf->variables, cf->pool, 4, 
                           sizeof(ngx_http_variable_t))
            != NGX_OK)
        {
            return NGX_ERROR;
        }

    } else {
        for (i = 0; i < cmcf->variables.nelts; i++) {
            if (name->len != v[i].name.len
                || ngx_strncasecmp(name->data, v[i].name.data, name->len) != 0)
            {
                continue;
            }

            return i; //���������е��±�
        }
    }

    v = ngx_array_push(&cmcf->variables);
    if (v == NULL) {
        return NGX_ERROR;
    }

    v->name.len = name->len;
    v->name.data = ngx_pnalloc(cf->pool, name->len);
    if (v->name.data == NULL) {
        return NGX_ERROR;
    }

    ngx_strlow(v->name.data, name->data, name->len);

    v->set_handler = NULL;
    v->get_handler = NULL;
    v->data = 0;
    v->flags = 0;
    v->index = cmcf->variables.nelts - 1; //�����cmcf->variables�����е�λ��

    return v->index; //���������е��±�  ngx_http_variable_t->index
}

/*
ngx_http_get_variable_index��ѱ�������һ��array�����ر�����array�е��±꣬����ģ�鱣��������±꣬��ͨ��
ngx_http_get_indexed_variableֱ�ӵõ�����ֵ
*/
ngx_http_variable_value_t *
ngx_http_get_indexed_variable(ngx_http_request_t *r, ngx_uint_t index)
{
    ngx_http_variable_t        *v;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    if (cmcf->variables.nelts <= index) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "unknown variable index: %ui", index);
        return NULL;
    }

    if (r->variables[index].not_found || r->variables[index].valid) {
        return &r->variables[index];
    }

    v = cmcf->variables.elts;

    //ngx_http_rewrite_set������Ϊngx_http_rewrite_var  args��get_handler��ngx_http_variable_request ngx_http_variables_init_vars������"http_"�ȵ�get_handler
    if (v[index].get_handler(r, &r->variables[index], v[index].data)
        == NGX_OK)
    {
        if (v[index].flags & NGX_HTTP_VAR_NOCACHEABLE) {
            r->variables[index].no_cacheable = 1;
        }

        return &r->variables[index];
    }

    r->variables[index].valid = 0;
    r->variables[index].not_found = 1;

    return NULL;
}


ngx_http_variable_value_t *
ngx_http_get_flushed_variable(ngx_http_request_t *r, ngx_uint_t index)
{
    ngx_http_variable_value_t  *v;

    v = &r->variables[index];

    if (v->valid || v->not_found) {
        if (!v->no_cacheable) {
            return v;
        }

        v->valid = 0;
        v->not_found = 0;
    }

    return ngx_http_get_indexed_variable(r, index);
}

/*
NGX HTTP_VAR_INDEXED��NGXHTTP_VARNOHASH������cmcf->variables_hash�Լ�ȡֵ����ngx_http_get_variable�ȣ����Ƕ���ΪSSIģ��ʵ�ֶ���Ƶ�
*/
ngx_http_variable_value_t *
ngx_http_get_variable(ngx_http_request_t *r, ngx_str_t *name, ngx_uint_t key)
{
    ngx_http_variable_t        *v;
    ngx_http_variable_value_t  *vv;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    v = ngx_hash_find(&cmcf->variables_hash, key, name->data, name->len);

    if (v) {
        if (v->flags & NGX_HTTP_VAR_INDEXED) {
            return ngx_http_get_flushed_variable(r, v->index);

        } else {

            vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));

            if (vv && v->get_handler(r, vv, v->data) == NGX_OK) {
                return vv;
            }

            return NULL;
        }
    }

    vv = ngx_palloc(r->pool, sizeof(ngx_http_variable_value_t));
    if (vv == NULL) {
        return NULL;
    }

    if (ngx_strncmp(name->data, "http_", 5) == 0) {

        if (ngx_http_variable_unknown_header_in(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (ngx_strncmp(name->data, "sent_http_", 10) == 0) {

        if (ngx_http_variable_unknown_header_out(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (ngx_strncmp(name->data, "upstream_http_", 14) == 0) {

        if (ngx_http_upstream_header_variable(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (ngx_strncmp(name->data, "cookie_", 7) == 0) {

        if (ngx_http_variable_cookie(r, vv, (uintptr_t) name) == NGX_OK) {
            return vv;
        }

        return NULL;
    }

    if (ngx_strncmp(name->data, "upstream_cookie_", 16) == 0) {

        if (ngx_http_upstream_cookie_variable(r, vv, (uintptr_t) name)
            == NGX_OK)
        {
            return vv;
        }

        return NULL;
    }

    if (ngx_strncmp(name->data, "arg_", 4) == 0) {

        if (ngx_http_variable_argument(r, vv, (uintptr_t) name) == NGX_OK) {
            return vv;
        }

        return NULL;
    }

    vv->not_found = 1;

    return vv;
}


static ngx_int_t
ngx_http_variable_request(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t  *s;

    s = (ngx_str_t *) ((char *) r + data);

    if (s->data) {
        v->len = s->len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = s->data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


#if 0

static void
ngx_http_variable_request_set(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  *s;

    s = (ngx_str_t *) ((char *) r + data);

    s->len = v->len;
    s->data = v->data;
}

#endif


static ngx_int_t
ngx_http_variable_request_get_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t  *sp;

    sp = (size_t *) ((char *) r + data);

    v->data = ngx_pnalloc(r->pool, NGX_SIZE_T_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(v->data, "%uz", *sp) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static void
ngx_http_variable_request_set_size(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ssize_t    s, *sp;
    ngx_str_t  val;

    val.len = v->len;
    val.data = v->data;

    s = ngx_parse_size(&val);

    if (s == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid size \"%V\"", &val);
        return;
    }

    sp = (ssize_t *) ((char *) r + data);

    *sp = s;

    return;
}


static ngx_int_t
ngx_http_variable_header(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_table_elt_t  *h;

    h = *(ngx_table_elt_t **) ((char *) r + data);

    if (h) {
        v->len = h->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = h->value.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_headers_internal(r, v, data, ';');
}


static ngx_int_t
ngx_http_variable_headers(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_headers_internal(r, v, data, ',');
}


static ngx_int_t
ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep)
{
    size_t             len;
    u_char            *p, *end;
    ngx_uint_t         i, n;
    ngx_array_t       *a;
    ngx_table_elt_t  **h;

    a = (ngx_array_t *) ((char *) r + data);

    n = a->nelts;
    h = a->elts;

    len = 0;

    for (i = 0; i < n; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        len += h[i]->value.len + 2;
    }

    if (len == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len -= 2;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (n == 1) {
        v->len = (*h)->value.len;
        v->data = (*h)->value.data;

        return NGX_OK;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = len;
    v->data = p;

    end = p + len;

    for (i = 0; /* void */ ; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        p = ngx_copy(p, h[i]->value.data, h[i]->value.len);

        if (p == end) {
            break;
        }

        *p++ = sep; *p++ = ' ';
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_unknown_header_in(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data,
                                            &r->headers_in.headers.part,
                                            sizeof("http_") - 1);
}


static ngx_int_t
ngx_http_variable_unknown_header_out(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data,
                                            &r->headers_out.headers.part,
                                            sizeof("sent_http_") - 1);
}


ngx_int_t
ngx_http_variable_unknown_header(ngx_http_variable_value_t *v, ngx_str_t *var,
    ngx_list_part_t *part, size_t prefix)
{
    u_char            ch;
    ngx_uint_t        i, n;
    ngx_table_elt_t  *header;

    header = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            header = part->elts;
            i = 0;
        }

        if (header[i].hash == 0) {
            continue;
        }

        for (n = 0; n + prefix < var->len && n < header[i].key.len; n++) {
            ch = header[i].key.data[n];

            if (ch >= 'A' && ch <= 'Z') {
                ch |= 0x20;

            } else if (ch == '-') {
                ch = '_';
            }

            if (var->data[n + prefix] != ch) {
                break;
            }
        }

        if (n + prefix == var->len && n == header[i].key.len) {
            v->len = header[i].value.len;
            v->valid = 1;
            v->no_cacheable = 0;
            v->not_found = 0;
            v->data = header[i].value.data;

            return NGX_OK;
        }
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p, *s;

    s = r->request_line.data;

    if (s == NULL) {
        s = r->request_start;

        if (s == NULL) {
            v->not_found = 1;
            return NGX_OK;
        }

        for (p = s; p < r->header_in->last; p++) {
            if (*p == CR || *p == LF) {
                break;
            }
        }

        r->request_line.len = p - s;
        r->request_line.data = s;
    }

    v->len = r->request_line.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_cookie(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t *name = (ngx_str_t *) data;

    ngx_str_t  cookie, s;

    s.len = name->len - (sizeof("cookie_") - 1);
    s.data = name->data + sizeof("cookie_") - 1;

    if (ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &s, &cookie)
        == NGX_DECLINED)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = cookie.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cookie.data;

    return NGX_OK;
}

/*��������GET /download/nginx-1.9.2.rar?st=xhWL03HbtjrojpEAfiD6Mw&e=1452139931 HTTP/1.1��st��e�γɱ���$arg_st #arg_e��value�ֱ�
ΪxhWL03HbtjrojpEAfiD6Mw 1452139931��$arg_st=xhWL03HbtjrojpEAfiD6Mw��#arg_e=1452139931 */
static ngx_int_t
ngx_http_variable_argument(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t *name = (ngx_str_t *) data;

    u_char     *arg;
    size_t      len;
    ngx_str_t   value;

    len = name->len - (sizeof("arg_") - 1);
    arg = name->data + sizeof("arg_") - 1;

    if (ngx_http_arg(r, arg, len, &value) != NGX_OK) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = value.data;
    v->len = value.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


#if (NGX_HAVE_TCP_INFO)

static ngx_int_t
ngx_http_variable_tcpinfo(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    struct tcp_info  ti;
    socklen_t        len;
    uint32_t         value;

    len = sizeof(struct tcp_info);
    if (getsockopt(r->connection->fd, IPPROTO_TCP, TCP_INFO, &ti, &len) == -1) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = ngx_pnalloc(r->pool, NGX_INT32_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (data) {
    case 0:
        value = ti.tcpi_rtt;
        break;

    case 1:
        value = ti.tcpi_rttvar;
        break;

    case 2:
        value = ti.tcpi_snd_cwnd;
        break;

    case 3:
        value = ti.tcpi_rcv_space;
        break;

    /* suppress warning */
    default:
        value = 0;
        break;
    }

    v->len = ngx_sprintf(v->data, "%uD", value) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_variable_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_in.content_length) {
        v->len = r->headers_in.content_length->value.len;
        v->data = r->headers_in.content_length->value.data;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

    } else if (r->reading_body) {
        v->not_found = 1;
        v->no_cacheable = 1;

    } else if (r->headers_in.content_length_n >= 0) {
        p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_sprintf(p, "%O", r->headers_in.content_length_n) - p;
        v->data = p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_host(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_http_core_srv_conf_t  *cscf;

    if (r->headers_in.server.len) {
        v->len = r->headers_in.server.len;
        v->data = r->headers_in.server.data;

    } else {
        cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

        v->len = cscf->server_name.len;
        v->data = cscf->server_name.data;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_binary_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    switch (r->connection->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;

        v->len = sizeof(struct in6_addr);
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = sin6->sin6_addr.s6_addr;

        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->sockaddr;

        v->len = sizeof(in_addr_t);
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) &sin->sin_addr;

        break;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = r->connection->addr_text.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->connection->addr_text.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            port;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (r->connection->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;
        port = ntohs(sin6->sin6_port);
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        port = 0;
        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->sockaddr;
        port = ntohs(sin->sin_port);
        break;
    }

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_proxy_protocol_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = r->connection->proxy_protocol_addr.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->connection->proxy_protocol_addr.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  s;
    u_char     addr[NGX_SOCKADDR_STRLEN];

    s.len = NGX_SOCKADDR_STRLEN;
    s.data = addr;

    if (ngx_connection_local_sockaddr(r->connection, &s, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    s.data = ngx_pnalloc(r->pool, s.len);
    if (s.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(s.data, addr, s.len);

    v->len = s.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t            port;
    struct sockaddr_in   *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6  *sin6;
#endif

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ngx_connection_local_sockaddr(r->connection, NULL, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (r->connection->local_sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->local_sockaddr;
        port = ntohs(sin6->sin6_port);
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        port = 0;
        break;
#endif

    default: /* AF_INET */
        sin = (struct sockaddr_in *) r->connection->local_sockaddr;
        port = ntohs(sin->sin_port);
        break;
    }

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_scheme(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
#if (NGX_HTTP_SSL)

    if (r->connection->ssl) {
        v->len = sizeof("https") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "https";

        return NGX_OK;
    }

#endif

    v->len = sizeof("http") - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) "http";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_https(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
#if (NGX_HTTP_SSL)

    if (r->connection->ssl) {
        v->len = sizeof("on") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "on";

        return NGX_OK;
    }

#endif

    *v = ngx_http_variable_null_value;

    return NGX_OK;
}


static void
ngx_http_variable_set_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    r->args.len = v->len;
    r->args.data = v->data;
    r->valid_unparsed_uri = 0;
}


static ngx_int_t
ngx_http_variable_is_args(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->args.len == 0) {
        v->len = 0;
        v->data = NULL;
        return NGX_OK;
    }

    v->len = 1;
    v->data = (u_char *) "?";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_document_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t                  path;
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->root_lengths == NULL) {
        v->len = clcf->root.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = clcf->root.data;

    } else {
        if (ngx_http_script_run(r, &path, clcf->root_lengths->elts, 0,
                                clcf->root_values->elts)
            == NULL)
        {
            return NGX_ERROR;
        }

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, &path)
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        v->len = path.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = path.data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_realpath_root(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                    *real;
    size_t                     len;
    ngx_str_t                  path;
    ngx_http_core_loc_conf_t  *clcf;
#if (NGX_HAVE_MAX_PATH)
    u_char                     buffer[NGX_MAX_PATH];
#endif

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->root_lengths == NULL) {
        path = clcf->root;

    } else {
        if (ngx_http_script_run(r, &path, clcf->root_lengths->elts, 1,
                                clcf->root_values->elts)
            == NULL)
        {
            return NGX_ERROR;
        }

        path.data[path.len - 1] = '\0';

        if (ngx_get_full_name(r->pool, (ngx_str_t *) &ngx_cycle->prefix, &path)
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

#if (NGX_HAVE_MAX_PATH)
    real = buffer;
#else
    real = NULL;
#endif

    real = ngx_realpath(path.data, real);

    if (real == NULL) {
        ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno,
                      ngx_realpath_n " \"%s\" failed", path.data);
        return NGX_ERROR;
    }

    len = ngx_strlen(real);

    v->data = ngx_pnalloc(r->pool, len);
    if (v->data == NULL) {
#if !(NGX_HAVE_MAX_PATH)
        ngx_free(real);
#endif
        return NGX_ERROR;
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    ngx_memcpy(v->data, real, len);

#if !(NGX_HAVE_MAX_PATH)
    ngx_free(real);
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_filename(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t     root;
    ngx_str_t  path;

    if (ngx_http_map_uri_to_path(r, &path, &root, 0) == NULL) {
        return NGX_ERROR;
    }

    /* ngx_http_map_uri_to_path() allocates memory for terminating '\0' */

    v->len = path.len - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = path.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_server_name(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_core_srv_conf_t  *cscf;

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

    v->len = cscf->server_name.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cscf->server_name.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->main->method_name.data) {
        v->len = r->main->method_name.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->main->method_name.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_user(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_int_t  rc;

    rc = ngx_http_auth_basic_user(r);

    if (rc == NGX_DECLINED) {
        v->not_found = 1;
        return NGX_OK;
    }

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    v->len = r->headers_in.user.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->headers_in.user.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", r->connection->sent) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_body_bytes_sent(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    off_t    sent;
    u_char  *p;

    sent = r->connection->sent - r->header_size;

    if (sent < 0) {
        sent = 0;
    }

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", sent) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_pipe(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->data = (u_char *) (r->pipeline ? "p" : ".");
    v->len = 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  status;

    v->data = ngx_pnalloc(r->pool, NGX_INT_T_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    if (r->err_status) {
        status = r->err_status;

    } else if (r->headers_out.status) {
        status = r->headers_out.status;

    } else if (r->http_version == NGX_HTTP_VERSION_9) {
        status = 9;

    } else {
        status = 0;
    }

    v->len = ngx_sprintf(v->data, "%03ui", status) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_content_type(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->headers_out.content_type.len) {
        v->len = r->headers_out.content_type.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.content_type.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_content_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_out.content_length) {
        v->len = r->headers_out.content_length->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.content_length->value.data;

        return NGX_OK;
    }

    if (r->headers_out.content_length_n >= 0) {
        p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_sprintf(p, "%O", r->headers_out.content_length_n) - p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = p;

        return NGX_OK;
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_location(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  name;

    if (r->headers_out.location) {
        v->len = r->headers_out.location->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.location->value.data;

        return NGX_OK;
    }

    ngx_str_set(&name, "sent_http_location");

    return ngx_http_variable_unknown_header(v, &name,
                                            &r->headers_out.headers.part,
                                            sizeof("sent_http_") - 1);
}


static ngx_int_t
ngx_http_variable_sent_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->headers_out.last_modified) {
        v->len = r->headers_out.last_modified->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->headers_out.last_modified->value.data;

        return NGX_OK;
    }

    if (r->headers_out.last_modified_time >= 0) {
        p = ngx_pnalloc(r->pool, sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1);
        if (p == NULL) {
            return NGX_ERROR;
        }

        v->len = ngx_http_time(p, r->headers_out.last_modified_time) - p;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = p;

        return NGX_OK;
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    size_t   len;
    char    *p;

    if (r->headers_out.status == NGX_HTTP_SWITCHING_PROTOCOLS) {
        len = sizeof("upgrade") - 1;
        p = "upgrade";

    } else if (r->keepalive) {
        len = sizeof("keep-alive") - 1;
        p = "keep-alive";

    } else {
        len = sizeof("close") - 1;
        p = "close";
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_keep_alive(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                    *p;
    ngx_http_core_loc_conf_t  *clcf;

    if (r->keepalive) {
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        if (clcf->keepalive_header) {

            p = ngx_pnalloc(r->pool, sizeof("timeout=") - 1 + NGX_TIME_T_LEN);
            if (p == NULL) {
                return NGX_ERROR;
            }

            v->len = ngx_sprintf(p, "timeout=%T", clcf->keepalive_header) - p;
            v->valid = 1;
            v->no_cacheable = 0;
            v->not_found = 0;
            v->data = p;

            return NGX_OK;
        }
    }

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_sent_transfer_encoding(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->chunked) {
        v->len = sizeof("chunked") - 1;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "chunked";

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_completion(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->request_complete) {
        v->len = 2;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = (u_char *) "OK";

        return NGX_OK;
    }

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) "";

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_body(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char       *p;
    size_t        len;
    ngx_buf_t    *buf;
    ngx_chain_t  *cl;

    if (r->request_body == NULL
        || r->request_body->bufs == NULL
        || r->request_body->temp_file)
    {
        v->not_found = 1;

        return NGX_OK;
    }

    cl = r->request_body->bufs;
    buf = cl->buf;

    if (cl->next == NULL) {
        v->len = buf->last - buf->pos;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = buf->pos;

        return NGX_OK;
    }

    len = buf->last - buf->pos;
    cl = cl->next;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        len += buf->last - buf->pos;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;
    cl = r->request_body->bufs;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        p = ngx_cpymem(p, buf->pos, buf->last - buf->pos);
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_body_file(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->request_body == NULL || r->request_body->temp_file == NULL) {
        v->not_found = 1;

        return NGX_OK;
    }

    v->len = r->request_body->temp_file->file.name.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->request_body->temp_file->file.name.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_length(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_OFF_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%O", r->request_length) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request_time(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char          *p;
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    v->len = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_connection(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_ATOMIC_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%uA", r->connection->number) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_connection_requests(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_INT_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%ui", r->connection->requests) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_nginx_version(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = sizeof(NGINX_VERSION) - 1;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = (u_char *) NGINX_VERSION;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_hostname(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = ngx_cycle->hostname.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = ngx_cycle->hostname.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_pid(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, NGX_INT64_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_sprintf(p, "%P", ngx_pid) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T.%03M", tp->sec, tp->msec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_time_iso8601(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, ngx_cached_http_log_iso8601.len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(p, ngx_cached_http_log_iso8601.data,
               ngx_cached_http_log_iso8601.len);

    v->len = ngx_cached_http_log_iso8601.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_time_local(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    p = ngx_pnalloc(r->pool, ngx_cached_http_log_time.len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(p, ngx_cached_http_log_time.data, ngx_cached_http_log_time.len);

    v->len = ngx_cached_http_log_time.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


void *
ngx_http_map_find(ngx_http_request_t *r, ngx_http_map_t *map, ngx_str_t *match)
{
    void        *value;
    u_char      *low;
    size_t       len;
    ngx_uint_t   key;

    len = match->len;

    if (len) {
        low = ngx_pnalloc(r->pool, len);
        if (low == NULL) {
            return NULL;
        }

    } else {
        low = NULL;
    }

    key = ngx_hash_strlow(low, match->data, len);

    value = ngx_hash_find_combined(&map->hash, key, low, len);
    if (value) {
        return value;
    }

#if (NGX_PCRE)

    if (len && map->nregex) {
        ngx_int_t              n;
        ngx_uint_t             i;
        ngx_http_map_regex_t  *reg;

        reg = map->regex;

        for (i = 0; i < map->nregex; i++) {

            n = ngx_http_regex_exec(r, reg[i].regex, match);

            if (n == NGX_OK) {
                return reg[i].value;
            }

            if (n == NGX_DECLINED) {
                continue;
            }

            /* NGX_ERROR */

            return NULL;
        }
    }

#endif

    return NULL;
}


#if (NGX_PCRE)

static ngx_int_t
ngx_http_variable_not_found(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    v->not_found = 1;
    return NGX_OK;
}

/*
1. ����ngx_regex_compile����������ʽ�����ҵõ���ص����ݣ�������ģʽ��Ŀ��������ģʽ��Ŀ���б�ȡ�
2.	��������ʽ��Ϣ�洢��ngx_http_regex_t���棬��������������ģʽ��Ŀ
3. 	��������ģʽ ���뵽cmcf->variables_keys��cmcf->variables�С��Ա�����ͨ�����ֲ��ұ���ֵ��*/
ngx_http_regex_t *
ngx_http_regex_compile(ngx_conf_t *cf, ngx_regex_compile_t *rc)
{
    u_char                     *p;
    size_t                      size;
    ngx_str_t                   name;
    ngx_uint_t                  i, n;
    ngx_http_variable_t        *v;
    ngx_http_regex_t           *re;
    ngx_http_regex_variable_t  *rv;
    ngx_http_core_main_conf_t  *cmcf;

    rc->pool = cf->pool;

    if (ngx_regex_compile(rc) != NGX_OK) { //����pcre_compile������������ؽ������rc->regex = re;
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V", &rc->err);
        return NULL;
    }

    re = ngx_pcalloc(cf->pool, sizeof(ngx_http_regex_t));
    if (re == NULL) {
        return NULL;
    }

    re->regex = rc->regex;
    re->ncaptures = rc->captures;//�õ�$2,$3�ж��ٸ���pcre_fullinfo(re, NULL, PCRE_INFO_CAPTURECOUNT, &rc->captures);
    re->name = rc->pattern;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    cmcf->ncaptures = ngx_max(cmcf->ncaptures, re->ncaptures); 

    n = (ngx_uint_t) rc->named_captures;//��������ģʽ: pcre_fullinfo(re, NULL, PCRE_INFO_NAMECOUNT, &rc->named_captures);

    if (n == 0) { //���������ģʽ(?<name>exp)û��
        return re;
    }

    rv = ngx_palloc(rc->pool, n * sizeof(ngx_http_regex_variable_t));
    if (rv == NULL) {//Ϊÿһ��������������ռ䵥���洢��
        return NULL;
    }

    re->variables = rv;//��¼������������ģʽ
    re->nvariables = n;

    size = rc->name_size; 
    p = rc->names; //�����������ģʽ�Ķ�ά�������¼����ģʽ�����ƣ��Լ�������ģʽ�е��±�.
    //names����ô�õ���: pcre_fullinfo(re, NULL, PCRE_INFO_NAMETABLE, &rc->names);
    //һ��������ģʽ�Ķ�ά��洢��p��λ�á�ÿһ����һ������ģʽ��ÿ�е��ֽ���Ϊname_size
    //�ο�http://blog.csdn.net/kofiory/article/details/5829697

    ///ngx_http_regex_compile�з���ռ�,�������   ngx_http_regex_exec��ȡ��Ӧ������ֵ
    for (i = 0; i < n; i++) { //����ÿһ��named_captures��
        rv[i].capture = 2 * ((p[0] << 8) + p[1]); //��һ���Լ�����8λ+�ڶ����ֽڵ������������ģʽ������ģʽ�е��±ꡣ

        name.data = &p[2]; //�������ֽڿ�ʼ�������������֡�
        name.len = ngx_strlen(name.data); //���ֳ���
        
        ///�����������ģʽ���뵽cmcf->variables_keys��
        v = ngx_http_add_variable(cf, &name, NGX_HTTP_VAR_CHANGEABLE);
        if (v == NULL) {
            return NULL;
        }

        //Ȼ���cmcf->variables�в������������Ӧ���±��Ƕ��٣����û�У���Ȼ�����ӵ�����ȥ��
        rv[i].index = ngx_http_get_variable_index(cf, &name);
        if (rv[i].index == NGX_ERROR) {
            return NULL;
        }

        //��ʼ��Ϊ�պ���
        v->get_handler = ngx_http_variable_not_found;

        p += size;
    }

    return re;
}

//���Ѿ������regex ��e->line,Ҳ����sȥƥ�䣬�����Ƿ�ƥ��ɹ������ƥ��ɹ�����������r->variables���桢
ngx_int_t
ngx_http_regex_exec(ngx_http_request_t *r, ngx_http_regex_t *re, ngx_str_t *s)
{
    ngx_int_t                   rc, index;
    ngx_uint_t                  i, n, len;
    ngx_http_variable_value_t  *vv;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    if (re->ncaptures) { //����ncaptures��С�������ڴ����ڴ������ƥ��Ľ����
        len = cmcf->ncaptures;

        if (r->captures == NULL) {
            r->captures = ngx_palloc(r->pool, len * sizeof(int));
            if (r->captures == NULL) {
                return NGX_ERROR;
            }
        }

    } else {
        len = 0;
    }

    //����������ʽ��ָ�����ַ����н��в��Һ�ƥ��,�����ƥ��Ľ��.
    rc = ngx_regex_exec(re->regex, s, r->captures, len); //len��ȡֵ��http://www.rosoo.net/a/201004/9082.html

    if (rc == NGX_REGEX_NO_MATCHED) {
        return NGX_DECLINED;
    }

    if (rc < 0) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      ngx_regex_exec_n " failed: %i on \"%V\" using \"%V\"",
                      rc, s, &re->name);
        return NGX_ERROR;
    }

    ////ngx_http_regex_compile�з���ռ�,�������   ngx_http_regex_exec��ȡ��Ӧ������ֵ
    for (i = 0; i < re->nvariables; i++) { //��������ʽ�е�������ģʽ�ı�����ȡ��
        /*
           ����:rewrite   ^(?<name1>/download/.*)/media/(?<name2>.*)\..*$    $name1/mp3/$name2.mp3  break;
           ���uriΪ/download/aaa/media/bbb.com����name1����Ϊ/download/aaa  name2����Ϊbbb 
           �����vv->len��vv->data�ֱ�ָ��"/download/aaa"�ַ������ȣ����ַ���"/download/aaa"
          */
        n = re->variables[i].capture;
        index = re->variables[i].index;
        vv = &r->variables[index];

        //��s��re�Ķ�Ӧ��ϵ�У���s�л�ȡre�еı�����ֵ
        vv->len = r->captures[n + 1] - r->captures[n];
        vv->valid = 1;
        vv->no_cacheable = 0;
        vv->not_found = 0;
        vv->data = &s->data[r->captures[n]];
        
#if (NGX_DEBUG)
        {
        ngx_http_variable_t  *v;

        v = cmcf->variables.elts;

        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http regex set $%V to \"%*s\"",
                       &v[index].name, vv->len, vv->data);
        }
#endif
    }

    /*  
     ����������ʽ���re.name= ^(/download/.*)/media/(.*)/tt/(.*)$��  s=/download/aa/media/bdb/tt/ad,�����ǻ�ƥ�䣬ͬʱƥ���
     ��������3�����򷵻�ֵΪ3+1=4,�����ƥ���򷵻�-1

     ����*2����Ϊ��ȡǰ�������е�3��������Ӧ��ֵ��Ҫ�ɶ�ʹ��r->captures���ο�ngx_http_script_copy_capture_code��
     */
    r->ncaptures = rc * 2;  
    r->captures_data = s->data;
    
    return NGX_OK;
}

#endif

//ngx_http_variables_add_core_vars��ngx_http_core_variables�еĸ��ֱ�����Ϣ��ŵ�cmcf->variables_keys��
ngx_int_t
ngx_http_variables_add_core_vars(ngx_conf_t *cf)
{
    ngx_int_t                   rc;
    ngx_http_variable_t        *cv, *v;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    cmcf->variables_keys = ngx_pcalloc(cf->temp_pool,
                                       sizeof(ngx_hash_keys_arrays_t));
    if (cmcf->variables_keys == NULL) {
        return NGX_ERROR;
    }

    cmcf->variables_keys->pool = cf->pool;
    cmcf->variables_keys->temp_pool = cf->pool;

    if (ngx_hash_keys_array_init(cmcf->variables_keys, NGX_HASH_SMALL)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (cv = ngx_http_core_variables; cv->name.len; cv++) {
        v = ngx_palloc(cf->pool, sizeof(ngx_http_variable_t));
        if (v == NULL) {
            return NGX_ERROR;
        }

        *v = *cv;

        rc = ngx_hash_add_key(cmcf->variables_keys, &v->name, v,
                              NGX_HASH_READONLY_KEY);

        if (rc == NGX_OK) {
            continue;
        }

        if (rc == NGX_BUSY) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "conflicting variable name \"%V\"", &v->name);
        }

        return NGX_ERROR;
    }

    return NGX_OK;
}

/*
����Ϸ��Լ���߼��ܼ򵥣�ʵ���ں���ngx_http_variables_init_vars�ڣ������cmcf->variables���ռ���������ʹ�ñ��������ȥ�Ѷ������
cmcf->variableskeys�����ֲ��ҡ�����ҵ����ʾ�û�ʹ���������û�ҵ�������Ҫע�⣬�⻹ֻ��˵����������һ���Ƿ�������
    ��һ���ֱ�����Ȼû�а�����cmcf->variables_key�ڣ���������ȴ�Ϸ����ⲿ�ֱ������ԡ�http_������sent_http_������upstream http������cookie_����
��arg����ͷ�������������Щ�����Ӵ��Ҳ���Ԥ֪����������ǰ���岢�ռ���cmcf->variables_keys�ڣ������ԡ�arg����ͷ����Ĳ�����������
�ݿͻ�������URIʱ�����Ĳ�����I˾����ͬ��һ�������ڡ�http:// 192.168. 164.2/?pageid=2��������������Զ����ɱ���$argpageid����˻����ж�
�û��������ļ���ʹ�õı����Ƿ�������������������˵���Ǽ���û�ʹ�õı�����ǰ�漸���ַ��Ƿ�������һ�£���Ҳ���˵�����û��Զ������ʱ
��ò�Ҫ����Щ�ַ���ͷ������Ȼ������û��������ļ���ʹ���˱���$argpageid�����ͻ�������ʱȴ��û�д���pageid��������ʱ����$arg_pageidֵΪ�գ�
�����ܻ����ǺϷ��ģ��������ʾ�������������Ĵ��������������ļ��ڱ������Ƿ���д��ȷ��(unknown "XXX" variable)
    ����ngx_http_variables_init_vars()�ڶ���ʹ�ñ������кϷ��Լ���ͬʱ�����ںϷ���ʹ�ñ����Ὣ���Ӧ��������Ҫ�ֶ����úã�
��get_handler0�ص���data���ݡ�flags��ꡣ��ǰ������Ľṹ��ngx_http_variables����������name�洢���Ǳ������ַ�����index�洢��
�Ǹñ�����cmcf->variables�ڵ��±꣨ͨ������ngx_http_get_variable_index��ã������������ǲ���ģ���set_handlerr()�ص�Ŀǰֻ��
ʹ��set����ָ���ű�����ʱ�Ż��õ���������ֱ��ʹ��cmcf->variables_keys���Ӧ�����ĸ��ֶΣ�����һ�������ļ�������ϣ�
set_handlerr()�ص�Ҳ���ò����ˣ�����ֻ��ʣ�µ������ֶβ���Ҫ����ֵ����������cmcf->variables_keys���Ӧ�����Ķ�Ӧ�ֶο���������
����������������͸��ݲ�ͬ�����й̶��ĸ�ֵ��
*/
ngx_int_t
ngx_http_variables_init_vars(ngx_conf_t *cf)
{
    ngx_uint_t                  i, n;
    ngx_hash_key_t             *key;
    ngx_hash_init_t             hash;
    ngx_http_variable_t        *v, *av;
    ngx_http_core_main_conf_t  *cmcf;

    /* set the handlers for the indexed http variables */

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    v = cmcf->variables.elts;
    key = cmcf->variables_keys->keys.elts;

    for (i = 0; i < cmcf->variables.nelts; i++) {

        for (n = 0; n < cmcf->variables_keys->keys.nelts; n++) {

            av = key[n].value;

            if (v[i].name.len == key[n].key.len
                && ngx_strncmp(v[i].name.data, key[n].key.data, v[i].name.len)
                   == 0)
            { //���ںϷ���ʹ�ñ����Ὣ���Ӧ��������Ҫ�ֶ����úã���get_handler0�ص���data���ݡ�flags���
                v[i].get_handler = av->get_handler;
                v[i].data = av->data;

                av->flags |= NGX_HTTP_VAR_INDEXED;
                v[i].flags = av->flags;

                av->index = i;

                if (av->get_handler == NULL) {
                    break;
                }

                goto next;
            }
        }

        /*
          ngx_http_variables_init_vars����variables�еı����Ƿ���variables_keys�У�������ڣ�˵�����ʹ�õ��ı����п�����û�ж�����ģ�
          �����ǲ���ʹ�õġ�Ϊʲô���п��ܣ���forѭ��������Ǽ���if�����жϣ��������variables_keys�еı����������if�ж��о�˵�����
          �����ǺϷ��ģ����򲻺Ϸ�

            ��һ���ֱ�����Ȼû�а�����cmcf->variables_key�ڣ���������ȴ�Ϸ����ⲿ�ֱ������ԡ�http_������sent_http_������upstream http������cookie_����
        ��arg����ͷ�������������Щ�����Ӵ��Ҳ���Ԥ֪����������ǰ���岢�ռ���cmcf->variables_keys�ڣ������ԡ�arg����ͷ����Ĳ�����������
        �ݿͻ�������URIʱ�����Ĳ�����ͬ����ͬ��һ�������ڡ�http:// 192.168. 164.2/?pageid=2��������������Զ����ɱ���$argpageid����˻����ж�
        �û��������ļ���ʹ�õı����Ƿ�������������������˵���Ǽ���û�ʹ�õı�����ǰ�漸���ַ��Ƿ�������һ�£���Ҳ���˵�����û��Զ������ʱ
        ��ò�Ҫ����Щ�ַ���ͷ������Ȼ������û��������ļ���ʹ���˱���$argpageid�����ͻ�������ʱȴ��û�д���pageid��������ʱ����$arg_pageidֵΪ�գ�
        �����ܻ����ǺϷ��ģ��������ʾ�������������Ĵ��������������ļ��ڱ������Ƿ���д��ȷ��(unknown "XXX" variable)
          */ //��variables�У����ǲ���variables_keys�еı������������������ַ�����ͷ�����ǷǷ���

        //���set ���õı����������������ַ�����ͷ������ngx_http_rewrite_set��ʱ����ʵ�Ѿ����ú���get_hendler��data
        
        if (ngx_strncmp(v[i].name.data, "http_", 5) == 0) {
            v[i].get_handler = ngx_http_variable_unknown_header_in;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (ngx_strncmp(v[i].name.data, "sent_http_", 10) == 0) {
            v[i].get_handler = ngx_http_variable_unknown_header_out;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (ngx_strncmp(v[i].name.data, "upstream_http_", 14) == 0) {
            v[i].get_handler = ngx_http_upstream_header_variable;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        if (ngx_strncmp(v[i].name.data, "cookie_", 7) == 0) {
            v[i].get_handler = ngx_http_variable_cookie;
            v[i].data = (uintptr_t) &v[i].name;

            continue;
        }

        if (ngx_strncmp(v[i].name.data, "upstream_cookie_", 16) == 0) {
            v[i].get_handler = ngx_http_upstream_cookie_variable;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        if (ngx_strncmp(v[i].name.data, "arg_", 4) == 0) {
            v[i].get_handler = ngx_http_variable_argument;
            v[i].data = (uintptr_t) &v[i].name;
            v[i].flags = NGX_HTTP_VAR_NOCACHEABLE;

            continue;
        }

        ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "unknown \"%V\" variable", &v[i].name);

        return NGX_ERROR;

    next:
        continue;
    }


    for (n = 0; n < cmcf->variables_keys->keys.nelts; n++) {
        av = key[n].value;

        if (av->flags & NGX_HTTP_VAR_NOHASH) {
            key[n].key.data = NULL;
        }
    }

    hash.hash = &cmcf->variables_hash;
    hash.key = ngx_hash_key;
    hash.max_size = cmcf->variables_hash_max_size;
    hash.bucket_size = cmcf->variables_hash_bucket_size;
    hash.name = "variables_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    if (ngx_hash_init(&hash, cmcf->variables_keys->keys.elts,
                      cmcf->variables_keys->keys.nelts)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    cmcf->variables_keys = NULL;

    return NGX_OK;
}

