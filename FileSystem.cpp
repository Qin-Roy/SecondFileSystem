#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "FileSystem.h"
#include "User.h"
#include "Buf.h"
#include "Kernel.h"
using namespace std;

/*==============================class SuperBlock===================================*/
/* 系统全局超级块SuperBlock对象 */

SuperBlock::SuperBlock()
{
	//nothing to do here
}

SuperBlock::~SuperBlock()
{
	//nothing to do here
}

/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	//nothing to do here
}

FileSystem::~FileSystem()
{
	//nothing to do here
}

//初始化成员变量
void FileSystem::Initialize()
{
	this->m_BufferManager = &Kernel::Instance().GetBufferManager();
}

//系统初始化时读入SuperBlock
void FileSystem::LoadSuperBlock()
{
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
	Buf* pBuf;

	for (int i = 0; i < 2; i++)
	{
		int* p = (int *)&g_spb + i * 128;

		pBuf = bufMgr.Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);

		memcpy(p, pBuf->b_addr, BufferManager::BUFFER_SIZE);
		
		bufMgr.Brelse(pBuf);
	}
	
	g_spb.s_ronly = 0;
}

//根据文件存储设备的设备号dev获取该文件系统的SuperBlock
SuperBlock* FileSystem::GetFS()
{
	return &g_spb;
}

//SuperBlock对象的内存副本更新到存储设备的SuperBlock中去
void FileSystem::Update()
{
	int i;
	SuperBlock* sb=&g_spb;
	Buf* pBuf;

	/* 同步SuperBlock到磁盘 */
	if (true)	/* 该Mount装配块对应某个文件系统 */
		{
			/* 如果该SuperBlock内存副本没有被修改，直接管理inode和空闲盘块被上锁或该文件系统是只读文件系统 */
			if (sb->s_fmod == 0 || sb->s_ronly != 0)
			{
				return;
			}

			/* 清SuperBlock修改标志 */
			sb->s_fmod = 0;

			/*
			* 为将要写回到磁盘上去的SuperBlock申请一块缓存，由于缓存块大小为512字节，
			* SuperBlock大小为1024字节，占据2个连续的扇区，所以需要2次写入操作。
			*/
			for (int j = 0; j < 2; j++)
			{
				/* 第一次p指向SuperBlock的第0字节，第二次p指向第512字节 */
				int* p = (int *)sb + j * 128;

				/* 将要写入到设备dev上的SUPER_BLOCK_SECTOR_NUMBER + j扇区中去 */
				pBuf = this->m_BufferManager->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

				/* 将SuperBlock中第0 - 511字节写入缓存区 */
				memcpy(pBuf->b_addr, p, 512);
				//Utility::DWordCopy(p, (int *)pBuf->b_addr, 128);

				/* 将缓冲区中的数据写到磁盘上 */
				this->m_BufferManager->Bwrite(pBuf);
			}
		}

	/* 同步修改过的内存Inode到对应外存Inode */
	g_InodeTable.UpdateInodeTable();

	/* 将延迟写的缓存块写到磁盘上 */
	this->m_BufferManager->Bflush();
}

//在存储设备dev上分配一个空闲外存INode，一般用于创建新的文件。
Inode* FileSystem::IAlloc()
{
	SuperBlock* sb;
	Buf* pBuf;
	Inode* pNode;
	User& u = Kernel::Instance().GetUser();
	int ino;	/* 分配到的空闲外存Inode编号 */

	/* 获取相应设备的SuperBlock内存副本 */
	sb = this->GetFS();

	/*
	* SuperBlock直接管理的空闲Inode索引表已空，
	* 必须到磁盘上搜索空闲Inode。
	* 先对inode列表上锁，
	* 因为在以下程序中会进行读盘操作可能会导致进程切换，
	* 其他进程有可能访问该索引表，将会导致不一致性。
	*/
	if (sb->s_ninode <= 0)
	{
		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		ino = -1;

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for (int i = 0; i < sb->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int* p = (int *)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用 */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if (mode != 0)
				{
					continue;
				}

				/*
				* 如果外存inode的i_mode==0，此时并不能确定
				* 该inode是空闲的，因为有可能是内存inode没有写到
				* 磁盘上,所以要继续搜索内存inode中是否有相应的项
				*/
				if (g_InodeTable.IsLoaded(ino) == -1)
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					sb->s_inode[sb->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if (sb->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			this->m_BufferManager->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}
		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if (sb->s_ninode <= 0)
		{
			printf("IAlloc:No Space !\n");
			u.u_error = User::my_ENOSPC;
			return NULL;
		}
	}

	/*
	* 上面部分已经保证，除非系统中没有可用外存Inode，
	* 否则空闲Inode索引表中必定会记录可用外存Inode的编号。
	*/
	while (true)
	{
		/* 从索引表“栈顶”获取空闲外存Inode编号 */
		ino = sb->s_inode[--sb->s_ninode];

		/* 将空闲Inode读入内存 */
		pNode = g_InodeTable.IGet(ino);
		/* 未能分配到内存inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		/* 如果该Inode空闲,清空Inode中的数据 */
		if (0 == pNode->i_mode)
		{
			pNode->Clean();
			/* 设置SuperBlock被修改标志 */
			sb->s_fmod = 1;
			return pNode;
		}
		else	/* 如果该Inode已被占用 */
		{
			g_InodeTable.IPut(pNode);
			continue;	/* while循环 */
		}
	}
	return NULL;	/* GCC likes it! */
}

//在存储设备dev上分配空闲磁盘块
Buf* FileSystem::Alloc()
{
	int blkno;	/* 分配到的空闲磁盘块编号 */
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	/* 获取SuperBlock对象的内存副本 */
	sb = this->GetFS();

	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = sb->s_free[--sb->s_nfree];

	/*
	* 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	* 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	* 都意味着分配空闲磁盘块操作失败。
	*/
	if (0 == blkno)
	{
		sb->s_nfree = 0;
		printf("Alloc:No Space !\n");
		u.u_error = User::my_ENOSPC;
		return NULL;
	}
	if (this->BadBlock(sb, blkno))
	{
		printf("Alloc:badblock !\n");
		return NULL;
	}

	/*
	* 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	* 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	*/
	if (sb->s_nfree <= 0)
	{
		/* 读入该空闲磁盘块 */
		pBuf = this->m_BufferManager->Bread( blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int *)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		sb->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		memcpy(sb->s_free, p, 400);
		//Utility::DWordCopy(p, sb->s_free, 100);

		/* 缓存使用完毕，释放以便被其它进程使用 */
		this->m_BufferManager->Brelse(pBuf);
	}

	/* 普通情况下成功分配到一空闲磁盘块 */
	pBuf = this->m_BufferManager->GetBlk( blkno);	/* 为该磁盘块申请缓存 */
	this->m_BufferManager->ClrBuf(pBuf);	/* 清空缓存中的数据 */
	sb->s_fmod = 1;	/* 设置SuperBlock被修改标志 */

	return pBuf;
}

//释放存储设备dev上编号为number的外存INode，一般用于删除文件。
void FileSystem::IFree( int number)
{
	SuperBlock* sb;

	sb = this->GetFS();	/* 获取相应设备的SuperBlock内存副本 */

	/*
	* 如果超级块直接管理的空闲外存Inode超过100个，
	* 同样让释放的外存Inode散落在磁盘Inode区中。
	*/
	if (sb->s_ninode >= 100)
	{
		return;
	}

	sb->s_inode[sb->s_ninode++] = number;

	/* 设置SuperBlock被修改标志 */
	sb->s_fmod = 1;
}

//释放存储设备dev上编号为blkno的磁盘块
void FileSystem::Free( int blkno)
{
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	sb = this->GetFS();

	/*
	* 尽早设置SuperBlock被修改标志，以防止在释放
	* 磁盘块Free()执行过程中，对SuperBlock内存副本
	* 的修改仅进行了一半，就更新到磁盘SuperBlock去
	*/
	sb->s_fmod = 1;	

	/*
	* 如果先前系统中已经没有空闲盘块，
	* 现在释放的是系统中第1块空闲盘块
	*/
	if (sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* 使用0标记空闲盘块链结束标志 */
	}

	/* SuperBlock中直接管理空闲磁盘块号的栈已满 */
	if (sb->s_nfree >= 100)
	{
		/*
		* 使用当前Free()函数正要释放的磁盘块，存放前一组100个空闲
		* 磁盘块的索引表
		*/
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为当前正要释放的磁盘块分配缓存 */
	
		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int *)pBuf->b_addr;

		/* 首先写入空闲盘块数，除了第一组为99块，后续每组都是100块 */
		*p++ = sb->s_nfree;
		/* 将SuperBlock的空闲盘块索引表s_free[100]写入缓存中后续位置 */
		memcpy(p, sb->s_free, 400);
		//Utility::DWordCopy(sb->s_free, p, 100);

		sb->s_nfree = 0;
		/* 将存放空闲盘块索引表的“当前释放盘块”写入磁盘，即实现了空闲盘块记录空闲盘块号的目标 */
		this->m_BufferManager->Bwrite(pBuf);

	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock中记录下当前释放盘块号 */
	sb->s_fmod = 1;
}

//检查设备dev上编号blkno的磁盘块是否属于数据盘块区
bool FileSystem::BadBlock(SuperBlock *spb, int blkno)
{
	return 0;
}



/*==========================class FileManager===============================*/
FileManager::FileManager()
{
	//nothing to do here
}

FileManager::~FileManager()
{
	//nothing to do here
}

//初始化对全局对象的引用
void FileManager::Initialize()
{
	this->m_FileSystem = &Kernel::Instance().GetFileSystem();

	this->m_InodeTable = &g_InodeTable;
	this->m_OpenFileTable = &g_OpenFileTable;
	this->m_gspb = &g_spb;

	this->m_InodeTable->Initialize();
}

/*
* 功能：打开文件
* 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
* */
void FileManager::Open()
{
	// printf("FileManager:Open\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* 没有找到相应的Inode */
	if (NULL == pInode)
	{
		printf("\n");
		return;
	}
	this->Open1(pInode, u.u_arg[1], 0);
}

/*
* 功能：创建一个新的文件
* 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（应该是 1）
* */
void FileManager::Creat()
{
	// printf("FileManager:Creat\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();
	unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

	/* 搜索目录的模式为1，表示创建；若父目录不可写，出错返回 */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* 没有找到相应的Inode，或NameI出错 */
	if (NULL == pInode)
	{
		if (u.u_error)
		{
			printf("Fail to create\n");
			return;
		}
		/* 创建Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		/* 创建失败 */
		if (NULL == pInode)
		{
			printf("Fail to create\n");
			exit(1);
			return;
		}

		/*
		* 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		* 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		* 所表示的权限内容是一样的。
		*/
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）。UID没有改变
		* 原来UNIX的设计是这样：文件看上去就像新建的文件一样。然而，新文件所有者和许可权方式没变。
		* 也就是说creat指定的RWX比特无效。
		* 邓蓉认为这是不合理的，应该改变。
		* 现在的实现：creat指定的RWX比特有效 */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}

/*
* trf == 0由open调用
* trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
* trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
* mode参数：打开文件模式，表示文件操作是 读、写还是读写
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	// printf("FileManager:Open1\n");
	User& u = Kernel::Instance().GetUser();

	/*
	* 对所希望的文件已存在的情况下，即trf == 0或trf == 1进行权限检查
	* 如果所希望的名字不存在，即trf == 2，不需要进行权限检查，因为刚建立
	* 的文件的权限和传入的参数mode的所表示的权限内容是一样的。
	*/
	if (trf != 2)
	{
		if (mode & File::FREAD)
		{
			/* 检查读权限 */
			this->Access(pInode, Inode::IREAD);
		}
		if (mode & File::FWRITE)
		{
			/* 检查写权限 */
			this->Access(pInode, Inode::IWRITE);
			/* 系统调用去写目录文件是不允许的 */
			if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
			{
				printf("Unable to write directory file !\n");
				u.u_error = User::my_EISDIR;
			}
		}
	}

	if (u.u_error)
	{
		printf("Open1:u_error\n");
		this->m_InodeTable->IPut(pInode);
		return;
	}

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
	if (1 == trf)
	{
		printf("Exist a file with the same name, release disk blocks occupied by the Inode file\n");
		pInode->ITrunc(); //释放Inode对应文件占用的磁盘块
	}

	/* 分配打开文件控制块File结构 */
	File* pFile = this->m_OpenFileTable->FAlloc();
	if (NULL == pFile)
	{
		printf("Fail to allocate File block, release inode\n");
		this->m_InodeTable->IPut(pInode); //没有空闲项，释放节点
		return;
	}
	/* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
	if (u.u_error == 0)
	{
		// printf("Open1:success\n");
		return;
	}
}

//关闭文件
void FileManager::Close()
{
	// printf("FileManager:Close\n");
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	/* 获取打开文件控制块File结构 */
	File* pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		printf("Close Error:No File structure for fd found\n");
		return;
	}

	/* 释放打开文件描述符fd，递减File结构引用计数 */
	u.u_ofiles.SetF(fd, NULL);
	this->m_OpenFileTable->CloseF(pFile);
	// printf("Close:Success\n");
}

//调整文件的读写位置
void FileManager::Seek()
{
	// printf("FileManager.Seek\n");
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		printf("Seek Error:No File structure for fd found\n");
		return;  /* 若FILE不存在，GetF有设出错码 */
	}

	int offset = u.u_arg[1];

	/* 如果u.u_arg[2]在3 ~ 5之间，那么长度单位由字节变为512字节 */
	if (u.u_arg[2] > 2)
	{
		offset = offset << 9;
		u.u_arg[2] -= 3;
	}

	switch (u.u_arg[2])
	{
		/* 读写位置设置为offset */
	case 0:
		pFile->f_offset = offset;
		break;
		/* 读写位置加offset(可正可负) */
	case 1:
		pFile->f_offset += offset;
		break;
		/* 读写位置调整为文件长度加offset */
	case 2:
		pFile->f_offset = pFile->f_inode->i_size + offset;
		break;
	}
}

//获取文件信息
void FileManager::FStat()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd); //获取fd对应的File结构
	if (NULL == pFile)
	{
		return;
	}

	/* u.u_arg[1] = pStatBuf */
	this->Stat1(pFile->f_inode, u.u_arg[1]);
}

//获取文件信息
void FileManager::Stat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)
	{
		return;
	}
	this->Stat1(pInode, u.u_arg[1]);
	this->m_InodeTable->IPut(pInode);
}

//FStat()和Stat()系统调用的共享例程
void FileManager::Stat1(Inode* pInode, unsigned long statBuf)
{
	Buf* pBuf;
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	pInode->IUpdate();//更新外存Inode的最后的访问时间、修改时间
	pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

	/* 将p指向缓存区中编号为inumber外存Inode的偏移位置 */
	unsigned char* p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	memcpy(&statBuf, p, sizeof(statBuf));
//	Utility::DWordCopy((int *)p, (int *)statBuf, sizeof(DiskInode) / sizeof(int));

	bufMgr.Brelse(pBuf);
}

//Read()系统调用处理过程
void FileManager::Read()
{
	// printf("FileManager.Read\n");
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FREAD);
}

//Write()系统调用处理过程
void FileManager::Write()
{
	// printf("FileManager.Write\n");
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FWRITE);
}

//读写系统调用公共部分代码
void FileManager::Rdwr(enum File::FileFlags mode)
{
	// printf("FileManager.Rdwr\n");
	File* pFile;
	User& u = Kernel::Instance().GetUser();

	/* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if (NULL == pFile)
	{
		/* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
		/*	u.u_error = User::EBADF;	*/
		return;
	}

	/* 读写的模式不正确 */
	if ((pFile->f_flag & mode) == 0)
	{
		printf("Rdwr Error : mode error\n");
		u.u_error = User::my_EACCES;
		return;
	}
	u.u_IOParam.m_Base = (unsigned char *)u.u_arg[1];	/* 目标缓冲区首址 */
	u.u_IOParam.m_Count = u.u_arg[2];		/* 要求读/写的字节数 */
	
	/* 普通文件读写 ，或读写特殊文件。对文件实施互斥访问，互斥的粒度：每次系统调用。
	为此Inode类需要增加两个方法：NFlock()、NFrele()。
	这不是V6的设计。read、write系统调用对内存i节点上锁是为了给实施IO的进程提供一致的文件视图。*/
	
	/* 设置文件起始读位置 */
	u.u_IOParam.m_Offset = pFile->f_offset;
	if (File::FREAD == mode)
	{
		// printf("ReadI\n");
		pFile->f_inode->ReadI();
	}
	else
	{
		// printf("WriteI\n");
		pFile->f_inode->WriteI();
	}
	// cout<<"u.u_arg[2]:"<<u.u_arg[2]<<endl;
	// cout<<"u.u_IOParam.m_Count:"<<u.u_IOParam.m_Count<<endl;
	// cout<<"u.u_IOParam.m_Base:"<<u.u_IOParam.m_Base<<endl;
	/* 根据读写字数，移动文件读写偏移指针 */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);
		
	/* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
	u.u_ar0 = u.u_arg[2] - u.u_IOParam.m_Count;
	// cout<<"u.u_ar0:"<<u.u_ar0<<endl;
}

/* 返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 ，上锁的内存i节点  */
Inode* FileManager::NameI(char(*func)(), enum DirectorySearchMode mode)
{
	// printf("FileManager.NameI\n");
	Inode* pInode;
	Buf* pBuf;
	char curchar;
	char* pChar;
	int freeEntryOffset;	/* 以创建文件模式搜索目录时，记录空闲目录项的偏移量 */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	/*
	* 如果该路径是'/'开头的，从根目录开始搜索，
	* 否则从进程当前工作目录开始搜索。
	*/
	pInode = u.u_cdir;

	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;
	}

	/* 检查该Inode是否正在被使用，以及保证在整个目录搜索过程中该Inode不被释放 */
	this->m_InodeTable->IGet( pInode->i_number);

	/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();
	}

	/* 如果试图更改和删除当前目录文件则出错 */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		printf("Error occurred trying to change and delete files in the current directory\n");
		u.u_error = User::my_ENOENT;
		goto out;
	}

	/* 外层循环每次处理pathname中一段路径分量 */
	while (true)
	{
		/* 如果出错则释放当前搜索到的目录文件Inode，并退出 */
		if (u.u_error != User::my_NOERROR)
		{
			printf("NameI:error\n");
			break;	/* goto out; */
		}

		/* 整个路径搜索完毕，返回相应Inode指针。目录搜索成功返回。 */
		if ('\0' == curchar)
		{
			return pInode;
		}

		/* 如果要进行搜索的不是目录文件，释放相关Inode资源则退出 */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			printf("NameI:Not directory file\n");
			u.u_error = User::my_ENOTDIR;
			break;	/* goto out; */
		}

		/* 进行目录搜索权限检查,IEXEC在目录文件中表示搜索权限 */
		if (this->Access(pInode, Inode::IEXEC))
		{
			printf("NameI:no authority \n");
			u.u_error = User::my_EACCES;
			break;	/* 不具备目录搜索权限，goto out; */
		}

		/*
		* 将Pathname中当前准备进行匹配的路径分量拷贝到u.u_dbuf[]中，
		* 便于和目录项进行比较。
		*/
		pChar = &(u.u_dbuf[0]);
		while ('/' != curchar && '\0' != curchar && u.u_error == User::my_NOERROR)
		{
			if (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
			{
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();
		}
		/* 将u_dbuf剩余的部分填充为'\0' */
		while (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();
		}

		if (u.u_error != User::my_NOERROR)
		{
			break; /* goto out; */
		}

		/* 内层循环部分对于u.u_dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
		u.u_IOParam.m_Offset = 0;
		/* 设置为目录项个数 ，含空白的目录项*///注：这个pinode指向需要搜索的目录文件
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* 对目录项已经搜索完毕 */
			if (0 == u.u_IOParam.m_Count)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);
				}
				/* 如果是创建新文件 */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* 判断该目录是否可写 */
					if (this->Access(pInode, Inode::IWRITE))
					{
						printf("NameI:fail to write directory file \n");
						u.u_error = User::my_EACCES;
						goto out;	/* Failed */
					}

					/* 将父目录Inode指针保存起来，以后写目录项WriteDir()函数会用到 */
					u.u_pdir = pInode;

					if (freeEntryOffset)	/* 此变量存放了空闲目录项位于目录文件中的偏移量 */
					{
						/* 将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到 */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else  /*问题：为何if分支没有置IUPD标志？  这是因为文件的长度没有变呀*/
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* 找到可以写入的空闲目录项位置，NameI()函数返回 */
				//	cout << "找到可以写入的空闲目录项位置NameI返回NULL" << endl;
					return NULL;
				}

				/* 目录项搜索完毕而没有找到匹配项，释放相关Inode资源，并推出 */
				u.u_error = User::my_ENOENT;
				goto out;
			}

			/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
			if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);
				}
				/* 计算要读的物理盘块号 */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
				pBuf = bufMgr.Bread( phyBlkno);
			}

			/* 没有读完当前目录项盘块，则读取下一目录项至u.u_dent */
			int* src = (int *)(pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE));
			memcpy(&u.u_dent, src, sizeof(DirectoryEntry));
			//Utility::DWordCopy(src, (int *)&u.u_dent, sizeof(DirectoryEntry) / sizeof(int));

			u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			u.u_IOParam.m_Count--;

			/* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
			if (0 == u.u_dent.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = u.u_IOParam.m_Offset;
				}
				/* 跳过空闲目录项，继续比较下一目录项 */
				continue;
			}

			int i;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (u.u_dbuf[i] != u.u_dent.m_name[i])
				{
					break;	/* 匹配至某一字符不符，跳出for循环 */
				}
			}

			if (i < DirectoryEntry::DIRSIZ)
			{
				/* 不是要搜索的目录项，继续匹配下一目录项 */
				continue;
			}
			else
			{
				/* 目录项匹配成功，回到外层While(true)循环 */
				break;
			}
		}

		/*
		* 从内层目录项匹配循环跳至此处，说明pathname中
		* 当前路径分量匹配成功了，还需匹配pathname中下一路径
		* 分量，直至遇到'\0'结束。
		*/
		if (NULL != pBuf)
		{
			bufMgr.Brelse(pBuf);
		}

		/* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在u.u_dent.m_ino中 */
		if (FileManager::DELETE == mode && '\0' == curchar)
		{
			/* 如果对父目录没有写的权限 */
			if (this->Access(pInode, Inode::IWRITE))
			{
				u.u_error = User::my_EACCES;
				break;	/* goto out; */
			}
			return pInode;
		}

		/*
		* 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
		* 目录项m_ino字段获取相应下一级目录或文件的Inode。
		*/
		short dev = pInode->i_dev;
		this->m_InodeTable->IPut(pInode);
		pInode = this->m_InodeTable->IGet( u.u_dent.m_ino);
		/* 回到外层While(true)循环，继续匹配Pathname中下一路径分量 */

		if (NULL == pInode)	/* 获取失败 */
		{
			printf("NameI : fail to get pInode\n");
			return NULL;
		}
	}
out:
	this->m_InodeTable->IPut(pInode);
	printf("NameI : error ! return NULL ! \n");
	return NULL;
}

//获取路径中的下一个字符
char FileManager::NextChar()
{
	User& u = Kernel::Instance().GetUser();

	/* u.u_dirp指向pathname中的字符 */
	return *u.u_dirp++;
}

/* 由creat调用。
* 为新创建的文件写新的i节点和新的目录项
* 返回的pInode是上了锁的内存i节点，其中的i_count是 1。
*
* 在程序的最后会调用 WriteDir，在这里把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
*
*/
Inode* FileManager::MakNode(unsigned int mode)
{
	// printf("FileManager.MakNode\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* 分配一个空闲DiskInode，里面内容已全部清空 */
	pInode = this->m_FileSystem->IAlloc();
	if (NULL == pInode)
	{
		printf("MakNode:No free Inode\n");
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;
	/* 将目录项写入u.u_dent，随后写入目录文件 */
	this->WriteDir(pInode);
	return pInode;
}

//向父目录的目录文件写入一个目录项
void FileManager::WriteDir(Inode* pInode)
{
	// printf("FileManager.WriteDir\n");
	User& u = Kernel::Instance().GetUser();

	/* 设置目录项中Inode编号部分 */
	u.u_dent.m_ino = pInode->i_number;

	/* 设置目录项中pathname分量部分 */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;

	/* 将目录项写入父目录文件 */
	u.u_pdir->WriteI();
	this->m_InodeTable->IPut(u.u_pdir);
}

//设置当前工作路径
void FileManager::SetCurDir(char* pathname)
{
	User& u = Kernel::Instance().GetUser();

	/* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
	if (pathname[0] != '/')
	{
		int length = strlen(u.u_curdir);
		if (u.u_curdir[length - 1] != '/')
		{
			u.u_curdir[length] = '/';
			length++;
		}
		strcpy(u.u_curdir + length, pathname);
		//Utility::StringCopy(pathname, u.u_curdir + length);
	}
	else	/* 如果是从根目录'/'开始，则取代原有工作目录 */
	{
	//	cout << "将成功fanhui" << endl;
		strcpy(u.u_curdir, pathname);
		//Utility::StringCopy(pathname, u.u_curdir);
	}
}

/*
* 返回值是0，表示拥有打开文件的权限；1表示没有所需的访问权限。文件未能打开的原因记录在u.u_error变量中。
*/
int FileManager::Access(Inode* pInode, unsigned int mode)
{
	User& u = Kernel::Instance().GetUser();

	/* 对于写的权限，必须检查该文件系统是否是只读的 */
	if (Inode::IWRITE == mode)
	{
		if (this->m_FileSystem->GetFS()->s_ronly != 0)
		{
			printf("Access:Read only!\n");
			u.u_error = User::my_EROFS;
			return 1;
		}
	}
	/*
	* 对于超级用户，读写任何文件都是允许的
	* 而要执行某文件时，必须在i_mode有可执行标志
	*/
	if (true)
	{
		if (Inode::IEXEC == mode && (pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6))) == 0)
		{
			u.u_error = User::my_EACCES;
			printf("Access:Can't write!\n");
			return 1;
		}
		return 0;	/* Permission Check Succeed! */
	}

	u.u_error = User::my_EACCES;
	return 1;
}

//改变当前工作目录
void FileManager::ChDir()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();
	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)
	{
		return;
	}
	/* 搜索到的文件不是目录文件 */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		printf("ChDir:not directory file\n");
		u.u_error = User::my_ENOTDIR;
		this->m_InodeTable->IPut(pInode);
		return;
	}
	if (this->Access(pInode, Inode::IEXEC))
	{
		printf("ChDir:access denied\n");
		this->m_InodeTable->IPut(pInode);
		return;
	}
	this->m_InodeTable->IPut(u.u_cdir);
	u.u_cdir = pInode;

	this->SetCurDir((char *)u.u_arg[0] /* pathname */);
}

//取消文件
void FileManager::UnLink()
{
    // printf("FIleManager.UnLink\n");
	Inode* pInode;
	Inode* pDeleteInode;
	User& u = Kernel::Instance().GetUser();

	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE);
	if (NULL == pDeleteInode)
	{
		printf("UnLink:The inode for the deleted file was not found\n");
		return;
	}

	pInode = this->m_InodeTable->IGet( u.u_dent.m_ino);
	if (NULL == pInode)
	{
		printf("unlink -- iget\n");
		//Utility::Panic("unlink -- iget");
	}
	/* 写入清零后的目录项 */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

	u.u_dent.m_ino = 0;
	pDeleteInode->WriteI();

	/* 修改inode项 */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	this->m_InodeTable->IPut(pDeleteInode);
	this->m_InodeTable->IPut(pInode);
}

//用于建立特殊设备文件的系统调用
void FileManager::MkNod()
{
	// printf("FileManager.MkNod\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* 检查uid是否是root，该系统调用只有uid==root时才可被调用 */
	if (true)
	{
		pInode = this->NameI(FileManager::NextChar, FileManager::CREATE);
		/* 要创建的文件已经存在,这里并不能去覆盖此文件 */
		if (pInode != NULL)
		{
			printf("MkNod:file exist\n");
			u.u_error = User::my_EEXIST;
			this->m_InodeTable->IPut(pInode);
			return;
		}
	}
	else
	{
		/* 非root用户执行mknod()系统调用返回User::EPERM */
		u.u_error = User::my_EPERM;
		return;
	}
	/* 没有通过SUser()的检查 */
	if (User::my_NOERROR != u.u_error)
	{
		return;	/* 没有需要释放的资源，直接退出 */
	}
	pInode = this->MakNode(u.u_arg[1]);
	if (NULL == pInode)
	{
		return;
	}
	/* 所建立是设备文件 */
	if ((pInode->i_mode & (Inode::IFBLK | Inode::IFCHR)) != 0)
	{
		pInode->i_addr[0] = u.u_arg[2];
	}
	this->m_InodeTable->IPut(pInode);
}


/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = 0;
	this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
	//nothing to do here
}