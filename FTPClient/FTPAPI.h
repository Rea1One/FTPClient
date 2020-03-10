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


/**********可用命令*********/
SOCKET ftp_connect(char *host, int port, char *user, char *pwd); //连接到服务器
int ftp_quit(SOCKET sock); //断开连接
int ftp_type(SOCKET sock, char mode); //设置FTP传输类型
int ftp_cwd(SOCKET sock, char *path); //更改工作目录
int ftp_cdup(SOCKET sock); //回到上级目录
int ftp_mkd(SOCKET sock, char *path); //创建目录
SOCKET ftp_pasv_connect(SOCKET c_sock); //连接到PASV接口
int ftp_list(SOCKET c_sock, char *path, char **data, int *data_len); //列出FTP工作空间的所有目录
int ftp_deletefolder(SOCKET sock, char *path); //删除目录
int ftp_deletefile(SOCKET sock, char *filename); //删除文件
int ftp_renamefile(SOCKET sock, char *s, char *d); //修改文件/目录&移动文件/目录
int ftp_server2local(SOCKET c_sock, char *s, char *d, int * size); //从服务器复制文件到本地 RETR
int ftp_local2server(SOCKET c_sock, char *s, char *d, int * size); //从本地复制文件到服务器 STOR
int ftp_recv(SOCKET sock, char *re_buf, size_t *len); //获取响应码


#endif // FTPAPI_H_INCLUDED
