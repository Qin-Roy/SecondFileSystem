#include <iostream>
#include <stdio.h>
#include "File.h"
#include "User.h"
#include "Kernel.h"

using namespace std;

/*==============================class File===================================*/
File::File() //初始化
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

//进程请求打开文件时，在打开文件描述符表中分配一个空闲表项
int OpenFiles::AllocFreeSlot()
{
	int i;
	User& u = Kernel::Instance().GetUser(); //获取User结构

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* 进程打开文件描述符表中找到空闲项，则返回之 */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* 设置核心栈现场保护区中的EAX寄存器的值，即系统调用返回值 */
			u.u_ar0 = i;
			return i;
		}
	}

	u.u_ar0 = -1;   /* Open1，需要一个标志。当打开文件结构创建失败时，可以回收系统资源*/
	u.u_error = User::my_EMFILE;
	return -1;
}

//根据用户系统调用提供的文件描述符参数fd,找到对应的打开文件控制块File结构
File* OpenFiles::GetF(int fd)
{
	File* pFile;
	User& u = Kernel::Instance().GetUser(); //获取User结构

	/* 如果打开文件描述符的值超出了范围 */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		u.u_error = User::my_EBADF;
		return NULL;
	}

	pFile = this->ProcessOpenFileTable[fd];  //根据fd找到在打开文件表中对应的File对象
	if (pFile == NULL)
	{
		cout << "GetF错误：没有找到fd对应的File结构" << endl;
		u.u_error = User::my_EBADF;
	}
	return pFile;	/* 即使pFile==NULL也返回它，由调用GetF的函数来判断返回值 */
}

//为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES) //Bad File Number
	{
		cout << "SetF错误：fd错误" << endl;
		return;
	}
	/* 进程打开文件描述符指向系统打开文件表中相应的File结构 */
	this->ProcessOpenFileTable[fd] = pFile;
}

/*==============================class IOParameter===================================*/
IOParameter::IOParameter() //初始化
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	//nothing to do here
}

