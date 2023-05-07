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
    if (msync(this->addr, this->len, MS_SYNC) == -1) {  // 同步修改到磁盘
        perror("msync");
        exit(-1);
    }
    if (munmap(this->addr, this->len) == -1) {  // 取消映射
        perror("munmap");
        exit(-1);
    }
    if (close(this->fd) == -1) {  // 关闭文件
        perror("close");
        exit(-1);
    }
}

//初始化SuperBlock
void DiskDriver::InitialSuperBlock(SuperBlock &sb)
{
	sb.s_isize = FileSystem::INODE_ZONE_SIZE;
	sb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;

	//第一组99块 其他都是一百块一组 剩下的被超级快直接管理
	sb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

	//超级块直接管理的空闲盘块的第一个盘块的盘块号
	int start_last_datablk = FileSystem::DATA_ZONE_END_SECTOR - FileSystem::DATA_ZONE_SIZE % 100;
	for (int i = 0; i < sb.s_nfree; i++)
		sb.s_free[i] = start_last_datablk + i;

	sb.s_ninode = 100;
	for (int i = 0; i < sb.s_ninode; i++)
		sb.s_inode[i] = i ;//注：这里只是diskinode的编号，真正取用的时候要进行盘块的转换

	sb.s_fmod = 0;
	sb.s_ronly = 0;
}

//初始化空闲盘块
void DiskDriver::InitialDataBlock(char *data)
{
	struct {
		int nfree;//本组空闲的个数
		int free[100];//本组空闲的索引表
	}tmp_table;
	int last_datablk_num = FileSystem::DATA_ZONE_SIZE;//索引盘块的数量
	//初始化索引列表
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

//初始化c.img
void DiskDriver::InitialImg()
{
	SuperBlock spb;
	InitialSuperBlock(spb);
	DiskInode *di = new DiskInode[FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR];

	//设置rootDiskInode的初始值
	di[0].d_mode = Inode::IFDIR;  // 目录文件
	di[0].d_mode |= Inode::IEXEC; // 可执行

	char *datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
	memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
	InitialDataBlock(datablock);

	int len1 = sizeof(SuperBlock);
	int len2 = FileSystem::INODE_ZONE_SIZE*FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode);
	int len3 = FileSystem::DATA_ZONE_SIZE * 512;
	this->len = len1 + len2 + len3;
	//修改文件大小
	lseek(this->fd, this->len - 1, SEEK_SET); 
	write(this->fd, " ", 1); 
	//把文件映射成虚拟内存地址
	this->addr = (char*)mmap(NULL, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
	if (this->addr == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    //将超级块、inode区、数据区写入文件
	memcpy(this->addr, &spb, len1);
	memcpy(&this->addr[len1], di, len2);
	memcpy(&this->addr[len1+len2], datablock, len3);

	// cout << "Disk formatting completed" << endl;
//	exit(1);
}

//初始化系统
void DiskDriver::InitialSystem()
{
	//进入系统初始化：装填spb和根目录inode,设置user结构中的必要参数"
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

//初始化总函数
void DiskDriver::Initial(){
    this->fd = open("c.img", O_RDWR);
    if (this->fd == -1) { //文件不存在  
		this->fd = open("c.img", O_RDWR | O_CREAT, 0666);
		if (this->fd == -1) {
			cout<<"Failed to open or create a file"<<endl;
			exit(-1);
		}
		InitialImg();
	}
	else{ //文件存在  
		struct stat st; //定义文件信息结构体  
		int r = fstat(this->fd, &st);
		if (r == -1) {
			perror("fstat");
			close(this->fd);
			exit(-1);
		}
		//取得文件大小
		this->len = st.st_size;
		//把文件映射成虚拟内存地址
		this->addr=(char*)mmap(NULL, this->len, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
        if (this->addr == MAP_FAILED) {
            perror("mmap");
            exit(-1);
        }
    }
	Kernel::Instance().Initialize(this->addr); //初始化相关全局变量
	InitialSystem(); //初始化：装填spb和根目录inode,设置user结构中的必要参数"
}