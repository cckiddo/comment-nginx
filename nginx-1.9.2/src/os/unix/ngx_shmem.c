
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


/*
14.2�����ڴ�
�����ڴ���Linux���ṩ��������Ľ��̼�ͨ�ŷ�������ͨ��mmap����shmgetϵͳ�������ڴ��д�����һ�����������Ե�ַ�ռ䣬��ͨ��munmap��
��shmdtϵͳ���ÿ����ͷ�����ڴ档ʹ�ù����ڴ�ĺô��ǵ��������ʹ��ͬһ�鹲���ڴ�ʱ�����κ�һ�������޸��˹����ڴ��е����ݺ���
������ͨ��������ι����ڴ涼�ܹ��õ��޸ĺ�����ݡ�
    ע��:��Ȼmmap�����Դ����ļ��ķ�ʽӳ�乲���ڴ棬����Nginx��װ�Ĺ����ڴ������������û��ʹ�õ�ӳ���ļ����ܵġ�

    ����ngx_shm_t�ṹ��ķ���������������ngx_shm_alloc���ڷ����µĹ����ڴ棬��
ngx_shm_free�����ͷ��Ѿ����ڵĹ����ڴ档����������������ǰ������mmapΪ��˵��
Linux��������Ӧ�ó����ṩ�����ڴ�ģ�������ʾ��
void *mmap (void *start, size_t length,  int prot,  int flags, int fd, off_t offset) ;
    mmap���Խ������ļ�ӳ�䵽�ڴ��У�ֱ�Ӳ����ڴ�ʱLinux�ں˽�����ͬ���ڴ�ʹ�
���ļ��е����ݣ�fd������ָ����Ҫͬ���Ĵ����ļ�����offset�������ļ������ƫ����
����ʼ������ȻNginxû��ʹ����һ���ԡ���flags�����м���MAP ANON����MAP��
ANONYMOUS����ʱ��ʾ��ʹ���ļ�ӳ�䷽ʽ����ʱfd��offset������û�����壬Ҳ��
��Ҫ�����ˣ���ʱ��mmap������ngx_shm_alloc�Ĺ��ܼ�����ȫ��ͬ��length�������ǽ�
Ҫ���ڴ��п��ٵ����Ե�ַ�ռ��С����prot�������ǲ�����ι����ڴ�ķ�ʽ����ֻ��
���߿ɶ���д����start����˵��ϣ���Ĺ����ڴ���ʼӳ���ַ����Ȼ��ͨ�������start��Ϊ
NULL��ָ�롣
    �����������ʹ��mmapʵ��ngx_shm_alloc�������������¡�
ngx_int_t ngx_shm_ alloc (ngx_shm_t  ~shm)
{
    ��������һ��shm- >size��С�ҿ��Զ���д�Ĺ����ڴ棬�ڴ��׵�ַ�����addr��
    shm->addr=  (uchar *)mmap (NULL,  shm->size,
    PROT_READ l PROT_WRITE,
    MAP_ANONIMAP_SHARED,  -1,o);
if (shm->addr == MAP_FAILED)
     return NGX ERROR;
}
    return NGX OK;
    )
    ���ﲻ�ٽ���shmget�������빲���ڴ�ķ�ʽ�����������������ơ�
    ������ʹ�ù����ڴ�ʱ����Ҫ����munmap����shmdt���ͷŹ����ڴ棬���ﻹ������
mmap��Ե�munmapΪ����˵����
    ���У�start����ָ�����ڴ���׵�ַ����length������ʾ��ι����ڴ�ĳ��ȡ�����
����ngx_shm_free����������ͨ��munmap���ͷŹ����ڴ�ġ�
    void  ngx_shm��free (ngx_shm_t��shm)
    {
    ����ʹ��ngx_shm_t�е�addr��size��������munmap�ͷŹ����ڴ漴��
    if  (munmap( (void��)  shm->addr,  shm- >size)  ==~1)  (
    ngx_log_error (NGX��LOG__  ALERT,  shm- >log,    ngx_errno,  ��munmap(%p,  %uz)
failed",  shm- >addr,   shm- >size)j
    )
    )
    Nginx�����̼乲�����ݵ���Ҫ��ʽ����ʹ�ù����ڴ棨��ʹ�ù����ڴ�ʱ��Nginx -
������master���̴�������master����fork��worker�ӽ��̺����еĽ��̿�ʼʹ����
���ڴ��е����ݣ����ڿ���Nginxģ��ʱ�����Ҫʹ������������Nginx�Ѿ���װ�õ�ngx_
shm��alloc������ngx_shm_free������������3��ʵ�֣���ӳ���ļ�ʹ��mmap���乲��
�ڴ桢��/dev/zero�ļ�ʹ��mmapӳ�乲���ڴ桢��shmget���������乲���ڴ棩������
Nginx�Ŀ�ƽ̨���Կ��ǵú��ܵ���������һ��ͳ��HTTP�������״����������˵������
�ڴ���÷���

*/

#include <ngx_config.h>
#include <ngx_core.h>

/*
�ڿ���Nginxģ��ʱ�����Ҫʹ������������Nginx�Ѿ���װ�õ�ngx_shm_alloc������ngx_shm_free������������3��ʵ�֣���ӳ���ļ�ʹ��mmap���乲��
�ڴ桢��/dev/zero�ļ�ʹ��mmapӳ�乲���ڴ桢��shmget(system-v��׼)���������乲���ڴ棩
*/
//#if�����������defineΪ1���������������һ��������ѡ���һ��if�е�
#if (NGX_HAVE_MAP_ANON)

ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    shm->addr = (u_char *) mmap(NULL, shm->size,
                                PROT_READ|PROT_WRITE,
                                MAP_ANON|MAP_SHARED, -1, 0);

    if (shm->addr == MAP_FAILED) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "mmap(MAP_ANON|MAP_SHARED, %uz) failed", shm->size);
        return NGX_ERROR;
    }

    return NGX_OK;
}


void
ngx_shm_free(ngx_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "munmap(%p, %uz) failed", shm->addr, shm->size);
    }
}

#elif (NGX_HAVE_MAP_DEVZERO)
/*
�ڿ���Nginxģ��ʱ�����Ҫʹ������������Nginx�Ѿ���װ�õ�ngx_shm_alloc������ngx_shm_free������������3��ʵ�֣���ӳ���ļ�ʹ��mmap���乲��
�ڴ桢��/dev/zero�ļ�ʹ��mmapӳ�乲���ڴ桢��shmget(system-v��׼)���������乲���ڴ棩
*/

ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    ngx_fd_t  fd;
  
    fd = open("/dev/zero", O_RDWR);

    if (fd == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "open(\"/dev/zero\") failed");
        return NGX_ERROR;
    }

    shm->addr = (u_char *) mmap(NULL, shm->size, PROT_READ|PROT_WRITE,
                                MAP_SHARED, fd, 0);

    if (shm->addr == MAP_FAILED) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "mmap(/dev/zero, MAP_SHARED, %uz) failed", shm->size);
    }

    if (close(fd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "close(\"/dev/zero\") failed");
    }

    return (shm->addr == MAP_FAILED) ? NGX_ERROR : NGX_OK;
}


void
ngx_shm_free(ngx_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "munmap(%p, %uz) failed", shm->addr, shm->size);
    }
}

#elif (NGX_HAVE_SYSVSHM)
/*
�ڿ���Nginxģ��ʱ�����Ҫʹ������������Nginx�Ѿ���װ�õ�ngx_shm_alloc������ngx_shm_free������������3��ʵ�֣���ӳ���ļ�ʹ��mmap���乲��
�ڴ桢��/dev/zero�ļ�ʹ��mmapӳ�乲���ڴ桢��shmget(system-v��׼)���������乲���ڴ棩
*/

#include <sys/ipc.h>
#include <sys/shm.h>


ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    int  id;
 
    id = shmget(IPC_PRIVATE, shm->size, (SHM_R|SHM_W|IPC_CREAT));

    if (id == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "shmget(%uz) failed", shm->size);
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, shm->log, 0, "shmget id: %d", id);

    shm->addr = shmat(id, NULL, 0);

    if (shm->addr == (void *) -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "shmat() failed");
    }

    if (shmctl(id, IPC_RMID, NULL) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "shmctl(IPC_RMID) failed");
    }

    return (shm->addr == (void *) -1) ? NGX_ERROR : NGX_OK;
}


void
ngx_shm_free(ngx_shm_t *shm)
{
    if (shmdt(shm->addr) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno,
                      "shmdt(%p) failed", shm->addr);
    }
}

#endif
