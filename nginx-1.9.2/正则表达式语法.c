/*
�����﷨:
����������Ԫ�ַ�����Ļ������������.,����*,�ͳ��������⣺��û�취ָ�����ǣ���Ϊ���ǻᱻ���ͳɱ����˼����ʱ��͵�ʹ��\��ȡ����Щ�ַ����������塣��ˣ���Ӧ��ʹ��\.��\*����Ȼ��Ҫ����\������Ҳ����\\.


.     ƥ������з�����������ַ�
\w     ƥ����ĸ�����ֻ��»��߻���
\s     ƥ������Ŀհ׷� ����Ŀհ׷��������ո��Ʊ��(Tab)�����з�������ȫ�ǿո�ȡ�
\d     ƥ������      ��:\d+ƥ��1�����������������
\b     ƥ�䵥�ʵĿ�ʼ�����
^     ƥ���ַ����Ŀ�ʼ
$     ƥ���ַ����Ľ���

*     �ظ���λ�����    ��:Ȼ����������������ĸ������(\w*)   +�Ǻ�*���Ƶ�Ԫ�ַ�����ͬ����*ƥ���ظ������(������0��)����+��ƥ���ظ�1�λ����Ρ�

+     �ظ�һ�λ�����    ��:\d+ƥ��1�����������������  +�Ǻ�*���Ƶ�Ԫ�ַ�����ͬ����*ƥ���ظ������(������0��)����+��ƥ���ظ�1�λ����Ρ�

?     �ظ���λ�һ��
{n}     �ظ�n��
{n,}     �ظ�n�λ�����
{n,m}     �ظ�n��m��
*?     �ظ�����Σ������������ظ�
+?     �ظ�1�λ����Σ������������ظ�
??     �ظ�0�λ�1�Σ������������ظ�
{n,m}?     �ظ�n��m�Σ������������ظ�
{n,}?     �ظ�n�����ϣ������������ظ�

\W     ƥ�����ⲻ����ĸ�����֣��»��ߣ����ֵ��ַ�
\S     ƥ�����ⲻ�ǿհ׷����ַ�
\D     ƥ����������ֵ��ַ�
\B     ƥ�䲻�ǵ��ʿ�ͷ�������λ��
[^x]     ƥ�����x����������ַ�
[^aeiou]     ƥ�����aeiou�⼸����ĸ����������ַ�

����     (exp)     ƥ��exp,�������ı����Զ�����������
(?<name>exp)     ƥ��exp,�������ı�������Ϊname�����Ҳ����д��(?'name'exp)
(?:exp)     ƥ��exp,������ƥ����ı���Ҳ�����˷���������
������     (?=exp)     ƥ��expǰ���λ��
(?<=exp)     ƥ��exp�����λ��
(?!exp)     ƥ�������Ĳ���exp��λ��
(?<!exp)     ƥ��ǰ�治��exp��λ��
ע��     (?#comment)     �������͵ķ��鲻��������ʽ�Ĵ�������κ�Ӱ�죬�����ṩע�������Ķ�










PCRE��������ʹ��ʾ�� .
���ࣺ C/C++ 2011-03-13 23:56 15411���Ķ� ����(6) �ղ� �ٱ� 
������ʽlistbuffercompilationnullperlPCRE��һ��NFA�������棬��Ȼ�����ṩ��ȫ��Perlһ�µ������﷨���ܡ�
����ͬʱҲʵ����DFA��ֻ��������ѧ�����ϵ�����

 

PCRE�ṩ��19���ӿں�����Ϊ�˼򵥽��ܣ�ʹ��PCRE�ڴ��Ĳ��Գ���(pcretest.c)ʾ���÷���

1. pcre_compile

       ԭ�ͣ�

         #include <pcre.h> 

pcre *pcre_compile(const char *pattern, int options, const char **errptr, int *erroffset, const unsigned char *tableptr); 

���ܣ���һ��������ʽ�����һ���ڲ���ʾ����ƥ�����ַ���ʱ�����Լ���ƥ�䡣��ͬpcre_compile2����һ��ֻ��ȱ��һ������errorcodeptr��

������

pattern    ������ʽ

options     Ϊ0��������������ѡ��

       errptr       ������Ϣ

        erroffset  ����λ��

tableptr   ָ��һ���ַ������ָ�룬��������Ϊ��NULL

ʾ����

L1720     re = pcre_compile((char *)p, options, &error, &erroroffset, tables);

 

2. pcre_compile2

       ԭ�ͣ�

#include <pcre.h> 

pcre *pcre_compile2(const char *pattern, int options, int *errorcodeptr, const char **errptr, int *erroffset, const unsigned char *tableptr); 

���ܣ���һ��������ʽ�����һ���ڲ���ʾ����ƥ�����ַ���ʱ�����Լ���ƥ�䡣��ͬpcre_compile����һ��ֻ�Ƕ�һ������errorcodeptr��

������

pattern    ������ʽ

options     Ϊ0��������������ѡ��

errorcodeptr    ��ų�����

       errptr       ������Ϣ

        erroffset  ����λ��

tableptr   ָ��һ���ַ������ָ�룬��������Ϊ��NULL

 

3. pcre_config

       ԭ�ͣ�

#include <pcre.h> 

int pcre_config(int what, void *where);

���ܣ���ѯ��ǰPCRE�汾��ʹ�õ�ѡ����Ϣ��

������

what         ѡ����

where       �洢�����λ��

ʾ����

Line1312 (void)pcre_config(PCRE_CONFIG_POSIX_MALLOC_THRESHOLD, &rc);

 

4. pcre_copy_named_substring

       ԭ�ͣ�

#include <pcre.h> 

int pcre_copy_named_substring(const pcre *code, const char *subject, int *ovector, int stringcount, const char *stringname, 
char *buffer, int buffersize); 

���ܣ��������ֻ�ȡ������ִ���

������

code                            �ɹ�ƥ���ģʽ

subject               ƥ��Ĵ�

ovector              pcre_exec() ʹ�õ�ƫ������

stringcount   pcre_exec()�ķ���ֵ

stringname       �����ִ�������

buffer                 �����洢�Ļ�����

buffersize                   ��������С

ʾ����

Line2730 int rc = pcre_copy_named_substring(re, (char *)bptr, use_offsets,

            count, (char *)copynamesptr, copybuffer, sizeof(copybuffer));

 

5. pcre_copy_substring
       ԭ�ͣ�

#include <pcre.h> 

int pcre_copy_substring(const char *subject, int *ovector, int stringcount, int stringnumber, char *buffer, int buffersize); 

���ܣ����ݱ�Ż�ȡ������ִ���

������

code                            �ɹ�ƥ���ģʽ

subject               ƥ��Ĵ�

ovector              pcre_exec() ʹ�õ�ƫ������

stringcount   pcre_exec()�ķ���ֵ

stringnumber   �����ִ����

buffer                 �����洢�Ļ�����

buffersize                   ��������С

ʾ����

Line2730 int rc = pcre_copy_substring((char *)bptr, use_offsets, count,

              i, copybuffer, sizeof(copybuffer));

 

6. pcre_dfa_exec

       ԭ�ͣ�

#include <pcre.h> 

int pcre_dfa_exec(const pcre *code, const pcre_extra *extra, const char *subject, int length, int startoffset, int options, 
int *ovector, int ovecsize, int *workspace, int wscount);

���ܣ�ʹ�ñ���õ�ģʽ����ƥ�䣬���õ���һ�ַǴ�ͳ�ķ���DFA��ֻ�Ƕ�ƥ�䴮ɨ��һ�Σ���Perl�����ݣ���

������

code                   ����õ�ģʽ

extra         ָ��һ��pcre_extra�ṹ�壬����ΪNULL

subject    ��Ҫƥ����ַ���

length       ƥ����ַ������ȣ�Byte��

startoffset        ƥ��Ŀ�ʼλ��

options     ѡ��λ

ovector    ָ��һ���������������

ovecsize   �����С

workspace        һ������������

wscount   �����С

ʾ����

Line2730 count = pcre_dfa_exec(re, extra, (char *)bptr, len, start_offset,

              options | g_notempty, use_offsets, use_size_offsets, workspace,

              sizeof(workspace)/sizeof(int));

 

7. pcre_copy_substring
       ԭ�ͣ�

#include <pcre.h> 

int pcre_exec(const pcre *code, const pcre_extra *extra, const char *subject, int length, int startoffset, int options, 
int *ovector, int ovecsize);

���ܣ�ʹ�ñ���õ�ģʽ����ƥ�䣬������Perl���Ƶ��㷨������ƥ�䴮��ƫ��λ�á���

������

code                   ����õ�ģʽ

extra         ָ��һ��pcre_extra�ṹ�壬����ΪNULL

subject    ��Ҫƥ����ַ���

length       ƥ����ַ������ȣ�Byte��

startoffset        ƥ��Ŀ�ʼλ��

options     ѡ��λ

ovector    ָ��һ���������������

ovecsize   �����С

 

8. pcre_free_substring
       ԭ�ͣ�

#include <pcre.h> 

void pcre_free_substring(const char *stringptr);

���ܣ��ͷ�pcre_get_substring()��pcre_get_named_substring()������ڴ�ռ䡣

������

stringptr            ָ���ַ�����ָ��

ʾ����

Line2730        const char *substring;

int rc = pcre_get_substring((char *)bptr, use_offsets, count,

              i, &substring);

����

pcre_free_substring(substring);

 

9. pcre_free_substring_list
       ԭ�ͣ�

#include <pcre.h> 

void pcre_free_substring_list(const char **stringptr);

���ܣ��ͷ���pcre_get_substring_list������ڴ�ռ䡣

������

stringptr            ָ���ַ��������ָ��

ʾ����

Line2773        const char **stringlist;

int rc = pcre_get_substring_list((char *)bptr, use_offsets, count,

����

pcre_free_substring_list(stringlist);

 

10. pcre_fullinfo
       ԭ�ͣ�

#include <pcre.h> 

int pcre_fullinfo(const pcre *code, const pcre_extra *extra, int what, void *where);

���ܣ����ر��������ģʽ����Ϣ��

������

code          ����õ�ģʽ

extra         pcre_study()�ķ���ֵ������NULL
what         ʲô��Ϣ
where       �洢λ��
ʾ����

Line997          if ((rc = pcre_fullinfo(re, study, option, ptr)) < 0)

fprintf(outfile, "Error %d from pcre_fullinfo(%d)/n", rc, option);

}

 

11. pcre_get_named_substring
       ԭ�ͣ�

#include <pcre.h> 

int pcre_get_named_substring(const pcre *code, const char *subject, int *ovector, int stringcount, const char *stringname, const char **stringptr);

���ܣ����ݱ�Ż�ȡ������ִ���

������

code                            �ɹ�ƥ���ģʽ

subject               ƥ��Ĵ�

ovector              pcre_exec() ʹ�õ�ƫ������

stringcount   pcre_exec()�ķ���ֵ

stringname       �����ִ�������

stringptr     ��Ž�����ַ���ָ��
ʾ����

Line2759        const char *substring;

int rc = pcre_get_named_substring(re, (char *)bptr, use_offsets,

            count, (char *)getnamesptr, &substring);

 

12. pcre_get_stringnumber
       ԭ�ͣ�

#include <pcre.h> 

int pcre_get_stringnumber(const pcre *code, const char *name);

���ܣ�����������������ֻ�ȡ��Ӧ�ı�š�

������

code                            �ɹ�ƥ���ģʽ

name                 ��������

 

13. pcre_get_substring
       ԭ�ͣ�

#include <pcre.h> 

int pcre_get_substring(const char *subject, int *ovector, int stringcount, int stringnumber, const char **stringptr);

���ܣ���ȡƥ����Ӵ���

������

subject       �ɹ�ƥ��Ĵ�
ovector       pcre_exec() ʹ�õ�ƫ������

stringcount    pcre_exec()�ķ���ֵ
stringnumber  ��ȡ���ַ������
stringptr      �ַ���ָ��
 

14. pcre_get_substring_list
       ԭ�ͣ�

#include <pcre.h> 

int pcre_get_substring_list(const char *subject, int *ovector, int stringcount, const char ***listptr);

���ܣ���ȡƥ��������Ӵ���

������

subject       �ɹ�ƥ��Ĵ�
ovector       pcre_exec() ʹ�õ�ƫ������

stringcount    pcre_exec()�ķ���ֵ
listptr             �ַ����б��ָ��
 

15. pcre_info

       ԭ�ͣ�

#include <pcre.h> 

int pcre_info(const pcre *code, int *optptr, int *firstcharptr);

�ѹ�ʱ��ʹ��pcre_fullinfo�����
 

16. pcre_maketables
       ԭ�ͣ�

#include <pcre.h> 

const unsigned char *pcre_maketables(void);

���ܣ�����һ���ַ�������ÿһ��Ԫ�ص�ֵ������256��������������pcre_compile()�滻���ڽ����ַ���

������

ʾ����

Line2759 tables = pcre_maketables();

 

17. pcre_refcount
       ԭ�ͣ�

#include <pcre.h> 

int pcre_refcount(pcre *code, int adjust);

���ܣ�����ģʽ�����ü�����

������

code       �ѱ����ģʽ
adjust      ���������ü���ֵ

 

18. pcre_study
       ԭ�ͣ�

#include <pcre.h> 

pcre_extra *pcre_study(const pcre *code, int options, const char **errptr);

���ܣ��Ա����ģʽ����ѧϰ����ȡ���Լ���ƥ����̵���Ϣ��

������

code      �ѱ����ģʽ
options    ѡ��
errptr     ������Ϣ
ʾ����

Line1797 extra = pcre_study(re, study_options, &error);

 

19. pcre_version
       ԭ�ͣ�

#include <pcre.h> 

char *pcre_version(void);

���ܣ�����PCRE�İ汾��Ϣ��

������

ʾ����

Line1384 if (!quiet) fprintf(outfile, "PCRE version %s/n/n", pcre_version());

 

����ʵ����

[cpp] view plaincopyprint?
01.#define PCRE_STATIC // ��̬�����ѡ��   
02.#include <stdio.h>   
03.#include <string.h>   
04.#include <pcre.h>   
05.#define OVECCOUNT 30 /* should be a multiple of 3 * /   
06.#define EBUFLEN 128   
07.#define BUFLEN 1024   
08.  
09.int main()  
10.{  
11.    pcre  *re;  
12.    const char *error;  
13.    int  erroffset;  
14.    int  ovector[OVECCOUNT];  
15.    int  rc, i;  
16.    char  src [] = "111 <title>Hello World</title> 222";   // Ҫ������ƥ����ַ���   
17.    char  pattern [] = "<title>(.*)</(tit)le>";              // ��Ҫ��������ַ�����ʽ��������ʽ   
18.    printf("String : %s/n", src);  
19.    printf("Pattern: /"%s/"/n", pattern);  
20.    re = pcre_compile(pattern,       // pattern, �����������Ҫ��������ַ�����ʽ��������ʽ   
21.                      0,            // options, �������������ָ������ʱ��һЩѡ��   
22.                      &error,       // errptr, ����������������������Ϣ   
23.                      &erroffset,   // erroffset, ���������pattern�г���λ�õ�ƫ����   
24.                      NULL);        // tableptr, �������������ָ���ַ���һ�������NULL   
25.    // ����ֵ��������õ�������ʽ��pcre�ڲ���ʾ�ṹ   
26.    if (re == NULL) {                 //�������ʧ�ܣ����ش�����Ϣ   
27.        printf("PCRE compilation failed at offset %d: %s/n", erroffset, error);  
28.        return 1;  
29.    }  
30.    rc = pcre_exec(re,            // code, �����������pcre_compile����õ�������ṹ��ָ��   
31.                   NULL,          // extra, ���������������pcre_exec��һЩ�����������Ϣ�Ľṹ��ָ��   
32.                   src,           // subject, ���������Ҫ������ƥ����ַ���   
33.                   strlen(src),  // length, ��������� Ҫ������ƥ����ַ�����ָ��   
34.                   0,             // startoffset, �������������ָ��subject��ʲôλ�ÿ�ʼ��ƥ���ƫ����   
35.                   0,             // options, ��������� ����ָ��ƥ������е�һЩѡ��   
36.                   ovector,       // ovector, �����������������ƥ��λ��ƫ����������   
37.                   OVECCOUNT);    // ovecsize, ��������� ��������ƥ��λ��ƫ���������������С   
38.    // ����ֵ��ƥ��ɹ����طǸ�����û��ƥ�䷵�ظ���   
39.    if (rc < 0) {                     //���û��ƥ�䣬���ش�����Ϣ   
40.        if (rc == PCRE_ERROR_NOMATCH) printf("Sorry, no match .../n");  
41.        else printf("Matching error %d/n", rc);  
42.        pcre_free(re);  
43.        return 1;  
44.    }  
45.    printf("/nOK, has matched .../n/n");   //û�г����Ѿ�ƥ��   
46.    for (i = 0; i < rc; i++) {             //�ֱ�ȡ��������� $0��������ʽ $1��һ��()   
47.        char *substring_start = src + ovector[2*i];  
48.        int substring_length = ovector[2*i+1] - ovector[2*i];  
49.        printf("$%2d: %.*s/n", i, substring_length, substring_start);  
50.    }  
51.    pcre_free(re);                     // ����������ʽre �ͷ��ڴ�   
52.    return 0;  
53.}  
#define PCRE_STATIC // ��̬�����ѡ��
#include <stdio.h>
#include <string.h>
#include <pcre.h>
#define OVECCOUNT 30 /* should be a multiple of 3 * /
#define EBUFLEN 128
#define BUFLEN 1024

int main()
{
    pcre  *re;
    const char *error;
    int  erroffset;
    int  ovector[OVECCOUNT];
    int  rc, i;
    char  src [] = "111 <title>Hello World</title> 222";   // Ҫ������ƥ����ַ���
    char  pattern [] = "<title>(.*)</(tit)le>";              // ��Ҫ��������ַ�����ʽ��������ʽ
    printf("String : %s/n", src);
    printf("Pattern: /"%s/"/n", pattern);
    re = pcre_compile(pattern,       // pattern, �����������Ҫ��������ַ�����ʽ��������ʽ
                      0,            // options, �������������ָ������ʱ��һЩѡ��
                      &error,       // errptr, ����������������������Ϣ
                      &erroffset,   // erroffset, ���������pattern�г���λ�õ�ƫ����
                      NULL);        // tableptr, �������������ָ���ַ���һ�������NULL
    // ����ֵ��������õ�������ʽ��pcre�ڲ���ʾ�ṹ
    if (re == NULL) {                 //�������ʧ�ܣ����ش�����Ϣ
        printf("PCRE compilation failed at offset %d: %s/n", erroffset, error);
        return 1;
    }
    rc = pcre_exec(re,            // code, �����������pcre_compile����õ�������ṹ��ָ��
                   NULL,          // extra, ���������������pcre_exec��һЩ�����������Ϣ�Ľṹ��ָ��
                   src,           // subject, ���������Ҫ������ƥ����ַ���
                   strlen(src),  // length, ��������� Ҫ������ƥ����ַ�����ָ��
                   0,             // startoffset, �������������ָ��subject��ʲôλ�ÿ�ʼ��ƥ���ƫ����
                   0,             // options, ��������� ����ָ��ƥ������е�һЩѡ��
                   ovector,       // ovector, �����������������ƥ��λ��ƫ����������
                   OVECCOUNT);    // ovecsize, ��������� ��������ƥ��λ��ƫ���������������С
    // ����ֵ��ƥ��ɹ����طǸ�����û��ƥ�䷵�ظ���
    if (rc < 0) {                     //���û��ƥ�䣬���ش�����Ϣ
        if (rc == PCRE_ERROR_NOMATCH) printf("Sorry, no match .../n");
        else printf("Matching error %d/n", rc);
        pcre_free(re);
        return 1;
    }
    printf("/nOK, has matched .../n/n");   //û�г����Ѿ�ƥ��
    for (i = 0; i < rc; i++) {             //�ֱ�ȡ��������� $0��������ʽ $1��һ��()
        char *substring_start = src + ovector[2*i];
        int substring_length = ovector[2*i+1] - ovector[2*i];
        printf("$%2d: %.*s/n", i, substring_length, substring_start);
    }
    pcre_free(re);                     // ����������ʽre �ͷ��ڴ�
    return 0;
}
 


�����������ϣ��������˲�������һ��forѭ���ĺ��壬ovector���ص���ƥ���ַ�����ƫ�ƣ�������ʼƫ�ƺͽ���ƫ�ƣ����Ծ���ѭ���ڲ���2*i����
*/
