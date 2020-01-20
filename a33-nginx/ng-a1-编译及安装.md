# nginx安装启用

## centos7安装nginx

> 安装依赖库

```shell
yum -y install gcc gcc-c++ pcre pcre-devel zlib zlib-devel openssl openssl-devel
```

> 解压源码, 添加编译参考并编译安装nginx

```shell
# 编译
./configure --prefix=/usr/local/nginx-1.15.5 --with-debug --with-http_ssl_module
# 安装
make && make install
```

## nginx命令配置解析和使用

nginx -t -c nginx.conf, 解析配置  
nginx -c nginx.conf, 启动 nginx  
nginx -V, 查看编译信息  
nginx -v, 查看 nginx 的版本信息

要注意这里配置要使用绝对路径  

## 参考文档

- [选择Nginx的原因](http://www.nginx.cn/nginxchswhyuseit)
- [Nginx中文文档](http://www.nginx.cn/doc/index.html)
- [Nginx安装文档：](http://www.nginx.cn/install)
- [提醒pcre不存在](http://blog.sina.com.cn/s/blog_4ad7c2540101duql.html)
- [配置SSL模块](https://www.cnblogs.com/saneri/p/5391821.html)
- [nginx使用ssl模块配置支持HTTPS访问](https://www.cnblogs.com/saneri/p/5391821.html)
- [vscode调试nginx](https://zhuanlan.zhihu.com/p/47236996)
