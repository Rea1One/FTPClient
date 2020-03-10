#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include "FTPAPI.h"
#include "FTPResponseCode.h"
#pragma comment(lib,"WS2_32.LIB") 

#define BUFSIZE 1024

/**
 * ����: ����һ��Socket������.
 * ����: IP������, �˿�
 * ����ֵ: Socket�׽���
 * */
SOCKET socket_connect(char *host, int port)
{
	int i = 0;
	//��ʼ�� Socket dll
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 0);
	if (WSAStartup(socketVersion, &wsaData))
	{
		printf("Init socket dll error!");
		exit(1);
	}

	struct hostent * server = gethostbyname(host);
	if (!server)
		return -1;
	unsigned char ch[4];
	char ip[20];
	//һ��hostname ���Զ�Ӧ���ip
	while (server->h_addr_list[i] != NULL)
	{
		memcpy(&ch, server->h_addr_list[i], 4);
		sprintf(ip, "%d.%d.%d.%d", ch[0], ch[1], ch[2], ch[3]);
		//printf("%s\n",ip);
		i++;
	}

	//����Socket
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
	if (SOCKET_ERROR == s)
	{
		printf("Create Socket Error!");
		exit(1);
	}
	//���ó�ʱ����
	int timeout = 3000; //���ӵ����绷��Ҫ���ó�ʱ�ж�
	int ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));//����ʱ��
	ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));//����ʱ��
	//ָ����������ַ
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.S_un.S_addr = inet_addr(ip);
	address.sin_port = htons((unsigned short)port);
	//����
	if (SOCKET_ERROR == connect(s, (LPSOCKADDR)&address, sizeof(address)))
	{
		printf("Can Not Connect To Server IP!\n");
		exit(1);
	}
	return s;
}

/**
 * ����: ���ӵ�һ��FTP������������socket
 * ����: IP������, �˿�
 * ����ֵ: Socket�׽���
 * */
SOCKET connect_server(char *host, int port)
{
	SOCKET ctrl_sock;
	char buf[BUFSIZE];
	int result;
	size_t len;

	ctrl_sock = socket_connect(host, port);
	if (-1 == ctrl_sock)
	{
		return -1;
	}
	if ((len = recv(ctrl_sock, buf, BUFSIZE, 0)) > 0)
	{
		//len = recv(ctrl_sock, buf, BUFSIZE, 0);
		buf[len] = 0;
		printf("%s\n", buf); //220-FileZilla Server version 0.9.43 beta
	}
	sscanf(buf, "%d", &result);

	if (FTP_SERVICE_READY != result)
	{
		printf("FTP Not ready, Close the socet.");
		closesocket(ctrl_sock); //�ر�Socket
		return -1;
	}
	return ctrl_sock;
}

/**
 * ����: send�������������recv���
 * ����: SOCKET������������-�����������������ֽ���
 * ����ֵ: 0 ��ʾ���ͳɹ�  -1��ʾ����ʧ��
 * */
int ftp_sendcmd_re(SOCKET sock, char *cmd, char *re_buf, size_t *len)
{
	char buf[BUFSIZE];
	size_t r_len;
	if (send(sock, cmd, strlen(cmd), 0) == -1)
	{
		return -1;
	}
	r_len = recv(sock, buf, BUFSIZE, 0);
	if (r_len < 1)
		return -1;
	buf[r_len] = 0;
	if (NULL != len)
		*len = r_len;
	if (NULL != re_buf)
		sprintf(re_buf, "%s", buf);
	return 0;
}

/**
 * ����: send��������
 * ����: SOCKET,����
 * ����ֵ: FTP��Ӧ��
 * */
int ftp_sendcmd(SOCKET sock, char *cmd)
{
	char buf[BUFSIZE];
	int result;
	size_t len;
	printf("FTP Client: %s", cmd);
	result = ftp_sendcmd_re(sock, cmd, buf, &len);
	printf("FTP Server: %s", buf);
	if (0 == result)
	{
		sscanf(buf, "%d", &result);
	}
	return result;
}

/**
 * ����: ��¼FTP������
 * ����: SOCKET�׽��֣������û�������������
 * ����ֵ: 0 ��ʾ��¼�ɹ�   -1 ��ʾ��¼ʧ��
 * */
int login_server(SOCKET sock, char *user, char *pwd)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "USER %s\r\n", user);
	//����Ҫ��socket��������
	int timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	result = ftp_sendcmd(sock, buf);
	if (FTP_LOGIN_SUCCESS == result) //ֱ�ӵ�¼
		return 0;
	else if (FTP_PASSWORD_REQUIREd == result) //��Ҫ����
	{
		sprintf(buf, "PASS %s\r\n", pwd);
		result = ftp_sendcmd(sock, buf);
		if (FTP_LOGIN_SUCCESS == result)
		{
			return 0;
		}
		else //530 �������
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

/**
 * ����: winsockʹ�ú�Ҫ����WSACleanup�����ر������豸���Ա��ͷ���ռ�õ���Դ
 * ����: SOCKET
 * ����ֵ: ��
 * */
void socket_close(int c_sock)
{
	WSACleanup();
}

/**
 * ����: ���ӵ�FTP������
 * ����: hostname��IP���˿ڣ��û���������
 * ����ֵ: �����ӵ�FTP��������SOCKET   -1 ��ʾ��¼ʧ��
 * */
SOCKET ftp_connect(char *host, int port, char *user, char *pwd)
{
	SOCKET sock;
	sock = connect_server(host, port);
	if (-1 == sock)
	{
		return -1;
	}
	if (-1 == login_server(sock, user, pwd))
	{
		closesocket(sock);
		return -1;
	}
	return sock;
}

/**
 * ����: �Ͽ�FTP������
 * ����: SOCKET
 * ����ֵ: �ɹ��Ͽ�״̬��
 * */
int ftp_quit(SOCKET sock)
{
	int result = 0;
	result = ftp_sendcmd(sock, "QUIT\r\n");
	closesocket(sock);
	socket_close(sock);
	return result;
}

/**
 * ����: ����FTP�������� A:ascii I:Binary
 * ����: SOCkET������
 * ����ֵ: 0 ��ʾ�ɹ�   -1 ��ʾʧ��
 * */
int ftp_type(SOCKET sock, char mode)
{
	char buf[BUFSIZ];
	sprintf(buf, "TYPE %c\r\n", mode);
	if (FTP_SUCCESS != ftp_sendcmd(sock, buf))
		return -1;
	else
		return 0;
}

/**
 * ����: ���Ĺ���Ŀ¼
 * ����: SOCKET������Ŀ¼
 * ����ֵ: 0 ��ʾ�ɹ�  -1 ��ʾʧ��
 * */
int ftp_cwd(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "CWD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)  //250 �ļ���Ϊ���
		return -1;
	else
		return 0;
}

/**
 * ����: �ص��ϼ�Ŀ¼
 * ����: SOCKET
 * ����ֵ: 0 ������������  result ������������Ӧ��
 * */
int ftp_cdup(SOCKET sock)
{
	int result;
	result = ftp_sendcmd(sock, "CDUP\r\n");
	if (FTP_FILE_ACTION_COMPLETE == result || FTP_SUCCESS == result)
		return 0;
	else
		return result;
}

/**
 * ����: ����Ŀ¼
 * ����: SOCKET���ļ�Ŀ¼·��(�����·��������·��)
 * ����ֵ: 0 ������������  result ������������Ӧ��
 * */
int ftp_mkd(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "MKD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_CREATED != result) //257 ·��������
		return result; //550 Ŀ¼�Ѵ���
	else
		return 0;
}

/**
 * ����: ���ӵ�PASV�ӿ�
 *       PASV����������ʽ�����ӹ����ǣ�
 *       �ͻ������������FTP�˿ڣ�Ĭ����21��������������
 *       �������������ӣ�����һ��������·��
 * ����: ������·SOCKET cmd-socket
 * ����ֵ: ������·SOCKET raw-socket  -1 ��ʾ����ʧ��
 * */
SOCKET ftp_pasv_connect(SOCKET c_sock)
{
	SOCKET r_sock;
	int send_result;
	size_t len;
	int addr[6]; //IP*4+Port*2
	char buf[BUFSIZE];
	char result_buf[BUFSIZE];

	//����PASV����ģʽ
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "PASV\r\n");
	send_result = ftp_sendcmd_re(c_sock, buf, result_buf, &len);
	if (send_result == 0)
	{
		sscanf(result_buf, "%*[^(](%d,%d,%d,%d,%d,%d)",
			&addr[0], &addr[1], &addr[2], &addr[3],
			&addr[4], &addr[5]);
	}

	//����PASV�˿�
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
	r_sock = socket_connect(buf, addr[4] * 256 + addr[5]);
	if (-1 == r_sock)
		return -1;
	return r_sock;
}

/**
 * ����: �г�FTP�����ռ������Ŀ¼
 * ����: ������·SOCKET�������ռ䣬�б���Ϣ���б���Ϣ��С
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��  -1 ��ʾ����pasv����
 * */
int ftp_list(SOCKET c_sock, char *path, char **data, int *data_len)
{
	SOCKET r_sock;
	char buf[BUFSIZE];
	int send_re;
	int result;
	size_t len, buf_len, total_len;

	//���ӵ�PASV�ӿ�
	r_sock = ftp_pasv_connect(c_sock);
	if (-1 == r_sock)
	{
		return -1;
	}
	//����LIST����
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "LIST %s\r\n", path);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
		return send_re;
	len = total_len = 0;
	buf_len = BUFSIZE;
	char *re_buf = (char *)malloc(buf_len);
	while ((len = recv(r_sock, buf, BUFSIZE, 0)) > 0)
	{
		if (total_len + len > buf_len)
		{
			buf_len *= 2;
			char *re_buf_n = (char *)malloc(buf_len);
			memcpy(re_buf_n, re_buf, total_len);
			free(re_buf);
			re_buf = re_buf_n;
		}
		memcpy(re_buf + total_len, buf, len);
		total_len += len;
	}
	closesocket(r_sock);

	//����������շ���ֵ
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result != 226)
	{
		free(re_buf);
		return result;
	}
	*data = re_buf;
	*data_len = total_len;
	return 0;
}

/**
 * ����: ɾ��Ŀ¼
 * ����: ������·SOCKET��·��Ŀ¼
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��
 * */
int ftp_deletefolder(SOCKET sock, char *path)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "RMD %s\r\n", path);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)
	{
		//550 Directory not empty.
		//550 Directory not found.
		return result;
	}
	return 0;
}

/**
 * ����: ɾ���ļ�
 * ����: ������·SOCKET��·���ļ�(���/����)
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��
 * */
int ftp_deletefile(SOCKET sock, char *filename)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "DELE %s\r\n", filename);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != 250) //250 File deleted successfully
	{
		//550 File not found.
		return result;
	}
	return 0;
}

/**
 * ����: �޸��ļ���&�ƶ�Ŀ¼
 * ����: ������·SOCKET��Դ��ַ��Ŀ�ĵ�ַ
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��
 * */
int ftp_renamefile(SOCKET sock, char *s, char *d)
{
	char buf[BUFSIZE];
	int result;
	sprintf(buf, "RNFR %s\r\n", s);
	result = ftp_sendcmd(sock, buf);
	if (350 != result) //350 �ļ���Ϊ��ͣ����ΪҪ�����ƶ�����
		return result;
	sprintf(buf, "RNTO %s\r\n", d);
	result = ftp_sendcmd(sock, buf);
	if (FTP_FILE_ACTION_COMPLETE != result)
	{
		return result;
	}
	return 0;
}

/**
 * ����: �ӷ����������ļ������� RETR
 * ����: SOCKET��Դ��ַ��Ŀ�ĵ�ַ���ļ���С
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��
 *         -1:�ļ�����ʧ��  -2 pasv�ӿڴ���
 * */
int ftp_server2local(SOCKET c_sock, char *s, char *d, int * size)
{
	SOCKET d_sock;
	size_t len, write_len;
	char buf[BUFSIZ];
	int result;
	*size = 0;
	//�򿪱����ļ�
	FILE * fp = fopen(d, "wb");
	if (NULL == fp)
	{
		printf("Can't Open the file.\n");
		return -1;
	}
	//���ô���ģʽ
	ftp_type(c_sock, 'I');

	//���ӵ�PASV�ӿ� ���ڴ����ļ�
	d_sock = ftp_pasv_connect(c_sock);
	if (-1 == d_sock)
	{
		fclose(fp); //�ر��ļ�
		return -2;
	}

	//����RETR����
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "RETR %s\r\n", s);
	result = ftp_sendcmd(c_sock, buf);
	// 150 Opening data channel for file download from server of "xxxx"
	if (result >= 300 || result == 0) //ʧ�ܿ�����û��Ȩ��ʲô�ģ����忴��Ӧ��
	{
		fclose(fp);
		return result;
	}

	//��ʼ��PASV��ȡ����(����)
	memset(buf, sizeof(buf), 0);
	while ((len = recv(d_sock, buf, BUFSIZE, 0)) > 0)
	{
		write_len = fwrite(&buf, len, 1, fp);
		if (write_len != 1) //д���ļ�������
		{
			closesocket(d_sock); //�ر��׽���
			fclose(fp); //�ر��ļ�
			return -1;
		}
		if (NULL != size)
		{
			*size += write_len;
		}
	}
	//�������
	closesocket(d_sock);
	fclose(fp);

	//����������շ���ֵ
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	printf("%s\n", buf);
	sscanf(buf, "%d", &result);
	if (result >= 300)
	{
		return result;
	}
	//226 Successfully transferred "xxxx"
	return 0;
}

/**
 * ����: �ӱ��ظ����ļ��������� STOR
 * ����: SOCKET��Դ��ַ��Ŀ�ĵ�ַ���ļ���С
 * ����ֵ: 0 ��ʾ�б�ɹ�  result>0 ��ʾ����������Ӧ��
 *         -1:�ļ�����ʧ��  -2 pasv�ӿڴ���
 * */
int ftp_local2server(SOCKET c_sock, char *s, char *d, int * size)
{
	SOCKET d_sock;
	size_t len, send_len;
	char buf[BUFSIZE];
	FILE * fp;
	int send_re;
	int result;
	//�򿪱����ļ�
	fp = fopen(s, "rb");
	if (NULL == fp)
	{
		printf("Can't Not Open the file.\n");
		return -1;
	}
	//���ô���ģʽ
	ftp_type(c_sock, 'I');
	//���ӵ�PASV�ӿ�
	d_sock = ftp_pasv_connect(c_sock);
	if (d_sock == -1)
	{
		fclose(fp);
		return -1;
	}

	//����STOR����
	memset(buf, sizeof(buf), 0);
	sprintf(buf, "STOR %s\r\n", d);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
	{
		fclose(fp);
		return send_re;
	}

	//��ʼ��PASVͨ��д����
	memset(buf, sizeof(buf), 0);
	while ((len = fread(buf, 1, BUFSIZE, fp)) > 0)
	{
		send_len = send(d_sock, buf, len, 0);
		if (send_len != len)
		{
			closesocket(d_sock);
			fclose(fp);
			return -1;
		}
		if (NULL != size)
		{
			*size += send_len;
		}
	}
	//����ϴ�
	closesocket(d_sock);
	fclose(fp);

	//�������������Ӧ��
	memset(buf, sizeof(buf), 0);
	len = recv(c_sock, buf, BUFSIZE, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result >= 300)
	{
		return result;
	}
	return 0;
}

/**
 * ����: ��ȡһ����Ӧ��
 * ����:
 * ����ֵ:
 * */
int ftp_recv(SOCKET sock, char *re_buf, size_t *len)
{
	char buf[BUFSIZE];
	size_t r_len;
	int timeout = 3000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	r_len = recv(sock, buf, BUFSIZE, 0);
	timeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if (r_len < 1)
		return -1;
	buf[r_len] = 0;
	if (NULL != len)
		*len = r_len;
	if (NULL != re_buf)
		sprintf(re_buf, "%s", buf);
	return 0;
}

int main() {
	int size = 144;
	SOCKET s = ftp_connect("127.0.0.1", 21, "user", "user"); //��¼��FTP������
	int ret = ftp_local2server(s, "D:\cat.jpg", "cat.jpg", &size); //�ӱ����ϴ��ļ���������
//	ftp_quit(s); //�˳�FTP������
}