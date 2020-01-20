
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static ngx_int_t ngx_test_full_name(ngx_str_t *name);


static ngx_atomic_t   temp_number = 0;
ngx_atomic_t         *ngx_temp_number = &temp_number;
ngx_atomic_int_t      ngx_random_number = 123456; // ngx_random_number = (tp->msec << 16) + ngx_pid;


ngx_int_t
ngx_get_full_name(ngx_pool_t *pool, ngx_str_t *prefix, ngx_str_t *name)
{
    size_t      len;
    u_char     *p, *n;
    ngx_int_t   rc;

    rc = ngx_test_full_name(name);

    if (rc == NGX_OK) {
        return rc;
    }

    len = prefix->len;

#if (NGX_WIN32)

    if (rc == 2) {
        len = rc;
    }

#endif

    n = ngx_pnalloc(pool, len + name->len + 1);
    if (n == NULL) {
        return NGX_ERROR;
    }

    p = ngx_cpymem(n, prefix->data, len);
    ngx_cpystrn(p, name->data, name->len + 1);

    name->len += len;
    name->data = n;

    return NGX_OK;
}


static ngx_int_t
ngx_test_full_name(ngx_str_t *name)
{
#if (NGX_WIN32)
    u_char  c0, c1;

    c0 = name->data[0];

    if (name->len < 2) {
        if (c0 == '/') {
            return 2;
        }

        return NGX_DECLINED;
    }

    c1 = name->data[1];

    if (c1 == ':') {
        c0 |= 0x20;

        if ((c0 >= 'a' && c0 <= 'z')) {
            return NGX_OK;
        }

        return NGX_DECLINED;
    }

    if (c1 == '/') {
        return NGX_OK;
    }

    if (c0 == '/') {
        return 2;
    }

    return NGX_DECLINED;

#else

    if (name->data[0] == '/') {
        return NGX_OK;
    }

    return NGX_DECLINED;

#endif
}

/*
�������xxx_buffers  XXX_buffer_sizeָ���Ŀռ䶼�����ˣ����ѻ����е�����д����ʱ�ļ���Ȼ�������������ngx_event_pipe_write_chain_to_temp_file
��д����ʱ�ļ���ֱ��read����NGX_AGAIN,Ȼ����ngx_event_pipe_write_to_downstream->ngx_output_chain->ngx_output_chain_copy_buf�ж�ȡ��ʱ�ļ�����
���͵���ˣ������ݼ���������ͨ��epoll read����ѭ��������
*/

/*ngx_http_upstream_init_request->ngx_http_upstream_cache �ͻ��˻�ȡ���� ���Ӧ��������ݺ���ngx_http_upstream_send_response->ngx_http_file_cache_create
�д�����ʱ�ļ���Ȼ����ngx_event_pipe_write_chain_to_temp_file�Ѷ�ȡ�ĺ������д����ʱ�ļ��������
ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update�а���ʱ�ļ�����rename(�൱��mv)��proxy_cache_pathָ��
��cacheĿ¼����
*/


//����temp_file��ʱ�ļ�������chain�����е�����д���ļ�������ֵΪд�뵽�ļ��е��ֽ���
//�������xxx_buffers  XXX_buffer_sizeָ���Ŀռ䶼�����ˣ����ѻ����е�����д����ʱ�ļ���Ȼ���������������д����ʱ�ļ���ֱ��read����NGX_AGAIN
ssize_t
ngx_write_chain_to_temp_file(ngx_temp_file_t *tf, ngx_chain_t *chain)
{
    ngx_int_t  rc;

    if (tf->file.fd == NGX_INVALID_FILE) {
        rc = ngx_create_temp_file(&tf->file, tf->path, tf->pool,
                                  tf->persistent, tf->clean, tf->access); //������ʱ�ļ�

        if (rc != NGX_OK) {
            return rc;
        }

        if (tf->log_level) {
            ngx_log_error(tf->log_level, tf->file.log, 0, "%s %V",
                          tf->warn, &tf->file.name);
        }
    }

    //д��ʱ�ļ���ʱ�����tf->file->offset  tf->file->sys_offset(Ҳ����ngx_file_t�еĳ�Ա)  tf->offset(������ngx_temp_file_t->offset)�ڸú���������
    return ngx_write_chain_to_file(&tf->file, chain, tf->offset, tf->pool);
}


/*
����fastcgi_cache_path /var/yyz/cache_xxx levels=1:2 keys_zone=fcgi:1m inactive=30m max_size=64 use_temp_path=off;��ӡ����:

2015/12/16 04:25:19[               ngx_create_temp_file,   169]  [debug] 19348#19348: *3 hashed path: /var/yyz/cache_xxx/temp/2/00/0000000002
2015/12/16 04:25:19[               ngx_create_temp_file,   174]  [debug] 19348#19348: *3 temp fd:-1
2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2"
2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2/00"
2015/12/16 04:25:19[               ngx_create_temp_file,   169]  [debug] 19348#19348: *3 hashed path: /var/yyz/cache_xxx/temp/2/00/0000000002
2015/12/16 04:25:19[               ngx_create_temp_file,   174]  [debug] 19348#19348: *3 temp fd:15
*/
ngx_int_t
ngx_create_temp_file(ngx_file_t *file, ngx_path_t *path, ngx_pool_t *pool,
    ngx_uint_t persistent, ngx_uint_t clean, ngx_uint_t access)
{
    uint32_t                  n;
    ngx_err_t                 err;
    ngx_pool_cleanup_t       *cln;
    ngx_pool_cleanup_file_t  *clnf;

    file->name.len = path->name.len + 1 + path->len + 10;

    file->name.data = ngx_pnalloc(pool, file->name.len + 1);
    if (file->name.data == NULL) {
        return NGX_ERROR;
    }

#if 0
    for (i = 0; i < file->name.len; i++) {
         file->name.data[i] = 'X';
    }
#endif

    ngx_memcpy(file->name.data, path->name.data, path->name.len);  //��path->name������file->name

    n = (uint32_t) ngx_next_temp_number(0);

    cln = ngx_pool_cleanup_add(pool, sizeof(ngx_pool_cleanup_file_t));
    if (cln == NULL) {
        return NGX_ERROR;
    }

    for ( ;; ) {
        (void) ngx_sprintf(file->name.data + path->name.len + 1 + path->len,
                           "%010uD%Z", n); //���ﱣ֤ÿ�δ�������ʱ�ļ���һ��

        //ע��ǰ���Ѿ���path->name������file->name
        ngx_create_hashed_filename(path, file->name.data, file->name.len);

        ngx_log_debug3(NGX_LOG_DEBUG_CORE, file->log, 0,
                       "hashed path: %s, persistent:%ui, access:%ui", 
                       file->name.data, persistent, access);

        file->fd = ngx_open_tempfile(file->name.data, persistent, access);

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, file->log, 0,
                       "temp fd:%d", file->fd);

        if (file->fd != NGX_INVALID_FILE) { //˵���Ѿ����ڸ��ļ�,���û�����������Ĵ�����Ȼ��������˳�

            cln->handler = clean ? ngx_pool_delete_file : ngx_pool_cleanup_file;
            clnf = cln->data;

            clnf->fd = file->fd;
            clnf->name = file->name.data;
            clnf->log = pool->log;

            return NGX_OK;
        }

        err = ngx_errno;

        if (err == NGX_EEXIST) { //˵�����ļ��Ѿ����ڣ������»�ȡһ������
            n = (uint32_t) ngx_next_temp_number(1);
            continue;
        }

        if ((path->level[0] == 0) || (err != NGX_ENOPATH)) {
            ngx_log_error(NGX_LOG_CRIT, file->log, err,
                          ngx_open_tempfile_n " \"%s\" failed",
                          file->name.data);
            return NGX_ERROR;
        }

        if (ngx_create_path(file, path) == NGX_ERROR) { //������Ӧ���ļ�
            return NGX_ERROR;
        }
    }
}

/*
ngx_create_hashed_filename ��ͨ��level���ö�Ӧ���ļ���·�����Ǹ���md5ֵ�����ĺ����λ��������ļ��С�
*/
void
ngx_create_hashed_filename(ngx_path_t *path, u_char *file, size_t len)
{
    size_t      i, level;
    ngx_uint_t  n;

    i = path->name.len + 1;

    file[path->name.len + path->len]  = '/';

    /*
     levels=1:2����˼��˵ʹ������Ŀ¼����һ��Ŀ¼����һ���ַ����ڶ����������ַ�������nginx���֧��3��Ŀ¼����levels=xxx:xxx:xxx��
     ��ô����Ŀ¼���ֵ��ַ��������أ��������ǵĴ洢Ŀ¼Ϊ/cache��levels=1:2����ô����������ļ� ���������洢�ģ�
     /cache/0/8d/8ef9229f02c5672c747dc7a324d658d0  ע������8d0��cache�����/0/8dһ��
    */

    for (n = 0; n < 3; n++) { //����������MD5ֵ�ַ����е����level[i]���ֽڵ�level����λ��
        level = path->level[n];

        if (level == 0) {
            break;
        }

        len -= level;
        file[i - 1] = '/';
        ngx_memcpy(&file[i], &file[len], level);
        i += level + 1;
    }
}

/*
����fastcgi_cache_path /var/yyz/cache_xxx levels=1:2 keys_zone=fcgi:1m inactive=30m max_size=64 use_temp_path=off;��ӡ����:

2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2"
2015/12/16 04:25:19[                    ngx_create_path,   260]  [debug] 19348#19348: *3 temp file: "/var/yyz/cache_xxx/temp/2/00"
*/
ngx_int_t
ngx_create_path(ngx_file_t *file, ngx_path_t *path)
{
    size_t      pos;
    ngx_err_t   err;
    ngx_uint_t  i;

    pos = path->name.len;

    for (i = 0; i < 3; i++) {
        if (path->level[i] == 0) {
            break;
        }

        pos += path->level[i] + 1;

        file->name.data[pos] = '\0';

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, file->log, 0,
                       "temp file: \"%s\"", file->name.data);

        if (ngx_create_dir(file->name.data, 0700) == NGX_FILE_ERROR) {
            err = ngx_errno;
            if (err != NGX_EEXIST) {
                ngx_log_error(NGX_LOG_CRIT, file->log, err,
                              ngx_create_dir_n " \"%s\" failed",
                              file->name.data);
                return NGX_ERROR;
            }
        }

        file->name.data[pos] = '/';
    }

    return NGX_OK;
}


ngx_err_t
ngx_create_full_path(u_char *dir, ngx_uint_t access)
{
    u_char     *p, ch;
    ngx_err_t   err;

    err = 0;

#if (NGX_WIN32)
    p = dir + 3;
#else
    p = dir + 1;
#endif

    for ( /* void */ ; *p; p++) {
        ch = *p;

        if (ch != '/') {
            continue;
        }

        *p = '\0';

        if (ngx_create_dir(dir, access) == NGX_FILE_ERROR) {
            err = ngx_errno;

            switch (err) {
            case NGX_EEXIST:
                err = 0;
            case NGX_EACCES:
                break;

            default:
                return err;
            }
        }

        *p = '/';
    }

    return err;
}

//�൱�ڻ�ȡһ�������
ngx_atomic_uint_t
ngx_next_temp_number(ngx_uint_t collision) //collision��1����
{
    ngx_atomic_uint_t  n, add;

    add = collision ? ngx_random_number : 1;

    n = ngx_atomic_fetch_add(ngx_temp_number, add);

    return n + add;
}

/*
char*(*set)(ngx_conf_t *cf, ngx_command_t 'vcmd,void *conf)
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
��ngx_conf_set_access_slot  ��¼��Ȩ��������һ�µģ�����user/group/all�����Ȩ��ֻ������Ϊrw������д����  ��
��                          ����r��ֻ�������������������κ���ʽ����w����rx�ȡ�ngx_conf_set- access slot   ��
��                          ���������������ת��Ϊһ������                                                ��
�ǩ��������������������������贈����������������������������������������������������������������������������
��                          ��  ���������������·��������������Я��1����������ʾ1���������·����      ��
��ngx_conf_set_path_slot    ��                                                                            ��
��                          ��ngx_conf_set_path_slot����Ѳ���ת��Ϊngx_path_t�ṹ                        ��
�����������������������������ߩ�����������������������������������������������������������������������������
*/

/*
ngx_conf_set_path_slot����Я��1��4�����������е�1������������·������2��4
�������������������󲿷������¿��Բ�ʹ�ã������Բμ�client_body_temp_path
��������÷���client_body_temp_path�����������ngx_conf_set_path_slotԤ�跽��������
�����ġ�
    ngx_conf_set_path_slot����������е�·������ת��Ϊngx_path_t�ṹ����һ��ngx_path_t�Ķ��塣
typedef struct {
     ngx_str_t name;
     size_t len;
      size  t  level [3]
    ngx_path_manager_pt manager;
    ngx_path_loader_pt loader;
    void *data;
     u_char *conf_file;
     ngx_uint_t line;
 } ngx_path_t ;
���У�name��Ա�洢���ַ�����ʽ��·������level�������洢�ŵ�2����3����4
��������������ڵĻ�����������ngx_http_mytest_conf_t�ṹ�е�ngx_path_t* my_path;��
�洢�����test_path����Ĳ���ֵ��
static ngx_command_t  ngx_http_mytest_commands []  =
   {   ngx_string ( " test_path" ) ,
              NGX_HTTP_LOC_CONF I NGX_CONF_TAKE1234,  '
            ngx_conf_set_path_slot ,
               NGX_HTTP_ LOC  CONF_OFFSET,
               offsetof (ngx_http_mytest  conf_t ,   my_path) ,
                  NULL 
    },
    ngx null_Command
  }
   ���nginx.conf�д���������test_path /usr/local/nginx/ 1 2 3, my_pathָ���ngx_
path_t�ṹ�У�name��������/usr/local/nginx/����level[0]Ϊ1��level[l]Ϊ2��level[2]Ϊ3��
�����������test_path /usr/local/nginx/;����ôlevel�����3����Ա����O��
*/
char *
ngx_conf_set_path_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ssize_t      level;
    ngx_str_t   *value;
    ngx_uint_t   i, n;
    ngx_path_t  *path, **slot;

    slot = (ngx_path_t **) (p + cmd->offset);

    if (*slot) {
        return "is duplicate";
    }

    path = ngx_pcalloc(cf->pool, sizeof(ngx_path_t));
    if (path == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    path->name = value[1];

    if (path->name.data[path->name.len - 1] == '/') {  //��·��������/ȥ��
        path->name.len--;
    }

    if (ngx_conf_full_name(cf->cycle, &path->name, 0) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    path->conf_file = cf->conf_file->file.name.data;
    path->line = cf->conf_file->line;

    for (i = 0, n = 2; n < cf->args->nelts; i++, n++) {
        level = ngx_atoi(value[n].data, value[n].len);
        if (level == NGX_ERROR || level == 0) {
            return "invalid value";
        }

        path->level[i] = level;
        path->len += level + 1;
    }

    if (path->len > 10 + i) {
        return "invalid value";
    }

    *slot = path;

    if (ngx_add_path(cf, slot) == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


char *
ngx_conf_merge_path_value(ngx_conf_t *cf, ngx_path_t **path, ngx_path_t *prev,
    ngx_path_init_t *init)
{
    if (*path) {
        return NGX_CONF_OK;
    }

    if (prev) {
        *path = prev;
        return NGX_CONF_OK;
    }

    *path = ngx_pcalloc(cf->pool, sizeof(ngx_path_t));
    if (*path == NULL) {
        return NGX_CONF_ERROR;
    }

    (*path)->name = init->name;

    if (ngx_conf_full_name(cf->cycle, &(*path)->name, 0) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    (*path)->level[0] = init->level[0];
    (*path)->level[1] = init->level[1];
    (*path)->level[2] = init->level[2];

    (*path)->len = init->level[0] + (init->level[0] ? 1 : 0)
                   + init->level[1] + (init->level[1] ? 1 : 0)
                   + init->level[2] + (init->level[2] ? 1 : 0);

    if (ngx_add_path(cf, path) != NGX_OK) {
        return NGX_CONF_ERROR;
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
ngx_conf_set_access_slot �������ö���дȨ�ޣ�����������°��1��3����������ˣ�
��ngx_command_t�е�type��ԱҪ����NGX��CONF TAKE123��������ȡֵ�ɲμ���4-2��
������ngx_http_mytest_conf_t�ṹ�е�ngx_uint_t my_access;���洢�����test_access��
��Ĳ���ֵ��������ʾ��
static: ngx_command_t  ngx_http_mytest_commands []  = {
    {   ngx_string ( " test_access " ) ,
              NGX_HTTP_LOC_CONF  I  NGX_CONF_TAKE123 ,
             ngx_conf_set_access_slot ,
                NGX_HTTP_LOC_CONF OFFSET,
                offsetof (ngx_http_mytest  conf_t ,   my_access) ,
                 NULL 
		},
		ngx_null_command
	}
    ������ngx_conf_set access slot�Ϳ��Խ�������дȨ�޵��������ˡ����磬��nginx
conf�г���������test access user:rw group:rw all:r;ʱ��my_access��ֵ����436��
    ע��  ��ngx_http_mytest_create loc conf�����ṹ��ʱ�������ʹ��ngx_conf_set_
access slot����ô�����my_access��ʼ��ΪNGX CONF UNSET UINT�꣬Ҳ����4.2.1
���е����mycf->my_access=NGX_CONF_UNSET_UINT;������ngx_conf_set_access_slot
����ʱ�ᱨ��
*/
char *
ngx_conf_set_access_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *confp = conf;

    u_char      *p;
    ngx_str_t   *value;
    ngx_uint_t   i, right, shift, *access;

    access = (ngx_uint_t *) (confp + cmd->offset);

    if (*access != NGX_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *access = 0600;

    for (i = 1; i < cf->args->nelts; i++) { //����λ��ʾall��Ȩ�ޣ��м���λΪgroupȨ�ޣ�����λΪuserȨ��

        p = value[i].data;

        if (ngx_strncmp(p, "user:", sizeof("user:") - 1) == 0) {
            shift = 6;
            p += sizeof("user:") - 1;

        } else if (ngx_strncmp(p, "group:", sizeof("group:") - 1) == 0) {
            shift = 3;
            p += sizeof("group:") - 1;

        } else if (ngx_strncmp(p, "all:", sizeof("all:") - 1) == 0) {
            shift = 0;
            p += sizeof("all:") - 1;

        } else {
            goto invalid;
        }

        if (ngx_strcmp(p, "rw") == 0) {
            right = 6;

        } else if (ngx_strcmp(p, "r") == 0) {
            right = 4;
        } else {
            goto invalid;
        }

        *access |= right << shift;
    }

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid value \"%V\"", &value[i]);

    return NGX_CONF_ERROR;
}

/*
���slot�Ƿ��Ѿ�����  ��������ڣ�����ӵ�cf->cycle->pathes  
���cache->path�Ƿ��Ѿ����� ��������ڣ�����ӵ�cf->cycle->pathes  
*/
ngx_int_t
ngx_add_path(ngx_conf_t *cf, ngx_path_t **slot)
{
    ngx_uint_t   i, n;
    ngx_path_t  *path, **p;

    path = *slot;

    p = cf->cycle->paths.elts;
    for (i = 0; i < cf->cycle->paths.nelts; i++) {
        if (p[i]->name.len == path->name.len
            && ngx_strcmp(p[i]->name.data, path->name.data) == 0)
        {
            if (p[i]->data != path->data) { //ָ��ͬһ���ڴ��name
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "the same path name \"%V\" "
                                   "used in %s:%ui and",
                                   &p[i]->name, p[i]->conf_file, p[i]->line);
                return NGX_ERROR;
            }

            for (n = 0; n < 3; n++) { 
                if (p[i]->level[n] != path->level[n]) {//name��ͬ������level��ͬ��˵���г�ͻ�����Ե�һ��Ϊ׼
                    if (path->conf_file == NULL) {
                        if (p[i]->conf_file == NULL) {
                            ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                                      "the default path name \"%V\" has "
                                      "the same name as another default path, "
                                      "but the different levels, you need to "
                                      "redefine one of them in http section",
                                      &p[i]->name);
                            return NGX_ERROR;
                        }

                        ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                                      "the path name \"%V\" in %s:%ui has "
                                      "the same name as default path, but "
                                      "the different levels, you need to "
                                      "define default path in http section",
                                      &p[i]->name, p[i]->conf_file, p[i]->line);
                        return NGX_ERROR;
                    }

                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                      "the same path name \"%V\" in %s:%ui "
                                      "has the different levels than",
                                      &p[i]->name, p[i]->conf_file, p[i]->line);
                    return NGX_ERROR;
                }

                if (p[i]->level[n] == 0) {
                    break;
                }
            }

            *slot = p[i];

            return NGX_OK;
        }
    }

    p = ngx_array_push(&cf->cycle->paths);
    if (p == NULL) {
        return NGX_ERROR;
    }

    *p = path; //��������slot��ӵ�cf->cycle->paths

    return NGX_OK;
}


ngx_int_t
ngx_create_paths(ngx_cycle_t *cycle, ngx_uid_t user)
{
    ngx_err_t         err;
    ngx_uint_t        i;
    ngx_path_t      **path;

    path = cycle->paths.elts;
    for (i = 0; i < cycle->paths.nelts; i++) {

        if (ngx_create_dir(path[i]->name.data, 0700) == NGX_FILE_ERROR) {
            err = ngx_errno;
            if (err != NGX_EEXIST) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, err,
                              ngx_create_dir_n " \"%s\" failed",
                              path[i]->name.data);
                return NGX_ERROR;
            }
        }

        if (user == (ngx_uid_t) NGX_CONF_UNSET_UINT) {
            continue;
        }

#if !(NGX_WIN32)
        {
        ngx_file_info_t   fi;

        if (ngx_file_info((const char *) path[i]->name.data, &fi)
            == NGX_FILE_ERROR)
        {
            ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                          ngx_file_info_n " \"%s\" failed", path[i]->name.data);
            return NGX_ERROR;
        }

        if (fi.st_uid != user) {
            if (chown((const char *) path[i]->name.data, user, -1) == -1) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                              "chown(\"%s\", %d) failed",
                              path[i]->name.data, user);
                return NGX_ERROR;
            }
        }

        if ((fi.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR))
                                                  != (S_IRUSR|S_IWUSR|S_IXUSR))
        {
            fi.st_mode |= (S_IRUSR|S_IWUSR|S_IXUSR);

            if (chmod((const char *) path[i]->name.data, fi.st_mode) == -1) {
                ngx_log_error(NGX_LOG_EMERG, cycle->log, ngx_errno,
                              "chmod() \"%s\" failed", path[i]->name.data);
                return NGX_ERROR;
            }
        }
        }
#endif
    }

    return NGX_OK;
}


ngx_int_t
ngx_ext_rename_file(ngx_str_t *src, ngx_str_t *to, ngx_ext_rename_file_t *ext)
{
    u_char           *name;
    ngx_err_t         err;
    ngx_copy_file_t   cf;

#if !(NGX_WIN32)

    if (ext->access) {
        if (ngx_change_file_access(src->data, ext->access) == NGX_FILE_ERROR) { //����ext->access�޸��ļ�Ȩ��
            ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                          ngx_change_file_access_n " \"%s\" failed", src->data);
            err = 0;
            goto failed;
        }
    }

#endif

    if (ext->time != -1) {
        if (ngx_set_file_time(src->data, ext->fd, ext->time) != NGX_OK) {
            ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                          ngx_set_file_time_n " \"%s\" failed", src->data);
            err = 0;
            goto failed;
        }
    }

    if (ngx_rename_file(src->data, to->data) != NGX_FILE_ERROR) { //�ɹ�ֱ�ӷ���
        return NGX_OK;
    }

    err = ngx_errno;

    if (err == NGX_ENOPATH) { //˵��toĿ¼�ļ������ڣ��򴴽���Ȼ������rename

        if (!ext->create_path) {
            goto failed;
        }

        err = ngx_create_full_path(to->data, ngx_dir_access(ext->path_access));

        if (err) {
            ngx_log_error(NGX_LOG_CRIT, ext->log, err,
                          ngx_create_dir_n " \"%s\" failed", to->data);
            err = 0;
            goto failed;
        }

        if (ngx_rename_file(src->data, to->data) != NGX_FILE_ERROR) {
            return NGX_OK;//�ɹ�ֱ�ӷ���
        }

        err = ngx_errno;
    }

#if (NGX_WIN32)

    if (err == NGX_EEXIST) {
        err = ngx_win32_rename_file(src, to, ext->log);

        if (err == 0) {
            return NGX_OK;
        }
    }

#endif

    if (err == NGX_EXDEV) {

        cf.size = -1;
        cf.buf_size = 0;
        cf.access = ext->access;
        cf.time = ext->time;
        cf.log = ext->log;

        name = ngx_alloc(to->len + 1 + 10 + 1, ext->log);
        if (name == NULL) {
            return NGX_ERROR;
        }

        (void) ngx_sprintf(name, "%*s.%010uD%Z", to->len, to->data,
                           (uint32_t) ngx_next_temp_number(0));

        if (ngx_copy_file(src->data, name, &cf) == NGX_OK) {

            if (ngx_rename_file(name, to->data) != NGX_FILE_ERROR) {
                ngx_free(name);

                if (ngx_delete_file(src->data) == NGX_FILE_ERROR) {
                    ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                                  ngx_delete_file_n " \"%s\" failed",
                                  src->data);
                    return NGX_ERROR;
                }

                return NGX_OK;
            }

            ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                          ngx_rename_file_n " \"%s\" to \"%s\" failed",
                          name, to->data);

            if (ngx_delete_file(name) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                              ngx_delete_file_n " \"%s\" failed", name);

            }
        }

        ngx_free(name);

        err = 0;
    }

failed:

    if (ext->delete_file) {
        if (ngx_delete_file(src->data) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_CRIT, ext->log, ngx_errno,
                          ngx_delete_file_n " \"%s\" failed", src->data);
        }
    }

    if (err) {
        ngx_log_error(NGX_LOG_CRIT, ext->log, err,
                      ngx_rename_file_n " \"%s\" to \"%s\" failed",
                      src->data, to->data);
    }

    return NGX_ERROR;
}


ngx_int_t
ngx_copy_file(u_char *from, u_char *to, ngx_copy_file_t *cf)
{
    char             *buf;
    off_t             size;
    size_t            len;
    ssize_t           n;
    ngx_fd_t          fd, nfd;
    ngx_int_t         rc;
    ngx_file_info_t   fi;

    rc = NGX_ERROR;
    buf = NULL;
    nfd = NGX_INVALID_FILE;

    fd = ngx_open_file(from, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);

    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_CRIT, cf->log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", from);
        goto failed;
    }

    if (cf->size != -1) {
        size = cf->size;

    } else {
        if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_fd_info_n " \"%s\" failed", from);

            goto failed;
        }

        size = ngx_file_size(&fi);
    }

    len = cf->buf_size ? cf->buf_size : 65536;

    if ((off_t) len > size) {
        len = (size_t) size;
    }

    buf = ngx_alloc(len, cf->log);
    if (buf == NULL) {
        goto failed;
    }

    nfd = ngx_open_file(to, NGX_FILE_WRONLY, NGX_FILE_CREATE_OR_OPEN,
                        cf->access);

    if (nfd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_CRIT, cf->log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", to);
        goto failed;
    }

    while (size > 0) {

        if ((off_t) len > size) {
            len = (size_t) size;
        }

        n = ngx_read_fd(fd, buf, len);

        if (n == -1) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_read_fd_n " \"%s\" failed", from);
            goto failed;
        }

        if ((size_t) n != len) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, 0,
                          ngx_read_fd_n " has read only %z of %uz from %s",
                          n, size, from);
            goto failed;
        }

        n = ngx_write_fd(nfd, buf, len);

        if (n == -1) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_write_fd_n " \"%s\" failed", to);
            goto failed;
        }

        if ((size_t) n != len) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, 0,
                          ngx_write_fd_n " has written only %z of %uz to %s",
                          n, size, to);
            goto failed;
        }

        size -= n;
    }

    if (cf->time != -1) {
        if (ngx_set_file_time(to, nfd, cf->time) != NGX_OK) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_set_file_time_n " \"%s\" failed", to);
            goto failed;
        }
    }

    rc = NGX_OK;

failed:

    if (nfd != NGX_INVALID_FILE) {
        if (ngx_close_file(nfd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", to);
        }
    }

    if (fd != NGX_INVALID_FILE) {
        if (ngx_close_file(fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cf->log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", from);
        }
    }

    if (buf) {
        ngx_free(buf);
    }

    return rc;
}


/*
 * ctx->init_handler() - see ctx->alloc
 * ctx->file_handler() - file handler
 * ctx->pre_tree_handler() - handler is called before entering directory
 * ctx->post_tree_handler() - handler is called after leaving directory
 * ctx->spec_handler() - special (socket, FIFO, etc.) file handler
 *
 * ctx->data - some data structure, it may be the same on all levels, or
 *     reallocated if ctx->alloc is nonzero
 *
 * ctx->alloc - a size of data structure that is allocated at every level
 *     and is initialized by ctx->init_handler()
 *
 * ctx->log - a log
 *
 * on fatal (memory) error handler must return NGX_ABORT to stop walking tree
 */

/*
ngx_walk_tree�ǵݹ麯������ÿ��·��(dir)ֱ��ÿ���ļ�(file)��������·�����ļ����õ�key���ڻ����rbtree(�����)���������key(����)��
 ���û���ҵ��Ļ��������ڴ��з���һ��ӳ������ļ���node(���ǲ�����ļ������ݽ��л���)��Ȼ����뵽������кͼ�����С�  
*/

/*
ctx->file_handler=>  
ngx_http_file_cache_manage_file=>  
ngx_http_file_cache_add_file=>  
ngx_http_file_cache_add  
*/

//ʹ�� ngx_walk_tree �ݹ��������Ŀ¼�����Բ�ͬ���͵��ļ����ݻص���������ͬ�Ĵ���
//ngx_walk_tree���������Ҫ�Ǳ������е�cacheĿ¼��Ȼ�����ÿһ��cache�ļ�����file_handler�ص���
ngx_int_t
ngx_walk_tree(ngx_tree_ctx_t *ctx, ngx_str_t *tree)
{
    void       *data, *prev;
    u_char     *p, *name;
    size_t      len;
    ngx_int_t   rc;
    ngx_err_t   err;
    ngx_str_t   file, buf;
    ngx_dir_t   dir;

    ngx_str_null(&buf);

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                   "walk tree \"%V\"", tree);

    if (ngx_open_dir(tree, &dir) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_CRIT, ctx->log, ngx_errno,
                      ngx_open_dir_n " \"%s\" failed", tree->data);
        return NGX_ERROR;
    }

    prev = ctx->data;

    if (ctx->alloc) {
        data = ngx_alloc(ctx->alloc, ctx->log);
        if (data == NULL) {
            goto failed;
        }

        if (ctx->init_handler(data, prev) == NGX_ABORT) {
            goto failed;
        }

        ctx->data = data;

    } else {
        data = NULL;
    }

    for ( ;; ) {

        ngx_set_errno(0);

        if (ngx_read_dir(&dir) == NGX_ERROR) {
            err = ngx_errno;

            if (err == NGX_ENOMOREFILES) {
                rc = NGX_OK;

            } else {
                ngx_log_error(NGX_LOG_CRIT, ctx->log, err,
                              ngx_read_dir_n " \"%s\" failed", tree->data);
                rc = NGX_ERROR;
            }

            goto done;
        }

        len = ngx_de_namelen(&dir);
        name = ngx_de_name(&dir);

        ngx_log_debug2(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                      "tree name %uz:\"%s\"", len, name);

        if (len == 1 && name[0] == '.') {
            continue;
        }

        if (len == 2 && name[0] == '.' && name[1] == '.') {
            continue;
        }

        file.len = tree->len + 1 + len;

        if (file.len + NGX_DIR_MASK_LEN > buf.len) {

            if (buf.len) {
                ngx_free(buf.data);
            }

            buf.len = tree->len + 1 + len + NGX_DIR_MASK_LEN;

            buf.data = ngx_alloc(buf.len + 1, ctx->log);
            if (buf.data == NULL) {
                goto failed;
            }
        }

        p = ngx_cpymem(buf.data, tree->data, tree->len);
        *p++ = '/';
        ngx_memcpy(p, name, len + 1);

        file.data = buf.data;

        ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                       "tree path \"%s\"", file.data);

        if (!dir.valid_info) {
            if (ngx_de_info(file.data, &dir) == NGX_FILE_ERROR) {
                ngx_log_error(NGX_LOG_CRIT, ctx->log, ngx_errno,
                              ngx_de_info_n " \"%s\" failed", file.data);
                continue;
            }
        }

        if (ngx_de_is_file(&dir)) {

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree file \"%s\"", file.data);

            ctx->size = ngx_de_size(&dir);
            ctx->fs_size = ngx_de_fs_size(&dir);
            ctx->access = ngx_de_access(&dir);
            ctx->mtime = ngx_de_mtime(&dir);

            if (ctx->file_handler(ctx, &file) == NGX_ABORT) {
                goto failed;
            }

        } else if (ngx_de_is_dir(&dir)) {

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree enter dir \"%s\"", file.data);

            ctx->access = ngx_de_access(&dir);
            ctx->mtime = ngx_de_mtime(&dir);

            rc = ctx->pre_tree_handler(ctx, &file);

            if (rc == NGX_ABORT) {
                goto failed;
            }

            if (rc == NGX_DECLINED) {
                ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                               "tree skip dir \"%s\"", file.data);
                continue;
            }

            if (ngx_walk_tree(ctx, &file) == NGX_ABORT) {
                goto failed;
            }

            ctx->access = ngx_de_access(&dir);
            ctx->mtime = ngx_de_mtime(&dir);

            if (ctx->post_tree_handler(ctx, &file) == NGX_ABORT) {
                goto failed;
            }

        } else {

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, ctx->log, 0,
                           "tree special \"%s\"", file.data);

            if (ctx->spec_handler(ctx, &file) == NGX_ABORT) {
                goto failed;
            }
        }
    }

failed:

    rc = NGX_ABORT;

done:

    if (buf.len) {
        ngx_free(buf.data);
    }

    if (data) {
        ngx_free(data);
        ctx->data = prev;
    }

    if (ngx_close_dir(&dir) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_CRIT, ctx->log, ngx_errno,
                      ngx_close_dir_n " \"%s\" failed", tree->data);
    }

    return rc;
}

