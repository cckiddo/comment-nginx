
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/*
һ��������ʽƥ�䣬���У�
* ~ Ϊ���ִ�Сдƥ��
* ~* Ϊ�����ִ�Сдƥ��
* !~��!~*�ֱ�Ϊ���ִ�Сд��ƥ�估�����ִ�Сд��ƥ��
�����ļ���Ŀ¼ƥ�䣬���У�
* -f��!-f�����ж��Ƿ�����ļ�
* -d��!-d�����ж��Ƿ����Ŀ¼
* -e��!-e�����ж��Ƿ�����ļ���Ŀ¼
* -x��!-x�����ж��ļ��Ƿ��ִ��
����rewriteָ������һ�����Ϊflag��ǣ�flag����У�
1.last    �൱��apache�����[L]��ǣ���ʾrewrite��
2.break��������ƥ����ɺ���ֹƥ�䣬����ƥ�����Ĺ���
3.redirect  ����302��ʱ�ض����������ַ����ʾ��ת���URL��ַ��
4.permanent  ����301�����ض����������ַ����ʾ��ת���URL��ַ��


ʹ��last��breakʵ��URI��д���������ַ�����䡣����������ϸ΢���ʹ��aliasָ�������last���;ʹ��proxy_passָ��ʱ����Ҫʹ��break��ǡ�Last����ڱ���rewrite����ִ����Ϻ󣬻��������server{......}��ǩ���·������󣬶�break������ڱ�������ƥ����ɺ���ֹƥ�䡣
���磺������ǽ�����URL/photo/123456 �ض���/path/to/photo/12/1234/123456.png
rewrite "/photo/([0-9]{2})([0-9]{2})([0-9]{2})"/path/to/photo/$1/$1$2/$1$2$3.png ;


�ģ�NginxRewrite �������ָ��


1.breakָ��
ʹ�û�����server,location,if;
��ָ�����������ɵ�ǰ�Ĺ��򼯣����ٴ���rewriteָ�


2.ifָ��
ʹ�û�����server,location
��ָ�����ڼ��һ�������Ƿ���ϣ�����������ϣ���ִ�д������ڵ���䡣Ifָ�֧��Ƕ�ף���֧�ֶ������&&��||����


3.returnָ��
�﷨��returncode ;
ʹ�û�����server,location,if;
��ָ�����ڽ��������ִ�в�����״̬����ͻ��ˡ�
ʾ����������ʵ�URL��".sh"��".bash"��β���򷵻�403״̬��
location ~ .*\.(sh|bash)?$
{
return 403;
}


4.rewrite ָ��
�﷨��rewriteregex replacement flag
ʹ�û�����server,location,if
��ָ����ݱ��ʽ���ض���URI�������޸��ַ�����ָ����������ļ��е�˳����ִ�С�ע����д���ʽֻ�����·����Ч����������������������Ӧ��ʹ��if��䣬ʾ�����£�
if( $host ~* www\.(.*) )
{
set $host_without_www $1;
rewrite ^(.*)$  http://$host_without_www$1permanent;
}


5.Setָ��
�﷨��setvariable value ; Ĭ��ֵ:none; ʹ�û�����server,location,if;
��ָ�����ڶ���һ������������������ֵ��������ֵ����Ϊ�ı��������Լ��ı����������ϡ�
ʾ����set$varname "hello world";


6.Uninitialized_variable_warnָ��
�﷨��uninitialized_variable_warnon|off
ʹ�û�����http,server,location,if
��ָ�����ڿ����͹ر�δ��ʼ�������ľ�����Ϣ��Ĭ��ֵΪ������

 


�壮Nginx��Rewrite�����дʵ��
1.�����ʵ��ļ���Ŀ¼������ʱ���ض���ĳ��php�ļ�
if( !-e $request_filename )
{
rewrite ^/(.*)$ index.php last;
}


2.Ŀ¼�Ի� /123456/xxxx  ====>  /xxxx?id=123456
rewrite ^/(\d+)/(.+)/  /$2?id=$1 last;


3.����ͻ���ʹ�õ���IE����������ض���/ieĿ¼��
if( $http_user_agent  ~ MSIE)
{
rewrite ^(.*)$ /ie/$1 break;
}


4.��ֹ���ʶ��Ŀ¼
location ~ ^/(cron|templates)/
{
deny all;
break;
}


5.��ֹ������/data��ͷ���ļ�
location ~ ^/data
{
deny all;
}


6.��ֹ������.sh,.flv,.mp3Ϊ�ļ���׺�����ļ�
location ~ .*\.(sh|flv|mp3)$
{
return 403;
}


7.����ĳЩ�����ļ������������ʱ��
location ~ .*\.(gif|jpg|jpeg|png|bmp|swf)$
{
expires 30d;
}
location ~ .*\.(js|css)$
{
expires 1h;
}


8.��favicon.ico��robots.txt���ù���ʱ��;
����Ϊfavicon.icoΪ99��,robots.txtΪ7�첢����¼404������־
location ~(favicon.ico) {
log_not_found off;
expires 99d;
break;
}
location ~(robots.txt) {
log_not_found off;
expires 7d;
break;
}


9.�趨ĳ���ļ��Ĺ���ʱ��;����Ϊ600�룬������¼������־
location ^~ /html/scripts/loadhead_1.js {
access_log  off;
root /opt/lampp/htdocs/web;
expires 600;
break;
}


10.�ļ������������ù���ʱ��
�����return412 Ϊ�Զ����http״̬�룬Ĭ��Ϊ403�������ҳ���ȷ�ĵ���������
��rewrite ^/ http://img.linuxidc.net/leech.gif;����ʾһ�ŷ�����ͼƬ
��access_log off;������¼������־������ѹ��
��expires 3d�������ļ�3������������


location ~*^.+\.(jpg|jpeg|gif|png|swf|rar|zip|css|js)$ {
valid_referers none blocked *.linuxidc.com*.linuxidc.net localhost 208.97.167.194;
if ($invalid_referer) {
rewrite ^/ http://img.linuxidc.net/leech.gif;
return 412;
break;
}
access_log  off;
root /opt/lampp/htdocs/web;
expires 3d;
break;
}


11.ֻ����̶�ip������վ������������


root /opt/htdocs/www;
allow  208.97.167.194; 
allow  222.33.1.2; 
allow  231.152.49.4;
deny  all;
auth_basic ��C1G_ADMIN��;
auth_basic_user_file htpasswd;


12���༶Ŀ¼�µ��ļ�ת��һ���ļ�����ǿseoЧ��
/job-123-456-789.html ָ��/job/123/456/789.html


rewrite^/job-([0-9]+)-([0-9]+)-([0-9]+)\.html$ /job/$1/$2/jobshow_$3.html last;


13.�ļ���Ŀ¼�����ڵ�ʱ���ض���


if (!-e $request_filename) {
proxy_pass http://127.0.0.1;
}


14.����Ŀ¼��ĳ���ļ���ָ��2��Ŀ¼
��/shanghaijob/ ָ�� /area/shanghai/
����㽫last�ĳ�permanent����ô�������ַ������/location/shanghai/
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;
���������и������Ƿ���/shanghaiʱ������ƥ��
rewrite ^/([0-9a-z]+)job$ /area/$1/ last;
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;
����/shanghai Ҳ���Է����ˣ���ҳ���е���������޷�ʹ�ã�
��./list_1.html��ʵ��ַ��/area/shanghia/list_1.html����/list_1.html,�����޷����ʡ�
���Ҽ����Զ���תҲ�ǲ��п�
(-d $request_filename)���и������Ǳ���Ϊ��ʵĿ¼�����ҵ�rewrite���ǵģ�����û��Ч��
if (-d $request_filename){
rewrite ^/(.*)([^/])$ http://$host/$1$2/permanent;
}
֪��ԭ���ͺð��ˣ������ֶ���ת��
rewrite ^/([0-9a-z]+)job$ /$1job/permanent;
rewrite ^/([0-9a-z]+)job/(.*)$ /area/$1/$2last;


15.������ת
server
{
listen      80;
server_name  jump.linuxidc.com;
index index.html index.htm index.php;
root  /opt/lampp/htdocs/www;
rewrite ^/ http://www.linuxidc.com/;
access_log  off;
}


16.������ת��
server_name  www.linuxidc.comwww.linuxidc.net;
index index.html index.htm index.php;
root  /opt/lampp/htdocs;
if ($host ~ "linuxidc\.net") {
rewrite ^(.*) http://www.linuxidc.com$1permanent;
}


����nginxȫ�ֱ���
arg_PARAMETER    #�����������GET�����У�����б���PARAMETERʱ��ֵ��
args                    #�������������������(GET����)�Ĳ������磺foo=123&bar=blahblah;
binary_remote_addr #�����ƵĿͻ���ַ��
body_bytes_sent    #��Ӧʱ�ͳ���body�ֽ�����������ʹ�����жϣ��������Ҳ�Ǿ�ȷ�ġ�
content_length    #����ͷ�е�Content-length�ֶΡ�
content_type      #����ͷ�е�Content-Type�ֶΡ�
cookie_COOKIE    #cookie COOKIE������ֵ
document_root    #��ǰ������rootָ����ָ����ֵ��
document_uri      #��uri��ͬ��
host                #��������ͷ�ֶΣ�����Ϊ���������ơ�
hostname          #Set to themachine��s hostname as returned by gethostname
http_HEADER
is_args              #�����args����������������ڡ�?����������ڡ�"����ֵ��
http_user_agent    #�ͻ���agent��Ϣ
http_cookie          #�ͻ���cookie��Ϣ
limit_rate            #����������������������ʡ�
query_string          #��args��ͬ��
request_body_file  #�ͻ�������������Ϣ����ʱ�ļ�����
request_method    #�ͻ�������Ķ�����ͨ��ΪGET��POST��
remote_addr          #�ͻ��˵�IP��ַ��
remote_port          #�ͻ��˵Ķ˿ڡ�
remote_user          #�Ѿ�����Auth Basic Module��֤���û�����
request_completion #����������������ΪOK. ������δ������������������������������һ��ʱ��Ϊ��(Empty)��
request_method    #GET��POST
request_filename  #��ǰ������ļ�·������root��aliasָ����URI�������ɡ�
request_uri          #�������������ԭʼURI�����������������磺��/foo/bar.php?arg=baz���������޸ġ�
scheme                #HTTP��������http��https����
server_protocol      #����ʹ�õ�Э�飬ͨ����HTTP/1.0��HTTP/1.1��
server_addr          #��������ַ�������һ��ϵͳ���ú����ȷ�����ֵ��
server_name        #���������ơ�
server_port          #���󵽴�������Ķ˿ںš�


�ߣ�Apache��Nginx����Ķ�Ӧ��ϵ
Apache��RewriteCond��ӦNginx��if
Apache��RewriteRule��ӦNginx��rewrite
Apache��[R]��ӦNginx��redirect
Apache��[P]��ӦNginx��last
Apache��[R,L]��ӦNginx��redirect
Apache��[P,L]��ӦNginx��last
Apache��[PT,L]��ӦNginx��last


���磺����ָ�����������ʱ�վ������������һ��ת��www.linuxidc.net
  Apache:
RewriteCond %{HTTP_HOST} !^(.*?)\.aaa\.com$[NC]
RewriteCond %{HTTP_HOST} !^localhost$ 
RewriteCond %{HTTP_HOST}!^192\.168\.0\.(.*?)$
RewriteRule ^/(.*)$ http://www.linuxidc.net[R,L]


  Nginx:
if( $host ~* ^(.*)\.aaa\.com$ )
{
set $allowHost ��1��;
}
if( $host ~* ^localhost )
{
set $allowHost ��1��;
}
if( $host ~* ^192\.168\.1\.(.*?)$ )
{
set $allowHost ��1��;
}
if( $allowHost !~ ��1�� )
{
rewrite ^/(.*)$ http://www.linuxidc.netredirect ;
}

��ƪ������Դ�� Linux������վ(www.linuxidc.com)  ԭ�����ӣ�http://www.linuxidc.com/Linux/2014-01/95493.htm



















Nginx Rewrite��� 
Nginx Rewrite���

�������ӣ�http://blog.cafeneko.info/2010/10/nginx_rewrite_note/

 

ԭ�����£�

 

����������Ǩ�ƹ�����,�������Ѿ���WP permalink rewrite������.

��Ϊ���������õ�Apache, ʹ�õ���WP����Ϳ��Ը��ĵ�.htaccess,û��̫����Ѷ�.�������VPS���ܵ���Nginx,��Ҫ����ΪNginx���ٶȱ�ApacheҪ��ܶ�.

������һ����Ͳ�����ô�����,��ΪNginx��rewrite��Apache��ͬ,�������ڷ�����������ܸ���.

����������һЩ�о��ʼ�.(�������������ر�˵����ժ��nginx wiki)

 

/1 Nginx rewrite�����﷨
 

Nginx��rewrite�﷨��ʵ�ܼ�.�õ���ָ���޷����⼸��

?set
?if
?return
?break
?rewrite
��ȸ��С,�������������ȫ.ֻ�Ǽ򵥵ļ���ָ��ȴ�����������Բ���apache�ļ���������.

1.set

set��Ҫ���������ñ����õ�,ûʲô�ر��

2.if

if��Ҫ�����ж�һЩ��rewrite������޷�ֱ��ƥ�������,�������ļ��������,http header,cookie��,

�÷�: if(����) {��}

- ��if���ʽ�е�����Ϊtrue,��ִ��if���е����

- �����ʽֻ��һ������ʱ,���ֵΪ�ջ����κ���0��ͷ���ַ������ᵱ��false

- ֱ�ӱȽ�����ʱ,ʹ�� = �� !=

- ʹ��������ʽƥ��ʱ,ʹ��

~ ��Сд����ƥ��   YANG ADD Ӧ������ȫƥ��
~* ��Сд������ƥ��  yang add Ӧ���ǲ����ִ�Сдƥ��
!~ ��Сд���в�ƥ��   yang add�������ȫƥ�䣬��Ϊ��
!~* ��Сд�����в�ƥ�� yang add ��������ִ�Сдƥ��ɹ������ؼ�

�⼸�仰�������е���,��֮��ס: ~Ϊ����ƥ��, ����*Ϊ��Сд������, ǰ��!Ϊ���ǡ�����

���һ��,��Ϊnginxʹ�û�����{}�ж�����,���Ե������а���������ʱ,�������˫���Ž����������.�����潲����rewrite����е������������. 
���� ��\d{4}\d{2}\.+��

- ʹ��-f,-d,-e,-x����ļ���Ŀ¼

-f ����ļ�����
-d ���Ŀ¼����
-e ����ļ�,Ŀ¼���߷������Ӵ���
-x ����ļ���ִ��

��~����,ǰ��!��Ϊ���ǡ�����

����

if ($http_user_agent ~ MSIE) {
  rewrite  ^(.*)$  /msie/$1  break;
}//���UA������MSIE��,rewrite ����/msieĿ¼��

if ($http_cookie ~* "id=([^;] +)(?:;|$)" ) {
  set  $id  $1;
}//���cookieƥ������,���ñ���$id�����������ò���

if ($request_method = POST ) {
  return 405;
}//����ύ����ΪPOST,�򷵻�״̬405 (Method not allowed)

if (!-f $request_filename) {
  break;
  proxy_pass  http://127.0.0.1;
}//��������ļ���������,�������localhost

if ($args ~ post=140){
  rewrite ^ http://example.com/ permanent;
}//���query string�а�����post=140��,�����ض���example.com

3.return

return������ֱ������HTTP����״̬,����403,404��(301,302������return����,����������rewrite�ᵽ)

4.break

����ֹͣrewrite���,�����潲����rewrite��break flag������һ����,��������ǰ����һ�����,������rewrite����flag

5.rewrite

����ĵĹ���(�ϻ�)

�÷�: rewrite ���� �滻 ��־λ

���б�־λ������

break �C ֹͣrewrite���,Ҳ����˵������break flag��rewrite��䱻ִ��ʱ,��������rewrite�����ս�� 
last �C ֹͣrewrite���,���Ǹ�break�б��ʵĲ�ͬ,last����䲻һ�������ս��,��������nginx��locationƥ��һ���ᵽ 
redirect �C ����302��ʱ�ض���,һ�������ض���������URL(����http:����) 
permanent �C ����301�����ض���,һ�������ض���������URL(����http:����)

��Ϊ301��302���ܼ򵥵�ֻ��������״̬��,���������ض����URL,�����returnָ���޷�����301,302��ԭ����. ��Ϊ�滻,rewrite���Ը�����ʹ��redirect��permanent��־ʵ��301��302. ������һƪ��־���ᵽ��Blog���Ҫ���������ض���,��nginx�оͻ���ôд

rewrite ^(.*)$ http://newdomain.com/ permanent;������˵һ��rewrite��ʵ��Ӧ��

rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  last;�������Ϊ /download/eva/media/op1.mp3 ������rewrite�� /download/eva/mp3/op1.mp3

ʹ��������������,�ܼ򵥲���ô? ����Ҫע�����rewrite�кܶ�Ǳ������Ҫע��

- rewrite����Ч����Ϊsever, location, if

- rewriteֻ�����·������ƥ��,������hostname ����˵������301�ض��������˵��

rewrite ~* cafeneko\.info http://newdomain.com/ permanent;�������Զ�޷�ִ�е�,�����URLΪ��

http://blog.cafeneko.info/2010/10/neokoseseiki_in_new_home/?utm_source=rss&utm_medium=rss&utm_campaign=neokoseseiki_in_new_home

����cafeneko.info����hostname,������?Ϊֹ�������·��,?�����һ������query string

����rewrite��˵,��������ʽ���ԡ�/2010/10/neokoseseiki_in_new_home����һ���ֽ���ƥ��,��������hostname,Ҳ������query string .���Գ������·���а���������һ����string,�����ǲ���ƥ���. �����Ҫ������ƥ��Ļ���Ҫʹ��if�����,�������ȥwww��ת

if ($host ~* ^www\.(cafeneko\.info)) {
  set $host_without_www $1;
  rewrite ^(.*)$ http://$host_without_www$1 permanent;
}- ʹ�����·��rewriteʱ,�����HTTP header�е�HOST��nginx��server_nameƥ������rewrite,���HOST��ƥ�����û��HOST��Ϣ�Ļ���rewrite��server_name���õĵ�һ������,���û������server_name�Ļ�,��ʹ�ñ�����localhost����rewrite

- ǰ���ᵽ��,rewrite�������ǲ�ƥ��query string��,����Ĭ�������,query string���Զ�׷�ӵ�rewrite��ĵ�ַ�ϵ�,��������Զ�׷��query string,����rewrite��ַ��ĩβ���?

rewrite  ^/users/(.*)$  /show?user=$1?  last;rewrite�Ļ���֪ʶ������ô��..����û����..������ͷ�۵Ĳ���û��˵��

 

/2 Nginx location �� rewrite retry
 

nginx��rewrite�и������ص����� �� rewrite���url���ٴν���rewrite���,�������10��,10�κ�û����ֹ�Ļ��ͻ᷵��HTTP 500

�ù�nginx�����Ѷ�֪��location����,location�����е���Apache�е�RewriteBase,������nginx��˵location�ǿ��Ƶļ������,��������ݲ�������rewrite.

���������΢�Ƚ�һ��location��֪ʶ.location��nginx���������ͬһ��server��ͬ�������ַʹ�ö��������õķ�ʽ

����:

location  = / {
  ....����A
}
 
location  / {
  ....����B
}
 
location ^~ /images/ {
  ....����C
}
 
location ~* \.(gif|jpg|jpeg)$ {
  ....����D
}���� / ��ʹ������A 
���� /documents/document.html ��ʹ������B 
���� /images/1.gif ��ʹ������C 
���� /documents/1.jpg ��ʹ������D

����ж������ĸ�location���Ұ��²��, ������ʵսƪ�ٻ�ͷ�����������.

��������ֻ��Ҫ����һ�����: nginx�����ж��location��ʹ�ò�ͬ����.

sever����������а���rewrite����,�������ִ��,����ֻ��ִ��һ��, Ȼ�����ж������ĸ�location������,��ȥִ�и�location�е�rewrite, ����location�е�rewriteִ�����ʱ,rewrite������ֹͣ,���Ǹ���rewrite����URL�ٴ��ж�location��ִ�����е�����. ��ô,����ʹ���һ������,���rewriteд�Ĳ���ȷ�Ļ�,�ǻ���location������������ѭ����.����nginx�Ż��һ���������10�ε�����. �����������

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  last;
}�������Ϊ /download/eva/media/op1.mp3 ������rewrite�� /download/eva/mp3/op1.mp3

���rewrite�Ľ������������location /download/ ��Ȼ��β�û������rewrite�����������ʽ,����Ϊȱ����ֹrewrite�ı�־,���Ի᲻ͣ����download��rewrite����ֱ���ﵽ10�����޷���HTTP 500

�����������ʱ�ͻ�����,�����rewrite�������б�־λlastô? last������ֹrewrite����˼ô?

˵�������Ҿ�Ҫ��Թ����,�������ҵ�����nginx rewrite��������80%��last��־�Ľ��Ͷ���

last �C �����϶������Flag

����������ӵ���!!! ʲô�л����϶���? ʲô�ǲ����������?  =��=

����Ȥ�Ŀ��ԷŹ��������϶������Flag����

�����ջ�����stack overflow�ҵ��˴�:

last��break���Ĳ�ͬ����

- break����ֹ��ǰlocation��rewrite���,���Ҳ��ٽ���locationƥ�� 
�C last����ֹ��ǰlocation��rewrite���,�����������locationƥ�䲢���������е�rewrite����

�����������������

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  ;
  rewrite  ^(/download/.*)/movie/(.*)\..*$  $1/avi/$2.mp3  ;
  rewrite  ^(/download/.*)/avvvv/(.*)\..*$  $1/rmvb/$2.mp3 ;
}����û��д��־λ,���λ�����Բ���

�������Ϊ /download/acg/moive/UBW.avi

last�������: �ڵ�2��rewrite����ֹ,������location /download..��ѭ�� 
break�������: �ڵ�2��rewrite����ֹ,����Ϊ���յ�rewrite��ַ.

Ҳ����˵,�����ĳλ��ͼ����eva op����û�µ�������HTTP 500����һ��������������Ϊ����last��־���ԲŻ������ѭ��,�����break��û����.

location /download/ {
  rewrite  ^(/download/.*)/media/(.*)\..*$  $1/mp3/$2.mp3  break;
}�����������,�Ҹ��˵Ľ�����,�����ȫ�����ʵ�rewrite,��÷���server�����в����ٲ���Ҫ��location����.location�����е�rewriteҪ���������last����break.

���˿��ܻ���,��break��������һʧ��ô?

����.��Щ�����Ҫ��last��. ���͵����Ӿ���wordpress��permalink rewrite

�����������, wordpress��rewrite�Ƿ���location /����,��������rewrite��/index.php

��ʱ�������ʹ��break�˾͹���,��������. �⣨���������䡭��Ϊnginx���ص���û�н��͵�index.php��Դ�롭

����һ��Ҫʹ��last�ſ����ڽ���location / ��rewrite, ���ٴ�����location ~ \.php$,���佻��fastcgi���н���.��󷵻ظ�������Ĳ��ǽ��͹���html����.

����nginx rewrite�ļ�鵽�����ȫ��������,ˮƽ��������,����ָ����©��

 

/3 ʵս! WordPress��Permalink+Supercache rewriteʵ��
 

���rewriteд����ʵ������supercache���߱��ҵ�ĳ��������,���Ϻ����ײ鵽,����һЩ�޸�. �ȸ����������ļ���ȫ������..�������������..����·��ʲô����֪��Ҳûɶ�ö԰�?

server {
	listen   80;
	server_name  cafeneko.info www.cafeneko.info;
 
	access_log  ***;
	error_log   *** ;
 
	root   ***;
	index  index.php;
 
	gzip_static on;
 
	if (-f $request_filename) {
		break;
	}
 
	set $supercache_file '';
	set $supercache_uri $request_uri;
 
	if ($request_method = POST) {
		set $supercache_uri '';
	}
 
	if ($query_string) {
		set $supercache_uri '';
	}
 
	if ($http_cookie ~* "comment_author_|wordpress_logged_|wp-postpass_" ) {
		set $supercache_uri '';
	}
 
	if ($supercache_uri ~ ^(.+)$) {
		set $supercache_file /wp-content/cache/supercache/$http_host/$1index.html;
	}
 
	if (-f $document_root$supercache_file) {
		rewrite ^(.*)$ $supercache_file break;
	}
 
	if (!-e $request_filename) {
		rewrite . /index.php last;
	}
 
	location ~ \.php$ {
 
		fastcgi_pass   127.0.0.1:9000;
		fastcgi_index  index.php;
		fastcgi_param  SCRIPT_FILENAME  ***$fastcgi_script_name;
		include        fastcgi_params;
	}
 
	location ~ /\.ht {
		deny  all;
	}
}�����ǽ���:

gzip_static on;��������֧��gzip,����ѹ��ǰ��Ѱ���Ƿ����ѹ���õ�ͬ��gz�ļ������ٴ�ѹ���˷���Դ,���supercache��ѹ������һ��ʹ��Ч�����,���supercacheԭ����Apache mod_rewriteʵ��,nginx��ʵ�ּ򵥵Ķ�. Apache mod_rewrite�����������׿�����һģһ���������ж����ֱ�rewrite֧��gzipѹ���Ͳ�֧�ֵ����.

if (-f $request_filename) {
	break;
}//�����ֱ������ĳ����ʵ���ڵ��ļ�,����break���ֹͣrewrite���

set $supercache_file '';
set $supercache_uri $request_uri;//��$request_uri��ʼ������ $supercache_uri.

if ($request_method = POST) {
	set $supercache_uri '';
}//�������ʽΪPOST,��ʹ��supercache.���������$supercache_uri�ķ������������,����ῴ��

if ($query_string) {
	set $supercache_uri '';
}//��Ϊʹ����rewrite��ԭ��,��������²�Ӧ����query_string(һ��ֻ�к�̨�Ż����query string),�еĻ���ʹ��supercache

if ($http_cookie ~* "comment_author_|wordpress_logged_|wp-postpass_" ) {
	set $supercache_uri '';
}//Ĭ�������,supercache�ǽ���unknown userʹ�õ�.���������¼�û��������۹����û���ʹ��.

comment_author�ǲ��������û���cookie, wordpress_logged�ǲ��Ե�¼�û���cookie. wp-postpass�������,����������������������������µ�?ֻҪcookie�к�����Щ�ַ�������������.

ԭ����д���м���¼�û�cookie�õ���wordpress_,�������ڲ����з��ֵ���/�ǳ��Ժ󻹻���һ����wordpress_test_cookie����,��֪����ʲô����,��Ҳ�����һ���û��Ƿ��������cookie.���ڿ��ǵ��ǳ��Ժ����cookie��Ȼ���ڿ��ܻ�Ӱ�쵽cache���ж�,���ǰ�����ĳ���ƥ��wordpress_logged_

if ($supercache_uri ~ ^(.+)$) {
	set $supercache_file /wp-content/cache/supercache/$http_host$1index.html;
}//�������$supercache_uri��Ϊ��,������cache file��·��

������΢������$http_host$1index.html�⴮����,��ʵд�� $http_host/$1/index.html �ͺö��ܶ�

�����rewrite��ʽ��urlΪ��

cafeneko.info/2010/09/tsukihime-doujin_part01/

���� 
$http_host = ��cafeneko.info�� , $1 = $request_uri = ��/2010/09/tsukihime-doujin_part01/��

�� $http_host$1index.html = ��cafeneko.info/2010/09/tsukihime-doujin_part01/index.html��

�� $http_host/$1/index.html = ��cafeneko.info//2010/09/tsukihime-doujin_part01//index.html��

��Ȼ�ڵ��Թ��������߲�û�в�ͬ,����Ϊ�˱�����ȷ��·��,����ʡ�����м��/����.

�������rewrite���url = ��cafeneko.info/wp-content/cache/supercache/cafeneko.info/2010/09/tsukihime-doujin_part01/index.html��

if (-f $document_root$supercache_file) {
	rewrite ^(.*)$ $supercache_file break;
}//���cache�ļ��Ƿ����,���ڵĻ���ִ��rewrite,����������Ϊ��rewrite��html��̬�ļ�,���Կ���ֱ����break��ֹ��.

if (!-e $request_filename) {
	rewrite . /index.php last;
}//ִ�е�����˵����ʹ��suercache,����wordpress��permalink rewrite

���������ļ�/Ŀ¼�Ƿ����,�������������������, rewrite��index.php

˳��˵һ��,��ʱ�������rewrite�����Ұ�˼�������. .

ֻ��ƥ��һ���ַ���?����ʲô��˼?

һ�������,�����nginx rewrite��򵥵ķ������ǰ�flagд��redirect,�����������������ַ���￴����ʵ��rewrite��ַ.

Ȼ������permalink rewriteȴ���������ַ���,��Ϊһ��д��redirect�Ժ�,���ܵ�ʲô����,ֻҪû��supercache,������ת����ҳ��.

��������һЩ���²�������rewrite�ı���,��ʵ���ڱ��������ַ����������,�ڷ������˽�����ת���ض���ҳ��.

էһ��supercache�������е���302����̬�ļ�,���Կ�����redirect����.

����permalinkȴ��������ȫ��ͬ��rewrite,���wordpress�Ĵ���ʽ�й�. ���о�����Ͳ���˵��,��˵���Ǳ���URL���佫����rewrite��index.php,WP��������URL�ṹ�ٶ��䲢����ƥ��(����,ҳ��,tag��),Ȼ���ٹ���ҳ��. ������ʵ����rewrite

rewrite . /index.php last;˵����,�κ����󶼻ᱻrewrite��index.php.��Ϊ��.��ƥ�������ַ�,��������rewrite��ʵ����д���κ���ʽ�����������е�����.����˵

rewrite . /index.php last;
rewrite ^ /index.php last;
rewrite .* /index.php last;Ч������һ����,��������permalink rewrite.

���Ҫ��ľ������˿���ע�⵽�ҵ�rewrite�����Ƿ���server���е�.�������ҵ��Ĵ��������wordpress��nginx rewrite�����Ƿ���location /�����,����������ȴ������server����,Ϊ��?

ԭ����WP��ĳ��������ڵ�ǰҳ����һ��POST��XHR����,����ûʲô�ر�,������ͳ�����XHR�����URL�ṹ��.

������permalinkһ��Ϊ: domain.com/year/month/postname/ ���� domain.com/tags/tagname/ ֮��.

�����XHR�����URLȴ�� domain.com/year/month/postname/index.php ���� domain.com/tags/tagname/index.php

����һ����������location ~ \.php$������fastcgi,����Ϊ����û������rewrite��ҳ�治���ܴ���,����������XHR����һ��404

����location֮��ƥ�����ȼ���ԭ��,�ҽ���Ҫ��rewrite����ȫ���Ž���server������,�����͵��Ա�֤�ڽ���locationƥ��֮ǰ��һ������rewrite��.

��ʱ��������Ҫ����,Ϊʲô���е���location ~ \.php$������location / ?

�����졭��̾�����Ҫ������ɱ��locationƥ�������ˡ�.

locatoin������rewrite��������ִ��,��������ƥ�����ȼ���,��һ������ͬʱ���㼸��location��ƥ��ʱ,��ֻ��ѡ����һ������ִ��.

��Ѱ�ҵķ���Ϊ:

1. ����Ѱ�����еĳ���ƥ��,��location /, location /av/, �����·����������ƥ��,ƥ�䳤����ߵĻᱻʹ��, 
2. Ȼ���������ļ��г��ֵ�˳�����β���������ʽ,�� location ~ download\/$, location ~* \.wtf, ��һ��ƥ��ᱻʹ�� 
3. ���û��ƥ�������,��ʹ��֮ǰ�ĳ���ƥ��

�����漸�ַ�����ƥ��ʱ��������ֹ����location�ĳ���

1. = ��ȫƥ��,location = /download/ 
2. ^~ ��ֹ�������,��location ^~ /download/ ����������ƥ��,����ֹ�������,�������ֻ��ƥ�䳣�� 
3. ��û��=����^~�������,���������ȫƥ��,Ҳ��������ֹ����,��������Ϊ /download/ ����ȫ����location /download/���������������������

�ܽ�:

1. �����ȫƥ��(������û��=),���Ի�������ֹ
2. ���ƥ����Ը�������,�������ƥ�䲢�� ^~, ���Ի���ֹ 
3. ���������ļ��г��ֵ�˳����Ը���������ʽ 
4. �����3��������,��ʹ����ƥ��location,����ʹ�õ�2����location

���⻹���Զ���һ�������named location,��@��ͷ,��location @thisissparta ��������location���岻����һ��Ĵ���,����ר������try_file, error_page�Ĵ���,���ﲻ������.

����û? ��ǰ�ĵ�����������

location  = / {
  ....����A
}
 
location  / {
  ....����B
}
 
location ^~ /images/ {
  ....����C
}
 
location ~* \.(gif|jpg|jpeg)$ {
  ....����D
}���� / ��ʹ������A -> ��ȫ����
���� /documents/document.html ��ʹ������B -> ƥ�䳣��B,��ƥ������C��D,������B 
���� /images/1.gif ��ʹ������C -> ƥ�䳣��B,ƥ������C,ʹ���׸����е�����,������C 
���� /documents/1.jpg ��ʹ������D -> ƥ�䳣��B,��ƥ������C,ƥ������D,ʹ���׸����е�����,������D

��ô�ٻ�ͷ�����Ǹղ�˵������.Ϊʲô�Ǹ�URL�����ֵ�XHR���������location ~ \.php$������location / ? ��������Ӧ���Ѿ�֪������.

����Ҫ������������򵥵ķ������ǰ�rewrite������ڱ�location��ִ�е�server������Ϳ�����Ӵ.

��ε��о��ʼǾ͵���Ϊֹ��.

�����һ��˼����,�������rewrite�������server��,����ʲô�������Խ�����XHR 404������?

ԭ����location /�������location ~ \.php$��rootΪֹ�Ĳ���.

���Ǵ��ڵ�.����ʹ��Ŀǰ�ķ���ǰ�����Խ���ڱ���location /��ǰ���³����˺ܶ��ַ������벻Ҫ����Ϊ����permalink����������location.��Ϊwp��permalink����ܶ�,������ƪ����,ҳ��,����,tag,����,�浵�ȵ�..��ӭ�ڻظ������� /

�ο�:
Nginx wiki

-EOF-

 

����  @2010.10.23

֮ǰ��supercache rewrite���������ڴ󲿷ֵ�WP.���ǲ���������mobile press������ƶ��豸֧��.

��Ϊ���в�û�м���ƶ��豸��user agent,�Ӷ������ƶ��豸Ҳ�ᱻrewrite��cache��.�����Ľ�������ƶ��豸��Ҳ�ǿ����ĸ�PCһ������ȫ��blog. �������ܱȽϺõ��ֻ�����iphone��׿ʲô�Ĵ��ûʲô����,���Ƚ�һ��ı���nokia����opera mini�ȿ��ͻ�Ƚ�������,��ΰ�supercacheԭ����htaccess�е��ƶ��豸���Ĵ����Ҳ��ֲ�˹���.

��ǰ�ĵ������ļ���cookie������������´����

	# Bypass special user agent
	if ($http_user_agent ~* "2.0 MMP|240x320|400X240|AvantGo|BlackBerry|Blazer|Cellphone|Danger|DoCoMo|Elaine/3.0|EudoraWeb|Googlebot-Mobile|hiptop|IEMobile|KYOCERA/WX310K|LG/U990|MIDP-2.|MMEF20|MOT-V|NetFront|Newt|Nintendo Wii|Nitro|Nokia|Opera Mini|Palm|PlayStation Portable|portalmmm|Proxinet|ProxiNet|SHARP-TQ-GX10|SHG-i900|Small|SonyEricsson|Symbian OS|SymbianOS|TS21i-10|UP.Browser|UP.Link|webOS|Windows CE|WinWAP|YahooSeeker/M1A1-R2D2|iPhone|iPod|Android|BlackBerry9530|LG-TU915 Obigo|LGE VX|webOS|Nokia5800") {
		set $supercache_uri '';
	}
 
	if ($http_user_agent ~* "w3c |w3c-|acs-|alav|alca|amoi|audi|avan|benq|bird|blac|blaz|brew|cell|cldc|cmd-|dang|doco|eric|hipt|htc_|inno|ipaq|ipod|jigs|kddi|keji|leno|lg-c|lg-d|lg-g|lge-|lg/u|maui|maxo|midp|mits|mmef|mobi|mot-|moto|mwbp|nec-|newt|noki|palm|pana|pant|phil|play|port|prox|qwap|sage|sams|sany|sch-|sec-|send|seri|sgh-|shar|sie-|siem|smal|smar|sony|sph-|symb|t-mo|teli|tim-|tosh|tsm-|upg1|upsi|vk-v|voda|wap-|wapa|wapi|wapp|wapr|webc|winw|winw|xda\ |xda-") {
		set $supercache_uri '';
	}�����Ϳ��Զ��ƶ��豸�ƿ�cache����,��ֱ��ʹ��mobile press�������ƶ����Ч����.


*/

typedef struct {
    /*
     ����ngx_httpscript_start_code����ngx_array_push_n��lcf->codes������������sizeof(ngx_http_script_value_code_t��Ԫ�أ�ע
 ��ÿ��Ԫ�صĴ�СΪһ���ֽڣ�������ʵҲ����Ϊngx_httpscript_valuecodet���ͱ���val����洢�ռ䣨�ܰ��ļ��ɣ�
     */ //��ŵ����Ѿ�ʹ���˵ı�����ngx_http_script_XXX_code_t�ṹ����Щ�ṹ��code��������ngx_http_rewrite_handler��õ�ִ��
     //set  break  return�ȶ�����Ӷ�Ӧ��xxx_code��������codes��

    /*ע�������Ƕ�Ӧ��server{}�����location{}���е�code������������Ӧ�ľ�����server{]����location{}�е�code,��˲�ͬserver{}��
     ��location{}�е�code��Ӧ��rlcf->codes��һ������ҪNGX_HTTP_FIND_CONFIG_PHASE�׶��ҵ���Ӧ��location����ܼ���ִ��location{}�е�rewrite
     ������ýű�����*/
    ngx_array_t  *codes;        /* uintptr_t */ //set  $variable  value�е�value�ô��뵽codes�У���ngx_http_rewrite_value

    ngx_uint_t    stack_size; //Ĭ��10

    ngx_flag_t    log;
    ngx_flag_t    uninitialized_variable_warn;
} ngx_http_rewrite_loc_conf_t;


static void *ngx_http_rewrite_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_rewrite_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_rewrite_init(ngx_conf_t *cf);
static char *ngx_http_rewrite(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_rewrite_return(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_rewrite_break(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_rewrite_if(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char * ngx_http_rewrite_if_condition(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf);
static char *ngx_http_rewrite_variable(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf, ngx_str_t *value);
static char *ngx_http_rewrite_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char * ngx_http_rewrite_value(ngx_conf_t *cf,
    ngx_http_rewrite_loc_conf_t *lcf, ngx_str_t *value);


static ngx_command_t  ngx_http_rewrite_commands[] = { //�ο�http://blog.csdn.net/brainkick/article/details/7065194
    /*
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
     */
    { ngx_string("rewrite"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE23,
      ngx_http_rewrite,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL }, 

    /*
     return
     �﷨��return code 
     Ĭ��ֵ��none
     ʹ���ֶΣ�server, location, if 
     ���ָ�����ִ��������䲢Ϊ�ͻ��˷���״̬���룬����ʹ�����е�ֵ��204��400��402-406��408��410, 411, 413, 416��500-504�����⣬�Ǳ�׼����444���ر����Ӳ��Ҳ������κε�ͷ����

    ʾ����������ʵ�URL��".sh"��".bash"��β���򷵻�403״̬��
    location ~ .*\.(sh|bash)?$
    {
        return 403;
    }
     */
    { ngx_string("return"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE12,
      ngx_http_rewrite_return,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /*
    break 
    �﷨��break
    Ĭ��ֵ��none
    ʹ���ֶΣ�server, location, if 
    ��ɵ�ǰ���õĹ���ִֹͣ����������дָ�  ��ָ�����������ɵ�ǰ�Ĺ��򼯣����ٴ���rewriteָ�
    ʾ����
    if ($slow) {
      limit_rate  10k;
      break;
    }
     */ //break����������Ľű����棬ִֹͣ��ngx_http_rewrite_handler�е�rewrite��ص�code������Ҳ�͸�����������rewrite������صļ�����������set break  return rewrite if������õ�����ִ��
    { ngx_string("break"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_NOARGS,
      ngx_http_rewrite_break,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /*
     �﷨��if (condition) { ... } 
     Ĭ��ֵ��none
     ʹ���ֶΣ�server, location 
     ��ָ�����ڼ��һ�������Ƿ���ϣ�����������ϣ���ִ�д������ڵ���䡣Ifָ�֧��Ƕ�ף���֧�ֶ������&&��||����
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
     */ //if�������̲ο�http://blog.sina.com.cn/s/blog_7303a1dc0101cm9z.html
    { ngx_string("if"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_BLOCK|NGX_CONF_1MORE,
      ngx_http_rewrite_if,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /*
    �﷨��set variable value 
    Ĭ��ֵ��none
    ʹ���ֶΣ�server, location, if 
    ָ������һ��������Ϊ�丳ֵ����ֵ�������ı������������ǵ���ϡ�
    �����ʹ��set����һ���µı��������ǲ���ʹ��set����$http_xxxͷ��������ֵ��
     */
    { ngx_string("set"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                       |NGX_CONF_TAKE2,
      ngx_http_rewrite_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("rewrite_log"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rewrite_loc_conf_t, log),
      NULL },

    /*
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
     */
    { ngx_string("uninitialized_variable_warn"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_SIF_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_rewrite_loc_conf_t, uninitialized_variable_warn),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_rewrite_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_rewrite_init,                 /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_rewrite_create_loc_conf,      /* create location configuration */
    ngx_http_rewrite_merge_loc_conf        /* merge location configuration */
};


ngx_module_t  ngx_http_rewrite_module = {//�ο�http://blog.csdn.net/brainkick/article/details/7065194
    NGX_MODULE_V1,
    &ngx_http_rewrite_module_ctx,          /* module context */
    ngx_http_rewrite_commands,             /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

//checker������ngx_http_core_rewrite_phaseִ��
static ngx_int_t
ngx_http_rewrite_handler(ngx_http_request_t *r) 
{
    ngx_int_t                     index;
    ngx_http_script_code_pt       code;
    ngx_http_script_engine_t     *e;
    ngx_http_core_srv_conf_t     *cscf;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_rewrite_loc_conf_t  *rlcf;

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    index = cmcf->phase_engine.location_rewrite_index;

    if (r->phase_handler == index && r->loc_conf == cscf->ctx->loc_conf) { 
        /* skipping location rewrite phase for server null location */
        return NGX_DECLINED;
    }

    //��NGX_HTTP_SERVER_REWRITE_PHASE�׶ζ�Ӧ����server{]���е�rewrite���ã���Ϊr->loc_conf[]ָ�����server{]�������е�loc_conf��
    //NGX_HTTP_REWRITE_PHASE��Ӧ����location{}���е�rewrite���ã�����Ϊr->loc_conf[]ָ�����location{]�������е�loc_conf��
    //��NGX_HTTP_FIND_CONFIG_PHASE���ҵ�uri��Ӧ��location{}���ã��Ӷ�NGX_HTTP_REWRITE_PHASE�ܹ�ִ�����location{}�е�rewrite����
    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_rewrite_module);
    if (rlcf->codes == NULL) { //˵��û����ʹ�õı���
        return NGX_DECLINED;
    }

    e = ngx_pcalloc(r->pool, sizeof(ngx_http_script_engine_t));
    if (e == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* sp��һ��ngx_http_variable_value_t�����飬���汣���˴������з������һЩ����   
    ��һЩ�м������ڵ�ǰ�����п��Կ��Է�����õ�֮ǰ����֮��ı���(ͨ��sp--����sp++)  */
    e->sp = ngx_pcalloc(r->pool,
                        rlcf->stack_size * sizeof(ngx_http_variable_value_t));
    if (e->sp == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* �����������ý������������õ�һЩ����ṹ�壬�����rlcf->codes��һ�����飬ע����ǣ���Щ�ṹ��ĵ�һ����Ա����һ������handler��
    ���ﴦ��ʱ�����Ὣ�ýṹ������ǿת���õ��䴦��handler��Ȼ����˳������ִ��֮   */
    e->ip = rlcf->codes->elts;  

    e->request = r; // ��Ҫ���������  
    e->quote = 1; // ��ʼʱ��Ϊuri��Ҫ���⴦������escape������urldecode���� 
    e->log = rlcf->log;
    // ���洦�����ʱ���ܳ��ֵ�һЩhttp response code���Ա�����ض��Ĵ���    
    e->status = NGX_DECLINED;

    /*
    ���ζ�e->ip �����еĲ�ͬ�ṹ���д����ڴ���ʱͨ������ǰ�ṹ����ǿת���Ϳ��Եõ�����Ĵ���handler����Ϊÿ���ṹ�ĵ�һ��
    ��������һ��handler�����ǿ�   �ĳ�����Щ�ṹ��Ա����ƶ�����������ͼ�ġ�
     */
    while (*(uintptr_t *) e->ip) {
    //����ngx_http_script_value_code; ngx_http_script_set_var_code��������ʵ����ִ��code�ж����e->ip�ƶ���Ӧ��ngx_http_script_xxx_code_t���ȣ��Ӷ�
    //ִ����һ��ngx_http_script_xxx_code_t

    /*
    ����Ĭ�����е�ngx_http_scriptxxx_codet�ṹ���һ���ֶαض�Ϊ�ص�����ָ�룬�����������Լ��Ľű����湦�ܲ��裬������Ҫע�⡣
     */
        code = *(ngx_http_script_code_pt *) e->ip;
        code(e); //e->ip�Ǵ�ngx_http_rewrite_loc_conf_t->codes->elts������ȡ����
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "ngx_http_rewrite_handler e->status:%d, r->err_status:%ui", e->status, r->err_status);
    //Ĭ���Ƿ���NGX_DECLINED��������code()���޸���e->status������޸���status������ִ�иú�����ngx_http_core_rewrite_phase�н�������
    if (e->status < NGX_HTTP_BAD_REQUEST) {
        return e->status;
    }

    if (r->err_status == 0) {
        return e->status;
    }

    return r->err_status;
}


static ngx_int_t
ngx_http_rewrite_var(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_http_variable_t          *var;
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_rewrite_loc_conf_t  *rlcf;

    rlcf = ngx_http_get_module_loc_conf(r, ngx_http_rewrite_module);

    if (rlcf->uninitialized_variable_warn == 0) {
        *v = ngx_http_variable_null_value;
        return NGX_OK;
    }

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    var = cmcf->variables.elts;

    /*
     * the ngx_http_rewrite_module sets variables directly in r->variables,
     * and they should be handled by ngx_http_get_indexed_variable(),
     * so the handler is called only if the variable is not initialized
     */

    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                  "using uninitialized \"%V\" variable", &var[data].name);

    *v = ngx_http_variable_null_value;

    return NGX_OK;
}


static void *
ngx_http_rewrite_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_rewrite_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_rewrite_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->stack_size = NGX_CONF_UNSET_UINT;
    conf->log = NGX_CONF_UNSET;
    conf->uninitialized_variable_warn = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_rewrite_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_rewrite_loc_conf_t *prev = parent;
    ngx_http_rewrite_loc_conf_t *conf = child;

    uintptr_t  *code;

    ngx_conf_merge_value(conf->log, prev->log, 0);
    ngx_conf_merge_value(conf->uninitialized_variable_warn,
                         prev->uninitialized_variable_warn, 1);
    ngx_conf_merge_uint_value(conf->stack_size, prev->stack_size, 10);

    if (conf->codes == NULL) {
        return NGX_CONF_OK;
    }

    if (conf->codes == prev->codes) {
        return NGX_CONF_OK;
    }

    code = ngx_array_push_n(conf->codes, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_CONF_ERROR;
    }

    *code = (uintptr_t) NULL;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_rewrite_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rewrite_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rewrite_handler;

    return NGX_OK;
}

/*
1. ����������ʽ����ȡ��ģʽ��������ģʽ����variables�ȣ�
2.	�������ĸ�����last,break�ȡ�
3.����ngx_http_script_compile��Ŀ���ַ�������Ϊ�ṹ����codes������飬�Ա����ʱ���м��㣻
4.���ݵ������Ľ��������lcf->codes �飬����rewriteʱ��һ����Ľ���ƥ�伴�ɡ�ʧ���Զ��������飬������һ��rewrite
*/ //ngx_http_rewrite_handler�л�ִ�иú����е����code
static char *
ngx_http_rewrite(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)   
{//����Ľ�����://����rewrite  ^(/xyz/aa.*)$   http://$http_host/aa.mp4   break;Ϊ��
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    ngx_str_t                         *value;
    ngx_uint_t                         last;
    ngx_regex_compile_t                rc; 
    ngx_http_script_code_pt           *code;
    ngx_http_script_compile_t          sc;
    ngx_http_script_regex_code_t      *regex;
    ngx_http_script_regex_end_code_t  *regex_end;
    u_char                             errstr[NGX_MAX_CONF_ERRSTR];

    //�ú����е�lcf->codes���ngx_http_script_regex_start_code
    /*regex��codes[]�л�ȡ�ռ�,��ע�������codes��Ӧ����rewrite����������ڵ�server{}����location�е��������У����粻ͬlocation{}
    �е�rewrite������������Ĵ��ڲ�ͬlocation�� */
    regex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                       sizeof(ngx_http_script_regex_code_t)); 
    if (regex == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(regex, sizeof(ngx_http_script_regex_code_t));

    value = cf->args->elts;

    //ngx_http_rewrite�У�rewrite aaa bbb break;�����У�aaa����ʹ��ngx_regex_compile_t��bbb����ʹ��ngx_http_script_compile_t
    ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

    rc.pattern = value[1];//��¼ ^(/xyz/aa.*)$
    rc.err.len = NGX_MAX_CONF_ERRSTR;
    rc.err.data = errstr;

    /* TODO: NGX_REGEX_CASELESS */
    //����������ʽ����дngx_http_regex_t�ṹ�����ء���������������ģʽ�ȶ��������ˡ�
    regex->regex = ngx_http_regex_compile(cf, &rc);
    if (regex->regex == NULL) {
        return NGX_CONF_ERROR;
    }

    //ngx_http_script_regex_start_code����ƥ��������ʽ������Ŀ���ַ������Ȳ�����ռ䡣
	//��������Ϊ��һ��code���������Ŀ���ַ�����С��β������ngx_http_script_regex_end_code
    regex->code = ngx_http_script_regex_start_code;
    regex->uri = 1;
    regex->name = value[1];

    if (value[2].data[value[2].len - 1] == '?') {//���Ŀ�������������ʺý�β����nginx���´�������������

        /* the last "?" drops the original arguments */
        value[2].len--;

    } else {
        regex->add_args = 1;//�Զ�׷�Ӳ�����
    }

    last = 0;

    if (ngx_strncmp(value[2].data, "http://", sizeof("http://") - 1) == 0
        || ngx_strncmp(value[2].data, "https://", sizeof("https://") - 1) == 0
        || ngx_strncmp(value[2].data, "$scheme", sizeof("$scheme") - 1) == 0)
    {//nginx�жϣ��������http://�ȿ�ͷ��rewrite���ʹ����ǿ����ض��򡣻���302����
        regex->status = NGX_HTTP_MOVED_TEMPORARILY; //���������ظ�302��������յ��󣬻�Ѵ���������������ͻ������µ��ض����ַ
        regex->redirect = 1; 
        last = 1;
    }

    /*
     ��last - �����дָ�֮��������Ӧ��URI��location��
    ��break - �����дָ�
    ��redirect - ����302��ʱ�ض�������滻�ֶ���http://��ͷ��ʹ�á�
    ��permanent - ����301�����ض���

      ����Ϊ:
      location ~* /1mytest  {			
            rewrite   ^.*$ www.11.com/ last;		
       }  
      uriΪ:http://10.135.10.167/1mytest ,��ִ����ngx_http_script_regex_end_code��uri���Ϊwww.11.com/

      last����break���ã�nginx�����ִ�к��������phase���̣���ȥ����www.11.com/Ŀ¼·��������Դ
      redirect����permanent���ã�statusֵ���Ϊ301 ���� 302����ngx_http_core_rewrite_phase��ִ����code�󣬻�ֱ�ӷ����ض����ַwww.11.com/���ͻ���
        �ͻ�����������������󱾷���������uriΪwww.11.com/(�����ҳ���Ϊhttp://10.135.10.167/www.11.com)�����������www.galaxywind.com/��Ϊhttp://www.11.com/,�򲻻����󱾷�����������ֱ����ת��http://www.11.com/
     */
    if (cf->args->nelts == 4) {
        if (ngx_strcmp(value[3].data, "last") == 0) {  // ����rewrite   ^.*$ www.galaxywind.com last;�ͻ���ִ��rewrite
            last = 1;

        } else if (ngx_strcmp(value[3].data, "break") == 0) {
            /*
                ��ngx_http_core_post_rewrite_phase�оͲ���ִ�������if��䣬Ҳ�Ͳ����ٴ��ߵ��ٴ���rewrite��find config�Ĺ����ˣ����Ǽ��������������̡�
               */
            regex->break_cycle = 1;
            last = 1;

        } else if (ngx_strcmp(value[3].data, "redirect") == 0) {
            regex->status = NGX_HTTP_MOVED_TEMPORARILY; //���������ظ�302��������յ��󣬻�Ѵ���������������ͻ������µ��ض����ַ
            regex->redirect = 1;
            last = 1;

        } else if (ngx_strcmp(value[3].data, "permanent") == 0) {
            regex->status = NGX_HTTP_MOVED_PERMANENTLY;
            regex->redirect = 1;
            last = 1;

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }
    }

    //ngx_http_rewrite�У�rewrite aaa bbb break;�����У�aaa����ʹ��ngx_regex_compile_t��bbb����ʹ��ngx_http_script_compile_t
    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = &value[2];//�ַ��� http://$http_host/aa.mp4
    sc.lengths = &regex->lengths; //�����������������һЩ�����Ŀ���ַ������ȵĺ����ص������ϻ��������: ���� ���� ����
    sc.values = &lcf->codes;//����ģʽ�������
    sc.variables = ngx_http_script_variables_count(&value[2]);
    sc.main = regex; //���Ƕ���ı��ʽ�����������lengths�ȡ�
    sc.complete_lengths = 1;// complete_lengths��1����Ϊ�˸�lengths�����β(��nullָ�����)����Ϊ��������������еĳ�Աʱ������NULLʱ��ִ�оͽ����ˡ� 
    sc.compile_args = !regex->redirect;

    //rewrite  ^(.*)$   http://$http_host.mp4   break;�е�http://$http_host.mp4��ͨ��ngx_http_script_compile��������ص�code��ӵ�lcf->codes[]��
    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    regex = sc.main;//������ô����ԭ���ǿ��������ı��ڴ��ַ��

    regex->size = sc.size;
    regex->args = sc.args;

    if (sc.variables == 0 && !sc.dup_capture) {//���û�б������Ǿͽ�lengths�ÿգ������Ͳ�������������������ֱ�ӽ����ַ�������codes
        regex->lengths = NULL;
    }

    regex_end = ngx_http_script_add_code(lcf->codes,
                                      sizeof(ngx_http_script_regex_end_code_t),
                                      &regex);
    if (regex_end == NULL) {
        return NGX_CONF_ERROR;
    }

    /*��������Ĵ��������rewrite����������µĺ����ṹ: rewrite   ^(.*)$   http://$http_host.mp4   break;
	ngx_http_script_regex_start_code ��������������ʽ������lengths����ܳ��ȣ�����ռ䡣
			ngx_http_script_copy_len_code		7
			ngx_http_script_copy_var_len_code 	18
			ngx_http_script_copy_len_code		4	=== 29 

	ngx_http_script_copy_code		����"http://" ��e->buf
	ngx_http_script_copy_var_code	����"115.28.34.175:8881"
	ngx_http_script_copy_code 		����".mp4"
	ngx_http_script_regex_end_code
	*/
    regex_end->code = ngx_http_script_regex_end_code;//�����ص�����Ӧǰ��Ŀ�ʼ��
    regex_end->uri = regex->uri;
    regex_end->args = regex->args;
    regex_end->add_args = regex->add_args;//�Ƿ���Ӳ�����
    regex_end->redirect = regex->redirect;

    if (last) {//�ο����棬���rewrite ĩβ��last,break,�ȣ��Ͳ����ٴν�������������ˣ���ô���ͽ�code����Ϊ�ա�
        code = ngx_http_script_add_code(lcf->codes, sizeof(uintptr_t), &regex);
        if (code == NULL) {
            return NGX_CONF_ERROR;
        }

        *code = NULL;
    }

    //��һ�����������ĵ�ַ�����ƥ��ʧ�ܣ����ֱ��������regexƥ����ص�����code
    regex->next = (u_char *) lcf->codes->elts + lcf->codes->nelts
                                              - (u_char *) regex;

    return NGX_CONF_OK;
}

//return code��
//ע��codeΪngx_http_script_return_code
static char *
ngx_http_rewrite_return(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    u_char                            *p;
    ngx_str_t                         *value, *v;
    ngx_http_script_return_code_t     *ret;
    ngx_http_compile_complex_value_t   ccv;

    ret = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                     sizeof(ngx_http_script_return_code_t));
    if (ret == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts; // return code �е�code��һ���Ƿ����� 204��400��402-406��408��410, 411, 413, 416��500-504

    ngx_memzero(ret, sizeof(ngx_http_script_return_code_t));

    ret->code = ngx_http_script_return_code;

    p = value[1].data;

    ret->status = ngx_atoi(p, value[1].len);

    if (ret->status == (uintptr_t) NGX_ERROR) {

        if (cf->args->nelts == 2
            && (ngx_strncmp(p, "http://", sizeof("http://") - 1) == 0
                || ngx_strncmp(p, "https://", sizeof("https://") - 1) == 0
                || ngx_strncmp(p, "$scheme", sizeof("$scheme") - 1) == 0))
        {
            ret->status = NGX_HTTP_MOVED_TEMPORARILY;
            v = &value[1];

        } else {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid return code \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

    } else {

        if (ret->status > 999) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid return code \"%V\"", &value[1]);
            return NGX_CONF_ERROR;
        }

        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        v = &value[2];
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = v;
    ccv.complex_value = &ret->text;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

//���ngx_http_rewrite_handler�����룬���Կ����������һ��code�ڵ㵽codes���飬��ô��ngx_http_rewrite_handler��forѭ��ִ�е���
//�ڵ�code��ʱ�򣬾ͻ��e->ip��ΪNULL��������ֱ���˳�while (*(uintptr_t *) e->ip){}ѭ��
static char *
ngx_http_rewrite_break(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t *lcf = conf;

    ngx_http_script_code_pt  *code;

    code = ngx_http_script_start_code(cf->pool, &lcf->codes, sizeof(uintptr_t));
    if (code == NULL) {
        return NGX_CONF_ERROR;
    }

    *code = ngx_http_script_break_code;

    return NGX_CONF_OK;
}

/*
location / {
    if ($uri ~* "(.*).html$" ) {
       set  $file  $1;
           rewrite ^(.*)$ http://$http_host$file.mp4 break;
    }
}
������������ã�cf->ctx�����ģ���ʵ��locatin{}���ÿ��������
*///if�������̲ο�http://blog.sina.com.cn/s/blog_7303a1dc0101cm9z.html
static char * //if�Ľ������̺�location{}�������̲��
ngx_http_rewrite_if(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) //�������Ľ�����������ı�ע�е�����Ϊ��
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    void                         *mconf;
    char                         *rv;
    u_char                       *elts;
    ngx_uint_t                    i;
    ngx_conf_t                    save;
    ngx_http_module_t            *module;
    ngx_http_conf_ctx_t          *ctx, *pctx;
    ngx_http_core_loc_conf_t     *clcf, *pclcf;
    ngx_http_script_if_code_t    *if_code;
    ngx_http_rewrite_loc_conf_t  *nlcf;

    //if�Ľ������̺�location{}�������̲��,Ҳ��ctx
    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    pctx = cf->ctx; //����{}��������ctx
    ctx->main_conf = pctx->main_conf;
    ctx->srv_conf = pctx->srv_conf;

    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    for (i = 0; ngx_modules[i]; i++) {
        if (ngx_modules[i]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[i]->ctx;

        if (module->create_loc_conf) {
            /*
               �ڽ���ifʱ�� nginx���������һ��location���Դ��ģ���������location typeΪnoname��ͨ��ngx_http_add_location���á�location����
               �ӵ��ϲ��locations�С����ｫif����location��Ȼ�����ĺ����ԣ���Ϊif������Ҳ����Ҫ����urlƥ��ġ�
               */
            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                 return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[i]->ctx_index] = mconf;
        }
    }

    pclcf = pctx->loc_conf[ngx_http_core_module.ctx_index];//��if{}����location{}��������Ϣ

    clcf = ctx->loc_conf[ngx_http_core_module.ctx_index]; //if{}��������Ϣ
    clcf->loc_conf = ctx->loc_conf;
    clcf->name = pclcf->name;
    clcf->noname = 1; //if���ñ���Ϊlocation��noname��ʽ

    if (ngx_http_add_location(cf, &pclcf->locations, clcf) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (ngx_http_rewrite_if_condition(cf, lcf) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    if_code = ngx_array_push_n(lcf->codes, sizeof(ngx_http_script_if_code_t));
    if (if_code == NULL) {
        return NGX_CONF_ERROR;
    }

    if_code->code = ngx_http_script_if_code;

    elts = lcf->codes->elts;


    /* the inner directives must be compiled to the same code array */

    nlcf = ctx->loc_conf[ngx_http_rewrite_module.ctx_index];
    nlcf->codes = lcf->codes;


    save = *cf;
    cf->ctx = ctx;

    if (pclcf->name.len == 0) {
        if_code->loc_conf = NULL;
        cf->cmd_type = NGX_HTTP_SIF_CONF;

    } else {
        if_code->loc_conf = ctx->loc_conf;
        cf->cmd_type = NGX_HTTP_LIF_CONF;
    }

    rv = ngx_conf_parse(cf, NULL);

    *cf = save;

    if (rv != NGX_CONF_OK) {
        return rv;
    }


    if (elts != lcf->codes->elts) {
        if_code = (ngx_http_script_if_code_t *)
                   ((u_char *) if_code + ((u_char *) lcf->codes->elts - elts));
    }

    if_code->next = (u_char *) lcf->codes->elts + lcf->codes->nelts
                                                - (u_char *) if_code;

    /* the code array belong to parent block */

    nlcf->codes = NULL;

    return NGX_CONF_OK;
}


static char *
ngx_http_rewrite_if_condition(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf)
{
    u_char                        *p;
    size_t                         len;
    ngx_str_t                     *value;
    ngx_uint_t                     cur, last;
    ngx_regex_compile_t            rc;
    ngx_http_script_code_pt       *code;
    ngx_http_script_file_code_t   *fop;
    ngx_http_script_regex_code_t  *regex;
    u_char                         errstr[NGX_MAX_CONF_ERRSTR];

    value = cf->args->elts;
    last = cf->args->nelts - 1;

    if (value[1].len < 1 || value[1].data[0] != '(') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    if (value[1].len == 1) {
        cur = 2;

    } else {
        cur = 1;
        value[1].len--;
        value[1].data++;
    }

    if (value[last].len < 1 || value[last].data[value[last].len - 1] != ')') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[last]);
        return NGX_CONF_ERROR;
    }

    if (value[last].len == 1) {
        last--;

    } else {
        value[last].len--;
        value[last].data[value[last].len] = '\0';
    }

    len = value[cur].len;
    p = value[cur].data;

    if (len > 1 && p[0] == '$') {

        if (cur != last && cur + 2 != last) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid condition \"%V\"", &value[cur]);
            return NGX_CONF_ERROR;
        }

        if (ngx_http_rewrite_variable(cf, lcf, &value[cur]) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        if (cur == last) {
            return NGX_CONF_OK;
        }

        cur++;

        len = value[cur].len;
        p = value[cur].data;

        if (len == 1 && p[0] == '=') {

            if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
                return NGX_CONF_ERROR;
            }

            code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                              sizeof(uintptr_t));
            if (code == NULL) {
                return NGX_CONF_ERROR;
            }

            *code = ngx_http_script_equal_code;

            return NGX_CONF_OK;
        }

        if (len == 2 && p[0] == '!' && p[1] == '=') {

            if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
                return NGX_CONF_ERROR;
            }

            code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                              sizeof(uintptr_t));
            if (code == NULL) {
                return NGX_CONF_ERROR;
            }

            *code = ngx_http_script_not_equal_code;
            return NGX_CONF_OK;
        }

        if ((len == 1 && p[0] == '~')
            || (len == 2 && p[0] == '~' && p[1] == '*')
            || (len == 2 && p[0] == '!' && p[1] == '~')
            || (len == 3 && p[0] == '!' && p[1] == '~' && p[2] == '*'))
        {
            regex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                         sizeof(ngx_http_script_regex_code_t));
            if (regex == NULL) {
                return NGX_CONF_ERROR;
            }

            ngx_memzero(regex, sizeof(ngx_http_script_regex_code_t));

            ngx_memzero(&rc, sizeof(ngx_regex_compile_t));

            rc.pattern = value[last];
            rc.options = (p[len - 1] == '*') ? NGX_REGEX_CASELESS : 0;
            rc.err.len = NGX_MAX_CONF_ERRSTR;
            rc.err.data = errstr;

            regex->regex = ngx_http_regex_compile(cf, &rc);
            if (regex->regex == NULL) {
                return NGX_CONF_ERROR;
            }

            regex->code = ngx_http_script_regex_start_code;
            regex->next = sizeof(ngx_http_script_regex_code_t);
            regex->test = 1;
            if (p[0] == '!') {
                regex->negative_test = 1;
            }
            regex->name = value[last];

            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "unexpected \"%V\" in condition", &value[cur]);
        return NGX_CONF_ERROR;

    } else if ((len == 2 && p[0] == '-')
               || (len == 3 && p[0] == '!' && p[1] == '-'))
    {
        if (cur + 1 != last) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid condition \"%V\"", &value[cur]);
            return NGX_CONF_ERROR;
        }

        value[last].data[value[last].len] = '\0';
        value[last].len++;

        if (ngx_http_rewrite_value(cf, lcf, &value[last]) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        fop = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                          sizeof(ngx_http_script_file_code_t));
        if (fop == NULL) {
            return NGX_CONF_ERROR;
        }

        fop->code = ngx_http_script_file_code;

        if (p[1] == 'f') {
            fop->op = ngx_http_script_file_plain;
            return NGX_CONF_OK;
        }

        if (p[1] == 'd') {
            fop->op = ngx_http_script_file_dir;
            return NGX_CONF_OK;
        }

        if (p[1] == 'e') {
            fop->op = ngx_http_script_file_exists;
            return NGX_CONF_OK;
        }

        if (p[1] == 'x') {
            fop->op = ngx_http_script_file_exec;
            return NGX_CONF_OK;
        }

        if (p[0] == '!') {
            if (p[2] == 'f') {
                fop->op = ngx_http_script_file_not_plain;
                return NGX_CONF_OK;
            }

            if (p[2] == 'd') {
                fop->op = ngx_http_script_file_not_dir;
                return NGX_CONF_OK;
            }

            if (p[2] == 'e') {
                fop->op = ngx_http_script_file_not_exists;
                return NGX_CONF_OK;
            }

            if (p[2] == 'x') {
                fop->op = ngx_http_script_file_not_exec;
                return NGX_CONF_OK;
            }
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid condition \"%V\"", &value[cur]);
        return NGX_CONF_ERROR;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid condition \"%V\"", &value[cur]);

    return NGX_CONF_ERROR;
}


static char *
ngx_http_rewrite_variable(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf,
    ngx_str_t *value)
{
    ngx_int_t                    index;
    ngx_http_script_var_code_t  *var_code;

    value->len--;
    value->data++;

    index = ngx_http_get_variable_index(cf, value);

    if (index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    var_code = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                          sizeof(ngx_http_script_var_code_t));
    if (var_code == NULL) {
        return NGX_CONF_ERROR;
    }

    var_code->code = ngx_http_script_var_code;
    var_code->index = index;

    return NGX_CONF_OK;
}

/*Syntax:	set $variable value
1. ��$variable���뵽����ϵͳ�У�cmcf->variables_keys->keys��cmcf->variables��


a. ���value�Ǽ��ַ�������ô����֮��lcf->codes�ͻ�׷�������ĵ�����: 
	ngx_http_script_value_code  ֱ�Ӽ��ַ���ָ��һ�¾��У������ÿ����ˡ�
b. ���value�Ǹ��ӵİ��������Ĵ�����ôlcf->codes�ͻ�׷�����µĽ�ȥ :
	ngx_http_script_complex_value_code  ����lengths��lcode��ȡ����ַ������ܳ��ȣ����������ڴ�
		lengths
	values��������ݱ��ʽ�Ĳ�ͬ����ͬ�� �ֱ�value����ĸ��ӱ��ʽ��ֳ��﷨��Ԫ������һ������ֵ�����ϲ���һ��
	ngx_http_script_set_var_code		���������ϲ��������ս�����õ�variables[]������ȥ��
*/
static char *
ngx_http_rewrite_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_rewrite_loc_conf_t  *lcf = conf;

    ngx_int_t                            index;
    ngx_str_t                           *value;
    ngx_http_variable_t                 *v;
    ngx_http_script_var_code_t          *vcode;
    ngx_http_script_var_handler_code_t  *vhcode;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {//����������$��ͷ
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;
    //������������������������뵽cmcf->variables_keys->keys���档
    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }
    //������뵽cmcf->variables���棬���������±�
    index = ngx_http_get_variable_index(cf, &value[1]);
    if (index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    //set $variable value�еĵ�һ������$variable��Ӧ�����������ngx_http_variables_init_vars����ngx_http_variable_t��get_handler��data��Ա
    if (v->get_handler == NULL
        && ngx_strncasecmp(value[1].data, (u_char *) "http_", 5) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "sent_http_", 10) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "upstream_http_", 14) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "cookie_", 7) != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "upstream_cookie_", 16)
           != 0
        && ngx_strncasecmp(value[1].data, (u_char *) "arg_", 4) != 0)
        //����������Ʋ������Ͽ�ͷ������get_handlerΪngx_http_rewrite_var��dataΪindex ��
    {
        //����һ��Ĭ�ϵ�handler����ngx_http_variables_init_vars������ʵ�ǻὫ������Щ"http_" "sent_http_"��Щ����get_hendler��
        v->get_handler = ngx_http_rewrite_var;
        v->data = index;
    }

/*
    �ű�������һϵ�еİ��������Լ�������ݣ����Ǳ���֯��ngx_httpscript_ xxx_codet�����Ľṹ�壬������ֲ�ͬ���ܵĲ�
�����裩���������ڱ���lcf->codes�����ڣ���ngx_httprewrite_loc_conf_t���ͱ���Icf���뵱ǰlocation������ģ���������ű�����ֻ��
���ͻ���������ʵ�ǰ���locationʱ�Żᱻ����ִ�С����������У���set $file t_a;�������Ľű�����ֻ�е��ͻ����������/t��¼ʱ�Ż�
��������������ͻ���������ʸ�Ŀ¼ʱ���������޹�ϵ��
       location / {
			root web;
        }
       location /t {
			set $file t_a;
       }
*/
    //ngx_http_rewrite_handler�л��Ƴ�ִ��lcf->codes�����еĸ���ngx_http_script_xxx_code_t->code������

    //set $variable value��value���������ﴦ�� ,

    /*
    ��������Կ���ûsetһ�ξͻᴴ��һ��ngx_http_script_var_code_t��ngx_http_script_xxx_value_code_t������������������ͬ����
    ������ͬ��ֵ����ô�ͻ��ж��var_code_t��value_code_t�ԣ�ʵ������ngx_http_rewrite_handler����ִ�е�ʱ����������Ϊ׼������:
    50��    location / {
    51��        root    web;
    52:         set $file indexl.html;
    53��        index $file;
    54��
    65:         set $file  index2.html;
            }
    ���������׷�ٷ��ʵ�����index2.html
    */

    /*
    ���set $variable value�е�value����ͨ�ַ������������ngx_http_rewrite_value��ngx_http_rewrite_loc_conf_t->codes�����л�ȡngx_http_script_value_code_t�ռ䣬�������ں����
ngx_http_script_start_code����ͬ����ngx_http_rewrite_loc_conf_t->codes�����л�ȡngx_http_script_var_code_t�ռ䣬�����codes������
��ű���ֵvalue��ngx_http_script_value_code_t�ռ�����var��������ngx_http_script_var_code_t�ڿռ����ǿ��ŵģ�ͼ�λ���<��������nginx ͼ8-4>

     ���set $variable value�е�value�Ǳ��������������ngx_http_rewrite_value��ngx_http_rewrite_loc_conf_t->codes�����л�ȡngx_http_script_complex_value_code_t�ռ䣬�������ں����
 ngx_http_script_start_code����ͬ����ngx_http_rewrite_loc_conf_t->codes�����л�ȡngx_http_script_complex_value_code_t�ռ䣬�����codes������
 ��ű���ֵvalue��ngx_http_script_value_code_t�ռ�����var��������ngx_http_script_var_code_t�ڿռ����ǿ��ŵģ�ͼ�λ���<��������nginx ͼ8-4>
     *///
    if (ngx_http_rewrite_value(cf, lcf, &value[2]) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    if (v->set_handler) {
        vhcode = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                   sizeof(ngx_http_script_var_handler_code_t));
        if (vhcode == NULL) {
            return NGX_CONF_ERROR;
        }

        vhcode->code = ngx_http_script_var_set_handler_code;
        vhcode->handler = v->set_handler;
        vhcode->data = v->data;

        return NGX_CONF_OK;
    }

    vcode = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                       sizeof(ngx_http_script_var_code_t));
    if (vcode == NULL) {
        return NGX_CONF_ERROR;
    }

    vcode->code = ngx_http_script_set_var_code;
    vcode->index = (uintptr_t) index;

    return NGX_CONF_OK;
}

//set $varialbe value�У�ngx_http_rewrite_value������value
//ͼ�λ��ο�http://blog.csdn.net/brainkick/article/details/7065244
static char *
ngx_http_rewrite_value(ngx_conf_t *cf, ngx_http_rewrite_loc_conf_t *lcf,
    ngx_str_t *value)
{
    ngx_int_t                              n;
    ngx_http_script_compile_t              sc;
    ngx_http_script_value_code_t          *val;
    ngx_http_script_complex_value_code_t  *complex;

    n = ngx_http_script_variables_count(value);

    if (n == 0) {//���û�б������Ǹ����ַ���������lcf->codes,
        val = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                         sizeof(ngx_http_script_value_code_t));
        if (val == NULL) {
            return NGX_CONF_ERROR;
        }

        n = ngx_atoi(value->data, value->len); //

        if (n == NGX_ERROR) {
            n = 0;
        }

        val->code = ngx_http_script_value_code; 
        val->value = (uintptr_t) n; //���value���������ַ�������nΪ�ַ���ת�����Ӧ������
        val->text_len = (uintptr_t) value->len;
        val->text_data = (uintptr_t) value->data;

        return NGX_CONF_OK;
    }

    //����$�ı�����valueҲ�Ǳ�������set $aa $bb�������$bb���Ǳ�����
    complex = ngx_http_script_start_code(cf->pool, &lcf->codes,
                                 sizeof(ngx_http_script_complex_value_code_t));
    //ʵ�������valueҲ�Ǳ���������������ngx_http_script_start_code�������ngx_http_script_compile�õ�����lcf->codes����ڵ㣬
    //��ngx_http_rewrite_handler�л�����ִ��������codes����ڵ�
    if (complex == NULL) {
        return NGX_CONF_ERROR;
    }

    complex->code = ngx_http_script_complex_value_code;
    complex->lengths = NULL;

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    sc.cf = cf;
    sc.source = value;  
    // lengths��values�������ngx_http_script_compile��ִ�й����б����   
    sc.lengths = &complex->lengths; //�ں����ngx_http_script_compile�а�value������
    sc.values = &lcf->codes; 
    //ʵ�������valueҲ�Ǳ���������������ngx_http_script_start_code�������ngx_http_script_compile�õ�����lcf->codes����ڵ� ��ngx_http_rewrite_handler�л�����ִ��������codes����ڵ�
    sc.variables = n;
    // complete_lengths��1����Ϊ�˸�lengths�����β(��nullָ�����)����Ϊ��������������еĳ�Աʱ������NULLʱ��ִ�оͽ����ˡ� 
    sc.complete_lengths = 1;

    //�ڸú���ngx_http_script_add_copy_code�л�Ѵ���������ngx_http_script_copy_code_t���뵽�����lcf->codes[]->lengths��lcf->codes��
    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

