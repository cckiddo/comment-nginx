# 添加一个新模块

## 确定要添加的模块信息

1. 首先，命名: ngx_http_mytest_module
2. 分析功能及配置的范围，比如 loc, server, main
3. 如果 HTTP 的类型， HTTP 模块，或者是 filter 模块
4. 确定 HTTP 的 11 个阶段

## 编写config文件

推荐使用 config 文件的方式添加，简单的示例  

```shell
# 一般设置为模块名称
ngx_addon_name=ngx_http_mytest_module
# 保存所有HTTP模块名称
HTTP_MODULES="$HTTP_MODULES ngx_http_mytest_module"
# 指定新增模块的源代码，其他模块: 核心模块, 事件模块, HTTP模块, HTTP过滤模块, HTTP头部过滤模块
# $HTTP_FILTER_MODULES, $CORE_MODULES, $EVENT_MODULES, $HTTP_MODULES, $HTTP_FILTER_MODULES $HTTP_HEADERS_FILTER_MODULE
NGX_ADDON_SRCS="$NGX_ADDON_SRCS $ngx_addon_dir/ngx_http_mytest_module.c"

# $NGX_ADDON_DEPS, 指定了模块信赖的路径，指定 .h 文件等
```

另外可以直接修改 Makefile 来添加模块，只是较复杂且不好用  
objs/Makefile, objs/ngx_modules.c  

## 将模块编译进程序

通过编译时添加 --add-module=modure_dir 的方式添加模块  

```shell
# 编译安装
./configure --prefix=/usr/local/nginx-1.15 --add-module=/home/caiyx/nginx/nginx-1.15.5/src/http/modules/mytest && make install
```

编译之后查看模块是否加载成功: fgrep hello ./objs/ngx_modules.c  

## 配置中使用新模块

```conf
location /test {
    hello_string haha;
    hello_counter on;
}
```

> 访问触发功能

curl -voa -x 127.1:80 http://127.0.0.1/test  
