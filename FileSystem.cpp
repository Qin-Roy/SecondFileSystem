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
/* ϵͳȫ�ֳ�����SuperBlock���� */

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

//��ʼ����Ա����
void FileSystem::Initialize()
{
	this->m_BufferManager = &Kernel::Instance().GetBufferManager();
}

//ϵͳ��ʼ��ʱ����SuperBlock
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

//�����ļ��洢�豸���豸��dev��ȡ���ļ�ϵͳ��SuperBlock
SuperBlock* FileSystem::GetFS()
{
	return &g_spb;
}

//SuperBlock������ڴ渱�����µ��洢�豸��SuperBlock��ȥ
void FileSystem::Update()
{
	int i;
	SuperBlock* sb=&g_spb;
	Buf* pBuf;

	/* ͬ��SuperBlock������ */
	if (true)	/* ��Mountװ����Ӧĳ���ļ�ϵͳ */
		{
			/* �����SuperBlock�ڴ渱��û�б��޸ģ�ֱ�ӹ���inode�Ϳ����̿鱻��������ļ�ϵͳ��ֻ���ļ�ϵͳ */
			if (sb->s_fmod == 0 || sb->s_ronly != 0)
			{
				return;
			}

			/* ��SuperBlock�޸ı�־ */
			sb->s_fmod = 0;

			/*
			* Ϊ��Ҫд�ص�������ȥ��SuperBlock����һ�黺�棬���ڻ�����СΪ512�ֽڣ�
			* SuperBlock��СΪ1024�ֽڣ�ռ��2��������������������Ҫ2��д�������
			*/
			for (int j = 0; j < 2; j++)
			{
				/* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
				int* p = (int *)sb + j * 128;

				/* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
				pBuf = this->m_BufferManager->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

				/* ��SuperBlock�е�0 - 511�ֽ�д�뻺���� */
				memcpy(pBuf->b_addr, p, 512);
				//Utility::DWordCopy(p, (int *)pBuf->b_addr, 128);

				/* ���������е�����д�������� */
				this->m_BufferManager->Bwrite(pBuf);
			}
		}

	/* ͬ���޸Ĺ����ڴ�Inode����Ӧ���Inode */
	g_InodeTable.UpdateInodeTable();

	/* ���ӳ�д�Ļ����д�������� */
	this->m_BufferManager->Bflush();
}

//�ڴ洢�豸dev�Ϸ���һ���������INode��һ�����ڴ����µ��ļ���
Inode* FileSystem::IAlloc()
{
	SuperBlock* sb;
	Buf* pBuf;
	Inode* pNode;
	User& u = Kernel::Instance().GetUser();
	int ino;	/* ���䵽�Ŀ������Inode��� */

	/* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */
	sb = this->GetFS();

	/*
	* SuperBlockֱ�ӹ���Ŀ���Inode�������ѿգ�
	* ���뵽��������������Inode��
	* �ȶ�inode�б�������
	* ��Ϊ�����³����л���ж��̲������ܻᵼ�½����л���
	* ���������п��ܷ��ʸ����������ᵼ�²�һ���ԡ�
	*/
	if (sb->s_ninode <= 0)
	{
		/* ���Inode��Ŵ�0��ʼ���ⲻͬ��Unix V6�����Inode��1��ʼ��� */
		ino = -1;

		/* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
		for (int i = 0; i < sb->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* ��ȡ��������ַ */
			int* p = (int *)pBuf->b_addr;

			/* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
				if (mode != 0)
				{
					continue;
				}

				/*
				* ������inode��i_mode==0����ʱ������ȷ��
				* ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
				* ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				*/
				if (g_InodeTable.IsLoaded(ino) == -1)
				{
					/* �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������ */
					sb->s_inode[sb->s_ninode++] = ino;

					/* ��������������Ѿ�װ�����򲻼������� */
					if (sb->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
			this->m_BufferManager->Brelse(pBuf);

			/* ��������������Ѿ�װ�����򲻼������� */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}
		/* ����ڴ�����û���������κο������Inode������NULL */
		if (sb->s_ninode <= 0)
		{
			printf("IAlloc:No Space !\n");
			u.u_error = User::my_ENOSPC;
			return NULL;
		}
	}

	/*
	* ���沿���Ѿ���֤������ϵͳ��û�п������Inode��
	* �������Inode�������бض����¼�������Inode�ı�š�
	*/
	while (true)
	{
		/* ��������ջ������ȡ�������Inode��� */
		ino = sb->s_inode[--sb->s_ninode];

		/* ������Inode�����ڴ� */
		pNode = g_InodeTable.IGet(ino);
		/* δ�ܷ��䵽�ڴ�inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		/* �����Inode����,���Inode�е����� */
		if (0 == pNode->i_mode)
		{
			pNode->Clean();
			/* ����SuperBlock���޸ı�־ */
			sb->s_fmod = 1;
			return pNode;
		}
		else	/* �����Inode�ѱ�ռ�� */
		{
			g_InodeTable.IPut(pNode);
			continue;	/* whileѭ�� */
		}
	}
	return NULL;	/* GCC likes it! */
}

//�ڴ洢�豸dev�Ϸ�����д��̿�
Buf* FileSystem::Alloc()
{
	int blkno;	/* ���䵽�Ŀ��д��̿��� */
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	/* ��ȡSuperBlock������ڴ渱�� */
	sb = this->GetFS();

	/* ��������ջ������ȡ���д��̿��� */
	blkno = sb->s_free[--sb->s_nfree];

	/*
	* ����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿顣
	* ���߷��䵽�Ŀ��д��̿��Ų����������̿�������(��BadBlock()���)��
	* ����ζ�ŷ�����д��̿����ʧ�ܡ�
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
	* ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	* ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	*/
	if (sb->s_nfree <= 0)
	{
		/* ����ÿ��д��̿� */
		pBuf = this->m_BufferManager->Bread( blkno);

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->b_addr;

		/* ���ȶ��������̿���s_nfree */
		sb->s_nfree = *p++;

		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		memcpy(sb->s_free, p, 400);
		//Utility::DWordCopy(p, sb->s_free, 100);

		/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
		this->m_BufferManager->Brelse(pBuf);
	}

	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = this->m_BufferManager->GetBlk( blkno);	/* Ϊ�ô��̿����뻺�� */
	this->m_BufferManager->ClrBuf(pBuf);	/* ��ջ����е����� */
	sb->s_fmod = 1;	/* ����SuperBlock���޸ı�־ */

	return pBuf;
}

//�ͷŴ洢�豸dev�ϱ��Ϊnumber�����INode��һ������ɾ���ļ���
void FileSystem::IFree( int number)
{
	SuperBlock* sb;

	sb = this->GetFS();	/* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */

	/*
	* ���������ֱ�ӹ���Ŀ������Inode����100����
	* ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С�
	*/
	if (sb->s_ninode >= 100)
	{
		return;
	}

	sb->s_inode[sb->s_ninode++] = number;

	/* ����SuperBlock���޸ı�־ */
	sb->s_fmod = 1;
}

//�ͷŴ洢�豸dev�ϱ��Ϊblkno�Ĵ��̿�
void FileSystem::Free( int blkno)
{
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	sb = this->GetFS();

	/*
	* ��������SuperBlock���޸ı�־���Է�ֹ���ͷ�
	* ���̿�Free()ִ�й����У���SuperBlock�ڴ渱��
	* ���޸Ľ�������һ�룬�͸��µ�����SuperBlockȥ
	*/
	sb->s_fmod = 1;	

	/*
	* �����ǰϵͳ���Ѿ�û�п����̿飬
	* �����ͷŵ���ϵͳ�е�1������̿�
	*/
	if (sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* ʹ��0��ǿ����̿���������־ */
	}

	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if (sb->s_nfree >= 100)
	{
		/*
		* ʹ�õ�ǰFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		* ���̿��������
		*/
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */
	
		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->b_addr;

		/* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = sb->s_nfree;
		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		memcpy(p, sb->s_free, 400);
		//Utility::DWordCopy(sb->s_free, p, 100);

		sb->s_nfree = 0;
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		this->m_BufferManager->Bwrite(pBuf);

	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
	sb->s_fmod = 1;
}

//����豸dev�ϱ��blkno�Ĵ��̿��Ƿ����������̿���
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

//��ʼ����ȫ�ֶ��������
void FileManager::Initialize()
{
	this->m_FileSystem = &Kernel::Instance().GetFileSystem();

	this->m_InodeTable = &g_InodeTable;
	this->m_OpenFileTable = &g_OpenFileTable;
	this->m_gspb = &g_spb;

	this->m_InodeTable->Initialize();
}

/*
* ���ܣ����ļ�
* Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
* */
void FileManager::Open()
{
	// printf("FileManager:Open\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* û���ҵ���Ӧ��Inode */
	if (NULL == pInode)
	{
		printf("\n");
		return;
	}
	this->Open1(pInode, u.u_arg[1], 0);
}

/*
* ���ܣ�����һ���µ��ļ�
* Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������Ӧ���� 1��
* */
void FileManager::Creat()
{
	// printf("FileManager:Creat\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();
	unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

	/* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* û���ҵ���Ӧ��Inode����NameI���� */
	if (NULL == pInode)
	{
		if (u.u_error)
		{
			printf("Fail to create\n");
			return;
		}
		/* ����Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		/* ����ʧ�� */
		if (NULL == pInode)
		{
			printf("Fail to create\n");
			exit(1);
			return;
		}

		/*
		* �����ϣ�������ֲ����ڣ�ʹ�ò���trf = 2������open1()��
		* ����Ҫ����Ȩ�޼�飬��Ϊ�ոս������ļ���Ȩ�޺ʹ������mode
		* ����ʾ��Ȩ��������һ���ġ�
		*/
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()����UIDû�иı�
		* ԭ��UNIX��������������ļ�����ȥ�����½����ļ�һ����Ȼ�������ļ������ߺ����Ȩ��ʽû�䡣
		* Ҳ����˵creatָ����RWX������Ч��
		* ������Ϊ���ǲ�����ģ�Ӧ�øı䡣
		* ���ڵ�ʵ�֣�creatָ����RWX������Ч */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}

/*
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	// printf("FileManager:Open1\n");
	User& u = Kernel::Instance().GetUser();

	/*
	* ����ϣ�����ļ��Ѵ��ڵ�����£���trf == 0��trf == 1����Ȩ�޼��
	* �����ϣ�������ֲ����ڣ���trf == 2������Ҫ����Ȩ�޼�飬��Ϊ�ս���
	* ���ļ���Ȩ�޺ʹ���Ĳ���mode������ʾ��Ȩ��������һ���ġ�
	*/
	if (trf != 2)
	{
		if (mode & File::FREAD)
		{
			/* ����Ȩ�� */
			this->Access(pInode, Inode::IREAD);
		}
		if (mode & File::FWRITE)
		{
			/* ���дȨ�� */
			this->Access(pInode, Inode::IWRITE);
			/* ϵͳ����ȥдĿ¼�ļ��ǲ������ */
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

	/* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
	if (1 == trf)
	{
		printf("Exist a file with the same name, release disk blocks occupied by the Inode file\n");
		pInode->ITrunc(); //�ͷ�Inode��Ӧ�ļ�ռ�õĴ��̿�
	}

	/* ������ļ����ƿ�File�ṹ */
	File* pFile = this->m_OpenFileTable->FAlloc();
	if (NULL == pFile)
	{
		printf("Fail to allocate File block, release inode\n");
		this->m_InodeTable->IPut(pInode); //û�п�����ͷŽڵ�
		return;
	}
	/* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
	if (u.u_error == 0)
	{
		// printf("Open1:success\n");
		return;
	}
}

//�ر��ļ�
void FileManager::Close()
{
	// printf("FileManager:Close\n");
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	/* ��ȡ���ļ����ƿ�File�ṹ */
	File* pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		printf("Close Error:No File structure for fd found\n");
		return;
	}

	/* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
	u.u_ofiles.SetF(fd, NULL);
	this->m_OpenFileTable->CloseF(pFile);
	// printf("Close:Success\n");
}

//�����ļ��Ķ�дλ��
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
		return;  /* ��FILE�����ڣ�GetF��������� */
	}

	int offset = u.u_arg[1];

	/* ���u.u_arg[2]��3 ~ 5֮�䣬��ô���ȵ�λ���ֽڱ�Ϊ512�ֽ� */
	if (u.u_arg[2] > 2)
	{
		offset = offset << 9;
		u.u_arg[2] -= 3;
	}

	switch (u.u_arg[2])
	{
		/* ��дλ������Ϊoffset */
	case 0:
		pFile->f_offset = offset;
		break;
		/* ��дλ�ü�offset(�����ɸ�) */
	case 1:
		pFile->f_offset += offset;
		break;
		/* ��дλ�õ���Ϊ�ļ����ȼ�offset */
	case 2:
		pFile->f_offset = pFile->f_inode->i_size + offset;
		break;
	}
}

//��ȡ�ļ���Ϣ
void FileManager::FStat()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	pFile = u.u_ofiles.GetF(fd); //��ȡfd��Ӧ��File�ṹ
	if (NULL == pFile)
	{
		return;
	}

	/* u.u_arg[1] = pStatBuf */
	this->Stat1(pFile->f_inode, u.u_arg[1]);
}

//��ȡ�ļ���Ϣ
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

//FStat()��Stat()ϵͳ���õĹ�������
void FileManager::Stat1(Inode* pInode, unsigned long statBuf)
{
	Buf* pBuf;
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	pInode->IUpdate();//�������Inode�����ķ���ʱ�䡢�޸�ʱ��
	pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

	/* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
	unsigned char* p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	memcpy(&statBuf, p, sizeof(statBuf));
//	Utility::DWordCopy((int *)p, (int *)statBuf, sizeof(DiskInode) / sizeof(int));

	bufMgr.Brelse(pBuf);
}

//Read()ϵͳ���ô������
void FileManager::Read()
{
	// printf("FileManager.Read\n");
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FREAD);
}

//Write()ϵͳ���ô������
void FileManager::Write()
{
	// printf("FileManager.Write\n");
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FWRITE);
}

//��дϵͳ���ù������ִ���
void FileManager::Rdwr(enum File::FileFlags mode)
{
	// printf("FileManager.Rdwr\n");
	File* pFile;
	User& u = Kernel::Instance().GetUser();

	/* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if (NULL == pFile)
	{
		/* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
		/*	u.u_error = User::EBADF;	*/
		return;
	}

	/* ��д��ģʽ����ȷ */
	if ((pFile->f_flag & mode) == 0)
	{
		printf("Rdwr Error : mode error\n");
		u.u_error = User::my_EACCES;
		return;
	}
	u.u_IOParam.m_Base = (unsigned char *)u.u_arg[1];	/* Ŀ�껺������ַ */
	u.u_IOParam.m_Count = u.u_arg[2];		/* Ҫ���/д���ֽ��� */
	
	/* ��ͨ�ļ���д �����д�����ļ������ļ�ʵʩ������ʣ���������ȣ�ÿ��ϵͳ���á�
	Ϊ��Inode����Ҫ��������������NFlock()��NFrele()��
	�ⲻ��V6����ơ�read��writeϵͳ���ö��ڴ�i�ڵ�������Ϊ�˸�ʵʩIO�Ľ����ṩһ�µ��ļ���ͼ��*/
	
	/* �����ļ���ʼ��λ�� */
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
	/* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);
		
	/* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
	u.u_ar0 = u.u_arg[2] - u.u_IOParam.m_Count;
	// cout<<"u.u_ar0:"<<u.u_ar0<<endl;
}

/* ����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ� ���������ڴ�i�ڵ�  */
Inode* FileManager::NameI(char(*func)(), enum DirectorySearchMode mode)
{
	// printf("FileManager.NameI\n");
	Inode* pInode;
	Buf* pBuf;
	char curchar;
	char* pChar;
	int freeEntryOffset;	/* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	/*
	* �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
	* ����ӽ��̵�ǰ����Ŀ¼��ʼ������
	*/
	pInode = u.u_cdir;

	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;
	}

	/* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
	this->m_InodeTable->IGet( pInode->i_number);

	/* �������////a//b ����·�� ����·���ȼ���/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();
	}

	/* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		printf("Error occurred trying to change and delete files in the current directory\n");
		u.u_error = User::my_ENOENT;
		goto out;
	}

	/* ���ѭ��ÿ�δ���pathname��һ��·������ */
	while (true)
	{
		/* ����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */
		if (u.u_error != User::my_NOERROR)
		{
			printf("NameI:error\n");
			break;	/* goto out; */
		}

		/* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
		if ('\0' == curchar)
		{
			return pInode;
		}

		/* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			printf("NameI:Not directory file\n");
			u.u_error = User::my_ENOTDIR;
			break;	/* goto out; */
		}

		/* ����Ŀ¼����Ȩ�޼��,IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
		if (this->Access(pInode, Inode::IEXEC))
		{
			printf("NameI:no authority \n");
			u.u_error = User::my_EACCES;
			break;	/* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
		}

		/*
		* ��Pathname�е�ǰ׼������ƥ���·������������u.u_dbuf[]�У�
		* ���ں�Ŀ¼����бȽϡ�
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
		/* ��u_dbufʣ��Ĳ������Ϊ'\0' */
		while (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* �������////a//b ����·�� ����·���ȼ���/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();
		}

		if (u.u_error != User::my_NOERROR)
		{
			break; /* goto out; */
		}

		/* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
		u.u_IOParam.m_Offset = 0;
		/* ����ΪĿ¼����� �����հ׵�Ŀ¼��*///ע�����pinodeָ����Ҫ������Ŀ¼�ļ�
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* ��Ŀ¼���Ѿ�������� */
			if (0 == u.u_IOParam.m_Count)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);
				}
				/* ����Ǵ������ļ� */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* �жϸ�Ŀ¼�Ƿ��д */
					if (this->Access(pInode, Inode::IWRITE))
					{
						printf("NameI:fail to write directory file \n");
						u.u_error = User::my_EACCES;
						goto out;	/* Failed */
					}

					/* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
					u.u_pdir = pInode;

					if (freeEntryOffset)	/* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
					{
						/* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else  /*���⣺Ϊ��if��֧û����IUPD��־��  ������Ϊ�ļ��ĳ���û�б�ѽ*/
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
				//	cout << "�ҵ�����д��Ŀ���Ŀ¼��λ��NameI����NULL" << endl;
					return NULL;
				}

				/* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����Ƴ� */
				u.u_error = User::my_ENOENT;
				goto out;
			}

			/* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
			if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);
				}
				/* ����Ҫ���������̿�� */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
				pBuf = bufMgr.Bread( phyBlkno);
			}

			/* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����u.u_dent */
			int* src = (int *)(pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE));
			memcpy(&u.u_dent, src, sizeof(DirectoryEntry));
			//Utility::DWordCopy(src, (int *)&u.u_dent, sizeof(DirectoryEntry) / sizeof(int));

			u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			u.u_IOParam.m_Count--;

			/* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
			if (0 == u.u_dent.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = u.u_IOParam.m_Offset;
				}
				/* ��������Ŀ¼������Ƚ���һĿ¼�� */
				continue;
			}

			int i;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (u.u_dbuf[i] != u.u_dent.m_name[i])
				{
					break;	/* ƥ����ĳһ�ַ�����������forѭ�� */
				}
			}

			if (i < DirectoryEntry::DIRSIZ)
			{
				/* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
				continue;
			}
			else
			{
				/* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
				break;
			}
		}

		/*
		* ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
		* ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
		* ������ֱ������'\0'������
		*/
		if (NULL != pBuf)
		{
			bufMgr.Brelse(pBuf);
		}

		/* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
		if (FileManager::DELETE == mode && '\0' == curchar)
		{
			/* ����Ը�Ŀ¼û��д��Ȩ�� */
			if (this->Access(pInode, Inode::IWRITE))
			{
				u.u_error = User::my_EACCES;
				break;	/* goto out; */
			}
			return pInode;
		}

		/*
		* ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
		* Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
		*/
		short dev = pInode->i_dev;
		this->m_InodeTable->IPut(pInode);
		pInode = this->m_InodeTable->IGet( u.u_dent.m_ino);
		/* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

		if (NULL == pInode)	/* ��ȡʧ�� */
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

//��ȡ·���е���һ���ַ�
char FileManager::NextChar()
{
	User& u = Kernel::Instance().GetUser();

	/* u.u_dirpָ��pathname�е��ַ� */
	return *u.u_dirp++;
}

/* ��creat���á�
* Ϊ�´������ļ�д�µ�i�ڵ���µ�Ŀ¼��
* ���ص�pInode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
*
* �ڳ����������� WriteDir��������������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
*
*/
Inode* FileManager::MakNode(unsigned int mode)
{
	// printf("FileManager.MakNode\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* ����һ������DiskInode������������ȫ����� */
	pInode = this->m_FileSystem->IAlloc();
	if (NULL == pInode)
	{
		printf("MakNode:No free Inode\n");
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;
	/* ��Ŀ¼��д��u.u_dent�����д��Ŀ¼�ļ� */
	this->WriteDir(pInode);
	return pInode;
}

//��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼��
void FileManager::WriteDir(Inode* pInode)
{
	// printf("FileManager.WriteDir\n");
	User& u = Kernel::Instance().GetUser();

	/* ����Ŀ¼����Inode��Ų��� */
	u.u_dent.m_ino = pInode->i_number;

	/* ����Ŀ¼����pathname�������� */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;

	/* ��Ŀ¼��д�븸Ŀ¼�ļ� */
	u.u_pdir->WriteI();
	this->m_InodeTable->IPut(u.u_pdir);
}

//���õ�ǰ����·��
void FileManager::SetCurDir(char* pathname)
{
	User& u = Kernel::Instance().GetUser();

	/* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
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
	else	/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
	{
	//	cout << "���ɹ�fanhui" << endl;
		strcpy(u.u_curdir, pathname);
		//Utility::StringCopy(pathname, u.u_curdir);
	}
}

/*
* ����ֵ��0����ʾӵ�д��ļ���Ȩ�ޣ�1��ʾû������ķ���Ȩ�ޡ��ļ�δ�ܴ򿪵�ԭ���¼��u.u_error�����С�
*/
int FileManager::Access(Inode* pInode, unsigned int mode)
{
	User& u = Kernel::Instance().GetUser();

	/* ����д��Ȩ�ޣ���������ļ�ϵͳ�Ƿ���ֻ���� */
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
	* ���ڳ����û�����д�κ��ļ����������
	* ��Ҫִ��ĳ�ļ�ʱ��������i_mode�п�ִ�б�־
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

//�ı䵱ǰ����Ŀ¼
void FileManager::ChDir()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();
	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)
	{
		return;
	}
	/* ���������ļ�����Ŀ¼�ļ� */
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

//ȡ���ļ�
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
	/* д��������Ŀ¼�� */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

	u.u_dent.m_ino = 0;
	pDeleteInode->WriteI();

	/* �޸�inode�� */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	this->m_InodeTable->IPut(pDeleteInode);
	this->m_InodeTable->IPut(pInode);
}

//���ڽ��������豸�ļ���ϵͳ����
void FileManager::MkNod()
{
	// printf("FileManager.MkNod\n");
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* ���uid�Ƿ���root����ϵͳ����ֻ��uid==rootʱ�ſɱ����� */
	if (true)
	{
		pInode = this->NameI(FileManager::NextChar, FileManager::CREATE);
		/* Ҫ�������ļ��Ѿ�����,���ﲢ����ȥ���Ǵ��ļ� */
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
		/* ��root�û�ִ��mknod()ϵͳ���÷���User::EPERM */
		u.u_error = User::my_EPERM;
		return;
	}
	/* û��ͨ��SUser()�ļ�� */
	if (User::my_NOERROR != u.u_error)
	{
		return;	/* û����Ҫ�ͷŵ���Դ��ֱ���˳� */
	}
	pInode = this->MakNode(u.u_arg[1]);
	if (NULL == pInode)
	{
		return;
	}
	/* ���������豸�ļ� */
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