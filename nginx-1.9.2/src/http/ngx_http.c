
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
Nginx配置文件详解

#运行用户
user nobody nobody;
#启动进程
worker_processes 2;
#全局错误日志及PID文件
error_log logs/error.log notice;
pid logs/nginx.pid;
#工作模式及连接数上限
events {
use epoll;
worker_connections 1024;
}
#设定http服务器，利用它的反向代理功能提供负载均衡支持
http {
#设定mime类型
include conf/mime.types;
default_type application/octet-stream;
#设定日志格式
log_format main ‘$remote_addr – $remote_user [$time_local] ‘
‘”$request” $status $bytes_sent ‘
‘”$http_referer” “$http_user_agent” ‘
‘”$gzip_ratio”‘;
log_format download ‘$remote_addr – $remote_user [$time_local] ‘
‘”$request” $status $bytes_sent ‘
‘”$http_referer” “$http_user_agent” ‘
‘”$http_range” “$sent_http_content_range”‘;
#设定请求缓冲
client_header_buffer_size 1k;
large_client_header_buffers 4 4k;

#开启gzip模块
gzip on;
gzip_min_length 1100;
gzip_buffers 4 8k;
gzip_types text/plain;
output_buffers 1 32k;
postpone_output 1460;
#设定access log
access_log logs/access.log main;
client_header_timeout 3m;
client_body_timeout 3m;
send_timeout 3m;
sendfile on;
tcp_nopush on;
tcp_nodelay on;
keepalive_timeout 65;
#设定负载均衡的服务器列表
upstream mysvr {
#weigth参数表示权值，权值越高被分配到的几率越大
#本机上的Squid开启3128端口
server 192.168.8.1:3128 weight=5;
server 192.168.8.2:80 weight=1;
server 192.168.8.3:80 weight=6;
}

#设定虚拟主机
server {
listen 80;
server_name 192.168.8.1 www.hahaer.com;
charset gb2312;
#设定本虚拟主机的访问日志
access_log logs/www.hahaer.com.access.log main;
#如果访问 /img/ *, /js/ *, /css/ * 资源，则直接取本地文件，不通过squid
#如果这些文件较多，不推荐这种方式，因为通过squid的缓存效果更好
location ~ ^/(img|js|css)/ {
root /data3/Html;
expires 24h;
}

#对 “/” 启用负载均衡
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

#设定查看Nginx状态的地址
location /NginxStatus {
stub_status on;
access_log on;
auth_basic “NginxStatus”;
auth_basic_user_file conf/htpasswd;
}
}
}

备注：conf/htpasswd 文件的内容用 apache 提供的 htpasswd 工具来产生即可。

3.) 查看 Nginx 运行状态

输入地址 http://192.168.8.1/NginxStatus/，输入验证帐号密码，即可看到类似如下内容：



Active connections: 328
server accepts handled requests
9309 8982 28890
Reading: 1 Writing: 3 Waiting: 324
*/

/*
这时，HTTP框架会为所有的HTTP模块建立3个数组，分别存放所有HTTP模块的ngx_http_module_t中的
create_main_conf、create_srv_conf、create_loc_conf方法返回的地址指针（就像本章的例子
中mytest模块在create_loc_conf中生成了ngx_http_mytest_conf_t结构，并在create_lo_conf方法返回时将指针传递给HTTP框架）。
当然，如果HTTP模块对于配置项不感兴趣，
它没有实现create_main_conf、create_srv_conf、create_loc_conf等方法，那么数组中相应位
置存储的指针是NULL。ngx_http_conf_ctx_t的3个成员main_conf、srv_conf、loc_conf分
别指向这3个数组。下面看一段简化的代码，了解如何设置create_loc_conf返回的地址。
ngx_http_conf_ctx_t *ctx;
//HTTP框架生成了1个ngx_ht tp_conf_ctxt结构
ctx=ngx_pcalloc (cf->pool,  sizeof (ngx_http_conf_ctx_t))j
if (ctx==NULL){
    return NGX_CONF_ERROR;
)
／／生成1个数组存储所有的HTTP模块create_loc_conf方法返回的地址
ctx->loc_conf=ngx_pcalloc (cf->pool,  sizeof (void*)  * ngx_http_max_module);
if (ctx->loc conf==NULL)  {
    return NGX_CONF_ERROR;
)
*/
/*当Nginx检测到http{．．．）这个关键配置项时，HTTP配置模型就启动了，这时会首先建立1个ngx_http_conf_ ctx_t结构。
在http{．．．）块中就通过1个ngx_http_conf_ctx t结构保存了所有HTTP模块的配
置数据结构的入口。以后遇到任何server{．．．）块或者location{．．．）块时，也会建立ngx_http_
conf_ctx_t结构，生成同样的数组来保存所有HTTP模块通过create_srv_ conf  create_loc_
conf等方法返回的指针地址。ngx_http_conf_ctx_t是了解http配置块的基础
*/ //该结构中的变量直接指向ngx_http_module_t中的三个create_main_conf  create_srv_conf  create_loc_conf
/*
  http {
       xxxx
       server {
            location /xxx {
            }
       }
  }
  这种情况的配置文件，在执行到http的时候开辟ngx_http_conf_ctx_t会分别调用一次main srv loc_creat，执行到server时开辟ngx_http_conf_ctx_t会调用srv_creat loc_creat, 执行到location时开辟ngx_http_conf_ctx_t会调用一次loc_creat
  所以这种情况会调用1次main_creat 2才srv_creat 3次loc_creat。

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
  这种情况的配置文件，在执行到http的时候开辟ngx_http_conf_ctx_t会分别调用一次main srv loc_creat，执行到server时开辟ngx_http_conf_ctx_t会调用srv_creat loc_creat, 执行到location时开辟ngx_http_conf_ctx_t会调用一次loc_creat
  所以这种情况会调用1次main_creat 1+2才srv_creat 1+2+2次loc_creat。
*/

/*
http{}中会调用main_conf srv_conf loc_conf分配空间，见ngx_http_block。server{}中会调用srv_conf loc_conf创
建空间,见ngx_http_core_server， location{}中会创建loc_conf空间,见ngx_http_core_location
图形化参考:深入理解NGINX中的图9-2(P302)  图10-1(P353) 图10-1(P356) 图10-1(P359)  图4-2(P145)

ngx_http_conf_ctx_t、ngx_http_core_main_conf_t、ngx_http_core_srv_conf_t、ngx_http_core_loc_conf_s和ngx_cycle_s->conf_ctx的关系见:
Nginx的http配置结构体的组织结构:http://tech.uc.cn/?p=300
*/
typedef struct { //相关空间创建和赋值见ngx_http_block, 该结构是ngx_conf_t->ctx成员。所有的配置所处内存的源头在ngx_cycle_t->conf_ctx,见ngx_init_cycle
/* 指向一个指针数组，数组中的每个成员都是由所有HTTP模块的create_main_conf方法创建的存放全局配置项的结构体，它们存放着解析直属http{}块内的main级别的配置项参数 */
    void        **main_conf;  /* 指针数组，数组中的每个元素指向所有HTTP模块ngx_http_module_t->create_main_conf方法产生的结构体 */

/* 指向一个指针数组，数组中的每个成员都是由所有HTTP模块的create_srv_conf方法创建的与server相关的结构体，它们或存放main级别配置项，
或存放srv级别的server配置项，这与当前的ngx_http_conf_ctx_t是在解析http{}或者server{}块时创建的有关 */
    void        **srv_conf;/* 指针数组，数组中的每个元素指向所有HTTP模块ngx_http_module_t->create->srv->conf方法产生的结构体 */

/*
指向一个指针数组，数组中的每个成员都是由所有HTTP模块的create_loc_conf方法创建的与location相关的结构体，它们可能存放着main、srv、loc级
别的配置项，这与当前的ngx_http_conf_ctx_t是在解析http{}、server{}或者location{}块时创建的有关存放location{}配置项
*/
    void        **loc_conf;/* 指针数组，数组中的每介元素指向所有HTTP模块ngx_http_module_t->create->loc->conf方法产生的结构体 */
} ngx_http_conf_ctx_t; //ctx是content的简称，表示上下文
//参考:http://tech.uc.cn/?p=300   ngx_http_conf_ctx_t变量的指针ctx存储在ngx_cycle_t的conf_ctx所指向的指针数组，以ngx_http_module的index为下标的数组元素
//http://tech.uc.cn/?p=300参数解析相关数据结构参考

/*
Nginx安装完毕后，会有响应的安装目录，安装目录里nginx.conf为nginx的主配置文件，ginx主配置文件分为4部分，main（全局配置）、server（主机设置）、upstream（负载均衡服务器设）和location（URL匹配特定位置的设置），这四者关系为：server继承main，location继承server，upstream既不会继承其他设置也不会被继承。

一、Nginx的main（全局配置）文件

[root@rhel6u3-7 server]# vim /usr/local/nginx/conf/nginx.conf
user nginx nginx; //指定nginx运行的用户及用户组为nginx，默认为nobody
worker_processes 2； //开启的进程数，一般跟逻辑cpu核数一致
error_log logs/error.log notice; //定于全局错误日志文件，级别以notice显示。还有debug、info、warn、error、crit模式，debug输出最多，crit输出最少，更加实际环境而定。
pid logs/nginx.pid; //指定进程id的存储文件位置
worker_rlimit_nofile 65535; //指定一个nginx进程打开的最多文件描述符数目，受系统进程的最大打开文件数量限制
events {
use epoll; 设置工作模式为epoll，除此之外还有select、poll、kqueue、rtsig和/dev/poll模式
worker_connections 65535; //定义每个进程的最大连接数 受系统进程的最大打开文件数量限制
}
…….

[root@rhel6u3-7 server]# cat /proc/cpuinfo | grep "processor" | wc –l //查看逻辑CPU核数
[root@rhel6u3-7 server]# ulimit -n 65535 //设置系统进程的最大打开文件数量

二、Nginx的HTTP服务器配置，Gzip配置。

http {
*****************************以下是http服务器全局配置*********************************
include mime.types; //主模块指令，实现对配置文件所包含的文件的设定，可以减少主配置文件的复杂度，DNS主配置文件中的zonerfc1912，acl基本上都是用的include语句
default_type application/octet-stream; //核心模块指令，这里默认设置为二进制流，也就是当文件类型未定义时使用这种方式
//下面代码为日志格式的设定，main为日志格式的名称，可自行设置，后面引用。
log_format main '$remote_addr - $remote_user [$time_local] "$request" '
'$status $body_bytes_sent "$http_referer" '
'"$http_user_agent" "$http_x_forwarded_for"';
access_log logs/access.log main; //引用日志main
client_max_body_size 20m; //设置允许客户端请求的最大的单个文件字节数
client_header_buffer_size 32k; //指定来自客户端请求头的headebuffer大小
client_body_temp_path /dev/shm/client_body_temp; //指定连接请求试图写入缓存文件的目录路径
large_client_header_buffers 4 32k; //指定客户端请求中较大的消息头的缓存最大数量和大小，目前设置为4个32KB
sendfile on; //开启高效文件传输模式
tcp_nopush on; //开启防止网络阻塞
tcp_nodelay on; //开启防止网络阻塞
keepalive_timeout 65; //设置客户端连接保存活动的超时时间
client_header_timeout 10; //用于设置客户端请求读取超时时间
client_body_timeout 10; //用于设置客户端请求主体读取超时时间
send_timeout 10; //用于设置相应客户端的超时时间
//以下是httpGzip模块配置
#httpGzip modules
gzip on; //开启gzip压缩
gzip_min_length 1k; //设置允许压缩的页面最小字节数
gzip_buffers 4 16k; //申请4个单位为16K的内存作为压缩结果流缓存
gzip_http_version 1.1; //设置识别http协议的版本,默认是1.1
gzip_comp_level 2; //指定gzip压缩比,1-9 数字越小,压缩比越小,速度越快.
gzip_types text/plain application/x-javascript text/css application/xml; //指定压缩的类型
gzip_vary on; //让前端的缓存服务器存经过gzip压缩的页面

三、nginx的server虚拟主机配置

两种方式一种是直接在主配置文件中设置server字段配置虚拟主机，另外一种是使用include字段设置虚拟主机，这样可以减少主配置文件的复杂性。

*****************************以下是server主机设置*********************************
server {
listen 80; //监听端口为80
server_name www.88181.com; //设置主机域名
charset gb2312; //设置访问的语言编码
access_log logs/www.88181.com.access.log main; //设置虚拟主机访问日志的存放路径及日志的格式为main
location / { //设置虚拟主机的基本信息
root sites/www; //设置虚拟主机的网站根目录
index index.html index.htm; //设置虚拟主机默认访问的网页
}
location /status { // 查看nginx当前的状态情况,需要模块 “--with-http_stub_status_module”支持
stub_status on;
access_log /usr/local/nginx/logs/status.log;
auth_basic "NginxStatus"; }
}
include /usr/local/nginx/server/www1.88181.com; //使用include字段设置server,内容如下
[root@rhel6u3-7 ~]# cat /usr/local/nginx/server/www1.88181.com
server {
listen 80;
server_name www1.88181.com;
location / {
root sites/www1;
index index.html index.htm;
}
}
本篇文章来源于 Linux公社网站(www.linuxidc.com)  原文链接：http://www.linuxidc.com/Linux/2013-02/80069p2.htm
*/
/*
HTTP框架在读取、重载配置文件时定义了由ngx_http_module_t接口描述的8个阶段，HTTP框架在启动过程中会在每个阶段中调用ngx_http_module_t
中相应的方法。当然，如果ngx_http_module_t中的某个回调方法设为NULL空指针，那么HTTP框架是不会调用它的。
*/
/*
不过，这8个阶段的调用顺序与上述定义的顺序是不同的。在Nginx启动过程中，HTTP框架调用这些回调方法的实际顺序有可能是这样的（与nginx.conf配置项有关）：
1）create_main_conf
2）create_srv_conf
3）create_loc_conf
4）preconfiguration
5）init_main_conf
6）merge_srv_conf
7）merge_loc_conf
8）postconfiguration
当遇到http{}配置块时，HTTP框架会调用所有HTTP模块可能实现的create main conf、create_srv_conf、
create_loc_conf方法生成存储main级别配置参数的结构体；在遇到servero{}块时会再次调用所有HTTP模
块的create_srv conf、create loc_conf回调方法生成存储srv级别配置参数的结构体；在遇到location{}时
则会再次调用create_loc_conf回调方法生成存储loc级别配置参数的结构体。
例如如下配置:
{
  http {
     server{
        location xx {
            mytest;
        }
     }
  }
}
则在解析到http{}的时候会调用一次
location{}中，则在解析到http{}的时候调用一次create_loc_conf，解析到server{}的时候会调用一次create_loc_conf
解析到location{}的时候还会调用一次create_loc_conf

事实上，Nginx预设的配置项合并方法有10个，它们的行为与上述的ngx_conf_merge_
str- value是相似的。Nginx已经实现好的10个简单的配置项合并宏，它们的
参数类型与ngx_conf_merge_str_ value -致，而且除了ngx_conf_merge_bufs value外，它们
都将接收3个参数，分别表示父配置块参数、子配置块参数、默认值。
  Nginx预设的10种配置项合并宏
┏━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃    配置项合并塞          ┃    意义                                                                  ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并可以使用等号(=)直接赋值的变量，并且该变量在create loc conf等分      ┃
┃ngx_conf_merge_value      ┃配方法中初始化为NGX CONF UNSET，这样类型的成员可以使用ngx_conf_           ┃
┃                          ┃merge_value合并宏                                                         ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并指针类型的变量，并且该变量在create loc conf等分配方法中初始化为     ┃
┃ngx_conf_merge_ptr_value  ┃NGX CONF UNSET PTR，这样类型的成员可以使用ngx_conf_merge_ptr_value        ┃
┃                          ┃合并宏                                                                    ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并整数类型的变量，并且该变量在create loc conf等分配方法中初始化为     ┃
┃ngx_conf_merge_uint_value ┃NGX CONF UNSET UINT，这样类型的成员可以使用ngx_conf_merge_uint_           ┃
┃                          ┃ value合并宏                                                              ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并表示毫秒的ngx_msec_t类型的变量，并且该变量在create loc conf等分     ┃
┃ngx_conf_merge_msec_value ┃配方法中初始化为NGX CONF UNSET MSEC，这样类型的成员可以使用ngx_           ┃
┃                          ┃conf_merge_msec_value合并宏                                               ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  舍并表示秒的timej类型的变量，并且该变量在create loc conf等分配方法中    ┃
┃ngx_conf_merge_sec_value  ┃初始化为NGX CONF UNSET，这样类型的成员可以使用ngx_conf_merge_sec_         ┃
┃                          ┃value合并宏                                                               ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并size-t等表示空间长度的变量，并且该变量在create- loc_ conf等分配方   ┃
┃ngx_conf_merge_size_value ┃法中初始化为NGX。CONF UNSET SIZE，这样类型的成员可以使用ngx_conf_         ┃
┃                          ┃merge_size_value合并宏                                                    ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  合并off等表示偏移量的变量，并且该变最在create loc conf等分配方法中      ┃
┃ngx_conf_merge_off_value  ┃初始化为NGX CONF UNSET．这样类型的成员可以使用ngx_conf_merge_off_         ┃
┃                          ┃value合并宏                                                               ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  ngx_str_t类型的成员可以使用ngx_conf_merge_str_value合并，这时传人的     ┃
┃ngx_conf_merge_str_value  ┃                                                                          ┃
┃                          ┃default参数必须是一个char水字符串                                         ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃                          ┃  ngx_bufs t类型的成员可以使用ngx_conf merge_str_value舍并宏，这时传人的  ┃
┃ngx_conf_merge_bufs_value ┃                                                                          ┃
┃                          ┃default参数是两个，因为ngx_bufsj类型有两个成员，所以需要传人两个默认值    ┃
┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
┃ngx_conf_merge_bitmask_   ┃  以二进制位来表示标志位的整型成员，可以使用ngx_conf_merge_bitmask_       ┃
┃value                     ┃value合并宏                                                               ┃
┗━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
*/

/*
当遇到http{}配置块时，HTTP框架会调用所有HTTP模块可能实现的create main conf、create_srv_conf、
create_loc_conf方法生成存储main级别配置参数的结构体；在遇到servero{}块时会再次调用所有HTTP模
块的create_srv conf、create loc_conf回调方法生成存储srv级别配置参数的结构体；在遇到location{}时
则会再次调用create_loc_conf回调方法生成存储loc级别配置参数的结构体。
例如如下配置:
{
  http {
     server{
        location xx { //location主要用于uri请求匹配
            mytest;
        }
     }
  }
}
则在解析到http{}的时候会调用一次
location{}中，则在解析到http{}的时候调用一次create_loc_conf，解析到server{}的时候会调用一次create_loc_conf
解析到location{}的时候还会调用一次create_loc_conf

以ngx_http_mytest_config_module为例:
HTTP框架在解析nginx.conf文件时只要遇到http{}、server{}、location{}配置块就会立刻分配一个
http_mytest_conf_t结构体。
*/

//所有的核心模块NGX_CORE_MODULE对应的上下文ctx为ngx_core_module_t，子模块，例如http{} NGX_HTTP_MODULE模块对应的为上下文为ngx_http_module_t
//events{} NGX_EVENT_MODULE模块对应的为上下文为ngx_event_module_t

//ginx主配置文件分为4部分，main（全局配置）、server（主机设置）、upstream（负载均衡服务器设）和location（URL匹配特定位置的设置），这四者关系为：server继承main，location继承server，upstream既不会继承其他设置也不会被继承。
//成员中的create一般在解析前执行函数，merge在函数后执行
typedef struct { //注意和ngx_http_conf_ctx_t结构配合        初始化赋值执行，如果为"http{}"中的配置，在ngx_http_block中, ,所有的NGX_HTTP_MODULE模块都在ngx_http_block中执行
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf); //解析配置文件前调用
    //一般用来把对应的模块加入到11个阶段对应的阶段去ngx_http_phases,例如ngx_http_realip_module的ngx_http_realip_init
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf); //完成配置文件的解析后调用

/*当需要创建数据结构用于存储main级别（直属于http{...}块的配置项）的全局配置项时，可以通过create_main_conf回调方法创建存储全局配置项的结构体*/
    void       *(*create_main_conf)(ngx_conf_t *cf);
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);//常用于初始化main级别配置项

/*当需要创建数据结构用于存储srv级别（直属于虚拟主机server{...}块的配置项）的配置项时，可以通过实现create_srv_conf回调方法创建存储srv级别配置项的结构体*/
    void       *(*create_srv_conf)(ngx_conf_t *cf);
// merge_srv_conf回调方法主要用于合并main级别和srv级别下的同名配置项
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

    /*当需要创建数据结构用于存储loc级别（直属于location{...}块的配置项）的配置项时，可以实现create_loc_conf回调方法*/
    void       *(*create_loc_conf)(ngx_conf_t *cf);

    /*
    typedef struct {
           void * (*create_loc_conf) (ngx_conf_t *cf) ;
          char*(*merge_loc_conf) (ngx_conf_t *cf, void *prev,
    }ngx_http_module_t
        上面这段代码定义了create loc_conf方法，意味着HTTP框架会建立loc级别的配置。
    什么意思呢？就是说，如果没有实现merge_loc_conf方法，也就是在构造ngx_http_module_t
    时将merge_loc_conf设为NULL了，那么在4.1节的例子中server块或者http块内出现的
    配置项都不会生效。如果我们希望在server块或者http块内的配置项也生效，那么可以通过
    merge_loc_conf方法来实现。merge_loc_conf会把所属父配置块的配置项与子配置块的同名
    配置项合并，当然，如何合并取决于具体的merge_loc_conf实现。
        merge_loc_conf有3个参数，第1个参数仍然是ngx_conf_t *cf，提供一些基本的数据
    结构，如内存池、日志等。我们需要关注的是第2、第3个参数，其中第2个参数void *prev
    悬指解析父配置块时生成的结构体，而第3个参数void:-leconf则指出的是保存子配置块的结
    构体。
    */
    // merge_loc_conf回调方法主要用于合并srv级别和loc级别下的同名配置项
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf); //nginx提供了10个预设合并宏，见上面
} ngx_http_module_t; //

//所有的核心模块NGX_CORE_MODULE对应的上下文ctx为ngx_core_module_t，子模块，例如http{} NGX_HTTP_MODULE模块对应的为上下文为ngx_http_module_t
//events{} NGX_EVENT_MODULE模块对应的为上下文为ngx_event_module_t
//http{}内部的所有属性都属于NGX_HTTP_MODULE
#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

/*
NGX_MAIN_CONF：配置项可以出现在全局配置中，即不属于任何{}配置块。
NGX_EVET_CONF：配置项可以出现在events{}块内。
NGX_HTTP_MAIN_CONF： 配置项可以出现在http{}块内。
NGX_HTTP_SRV_CONF:：配置项可以出现在server{}块内，该server块必需属于http{}块。
NGX_HTTP_LOC_CONF：配置项可以出现在location{}块内，该location块必需属于server{}块。
NGX_HTTP_UPS_CONF： 配置项可以出现在upstream{}块内，该location块必需属于http{}块。
NGX_HTTP_SIF_CONF：配置项可以出现在server{}块内的if{}块中。该if块必须属于http{}块。
NGX_HTTP_LIF_CONF: 配置项可以出现在location{}块内的if{}块中。该if块必须属于http{}块。
NGX_HTTP_LMT_CONF: 配置项可以出现在limit_except{}块内,该limit_except块必须属于http{}块。
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


//注意ngx_http_get_module_main_conf ngx_http_get_module_loc_conf和ngx_http_get_module_ctx的区别

//获取该条request对应的自己所处的main,例如http{}的对应模块的配置信息
#define ngx_http_get_module_main_conf(r, module)                             \
    (r)->main_conf[module.ctx_index]
//获取该条request对应的自己所处的srv，例如server{} upstream{}的对应模块的配置信息
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]

//ngx_http_get_module_ctx存储运行过程中的各种状态(例如读取后端数据，可能需要多次读取)  ngx_http_get_module_loc_conf获取该模块在local{}中的配置信息
//获取该条request对应的自己所处的loc，例如location{} 的对应模块的配置信息
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

    if (ngx_array_init(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers,
                       cf->pool, 1, sizeof(ngx_http_handler_pt))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers,
                       cf->pool, 1, sizeof(ngx_http_handler_pt))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers,
                       cf->pool, 2, sizeof(ngx_http_handler_pt))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers,
                       cf->pool, 4, sizeof(ngx_http_handler_pt))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers,
                       cf->pool, 1, sizeof(ngx_http_handler_pt))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}

//把ngx_http_headers_in中的所有成员做hash运算，然后存放到cmcf->headers_in_hash中
static ngx_int_t
ngx_http_init_headers_in_hash(ngx_conf_t *cf, ngx_http_core_main_conf_t *cmcf)
{
    ngx_array_t         headers_in;
    ngx_hash_key_t     *hk;
    ngx_hash_init_t     hash;
    ngx_http_header_t  *header;

    if (ngx_array_init(&headers_in, cf->temp_pool, 32, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (header = ngx_http_headers_in; header->name.len; header++) {
        hk = ngx_array_push(&headers_in);
        if (hk == NULL) {
            return NGX_ERROR;
        }

        hk->key = header->name;
        hk->key_hash = ngx_hash_key_lc(header->name.data, header->name.len);
        hk->value = header;
    }

    hash.hash = &cmcf->headers_in_hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = 512;
    hash.bucket_size = ngx_align(64, ngx_cacheline_size);
    hash.name = "headers_in_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    if (ngx_hash_init(&hash, headers_in.elts, headers_in.nelts) != NGX_OK) {
        return NGX_ERROR;
    }

    return NGX_OK;
}

/*
在各个HTTP模块能够介入的7个阶段中，实际上共享了4个checker方法：ngx_http_core_generic_phase、ngx_http_core_rewrite_phase、
ngx_http_core_access_phase、ngx_http_core_content_phase。这4个checker方法的主要任务在于，根据phase_handler执行某个HTTP模块实现的
回调方法，并根据方法的返回值决定：当前阶段已经完全结束了吗？下次要执行的回调方法是哪一个？究竟是立刻执行下一个回调方法还是先把控制权交还给epoll?
*/
//cmcf->phases[]数组各个阶段的ngx_http_handler_pt节点信息全部赋值给cmcf->phase_engine.handlers数组队列
static ngx_int_t
ngx_http_init_phase_handlers(ngx_conf_t *cf, ngx_http_core_main_conf_t *cmcf)
{
    ngx_int_t                   j;
    ngx_uint_t                  i, n;
    ngx_uint_t                  find_config_index, use_rewrite, use_access;
    ngx_http_handler_pt        *h;
    ngx_http_phase_handler_t   *ph;
    ngx_http_phase_handler_pt   checker;

    cmcf->phase_engine.server_rewrite_index = (ngx_uint_t) -1;
    cmcf->phase_engine.location_rewrite_index = (ngx_uint_t) -1;
    find_config_index = 0;
    use_rewrite = cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers.nelts ? 1 : 0;
    use_access = cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers.nelts ? 1 : 0;

    n = use_rewrite + use_access + cmcf->try_files + 1 /* find config phase */;

    for (i = 0; i < NGX_HTTP_LOG_PHASE; i++) {
        n += cmcf->phases[i].handlers.nelts;
    }

    ph = ngx_pcalloc(cf->pool,
                     n * sizeof(ngx_http_phase_handler_t) + sizeof(void *));
    if (ph == NULL) {
        return NGX_ERROR;
    }

    cmcf->phase_engine.handlers = ph;
    n = 0;

    for (i = 0; i < NGX_HTTP_LOG_PHASE; i++) {
        h = cmcf->phases[i].handlers.elts;

        switch (i) { //所有阶段的checker在ngx_http_core_run_phases中调用
        /*
        NGX_HTTP_SERVERREWRITEPHASE阶段，和第3阶段NGXHTTPREWRITE_PHASE都属于地址重写，也都是针对rewrite模块而设定的阶段，前
        者用于server上下文里的地址重写，而后者用于location上下文里的地址重写。为什么要设置两个地址重写阶段，原因在于rewrite模块
        的相关指令（比如rewrite、if、set等）既可用于server上下文．又可用于location上下文。在客户端请求被Nginx接收后，首先做server
        查找与定位，在定位到server（如果没查找到就是默认server）后执行NGXHTTP_SERVER_REWRITEPHASE阶段上的回调函数，然后再进入到下
        一个阶段：NGX_HTTP_FIND_CONFIG_PHASE阶段。
          */
        case NGX_HTTP_SERVER_REWRITE_PHASE:
            if (cmcf->phase_engine.server_rewrite_index == (ngx_uint_t) -1) {
                cmcf->phase_engine.server_rewrite_index = n;
            }
            checker = ngx_http_core_rewrite_phase;

            break;

/*
NGX_HTTP_FIND_CONFIG_PHASE、NGX_HTTP_POSTREWRITE_PHASE、NGX_HTTP_POST_ACCESS_PHASE、NGX_HTTP_TRY_FILES_PHASE这4个阶段则
不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求,但是他们的会占用cmcf->phase_engine.handlers[]数组中的一个成员

 NGX_HTTP_FIND_CONFIG_PHASE阶段上不能挂载任何回调函数，因为它们永远也不 会被执行，该阶段完成的是Nginx的特定任务，即进行
 Location定位。只有把当前请求的对应location找到了，才能以该location上下文中取出更多精确的用户配置值，做后续的进一步请求处理。
 */
        case NGX_HTTP_FIND_CONFIG_PHASE: //该阶段则不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求方法，直接把http框架方法加入到cmcf->phase_engine.handlers数组中
            find_config_index = n;

            ph->checker = ngx_http_core_find_config_phase; //
            ph->phase = i;
            n++; //所有处理方法数增加一次
            ph++; //指向cmcf->phase_engine.handlers数组中下一个未用的ngx_http_phase_handler_t节点位置

            continue;

        case NGX_HTTP_REWRITE_PHASE:
            if (cmcf->phase_engine.location_rewrite_index == (ngx_uint_t) -1) {
                cmcf->phase_engine.location_rewrite_index = n;
            }
            checker = ngx_http_core_rewrite_phase;

            break;


        case NGX_HTTP_POST_REWRITE_PHASE://该阶段则不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求方法，直接把http框架方法加入到cmcf->phase_engine.handlers数组中
            if (use_rewrite) {
                ph->checker = ngx_http_core_post_rewrite_phase;
                ph->next = find_config_index;//注意:NGX_HTTP_POST_REWRITE_PHASE的下一阶段是NGX_HTTP_FIND_CONFIG_PHASE
                ph->phase = i;
                n++;
                ph++;
            }
            continue;

        case NGX_HTTP_ACCESS_PHASE:
            checker = ngx_http_core_access_phase;
            n++;
            break;

        case NGX_HTTP_POST_ACCESS_PHASE://该阶段则不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求方法，直接把http框架方法加入到cmcf->phase_engine.handlers数组中
            if (use_access) {//只有配置了try_files aaa bbb后才会在 cmcf->phase_engine.handlers添加节点pt，见ngx_http_init_phase_handlers，如果没有配置，则直接跳过try_files阶段
                ph->checker = ngx_http_core_post_access_phase;
                ph->next = n;
                ph->phase = i;
                ph++;
            }

            continue;

        case NGX_HTTP_TRY_FILES_PHASE://该阶段则不允许HTTP模块加入自己的ngx_http_handler_pt方法处理用户请求方法，直接把http框架方法加入到cmcf->phase_engine.handlers数组中
            if (cmcf->try_files) {
                ph->checker = ngx_http_core_try_files_phase;
                ph->phase = i;
                n++;
                ph++;
            }

            continue;

        case NGX_HTTP_CONTENT_PHASE:
            checker = ngx_http_core_content_phase;
            break;

        default: //NGX_HTTP_POST_READ_PHASE   NGX_HTTP_PREACCESS_PHASE  NGX_HTTP_LOG_PHASE默认都是该函数
            checker = ngx_http_core_generic_phase;
        }

//计算i阶段的ngx_http_handler_pt处理方法在所有阶段处理方法中的位置，也就是计算i阶段的处理方法应该存放在cmcf->phase_engine.handlers数组中的具体位置
        n += cmcf->phases[i].handlers.nelts;

        //注意:每一个阶段中最后加入到handlers[]中的会首先添加到cmcf->phase_engine.handlers
        for (j = cmcf->phases[i].handlers.nelts - 1; j >=0; j--) {
            ph->checker = checker; //每个阶段对应的checker，例如:NGX_HTTP_SERVER_REWRITE_PHASE阶段为ngx_http_core_rewrite_phase
            //i阶段的所有ngx_http_handler_pt处理方法通过ph连接起来，也就是全部添加到cmcf->phase_engine.handlers数组中，各个成员通过ph->next连接在一起
            ph->handler = h[j];
            ph->phase = i;
            ph->next = n;//下一阶段的第一个ngx_http_handler_pt处理方法在数组中的位置
            ph++; //指向cmcf->phase_engine.handlers数组中下一个未用的ngx_http_phase_handler_t节点位置
        }
    }

    return NGX_OK;
}

//这里面包括合并location
/*
图4-4包括4重循环，第1层（最外层）遍历所有的HTTP模块(在该函数外面)，第2层遍历所有的
server{．．．)配置块，第3层是遍历某个server{}块中嵌套的所有location{．．．）块，第4层遍历
某个location{）块中继续嵌套的所有location块（实际上，它会一直递归下去以解析可能被
层层嵌套的’location块）。读者可以对照上述4重循环来理解合并配置项的流程图。
*/ //cf->ctx为http{}的上下文ctx,cmcf为server{}中的所有上下文ctx
//结合ngx_http_core_server与ngx_http_core_location一起阅读该段代码
static char *
ngx_http_merge_servers(ngx_conf_t *cf, ngx_http_core_main_conf_t *cmcf,
    ngx_http_module_t *module, ngx_uint_t ctx_index)
{
    char                        *rv;
    ngx_uint_t                   s;
    ngx_http_conf_ctx_t         *ctx, saved;
    ngx_http_core_loc_conf_t    *clcf;
    ngx_http_core_srv_conf_t   **cscfp;

/*
    //这里的cmcf是main和srv都可以出现的配置，但是是出现在http{}内，server{}外的，所以需要与server{}内的相同配置合并
    //cmcf在外层赋值为ctx->main_conf，所以为http{}内，server{}外的server配置项， saved.srv_conf为ctx->srv_conf，所以为server{}内的server配置项
    例如
    http {
        aaa;
        server {
            aaa
        }
    }
    第一个aaa为cscfp存储的aaa,第二个aaa为saved.srv_conf存储的aaa
*/
    cscfp = cmcf->servers.elts; //server{}中的所有上下文ctx
    ctx = (ngx_http_conf_ctx_t *) cf->ctx;  //ctx为http{}的上下文ctx
    saved = *ctx; //cf->ctx为http{}的上下文ctx
    rv = NGX_CONF_OK;

    for (s = 0; s < cmcf->servers.nelts; s++) {

        /* merge the server{}s' srv_conf's */
        /*
            例如
            http {
                aaa;
                server {
                    aaa
                }
            }
            第二个aaa为cscfp存储的aaa,第一个aaa为saved.srv_conf存储的aaa
          */
        ctx->srv_conf = cscfp[s]->ctx->srv_conf;//这是server{}中的srv_conf

        if (module->merge_srv_conf) {//以server为例，saved.srv_conf[ctx_index]为http{}中的srv_conf，cscfp[s]->ctx->srv_conf[ctx_index]为server{}中的srv_conf
        //这里的saved.srv_conf是main和srv都可以出现的配置，但是是出现在http{}内，server{}外的，所以需要与server{}内的相同配置合并
            rv = module->merge_srv_conf(cf, saved.srv_conf[ctx_index],
                                        cscfp[s]->ctx->srv_conf[ctx_index]); //把http{}和server{}内的server配置项合并
            if (rv != NGX_CONF_OK) {
                goto failed;
            }
        }

        if (module->merge_loc_conf) {

            /* merge the server{}'s loc_conf */
            /*
                例如
                http {
                    bbb;
                    server {
                        bbb;
                    }

                    location {
                        bbb;
                    }
                }
                第一个bbb为cscfp[s]->ctx->loc_conf存储的bbb,第二个aaa为saved.loc_conf存储的bbb,第三个bbb为
              */

            ctx->loc_conf = cscfp[s]->ctx->loc_conf;

            rv = module->merge_loc_conf(cf, saved.loc_conf[ctx_index],
                                        cscfp[s]->ctx->loc_conf[ctx_index]); //先合并一级和二级loc,也就是http和server中的location配置
            if (rv != NGX_CONF_OK) {
                goto failed;
            }

            /* merge the locations{}' loc_conf's */

            clcf = cscfp[s]->ctx->loc_conf[ngx_http_core_module.ctx_index]; //该server{}里面的所有location配置项头部，见ngx_http_add_location

            //cscfp[s]->ctx->loc_conf存储的是server{}上下文ctx中的loc_conf, clcf->location指向的是所有location{}上下文中的loc_conf
            rv = ngx_http_merge_locations(cf, clcf->locations,
                                          cscfp[s]->ctx->loc_conf,
                                          module, ctx_index); //location递归合并
            if (rv != NGX_CONF_OK) {
                goto failed;
            }
        }
    }

failed:

    *ctx = saved;

    return rv;
}


static char *
ngx_http_merge_locations(ngx_conf_t *cf, ngx_queue_t *locations,
    void **loc_conf, ngx_http_module_t *module, ngx_uint_t ctx_index)
{
    char                       *rv;
    ngx_queue_t                *q;
    ngx_http_conf_ctx_t        *ctx, saved;
    ngx_http_core_loc_conf_t   *clcf;
    ngx_http_location_queue_t  *lq;

    if (locations == NULL) {
        return NGX_CONF_OK;
    }

    ctx = (ngx_http_conf_ctx_t *) cf->ctx;
    saved = *ctx;

    for (q = ngx_queue_head(locations);
         q != ngx_queue_sentinel(locations);
         q = ngx_queue_next(q))
    {
        lq = (ngx_http_location_queue_t *) q;

        clcf = lq->exact ? lq->exact : lq->inclusive;
        ctx->loc_conf = clcf->loc_conf;

        rv = module->merge_loc_conf(cf, loc_conf[ctx_index],
                                    clcf->loc_conf[ctx_index]);
        if (rv != NGX_CONF_OK) {
            return rv;
        }

        rv = ngx_http_merge_locations(cf, clcf->locations, clcf->loc_conf,
                                      module, ctx_index);
        if (rv != NGX_CONF_OK) {
            return rv;
        }
    }

    *ctx = saved;

    return NGX_CONF_OK;
}

/*
= 开头表示精确匹配
^~ 开头表示uri以某个常规字符串开头，理解为匹配 url路径即可。nginx不对url做编码，因此请求为/static/20%/aa，可以被规则^~ /static/ /aa匹配到（注意是空格）。
~ 开头表示区分大小写的正则匹配
~*  开头表示不区分大小写的正则匹配
!~和!~*分别为区分大小写不匹配及不区分大小写不匹配 的正则
/ 通用匹配，任何请求都会匹配到。


location匹配命令

~      #波浪线表示执行一个正则匹配，区分大小写
~*    #表示执行一个正则匹配，不区分大小写
^~    #^~表示普通字符匹配，如果该选项匹配，只匹配该选项，不匹配别的选项，一般用来匹配目录
=      #进行普通字符精确匹配
@     #"@" 定义一个命名的 location，使用在内部定向时，例如 error_page, try_files



location 匹配的优先级(与location在配置文件中的顺序无关)
= 精确匹配会第一个被处理。如果发现精确匹配，nginx停止搜索其他匹配。
普通字符匹配，正则表达式规则和长的块规则将被优先和查询匹配，也就是说如果该项匹配还需去看有没有正则表达式匹配和更长的匹配。
^~ 则只匹配该规则，nginx停止搜索其他匹配，否则nginx会继续处理其他location指令。
最后匹配理带有"~"和"~*"的指令，如果找到相应的匹配，则nginx停止搜索其他匹配；当没有正则表达式或者没有正则表达式被匹配的情况下，那么匹配程度最高的逐字匹配指令会被使用。

location 优先级官方文档

1.Directives with the = prefix that match the query exactly. If found, searching stops.
2.All remaining directives with conventional strings, longest match first. If this match used the ^~ prefix, searching stops.
3.Regular expressions, in order of definition in the configuration file.
4.If #3 yielded a match, that result is used. Else the match from #2 is used.
1.=前缀的指令严格匹配这个查询。如果找到，停止搜索。
2.所有剩下的常规字符串，最长的匹配。如果这个匹配使用^?前缀，搜索停止。
3.正则表达式，在配置文件中定义的顺序。
4.如果第3条规则产生匹配的话，结果被使用。否则，如同从第2条规则被使用。


例如

location  = / {
# 只匹配"/".
[ configuration A ]
}
location  / {
# 匹配任何请求，因为所有请求都是以"/"开始
# 但是更长字符匹配或者正则表达式匹配会优先匹配
[ configuration B ]
}
location ^~ /images/ {
# 匹配任何以 /images/ 开始的请求，并停止匹配 其它location
[ configuration C ]
}
location ~* \.(gif|jpg|jpeg)$ {
# 匹配以 gif, jpg, or jpeg结尾的请求.
# 但是所有 /images/ 目录的请求将由 [Configuration C]处理.
[ configuration D ]
}请求URI例子:

?/ -> 符合configuration A
?/documents/document.html -> 符合configuration B
?/images/1.gif -> 符合configuration C
?/documents/1.jpg ->符合 configuration D
@location 例子
error_page 404 = @fetch;

location @fetch(
proxy_pass http://fetch;
)
*/
//ngx_http_add_location
//cscf为server{}配置里面的srv_conf,pclcf->locations为该server{}类里面的所有(包括解析server行的时候开辟的local_crate和解析location行的时候开辟的loc_creat空间)loc_conf头部
//参考:http://blog.csdn.net/fengmo_q/article/details/6683377和http://tech.uc.cn/?p=300
static ngx_int_t
ngx_http_init_locations(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
    ngx_http_core_loc_conf_t *pclcf) //形成3叉树见ngx_http_init_static_location_trees
{
    ngx_uint_t                   n;
    ngx_queue_t                 *q, *locations,
    *named,
    tail;//最后name noname regex都会连接到tail链表中
    ngx_http_core_loc_conf_t    *clcf;
    ngx_http_location_queue_t   *lq;
    ngx_http_core_loc_conf_t   **clcfp;
#if (NGX_PCRE)
    ngx_uint_t                   r;
    ngx_queue_t                 *regex;
#endif

    //这是合并过的loc配置项
    locations = pclcf->locations;//存储在server{}中创建的ctx->loc_conf中的是location{}创建的ctx->loc_conf,所以pclcf为server{}中的loc_conf
    //locations是所有的location{}项中创建的loc_conf,所以这两个之和就是server{}下所有的location配置项

    if (locations == NULL) {
        return NGX_OK;
    }
    //如果location = /aaa {}  location ^~ /aaa {} 这两个的uri那么是一样的，他们分别存储在一个ngx_http_core_loc_conf_t中，怎么区分呢? 就通过exact_match noregex来区分，并且在队列中完全匹配在前面
    /*
      按照类型排序location，排序完后的队列：  (exact_match 或 inclusive(前缀匹配)) (排序好的，如果某个exact_match名字和inclusive location相同，exact_match排在前面)
      |  regex（未排序）| named(排序好的)  |  noname（未排序）
      */
    /*
    例如:
        http {
            server {
                location { #1
                    location {#2

                    }

                    location {#3

                    }
                }

                location {#4
                    location {#5

                    }

                    location {#6

                    }
                }
            }

        }
     */ //locations是直属于server{}的location{}块(上面的#1 #2)， 或者直属于#1的#2 #3,或者直属于#4中的#5 #6。所以需要三次调用ngx_queue_sort
    //排序完后进行location切分。location排序完，整个list的结构是：前缀匹配|绝对匹配--->正则匹配--->命名--> 未命名
    ngx_queue_sort(locations, ngx_http_cmp_locations);

    named = NULL;
    n = 0;
#if (NGX_PCRE)
    regex = NULL;
    r = 0;
#endif


    for (q = ngx_queue_head(locations);
         q != ngx_queue_sentinel(locations);
         q = ngx_queue_next(q)) //找到queue中正则表达式和命名location中的位置
    {
        lq = (ngx_http_location_queue_t *) q;

        clcf = lq->exact ? lq->exact : lq->inclusive;

        /* 由于可能存在nested location，也就是location里面嵌套的location，这里需要递归的处理一下当前location下面的nested location */
        if (ngx_http_init_locations(cf, NULL, clcf) != NGX_OK) { //这里是一个递归，如果存在location下面还有locations的话还会进行递归调用
            return NGX_ERROR;
        }

#if (NGX_PCRE)

        if (clcf->regex) {
            r++; //location 后为正则表达式的个数

            if (regex == NULL) {
                regex = q;//记录链表中的regex正则表达式头
            }

            continue;
        }

#endif
     /*
        location / {
          try_files index.html index.htm @fallback;
        }

        location @fallback {
          root /var/www/error;
          index index.html;
        }
    */
        if (clcf->named) {
            n++; //location后为@name的location个数

            if (named == NULL) {
                named = q;//记录链表中的命名location头named
            }

            continue;
        }

        if (clcf->noname) { //只要遇到name，则直接返回，在后面拆分。注意未命名的在队列尾部，所以不会影响到前面的named和regx
            break;
        }
    }

    /*
    先分离noname(没有像后面两个那样集中管理)，在分离named(保存到cscf->named_locations)，最后分离regex(保存到pclcf->regex_locations= clcfp)，
    分离这些另类之后，我们处理那些普通的location，普通的location大家应该知道是指的哪些，nginx称为static location。
    */
    if (q != ngx_queue_sentinel(locations)) { //如果队列里面有noname，则以该noname头拆分为locations和tail两个队列链表
        ngx_queue_split(locations, q, &tail);//未命名的拆分出来  noname named regex都连接到tail链表中
    }
    //实际上这里拆分出的noname放入tail后，如果locations中包含named location,则noname和name放一起(cscf->named_locations)，如果没有named但是有regex，
    //则和regex放一起(pclcf->regex_locations)，如果name和regex和都没有，则和普通locations放一起(pclcf->locations)
    if (named) {
        clcfp = ngx_palloc(cf->pool,
                           (n + 1) * sizeof(ngx_http_core_loc_conf_t *));
        if (clcfp == NULL) {
            return NGX_ERROR;
        }

        /* 如果有named location，将它们保存在所属server的named_locations数组中 */
        //named_locations的每个成员指向named，，要注意最后一个named的成员的next链表上有连接noname location项目
        cscf->named_locations = clcfp; /* 所有的location @name {}这种ngx_http_core_loc_conf_t全部指向named_locations */

        for (q = named;
             q != ngx_queue_sentinel(locations);
             q = ngx_queue_next(q))
        {
            lq = (ngx_http_location_queue_t *) q;

            *(clcfp++) = lq->exact;
        }

        *clcfp = NULL;

        //拆分到tail中的节点pclcf->regex_locations 数组会指向他们，从而保证了pclcf->regex_locations和 cscf->named_locations中的所有location通过ngx_http_location_queue_t是连接在一起的
        //拆分到tail中的节点cscf->named_locations数组会指向他们
        ngx_queue_split(locations, named, &tail); //以name类型的头部位置拆分locations，并且把拆分的后半部分放入tail头部中
    }

#if (NGX_PCRE)

    if (regex) {

        clcfp = ngx_palloc(cf->pool,
                           (r + 1) * sizeof(ngx_http_core_loc_conf_t *));
        if (clcfp == NULL) {
            return NGX_ERROR;
        }

        /* 如果有正则匹配location，将它们保存在所属server的http core模块的loc配置的regex_locations 数组中，
             这里和named location保存位置不同的原因是由于named location只能存在server里面，而regex location可以作为nested location */
        pclcf->regex_locations = clcfp; /* 所有的location 正则表达式 {}这种ngx_http_core_loc_conf_t全部指向regex_locations */

        for (q = regex;
             q != ngx_queue_sentinel(locations);
             q = ngx_queue_next(q))
        {
            lq = (ngx_http_location_queue_t *) q;

            *(clcfp++) = lq->exact;
        }

        *clcfp = NULL;
//拆分到tail中的节点pclcf->regex_locations 数组会指向他们，从而保证了pclcf->regex_locations和 cscf->named_locations中的所有location通过ngx_http_location_queue_t是连接在一起的
        ngx_queue_split(locations, regex, &tail); //按照regex拆分
    }

#endif

    return NGX_OK;
}

/*
下面的ngx_http_init_static_location_trees函数就会将那些普通的location(就是ngx_http_init_locations中name noname regex以外的location)，即staticlocation，进行树化(一种三叉树)处理，
之所以要做这样的处理，是为了在处理http请求时能高效的搜索的匹配的location配置。
*/
/*
注意，这里的二叉查找树并不是红黑树，不过，为什么不使用红黑树呢？因为location是由nginx.conf中读取到的，它是静态不变的，
不存在运行过程中在树中添加或者删除location的场景，而且红黑树的查询效率也没有重新构造的静态的完全平衡二叉树高。
*/
//location tree的建立在ngx_http_init_static_location_trees中进行：
//location树产生排序见ngx_http_init_locations
static ngx_int_t
ngx_http_init_static_location_trees(ngx_conf_t *cf,
    ngx_http_core_loc_conf_t *pclcf) //很好的图解，参考http://blog.csdn.net/fengmo_q/article/details/6683377和http://tech.uc.cn/?p=300
{
    ngx_queue_t                *q, *locations;
    ngx_http_core_loc_conf_t   *clcf;
    ngx_http_location_queue_t  *lq;

    locations = pclcf->locations;

    if (locations == NULL) {
        return NGX_OK;
    }

    if (ngx_queue_empty(locations)) {
        return NGX_OK;
    }

    for (q = ngx_queue_head(locations);
         q != ngx_queue_sentinel(locations);
         q = ngx_queue_next(q))
    {
        lq = (ngx_http_location_queue_t *) q;

        clcf = lq->exact ? lq->exact : lq->inclusive;

        /* 这里也是由于nested location，需要递归一下 */
        if (ngx_http_init_static_location_trees(cf, clcf) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    /*
    join队列中名字相同的inclusive和exact类型location，也就是如果某个exact_match的location名字和普通字符串匹配的location名字相同的话，
    就将它们合到一个节点中，分别保存在节点的exact和inclusive下，这一步的目的实际是去重，为后面的建立排序树做准备
    */
    if (ngx_http_join_exact_locations(cf, locations) != NGX_OK) {
        return NGX_ERROR;
    }

    /* 递归每个location节点，得到当前节点的名字为其前缀的location的列表，保存在当前节点的list字段下 */
    ngx_http_create_locations_list(locations, ngx_queue_head(locations));


    /* 递归建立location三叉排序树 */
    pclcf->static_locations = ngx_http_create_locations_tree(cf, locations, 0);
    if (pclcf->static_locations == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}
/*
= 开头表示精确匹配
^~ 开头表示uri以某个常规字符串开头，理解为匹配 url路径即可。nginx不对url做编码，因此请求为/static/20%/aa，可以被规则^~ /static/ /aa匹配到（注意是空格）。
~ 开头表示区分大小写的正则匹配
~*  开头表示不区分大小写的正则匹配
!~和!~*分别为区分大小写不匹配及不区分大小写不匹配 的正则
/ 通用匹配，任何请求都会匹配到。


location匹配命令

~      #波浪线表示执行一个正则匹配，区分大小写
~*    #表示执行一个正则匹配，不区分大小写
^~    #^~表示普通字符匹配，如果该选项匹配，只匹配该选项，不匹配别的选项，一般用来匹配目录
=      #进行普通字符精确匹配
@     #"@" 定义一个命名的 location，使用在内部定向时，例如 error_page, try_files



location 匹配的优先级(与location在配置文件中的顺序无关)
= 精确匹配会第一个被处理。如果发现精确匹配，nginx停止搜索其他匹配。
普通字符匹配，正则表达式规则和长的块规则将被优先和查询匹配，也就是说如果该项匹配还需去看有没有正则表达式匹配和更长的匹配。
^~ 则只匹配该规则，nginx停止搜索其他匹配，否则nginx会继续处理其他location指令。
最后匹配理带有"~"和"~*"的指令，如果找到相应的匹配，则nginx停止搜索其他匹配；当没有正则表达式或者没有正则表达式被匹配的情况下，那么匹配程度最高的逐字匹配指令会被使用。

location 优先级官方文档

1.Directives with the = prefix that match the query exactly. If found, searching stops.
2.All remaining directives with conventional strings, longest match first. If this match used the ^~ prefix, searching stops.
3.Regular expressions, in order of definition in the configuration file.
4.If #3 yielded a match, that result is used. Else the match from #2 is used.
1.=前缀的指令严格匹配这个查询。如果找到，停止搜索。
2.所有剩下的常规字符串，最长的匹配。如果这个匹配使用^?前缀，搜索停止。
3.正则表达式，在配置文件中定义的顺序。
4.如果第3条规则产生匹配的话，结果被使用。否则，如同从第2条规则被使用。


例如

location  = / {
# 只匹配"/".
[ configuration A ]
}
location  / {
# 匹配任何请求，因为所有请求都是以"/"开始
# 但是更长字符匹配或者正则表达式匹配会优先匹配
[ configuration B ]
}
location ^~ /images/ {
# 匹配任何以 /images/ 开始的请求，并停止匹配 其它location
[ configuration C ]
}
location ~* \.(gif|jpg|jpeg)$ {
# 匹配以 gif, jpg, or jpeg结尾的请求.
# 但是所有 /images/ 目录的请求将由 [Configuration C]处理.
[ configuration D ]
}请求URI例子:

?/ -> 符合configuration A
?/documents/document.html -> 符合configuration B
?/images/1.gif -> 符合configuration C
?/documents/1.jpg ->符合 configuration D
@location 例子
error_page 404 = @fetch;

location @fetch(
proxy_pass http://fetch;
)
*/

//locations为父级对应的loc_conf[]的location  clcf为本级的loc_conf[]
//把所有的location{}中的配置通过lq->queue连接在一起，最终头部为父级server{}中的loc_conf配置里
ngx_int_t
ngx_http_add_location(ngx_conf_t *cf, ngx_queue_t **locations,
    ngx_http_core_loc_conf_t *clcf) //和ngx_http_init_locations配合使用
{
    ngx_http_location_queue_t  *lq;

    if (*locations == NULL) {
        *locations = ngx_palloc(cf->temp_pool,
                                sizeof(ngx_http_location_queue_t));
        if (*locations == NULL) {
            return NGX_ERROR;
        }

        ngx_queue_init(*locations);//父级就是该父级下所有loc的头部，头部是ngx_queue_t，next开始的成员为ngx_http_location_queue_t
    }

    lq = ngx_palloc(cf->temp_pool, sizeof(ngx_http_location_queue_t));
    if (lq == NULL) {
        return NGX_ERROR;
    }

    if (clcf->exact_match
#if (NGX_PCRE)
        || clcf->regex
#endif
        || clcf->named || clcf->noname)
    {
        lq->exact = clcf;
        lq->inclusive = NULL;

    } else { //前缀匹配
        lq->exact = NULL;
        lq->inclusive = clcf; //location ^~
    }

    lq->name = &clcf->name;
    lq->file_name = cf->conf_file->file.name.data;
    lq->line = cf->conf_file->line;

    ngx_queue_init(&lq->list);

    ngx_queue_insert_tail(*locations, &lq->queue); //添加到父级的loc_conf[]指针中

    return NGX_OK;
}


/*
= 开头表示精确匹配
^~ 开头表示uri以某个常规字符串开头，理解为匹配 url路径即可。nginx不对url做编码，因此请求为/static/20%/aa，可以被规则^~ /static/ /aa匹配到（注意是空格）。
~ 开头表示区分大小写的正则匹配
~*  开头表示不区分大小写的正则匹配
!~和!~*分别为区分大小写不匹配及不区分大小写不匹配 的正则
/ 通用匹配，任何请求都会匹配到。


location匹配命令

~      #波浪线表示执行一个正则匹配，区分大小写
~*    #表示执行一个正则匹配，不区分大小写
^~    #^~表示普通字符匹配，如果该选项匹配，只匹配该选项，不匹配别的选项，一般用来匹配目录
=      #进行普通字符精确匹配
@     #"@" 定义一个命名的 location，使用在内部定向时，例如 error_page, try_files



location 匹配的优先级(与location在配置文件中的顺序无关)
= 精确匹配会第一个被处理。如果发现精确匹配，nginx停止搜索其他匹配。
普通字符匹配，正则表达式规则和长的块规则将被优先和查询匹配，也就是说如果该项匹配还需去看有没有正则表达式匹配和更长的匹配。
^~ 则只匹配该规则，nginx停止搜索其他匹配，否则nginx会继续处理其他location指令。
最后匹配理带有"~"和"~*"的指令，如果找到相应的匹配，则nginx停止搜索其他匹配；当没有正则表达式或者没有正则表达式被匹配的情况下，那么匹配程度最高的逐字匹配指令会被使用。

location 优先级官方文档

1.Directives with the = prefix that match the query exactly. If found, searching stops.
2.All remaining directives with conventional strings, longest match first. If this match used the ^~ prefix, searching stops.
3.Regular expressions, in order of definition in the configuration file.
4.If #3 yielded a match, that result is used. Else the match from #2 is used.
1.=前缀的指令严格匹配这个查询。如果找到，停止搜索。
2.所有剩下的常规字符串，最长的匹配。如果这个匹配使用^?前缀，搜索停止。
3.正则表达式，在配置文件中定义的顺序。
4.如果第3条规则产生匹配的话，结果被使用。否则，如同从第2条规则被使用。


例如

location  = / {
# 只匹配"/".
[ configuration A ]
}
location  / {
# 匹配任何请求，因为所有请求都是以"/"开始
# 但是更长字符匹配或者正则表达式匹配会优先匹配
[ configuration B ]
}
location ^~ /images/ {
# 匹配任何以 /images/ 开始的请求，并停止匹配 其它location
[ configuration C ]
}
location ~* \.(gif|jpg|jpeg)$ {
# 匹配以 gif, jpg, or jpeg结尾的请求.
# 但是所有 /images/ 目录的请求将由 [Configuration C]处理.
[ configuration D ]
}请求URI例子:

?/ -> 符合configuration A
?/documents/document.html -> 符合configuration B
?/images/1.gif -> 符合configuration C
?/documents/1.jpg ->符合 configuration D
@location 例子
error_page 404 = @fetch;

location @fetch(
proxy_pass http://fetch;
)
*/
/*
比较函数ngx_http_cmp_locations的算法原则是：
1 首先是如果比较的额两个节点中插入的是未命名的，那么把该节点加入到后面，如果比较的两个节点都是未命名的，那么保持原定次序。
2 如果插入的两个节点中，插入的是命名的location，那么把该节点加入到后面，如果比较的两个节点都是命名的，那么比较location名称，按照字母序进行排序。
3 如果两个比较节点中，插入的是正则location，那么就把插入即诶的那加入到后面，如果比较的两个节点都是正则，那么就按照原定次序，即保持用户在配置文件里书序的先后顺序。
//所以插入的降序是未命名、命名、正则、前缀匹配|绝对匹配。
*/
static ngx_int_t
ngx_http_cmp_locations(const ngx_queue_t *one, const ngx_queue_t *two)
{
    ngx_int_t                   rc;
    ngx_http_core_loc_conf_t   *first, *second;
    ngx_http_location_queue_t  *lq1, *lq2;

    lq1 = (ngx_http_location_queue_t *) one;
    lq2 = (ngx_http_location_queue_t *) two;

    first = lq1->exact ? lq1->exact : lq1->inclusive;
    second = lq2->exact ? lq2->exact : lq2->inclusive;

    if (first->noname && !second->noname) {
        /* shift no named locations to the end */
        return 1;
    }

    if (!first->noname && second->noname) {
        /* shift no named locations to the end */
        return -1;
    }

    if (first->noname || second->noname) {
        /* do not sort no named locations */
        return 0;
    }

    if (first->named && !second->named) {
        /* shift named locations to the end */
        return 1;
    }

    if (!first->named && second->named) {
        /* shift named locations to the end */
        return -1;
    }

    if (first->named && second->named) {
        return ngx_strcmp(first->name.data, second->name.data);
    }

#if (NGX_PCRE)

    if (first->regex && !second->regex) {
        /* shift the regex matches to the end */
        return 1;
    }

    if (!first->regex && second->regex) {
        /* shift the regex matches to the end */
        return -1;
    }

    if (first->regex || second->regex) {
        /* do not sort the regex matches */
        return 0;
    }

#endif

    rc = ngx_filename_cmp(first->name.data, second->name.data,
                          ngx_min(first->name.len, second->name.len) + 1); //前缀匹配

    if (rc == 0 && !first->exact_match && second->exact_match) { //完全匹配
        /* an exact match must be before the same inclusive one */
        return 1;
    }

    return rc;
}

//很好的图解，参考http://blog.csdn.net/fengmo_q/article/details/6683377
static ngx_int_t
ngx_http_join_exact_locations(ngx_conf_t *cf, ngx_queue_t *locations)
{
    ngx_queue_t                *q, *x;
    ngx_http_location_queue_t  *lq, *lx;

    q = ngx_queue_head(locations);

    while (q != ngx_queue_last(locations)) {

        x = ngx_queue_next(q);

        lq = (ngx_http_location_queue_t *) q;
        lx = (ngx_http_location_queue_t *) x;

        if (lq->name->len == lx->name->len
            && ngx_filename_cmp(lq->name->data, lx->name->data, lx->name->len)
               == 0)
        {
            if ((lq->exact && lx->exact) || (lq->inclusive && lx->inclusive)) {
                ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                              "duplicate location \"%V\" in %s:%ui",
                              lx->name, lx->file_name, lx->line);

                return NGX_ERROR;
            }

            lq->inclusive = lx->inclusive;

            ngx_queue_remove(x);

            continue;
        }

        q = ngx_queue_next(q);
    }

    return NGX_OK;
}

/*
接下来调用ngx_http_create_locations_list，这个函数的作用是这样的：

在开始的这个locationqueue中，有一些是有相同路径前缀的，自然他们在排序的时候也是会挨在一起的，ngx_http_create_locations_list会将跟某个location有相同前缀的location，从原来的queue上取下，挂在该location的list成员下，这样在主queue上的location都是具有相互不同name的location了。举个例子：

主链：/a  -> /ab  ->  /abc ->  /b  -> /bc  ->  /bcd ->  /c ->  /cd ->  /cde

  //横向通过ngx_http_location_queue_t->queue连接  纵向通过list连接
  处理之后：/a  -> /b  ->  /c

            |       |        |

          /ab       /bc      /cd

            |       |        |

         /abc      /bcd       /cde
*/ //递归locations队列中的每个节点，得到以当前节点的名字为前缀的location，并保存在当前节点的list字段 下
static void //图形化参考http://blog.chinaunix.net/uid-27767798-id-3759557.html，这个博客很好理解
ngx_http_create_locations_list(ngx_queue_t *locations, ngx_queue_t *q) //图形化理解参考http://blog.csdn.net/fengmo_q/article/details/6683377
{
    u_char                     *name;
    size_t                      len;
    ngx_queue_t                *x, tail;
    ngx_http_location_queue_t  *lq, *lx;

    //如果location为空就没有必要继续走下面的流程了，尤其是递归到嵌套location
    if (q == ngx_queue_last(locations)) {
        return;
    }

    lq = (ngx_http_location_queue_t *) q;

    if (lq->inclusive == NULL) {
        //如果这个节点是精准匹配那么这个节点，就不会作为某些节点的前缀，不用拥有tree节点
        ngx_http_create_locations_list(locations, ngx_queue_next(q));
        return;
    }

    len = lq->name->len;
    name = lq->name->data;

    for (x = ngx_queue_next(q);
         x != ngx_queue_sentinel(locations);
         x = ngx_queue_next(x))
    {
        /*
            由于所有location已经按照顺序排列好，递归q节点的后继节点，如果后继节点的长度小于后缀节点的长度，那么可以断定，这个后
            继节点肯定和后缀节点不一样，并且不可能有共同的后缀；如果后继节点和q节点的交集做比较，如果不同，就表示不是同一个前缀，所以
            可以看出，从q节点的location list应该是从q.next到x.prev节点
          */
        lx = (ngx_http_location_queue_t *) x;

        if (len > lx->name->len
            || ngx_filename_cmp(name, lx->name->data, len) != 0)
        {
            break;
        }
    }

    q = ngx_queue_next(q);

    if (q == x) {//如果q和x节点直接没有节点，那么就没有必要递归后面了产生q节点的location list，直接递归q的后继节点x，产生x节点location list
        ngx_http_create_locations_list(locations, x);
        return;
    }

    ngx_queue_split(locations, q, &tail);//location从q节点开始分割，那么现在location就是q节点之前的一段list
    ngx_queue_add(&lq->list, &tail);//q节点的list初始为从q节点开始到最后的一段list


    /*
        原则上因为需要递归两段list，一个为p的location list（从p.next到x.prev），另一段为x.next到location的最后一个元素，这里如果x
        已经是location的最后一个了，那么就没有必要递归x.next到location的这一段了，因为这一段都是空的。
    */
    if (x == ngx_queue_sentinel(locations)) {
        ngx_http_create_locations_list(&lq->list, ngx_queue_head(&lq->list));
        return;
    }

    //到了这里可以知道需要递归两段location list了
    ngx_queue_split(&lq->list, x, &tail);//再次分割，lq->list剩下p.next到x.prev的一段了
    ngx_queue_add(locations, &tail);// 放到location 中去
    ngx_http_create_locations_list(&lq->list, ngx_queue_head(&lq->list));//递归p.next到x.prev

    ngx_http_create_locations_list(locations, x);//递归x.next到location 最后了
}


/*
 * to keep cache locality for left leaf nodes, allocate nodes in following
 * order: node, left subtree, right subtree, inclusive subtree
 */
/*    主要说下大概的过程吧，像这种算法的分析，讲起来会很啰嗦，这里只是提个纲，大家还是得自己看。

    1.      在一个queue中取中间位置(算法大家可以借鉴)，分成两部分
    2.      左边的部分，递归调用ngx_http_create_locations_tree，生成left树
    3.      右边的部分，递归调用ngx_http_create_locations_tree，生成right树
    4.      中间位置的location，通过它的list，通过递归调用ngx_http_create_locations_tree，生成tree树(可以认为是中树)。
*/
//很好的图解，参考http://blog.csdn.net/fengmo_q/article/details/6683377
static ngx_http_location_tree_node_t *
ngx_http_create_locations_tree(ngx_conf_t *cf, ngx_queue_t *locations,
    size_t prefix) //图形化理解参考http://blog.csdn.net/fengmo_q/article/details/6683377
{//图形化理解location三叉树形成过程，参考http://blog.chinaunix.net/uid-27767798-id-3759557.html
    size_t                          len;
    ngx_queue_t                    *q, tail;
    ngx_http_location_queue_t      *lq;
    ngx_http_location_tree_node_t  *node;

    q = ngx_queue_middle(locations);

    lq = (ngx_http_location_queue_t *) q;
    len = lq->name->len - prefix;

    node = ngx_palloc(cf->pool,
                      offsetof(ngx_http_location_tree_node_t, name) + len);
    if (node == NULL) {
        return NULL;
    }

    node->left = NULL;
    node->right = NULL;
    node->tree = NULL;
    node->exact = lq->exact;
    node->inclusive = lq->inclusive;

    node->auto_redirect = (u_char) ((lq->exact && lq->exact->auto_redirect)
                           || (lq->inclusive && lq->inclusive->auto_redirect));

    node->len = (u_char) len;
    ngx_memcpy(node->name, &lq->name->data[prefix], len);//可以看到实际node的name是父节点的增量（不存储公共前缀，也许这是为了节省空间）
    ngx_queue_split(locations, q, &tail);//location队列是从头节点开始到q节点之前的节点，tail是q节点到location左右节点的队列

    if (ngx_queue_empty(locations)) {
        /*
         * ngx_queue_split() insures that if left part is empty,
         * then right one is empty too
         */
        goto inclusive;
    }

    node->left = ngx_http_create_locations_tree(cf, locations, prefix); //递归构建node的左节点

    if (node->left == NULL) {
        return NULL;
    }

    ngx_queue_remove(q);

    if (ngx_queue_empty(&tail)) {
        goto inclusive;
    }

    node->right = ngx_http_create_locations_tree(cf, &tail, prefix);//递归构建node的右节点
    if (node->right == NULL) {
        return NULL;
    }

inclusive:

    if (ngx_queue_empty(&lq->list)) {
        return node;
    }

    node->tree = ngx_http_create_locations_tree(cf, &lq->list, prefix + len); //根据list指针构造node的tree指针
    if (node->tree == NULL) {
        return NULL;
    }

    return node;
}

//解析listen命令配置项的各种信息，
ngx_int_t
ngx_http_add_listen(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
    ngx_http_listen_opt_t *lsopt)
{
    in_port_t                   p;
    ngx_uint_t                  i;
    struct sockaddr            *sa;
    struct sockaddr_in         *sin;
    ngx_http_conf_port_t       *port;
    ngx_http_core_main_conf_t  *cmcf;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6        *sin6;
#endif

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    if (cmcf->ports == NULL) {
        cmcf->ports = ngx_array_create(cf->temp_pool, 2,
                                       sizeof(ngx_http_conf_port_t));
        if (cmcf->ports == NULL) {
            return NGX_ERROR;
        }
    }

    sa = &lsopt->u.sockaddr;

    switch (sa->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = &lsopt->u.sockaddr_in6;
        p = sin6->sin6_port;
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        p = 0;
        break;
#endif

    default: /* AF_INET */
        sin = &lsopt->u.sockaddr_in; //获取listen 命令中配置的ip和端口
        p = sin->sin_port;
        break;
    }

    port = cmcf->ports->elts;
    /*
     这段代码是遍历ports数组，查看新添加的端口信息是否已经存在，如果该端口信息存在则调用ngx_http_add_addresses函数在对应的端口结构
     上添加地址信息。否则，在prots数组中添加一个元素，并初始化，然后调用ngx_http_add_address函数添加地址信息。
     */
    //下面的功能就是把相同端口不通IP地址的listen信息存储到相同的cmcf->ports[i]中，不通端口的在不同的[]中
    //如果配置中有listen 1.1.1.1:50  2.2.2.2:50，则解析到第一个listen的时候，走for外面的函数，如果第二个会满足下面的if条件，走if后面的流程
    for (i = 0; i < cmcf->ports->nelts; i++) {

        if (p != port[i].port || sa->sa_family != port[i].family) {
            continue;
        }

        /* a port is already in the port list */
        //不通IP，相同端口
        //相同端口的存储在同一个ngx_http_conf_port_t结构中，不同的IP地址存到ngx_http_conf_port_t->addrs成员数组中
        return ngx_http_add_addresses(cf, cscf, &port[i], lsopt); //通过for循环把listen配置信息一项一项的添加
    }

    /* add a port to the port list */

    port = ngx_array_push(cmcf->ports);
    if (port == NULL) {
        return NGX_ERROR;
    }

    port->family = sa->sa_family;
    port->port = p;
    port->addrs.elts = NULL;

    return ngx_http_add_address(cf, cscf, port, lsopt); //把解析到的所有listen地址端口信息添加到
}


static ngx_int_t
ngx_http_add_addresses(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
    ngx_http_conf_port_t *port, ngx_http_listen_opt_t *lsopt) //cscf为该LISTEN IP:PORT所处server{}上下文
{
    u_char                *p;
    size_t                 len, off;
    ngx_uint_t             i, default_server, proxy_protocol;
    struct sockaddr       *sa;
    ngx_http_conf_addr_t  *addr;
#if (NGX_HAVE_UNIX_DOMAIN)
    struct sockaddr_un    *saun;
#endif
#if (NGX_HTTP_SSL)
    ngx_uint_t             ssl;
#endif
#if (NGX_HTTP_V2)
    ngx_uint_t             http2;
#endif

    /*
     * we cannot compare whole sockaddr struct's as kernel
     * may fill some fields in inherited sockaddr struct's
     */

    sa = &lsopt->u.sockaddr;

    switch (sa->sa_family) {

#if (NGX_HAVE_INET6)
    case AF_INET6:
        off = offsetof(struct sockaddr_in6, sin6_addr);
        len = 16;
        break;
#endif

#if (NGX_HAVE_UNIX_DOMAIN)
    case AF_UNIX:
        off = offsetof(struct sockaddr_un, sun_path);
        len = sizeof(saun->sun_path);
        break;
#endif

    default: /* AF_INET */
        off = offsetof(struct sockaddr_in, sin_addr);
        len = 4;
        break;
    }

    p = lsopt->u.sockaddr_data + off;

    addr = port->addrs.elts;

    /* 注意，如果是有多个ip:port相同，但是在不同的server{}中，则解析到这个ip:port中的第一个的时候，不会走到该循环中，而是执行循环外的
                ngx_http_add_address，这里面会把addr[i].default_server赋值为该ip:port所在server{} */
    for (i = 0; i < port->addrs.nelts; i++) {
        //比较新来的lsopt中的地址和ports->addrs地址池中是否有重复，如果addr[]中没有该ip:port存在，则直接指向for外面的ngx_http_add_address
        if (ngx_memcmp(p, addr[i].opt.u.sockaddr_data + off, len) != 0) {
            continue;
        }

        /* the address is already in the address list */
        //可能相同的IP:port配置在不同的server{}中，那么他们处于同一个ngx_http_conf_addr_t，但所属ngx_http_core_srv_conf_t不同
        /*向ngx_http_conf_addr_t的servers数组添加ngx_http_core_srv_conf_t结构  */
        if (ngx_http_add_server(cf, cscf, &addr[i]) != NGX_OK) {
            return NGX_ERROR;
        }

        //listen ip:port出现在不同的server中
        /* */
        /* preserve default_server bit during listen options overwriting */
        default_server = addr[i].opt.default_server; //这条新的listen是否有配置default_server选项

        proxy_protocol = lsopt->proxy_protocol || addr[i].opt.proxy_protocol;

#if (NGX_HTTP_SSL)
        ssl = lsopt->ssl || addr[i].opt.ssl;
#endif
#if (NGX_HTTP_V2)
        http2 = lsopt->http2 || addr[i].opt.http2;
#endif

        if (lsopt->set) {

            if (addr[i].opt.set) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                        "duplicate listen options for %s", addr[i].opt.addr);
                return NGX_ERROR;
            }
            //相同listen ip:port出现在不同的server中，那么opt指向最后解析的listen结构ngx_http_listen_opt_t，见ngx_http_add_addresses
            addr[i].opt = *lsopt;
        }

        /* check the duplicate "default" server for this address:port */

        if (lsopt->default_server) {

            if (default_server) { //如果前面相同的ip:port所处server{}中已经配置过default_server，则报错
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                        "a duplicate default server for %s", addr[i].opt.addr);
                return NGX_ERROR;
            }
            //相同listen ip:port出现在不同的server中，那么opt指向最后解析的listen配置中带有default_server选项所对应的server{}上下文ctx，见ngx_http_add_addresses
            default_server = 1;

            /* 注意，如果是有多个ip:port相同，但是在不同的server{}中，则解析到这个ip:port中的第一个的时候，不会走到该循环中，而是执行循环外的
                ngx_http_add_address，这里面会把addr[i].default_server赋值为该ip:port所在server{} */
            addr[i].default_server = cscf; //后面解析
        }

        addr[i].opt.default_server = default_server; //只要相同ip:port所处的多个server{]有一个有设置default，则认为有默认server
        addr[i].opt.proxy_protocol = proxy_protocol;
#if (NGX_HTTP_SSL)
        addr[i].opt.ssl = ssl;
#endif
#if (NGX_HTTP_V2)
        addr[i].opt.http2 = http2;
#endif

        return NGX_OK;
    }

    /* add the address to the addresses list that bound to this port */
    //IP和端口都相同并且在不同的server{}中,则他们在不同的ngx_http_conf_port_t->addrs中
    return ngx_http_add_address(cf, cscf, port, lsopt);
}


/*
 * add the server address, the server names and the server core module
 * configurations to the port list
 */
//port为ngx_http_core_main_conf_t中的ports，它用来存储listen配置项的相关信息
//如果listen的是相同port但ip不同，或者是IP和端口都相同并且在不同的server{}中，则获取一个新的ngx_http_conf_addr_t来存储
static ngx_int_t
ngx_http_add_address(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
    ngx_http_conf_port_t *port, ngx_http_listen_opt_t *lsopt)
{
    ngx_http_conf_addr_t  *addr;

    if (port->addrs.elts == NULL) {
        if (ngx_array_init(&port->addrs, cf->temp_pool, 4,
                           sizeof(ngx_http_conf_addr_t)) //开辟ngx_http_conf_addr_t存储
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

#if (NGX_HTTP_V2 && NGX_HTTP_SSL                                              \
     && !defined TLSEXT_TYPE_application_layer_protocol_negotiation           \
     && !defined TLSEXT_TYPE_next_proto_neg)

    if (lsopt->http2 && lsopt->ssl) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "nginx was built with OpenSSL that lacks ALPN "
                           "and NPN support, HTTP/2 is not enabled for %s",
                           lsopt->addr);
    }

#endif

    addr = ngx_array_push(&port->addrs);
    if (addr == NULL) {
        return NGX_ERROR;
    }

    addr->opt = *lsopt;
    addr->hash.buckets = NULL;
    addr->hash.size = 0;
    addr->wc_head = NULL;
    addr->wc_tail = NULL;
#if (NGX_PCRE)
    addr->nregex = 0;
    addr->regex = NULL;
#endif
    addr->default_server = cscf;
    addr->servers.elts = NULL;

    return ngx_http_add_server(cf, cscf, addr);
}


/* add the server core module configuration to the address:port */
/*向ngx_http_conf_addr_t的servers数组添加ngx_http_core_srv_conf_t结构  */
//可能相同的IP:port配置在不同的server{}中，那么他们处于同一个ngx_http_conf_addr_t，但所属ngx_http_core_srv_conf_t不同
//例如不同server{}中有相同的listen ip:port，他们都在同一个ngx_http_conf_addr_t中，但servers指向不同
static ngx_int_t
ngx_http_add_server(ngx_conf_t *cf, ngx_http_core_srv_conf_t *cscf,
    ngx_http_conf_addr_t *addr)
{
    ngx_uint_t                  i;
    ngx_http_core_srv_conf_t  **server;

    if (addr->servers.elts == NULL) {
        if (ngx_array_init(&addr->servers, cf->temp_pool, 4,
                           sizeof(ngx_http_core_srv_conf_t *))
            != NGX_OK)
        {
            return NGX_ERROR;
        }

    } else {
        server = addr->servers.elts;
        for (i = 0; i < addr->servers.nelts; i++) {
            if (server[i] == cscf) { //如果在同一个serve{}中配置两条相同的listen ip:port，两次的ip和port完全相等，则会走到这里
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "a duplicate listen %s", addr->opt.addr);
                return NGX_ERROR;
            }
        }
    }

    /*  */
    server = ngx_array_push(&addr->servers);
    if (server == NULL) {
        return NGX_ERROR;
    }

    *server = cscf;

    return NGX_OK;
}

/*
这个函数就是遍历所有的端口号，将端口号对应的地址结构的hash、wc_head和wc_tail初始化，这个在初始化后面的ngx_listening_t的servers字
段时会用到。然后调用ngx_http_init_listening函数完成ngx_listening_t初始化。
*/ //可以参考http://blog.csdn.net/chosen0ne/article/details/7754608
static ngx_int_t
ngx_http_optimize_servers(ngx_conf_t *cf, ngx_http_core_main_conf_t *cmcf,
    ngx_array_t *ports)
{
    ngx_uint_t             p, a;
    ngx_http_conf_port_t  *port;
    ngx_http_conf_addr_t  *addr;

    if (ports == NULL) {
        return NGX_OK;
    }

    port = ports->elts;
    for (p = 0; p < ports->nelts; p++) {
        //将addrs排序，带通配符的地址排在后面， (listen 1.2.2.2:30 bind) > listen 1.1.1.1:30  > listen *:30
        ngx_sort(port[p].addrs.elts, (size_t) port[p].addrs.nelts,
                 sizeof(ngx_http_conf_addr_t), ngx_http_cmp_conf_addrs);

        /*
         * check whether all name-based servers have the same
         * configuration as a default server for given address:port
         */

        addr = port[p].addrs.elts;
        for (a = 0; a < port[p].addrs.nelts; a++) {
            /* 多个server{}下面有listen IP:port ，并且每个server{}中的端口都相等，则他们保存在同一个port[i]中，只是ip地址不一样，以addrs区分 */
            if (addr[a].servers.nelts > 1
#if (NGX_PCRE)
                || addr[a].default_server->captures
#endif
               )
            { //相同端口，不同IP地址对应的server{},把每个server中的server_names配置进行hash存储
             /*
                初始addr(ngx_http_conf_addr_t)中的hash、wc_head和wc_tail哈希表。 这些哈希表以server_name（虚拟主机名）为key，server块
                的ngx_http_core_srv_conf_t为 value，用于在处理请求时，根据请求的host请求行快速找到处理该请求的server配置结构。
                */
                if (ngx_http_server_names(cf, cmcf, &addr[a]) != NGX_OK) {
                    return NGX_ERROR;
                }
            }
        }

        if (ngx_http_init_listening(cf, &port[p]) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_server_names(ngx_conf_t *cf, ngx_http_core_main_conf_t *cmcf,
    ngx_http_conf_addr_t *addr)
{
    ngx_int_t                   rc;
    ngx_uint_t                  n, s;
    ngx_hash_init_t             hash;
    ngx_hash_keys_arrays_t      ha;
    ngx_http_server_name_t     *name;
    ngx_http_core_srv_conf_t  **cscfp;
#if (NGX_PCRE)
    ngx_uint_t                  regex, i;

    regex = 0;
#endif

    ngx_memzero(&ha, sizeof(ngx_hash_keys_arrays_t));

    ha.temp_pool = ngx_create_pool(NGX_DEFAULT_POOL_SIZE, cf->log);
    if (ha.temp_pool == NULL) {
        return NGX_ERROR;
    }

    ha.pool = cf->pool;

    if (ngx_hash_keys_array_init(&ha, NGX_HASH_LARGE) != NGX_OK) {
        goto failed;
    }

    cscfp = addr->servers.elts;

    for (s = 0; s < addr->servers.nelts; s++) {

        name = cscfp[s]->server_names.elts; //获取与该listen ip:port处于同一个server{]里面的 server_names配置信息

        for (n = 0; n < cscfp[s]->server_names.nelts; n++) {

#if (NGX_PCRE)
            if (name[n].regex) { //通配符类型的server_name配置
                regex++;
                continue;
            }
#endif

            rc = ngx_hash_add_key(&ha, &name[n].name, name[n].server,
                                  NGX_HASH_WILDCARD_KEY); //把name添加到ha中对应的完全匹配 前者匹配 后置匹配数组或者hash中

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (rc == NGX_DECLINED) {
                ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                              "invalid server name or wildcard \"%V\" on %s",
                              &name[n].name, addr->opt.addr);
                return NGX_ERROR;
            }

            if (rc == NGX_BUSY) {
                ngx_log_error(NGX_LOG_WARN, cf->log, 0,
                              "conflicting server name \"%V\" on %s, ignored",
                              &name[n].name, addr->opt.addr);
            }
        }
    }

    hash.key = ngx_hash_key_lc;
    hash.max_size = cmcf->server_names_hash_max_size;
    hash.bucket_size = cmcf->server_names_hash_bucket_size;
    hash.name = "server_names_hash";
    hash.pool = cf->pool;

    if (ha.keys.nelts) { //完全匹配
        hash.hash = &addr->hash;
        hash.temp_pool = NULL;

        if (ngx_hash_init(&hash, ha.keys.elts, ha.keys.nelts) != NGX_OK) {
            goto failed;
        }
    }

    if (ha.dns_wc_head.nelts) { //前置匹配

        ngx_qsort(ha.dns_wc_head.elts, (size_t) ha.dns_wc_head.nelts,
                  sizeof(ngx_hash_key_t), ngx_http_cmp_dns_wildcards);

        hash.hash = NULL;
        hash.temp_pool = ha.temp_pool;

        if (ngx_hash_wildcard_init(&hash, ha.dns_wc_head.elts,
                                   ha.dns_wc_head.nelts)
            != NGX_OK)
        {
            goto failed;
        }

        addr->wc_head = (ngx_hash_wildcard_t *) hash.hash;
    }

    if (ha.dns_wc_tail.nelts) { //后置匹配

        ngx_qsort(ha.dns_wc_tail.elts, (size_t) ha.dns_wc_tail.nelts,
                  sizeof(ngx_hash_key_t), ngx_http_cmp_dns_wildcards);

        hash.hash = NULL;
        hash.temp_pool = ha.temp_pool;

        if (ngx_hash_wildcard_init(&hash, ha.dns_wc_tail.elts,
                                   ha.dns_wc_tail.nelts)
            != NGX_OK)
        {
            goto failed;
        }

        addr->wc_tail = (ngx_hash_wildcard_t *) hash.hash;
    }

    ngx_destroy_pool(ha.temp_pool);

#if (NGX_PCRE)

    if (regex == 0) {
        return NGX_OK;
    }

    addr->nregex = regex;
    addr->regex = ngx_palloc(cf->pool, regex * sizeof(ngx_http_server_name_t));
    if (addr->regex == NULL) {
        return NGX_ERROR;
    }

    i = 0;

    for (s = 0; s < addr->servers.nelts; s++) {

        name = cscfp[s]->server_names.elts;

        for (n = 0; n < cscfp[s]->server_names.nelts; n++) {
            if (name[n].regex) {
                addr->regex[i++] = name[n];
            }
        }
    }

#endif

    return NGX_OK;

failed:

    ngx_destroy_pool(ha.temp_pool);

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_cmp_conf_addrs(const void *one, const void *two)
{
    ngx_http_conf_addr_t  *first, *second;

    first = (ngx_http_conf_addr_t *) one;
    second = (ngx_http_conf_addr_t *) two;

    if (first->opt.wildcard) {
        /* a wildcard address must be the last resort, shift it to the end */
        return 1;
    }

    if (second->opt.wildcard) {
        /* a wildcard address must be the last resort, shift it to the end */
        return -1;
    }

    if (first->opt.bind && !second->opt.bind) {
        /* shift explicit bind()ed addresses to the start */
        return -1;
    }

    if (!first->opt.bind && second->opt.bind) {
        /* shift explicit bind()ed addresses to the start */
        return 1;
    }

    /* do not sort by default */

    return 0;
}


static int ngx_libc_cdecl
ngx_http_cmp_dns_wildcards(const void *one, const void *two)
{
    ngx_hash_key_t  *first, *second;

    first = (ngx_hash_key_t *) one;
    second = (ngx_hash_key_t *) two;

    return ngx_dns_strcmp(first->key.data, second->key.data);
}

static ngx_int_t
ngx_http_init_listening(ngx_conf_t *cf, ngx_http_conf_port_t *port)
{
    ngx_uint_t                 i, last, bind_wildcard;
    ngx_listening_t           *ls;
    ngx_http_port_t           *hport;
    ngx_http_conf_addr_t      *addr;

    addr = port->addrs.elts;
    last = port->addrs.nelts;

    /*
     * If there is a binding to an "*:port" then we need to bind() to
     * the "*:port" only and ignore other implicit bindings.  The bindings
     * have been already sorted: explicit bindings are on the start, then
     * implicit bindings go, and wildcard binding is in the end.  //例如有listen 80(implicit bindings);  listen *:80,则第一个无效，直接用第二个就行了
     */

    if (addr[last - 1].opt.wildcard) { //"*:port"  addr是拍了序的，见ngx_http_optimize_servers，最后面的是通配符
        addr[last - 1].opt.bind = 1; //如果是通配符，这里把bind值1
        bind_wildcard = 1; //表示有通配符listen

    } else {
        bind_wildcard = 0;
    }

    i = 0;

/*
 这个函数就是遍历某个端口port对应的所有address，如果所有address中不包含通配符，则对所有的address:port调用ngx_http_add_listening分配一
 个listen结构和ngx_http_port_t结构，并初始化它们。如果存在address包含通配符，则如果address:port需要bind，分配一个listen结构和
 ngx_http_port_t结构，并初始化它们，对所有address:port不需要bind的，它们和包含通配符*:port共同使用一个listen结构和ngx_http_port_t结构，
 并且listen结构中包含的地址是*:port，所以最好bind的地址是*:port。所有的listen都会存放在全局变量ngx_cycle的listening数组中，这样后面就
 可以利用这些address:port信息建立每个套接字了。
 */
    while (i < last) {
    //last代表的是address:port的个数，  如果没有通配符配置项，则有多少个last，就有多少次循环。bind=1的有多少次就执行多少次，如果有通配符和bind = 0的listen配置，
    //则在后面的if (bind_wildcard && !addr[i].opt.bind)进行continue，也就是这些未精确配置项合在一起在后面置执行一次分配ngx_http_port_t空间，把他们算在
    //addr[i]中，这里的i是通配符所在位置。

        //对所有address:port不需要bind的，它们和包含通配符*:port共同使用一个listen结构和ngx_http_port_t结构， 并且listen结构中包含的地址是*:port，所以最好bind的地址是*:port
        if (bind_wildcard && !addr[i].opt.bind) { //如果是通配符*:port,或者是listen配置没有加bind参数
            i++;//如果有通配符配置，并且bind = 0则把这些bind=0和通配符配置算作一项，执行后面的操作。通配符的bind在该函数前面置1，见addr[last - 1].opt.bind = 1
            continue;
        }

        //为该listen创建对应的ngx_listening_t结构并赋值
        ls = ngx_http_add_listening(cf, &addr[i]);
        if (ls == NULL) {
            return NGX_ERROR;
        }

        hport = ngx_pcalloc(cf->pool, sizeof(ngx_http_port_t));
        if (hport == NULL) {
            return NGX_ERROR;
        }

       /*
         * servers会用来保存虚拟主机的信息，在处理请求时会赋值给request 用于进行虚拟主机的匹配
         */
        ls->servers = hport;

        //如果是未精确配置的listen(bind = 0并且有配置一项通配符，则这里的i是通配符所在addr[]的位置)，如果没有配置通配符，则有多少个listen配置就会执行这里多少次。
        //只是在出现通配符listen的配置中，把未精确配置的所有项合到通配符所在addr[]位置
        hport->naddrs = i + 1; //保护listen通配符配置，并且没有bind的listen项数
        switch (ls->sockaddr->sa_family) {

#if (NGX_HAVE_INET6)
        case AF_INET6:
            if (ngx_http_add_addrs6(cf, hport, addr) != NGX_OK) {
                return NGX_ERROR;
            }
            break;
#endif
        default: /* AF_INET */
            if (ngx_http_add_addrs(cf, hport, addr) != NGX_OK) { //后面有addr++，所以这里的addr对应的是addr[i]的地址
                return NGX_ERROR;
            }
            break;
        }

        if (ngx_clone_listening(cf, ls) != NGX_OK) {
            return NGX_ERROR;
        }

        addr++;
        last--;
    }

    return NGX_OK;
}

//ngx_event_process_init
//master进程执行ngx_clone_listening中如果配置了多worker，监听80端口会有worker个listen赋值，master进程在ngx_open_listening_sockets
//中会监听80端口worker次，那么子进程创建起来后，不是每个字进程都关注这worker多个 listen事件了吗?为了避免这个问题，nginx通过
//在子进程运行ngx_event_process_init函数的时候，通过ngx_add_event来控制子进程关注的listen，最终实现只关注master进程中创建的一个listen事件


//ngx_listening_t创建空间，并通过addr赋值初始化
static ngx_listening_t *
ngx_http_add_listening(ngx_conf_t *cf, ngx_http_conf_addr_t *addr)
{
    ngx_listening_t           *ls;
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_core_srv_conf_t  *cscf;


    //为listen配置创建对应的ngx_listening_t结构，并赋值IP地址等，里面也会完成IP地址字符串格式的转换
    ls = ngx_create_listening(cf, &addr->opt.u.sockaddr, addr->opt.socklen);
    if (ls == NULL) {
        return NULL;
    }

    ls->addr_ntop = 1;

    // 设置ngx_listening_t的handler，这个handler会在监听到客户端连接时被调用，具体就是在ngx_event_accept函数中，ngx_http_init_connection函数顾名思义，就是初始化这个新建的连接
    ls->handler = ngx_http_init_connection;

    cscf = addr->default_server;
    ls->pool_size = cscf->connection_pool_size;
    ls->post_accept_timeout = cscf->client_header_timeout;

    clcf = cscf->ctx->loc_conf[ngx_http_core_module.ctx_index];

    ls->logp = clcf->error_log;
    ls->log.data = &ls->addr_text;
    ls->log.handler = ngx_accept_log_error; //该listen上面的打印会加上listen后面的IP地址字符串格式

#if (NGX_WIN32)
    {
    ngx_iocp_conf_t  *iocpcf = NULL;

    if (ngx_get_conf(cf->cycle->conf_ctx, ngx_events_module)) {
        iocpcf = ngx_event_get_conf(cf->cycle->conf_ctx, ngx_iocp_module);
    }
    if (iocpcf && iocpcf->acceptex_read) {
        ls->post_accept_buffer_size = cscf->client_header_buffer_size;
    }
    }
#endif

    ls->backlog = addr->opt.backlog;
    ls->rcvbuf = addr->opt.rcvbuf;
    ls->sndbuf = addr->opt.sndbuf;

    ls->keepalive = addr->opt.so_keepalive;
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    ls->keepidle = addr->opt.tcp_keepidle;
    ls->keepintvl = addr->opt.tcp_keepintvl;
    ls->keepcnt = addr->opt.tcp_keepcnt;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined SO_ACCEPTFILTER)
    ls->accept_filter = addr->opt.accept_filter;
#endif

#if (NGX_HAVE_DEFERRED_ACCEPT && defined TCP_DEFER_ACCEPT)
    ls->deferred_accept = addr->opt.deferred_accept;
#endif

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    ls->ipv6only = addr->opt.ipv6only;
#endif

#if (NGX_HAVE_SETFIB)
    ls->setfib = addr->opt.setfib;
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
    ls->fastopen = addr->opt.fastopen;
#endif

#if (NGX_HAVE_REUSEPORT)
    ls->reuseport = addr->opt.reuseport;
#endif

    return ls;
}

/*
ngx_http_add_addrs函数用于初始化ls->servers，这个属性主要是存放该监听socket对应的虚拟主机的信息，在处理请求时根据请求行的host匹配，
选择对应的一个server块的ngx_http_core_srv_conf_t结构，这个结构里存放了刚请求处理的全局配置信息
*/ //该函数主要把listen对应的server_name配置信息存放在hport->addrs[]中
static ngx_int_t
ngx_http_add_addrs(ngx_conf_t *cf, ngx_http_port_t *hport,
    ngx_http_conf_addr_t *addr)
{
    ngx_uint_t                 i;
    ngx_http_in_addr_t        *addrs;
    struct sockaddr_in        *sin;
    ngx_http_virtual_names_t  *vn;

    hport->addrs = ngx_pcalloc(cf->pool,
                               hport->naddrs * sizeof(ngx_http_in_addr_t));
    if (hport->addrs == NULL) {
        return NGX_ERROR;
    }

    addrs = hport->addrs;

    //一般naddrs为1，如果出现listen未精确配置(bind=0)并且有通配符配置项,才会大于1，这时候的addr未最后的通配符配置项的addr，这时候所有的未精确
    //listen配置的hport->addrs[]内容都是从通配符addr中获取。
    for (i = 0; i < hport->naddrs; i++) {

        sin = &addr[i].opt.u.sockaddr_in;
        addrs[i].addr = sin->sin_addr.s_addr;
        addrs[i].conf.default_server = addr[i].default_server; //server配置信息
#if (NGX_HTTP_SSL)
        addrs[i].conf.ssl = addr[i].opt.ssl;
#endif
#if (NGX_HTTP_V2)
        addrs[i].conf.http2 = addr[i].opt.http2;
#endif
        addrs[i].conf.proxy_protocol = addr[i].opt.proxy_protocol;

        //如果该server{}内有配置listen ,但没有配置server_name项，则直接返回
        if (addr[i].hash.buckets == NULL
            && (addr[i].wc_head == NULL
                || addr[i].wc_head->hash.buckets == NULL)
            && (addr[i].wc_tail == NULL
                || addr[i].wc_tail->hash.buckets == NULL)
#if (NGX_PCRE)
            && addr[i].nregex == 0
#endif
            )
        {
            continue;
        }

        vn = ngx_palloc(cf->pool, sizeof(ngx_http_virtual_names_t));
        if (vn == NULL) {
            return NGX_ERROR;
        }

        addrs[i].conf.virtual_names = vn;

        //这里面存储的server_name是相同ip:port所处的多个不同server{]配置中的所有server_name配置
        vn->names.hash = addr[i].hash; //完全匹配
        vn->names.wc_head = addr[i].wc_head; //前置匹配
        vn->names.wc_tail = addr[i].wc_tail; //后置匹配
#if (NGX_PCRE)
        vn->nregex = addr[i].nregex;
        vn->regex = addr[i].regex;
#endif
    }

    return NGX_OK;
}


#if (NGX_HAVE_INET6)

static ngx_int_t
ngx_http_add_addrs6(ngx_conf_t *cf, ngx_http_port_t *hport,
    ngx_http_conf_addr_t *addr)
{
    ngx_uint_t                 i;
    ngx_http_in6_addr_t       *addrs6;
    struct sockaddr_in6       *sin6;
    ngx_http_virtual_names_t  *vn;

    hport->addrs = ngx_pcalloc(cf->pool,
                               hport->naddrs * sizeof(ngx_http_in6_addr_t));
    if (hport->addrs == NULL) {
        return NGX_ERROR;
    }

    addrs6 = hport->addrs;

    for (i = 0; i < hport->naddrs; i++) {

        sin6 = &addr[i].opt.u.sockaddr_in6;
        addrs6[i].addr6 = sin6->sin6_addr;
        addrs6[i].conf.default_server = addr[i].default_server;
#if (NGX_HTTP_SSL)
        addrs6[i].conf.ssl = addr[i].opt.ssl;
#endif
#if (NGX_HTTP_V2)
        addrs6[i].conf.http2 = addr[i].opt.http2;
#endif

        if (addr[i].hash.buckets == NULL
            && (addr[i].wc_head == NULL
                || addr[i].wc_head->hash.buckets == NULL)
            && (addr[i].wc_tail == NULL
                || addr[i].wc_tail->hash.buckets == NULL)
#if (NGX_PCRE)
            && addr[i].nregex == 0
#endif
            )
        {
            continue;
        }

        vn = ngx_palloc(cf->pool, sizeof(ngx_http_virtual_names_t));
        if (vn == NULL) {
            return NGX_ERROR;
        }

        addrs6[i].conf.virtual_names = vn;

        vn->names.hash = addr[i].hash;
        vn->names.wc_head = addr[i].wc_head;
        vn->names.wc_tail = addr[i].wc_tail;
#if (NGX_PCRE)
        vn->nregex = addr[i].nregex;
        vn->regex = addr[i].regex;
#endif
    }

    return NGX_OK;
}

#endif


char *
ngx_http_types_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_array_t     **types;
    ngx_str_t        *value, *default_type;
    ngx_uint_t        i, n, hash;
    ngx_hash_key_t   *type;

    types = (ngx_array_t **) (p + cmd->offset);

    if (*types == (void *) -1) {
        return NGX_CONF_OK;
    }

    default_type = cmd->post;

    if (*types == NULL) {
        *types = ngx_array_create(cf->temp_pool, 1, sizeof(ngx_hash_key_t));
        if (*types == NULL) {
            return NGX_CONF_ERROR;
        }

        if (default_type) {
            type = ngx_array_push(*types);
            if (type == NULL) {
                return NGX_CONF_ERROR;
            }

            type->key = *default_type;
            type->key_hash = ngx_hash_key(default_type->data,
                                          default_type->len);
            type->value = (void *) 4;
        }
    }

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {

        if (value[i].len == 1 && value[i].data[0] == '*') {
            *types = (void *) -1;
            return NGX_CONF_OK;
        }

        hash = ngx_hash_strlow(value[i].data, value[i].data, value[i].len);
        value[i].data[value[i].len] = '\0';

        type = (*types)->elts;
        for (n = 0; n < (*types)->nelts; n++) {

            if (ngx_strcmp(value[i].data, type[n].key.data) == 0) {
                ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                                   "duplicate MIME type \"%V\"", &value[i]);
                goto next;
            }
        }

        type = ngx_array_push(*types);
        if (type == NULL) {
            return NGX_CONF_ERROR;
        }

        type->key = value[i];
        type->key_hash = hash;
        type->value = (void *) 4;

    next:

        continue;
    }

    return NGX_CONF_OK;
}


char *
ngx_http_merge_types(ngx_conf_t *cf, ngx_array_t **keys, ngx_hash_t *types_hash,
    ngx_array_t **prev_keys, ngx_hash_t *prev_types_hash,
    ngx_str_t *default_types)
{
    ngx_hash_init_t  hash;

    if (*keys) {

        if (*keys == (void *) -1) {
            return NGX_CONF_OK;
        }

        hash.hash = types_hash;
        hash.key = NULL;
        hash.max_size = 2048;
        hash.bucket_size = 64;
        hash.name = "test_types_hash";
        hash.pool = cf->pool;
        hash.temp_pool = NULL;

        if (ngx_hash_init(&hash, (*keys)->elts, (*keys)->nelts) != NGX_OK) {
            return NGX_CONF_ERROR;
        }

        return NGX_CONF_OK;
    }

    if (prev_types_hash->buckets == NULL) {

        if (*prev_keys == NULL) {

            if (ngx_http_set_default_types(cf, prev_keys, default_types)
                != NGX_OK)
            {
                return NGX_CONF_ERROR;
            }

        } else if (*prev_keys == (void *) -1) {
            *keys = *prev_keys;
            return NGX_CONF_OK;
        }

        hash.hash = prev_types_hash;
        hash.key = NULL;
        hash.max_size = 2048;
        hash.bucket_size = 64;
        hash.name = "test_types_hash";
        hash.pool = cf->pool;
        hash.temp_pool = NULL;

        if (ngx_hash_init(&hash, (*prev_keys)->elts, (*prev_keys)->nelts)
            != NGX_OK)
        {
            return NGX_CONF_ERROR;
        }
    }

    *types_hash = *prev_types_hash;

    return NGX_CONF_OK;

}


ngx_int_t
ngx_http_set_default_types(ngx_conf_t *cf, ngx_array_t **types,
    ngx_str_t *default_type)
{
    ngx_hash_key_t  *type;

    *types = ngx_array_create(cf->temp_pool, 1, sizeof(ngx_hash_key_t));
    if (*types == NULL) {
        return NGX_ERROR;
    }

    while (default_type->len) {

        type = ngx_array_push(*types);
        if (type == NULL) {
            return NGX_ERROR;
        }

        type->key = *default_type;
        type->key_hash = ngx_hash_key(default_type->data,
                                      default_type->len);
        type->value = (void *) 4;

        default_type++;
    }

    return NGX_OK;
}
