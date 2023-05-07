#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Buf.h"

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* ������ƿ顢������������ */
	static const int BUFFER_SIZE = 512;	/* ��������С�� ���ֽ�Ϊ��λ */

public:
	BufferManager();
	~BufferManager();

	void Initialize(char *start);/* ������ƿ���еĳ�ʼ������������ƿ���b_addrָ����Ӧ�������׵�ַ��*/

	Buf* GetBlk(int blkno);	/* ����һ�黺�棬���ڶ�д�豸dev�ϵ��ַ���blkno��*/
	void Brelse(Buf* bp);	/* �ͷŻ�����ƿ�buf */

	Buf* Bread(int blkno);	/* ��һ�����̿顣devΪ�������豸�ţ�blknoΪĿ����̿��߼���š� */
	
	void Bwrite(Buf* bp);	/* дһ�����̿� */
	void Bdwrite(Buf* bp);	/* �ӳ�д���̿� */
	void Bawrite(Buf* bp);	/* �첽д���̿� */

	void ClrBuf(Buf* bp);	/* ��ջ��������� */
	void Bflush();			/* ��devָ���豸�������ӳ�д�Ļ���ȫ����������� */

	Buf& GetBFreeList();	/* ��ȡ���ɻ�����п��ƿ�Buf�������� */

private:
	Buf* InCore(int blkno);	/* ���ָ���ַ����Ƿ����ڻ����� */

private:
	Buf bFreeList;			/* ���ɻ�����п��ƿ� */
	Buf m_Buf[NBUF];		/* ������ƿ����� */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* ���������� */

	// ��mmapӳ�䵽�ڴ�����ʼ��ַ
	char *p;
};

#endif
