#ifndef KERNEL_H
#define KERNEL_H

#include "User.h"
#include "BufferManager.h"
#include "OpenFileManager.h"
#include "FileSystem.h"

/*
* Kernel�����ڷ�װ�����ں���ص�ȫ����ʵ������
* ����PageManager, ProcessManager�ȡ�
*
* Kernel�����ڴ���Ϊ����ģʽ����֤�ں��з�װ���ں�
* ģ��Ķ���ֻ��һ��������
*/
class Kernel
{
public:
	Kernel();
	~Kernel();
	static Kernel& Instance();
	void Initialize(char*p);		/* �ú�����ɳ�ʼ���ں˴󲿷����ݽṹ�ĳ�ʼ�� */

	BufferManager& GetBufferManager(); //��ȡ���ٻ������ȫ�ֱ���
	FileSystem& GetFileSystem();       //��ȡ�ļ�ϵͳ���ȫ�ֱ���
	FileManager& GetFileManager();     //��ȡ�ļ��������ȫ�ֱ���
	User& GetUser();		    /* ��ȡ��ǰ���̵�User�ṹ */

private:
	void InitBuffer(char*p);
	void InitFileSystem();

private:
	static Kernel instance;		/* Kernel������ʵ�� */

	BufferManager* m_BufferManager;   //���ٻ������ȫ�ֱ���
	FileSystem* m_FileSystem;         //�ļ�ϵͳ���ȫ�ֱ���
	FileManager* m_FileManager;       //�ļ��������ȫ�ֱ���
	User *m_u;
};

#endif
