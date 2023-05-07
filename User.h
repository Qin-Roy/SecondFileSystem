#pragma once

#include<iostream>
#include"File.h"
#include"FileSystem.h"

/*
* @comment ������Unixv6�� struct user�ṹ��Ӧ�����ֻ�ı�
* ���������޸ĳ�Ա�ṹ���֣������������͵Ķ�Ӧ��ϵ����:
*/
class User
{
public:
	//����ҪEAX�Ĵ���

	/* u_error's Error Code */
	/* 1~32 ����linux ���ں˴����е�/usr/include/asm/errno.h, ����for V6++ */
	enum ErrorCode
	{
		my_NOERROR = 0,	/* No error */
		my_EPERM = 1,	/* Operation not permitted */
		my_ENOENT = 2,	/* No such file or directory */
		my_ESRCH = 3,	/* No such process */
		my_EINTR = 4,	/* Interrupted system call */
		my_EIO = 5,	/* I/O error */
		my_ENXIO = 6,	/* No such device or address */
		my_E2BIG = 7,	/* Arg list too long */
		my_ENOEXEC = 8,	/* Exec format error */
		my_EBADF = 9,	/* Bad file number */
		my_ECHILD = 10,	/* No child processes */
		my_EAGAIN = 11,	/* Try again */
		my_ENOMEM = 12,	/* Out of memory */
		my_EACCES = 13,	/* Permission denied */
		my_EFAULT = 14,	/* Bad address */
		my_ENOTBLK = 15,	/* Block device required */
		my_EBUSY = 16,	/* Device or resource busy */
		my_EEXIST = 17,	/* File exists */
		my_EXDEV = 18,	/* Cross-device link */
		my_ENODEV = 19,	/* No such device */
		my_ENOTDIR = 20,	/* Not a directory */
		my_EISDIR = 21,	/* Is a directory */
		my_EINVAL = 22,	/* Invalid argument */
		my_ENFILE = 23,	/* File table overflow */
		my_EMFILE = 24,	/* Too many open files */
		my_ENOTTY = 25,	/* Not a typewriter(terminal) */
		my_ETXTBSY = 26,	/* Text file busy */
		my_EFBIG = 27,	/* File too large */
		my_ENOSPC = 28,	/* No space left on device */
		my_ESPIPE = 29,	/* Illegal seek */
		my_EROFS = 30,	/* Read-only file system */
		my_EMLINK = 31,	/* Too many links */
		my_EPIPE = 32,	/* Broken pipe */
		my_ENOSYS = 100
		//EFAULT	= 106
	};

	//����Ҫ�ź�

public:
	/* ϵͳ������س�Ա */
	//ֻ�������ļ�ϵͳ��صĲ���
	int u_ar0;

	long int u_arg[5];				/* ��ŵ�ǰϵͳ���ò��� */
	char* u_dirp;				/* ϵͳ���ò���(һ������Pathname)��ָ�� */

	/* �ļ�ϵͳ��س�Ա */
	Inode* u_cdir;		/* ָ��ǰĿ¼��Inodeָ�� */
	Inode* u_pdir;		/* ָ��Ŀ¼��Inodeָ�� */

	DirectoryEntry u_dent;					/* ��ǰĿ¼��Ŀ¼�� */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* ��ǰ·������ */
	char u_curdir[128];						/* ��ǰ����Ŀ¼����·�� */

	ErrorCode u_error;			/* ��Ŵ����� */

	/* �ļ�ϵͳ��س�Ա */
	OpenFiles u_ofiles;		/* ���̴��ļ������������ */

	/* �ļ�I/O���� */
	IOParameter u_IOParam;	/* ��¼��ǰ����д�ļ���ƫ�������û�Ŀ�������ʣ���ֽ������� */

};  

