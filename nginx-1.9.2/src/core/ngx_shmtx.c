
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/*
�ź�������������Ϊ�ṹsem_t������������һ�������͵���������sem_init����������ʼ��һ���ź���������ԭ��Ϊ������ 
extern int sem_init __P ((sem_t *__sem, int __pshared, unsigned int __value));����

semΪָ���ź����ṹ��һ��ָ�룻pshared��Ϊ��ʱ���ź����ڽ��̼乲������ֻ��Ϊ��ǰ���̵������̹߳���value�������ź����ĳ�ʼֵ������

����sem_post( sem_t *sem )���������ź�����ֵ�������߳�����������ź�����ʱ���������������ʹ���е�һ���̲߳���������ѡ�����ͬ�������̵߳ĵ��Ȳ��Ծ����ġ�����

����sem_wait( sem_t *sem )������������ǰ�߳�ֱ���ź���sem��ֵ����0�����������sem��ֵ��һ������������Դ��ʹ�ú���١�����sem_trywait ( sem_t *sem )�Ǻ���sem_wait�����ķ������汾����ֱ�ӽ��ź���sem��ֵ��һ������

����sem_destroy(sem_t *sem)�����ͷ��ź���sem����

�ź�����sem_init���������ģ�����������˵����
#include<semaphore.h>
       int sem_init (sem_t *sem, int pshared, unsigned int value);

       ��������������Ƕ���semָ�����ź������г�ʼ�������ú����� ����ѡ���ָ��һ���������͵ĳ�ʼֵ��pshared�����������ź��������͡���� pshared��ֵ�ǣ����ͱ�ʾ���ǵ�ǰ��̵ľֲ��ź����������������̾��ܹ���������ź�������������ֻ�Բ��ý��̹�����ź�������Ȥ������������� �ܰ汾Ӱ�죩����pshared����һ�����㽫��ʹ��������ʧ�ܡ�

�����������������ź�����ֵ�����ǵĶ���������ʾ�� 

#include <semaphore.h> 
int sem_wait(sem_t * sem);
int sem_post(sem_t * sem);
������������Ҫ��һ����sem_init���ó�ʼ�����ź��������ָ����������
sem_post�����������Ǹ��ź�����ֵ����һ����1�������� һ����ԭ�Ӳ�������������ͬʱ��ͬһ���ź������ӡ�1�������������߳��ǲ����ͻ�ģ���ͬ ʱ��ͬһ���ļ����ж����Ӻ�д����������������п��ܻ������ͻ���ź�����ֵ��Զ����ȷ�ؼ�һ����2��������Ϊ�������߳���ͼ�ı�����
sem_wait����Ҳ��һ��ԭ�Ӳ��������������Ǵ��ź�����ֵ ��ȥһ����1����������Զ���ȵȴ����ź���Ϊһ������ֵ�ſ�ʼ��������Ҳ����˵�������� һ��ֵΪ2���ź�������sem_wait(),�߳̽������ִ�У����ź�����ֵ������1�������һ��ֵΪ0���ź�������sem_wait()����������� ��صȴ�ֱ���������߳����������ֵʹ��������0Ϊֹ������������̶߳���sem_wait()�еȴ�ͬһ���ź�����ɷ���ֵ����ô�������������߳����� һ����1��ʱ���ȴ��߳���ֻ��һ���ܹ����ź���������������ִ�У���һ���������ڵȴ�״̬��
�ź������֡�ֻ��һ����������ԭ�ӻ��ز��Ժ����á����������������ļ�ֵ���ڡ� ��������һ���ź�������sem_trywait������sem_wait�ķ��������

���һ���ź���������sem_destroy����������������������������ź�������������������Ķ��壺
 #include<semaphore.h>
 int sem_destroy (sem_t *sem);
 �������Ҳʹ��һ���ź���ָ�����������黹�Լ�սʤ��һ����Դ���������ź�����ʱ����������߳��ڵȴ������û��ͻ��յ�һ������
�������ĺ���һ������Щ�����ڳɹ�ʱ�����ء�0����


�ź��������ʵ�ֻ��������ܵ��أ����磬������ź���semֵΪ0������sem_post��
�������semֵ��1����������������κ�����������sem__ wait���ƽ�����ź���sem��ֵ
��l�����semֵ�Ѿ�С�ڻ����0�ˣ�������ס��ǰ���̣����̻����˯��״̬����ֱ��
�������̽��ź���sem��ֵ�ı�Ϊ��������ʱ���ܼ���ͨ����sem��1��ʹ�õ�ǰ����
��������ִ�С���ˣ�sem_post��������ʵ�ֽ����Ĺ��ܣ���sem wait��������ʵ�ּ���
�Ĺ��ܡ�

nginx��ԭ�ӱ������ź���������ʵ�ָ�Ч��������

��14-1��������5�ֲ�������
���������������������ש����������������������������������������������ש���������������������������������
��    ������        ��    ����                                      ��    ����                        ��
�ǩ������������������贈���������������������������������������������贈��������������������������������
��                  ��  ����mtx��ʾ��������ngx_shmtx_t���ͻ�������  ��                                ��
��                  ������������ԭ�ӱ���ʵ��ʱ������addr��ʾҪ����  ��                                ��
��ngx_shmtx_create  ����ԭ�ӱ������������������ļ�ʵ��ʱ������addr  ��  ��ʼ��mtx������               ��
��                  ��û���κ����壻����name�������������ļ�ʵ��ʱ  ��                                ��
��                  ���������壬����ʾ����ļ����ڵ�·�����ļ���    ��                                ��
�ǩ������������������贈���������������������������������������������贈��������������������������������
��ngx_shmtx_destory ��  ����mtx��ʾ��������ngx_shmtx_t���ͻ�����    ��  ����mtx������                 ��
�ǩ������������������贈���������������������������������������������贈��������������������������������
��                  ��                                              ��  ����������ͼ��ȡ������������  ��
��ngx_shmtx_trylock ��  ����mtx��ʾ��������ngx_shmtx_t���ͻ�����    ��1��ʾ��ȡ�������ɹ�������0��ʾ  ��
��                  ��                                              ����ȡ������ʧ��                  ��
�ǩ������������������贈���������������������������������������������贈��������������������������������
��                  ��                                              ��  ���������̵ķ����ȡ��������  ��
��ngx_shmtx_lock    ��  ����mtx��ʾ��������ngx_shmtx_t���ͻ�����    ��                                ��
��                  ��                                              ���ڷ�������ʱ���Ѿ����л�������  ��
�ǩ������������������贈���������������������������������������������贈��������������������������������
��ngx_shmtx_unlock  ��  ����mtx��ʾ��������ngx_shmtx_t���ͻ�����    ��  �ͷŻ�����                    ��
���������������������ߩ����������������������������������������������ߩ���������������������������������
*/

#if (NGX_HAVE_ATOMIC_OPS) //֧��ԭ�Ӳ���  //֧��ԭ�Ӳ�������ͨ��ԭ�Ӳ���ʵ����


static void ngx_shmtx_wakeup(ngx_shmtx_t *mtx);
/*
ngx_shmtx_t�ṹ���漰�����꣺NGX_HAVE_ATOMIC_OPS��NGX_HAVE_POSIX_SEM�����������Ӧ�Ż�������3�ֲ�ͬʵ�֡�
    ��1��ʵ�֣�����֧��ԭ�Ӳ���ʱ����ʹ���ļ�����ʵ��ngx_shmtx_t����������ʱ������fd��name��Ա��ʵ���ϻ���spin��Ա��
    ����ʱû���κ����壩����������Աʹ���ļ������ṩ�������������Ļ�������
    ��2��ʵ�֣�֧��ԭ�Ӳ���ȴ�ֲ�֧���ź�����
    ��3��ʵ�֣���֧��ԭ�Ӳ�����ͬʱ������ϵͳҲ֧���ź�����
    ������ʵ�ֵ�Ψһ������ngx_shmtx_lock����ִ��ʱ��Ч����Ҳ����˵��֧���ź���
ֻ��Ӱ���������̵�ngx_shmtx_lock�����������ķ�ʽ������֧���ź���ʱ��ngx_shmtx_
lockȡ������������һ�µģ���֧���ź�����ngx_shmtx_lock����spin
ָ����һ��ʱ���������ȴ������������ͷ���������ﵽspin���޻�û�л�ȡ��������ô��
��ʹ��sem_waitʹ��ۻǰ���̽���˯��״̬�������������ͷ������ں˺�Żỽ�������
�̡���Ȼ����ʵ��ʵ�ֹ����У�Nginx���˷ǳ��������ƣ���ʹ��ngx_shmtx_lock������
����һ��ʱ��������������ʼ�ղ�����������ô��ǰ���̽��п���ǿ���Եػ�õ����
������Ҳ�ǳ���Nginx����ʹ���������̵�˯��������Ŀ��ǡ�
*/
//addrΪ�����ڴ�ngx_shm_alloc���ٵĿռ��е�һ��128�ֽ��׵�ַ  nginx��ԭ�ӱ������ź���������ʵ�ָ�Ч��������
ngx_int_t
ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr, u_char *name)
{
    mtx->lock = &addr->lock;    //ֱ��ִ�й����ڴ�ռ�addr�е�lock������

    if (mtx->spin == (ngx_uint_t) -1) { //ע�⣬��spinֵΪ-1ʱ����ʾ����ʹ���ź�������ʱֱ�ӷ��سɹ�
        return NGX_OK;
    }

    mtx->spin = 2048; //spinֵĬ��Ϊ2048

//ͬʱʹ���ź���
#if (NGX_HAVE_POSIX_SEM)
    mtx->wait = &addr->wait;

    /*
    int  sem init (sem_t  sem,  int pshared,  unsigned int value) ,
    ���У�����sem��Ϊ���Ƕ�����ź�����������pshared��ָ��sem�ź��������ڽ��̼�ͬ�����������̼߳�ͬ������psharedΪ0ʱ��ʾ�̼߳�ͬ����
    ��psharedΪ1ʱ��ʾ���̼�ͬ��������Nginx��ÿ�����̶��ǵ��̵߳ģ���˽�����pshared��Ϊ1���ɡ�����value��ʾ�ź���sem�ĳ�ʼֵ��
     */
    //�Զ����ʹ�õķ�ʽ��ʼ��sem�ź�����sem��ʼֵΪ0
    if (sem_init(&mtx->sem, 1, 0) == -1) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno,
                      "sem_init() failed");
    } else {
        mtx->semaphore = 1; //���ź�����ʼ���ɹ�������semaphore��־λΪ1
    }

#endif

    return NGX_OK;
}


void
ngx_shmtx_destroy(ngx_shmtx_t *mtx)
{
#if (NGX_HAVE_POSIX_SEM) //֧���ź���ʱ���д�����Ҫִ��

    if (mtx->semaphore) { //���������spinֵ��Ϊ(ngx_uint_t)    -1ʱ���ҳ�ʼ���ź����ɹ���semaphore��־λ��Ϊl
        if (sem_destroy(&mtx->sem) == -1) { //�����ź���
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno,
                          "sem_destroy() failed");
        }
    }

#endif
}

/*
�������ж�mtx��lock���Ƿ����0����������ڣ���ô��ֱ�ӷ���false���ˣ�������ڵĻ�����ô��Ҫ����ԭ�Ӳ���ngx_atomic_cmp_set�ˣ�
�����ڱȽ�mtx��lock����������㣬��ô����Ϊ��ǰ���̵Ľ���id�ţ����򷵻�false��
*/ //�����ܲ��ܻ�õ���������  nginx��ԭ�ӱ������ź���������ʵ�ָ�Ч��������
ngx_uint_t
ngx_shmtx_trylock(ngx_shmtx_t *mtx)
{
    return (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid));
}

/*
����ʽ��ȡ��������ngx_shmtx_lock������Ϊ���ӣ��ڲ�֧���ź���ʱ����14.3.3�ڽ��ܵ�������������ȫ��ͬ������֧�����ź�������
���п���ʹ���̽���˯��״̬��
*/
//������Կ���֧��ԭ�Ӳ�����ϵͳ�������ź�����ʵ�����������ź����Ľ��   nginx��ԭ�ӱ������ź���������ʵ�ָ�Ч��������
void
ngx_shmtx_lock(ngx_shmtx_t *mtx)
{
    ngx_uint_t         i, n;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx lock");
    
    //һ����ѭ�������ϵ�ȥ���Ƿ��ȡ������ֱ����ȡ��֮����˳�   
    //����֧��ԭ�ӱ�����
    for ( ;; ) {
 //lockֵ�ǵ�ǰ����״̬��ע�⣬lockһ�����ڹ����ڴ��еģ������ܻ�ʱ�̱仯����val�ǵ�ǰ���̵�ջ�б�������������ִ������������lockֵ��һ��
        if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid)) {
            return;
        }
        //���ڶദ����״̬��spinֵ�������壬����PAUSEָ���ǲ���ִ�е�
        if (ngx_ncpu > 1) {
            //ѭ��ִ��PAUSE��������Ƿ��Ѿ��ͷ�
            for (n = 1; n < mtx->spin; n <<= 1) {
                //���ų�ʱ��û�л�õ���������ִ�и����PAUSE�Ż�����
                for (i = 0; i < n; i++) {
                    ngx_cpu_pause();
                }

                //�ٴ��ɹ����ڴ��л��lockԭ�ӱ�����ֵ
                if (*mtx->lock == 0
                    && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid))
                {
                    return;
                }
            }
        }

#if (NGX_HAVE_POSIX_SEM) //֧���ź���ʱ�ż���ִ��

        if (mtx->semaphore) {//semaphore��־λΪ1��ʹ���ź���
            (void) ngx_atomic_fetch_add(mtx->wait, 1);

            //���»�ȡһ�ο����鹲���ڴ��е�lockԭ�ӱ���
            if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid)) {
                (void) ngx_atomic_fetch_add(mtx->wait, -1);
                return;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                           "shmtx wait %uA", *mtx->wait);

            //���û���õ�������ʱNginx���̽���˯�ߣ�ֱ�����������ͷ�����
            /*
                ����ź���sem��ֵ�����semֵΪ��������semֵ��1����ʾ�õ����ź�����������ͬʱsem wait��������o�����semֵΪ0��
                �߸�������ǰ���̽���˯��״̬���ȴ���������ʹ��ngx_shmtx_unlock�����ͷ������ȴ�sem�ź�����Ϊ����������ʱLinux�ں�
                �����µ��ȵ�ǰ���̣��������semֵ�Ƿ�Ϊ�����ظ���������
               */
            while (sem_wait(&mtx->sem) == -1) {
                ngx_err_t  err;

                err = ngx_errno;

                if (err != NGX_EINTR) {//��EINTR�źų���ʱ����ʾsem waitֻ�Ǳ���ϣ������ǳ���
                    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, err,
                                  "sem_wait() failed while waiting on shmtx");
                    break;
                }
            }

            ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                           "shmtx awoke");

            continue; //ѭ�����lock����ֵ��ע�⣬��ʹ���ź����󲻻����sched_yield
        }

#endif

        ngx_sched_yield(); //�ڲ�ʹ���ź���ʱ������sched_yield����ʹ��ǰ������ʱ���ó���������
    }
}

/*
ngx_shmtx_unlock�������ͷ�������Ȼ����ͷŹ��̲����������̣�������ԭ�ӱ���lockֵʱ�ǿ���ʧ�ܵģ���Ϊ�������ͬʱ�޸�lockֵ��
��ngx_atomic_cmp_s et����Ҫ�����old��ֵ��lockֵ��ͬʱ�����޸ĳɹ�����ˣ�ngx_atomic_cmp_set��������ѭ���з���ִ�У�ֱ������
�ɹ�Ϊֹ��
*/
//�ж�����lock���뵱ǰ���̵Ľ���id�Ƿ���ȣ������ȵĻ�����ô�ͽ�lock����Ϊ0��Ȼ����൱���ͷ�������
void
ngx_shmtx_unlock(ngx_shmtx_t *mtx)
{
    if (mtx->spin != (ngx_uint_t) -1) {
        ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx unlock");
    }

    if (ngx_atomic_cmp_set(mtx->lock, ngx_pid, 0)) {
        ngx_shmtx_wakeup(mtx);
    }
}


ngx_uint_t
ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid)
{
    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "shmtx forced unlock");

    if (ngx_atomic_cmp_set(mtx->lock, pid, 0)) {
        ngx_shmtx_wakeup(mtx);
        return 1;
    }

    return 0;
}


static void
ngx_shmtx_wakeup(ngx_shmtx_t *mtx)
{
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_uint_t  wait;

    if (!mtx->semaphore) {
        return;
    }

    for ( ;; ) {

        wait = *mtx->wait;
        //���lock��ԭ�ȵ�ֵΪo��Ҳ����˵����û����ĳ�����̳���������ʱֱ�ӷ��أ����ߣ�semaphore��־λΪ0����ʾ����Ҫʹ���ź�����Ҳ��������
        if ((ngx_atomic_int_t) wait <= 0) {
            return;
        }

        if (ngx_atomic_cmp_set(mtx->wait, wait, wait - 1)) {
            break;
        }
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "shmtx wake %uA", wait);

    //�ͷ��ź�����ʱ�ǲ���ʹ����˯�ߵ�
    //ͨ��sem_post���ź���sem��1����ʾ��ǰ�����ͷ����ź�����������֪ͨ�������̵�sem_wait����ִ��
    if (sem_post(&mtx->sem) == -1) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno,
                      "sem_post() failed while wake shmtx");
    }

#endif
}


#else  //else��������ļ���ʵ�ֵ�ngx_shmtx_t��   //��֧��ԭ�Ӳ�������ͨ���ļ���ʵ��


ngx_int_t
ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr, u_char *name)
{
    //�����ڵ���ngx_shmtx_create����ǰ���и�ֵ��ngx_shmtx_t�ṹ���еĳ�Ա
    if (mtx->name) {
        /*
         ���ngx_shmtx_t�е�name��Ա��ֵ����ô�����name������ͬ����ζ��mtx�������Ѿ���ʼ�����ˣ�������Ҫ������mtx�еĻ����������·���mtx
          */
        if (ngx_strcmp(name, mtx->name) == 0) {
            mtx->name = name; //���name������ngx_shmtx_t�е�name��Աͩͬ�����ʾ�Ѿ���ʼ����
            return NGX_OK; //��Ȼ������ʼ������֤��fd����Ѿ��򿪹���ֱ�ӷ��سɹ�����
        }

        /*
           ���ngx_s hmtx_t�е�name�����name��һ�£�˵����һ��ʹ����һ���µ��ļ���Ϊ�ļ�������ô�ȵ���ngx_shmtx_destory��������ԭ�ļ���
          */
        ngx_shmtx_destroy(mtx);
    }

    mtx->fd = ngx_open_file(name, NGX_FILE_RDWR, NGX_FILE_CREATE_OR_OPEN,
                            NGX_FILE_DEFAULT_ACCESS);

    if (mtx->fd == NGX_INVALID_FILE) { //һ���ļ���Ϊ����ԭ����Ȩ�޲������޷��򿪣�ͨ��������޷����д���
        ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, ngx_errno,
                      ngx_open_file_n " \"%s\" failed", name);
        return NGX_ERROR;
    }

    //����ֻ��Ҫ����ļ����ں��е�INODE��Ϣ�����Կ��԰��ļ�ɾ����ֻҪfd���þ���
    if (ngx_delete_file(name) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno,
                      ngx_delete_file_n " \"%s\" failed", name);
    }

    mtx->name = name;

    return NGX_OK;
}


void
ngx_shmtx_destroy(ngx_shmtx_t *mtx)
{
    if (ngx_close_file(mtx->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", mtx->name);
    }
}

//ngx_shmtx_trylock������ͼʹ�÷������ķ�ʽ�����������1ʱ��ʾ��ȡ���ɹ�������0��ʾ��ȡ��ʧ��
ngx_uint_t
ngx_shmtx_trylock(ngx_shmtx_t *mtx)  //ngx_shmtx_unlock��ngx_shmtx_lock��Ӧ
{
    ngx_err_t  err;
    printf("yang test xxxxxxxxxxxxxxxxxxxxxx 1111111111111111111111\r\n");

    //��14.7�ڽ��ܹ���ngx_t rylock_fd����ʵ�ַ����������ļ����Ļ�ȡ
    err = ngx_trylock_fd(mtx->fd);

    if (err == 0) {
        return 1;
    }

    if (err == NGX_EAGAIN) { //���err��������NGX EAGAIN����ũʾ�������Ѿ����������̳�����
        return 0;
    }

#if __osf__ /* Tru64 UNIX */

    if (err == NGX_EACCES) {
        return 0;
    }

#endif

    ngx_log_abort(err, ngx_trylock_fd_n " %s failed", mtx->name);

    return 0;
}

/*
ngx_shmtx_lock���������ڻ�ȡ��ʧ��ʱ��������ļ���ִ�У�����ʹ��ǰ���̴���˯��״̬���ȴ����������ͷ������ں˻�������
�ɼ�������ͨ��14.7�ڽ��ܵ�ngx_lock_fd����ʵ�ֵģ�������ʾ��
*/
void
ngx_shmtx_lock(ngx_shmtx_t *mtx) //ngx_shmtx_unlock��ngx_shmtx_lock��Ӧ
{
    ngx_err_t  err;

    err = ngx_lock_fd(mtx->fd);

    if (err == 0) { //ngx_lock_fd��������0ʱ��ʾ�ɹ��س�����������-1ʱ��ʾ���ִ���
        return;
    }

    ngx_log_abort(err, ngx_lock_fd_n " %s failed", mtx->name);
}

//ngx_shmtx_lock����û�з���ֵ����Ϊ��һ�����ؾ��൱�ڻ�ȡ���������ˣ����ʹ�ô����������ִ�С�
//ngx_shmtx unlock����ͨ������ngx_unlock_fd�������ͷ��ļ���
void
ngx_shmtx_unlock(ngx_shmtx_t *mtx)
{
    ngx_err_t  err;

    err = ngx_unlock_fd(mtx->fd);

    if (err == 0) {
        return;
    }

    ngx_log_abort(err, ngx_unlock_fd_n " %s failed", mtx->name);
}


ngx_uint_t
ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid)
{
    return 0;
}

#endif
