#include <iostream>
#include <stdio.h>
#include "File.h"
#include "User.h"
#include "Kernel.h"

using namespace std;

/*==============================class File===================================*/
File::File() //��ʼ��
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

File::~File()
{
	//nothing to do here
}

/*==============================class OpenFiles===================================*/
OpenFiles::OpenFiles()
{
}

OpenFiles::~OpenFiles()
{
}

//����������ļ�ʱ���ڴ��ļ����������з���һ�����б���
int OpenFiles::AllocFreeSlot()
{
	int i;
	User& u = Kernel::Instance().GetUser(); //��ȡUser�ṹ

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* ���̴��ļ������������ҵ�������򷵻�֮ */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* ���ú���ջ�ֳ��������е�EAX�Ĵ�����ֵ����ϵͳ���÷���ֵ */
			u.u_ar0 = i;
			return i;
		}
	}

	u.u_ar0 = -1;   /* Open1����Ҫһ����־�������ļ��ṹ����ʧ��ʱ�����Ի���ϵͳ��Դ*/
	u.u_error = User::my_EMFILE;
	return -1;
}

//�����û�ϵͳ�����ṩ���ļ�����������fd,�ҵ���Ӧ�Ĵ��ļ����ƿ�File�ṹ
File* OpenFiles::GetF(int fd)
{
	File* pFile;
	User& u = Kernel::Instance().GetUser(); //��ȡUser�ṹ

	/* ������ļ���������ֵ�����˷�Χ */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		u.u_error = User::my_EBADF;
		return NULL;
	}

	pFile = this->ProcessOpenFileTable[fd];  //����fd�ҵ��ڴ��ļ����ж�Ӧ��File����
	if (pFile == NULL)
	{
		cout << "GetF����û���ҵ�fd��Ӧ��File�ṹ" << endl;
		u.u_error = User::my_EBADF;
	}
	return pFile;	/* ��ʹpFile==NULLҲ���������ɵ���GetF�ĺ������жϷ���ֵ */
}

//Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ����п���File������������ϵ
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES) //Bad File Number
	{
		cout << "SetF����fd����" << endl;
		return;
	}
	/* ���̴��ļ�������ָ��ϵͳ���ļ�������Ӧ��File�ṹ */
	this->ProcessOpenFileTable[fd] = pFile;
}

/*==============================class IOParameter===================================*/
IOParameter::IOParameter() //��ʼ��
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	//nothing to do here
}

