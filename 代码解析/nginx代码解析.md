# nginx代码解析

## upstream&proxy

Nginx 的核心功能-反向代理基于 upstream 模块。属于 HTTP 框架的一部分。  

### proxy模块在http框架的入口

proxy 模块提供了代理的头设置及对后端响应进行解析处理的功能，实际上 proxy 模块基本也是做了设置和解析处理的功能，具体的上游服务器的连接 接收及到向下游请求端的转发及web内容的缓存主要是upstream模块来提供的。  

具体入口为：解析 proxy_pass 指令时，会将 ngx_http_core_module 的 handler 成员赋值为ngx_http_proxy_handler  

> 在 NGX_HTTP_FIND_CONFIG_PHASE 阶段，会调用 ngx_http_core_find_config_phase -> ngx_http_update_location_config 中将 ngx_http_proxy_handler 赋值给请求结构体的 content_handler 成员  

- ngx_http_block()
- 解析进行到 ngx_http_core_location()
- 解析指令： ngx_http_proxy_pass()
- 设置  ngx_http_core_module 的 loc_conf 的 handler 指针为 ngx_http_proxy_handler()
- ngx_http_block 解析完成前设置 phase_handler
- NGX_HTTP_FIND_CONFIG_PHASE 的 check 函数设置为 ngx_http_core_find_config_phase()
- NGX_HTTP_CONTENT_PHASE 的 checker 函数设置为 ngx_http_core_content_phase

> 在 NGX_HTTP_CONTENT_PHASE 阶段，会调用 r->content_handler ，即执行 ngx_http_proxy_hander 开始进入 upstream 的流程

- 进入请求的阶段处理函数： ngx_http_core_run_phases()
- 进行到 NGX_HTTP_FIND_CONFIG_PHASE 阶段时，在 ngx_http_update_location_config() 中将 r->content_handler 赋值为 clcf->handler
- 进行到 NGX_HTTP_CONTENT_PHASE 时，在 ngx_http_core_content_phase 中调用 r->content_handler
- 执行 r->content_handler 开始 upstream 流程

### upstream阶段的主要处理流程

Nginx访问上游服务器的流程大致可以分为以下几个阶段：启动upstream机制、连接上游服务器、向上游服务器发送请求、接收上游服务器的响应包头、处理接收到的响应包体、结束请求。  

#### 启动 upstream

在proxy模块的介绍中可以看到，最终连接后端需要调用 ngx_http_proxy_hander， 在这个函数中会调用 ngx_http_upsteam_create 创建upsteam,  

接着调用 ngx_http_upstream_init 方法，根据 ngx_http_upstream_conf_t 中的成员初始化 upstream，同时会开始连接上游服务器，以此展开整个upstream处理流程。  

- ngx_http_upstream_init(), 启动了 upstream 机制
- ngx_http_upstream_init_request()，开始连接上游服务器

#### 连接上游服务器

- ngx_http_upstream_connect(), 连接上游服务器。创建socket、connetion，发起tcp建连请求，发送请求，挂接upstream的handler，包括第4、5步中处理上游应答的处理函数

#### 向上游服务器发送请求

- ngx_http_upstream_send_request()
- ngx_http_upstream_process_header()，接收上游服务器的响应包头。作为上游的连接的读事件处理函数，用于处理接收到的http头部信息
  - process_header()方法解析响应头部

#### 接收上游服务器的响应包头

- ngx_http_upstream_process_headers()处理请求头

#### 处理上游服务器的响应印刷体

接第四步的说明， 在处理上游的响应头之后可以看到，处理响应包体有两种情况：  

在内存中处理，不转发响应 ，调用 ngx_http_upstream_process_body_in_memory ， 该函数对应子请求的body处理  

如果需要转发相应体，可以自己实现input_filter，若不自己实现则使用默认的 ngx_http_upstream_non_buffered_filter ； 默认是将响应的body全部保存在内存中。  

需要转发响应，调用 ngx_http_upstream_send_response ， 分为两种处理方式：  
(1)upstream与上游之间网速很快时，使用大内存甚至文件，缓存上游大请求  
(2)upstream与下游之间网速很快时，使用固定大小内存，不需要过多缓存请求  
根据配置决定，目前shark中buffering一般配置为 0，即使用固定大小内存的方式  

#### 结束请求

与上游服务器交互出错，或者正常处理来自上游的响应时，就需要结束请求，释放upstream中使用到的资源。  

upstream提供的方法为 ngx_http_upstream_finalize_request 用于结束upsteam请求。除了直接调用该方法，还有两种方法： ngx_http_upsteam_cleanup 和 ngx_http_upstream_next 。

ngx_http_upsteam_cleanup 方法：在启动upstream即步骤1中 ngx_http_upsteam_cleanup 会挂到请求的cleanup链表上，由http框架在请求客户请求结束时调用。最终还是调用了 ngx_http_upstream_finalize_request 。  

ngx_http_upstream_next 方法：这个方法会进行判断是否需要重新发起请求。
