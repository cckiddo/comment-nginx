# nginx配置

1. 过期时间: expires 30s;
2. [access_log 日志格式设置](http://blog.csdn.net/czlun/article/details/73251723)
3. 配置 debug 日志，编译添加 --with-debug, 配置 error_log
4. 添加自定义响应头， add_header "Content-Length" 100;或者 more_set_headers "Server: HostSoft Web Server";
