
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_CONF_BUFFER  4096

static ngx_int_t ngx_conf_handler(ngx_conf_t *cf, ngx_int_t last);
static ngx_int_t ngx_conf_read_token(ngx_conf_t *cf);
static void ngx_conf_flush_files(ngx_cycle_t *cycle);

/*
��������ģ����Ψһһ��ֻ��1��ģ���ģ�����͡�����ģ������ͽ���NGX_CONF_MODULE�������е�ģ�����ngx_conf_module������Nginx��
�ײ��ģ�飬��ָ��������ģ����������Ϊ�������ṩ���ܡ���ˣ�������������ģ��Ļ�����
*/
static ngx_command_t  ngx_conf_commands[] = { //Ƕ�����������ļ�

    { ngx_string("include"),
      NGX_ANY_CONF|NGX_CONF_TAKE1,
      ngx_conf_include,
      0,
      0,
      NULL },

      ngx_null_command
};


ngx_module_t  ngx_conf_module = {
    NGX_MODULE_V1,
    NULL,                                  /* module context */
    ngx_conf_commands,                     /* module directives */
    NGX_CONF_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_conf_flush_files,                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


/* The eight fixed arguments */

static ngx_uint_t argument_number[] = {
    NGX_CONF_NOARGS,
    NGX_CONF_TAKE1,
    NGX_CONF_TAKE2,
    NGX_CONF_TAKE3,
    NGX_CONF_TAKE4,
    NGX_CONF_TAKE5,
    NGX_CONF_TAKE6,
    NGX_CONF_TAKE7
};

/* ���������в�����Ϣ���ڴ�ṹ�� */
char *
ngx_conf_param(ngx_conf_t *cf)
{
    char             *rv;
    ngx_str_t        *param;
    ngx_buf_t         b;
    ngx_conf_file_t   conf_file;

    param = &cf->cycle->conf_param;

    if (param->len == 0) {
        return NGX_CONF_OK;
    }

    ngx_memzero(&conf_file, sizeof(ngx_conf_file_t));

    ngx_memzero(&b, sizeof(ngx_buf_t));

    b.start = param->data;
    b.pos = param->data;
    b.last = param->data + param->len;
    b.end = b.last;
    b.temporary = 1;

    conf_file.file.fd = NGX_INVALID_FILE;
    conf_file.file.name.data = NULL;
    conf_file.line = 0;

    cf->conf_file = &conf_file;
    cf->conf_file->buffer = &b;

    rv = ngx_conf_parse(cf, NULL);

    cf->conf_file = NULL;

    return rv;
}

/*
    8) HTTP��ܿ�ʼѭ������nginx.conf�ļ���http{������}��������������
���̵���19���Ż᷵�ء�
    9)�����ļ��������ڼ�⵽1��������󣬻�������е�HTTPģ�飬
ngx_command_t�����е�name���Ƿ�������������ͬ��
    10)����ҵ���1��HTTPģ�飨��mytestģ�飩��������������Ȥ����test- myconfig
��������͵���ngx_command_t�ṹ�е�set����������
    11) set���������Ƿ���ɹ����������ʧ�ܣ���ôNginx���̻�ֹͣ��
    12)�����ļ��������������������������server{��������������ͻ����ngx_http_
core_moduleģ����������Ϊngx_http_core_moduleģ����ȷ��ʾϣ������server{}����
�������ע�⣬��ε��õ���18���Ż᷵�ء�
    13) ngx_http_core_module����ڽ���server{...}֮ǰ��Ҳ�����3��һ������ngx_
http_conf_ctx_t�ṹ�����������鱣������HTTPģ�鷵�ص�ָ���ַ��Ȼ���������ÿ
��HTTPģ���create_srv_conf��create_loc_conf���������ʵ�ֵĻ�����
    14)����һ����HTTPģ�鷵�ص�ָ���ַ���浽ngx_http_conf_ctx_t��Ӧ�������С�
    15)��ʼ���������ļ�������������server{������}����������ע�⣬��������ڵ�17
�����ء�
    16)�����ظ���9���Ĺ��̣�����nginx.conf�е�ǰserver{���������ڵ����������
    17)�����ļ�����������������������ֵ�ǰserver���Ѿ�������β����˵��server
���ڵ����������ϣ�����ngx_http_core_moduleģ�顣
    18) http coreģ��Ҳ������server�������ˣ������������ļ����������������������
���
    19)�����ļ����������������������ʱ���ִ�����http{����������β�������ظ�
HTTP��ܼ�������
*/

/*
����һ����ӵĵݹ麯����Ҳ����˵��Ȼ�����ڸú������ڿ�����ֱ�ӵĶ��䱾��ĵ��ã�������ִ�е�һЩ����������ngx_conf_handler�����ֻ�
����ngx_conf_parse����������γɵݹ飬��һ���ڴ���һЩ��������ָ��������������ָ��include��events��http�� server��location�ȵĴ���ʱ��
*/ //ngx���ý������ݽṹͼ��ο�:http://tech.uc.cn/?p=300  ����Ƚ�ȫ
char *
ngx_conf_parse(ngx_conf_t *cf, ngx_str_t *filename)  //�ο�:http://blog.chinaunix.net/uid-26335251-id-3483044.html
{
    char             *rv;
    u_char           *p;
    off_t             size;
    ngx_fd_t          fd;
    ngx_int_t         rc;
    ngx_buf_t         buf, *tbuf;
    ngx_conf_file_t  *prev, conf_file;
    ngx_conf_dump_t  *cd;

    /* ngx_conf_parse �����������ɶ������ļ��Ľ�������ʵ������������������ļ����������������������Ϳ� */
    /*
    ��ִ�е�ngx_conf_parse������ʱ�����õĽ������ܴ�������״̬��
    ��һ�֣��տ�ʼ����һ�������ļ�������ʱ�Ĳ���filenameָ��һ�������ļ�·���ַ�������Ҫ����ngx_conf_parse�򿪸��ļ�����ȡ���
    ���ļ���Ϣ�Ա���������ȡ�ļ����ݲ����н�����������������ܵ�nginx����ʱ��ʼ�������ļ�����ʱ����������������е�����include
    ָ��ʱҲ��������״̬����ngx_conf_parse��������Ϊincludeָ���ʾһ���µ������ļ�Ҫ��ʼ������״̬���Ϊtype = parse_file;��
    
    �ڶ��֣���ʼ����һ�����ÿ飬����ʱ�����ļ��Ѿ��򿪲���Ҳ�Ѿ����ļ����ֽ����˽������������������������events��http��ʱ��
    ��Щ����������Ĵ������ֻ�ݹ�ĵ���ngx_conf_parse��������ʱ���������ݻ������Ե�ǰ�������ļ�����������ٴδ�����״̬���Ϊtype = parse_block;��
    
    �����֣���ʼ������������ڶ��û�ͨ��������-g���������������Ϣ���н���ʱ��������״̬���磺
    nginx -g ��daemon on;��
    nginx�ڵ���ngx_conf_parse������������Ϣ��daemon on;�����н���ʱ��������״̬��״̬���Ϊtype = parse_param;��
    ǰ��˵����nginx�������ɱ����ɵģ������ֺ��˽���״̬֮�󣬽�������Ҫ��ȡ�������ݣ�������ngx_conf_read_token�������������ģ�
    */
    enum {
        parse_file = 0,
        parse_block,
        parse_param
    } type;

#if (NGX_SUPPRESS_WARN)
    fd = NGX_INVALID_FILE;
    prev = NULL;
#endif

    if (filename) {

        /* open configuration file */

        fd = ngx_open_file(filename->data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
        if (fd == NGX_INVALID_FILE) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                               ngx_open_file_n " \"%s\" failed",
                               filename->data);
            return NGX_CONF_ERROR;
        }

        
        /* ����cf->conf_file ������ */
        prev = cf->conf_file; //�����������ļ�֮ǰ��������prev�ݴ�

        cf->conf_file = &conf_file;

        if (ngx_fd_info(fd, &cf->conf_file->file.info) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_EMERG, cf->log, ngx_errno,
                          ngx_fd_info_n " \"%s\" failed", filename->data);
        }

        cf->conf_file->buffer = &buf;

    /*
    ����ngx_conf_read_token�������ļ���������ַ�ɨ�貢����Ϊ������token����Ȼ���ú���������Ƶ����ȥ��ȡ�����ļ�����ÿ�δ�
    �ļ��ڶ�ȡ�㹻�������������һ����СΪNGX_CONF_BUFFER�Ļ��������������һ�Σ��������ļ�ʣ�����ݱ����Ͳ����ˣ����������
    ���ں��� ngx_conf_parse�����벢�������õ�����cf->conf_file->buffer�ڣ����� ngx_conf_read_token����ʹ�øû�����
    */
        buf.start = ngx_alloc(NGX_CONF_BUFFER, cf->log);
        if (buf.start == NULL) {
            goto failed;
        }

        buf.pos = buf.start;
        buf.last = buf.start;
        buf.end = buf.last + NGX_CONF_BUFFER;
        buf.temporary = 1;

        cf->conf_file->file.fd = fd;
        cf->conf_file->file.name.len = filename->len;
        cf->conf_file->file.name.data = filename->data;
        cf->conf_file->file.offset = 0;
        cf->conf_file->file.log = cf->log;
        cf->conf_file->line = 1;

        type = parse_file;

        if (ngx_dump_config
#if (NGX_DEBUG)
            || 1
#endif
           )
        {
            p = ngx_pstrdup(cf->cycle->pool, filename);
            if (p == NULL) {
                goto failed;
            }

            size = ngx_file_size(&cf->conf_file->file.info);

            tbuf = ngx_create_temp_buf(cf->cycle->pool, (size_t) size);
            if (tbuf == NULL) {
                goto failed;
            }

            cd = ngx_array_push(&cf->cycle->config_dump);
            if (cd == NULL) {
                goto failed;
            }

            cd->name.len = filename->len;
            cd->name.data = p;
            cd->buffer = tbuf;

            cf->conf_file->dump = tbuf;

        } else {
            cf->conf_file->dump = NULL;
        }

    } else if (cf->conf_file->file.fd != NGX_INVALID_FILE) {

        type = parse_block;

    } else {
        type = parse_param; //���������в���
    }

    for ( ;; ) {

        /* ��ȡ�ļ��е����ݷŵ��������У������н������ѽ����Ľ���ŵ���cf->args ���棬 ָ���ÿ�����ʶ���������ռһ��λ�ã����� set debug off  ����ô�����д�����λ�á�*/
        rc = ngx_conf_read_token(cf); //��ȡ�ļ� 

        /*
         * ngx_conf_read_token() may return
         *
         *    NGX_ERROR             there is error
         *    NGX_OK                the token terminated by ";" was found
         *    NGX_CONF_BLOCK_START  the token terminated by "{" was found
         *    NGX_CONF_BLOCK_DONE   the "}" was found
         *    NGX_CONF_FILE_DONE    the configuration file is done
         */

        if (rc == NGX_ERROR) {
            goto done;
        }

        if (rc == NGX_CONF_BLOCK_DONE) {

            if (type != parse_block) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"}\"");
                goto failed;
            }

            goto done;
        }

        if (rc == NGX_CONF_FILE_DONE) {

            if (type == parse_block) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "unexpected end of file, expecting \"}\"");
                goto failed;
            }

            goto done;
        }

        if (rc == NGX_CONF_BLOCK_START) {

            if (type == parse_param) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "block directives are not supported "
                                   "in -g option");
                goto failed;
            }
        }

        /* rc == NGX_OK || rc == NGX_CONF_BLOCK_START */
        if (cf->handler) {
            /*
             * the custom handler, i.e., that is used in the http's
             * "types { ... }" directive
             */

            if (rc == NGX_CONF_BLOCK_START) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unexpected \"{\"");
                goto failed;
            }

            rv = (*cf->handler)(cf, NULL, cf->handler_conf);
            if (rv == NGX_CONF_OK) {
                continue;
            }

            if (rv == NGX_CONF_ERROR) {
                goto failed;
            }

            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, rv);

            goto failed;
        }

        /*
        ����������ں���ngx_conf_handle��ʵ�ֵģ�������������Ҫ��������ģ���е�����ָ�����ҵ�һ������ֱ�ӵ���ָ���set������
        ��ɶ�ģ���������Ϣ�����á� ������Ҫ�Ĺ��̾����ж��Ƿ����ҵ�����Ҫ�ж�����һЩ������ 
          a  ����һ�¡������ļ���ָ������ֺ�ģ��ָ���е�������Ҫһ��
          b  ģ������һ�¡������ļ�ָ����ģ�����ͺ͵�ǰģ��һ��
          c  ָ������һ�¡� �����ļ�ָ�����ͺ͵�ǰģ��ָ��һ��
          d  ��������һ�¡������ļ��в����ĸ����͵�ǰģ��ĵ�ǰָ�����һ�¡� 
        */
        rc = ngx_conf_handler(cf, rc); //�����ngx_conf_read_token���ص��ǽ�������һ�����ã��κ���ngx_modules�в���ƥ��������õ����ִ����Ӧ��set

        if (rc == NGX_ERROR) {
            goto failed;
        }
    }

failed:

    rc = NGX_ERROR;

done:

    if (filename) {
        if (cf->conf_file->buffer->start) {
            ngx_free(cf->conf_file->buffer->start);
        }

        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_close_file_n " %s failed",
                          filename->data);
            rc = NGX_ERROR;
        }
        
        /* �ָ������� */
        cf->conf_file = prev;
    }

    if (rc == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

/*
    ����������ں���ngx_conf_handle��ʵ�ֵģ�������������Ҫ��������ģ���е�����ָ�����ҵ�һ������ֱ�ӵ���ָ���set ������
    ��ɶ�ģ���������Ϣ�����á� ������Ҫ�Ĺ��̾����ж��Ƿ����ҵ�����Ҫ�ж�����һЩ������ 

  a  ����һ�¡������ļ���ָ������ֺ�ģ��ָ���е�������Ҫһ��
  b  ģ������һ�¡������ļ�ָ����ģ�����ͺ͵�ǰģ��һ��
  c  ָ������һ�¡� �����ļ�ָ�����ͺ͵�ǰģ��ָ��һ��
  d  ��������һ�¡������ļ��в����ĸ����͵�ǰģ��ĵ�ǰָ�����һ�¡� 
*/
static ngx_int_t
ngx_conf_handler(ngx_conf_t *cf, ngx_int_t last)
{
    char           *rv;
    void           *conf, **confp;
    ngx_uint_t      i, found;
    ngx_str_t      *name;
    ngx_command_t  *cmd;

    name = cf->args->elts;

    ngx_uint_t ia;
    char buf[2560];
    char tmp[256];
    memset(buf, 0, sizeof(buf));
   
   for(ia = 0; ia < cf->args->nelts; ia++) {
        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%s ", name[ia].data);
        strcat(buf, tmp);
    }
    //printf("yang test:%p %s <%s, %u>\n", cf->ctx, buf, __FUNCTION__, __LINE__); //�����ӡ����������Ϊ���е����ݣ���yang test:error_log logs/error.log debug   <ngx_conf_handler, 407>
   
   
    found = 0;

    for (i = 0; ngx_modules[i]; i++) { //����ģ�鶼ɨ��һ��
        cmd = ngx_modules[i]->commands;
        if (cmd == NULL) {
            continue;
        }
       
        for ( /* void */ ; cmd->name.len; cmd++) {

            if (name->len != cmd->name.len) {
                continue;
            }

            if (ngx_strcmp(name->data, cmd->name.data) != 0) {
                continue;
            }

            found = 1;

            if (ngx_modules[i]->type != NGX_CONF_MODULE
                && ngx_modules[i]->type != cf->module_type)
            {
                continue;
            }

            /* is the directive's location right ? */

            if (!(cmd->type & cf->cmd_type)) {
                continue;
            }

            if (!(cmd->type & NGX_CONF_BLOCK) && last != NGX_OK) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                  "directive \"%s\" is not terminated by \";\"",
                                  name->data);
                return NGX_ERROR;
            }

            if ((cmd->type & NGX_CONF_BLOCK) && last != NGX_CONF_BLOCK_START) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "directive \"%s\" has no opening \"{\"",
                                   name->data);
                return NGX_ERROR;
            }

            /* is the directive's argument count right ? */

            if (!(cmd->type & NGX_CONF_ANY)) {

                if (cmd->type & NGX_CONF_FLAG) {

                    if (cf->args->nelts != 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NGX_CONF_1MORE) {

                    if (cf->args->nelts < 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NGX_CONF_2MORE) {

                    if (cf->args->nelts < 3) {
                        goto invalid;
                    }

                } else if (cf->args->nelts > NGX_CONF_MAX_ARGS) {

                    goto invalid;

                } else if (!(cmd->type & argument_number[cf->args->nelts - 1]))
                {
                    goto invalid;
                }
            }

            /* set up the directive's configuration context */

            conf = NULL;

            /* ����ִ�е�http�У����ߵڶ���if��ȷ��httpһ��NGX_CORE_MODULE������ngx_cycle_s->conf_ctx�е�λ�ã�Ȼ���ڼ��������set��������
                �ú����п��ٿռ䣬����conf_ctx[]��������ľ����Աָ��ָ��ÿռ䣬�Ӷ�ʹhttp{}�ռ��ngx_cycle_s�������� */

            /*
            ��һ��if��ִ�е�������Ҫ��(NGX_DIRECT_CONF):ngx_core_commands  ngx_openssl_commands  ngx_google_perftools_commands   ngx_regex_commands  ngx_thread_pool_commands
            �ڶ���if��ִ�е�������Ҫ��(NGX_MAIN_CONF):http   events include��
            ������if��ִ�е�������Ҫ��(����):http{}   events{} server server{} location����location{}�ڲ�������
            */
            
            //confΪ���ٵ�main_conf  srv_conf loc_conf�ռ�ָ�룬�����Ŀռ�Ϊ����ģ��module��ctx��Ա��,
            //���Բο�ngx_http_mytest_config_module_ctx, http{}��Ӧ�Ŀռ俪����ngx_http_block
            //ע��:ͨ����ngx_init_cycle�д�ӡconf.ctx�Լ��������ӡcf->cxt���������е�һ��(http{}������ã�����http��һ��)�ĵ�ַ��һ���ģ�Ҳ���������conf.ctxʼ�յ���cycle->conf_ctx;
            //ÿ��һ���µ�{}��cf->cxt��ַ�ͻ�ָ����һ���ж�Ӧ��ngx_http_conf_ctx_t���˳�{}�к󣬻��cf->cxt�ָ����ϲ��ngx_http_conf_ctx_t��ַ
            //��ʱ��cf->ctxһ������ngx_cycle_s->conf_ctx����ngx_init_cycle
            if (cmd->type & NGX_DIRECT_CONF) {//ʹ��ȫ�����ã���Ҫ������������//ngx_core_commands  ngx_openssl_commands  ngx_google_perftools_commands   ngx_regex_commands  ngx_thread_pool_commands
                conf = ((void **) cf->ctx)[ngx_modules[i]->index]; //ngx_core_commands��Ӧ�Ŀռ����ĵط��ο�ngx_core_module->ngx_core_module_ctx
            } else if (cmd->type & NGX_MAIN_CONF) { //����ngx_http_commands ngx_errlog_commands  ngx_events_commands  ngx_conf_commands  ��Щ����commandһ��ֻ��һ������
                conf = &(((void **) cf->ctx)[ngx_modules[i]->index]); //ָ��ngx_cycle_s->conf_ctx
            } else if (cf->ctx) {
     /*http{}�ڲ������������Բο�:ngx_http_core_commands,ͨ����Щ���������NGX_HTTP_MAIN_CONF_OFFSET NGX_HTTP_SRV_CONF_OFFSET NGX_HTTP_LOC_CONF_OFFSET
     ����ȷ�����������еĵ�ַ��Ӧ��ngx_http_conf_ctx_t�еĵ�ַ�ռ�ͷ��ָ��λ��, ����ȷ����������Ϊngx_http_conf_ctx�ĳ�Աmain srv loc�е���һ��
     */      
                confp = *(void **) ((char *) cf->ctx + cmd->conf);   //�����http{}�ڲ����У���cf->ctx�Ѿ���ngx_http_block�б����¸�ֵΪ�µ�ngx_http_conf_ctx_t�ռ�
                //�����cf->ctxΪ�������������������Ŀռ��ˣ�������ngx_cycle_s->conf_ctx,����Ϊ�洢http{}�ڲ�������Ŀռ䣬��ngx_http_block����Ŀռ�

                if (confp) { //ͼ�λ��ο�:�������NGINX�е�ͼ9-2  ͼ10-1  ͼ4-2�����ͼ��,���������http://tech.uc.cn/?p=300��
                    conf = confp[ngx_modules[i]->ctx_index]; //��һ����ȷ����ngx_http_conf_ctx_t��main srv loc�е��Ǹ���Աͷ��������Ƕ�Ӧͷ�����������ָ���еľ�����һ��
                }
            }

            /* ���������http {����ִ��ngx_http_block���ڸú����л��httpģ���Ӧ��srv local main���ÿռ丳ֵ��conf,Ҳ����ngx_cycle_s->conf_ctx[]��Ӧ��ģ�� */
            rv = cmd->set(cf, cmd, conf); 
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
            if (rv == NGX_CONF_OK) {
                return NGX_OK;
            }

            if (rv == NGX_CONF_ERROR) {
                return NGX_ERROR;
            }

            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "\"%s\" directive %s", name->data, rv);

            return NGX_ERROR;
        }
    }

    if (found) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "\"%s\" directive is not allowed here", name->data);

        return NGX_ERROR;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "unknown directive \"%s\"", name->data);

    return NGX_ERROR;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid number of arguments in \"%s\" directive",
                       name->data);

    return NGX_ERROR;
}

/*
����ngx_conf_read_token�ڶ�ȡ�˺��������ı��token֮��Ϳ�ʼ��һ���輴����Щ��ǽ���ʵ�ʵĴ����Ƕ��ٲ����Ƕ�ȡ�˺��������ı���أ�����Դ������ڼ����������Ƕ�ȡ��ȫ���ı�ǣ�Ҳ��������������Ƿֺ�;Ϊֹ����ʱһ��������������б�Ƕ�����ȡ������� cf->args�����ڣ���˿��Ե������Ӧ�Ļص���������ʵ�ʵĴ������ڸ������������Ƕ��������ÿ�ǰ�����б�ǣ�������������{Ϊֹ����ʱ�����������������Ҫ�ı�Ƕ��Ѷ�ȡ�������������ÿ�{}�ڵı�ǽ��ڽ������ĺ���ngx_conf_parse�ݹ�����м��������������һ�������Ĺ��̡�
��Ȼ������ngx_conf_read_tokenҲ��������������·��أ����������ļ���ʽ�����ļ������꣨�����ļ��������������ô����꣨����������}�����⼸�ַ�������Ĵ����ܼ򵥣���������
���ڼ�/����������Ĵ���һ������£�����ͨ������ngx_conf_handler�����еģ���Ҳ������������Ҳ�����������ṩ���Զ���Ĵ�������
����typesָ�����ngx_conf_handlerҲ�����������飬���ȣ�����Ҫ�ҵ���ǰ��������������������Ӧ�� ngx_command_s�ṹ�壬
ǰ��˵����ngx_command_s������������������Ϣ�Լ���Ӧ�Ļص�ʵ�ʴ����������û�ҵ�����������Ӧ�� ngx_command_s�ṹ�壬
��ô˭����������������أ���Ȼ�ǲ��еģ����nginx��ֱ�ӽ��б����˳�������Σ��ҵ���ǰ��������������������Ӧ��ngx_command_s
�ṹ��֮�������һЩ��Ч����֤����Ϊngx_command_s�ṹ���ڰ�����������������Ϣ�������Ч����֤�ǿ��Խ��еģ���������������͡�
λ�á��������ĸ����ȵȡ�ֻ�о������ϸ���Ч����֤��������ŵ������Ӧ�Ļص�������
rv = cmd->set(cf, cmd, conf);
���д�����Ҳ���ǵ��������顣�ڴ������ڣ�����ʵ�ʵ���Ҫ�ֿ����ٴε��ú���ngx_conf_parse����˷���ֱ������������Ϣ���������ꡣ

*/

/*
 ������ȷ,ʲô��һ��token: 
 token�Ǵ����������ڿո�,���з�,˫����,�����ŵ�֮����ַ���. 
*/  

/****************************************** 
1.��ȡ�ļ�����,ÿ�ζ�ȡһ��buf��С(4K),����ļ����ݲ���4K��ȫ����ȡ��buf��. 
2.ɨ��buf�е�����,ÿ��ɨ��һ��token�ͻ����cf->args��,Ȼ�󷵻�. 
3.���غ����ngx_conf_parse���������*cf->handler��ngx_conf_handler(cf, rc)��������. 
3.����Ǹ���������,������ϴ�ִ�е�״̬�������������ļ�. 
.*****************************************/ 

static ngx_int_t
ngx_conf_read_token(ngx_conf_t *cf)//�ο�http://blog.chinaunix.net/uid-26335251-id-3483044.html
{
    u_char      *start, ch, *src, *dst;
    off_t        file_size;
    size_t       len;
    ssize_t      n, size;
    ngx_uint_t   found, need_space, last_space, sharp_comment, variable;
    ngx_uint_t   quoted, s_quoted, d_quoted, start_line;
    ngx_str_t   *word;
    ngx_buf_t   *b, *dump; //bΪ�ں�����㿪�ٵ�4096�ֽڵĴ洢file�ļ����ݵĿռ䣬dumpΪ��������β����ʱ�����Թ���һ��token��ʱ����ʱ���ⲿ���ڴ������

    found = 0;//��־λ,��ʾ�ҵ�һ��token 

    
    /************************* 
    ��־λ,��ʾ��ʱ��Ҫһ��token�ָ���,��tokenǰ��ķָ��� 
    һ��ոս�����һ��˫���Ż��ߵ�����,��ʱ����need_spaceΪ1, 
    ��ʾ�ڴ���һ���ַ�Ϊ�ָ��� 
    **********************/  
    need_space = 0;
    last_space = 1; //��־λ,��ʾ��һ���ַ�Ϊtoken�ָ���   
    sharp_comment = 0; //ע��(#)   
    variable = 0; //�����ַ�$��,��ʾһ������   
    quoted = 0; //��־λ,��ʾ��һ���ַ�Ϊ������   
    s_quoted = 0; //��־λ,��ʾ��ɨ��һ��˫����,�ڴ���һ��˫����   
    d_quoted = 0; //��־λ,��ʾ��ɨ��һ��������,�ڴ���һ��������  

    cf->args->nelts = 0;
    b = cf->conf_file->buffer;
    dump = cf->conf_file->dump;
    start = b->pos;
    start_line = cf->conf_file->line;

    file_size = ngx_file_size(&cf->conf_file->file.info);

    for ( ;; ) {

        if (b->pos >= b->last) { //��buf�ڵ�����ȫ��ɨ���   

            if (cf->conf_file->file.offset >= file_size) {

                if (cf->args->nelts > 0 || !last_space) {

                    if (cf->conf_file->file.fd == NGX_INVALID_FILE) {
                        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                           "unexpected end of parameter, "
                                           "expecting \";\"");
                        return NGX_ERROR;
                    }

                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                  "unexpected end of file, "
                                  "expecting \";\" or \"}\"");
                    return NGX_ERROR;
                }

                //�����ļ���ȡ���. 
                return NGX_CONF_FILE_DONE;
            }

            //��ɨ���buf�ĳ���   
            len = b->pos - start; //��ʾ��һ��4096��buffer��δ������ϵĿռ��С

            if (len == NGX_CONF_BUFFER) {//��ɨ��ȫ��buf   
                cf->conf_file->line = start_line;

                if (d_quoted) { //ȱ����˫����   
                    ch = '"';

                } else if (s_quoted) { //ȱ���ҵ�����   
                    ch = '\'';

                } else { //�ַ���̫��,һ��buf���޷��洢һ��token   
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "too long parameter \"%*s...\" started",
                                       10, start);
                    return NGX_ERROR;
                }

                
                //����̫��,����ȱ����˫���Ż����ҵ�����.   
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "too long parameter, probably "
                                   "missing terminating \"%c\" character", ch);
                return NGX_ERROR;
            }

            if (len) {//������Ч,��ֵ��ɨ��buf��buf�ײ�  
                ngx_memmove(b->start, start, len);
            }

            //size���������ļ�δ����ĳ���    
            size = (ssize_t) (file_size - cf->conf_file->file.offset);

            //size���ܴ��ڿ���buf���ȣ���Ϊ��һ��4096�ռ�����в��ֿռ���Ҫ����һ��4096�ռ�����
            if (size > b->end - (b->start + len)) {
                size = b->end - (b->start + len);
            }

            n = ngx_read_file(&cf->conf_file->file, b->start + len, size,
                              cf->conf_file->file.offset);

            if (n == NGX_ERROR) {
                return NGX_ERROR;
            }

            if (n != size) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   ngx_read_file_n " returned "
                                   "only %z bytes instead of %z",
                                   n, size);
                return NGX_ERROR;
            }

            //����pos,last,start   //�ο�http://blog.chinaunix.net/uid-26335251-id-3483044.html
            b->pos = b->start + len;//ָ���¶�ȡ�ռ��ͷ�������lenΪ��һ��4096��ĩβ�ռ�Ϊ����һ��stoken���ֵĿռ��С
            b->last = b->pos + n;//ָ���¶�ȡ�ռ��β
            start = b->start;

            if (dump) {
                dump->last = ngx_cpymem(dump->last, b->pos, size);
            }
        }

        ch = *b->pos++;

        if (ch == LF) {//�������з� 
            cf->conf_file->line++;

            /* �������Ϊע��,��ע���н���,��ȡ��ע�ͱ�ʶ 
                */
            if (sharp_comment) {
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {//�������Ϊע����,���ٶ��ַ��ж�,������ȡ�ַ�ִ��   
            continue;
        }

        /*
        ���Ϊ������,�����÷����ű�ʶ,���Ҳ��Ը��ַ����н���   
        ����ɨ����һ���ַ�   
        */
        if (quoted) {
            quoted = 0;
            continue;
        }

        if (need_space) { //��һ���ַ�Ϊ�����Ż���˫����,�ڴ�һ���ָ���   
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;
                need_space = 0;
                continue;
            }

            if (ch == ';') {  //�����ֺ�,��ʾһ�����������������.  
                return NGX_OK;
            }

            if (ch == '{') { //����{��ʾһ�����������ʼ   
                return NGX_CONF_BLOCK_START;
            }

            if (ch == ')') {
                last_space = 1;
                need_space = 0;

            } else {
                 ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                    "unexpected \"%c\"", ch);
                 return NGX_ERROR;
            }
        }

        if (last_space) {//�����һ���ַ��ǿո�,���з��ȷָ�token���ַ�(�����).  
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {//�����ָ�token���ַ�����,���ɱ�ʾһ�������.   
                continue;
            }

            start = b->pos - 1;
            start_line = cf->conf_file->line;

            switch (ch) {

            case ';':
            case '{':
                if (cf->args->nelts == 0) {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "unexpected \"%c\"", ch);
                    return NGX_ERROR;
                }

                if (ch == '{') {
                    return NGX_CONF_BLOCK_START;
                }

                return NGX_OK;

            case '}':
                if (cf->args->nelts != 0) {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "unexpected \"}\"");
                    return NGX_ERROR;
                }

                return NGX_CONF_BLOCK_DONE;

            case '#':
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"':
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0;
            }

        } else {
            if (ch == '{' && variable) {
                continue;
            }

            variable = 0;

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (ch == '$') { //������־λΪ1   
                variable = 1;
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {  //�Ѿ��ҵ��ɶ�˫����,����һ�������   
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {//�Ѿ��ҵ��ɶԵ�����,����һ�������   
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                       || ch == ';' || ch == '{')
            {
                last_space = 1;
                found = 1;
            }

            if (found) {
                word = ngx_array_push(cf->args);
                if (word == NULL) {
                    return NGX_ERROR;
                }

                word->data = ngx_pnalloc(cf->pool, b->pos - 1 - start + 1);
                if (word->data == NULL) {
                    return NGX_ERROR;
                }

                for (dst = word->data, src = start, len = 0;
                     src < b->pos - 1;
                     len++)
                {
                    if (*src == '\\') { //������б��(ת���ַ�)   
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';
                word->len = len;

                if (ch == ';') {//�����ֺ�,��ʾһ�����������������.   
                    return NGX_OK;
                }

                if (ch == '{') {
                    return NGX_CONF_BLOCK_START;
                }

                found = 0;
            }
        }
    }
}


char *
ngx_conf_include(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char        *rv;
    ngx_int_t    n;
    ngx_str_t   *value, file, name;
    ngx_glob_t   gl;

    value = cf->args->elts;
    file = value[1];

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

    if (ngx_conf_full_name(cf->cycle, &file, 1) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (strpbrk((char *) file.data, "*?[") == NULL) {

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        return ngx_conf_parse(cf, &file);
    }

    ngx_memzero(&gl, sizeof(ngx_glob_t));

    gl.pattern = file.data;
    gl.log = cf->log;
    gl.test = 1;

    if (ngx_open_glob(&gl) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, ngx_errno,
                           ngx_open_glob_n " \"%s\" failed", file.data);
        return NGX_CONF_ERROR;
    }

    rv = NGX_CONF_OK;

    for ( ;; ) {
        n = ngx_read_glob(&gl, &name);

        if (n != NGX_OK) {
            break;
        }

        file.len = name.len++;
        file.data = ngx_pstrdup(cf->pool, &name);
        if (file.data == NULL) {
            return NGX_CONF_ERROR;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        rv = ngx_conf_parse(cf, &file);

        if (rv != NGX_CONF_OK) {
            break;
        }
    }

    ngx_close_glob(&gl);

    return rv;
}

//��ȡ�����ļ�ȫ�棬����·������ŵ�cycle->conf_prefix����cycle->prefix��
ngx_int_t
ngx_conf_full_name(ngx_cycle_t *cycle, ngx_str_t *name, ngx_uint_t conf_prefix)
{
    ngx_str_t  *prefix;

    prefix = conf_prefix ? &cycle->conf_prefix : &cycle->prefix;

    return ngx_get_full_name(cycle->pool, prefix, name);
}

//����cycle->open_files�����е������ļ��������Ƿ���name�ļ������ڣ��������ֱ�ӷ��ظ��ļ�����Ӧ�����������������д��»�ȡһ��file,��name�ļ����뵽����
ngx_open_file_t *
ngx_conf_open_file(ngx_cycle_t *cycle, ngx_str_t *name) //access_log error_log���ö������ﴴ��ngx_open_file_t����ṹ��������cycle->open_filesͳһ����
{
    ngx_str_t         full;
    ngx_uint_t        i;
    ngx_list_part_t  *part;
    ngx_open_file_t  *file;

#if (NGX_SUPPRESS_WARN)
    ngx_str_null(&full);
#endif

    if (name->len) {
        full = *name;

        if (ngx_conf_full_name(cycle, &full, 0) != NGX_OK) {
            return NULL;
        }

        part = &cycle->open_files.part;
        file = part->elts;

        for (i = 0; /* void */ ; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }
                part = part->next;
                file = part->elts;
                i = 0;
            }

            if (full.len != file[i].name.len) {
                continue;
            }

            if (ngx_strcmp(full.data, file[i].name.data) == 0) {
                return &file[i];
            }
        }
    }

    file = ngx_list_push(&cycle->open_files);
    if (file == NULL) {
        return NULL;
    }

    if (name->len) {
        file->fd = NGX_INVALID_FILE;
        file->name = full;

    } else {
        file->fd = ngx_stderr;
        file->name = *name;
    }

    file->flush = NULL;
    file->data = NULL;

    return file;
}


static void
ngx_conf_flush_files(ngx_cycle_t *cycle)
{
    ngx_uint_t        i;
    ngx_list_part_t  *part;
    ngx_open_file_t  *file;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, cycle->log, 0, "flush files");

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].flush) {
            file[i].flush(&file[i], cycle->log);
        }
    }
}


void ngx_cdecl
ngx_conf_log_error(ngx_uint_t level, ngx_conf_t *cf, ngx_err_t err,
    const char *fmt, ...)
{
    u_char   errstr[NGX_MAX_CONF_ERRSTR], *p, *last;
    va_list  args;

    last = errstr + NGX_MAX_CONF_ERRSTR;

    va_start(args, fmt);
    p = ngx_vslprintf(errstr, last, fmt, args);
    va_end(args);

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (cf->conf_file == NULL) {
        ngx_log_error(level, cf->log, 0, "%*s", p - errstr, errstr);
        return;
    }

    if (cf->conf_file->file.fd == NGX_INVALID_FILE) {
        ngx_log_error(level, cf->log, 0, "%*s in command line",
                      p - errstr, errstr);
        return;
    }

    ngx_log_error(level, cf->log, 0, "%*s in %s:%ui",
                  p - errstr, errstr,
                  cf->conf_file->file.name.data, cf->conf_file->line);
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ���������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д�����룬��4-2�г�����ЩԤ��Ľ������������

��4-2Ԥ���14���������������
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access_slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������*/
char *
ngx_conf_set_flag_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t        *value;//value[0]��Ӧ������ value[1]��Ӧ���ú���Ĳ���ֵ
    ngx_flag_t       *fp;
    ngx_conf_post_t  *post;

    fp = (ngx_flag_t *) (p + cmd->offset);

    if (*fp != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    if (ngx_strcasecmp(value[1].data, (u_char *) "on") == 0) {
        *fp = 1;

    } else if (ngx_strcasecmp(value[1].data, (u_char *) "off") == 0) {
        *fp = 0;

    } else {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                     "invalid value \"%s\" in \"%s\" directive, "
                     "it must be \"on\" or \"off\"",
                     value[1].data, cmd->name.data);
        return NGX_CONF_ERROR;
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, fp);
    }

    return NGX_CONF_OK;
}


char *
ngx_conf_set_str_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t        *field, *value;
    ngx_conf_post_t  *post;

    field = (ngx_str_t *) (p + cmd->offset);

    if (field->data) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *field = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, field);
    }

    return NGX_CONF_OK;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ���������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д�����룬��4-2�г�����ЩԤ��Ľ������������

��4-2Ԥ���14���������������
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������*/

/*
test_str_array������Ҳֻ�ܳ�����location{������}���ڡ�������������ã�
    location ... {
        test_str_array      Content-Length ;
        test_str_array      Content-Encoding ;
    }
    ��ô��my_str_array->nelts��ֵ����2����ʾ����������test_str_array���������my_str_array->eltsָ��
ngx_str_t������ɵ����飬�����Ϳ��԰����·�ʽ����������ֵ��
ngx_str_t*  pstr  =  mycf->my_str_array->elts ;
���ǣ�pstr[0]��pstr[l]����ȡ������ֵ���ֱ���{len=14;data=��Content-Length����}��
{len=16;data=��Content-Encoding����)����������Կ�����������HTTPͷ��������������
ʱ�Ǻ��ʺ�ʹ��ngx_conf_set_str_array_slotԤ�跽���ġ�
*/
char *
ngx_conf_set_str_array_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t         *value, *s;
    ngx_array_t      **a;
    ngx_conf_post_t   *post;

    a = (ngx_array_t **) (p + cmd->offset);

    if (*a == NGX_CONF_UNSET_PTR) {
        *a = ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    s = ngx_array_push(*a);
    if (s == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;
/*
    location /ttt {			
        test_str_array      Content-Length ;			
        test_str_array      Content-Encoding ;		
    }

    printf("yang test ...%u, [%s..%u]..[%s...%u].. <%s, %u>\n", cf->args->nelts, 
        value[0].data, value[0].len , value[1].data, value[1].len, __FUNCTION__, __LINE__);
    ��ӡ����:
    yang test ...2, [test_str_array..14]..[Content-Length...14].. <ngx_conf_set_str_array_slot, 1274>
    yang test ...2, [test_str_array..14]..[Content-Encoding...16].. <ngx_conf_set_str_array_slot, 1274>
    
    ��������������������ngx_conf_set_str_array_slot������
    ��һ��:value[0]��Ӧ��name��Ӧtest_str_array��value[1]��name��ӦContent-Length
    ��һ��:value[1]��Ӧ��name��Ӧtest_str_array��value[1]��name��ӦContent-Encoding
*/
    *s = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, s);
    }

    return NGX_CONF_OK;
}

/*
ע��  ��ngx_http_mytest_create_loc_conf�����ṹ��ʱ�������ʹ��ngx_conf_set_
keyval_slot�������my_keyval��ʼ��ΪNULL��ָ�룬
����ngx_conf_set_keyval_slot�ڽ���ʱ�ᱨ��
*/
char *
ngx_conf_set_keyval_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t         *value;
    ngx_array_t      **a;
    ngx_keyval_t      *kv;
    ngx_conf_post_t   *post;

    a = (ngx_array_t **) (p + cmd->offset);

    if (*a == NULL) {
        *a = ngx_array_create(cf->pool, 4, sizeof(ngx_keyval_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    kv = ngx_array_push(*a);
    if (kv == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    kv->key = value[1];
    kv->value = value[2];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, kv);
    }

    return NGX_CONF_OK;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ����set�ص��������ڴ���mytest������ʱ�Ѿ�ʹ�ù�������mytest��������
���������ġ����������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д������
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������
*/

/*
ע����ngx_http_mytest_create loc conf�����ṹ��ʱ�������ʹ��ngx_conf_set_num_slot,
�����my_num��ʼ��ΪNGX_CONF_UNSET��  ����ngx_conf_set_num_slot�ڽ���ʱ�ᱨ��
*/
char *
ngx_conf_set_num_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_int_t        *np;
    ngx_str_t        *value;
    ngx_conf_post_t  *post;


    np = (ngx_int_t *) (p + cmd->offset);

    if (*np != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    *np = ngx_atoi(value[1].data, value[1].len);
    if (*np == NGX_ERROR) {
        return "invalid number";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, np);
    }

    return NGX_CONF_OK;
}

/*
���ϣ����������ĺ����ǿռ��С����ô��ngx_conf_set_size_slot��������������
�ǳ����ʵģ���Ϊngx_conf_set_size_slot����������Ĳ������е�λ�����磬k����K��ʾ
Kilobyte��m����M��ʾMegabyte����ngx_http_mytest_conf_t�ṹ�е�size_t my_size;��
�洢�������������my_size��ʾ�ĵ�λ���ֽڡ�

�����nginx.conf��������test_size lOk;����ômy_size��������Ϊ10240���������
Ϊtest_size lOm;����my_size������Ϊ10485760��
    ngx_conf_set size slotֻ�����������Ĳ���Я����λk����K��m����M����������
g����G�ĳ��֣�����ngx_conf_set_off_slot�ǲ�ͬ�ġ�
    ע��  ��ngx_http_mytest_create_loc_conf�����ṹ��ʱ�������ʹ��ngx_conf_set_size_slot��
    �����my_size��ʼ��ΪNGX_CONF_UNSET_SIZE
    mycf->my_size= NGX_CONF_UNSET_SIZE;������ngx_conf_set_size_slot�ڽ���ʱ�ᱨ��
*/
char *
ngx_conf_set_size_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    size_t           *sp;
    ngx_str_t        *value;
    ngx_conf_post_t  *post;


    sp = (size_t *) (p + cmd->offset);
    if (*sp != NGX_CONF_UNSET_SIZE) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = ngx_parse_size(&value[1]);
    if (*sp == (size_t) NGX_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NGX_CONF_OK;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ����set�ص��������ڴ���mytest������ʱ�Ѿ�ʹ�ù�������mytest��������
���������ġ����������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д�����룬
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������*/

/*
���ϣ����������ĺ����ǿռ��ƫ��λ�ã���ô����ʹ��ngx_conf_set_off_slotԤ
�跽������ʵ�ϣ�ngx_conf_set_off_slot��ngx_conf_set_size_slot�Ƿǳ����Ƶģ�������
����ngx_conf_set_off_slot֧�ֵĲ�����λ��Ҫ��1��g����G����ʾGigabyte����ngx_http_mytest_conf_t
�ṹ�е�off_t my_off;���洢�������������my_off��ʾ��ƫ������λ���ֽڡ�

�����nginx.conf��������test_off lg����ômy_off��������Ϊ1073741824��������
��λΪk��K��m��Mʱ����������ngx_conf_set_size_slot��ͬ��

ע��  ��ngx_http_mytest_create loc conf�����ṹ��ʱ�������ʹ��ngx_conf_set_
off_slot�������my_off��ʼ��ΪNGX_CONF_UNSET�꣬����ngx_conf_set off slot�ڽ���ʱ�ᱨ��

*/
char *
ngx_conf_set_off_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    off_t            *op;
    ngx_str_t        *value;
    ngx_conf_post_t  *post;


    op = (off_t *) (p + cmd->offset);
    if (*op != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *op = ngx_parse_offset(&value[1]);
    if (*op == (off_t) NGX_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, op);
    }

    return NGX_CONF_OK;
}

/*
ngx_conf_set sec slot��ngx_conf_set msec slot�ǳ����ƣ�  ֻ��ngx_conf_set sec slot
����ngx_http_mytest_conf_t�ṹ���е�time_t my_sec;���洢����ʱ���������my_sec��
ʾ��ʱ�䵥λ���룬��ngx_conf_set_msec_slotΪ���롣

ע��  ��ngx_http_mytest_create_loc_conf�����ṹ��ʱ�������ʹ��ngx_c onf_set_
sec slot����ô�����my_sec��ʼ��ΪNGX CONF UNSET��    ����ngx_conf_set_sec_slot�ڽ���ʱ�ᱨ��
*/
char *
ngx_conf_set_msec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_msec_t       *msp;
    ngx_str_t        *value;
    ngx_conf_post_t  *post;


    msp = (ngx_msec_t *) (p + cmd->offset);
    if (*msp != NGX_CONF_UNSET_MSEC) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *msp = ngx_parse_time(&value[1], 0);
    if (*msp == (ngx_msec_t) NGX_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, msp);
    }

    return NGX_CONF_OK;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ����set�ص��������ڴ���mytest������ʱ�Ѿ�ʹ�ù�������mytest��������
���������ġ����������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д������
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������

*/

char *
ngx_conf_set_sec_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    time_t           *sp;
    ngx_str_t        *value;
    ngx_conf_post_t  *post;


    sp = (time_t *) (p + cmd->offset);
    if (*sp != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = ngx_parse_time(&value[1], 1);
    if (*sp == (time_t) NGX_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NGX_CONF_OK;
}

//����fastcgi_buffers  5 3K     output_buffers  5   3K
char *
ngx_conf_set_bufs_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char *p = conf;

    ngx_str_t   *value;
    ngx_bufs_t  *bufs;


    bufs = (ngx_bufs_t *) (p + cmd->offset);
    if (bufs->num) {
        return "is duplicate";
    }

    value = cf->args->elts;

    bufs->num = ngx_atoi(value[1].data, value[1].len);
    if (bufs->num == NGX_ERROR || bufs->num == 0) {
        return "invalid value";
    }

    bufs->size = ngx_parse_size(&value[2]);
    if (bufs->size == (size_t) NGX_ERROR || bufs->size == 0) {
        return "invalid value";
    }

    return NGX_CONF_OK;
}

/*
ngx_conf_set enum_ slot��ʾö�������Ҳ����˵��Nginxģ������н���ָ��������
�Ĳ���ֵֻ�����Ѿ�����õ�ngx_conf_enumt������name�ַ����е�һ�����ȿ���ngx_conf enum_t�Ķ���������ʾ��
typedef struct {
      ngx_s t r_t    name ;
     ngx_uint_t value;
} ngx_conf_enum_t;
    ���У�name��ʾ�������Ĳ���ֻ����nameָ����ַ�����ȣ���value��ʾ�����
���г�����name��ngx_conf_set enum slot��������Ѷ�Ӧ��value���õ��洢�ı����С�
���磺
static ngx_conf_enum_t   test_enums []  =
    { ngx_string("apple") ,  1 },
     { ngx_string("banana"), 2 },
    { ngx_string("orange") ,  3 },
    { ngx_null_string, O }
    ����������ӱ�ʾ���������еĲ���������apple��banana��orange����֮һ��ע�⣬��
����ngx_null_string��β����Ҫ��ngx_uint_t���洢������Ĳ�����������ngx_
command tʱ����Ҫ�����������ж����test enums���鴫��postָ�룬������ʾ��
static ngx_command_t ngx_http_mytest_commands []  =  {
    {   ngx_string ( " test_enum" ) ,
              NGX_HTTP_LOC_CONF I NGX_CONF_TAKEI,
            ngx_conf set enum_slot, NGX_HTTP LOC_CONF_OFFSET,
                offsetof (ngx_http_mytest conf_t,  my_enum_seq) ,
                 test enums },
		ngx_null_command
    }

    �����������nginx.conf�г�����������test enum banana;��my_enum_seq��ֵ��2��
���������test enum�����˳�apple��banana��orange֮���ֵ��Nginx���ᱨ��invalid value������
*/
char *
ngx_conf_set_enum_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_uint_t       *np, i;
    ngx_str_t        *value;
    ngx_conf_enum_t  *e;

    np = (ngx_uint_t *) (p + cmd->offset);

    if (*np != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;
    e = cmd->post;//postΪngx_command_t�����һ����������Ҫ�����ṩ�ռ�洢  

    for (i = 0; e[i].name.len != 0; i++) { //��Ҳ����Ϊʲô���ύ����post�е�ngx_conf_enum_t��Ϊʲô���һ����Աһ��ҪΪ{ ngx_null_string, 0 }
        if (e[i].name.len != value[1].len
            || ngx_strcasecmp(e[i].name.data, value[1].data) != 0)
        {
            continue;
        }

        *np = e[i].value;

        return NGX_CONF_OK;
    }

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "invalid value \"%s\"", value[1].data);

    return NGX_CONF_ERROR;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_commandj 'vcmd,void *conf)
    ����set�ص��������ڴ���mytest������ʱ�Ѿ�ʹ�ù�������mytest��������
���������ġ����������������Ǽȿ����Լ�ʵ��һ���ص��������������������Ҳ����ʹ��NginxԤ���14��������������������
д������
�������������������������������ש�������������������������������������������������������������������������������
��    Ԥ�跽����              ��    ��Ϊ                                                                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���nginx��conf�ļ���ĳ��������Ĳ�����on����off����ϣ�����������      ��
��                            �����߹ر�ĳ�����ܵ���˼����������Nginxģ��Ĵ�����ʹ��ngx_flag_t��������       ��
��ngx_conf_set_flag_slot      �������������Ĳ������Ϳ��Խ�set�ص�������Ϊngx_conf_set_flag_slot����nginx.   ��
��                            ��conf�ļ��в�����onʱ�������е�ngx_flag_t���ͱ�������Ϊ1������Ϊoffʱ��        ��
��                            ����ΪO                                                                         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������ֻ��1��������ͬʱ�ڴ���������ϣ����ngx_str_t���͵ı�������      ��
��ngx_conf_set_str_slot       ��                                                                              ��
��                            �������������Ĳ����������ʹ��ngx_conf_set_ str slot����                      ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ���������������ֶ�Σ�ÿ����������涼����1�����������ڳ���������       ��
��                            ��ϣ������һ��ngx_array_t��̬���飨           �����洢���еĲ�������������      ��
��ngx_conf_set_str_array_slot ��                                                                              ��
��                            ����ÿ����������ngx_str_t���洢����ôԤ���ngx_conf_set_str_array_slot�з���    ��
��                            ���԰���������                                                                  ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_str_array_slot���ƣ�Ҳ����һ��ngx_array_t�������洢����ͬ    ��
��                            ����������Ĳ�����ֻ��ÿ��������Ĳ�������ֻ��1���������������������ԡ���       ��
��ngx_conf_set_keyval_slot    ��                                                                              ��
��                            ���������ؼ���ֵ��������ʽ������nginx��conf�ļ��У�ͬʱ��ngx_conf_set_keyval    ��
��                            �� slot��������������ת��Ϊ���飬����ÿ��Ԫ�ض��洢��key/value��ֵ��            ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��ngx_conf_set_num_slot       ��  ����������Я��1����������ֻ�������֡��洢��������ı�������������         ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��С��������һ�����֣���ʱ��ʾ�ֽ���       ��
��                            ��(Byte)��������ֺ����k����K���ͱ�ʾKilobyt��IKB=1024B��������ֺ��          ��
��ngx_conf_set_size_slot      ��                                                                              ��
��                            ����m����M���ͱ�ʾMegabyte��1MB=1024KB��ngx_conf_set_ size slot������         ��
��                            �����������Ĳ���ת�������ֽ���Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾ�ռ��ϵ�ƫ�������������õĲ����ǳ����ƣ�       ��
��                            ���������һ������ʱ��ʾByte��Ҳ�����ں���ӵ�λ������ngx_conf_set_size slot    ��
��ngx_conf_set_off_slot       ����ͬ���ǣ����ֺ���ĵ�λ����������k����K��m����M����������g����G��            ��
��                            ����ʱ��ʾGigabyte��IGB=1024MB��ngx_conf_set_off_slot�����󽫰���������       ��
��                            ������ת�������ֽ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ʾʱ�䡣����������������ֺ���ӵ�λ����         ��
��                            ������λΪs����û���κε�λ����ô������ֱ�ʾ�룻�����λΪm�����ʾ��          ��
��                            ���ӣ�Im=60s�������λΪh�����ʾСʱ��th=60m�������λΪd�����ʾ�죬          ��
��ngx_conf_set_msec_slot      ��                                                                              ��
��                            ��ld=24h�������λΪw�����ʾ�ܣ�lw=7d�������λΪM�����ʾ�£�1M=30d��         ��
��                            �������λΪy�����ʾ�꣬ly=365d��ngx_conf_set_msec_slot�����󽫰��������     ��
��                            ���Ĳ���ת�����Ժ���Ϊ��λ������                                                ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ��ngx_conf_set_msec slot�ǳ����ƣ�Ψһ��������ngx_conf_set msec��slot����   ��
��ngx_conf_set_sec_slot       ���󽫰��������Ĳ���ת�����Ժ���Ϊ��λ�����֣���ngx_conf_set_secһslot����    ��
��                            �������������Ĳ���ת��������Ϊ��λ������                                    ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��һ������������1�����������֣���2��������ʾ�ռ��С��        ��
��                            �����磬��gzip_buffers 4 8k;����ͨ��������ʾ�ж��ٸ�ngx_buf_t�������������е�1  ��
��                            ��������������Я���κε�λ����2�����������κε�λʱ��ʾByte�������k��          ��
��ngx_conf_set_bufs_slot      ����K��Ϊ��λ�����ʾKilobyte�������m����M��Ϊ��λ�����ʾMegabyte��           ��
��                            ��ngx_conf_set- bufs��slot������������������������ת����ngx_bufs_t�ṹ����  ��
��                            ����������Ա������������Ӧ��Nginx��ϲ���õĶ໺�����Ľ���������������       ��
��                            ���ӶԶ˷�����TCP����                                                           ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  ����������Я��1����������ȡֵ��Χ�����������趨�õ��ַ���֮һ������       ��
��                            ��C�����е�ö��һ���������ȣ�����Ҫ��ngx_conf_enum_t�ṹ�����������ȡֵ��      ��
��ngx_conf_set_enum_slot      ��                                                                              ��
��                            ��Χ�����趨ÿ��ֵ��Ӧ�����кš�Ȼ��ngx_conf_set enum slot������������      ��
��                            ����ת��Ϊ��Ӧ�����к�                                                          ��
�ǩ����������������������������贈������������������������������������������������������������������������������
��                            ��  дngx_conf_set bitmask slot���ƣ�����������Я��1����������ȡֵ��Χ��      ��
��                            �������趨�õ��ַ���֮һ�����ȣ�����Ҫ��ngx_conf_bitmask_t�ṹ�����������      ��
��ngx_conf_set_bitmask_slot   ��                                                                              ��
��                            ��ȡֵ��Χ�����趨ÿ��ֵ��Ӧ�ı���λ��ע�⣬ÿ��ֵ����Ӧ�ı���λ��Ҫ��ͬ��      ��
��                            ��Ȼ��ngx_conf_set_bitmask_ slot��������������ת��Ϊ��Ӧ�ı���λ              ��
�������������������������������ߩ�������������������������������������������������������������������������������
�����������������������������ש�����������������������������������������������������������������������������
��    Ԥ�跽����            ��    ��Ϊ                                                                    ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������Ŀ¼�����ļ��Ķ�дȨ�ޡ�����������Я��1��3��������      ��
��                          ��������������ʽ��user:rw group:rw all:rw��ע�⣬����������Linux���ļ�����Ŀ  ��
��ngx_conf_set_access slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������

*/

/*
ngx_conf_set_bitmask_slot��ngx_conf_set_enum_slotҲ�Ƿǳ����Ƶģ�������Ĳ���
��������ö�ٳ�Ա��Ψһ�Ĳ������Ч�ʷ��棬ngx_conf_set_enum_slot��ö�ٳ�Ա�Ķ�Ӧ
ֵ�����ͣ���ʾ���кţ�����ȡֵ��Χ�����͵ķ�Χ����ngx_conf_set_bitmask_slot��ö��
��Ա�Ķ�Ӧֵ��ȻҲ�����ͣ������԰�λ�Ƚϣ�����Ч��Ҫ�ߵöࡣҲ����˵��������4��
�ڣ�32λ���Ļ��������ö�������������ֻ����32�
    ����ngx_conf_set- bitmask- slot��ngx_conf_set enum- slot������Ԥ���������������
�ϵĲ��������ʾ�����������ö��ȡֵ�ṹ��Ҳ��ngx_conf_enum t�����ngx_conf_
bitmask t�������ǲ�û������
typedef struct {
     ngx_str t name;
     ngx_uint_t mask;
  ngx_conf_bitmask_t ;
�����Զ���test_ bitmasks����Ϊ��������˵����
static ngx_conf_bitmask_t   test_bitmasks Ll  = {
	{ngx_string ("good") ,  Ox0002  } ,
	{ngx_string ( "better") ,  Ox0004  } ,
	{ngx_string ( "best") ,  Ox0008  } ,
	{ngx_null_string, O}
}
    ������������ƶ���Ϊtest_bitmask����nginx.conf�ļ���test bitmask�������Ĳ���
ֻ����good��better��best��3��ֵ֮һ��������ngx_http_myte st_conf_t�е����³�Ա��
ngx_uint_t my_bitmask;
���洢test bitmask�Ĳ�����������ʾ��
static ngx_command_t   ngx_http_mytest_commands []  =  {
    {   ngx_string ( " test_bitmask " )  ,
                NGX_HTTP LOC CONF I NGX_CONF_TAKEI,
            ngx_conf set bitmask _slot,
                 NGX HTTP LOC CONF_OFFSET,
                offsetof (ngx_http_mytest conf_t, my_bitmask) ,
                test bitmasks 
		},
		ngx_null_command
    }

�����nginx.conf�г���������test- bitmask best;����ômy_bitmask��ֵ��Ox8��
*/
char *
ngx_conf_set_bitmask_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_uint_t          *np, i, m;
    ngx_str_t           *value;
    ngx_conf_bitmask_t  *mask;


    np = (ngx_uint_t *) (p + cmd->offset);
    value = cf->args->elts;
    mask = cmd->post; //�ο�test_bitmasks  �Ѹ������е��ַ���ת��Ϊ��Ӧ��λͼ

    for (i = 1; i < cf->args->nelts; i++) {
        for (m = 0; mask[m].name.len != 0; m++) {

            if (mask[m].name.len != value[i].len
                || ngx_strcasecmp(mask[m].name.data, value[i].data) != 0)
            {
                continue;
            }

            if (*np & mask[m].mask) {
                ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                                   "duplicate value \"%s\"", value[i].data);

            } else {
                *np |= mask[m].mask;
            }

            break;
        }

        if (mask[m].name.len == 0) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "invalid value \"%s\"", value[i].data);

            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


#if 0

char *
ngx_conf_unsupported(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    return "unsupported on this platform";
}

#endif


char *
ngx_conf_deprecated(ngx_conf_t *cf, void *post, void *data)
{
    ngx_conf_deprecated_t  *d = post;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                       "the \"%s\" directive is deprecated, "
                       "use the \"%s\" directive instead",
                       d->old_name, d->new_name);

    return NGX_CONF_OK;
}


char *
ngx_conf_check_num_bounds(ngx_conf_t *cf, void *post, void *data)
{
    ngx_conf_num_bounds_t  *bounds = post;
    ngx_int_t  *np = data;

    if (bounds->high == -1) {
        if (*np >= bounds->low) {
            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "value must be equal to or greater than %i",
                           bounds->low);

        return NGX_CONF_ERROR;
    }

    if (*np >= bounds->low && *np <= bounds->high) {
        return NGX_CONF_OK;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "value must be between %i and %i",
                       bounds->low, bounds->high);

    return NGX_CONF_ERROR;
}
