
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_FILES_H_INCLUDED_
#define _NGX_FILES_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef int                      ngx_fd_t;


typedef struct stat              ngx_file_info_t;//�ļ���С����Դ��Ϣ��ʵ�ʾ���Linuxϵͳ�����stat�ṹ
typedef ino_t                    ngx_file_uniq_t;


typedef struct {
    u_char                      *name;
    size_t                       size;
    void                        *addr;
    ngx_fd_t                     fd;
    ngx_log_t                   *log;
} ngx_file_mapping_t;


typedef struct {
    DIR                         *dir;
    struct dirent               *de;
    struct stat                  info;

    unsigned                     type:8;
    unsigned                     valid_info:1;
} ngx_dir_t;


typedef struct {
    size_t                       n;
    glob_t                       pglob;
    u_char                      *pattern;
    ngx_log_t                   *log;
    ngx_uint_t                   test;
} ngx_glob_t;


#define NGX_INVALID_FILE         -1
#define NGX_FILE_ERROR           -1



#ifdef __CYGWIN__

#ifndef NGX_HAVE_CASELESS_FILESYSTEM
#define NGX_HAVE_CASELESS_FILESYSTEM  1
#endif

/*
ʵ���ϣ�ngx_open_file��open���������𲻴�ngx_open_file���ص���Linuxϵͳ���ļ���������ڴ��ļ��ı�־λ��NginxҲ���������¼����������Է�װ��
#define NGX_FILE_RDONLY O_RDONLY
#define NGX_FILE_WRONLY O_WRONLY
#define NGX_FILE_RDWR O_RDWR
#define NGX_FILE_CREATE_OR_OPEN O_CREAT
#define NGX_FILE_OPEN 0
#define NGX_FILE_TRUNCATE O_CREAT|O_TRUNC
#define NGX_FILE_APPEND O_WRONLY|O_APPEND
#define NGX_FILE_NONBLOCK O_NONBLOCK

#define NGX_FILE_DEFAULT_ACCESS 0644
#define NGX_FILE_OWNER_ACCESS 0600
*/
#define ngx_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create|O_BINARY, access)

#else

#define ngx_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)

#endif

#define ngx_open_file_n          "open()"

#define NGX_FILE_RDONLY          O_RDONLY
#define NGX_FILE_WRONLY          O_WRONLY
#define NGX_FILE_RDWR            O_RDWR
#define NGX_FILE_CREATE_OR_OPEN  O_CREAT
#define NGX_FILE_OPEN            0
#define NGX_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define NGX_FILE_APPEND          (O_WRONLY|O_APPEND)
#define NGX_FILE_NONBLOCK        O_NONBLOCK

#if (NGX_HAVE_OPENAT)
#define NGX_FILE_NOFOLLOW        O_NOFOLLOW

#if defined(O_DIRECTORY)
#define NGX_FILE_DIRECTORY       O_DIRECTORY
#else
#define NGX_FILE_DIRECTORY       0
#endif

#if defined(O_SEARCH)
#define NGX_FILE_SEARCH          (O_SEARCH|NGX_FILE_DIRECTORY)

#elif defined(O_EXEC)
#define NGX_FILE_SEARCH          (O_EXEC|NGX_FILE_DIRECTORY)

#elif (NGX_HAVE_O_PATH)
#define NGX_FILE_SEARCH          (O_PATH|O_RDONLY|NGX_FILE_DIRECTORY)

#else
#define NGX_FILE_SEARCH          (O_RDONLY|NGX_FILE_DIRECTORY)
#endif

#endif /* NGX_HAVE_OPENAT */

/*
    -rw-rw-r--
����һ����10λ��
�������У� ��ǰ���Ǹ� - �����������
�����м������� rw- ������������ߣ�user��
����Ȼ�������� rw- ���������Ⱥ��group��
������������� r-- ������������ˣ�other��
*/
#define NGX_FILE_DEFAULT_ACCESS  0644 //-rw-r--r--  �������ж�дȨ�� ��Ⱥ�ж�дȨ�ޣ�������ֻ�ж�Ȩ��
#define NGX_FILE_OWNER_ACCESS    0600 //-rw-------


#define ngx_close_file           close
#define ngx_close_file_n         "close()"

//unlink ֻ��ɾ���ļ�������ɾ��Ŀ¼
#define ngx_delete_file(name)    unlink((const char *) name)
#define ngx_delete_file_n        "unlink()"


ngx_fd_t ngx_open_tempfile(u_char *name, ngx_uint_t persistent,
    ngx_uint_t access);
#define ngx_open_tempfile_n      "open()"


ssize_t ngx_read_file(ngx_file_t *file, u_char *buf, size_t size, off_t offset);
#if (NGX_HAVE_PREAD)
#define ngx_read_file_n          "pread()"
#else
#define ngx_read_file_n          "read()"
#endif

ssize_t ngx_write_file(ngx_file_t *file, u_char *buf, size_t size,
    off_t offset);

ssize_t ngx_write_chain_to_file(ngx_file_t *file, ngx_chain_t *ce,
    off_t offset, ngx_pool_t *pool);


#define ngx_read_fd              read
#define ngx_read_fd_n            "read()"

/*
 * we use inlined function instead of simple #define
 * because glibc 2.3 sets warn_unused_result attribute for write()
 * and in this case gcc 4.3 ignores (void) cast
 */
static ngx_inline ssize_t
ngx_write_fd(ngx_fd_t fd, void *buf, size_t n)
{
    return write(fd, buf, n);
}

#define ngx_write_fd_n           "write()"


#define ngx_write_console        ngx_write_fd


#define ngx_linefeed(p)          *p++ = LF;
#define NGX_LINEFEED_SIZE        1
#define NGX_LINEFEED             "\x0a"

/* rename���������Ǹ�һ���ļ����������øú�������ʵ���ļ��ƶ����ܣ���һ���ļ�������·�����̷���һ�¾�ʵ��������ļ����ƶ� */
//rename��mv�����ֻ࣬��rename֧�������޸�  Ҳ����˵��mvҲ�����ڸ�����������ʵ��������������ʱ����֧��*�ȷ��ŵģ�����rename���ԡ�
//rename���ļ���fd��stat��Ϣ����  ʹ�� RENAME ��ԭ���Եض���ʱ�ļ����и���������ԭ���� RDB �ļ���
#define ngx_rename_file(o, n)    rename((const char *) o, (const char *) n)
#define ngx_rename_file_n        "rename()"


#define ngx_change_file_access(n, a) chmod((const char *) n, a)
#define ngx_change_file_access_n "chmod()"


ngx_int_t ngx_set_file_time(u_char *name, ngx_fd_t fd, time_t s);
#define ngx_set_file_time_n      "utimes()"

/*

Linux stat�������⣺

��ͷ�ļ�:    #include <sys/stat.h>
                      #include <unistd.h>
���庯��:    int stat(const char *file_name, struct stat *buf);

����˵��:    ͨ���ļ���filename��ȡ�ļ���Ϣ����������buf��ָ�Ľṹ��stat��
 ����ֵ:     ִ�гɹ��򷵻�0��ʧ�ܷ���-1������������errno
�������:
     ENOENT         ����file_nameָ�����ļ�������
    ENOTDIR        ·���е�Ŀ¼���ڵ�ȴ��������Ŀ¼
    ELOOP          ���򿪵��ļ��й�������������⣬����Ϊ16��������
    EFAULT         ����bufΪ��Чָ�룬ָ���޷����ڵ��ڴ�ռ�
    EACCESS        ��ȡ�ļ�ʱ���ܾ�
    ENOMEM         �����ڴ治��
    ENAMETOOLONG   ����file_name��·������̫��
#include <sys/stat.h>
 #include <unistd.h>
 #include <stdio.h>
 int main() {
     struct stat buf;
     stat("/etc/hosts", &buf);
     printf("/etc/hosts file size = %d\n", buf.st_size);
 }
 -----------------------------------------------------
 struct stat {
     dev_t         st_dev;       //�ļ����豸���
    ino_t         st_ino;       //�ڵ�
    mode_t        st_mode;      //�ļ������ͺʹ�ȡ��Ȩ��
    nlink_t       st_nlink;     //�������ļ���Ӳ������Ŀ���ս������ļ�ֵΪ1
     uid_t         st_uid;       //�û�ID
     gid_t         st_gid;       //��ID
     dev_t         st_rdev;      //(�豸����)�����ļ�Ϊ�豸�ļ�����Ϊ���豸���
    off_t         st_size;      //�ļ��ֽ���(�ļ���С)
     unsigned long st_blksize;   //���С(�ļ�ϵͳ��I/O ��������С)
     unsigned long st_blocks;    //����
    time_t        st_atime;     //���һ�η���ʱ��
    time_t        st_mtime;     //���һ���޸�ʱ��
    time_t        st_ctime;     //���һ�θı�ʱ��(ָ����)
 };
��ǰ��������st_mode �������������������
    S_IFMT   0170000    �ļ����͵�λ����
    S_IFSOCK 0140000    scoket
     S_IFLNK 0120000     ��������
    S_IFREG 0100000     һ���ļ�
    S_IFBLK 0060000     ����װ��
    S_IFDIR 0040000     Ŀ¼
    S_IFCHR 0020000     �ַ�װ��
    S_IFIFO 0010000     �Ƚ��ȳ�
    S_ISUID 04000     �ļ���(set user-id on execution)λ
    S_ISGID 02000     �ļ���(set group-id on execution)λ
    S_ISVTX 01000     �ļ���stickyλ
    S_IRUSR(S_IREAD) 00400     �ļ������߾߿ɶ�ȡȨ��
    S_IWUSR(S_IWRITE)00200     �ļ������߾߿�д��Ȩ��
    S_IXUSR(S_IEXEC) 00100     �ļ������߾߿�ִ��Ȩ��
    S_IRGRP 00040             �û���߿ɶ�ȡȨ��
    S_IWGRP 00020             �û���߿�д��Ȩ��
    S_IXGRP 00010             �û���߿�ִ��Ȩ��
    S_IROTH 00004             �����û��߿ɶ�ȡȨ��
    S_IWOTH 00002             �����û��߿�д��Ȩ��
    S_IXOTH 00001             �����û��߿�ִ��Ȩ��
    �������ļ�������POSIX�ж����˼����Щ���͵ĺ궨�壺
    S_ISLNK (st_mode)    �ж��Ƿ�Ϊ��������
    S_ISREG (st_mode)    �Ƿ�Ϊһ���ļ�
    S_ISDIR (st_mode)    �Ƿ�ΪĿ¼
    S_ISCHR (st_mode)    �Ƿ�Ϊ�ַ�װ���ļ�
    S_ISBLK (s3e)        �Ƿ�Ϊ�Ƚ��ȳ�
    S_ISSOCK (st_mode)   �Ƿ�Ϊsocket
     ��һĿ¼����stickyλ(S_ISVTX)�����ʾ�ڴ�Ŀ¼�µ��ļ�ֻ�ܱ����ļ������ߡ���Ŀ¼�����߻�root��ɾ���������

ʹ��stat�������Ŀ�����ls-l���������Ի���й�һ���ļ���������Ϣ��
1 �������ǻ�ȡ�ļ�����ͨ�ļ���Ŀ¼���ܵ���socket���ַ����飨�������ԡ�
����ԭ��
#include <sys/stat.h>
int stat(const char *restrict pathname, struct stat *restrict buf);
�ṩ�ļ����֣���ȡ�ļ���Ӧ���ԡ�
int fstat(int filedes, struct stat *buf);
ͨ���ļ���������ȡ�ļ���Ӧ�����ԡ�
int lstat(const char *restrict pathname, struct stat *restrict buf);
�����ļ�����������ȡ�ļ����ԡ�







statϵͳ����ϵ�а�����fstat��stat��lstat�����Ƕ����������ء�����ļ�״̬��Ϣ���ģ����ߵĲ�֮ͬ�������趨Դ�ļ��ķ�ʽ��ͬ��

�����Ѿ�ѧϰ����struct stat�͸���st_mode��غ꣬���ھͿ��������Ǻ�statϵͳ�����໥��Ϲ����ˣ�

int fstat(int filedes, struct stat *buf);
int stat(const char *path, struct stat *buf);
int lstat(const char *path, struct stat *buf);
������һ�۾��ܿ�����fstat�ĵ�һ�������Ǻ�����������һ���ģ��ԣ�fstat��������������ϵͳ���õĵط����ڣ�fstatϵͳ���ý��ܵ��� һ�����ļ�����������
������������ֱ�ӽ��ܡ��ļ�ȫ·�������ļ�����������Ҫ������openϵͳ���ú���ܵõ��ģ����ļ�ȫ·��ֱ��д�Ϳ����ˡ�

stat��lstat�����𣺵��ļ���һ����������ʱ��lstat���ص��Ǹ÷������ӱ������Ϣ����stat���ص��Ǹ�����ָ����ļ�����Ϣ�����ƺ���Щ�ΰɣ���
���ǣ�lstat��stat����һ��l����������б��´�����������ļ��ģ���˵��������������ļ�ʱ��lstat��Ȼ����Ź����� statϵͳ����û��������£�
��ֻ�ܶԷ��������ļ���һֻ�۱�һֻ�ۣ�ֱ��ȥ����������ָ�ļ�ඣ� 
*/

#define ngx_file_info(file, sb)  stat((const char *) file, sb)
#define ngx_file_info_n          "stat()"

//fstat()����������fildes��ָ���ļ�״̬�����Ƶ�����buf��ָ��
#define ngx_fd_info(fd, sb)      fstat(fd, sb) //���ء�����ļ�״̬��Ϣ����
#define ngx_fd_info_n            "fstat()"

#define ngx_link_info(file, sb)  lstat((const char *) file, sb)
#define ngx_link_info_n          "lstat()"

/*
struct stat  
10.{  
11.  
12.    dev_t       st_dev;     / * ID of device containing file -�ļ������豸��ID* /  
13.  
14.    ino_t       st_ino;     / * inode number -inode�ڵ��* /  
15.  
16.    mode_t      st_mode;    / * protection -����ģʽ?* /  
17.  
18.    nlink_t     st_nlink;   / * number of hard links -������ļ���������(Ӳ����)* /  
19.  
20.    uid_t       st_uid;     / * user ID of owner -user id* /  
21.  
22.    gid_t       st_gid;     / * group ID of owner - group id* /  
23.  
24.    dev_t       st_rdev;    / * device ID (if special file) -�豸�ţ�����豸�ļ�* /  
25.  
26.    off_t       st_size;    / * total size, in bytes -�ļ���С���ֽ�Ϊ��λ* /  
27.  
28.    blksize_t   st_blksize; / * blocksize for filesystem I/O -ϵͳ��Ĵ�С* /  
29.  
30.    blkcnt_t    st_blocks;  / * number of blocks allocated -�ļ���ռ����* /  
31.  
32.    time_t      st_atime;   / * time of last access -�����ȡʱ��* /  
33.  
34.    time_t      st_mtime;   / * time of last modification -����޸�ʱ��* /  
35.  
36.    time_t      st_ctime;   / * time of last status change - * /  
37.  
38.};  
*/
#define ngx_is_dir(sb)           (S_ISDIR((sb)->st_mode))
#define ngx_is_file(sb)          (S_ISREG((sb)->st_mode))
#define ngx_is_link(sb)          (S_ISLNK((sb)->st_mode))
#define ngx_is_exec(sb)          (((sb)->st_mode & S_IXUSR) == S_IXUSR)
#define ngx_file_access(sb)      ((sb)->st_mode & 0777)
#define ngx_file_size(sb)        (sb)->st_size
#define ngx_file_fs_size(sb)     ngx_max((sb)->st_size, (sb)->st_blocks * 512)
#define ngx_file_mtime(sb)       (sb)->st_mtime
/* inode number -inode�ڵ��*/  
//ͬһ���豸�е�ÿ���ļ������ֵ���ǲ�ͬ��
#define ngx_file_uniq(sb)        (sb)->st_ino  


ngx_int_t ngx_create_file_mapping(ngx_file_mapping_t *fm);
void ngx_close_file_mapping(ngx_file_mapping_t *fm);


#define ngx_realpath(p, r)       (u_char *) realpath((char *) p, (char *) r)
#define ngx_realpath_n           "realpath()"

//getcwd()�Ὣ��ǰ����Ŀ¼�ľ���·�����Ƶ�����buf��ָ���ڴ�ռ���,����sizeΪbuf�Ŀռ��С
#define ngx_getcwd(buf, size)    (getcwd((char *) buf, size) != NULL)
#define ngx_getcwd_n             "getcwd()"
#define ngx_path_separator(c)    ((c) == '/')


#if defined(PATH_MAX)

#define NGX_HAVE_MAX_PATH        1
#define NGX_MAX_PATH             PATH_MAX

#else

#define NGX_MAX_PATH             4096

#endif


#define NGX_DIR_MASK_LEN         0


ngx_int_t ngx_open_dir(ngx_str_t *name, ngx_dir_t *dir);
#define ngx_open_dir_n           "opendir()"


#define ngx_close_dir(d)         closedir((d)->dir)
#define ngx_close_dir_n          "closedir()"


ngx_int_t ngx_read_dir(ngx_dir_t *dir);
#define ngx_read_dir_n           "readdir()"


#define ngx_create_dir(name, access) mkdir((const char *) name, access)
#define ngx_create_dir_n         "mkdir()"


#define ngx_delete_dir(name)     rmdir((const char *) name)
#define ngx_delete_dir_n         "rmdir()"


#define ngx_dir_access(a)        (a | (a & 0444) >> 2)


#define ngx_de_name(dir)         ((u_char *) (dir)->de->d_name)
#if (NGX_HAVE_D_NAMLEN)
#define ngx_de_namelen(dir)      (dir)->de->d_namlen
#else
#define ngx_de_namelen(dir)      ngx_strlen((dir)->de->d_name)
#endif

static ngx_inline ngx_int_t
ngx_de_info(u_char *name, ngx_dir_t *dir)
{
    dir->type = 0;
    return stat((const char *) name, &dir->info);
}

#define ngx_de_info_n            "stat()"
#define ngx_de_link_info(name, dir)  lstat((const char *) name, &(dir)->info)
#define ngx_de_link_info_n       "lstat()"

#if (NGX_HAVE_D_TYPE)

/*
 * some file systems (e.g. XFS on Linux and CD9660 on FreeBSD)
 * do not set dirent.d_type
 */

#define ngx_de_is_dir(dir)                                                   \
    (((dir)->type) ? ((dir)->type == DT_DIR) : (S_ISDIR((dir)->info.st_mode)))
#define ngx_de_is_file(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_REG) : (S_ISREG((dir)->info.st_mode)))
#define ngx_de_is_link(dir)                                                  \
    (((dir)->type) ? ((dir)->type == DT_LNK) : (S_ISLNK((dir)->info.st_mode)))

#else

#define ngx_de_is_dir(dir)       (S_ISDIR((dir)->info.st_mode))
#define ngx_de_is_file(dir)      (S_ISREG((dir)->info.st_mode))
#define ngx_de_is_link(dir)      (S_ISLNK((dir)->info.st_mode))

#endif

#define ngx_de_access(dir)       (((dir)->info.st_mode) & 0777)
#define ngx_de_size(dir)         (dir)->info.st_size
#define ngx_de_fs_size(dir)                                                  \
    ngx_max((dir)->info.st_size, (dir)->info.st_blocks * 512)
#define ngx_de_mtime(dir)        (dir)->info.st_mtime


ngx_int_t ngx_open_glob(ngx_glob_t *gl);
#define ngx_open_glob_n          "glob()"
ngx_int_t ngx_read_glob(ngx_glob_t *gl, ngx_str_t *name);
void ngx_close_glob(ngx_glob_t *gl);


ngx_err_t ngx_trylock_fd(ngx_fd_t fd);
ngx_err_t ngx_lock_fd(ngx_fd_t fd);
ngx_err_t ngx_unlock_fd(ngx_fd_t fd);

#define ngx_trylock_fd_n         "fcntl(F_SETLK, F_WRLCK)"
#define ngx_lock_fd_n            "fcntl(F_SETLKW, F_WRLCK)"
#define ngx_unlock_fd_n          "fcntl(F_SETLK, F_UNLCK)"


#if (NGX_HAVE_F_READAHEAD)

#define NGX_HAVE_READ_AHEAD      1

#define ngx_read_ahead(fd, n)    fcntl(fd, F_READAHEAD, (int) n)
#define ngx_read_ahead_n         "fcntl(fd, F_READAHEAD)"

#elif (NGX_HAVE_POSIX_FADVISE)

#define NGX_HAVE_READ_AHEAD      1

ngx_int_t ngx_read_ahead(ngx_fd_t fd, size_t n);
#define ngx_read_ahead_n         "posix_fadvise(POSIX_FADV_SEQUENTIAL)"

#else

#define ngx_read_ahead(fd, n)    0
#define ngx_read_ahead_n         "ngx_read_ahead_n"

#endif


#if (NGX_HAVE_O_DIRECT)

ngx_int_t ngx_directio_on(ngx_fd_t fd);
#define ngx_directio_on_n        "fcntl(O_DIRECT)"

ngx_int_t ngx_directio_off(ngx_fd_t fd);
#define ngx_directio_off_n       "fcntl(!O_DIRECT)"

#elif (NGX_HAVE_F_NOCACHE)

#define ngx_directio_on(fd)      fcntl(fd, F_NOCACHE, 1)
#define ngx_directio_on_n        "fcntl(F_NOCACHE, 1)"

#elif (NGX_HAVE_DIRECTIO)

#define ngx_directio_on(fd)      directio(fd, DIRECTIO_ON)
#define ngx_directio_on_n        "directio(DIRECTIO_ON)"

#else

#define ngx_directio_on(fd)      0
#define ngx_directio_on_n        "ngx_directio_on_n"

#endif

size_t ngx_fs_bsize(u_char *name);


#if (NGX_HAVE_OPENAT)

#define ngx_openat_file(fd, name, mode, create, access)                      \
    openat(fd, (const char *) name, mode|create, access)

#define ngx_openat_file_n        "openat()"

#define ngx_file_at_info(fd, name, sb, flag)                                 \
    fstatat(fd, (const char *) name, sb, flag)

#define ngx_file_at_info_n       "fstatat()"

#define NGX_AT_FDCWD             (ngx_fd_t) AT_FDCWD

#endif


#define ngx_stdout               STDOUT_FILENO
#define ngx_stderr               STDERR_FILENO
#define ngx_set_stderr(fd)       dup2(fd, STDERR_FILENO) //dup2��dup������������һ���ִ�� �ļ���������ʹ�����ļ�������ָ��ͬһ��file �ṹ�塣
#define ngx_set_stderr_n         "dup2(STDERR_FILENO)"


#if (NGX_HAVE_FILE_AIO)

ngx_int_t ngx_file_aio_init(ngx_file_t *file, ngx_pool_t *pool);
ssize_t ngx_file_aio_read(ngx_file_t *file, u_char *buf, size_t size,
    off_t offset, ngx_pool_t *pool);

extern ngx_uint_t  ngx_file_aio;

#endif

#if (NGX_THREADS)
ssize_t ngx_thread_read(ngx_thread_task_t **taskp, ngx_file_t *file,
    u_char *buf, size_t size, off_t offset, ngx_pool_t *pool);
#endif


#endif /* _NGX_FILES_H_INCLUDED_ */
