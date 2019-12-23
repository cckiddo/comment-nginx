# upstream

Nginx 的核心功能-反向代理基于 upstream 模块。属于 HTTP 框架的一部分。  

思考问题:  

1. upstream 是如何嵌入到一个请求中的
2. 

## upstream 是如何嵌入到一个请求中的

r->upstream, 不启用 upstream 时，指针为空  

1. 创建 upstream 的函数： ngx_http_upstream_create(),
2. 设置第三方服务器的地址
3. 设置 upstream 的回调方法
4. 调用 ngx_http_upstream_init() 方法启用 upstream

ngx_http_mytest_handler(), 此方法必须返回 NGX_DONE, 告诉 HTTP 框架不要再按阶段继续向下处理请求了  

r->subrequest_in_memory, 决定使用哪种处理上游响应包体的方式。  

1. subrequest_in_memory=1, upstream 不转发响应包体到下游，由 HTTP 模块实现的 input_filter 方法处理包体
2. subrequest_in_memory=0, upstream 会转发响应包体
3. buffering=0, 将使用固定大小的缓冲区来转发响应包体

## 核心对象

ngx_http_upstream_t
ngx_http_upstream_init(), 返回 NGX_DONE 告诉 HTTP 框架暂停执行请求的下一阶段。还需要执行 r->main->count++ ，告诉 HTTP 框架将当前请求的引用计数加1，不要销毁请求。  


## 8个回调方法

必须实现的3个回调：  

1. creat_request()
2. process_header()
3. finalize_request()

## upstream阶段的主要处理流程

## upstream的hook点
