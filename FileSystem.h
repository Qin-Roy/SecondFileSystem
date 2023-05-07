#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include"INode.h"
#include"File.h"
#include"OpenFileManager.h"
#include"BufferManager.h"

class OpenFileTable;
class InodeTable;

/*
* �ļ�ϵͳ�洢��Դ�����(Super Block)�Ķ��塣
*/
class SuperBlock
{
	/* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();

	/* Members */
public:
	int		s_isize;		/* ���Inode��ռ�õ��̿��� */
	int		s_fsize;		/* �̿����� */

	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[100];	/* ֱ�ӹ���Ŀ����̿������� */

	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[100];	/* ֱ�ӹ���Ŀ������Inode������ */

	int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	int		s_ronly;		/* ���ļ�ϵͳֻ�ܶ��� */
	int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[49];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */

};
/*
* �ļ�ϵͳ��(FileSystem)�����ļ��洢�豸��
* �ĸ���洢��Դ�����̿顢���INode�ķ��䡢
* �ͷš�
*/
class FileSystem
{
public:
	/* static consts */
	static const int SUPER_BLOCK_SECTOR_NUMBER = 0;	/* ����SuperBlockλ�ڴ����ϵ������ţ�ռ��1��2���������� */

	static const int ROOTINO = 0;			/* �ļ�ϵͳ��Ŀ¼���Inode��� */

	static const int INODE_NUMBER_PER_SECTOR = 8;		/* ���INode���󳤶�Ϊ64�ֽڣ�ÿ�����̿���Դ��512/64 = 8�����Inode */
	static const int INODE_ZONE_START_SECTOR = 2;		/* ���Inode��λ�ڴ����ϵ���ʼ������ */
	static const int INODE_ZONE_SIZE = 27 - 2;		   /* ���������Inode��ռ�ݵ������� */ //ע�����޸�

	static const int DATA_ZONE_START_SECTOR = 27;		/* ����������ʼ������ */    //ע�����޸�
	static const int DATA_ZONE_END_SECTOR = 1000 - 1;	/* �������Ľ��������� */
	static const int DATA_ZONE_SIZE = 1000 - DATA_ZONE_START_SECTOR;	/* ������ռ�ݵ��������� */

	/* Functions */
public:
	/* Constructors */
	FileSystem();
	/* Destructors */
	~FileSystem();

	/*
	* @comment ��ʼ����Ա����
	*/
	void Initialize();

	/*
	* @comment ϵͳ��ʼ��ʱ����SuperBlock
	*/
	void LoadSuperBlock();

	/*
	* @comment �����ļ��洢�豸���豸��dev��ȡ
	* ���ļ�ϵͳ��SuperBlock
	*/
	SuperBlock* GetFS();
	/*
	* @comment ��SuperBlock������ڴ渱�����µ�
	* �洢�豸��SuperBlock��ȥ
	*/
	void Update();

	/*
	* @comment  �ڴ洢�豸dev�Ϸ���һ������
	* ���INode��һ�����ڴ����µ��ļ���
	*/
	Inode* IAlloc();		
	/*
	* @comment  �ͷŴ洢�豸dev�ϱ��Ϊnumber
	* �����INode��һ������ɾ���ļ���
	*/
	void IFree( int number);

	/*
	* @comment �ڴ洢�豸dev�Ϸ�����д��̿�
	*/
	Buf* Alloc();
	/*
	* @comment �ͷŴ洢�豸dev�ϱ��Ϊblkno�Ĵ��̿�
	*/
	void Free( int blkno);

private:
	/*
	* @comment ����豸dev�ϱ��blkno�Ĵ��̿��Ƿ�����
	* �����̿���
	*/
	bool BadBlock(SuperBlock* spb,  int blkno);

	/* Members */
public:

private:
	BufferManager* m_BufferManager;		/* FileSystem����Ҫ�������ģ��(BufferManager)�ṩ�Ľӿ� */

};


/*
* �ļ�������(FileManager)
* ��װ���ļ�ϵͳ�ĸ���ϵͳ�����ں���̬�´�����̣�
* ����ļ���Open()��Close()��Read()��Write()�ȵ�
* ��װ�˶��ļ�ϵͳ���ʵľ���ϸ�ڡ�
*/
class FileManager
{
public:
	/* Ŀ¼����ģʽ������NameI()���� */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* �Դ��ļ���ʽ����Ŀ¼ */
		CREATE = 1,		/* ���½��ļ���ʽ����Ŀ¼ */
		DELETE = 2		/* ��ɾ���ļ���ʽ����Ŀ¼ */
	};

	/* Functions */
public:
	/* Constructors */
	FileManager();
	/* Destructors */
	~FileManager();


	/*
	* @comment ��ʼ����ȫ�ֶ��������
	*/
	void Initialize();

	/*
	* @comment Open()ϵͳ���ô������
	*/
	void Open();

	/*
	* @comment Creat()ϵͳ���ô������
	*/
	void Creat();

	/*
	* @comment Open()��Creat()ϵͳ���õĹ�������
	*/
	void Open1(Inode* pInode, int mode, int trf);

	/*
	* @comment Close()ϵͳ���ô������
	*/
	void Close();

	/*
	* @comment Seek()ϵͳ���ô������
	*/
	void Seek();

	/*
	* @comment FStat()��ȡ�ļ���Ϣ
	*/
	void FStat();

	/*
	* @comment FStat()��ȡ�ļ���Ϣ
	*/
	void Stat();

	/* FStat()��Stat()ϵͳ���õĹ������� */
	void Stat1(Inode* pInode, unsigned long statBuf);

	/*
	* @comment Read()ϵͳ���ô������
	*/
	void Read();

	/*
	* @comment Write()ϵͳ���ô������
	*/
	void Write();

	/*
	* @comment ��дϵͳ���ù������ִ���
	*/
	void Rdwr(enum File::FileFlags mode);

	/*
	* @comment Ŀ¼��������·��ת��Ϊ��Ӧ��Inode��
	* �����������Inode
	*/
	Inode* NameI(char(*func)(), enum DirectorySearchMode mode);

	/*
	* @comment ��ȡ·���е���һ���ַ�
	*/
	static char NextChar();

	/*
	* @comment ��Creat()ϵͳ����ʹ�ã�����Ϊ�������ļ������ں���Դ
	*/
	Inode* MakNode(unsigned int mode);

	/*
	* @comment ��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼��
	*/
	void WriteDir(Inode* pInode);

	/*
	* @comment ���õ�ǰ����·��
	*/
	void SetCurDir(char* pathname);

	/*
	* @comment �����ļ���Ŀ¼������������Ȩ�ޣ���Ϊϵͳ���õĸ�������
	*/
	int Access(Inode* pInode, unsigned int mode);

	/* �ı䵱ǰ����Ŀ¼ */
	void ChDir();

	/* ȡ���ļ� */
	void UnLink();

	/*����Ŀ¼*/
	void MkNod();

public:
	/* ��Ŀ¼�ڴ�Inode */
	Inode* rootDirInode;

	/* ��ȫ�ֶ���g_FileSystem�����ã��ö���������ļ�ϵͳ�洢��Դ */
	FileSystem* m_FileSystem;

	/* ��ȫ�ֶ���g_InodeTable�����ã��ö������ڴ�Inode��Ĺ��� */
	InodeTable* m_InodeTable;

	/* ��ȫ�ֶ���g_OpenFileTable�����ã��ö�������ļ�����Ĺ��� */
	OpenFileTable* m_OpenFileTable;

	/* ��ȫ�ֶ���g_spb������*/
	SuperBlock *m_gspb;
};


//================================================filedirectory====================================================//

class DirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* Ŀ¼����·�����ֵ�����ַ������� */

	/* Functions */
public:
	/* Constructors */
	DirectoryEntry();
	/* Destructors */
	~DirectoryEntry();

	/* Members */
public:
	int m_ino;		/* Ŀ¼����Inode��Ų��� */
	char m_name[DIRSIZ];	/* Ŀ¼����·�������� */
};

#endif





