#ifndef KERNEL_H
#define KERNEL_H

#include "User.h"
#include "BufferManager.h"
#include "OpenFileManager.h"
#include "FileSystem.h"

/*
* Kernel类用于封装所有内核相关的全局类实例对象，
* 例如PageManager, ProcessManager等。
*
* Kernel类在内存中为单体模式，保证内核中封装各内核
* 模块的对象都只有一个副本。
*/
class Kernel
{
public:
	Kernel();
	~Kernel();
	static Kernel& Instance();
	void Initialize(char*p);		/* 该函数完成初始化内核大部分数据结构的初始化 */

	BufferManager& GetBufferManager(); //获取高速缓存管理全局变量
	FileSystem& GetFileSystem();       //获取文件系统相关全局变量
	FileManager& GetFileManager();     //获取文件管理相关全局变量
	User& GetUser();		    /* 获取当前进程的User结构 */

private:
	void InitBuffer(char*p);
	void InitFileSystem();

private:
	static Kernel instance;		/* Kernel单体类实例 */

	BufferManager* m_BufferManager;   //高速缓存管理全局变量
	FileSystem* m_FileSystem;         //文件系统相关全局变量
	FileManager* m_FileManager;       //文件管理相关全局变量
	User *m_u;
};

#endif
