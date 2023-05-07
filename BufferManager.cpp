#include <iostream>
#include <string.h>

#include "BufferManager.h"
#include "Kernel.h"
using namespace std;

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

/* ������ƿ���еĳ�ʼ������������ƿ���b_addrָ����Ӧ�������׵�ַ��*/
void BufferManager::Initialize(char *start)
{
	this->p = start;
	int i;
	Buf* bp; //��ǰ�����
	// ��ʼʱbFreeList��b_forw��b_back��ָ���Լ�
	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList); 
	//��ʼ�����ɻ�����У���������ƿ���������ɻ������
	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);//bpָ��ǰ�����
		bp->b_addr = this->Buffer[i];//ָ��û�����ƿ�������Ļ��������׵�ַ
		/* ��ʼ��NODEV���� */
		bp->b_back = &(this->bFreeList);//bp->b_backָ�����ɻ�����ж���
		bp->b_forw = this->bFreeList.b_forw;//bp->b_forwָ�����ɻ�����ж��׵�b_forw
		this->bFreeList.b_forw->b_back = bp;//bFreeList��b_forw������b_backָ��ǰ�����
		this->bFreeList.b_forw = bp;//bFreeList��b_forwָ��ǰ�����
		/* ��ʼ�����ɶ��� */
		bp->b_flags = Buf::B_BUSY;//���õ�ǰ��������ʹ����
		Brelse(bp);//�ͷŻ�����ƿ�
	}
	return;
}

/* ����һ�黺�棬���ڶ�д�ַ���blkno��*/
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* headbp = &this->bFreeList;
	Buf *bp;
	/* �����ڸ��豸�����������Ƿ�����Ӧ�Ļ��� */
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_BUSY;
		return bp;
	}

	// /* ȡ���ɶ��е�һ�����п� */
	// bp = this->bFreeList.b_forw;
	// cout<<"&bp:"<<&bp<<endl;
	// if (bp->b_flags & Buf::B_DELWRI)
	// {
	// 	this->Bwrite(bp);
	// }
	// /* ע��: �����������������λ��ֻ����B_BUSY */
	// bp->b_flags = Buf::B_BUSY;

	// /* ��ԭ�豸�����г�� */
	// bp->b_back->b_forw = bp->b_forw;
	// bp->b_forw->b_back = bp->b_back;

	// // �������ɻ������
	// bp->b_back = this->bFreeList.b_back->b_forw;
	// this->bFreeList.b_back->b_forw = bp;
	// bp->b_forw = &this->bFreeList;
	// this->bFreeList.b_back = bp;

	// û�ҵ���ȥ��ͷ��
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_flags & Buf::B_BUSY)
			continue;
		bp->b_flags |= Buf::B_BUSY;
		break;
	}
	// cout<<"&bp:"<<&bp<<endl;
	if (bp->b_flags & Buf::B_DELWRI)
	{
		this->Bwrite(bp);
	}

	// ��д�أ����������־
	bp->b_flags = Buf::B_BUSY;

	bp->b_blkno = blkno;
	// cout<<"blkno:"<<blkno<<endl;
	return bp;
} 

/* �ͷŻ�����ƿ�buf */
void BufferManager::Brelse(Buf* bp)
{
	/* ע�����²�����û�����B_DELWRI��B_WRITE��B_READ��B_DONE��־
	 * B_DELWRI��ʾ��Ȼ���ÿ��ƿ��ͷŵ����ɶ������棬�����п��ܻ�û��Щ�������ϡ�
	 * B_DONE����ָ�û����������ȷ�ط�ӳ�˴洢�ڻ�Ӧ�洢�ڴ����ϵ���Ϣ
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	//ע������ȡ���˶����ɻ�����еĳ�ʼ�����ƶ�������ϵͳ��ֻ��һ������
	return;
}

/* ��һ�����̿顣blknoΪĿ����̿��߼���š� */
Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* �����豸�ţ��ַ�������뻺�� */
	bp = this->GetBlk(blkno);
	// cout<<"blkno:"<<blkno<<endl;
	/* ������豸�������ҵ����軺�棬��B_DONE�����ã��Ͳ������I/O���� */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* û���ҵ���Ӧ���棬����I/O������� */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;
	//ֱ�ӿ��������漴��
	memcpy(bp->b_addr, &p[BufferManager::BUFFER_SIZE * blkno], BufferManager::BUFFER_SIZE);
	// cout<<"data:"<<p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	// cout<<"&data:"<<&p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	return bp;
}

/* дһ�����̿� */
void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512�ֽ� */

	// if ((flags & Buf::B_ASYNC) == 0)
	// {
	// 	/* ͬ��д����Ҫ�ȴ�I/O�������� */
	// 	memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
	// 	this->Brelse(bp);
	// }
	// else if ((flags & Buf::B_DELWRI) == 0)
	// {
	// 	memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
	// 	this->Brelse(bp);
	// }
	memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
	this->Brelse(bp);
	// cout<<"bp->b_addr:"<<bp->b_addr<<endl;
	// cout<<"data:"<<this->p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	// cout<<"&data:"<<&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	return;
}

/* �ӳ�д���̿� */
void BufferManager::Bdwrite(Buf *bp)
{
	/* ����B_DONE������������ʹ�øô��̿����� */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

/* �첽д���̿� */
void BufferManager::Bawrite(Buf *bp)
{
	/* ���Ϊ�첽д */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

/* ��ջ��������� */
void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* ������������������ */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

/* ��devָ���豸�������ӳ�д�Ļ���ȫ����������� */
void BufferManager::Bflush()
{
	Buf* bp;
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* �ҳ����ɶ����������ӳ�д�Ŀ� */
		if ((bp->b_flags & Buf::B_DELWRI) )
		{
			/* ��ԭ�豸�����г�� */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			// �������ɻ������
			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;

			this->Bwrite(bp);
		}
	}
	return;
}

/* ���ָ���ַ����Ƿ����ڻ����� */
Buf* BufferManager::InCore( int blkno)
{
	Buf* bp;
	Buf*headbp = &this->bFreeList;

	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno == blkno )
			return bp;
	}
	return NULL;
}

/* ��ȡ���ɻ�����п��ƿ�Buf�������� */
Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

