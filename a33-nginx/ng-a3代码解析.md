<!-- TOC -->

- [nginx代码解析](#nginx代码解析)
    - [添加一个模块](#添加一个模块)
    - [upstream模块](#upstream模块)
        - [upstream 是如何嵌入到一个请求中的](#upstream-是如何嵌入到一个请求中的)
        - [核心对象](#核心对象)
        - [8个回调方法](#8个回调方法)

<!-- /TOC -->

# nginx代码解析

1. 添加一个模块
2. 配置解析
3. 添加一个 upstream 模块
4. 添加一个变量
5. 添加一个 filter 模块
6. work-master模式
7. 共享内存
8. event事件

## 添加一个模块

添加一个模块有很多非常重要的数据  

> 模块的 config 文件，标识模块的名称和源代码

如 ngx_addon_name=ngx_http_mytest_module  

> ngx_command_t 结构数据，标识模块的名称，set 具体操作函数，比如用来注册 handler 点

一个 commands 可能要有**多个** ngx_command_t 结构体，比较简单的模块可能只有一个 ngx_command_t 结构体，每个 ngx_command_t 都要有自己的处理函数。  
如简单的 mytest 模块中中， set = ngx_http_mytest, ngx_http_mytest() 中取的是 ngx_http_core_module 模块的 handler ，clcf->handler = ngx_http_mytest_handler ，所以生效的就是 NGX_HTTP_CONTENT_PHASE 阶段的处理  

ngx_init_cycle(), 配置解析时，就会去加载对应 command 的 set 函数去处理  
ngx_conf_parse() -> ngx_conf_handler() -> set()  

```c
/** 用来定义模块的配置文件参数，每一个数组元素都是 ngx_command_t 类型，结尾用 ngx_null_command */
struct ngx_command_s {
    ngx_str_t             name; /** 配置项名称，如 "gzip" */
    /** 配置项类型， type 将指定配置项可以出现的位置。例如，出现在 server{} 或 location{}
     * 中，以及它可以携带的参数个数 */
    ngx_uint_t            type;
    /** 出现了 name 中指定的配置项后，将会调用 set 方法处理配置项的参数 */
    char               *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
    /** 在配置文件中的偏移量 */
    ngx_uint_t            conf;
    /** 通常用于使用预设的解析方法解析配置项，这是配置模块的一个优秀设计。它需要与 conf 配合使用 */
    ngx_uint_t            offset;
    void                 *post; /** 配置项读取后的处理方法，必须是 ngx_conf_post_t 结构的指针 */
};
```

> 上下文 ctx

ctx在配置解析的阶段就会去展开， ngx_http_module 模块的 ngx_http_block() 调用。由 core 核心模块调用  

```c
typedef struct {
    /** 解析配置文件前调用 */
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf);
    /** 完成配置文件的解析后调用 */
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf);

    /** 当需要创建数据结构用于存储 main 级别（直属于 http{...}块的配置项）的全局配置时，
     * 可以通过 create_main_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_main_conf)(ngx_conf_t *cf);

    /** 常用于初始化 main 级别配置项 */
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);

    /** 当需要创建数据结构用于存储 srv 级别（直属于 server{...}块的配置项）的全局配置时，
     * 可以通过 create_srv_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_srv_conf)(ngx_conf_t *cf);

    /** 主要用于合并 main 级别和 srv 级别下的同名配置项 */
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);

    /** 当需要创建数据结构用于存储 loc 级别（直属于 location{...}块的配置项）的全局配置时，
     * 可以通过 create_loc_conf 回调方法创建存储全局配置项的结构体 */
    void       *(*create_loc_conf)(ngx_conf_t *cf);
    /** 主要用于合并 srv 级别和 loc 级别下的同名配置项 */
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
} ngx_http_module_t;
```

> ngx_module_t

ngx_cycle_modules()

```c
struct ngx_module_s {
    /* 对一类模块（由下面的 type 成员决定类别）而言， ctx_index 表示当前模块在这类模块中的
       序号。这个成员常常是由管理这类模块的一个 Nginx 核心模块设置的，对于所有的 HTTP 模块而
       言， ctx_index 是由核心模块 ngx_http_module 设置的。 ctx_index 非常重要， Nginx
       的模块化设计非常依赖于和个模块的顺序，它们既用于表达优先级，也用于表明每个模块的位置，借
       以帮助 Nginx 架构快速获得某个模块的数据  */
    ngx_uint_t            ctx_index;

    /* 表示当前模块在 ngx_modules 数组中的序号。注意， ctx_index 表示的是当前模块在一类模块
       中的序号，而 index 表示当前模块在所有模块中的序号，它同样关键。 Nginx 启动时会根据
       ngx_modules 数组设置各模块的 index 值  */
    ngx_uint_t            index;

    /* 模块名称 */
    char                 *name;

    /* spare 系列的保留变量，暂未使用 */
    ngx_uint_t            spare0;
    ngx_uint_t            spare1;

    /* 模块的版本，便于将来的扩展。目前只有一种，默认为1 */
    ngx_uint_t            version;
    const char           *signature;

    /* ctx 用于指定一类模块的上下文结构体。为什么需要 ctx 呢？因为前面说过， Nginx 模块
       有许多种类，不同模块之前的功能差别很大。例如，事件类型的砌块主要处理 I/O 事件相关的功能
       HTTP 类型的模块主要处理 HTTP 应用层的功能。这样，每个模块都有了自己的特性，而 ctx 将
       会指向特定类型模块的公共接口。例如， HTTP 模块中， ctx 需要指向 ngx_http_module_t
       结构体 */
    void                 *ctx;

    /* 处理 nginx.conf 中的配置项 */
    ngx_command_t        *commands;

    /* 表示该模块的类型，它与 ctx 指针是紧密相关的。在官方 Nginx 中，它的取值范围是以下5种：
     * NGX_HTTP_MODULE, NGX_CORE_MODULE, NGX_CONF_MODULE, NGX_EVENT_MODULE
     * NGX_MAIL_MODULE。还可以自定义新的模块类型 */
    ngx_uint_t            type;

    /** 在 Nginx 的启动、停止过程中，以下7个函数指针表示7个执行点会分别调用这7种方法。对于任何
       一个方法而言，如果不需要 Nginx 在某个时刻执行它，那么简单地把它设置为 NULL 空指针即可 */
    /*
     * 虽然从字面上理解应当在 master 进程启动时回调 init_master ，但到目前为止，
     * 构架代码从来不会调用它，因为，可将 init_master 设置为 NULL
    */
    ngx_int_t           (*init_master)(ngx_log_t *log);

    /** 在初始化所有模块时调用。 在 master/worker 模式下，这个阶段将在启动 worker 子进程前完成*/
    ngx_int_t           (*init_module)(ngx_cycle_t *cycle);

    /** 在正常服务前被调用。在 master/worker 模式下，多个 worker 子进程已经产生，在每个
     * worker 进程的初始化过程会调用所有模块的 init_process 函数 */
    ngx_int_t           (*init_process)(ngx_cycle_t *cycle);
    /** 由于 Nginx 暂不支持多线程模式，所以 init_thread ，exit_thread 在框架代码中
     * 没有被调用过，设为 NULL */
    ngx_int_t           (*init_thread)(ngx_cycle_t *cycle);
    void                (*exit_thread)(ngx_cycle_t *cycle);
    /** 在服务停止前调用。在 master/worker 模式下， worker 进程会在退出前调用它 */
    void                (*exit_process)(ngx_cycle_t *cycle);
    /** 在 master 进程退出前被调用 */
    void                (*exit_master)(ngx_cycle_t *cycle);

    /** 以下8个 spare_hook 变量也是保留字段，目前没有使用，但可用 Nginx 提供
     * 的 NGX_MODULE_V1_PADDING 宏来填充。即全0 */
    uintptr_t             spare_hook0;
    uintptr_t             spare_hook1;
    uintptr_t             spare_hook2;
    uintptr_t             spare_hook3;
    uintptr_t             spare_hook4;
    uintptr_t             spare_hook5;
    uintptr_t             spare_hook6;
    uintptr_t             spare_hook7;
};
```

> 挂载点

ngx_http_init_phases()  

## upstream模块

Nginx 的核心功能-反向代理基于 upstream 模块。属于 HTTP 框架的一部分  

### upstream 是如何嵌入到一个请求中的

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

### 核心对象

ngx_http_upstream_t
ngx_http_upstream_init(), 返回 NGX_DONE 告诉 HTTP 框架暂停执行请求的下一阶段。还需要执行 r->main->count++ ，告诉 HTTP 框架将当前请求的引用计数加1，不要销毁请求。  

### 8个回调方法

必须实现的3个回调：  

1. creat_request()
2. process_header()
3. finalize_request()
