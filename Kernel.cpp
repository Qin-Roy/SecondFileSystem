#include <iostream>
#include <stdio.h>
#include "Kernel.h"

using namespace std;

Kernel Kernel::instance;

/*
* �豸�������ٻ������ȫ��manager
*/
BufferManager g_BufferManager;

/*
* �ļ�ϵͳ���ȫ��manager
*/
FileSystem g_FileSystem;
FileManager g_FileManager;


// user�ṹ
User g_u;

Kernel::Kernel()
{
}

Kernel::~Kernel()
{
}

Kernel& Kernel::Instance()
{
	return Kernel::instance;
}

// ��ʼ������顢�������
void Kernel::InitBuffer(char*p)
{
	this->m_BufferManager = &g_BufferManager;

	// printf("Initialize Buffer...");
	this->GetBufferManager().Initialize(p);
}

// ��ʼ���ļ�ϵͳ
void Kernel::InitFileSystem()
{
	this->m_FileSystem = &g_FileSystem;
	this->m_FileManager = &g_FileManager;
	this->m_u = &g_u;
	
	// printf("Initialize File System...");
	this->GetFileSystem().Initialize();

	// printf("Initialize File Manager...");
	this->GetFileManager().Initialize();
}

// �ú�����ɳ�ʼ���ں˴󲿷����ݽṹ�ĳ�ʼ��
void Kernel::Initialize(char*p)
{
	InitBuffer(p);
	InitFileSystem();
}

//��ȡ���ٻ������ȫ�ֱ���
BufferManager& Kernel::GetBufferManager()
{
	return *(this->m_BufferManager);
}

//��ȡ�ļ�ϵͳ���ȫ�ֱ���
FileSystem& Kernel::GetFileSystem()
{
	return *(this->m_FileSystem);
}

//��ȡ�ļ��������ȫ�ֱ���
FileManager& Kernel::GetFileManager()
{
	return *(this->m_FileManager);
}

/* ��ȡ��ǰ���̵�User�ṹ */
User& Kernel::GetUser()
{
	return *(this->m_u);
}
