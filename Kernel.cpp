#include <iostream>
#include <stdio.h>
#include "Kernel.h"

using namespace std;

Kernel Kernel::instance;

/*
* 设备管理、高速缓存管理全局manager
*/
BufferManager g_BufferManager;

/*
* 文件系统相关全局manager
*/
FileSystem g_FileSystem;
FileManager g_FileManager;


// user结构
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

// 初始化缓存块、缓存队列
void Kernel::InitBuffer(char*p)
{
	this->m_BufferManager = &g_BufferManager;

	// printf("Initialize Buffer...");
	this->GetBufferManager().Initialize(p);
}

// 初始化文件系统
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

// 该函数完成初始化内核大部分数据结构的初始化
void Kernel::Initialize(char*p)
{
	InitBuffer(p);
	InitFileSystem();
}

//获取高速缓存管理全局变量
BufferManager& Kernel::GetBufferManager()
{
	return *(this->m_BufferManager);
}

//获取文件系统相关全局变量
FileSystem& Kernel::GetFileSystem()
{
	return *(this->m_FileSystem);
}

//获取文件管理相关全局变量
FileManager& Kernel::GetFileManager()
{
	return *(this->m_FileManager);
}

/* 获取当前进程的User结构 */
User& Kernel::GetUser()
{
	return *(this->m_u);
}
