#ifndef FTPAPI_H_INCLUDED
#define FTPAPI_H_INCLUDED

#include <stdio.h>
#include <winsock2.h>

SOCKET socket_connect(char *host, int port);
SOCKET connect_server(char *host, int port);
int ftp_sendcmd_re(SOCKET sock, char *cmd, char *re_buf, size_t *len);
int ftp_sendcmd(SOCKET sock, char *cmd);
int login_server(SOCKET sock, char *user, char *pwd);
void socket_close(int c_sock);


/**********��������*********/
SOCKET ftp_connect(char *host, int port, char *user, char *pwd); //���ӵ�������
int ftp_quit(SOCKET sock); //�Ͽ�����
int ftp_type(SOCKET sock, char mode); //����FTP��������
int ftp_cwd(SOCKET sock, char *path); //���Ĺ���Ŀ¼
int ftp_cdup(SOCKET sock); //�ص��ϼ�Ŀ¼
int ftp_mkd(SOCKET sock, char *path); //����Ŀ¼
SOCKET ftp_pasv_connect(SOCKET c_sock); //���ӵ�PASV�ӿ�
int ftp_list(SOCKET c_sock, char *path, char **data, int *data_len); //�г�FTP�����ռ������Ŀ¼
int ftp_deletefolder(SOCKET sock, char *path); //ɾ��Ŀ¼
int ftp_deletefile(SOCKET sock, char *filename); //ɾ���ļ�
int ftp_renamefile(SOCKET sock, char *s, char *d); //�޸��ļ�/Ŀ¼&�ƶ��ļ�/Ŀ¼
int ftp_server2local(SOCKET c_sock, char *s, char *d, int * size); //�ӷ����������ļ������� RETR
int ftp_local2server(SOCKET c_sock, char *s, char *d, int * size); //�ӱ��ظ����ļ��������� STOR
int ftp_recv(SOCKET sock, char *re_buf, size_t *len); //��ȡ��Ӧ��


#endif // FTPAPI_H_INCLUDED
