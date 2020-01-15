
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_POSIX_CONFIG_H_INCLUDED_
#define _NGX_POSIX_CONFIG_H_INCLUDED_


#if (NGX_HPUX)
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED  1
#define _HPUX_ALT_XOPEN_SOCKET_API
#endif


#if (NGX_TRU64)
#define _REENTRANT
#endif


#if (NGX_GNU_HURD)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* accept4() */
#endif
#define _FILE_OFFSET_BITS       64
#endif


#ifdef __CYGWIN__
#define timezonevar             /* timezone is variable */
#define NGX_BROKEN_SCM_RIGHTS   1
#endif


#include <sys/types.h>
#include <sys/time.h>
#if (NGX_HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if (NGX_HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <stdarg.h>
#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <glob.h>
#include <time.h>
#if (NGX_HAVE_SYS_PARAM_H)
#include <sys/param.h>          /* statfs() */
#endif
#if (NGX_HAVE_SYS_MOUNT_H)
#include <sys/mount.h>          /* statfs() */
#endif
#if (NGX_HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>        /* statvfs() */
#endif

#if (NGX_HAVE_SYS_FILIO_H)
#include <sys/filio.h>          /* FIONBIO */
#endif
#include <sys/ioctl.h>          /* FIONBIO */

#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>        /* TCP_NODELAY */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#if (NGX_HAVE_LIMITS_H)
#include <limits.h>             /* IOV_MAX */
#endif

#ifdef __CYGWIN__
#include <malloc.h>             /* memalign() */
#endif

#if (NGX_HAVE_CRYPT_H)
#include <crypt.h>
#endif


#ifndef IOV_MAX
#define IOV_MAX   16
#endif


#include <ngx_auto_config.h>


#if (NGX_HAVE_POSIX_SEM)
#include <semaphore.h>
#endif


#if (NGX_HAVE_POLL)
#include <poll.h>
#endif


#if (NGX_HAVE_KQUEUE)
#include <sys/event.h>
#endif


#if (NGX_HAVE_DEVPOLL)
#include <sys/ioctl.h>
#include <sys/devpoll.h>
#endif

/*
 struct iocb  f
//�洢��ҵ����Ҫ��ָ�롣���磬��Nginx�У�����ֶ�ͨ���洢�Ŷ�Ӧ��ngx_event_tͤ����ָ�롣��ʵ������io_getevents�����з��ص�io event�ṹ���data��Ա����ȫһ�µ�+��
    u int64 t aio_data;
//����Ҫ����
u  int32_t PADDED (aio_key,  aio_raservedl)j
//�����룬��ȡֵ��Χ��io iocb cmd t�е�ö������
u int16_t aio lio_opcode;
//��������ȼ�
int16 t aio_reqprio,
//�ļ�������
u int32 t aio fildes;
//����д������Ӧ���û�̬������
u int64 t aio buf;
//����д�������ֽڳ���
u int64 t aio_nbytes;
//����д������Ӧ���ļ��е�ƫ����
int64 t aio offset;
//�����ֶ�
u int64_t aio reserved2;
//��ʾ��������ΪIOCB FLAG RESFD����������ں˵����첽I/O���������ʱʹ��eventfd����֪ͨ������epoll���ʹ�ã�����Nginx�е�ʹ�÷����ɲμ�9.9.2��+��
    u int32_t aio_flags��
//��ʾ��ʹ��IOCB FLAG RESFD��־λʱ�����ڽ����¼�֪ͨ�ľ��
U int32 t aio resfd;
}
    ��ˣ������ú�iocb�ṹ��󣬾Ϳ������첽I/O�ύ�¼��ˡ�aio_lio_opcode������
ָ��������¼��Ĳ������ͣ�����ȡֵ��Χ���¡�
    typedef enum io_iocb_cmd{
    �����첽������
    IO_CMD_PREAD=O��
    �����첽д����
    IO_CMD_PWRITE=1��
    ����ǿ��ͬ��
    IO_CMD_FSYNC=2��
    ����Ŀǰ��ʹ��
    IO_CMD_FDSYNC=3��
    ����Ŀǰδʹ��
    IO_CMD_POLL=5��
    ���������κ�����
    IO_CMD_NOOP=6��
    )  io_iocb_cmd_t;

*/
#if (NGX_HAVE_FILE_AIO)  //  --with-file-aio)                 NGX_FILE_AIO=YES           ;;
#include <aio.h>
typedef struct aiocb  ngx_aiocb_t;
#endif


#define NGX_LISTEN_BACKLOG  511

#define ngx_debug_init()


#if (__FreeBSD__) && (__FreeBSD_version < 400017)

#include <sys/param.h>          /* ALIGN() */

/*
 * FreeBSD 3.x has no CMSG_SPACE() and CMSG_LEN() and has the broken CMSG_DATA()
 */

#undef  CMSG_SPACE
#define CMSG_SPACE(l)       (ALIGN(sizeof(struct cmsghdr)) + ALIGN(l))

#undef  CMSG_LEN
#define CMSG_LEN(l)         (ALIGN(sizeof(struct cmsghdr)) + (l))

#undef  CMSG_DATA
#define CMSG_DATA(cmsg)     ((u_char *)(cmsg) + ALIGN(sizeof(struct cmsghdr)))

#endif


extern char **environ;


#endif /* _NGX_POSIX_CONFIG_H_INCLUDED_ */
