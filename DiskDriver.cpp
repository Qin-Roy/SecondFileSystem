#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/stat.h> 
#include <sys/mman.h>

#include <string.h>
#include <iostream>

#include "DiskDriver.h"
#include "Buf.h"
#include "BufferManager.h"
#include "File.h"
#include "FileSystem.h"
#include "INode.h"
#include "Kernel.h"
#include "OpenFileManager.h"
#include "User.h"

using namespace std;

DiskDriver::DiskDriver(){
    this->fd = -1;
	this->addr = NULL;
    this->len = 0;
}

DiskDriver::~DiskDriver(){
    if (msync(this->addr, this->len, MS_SYNC) == -1) {  // ͬ���޸ĵ�����
        perror("msync");
        exit(-1);
    }
    if (munmap(this->addr, this->len) == -1) {  // ȡ��ӳ��
        perror("munmap");
        exit(-1);
    }
    if (close(this->fd) == -1) {  // �ر��ļ�
        perror("close");
        exit(-1);
    }
}

//��ʼ��SuperBlock
void DiskDriver::InitialSuperBlock(SuperBlock &sb)
{
	sb.s_isize = FileSystem::INODE_ZONE_SIZE;
	sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;

	//��һ��99�� ��������һ�ٿ�һ�� ʣ�µı�������ֱ�ӹ���
	sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

	//������ֱ�ӹ���Ŀ����̿�ĵ�һ���̿���̿��
	int start_last_datablk = FileSystem::DATA_ZONE_END_SECTOR - FileSystem::DATA_ZONE_SIZE % 100;
	for (int i = 0; i < sb.s_nfree; i++)
		sb.s_free[i] = start_last_datablk + i;

	sb.s_ninode = 100;
	for (int i = 0; i < sb.s_ninode; i++)
		sb.s_inode[i] = i ;//ע������ֻ��diskinode�ı�ţ�����ȡ�õ�ʱ��Ҫ�����̿��ת��

	sb.s_fmod = 0;
	sb.s_ronly = 0;
}

//��ʼ�������̿�
void DiskDriver::InitialDataBlock(char *data)
{
	struct {
		int nfree;//������еĸ���
		int free[100];//������е�������
	}tmp_table;
	int last_datablk_num = FileSystem::DATA_ZONE_SIZE;//�����̿������
	//��ʼ�������б�
	for (int i = 0;; i++)
	{
		if (last_datablk_num >= 100)
			tmp_table.nfree = 100;
		else
			break;
		last_datablk_num -= tmp_table.nfree;

		for (int j = 0; j < tmp_table.nfree; j++)
		{
			if (i == 0 && j == 0)
				tmp_table.free[j] = 0;
			else
			{
				tmp_table.free[j] = 100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1;
			}
		}
		memcpy(&data[99 * 512 + i * 100 * 512], (void*)&tmp_table, sizeof(tmp_table));
	}
}

//��ʼ��c.img
void DiskDriver::InitialImg()
{
	SuperBlock spb;
	InitialSuperBlock(spb);
	DiskInode *di = new DiskInode[FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR];

	//����rootDiskInode�ĳ�ʼֵ
	di[0].d_mode = Inode::IFDIR;  // Ŀ¼�ļ�
	di[0].d_mode |= Inode::IEXEC; // ��ִ��

	char *datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
	memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
	InitialDataBlock(datablock);

	int len1 = sizeof(SuperBlock);
	int len2 = FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode);
	int len3 = FileSystem::DATA_ZONE_SIZE * 512;
	this->len = len1 + len2 + len3;
	//�޸��ļ���С
	lseek(this->fd, this->len - 1, SEEK_SET); 
	write(this->fd, " ", 1); 
	//���ļ�ӳ��������ڴ��ַ
	this->addr = (char*)mmap(NULL, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
	if (this->addr == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    //�������顢inode����������д���ļ�
	memcpy(this->addr, &spb, len1);
	memcpy(&this->addr[len1], di, len2);
	memcpy(&this->addr[len1+len2], datablock, len3);

	// cout << "Disk formatting completed" << endl;
//	exit(1);
}

//��ʼ��ϵͳ
void DiskDriver::InitialSystem()
{
	//����ϵͳ��ʼ����װ��spb�͸�Ŀ¼inode,����user�ṹ�еı�Ҫ����"
	// cout<<"Initial System"<<endl;
	FileManager& fileMgr = Kernel::Instance().GetFileManager();
	fileMgr.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
	fileMgr.rootDirInode->i_flag &= (~Inode::ILOCK);

	Kernel::Instance().GetFileSystem().LoadSuperBlock();
	User& us = Kernel::Instance().GetUser();
	us.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);
	strcpy(us.u_curdir, "/");
	// cout << "System formatting completed" << endl;
}

//��ʼ���ܺ���
void DiskDriver::Initial(){
    this->fd = open("c.img", O_RDWR);
    if (this->fd == -1) { //�ļ�������  
		this->fd = open("c.img", O_RDWR | O_CREAT, 0666);
		if (this->fd == -1) {
			cout<<"Failed to open or create a file"<<endl;
			exit(-1);
		}
		InitialImg();
	}
	else{ //�ļ�����  
		struct stat st; //�����ļ���Ϣ�ṹ��  
		int r = fstat(this->fd, &st);
		if (r == -1) {
			perror("fstat");
			close(this->fd);
			exit(-1);
		}
		//ȡ���ļ���С
		this->len = st.st_size;
		//���ļ�ӳ��������ڴ��ַ
		this->addr=(char*)mmap(NULL, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
        if (this->addr == MAP_FAILED) {
            perror("mmap");
            exit(-1);
        }
    }
	Kernel::Instance().Initialize(this->addr); //��ʼ�����ȫ�ֱ���
	InitialSystem(); //��ʼ����װ��spb�͸�Ŀ¼inode,����user�ṹ�еı�Ҫ����"
}