/*
�������ϵ�һЩͳ�����ݣ�

1)ͳ��80�˿�������
netstat -nat|grep -i "80"|wc -l

2��ͳ��httpdЭ��������
ps -ef|grep httpd|wc -l

3����ͳ���������ϵģ�״̬Ϊ��established
netstat -na|grep ESTABLISHED|wc -l

4)������ĸ�IP��ַ�������,�������.
netstat -na|grep ESTABLISHED|awk {print $5}|awk -F: {print $1}|sort|uniq -c|sort -r +0n

netstat -na|grep SYN|awk {print $5}|awk -F: {print $1}|sort|uniq -c|sort -r +0n

---------------------------------------------------------------------------------------------

1���鿴apache��ǰ������������
netstat -an | grep ESTABLISHED | wc -l

�Ա�httpd.conf��MaxClients�����ֲ����١�

2���鿴�ж��ٸ���������
ps aux|grep httpd|wc -l

3������ʹ�����²����鿴����
server-status?auto

#ps -ef|grep httpd|wc -l
1388
ͳ��httpd���������������������һ�����̣�ʹ����Apache��������
��ʾApache�ܹ�����1388�������������ֵApache�ɸ��ݸ�������Զ�������

#netstat -nat|grep -i "80"|wc -l
4341
netstat -an���ӡϵͳ��ǰ��������״̬����grep -i "80"��������ȡ��80�˿��йص����ӵģ�wc -l����������ͳ�ơ�
���շ��ص����־��ǵ�ǰ����80�˿ڵ�����������

#netstat -na|grep ESTABLISHED|wc -l
376
netstat -an���ӡϵͳ��ǰ��������״̬����grep ESTABLISHED ��ȡ���ѽ������ӵ���Ϣ�� Ȼ��wc -lͳ�ơ�
���շ��ص����־��ǵ�ǰ����80�˿ڵ��ѽ������ӵ�������

netstat -nat||grep ESTABLISHED|wc - �ɲ鿴���н������ӵ���ϸ��¼

�鿴Apache�Ĳ�������������TCP����״̬��
����Linux���
netstat -n | awk '/^tcp/ {++S[$NF]} END {for(a in S) print a, S[a]}'

����������Ǵ� ���˻���������ҵ�� ���˻���������ҵ�������ܼ����ϴ��Ƕ���õģ��ǳ��������ؽ��ʾ����
����LAST_ACK 5
����SYN_RECV 30
����ESTABLISHED 1597
����FIN_WAIT1 51
����FIN_WAIT2 504
����TIME_WAIT 1057
�������е�
SYN_RECV��ʾ���ڵȴ��������������
ESTABLISHED��ʾ�������ݴ���״̬��
TIME_WAIT��ʾ������ϣ��ȴ���ʱ��������������

---------------------------------------------------------------------------------------------

�鿴Apache��������������TCP����״̬

�鿴httpd����������preforkģʽ��Apache�ܹ�����Ĳ�������������
����Linux���

ps -ef | grep httpd | wc -l

�������ؽ��ʾ����
����1388
������ʾApache�ܹ�����1388�������������ֵApache�ɸ��ݸ�������Զ��������������������ÿ̨�ķ�ֵ���ﵽ��2002��

�鿴Apache�Ĳ�������������TCP����״̬��
����Linux���

netstat -n | awk '/^tcp/ {++S[$NF]} END {for(a in S) print a, S[a]}'
���ؽ��ʾ����
����LAST_ACK 5
����SYN_RECV 30
����ESTABLISHED 1597
����FIN_WAIT1 51
����FIN_WAIT2 504
����TIME_WAIT 1057
�������е�SYN_RECV��ʾ���ڵȴ��������������ESTABLISHED��ʾ�������ݴ���״̬��TIME_WAIT��ʾ������ϣ��ȴ���ʱ��������������
����״̬������

����CLOSED���������ǻ �Ļ����ڽ���

����LISTEN���������ڵȴ��������

����SYN_RECV��һ�����������Ѿ�����ȴ�ȷ��

����SYN_SENT��Ӧ���Ѿ���ʼ����һ������

����ESTABLISHED���������ݴ���״̬

����FIN_WAIT1��Ӧ��˵���Ѿ����

����FIN_WAIT2����һ����ͬ���ͷ�

����ITMED_WAIT���ȴ����з�������

����CLOSING������ͬʱ���Թر�

����TIME_WAIT����һ���ѳ�ʼ��һ���ͷ�

����LAST_ACK���ȴ����з�����

*/
