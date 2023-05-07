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

/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/
void BufferManager::Initialize(char *start)
{
	this->p = start;
	int i;
	Buf* bp; //当前缓存块
	// 初始时bFreeList的b_forw、b_back都指向自己
	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList); 
	//初始化自由缓存队列，将缓存控制块添加入自由缓存队列
	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);//bp指向当前缓存块
		bp->b_addr = this->Buffer[i];//指向该缓存控制块所管理的缓冲区的首地址
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);//bp->b_back指向自由缓存队列队首
		bp->b_forw = this->bFreeList.b_forw;//bp->b_forw指向自由缓存队列队首的b_forw
		this->bFreeList.b_forw->b_back = bp;//bFreeList的b_forw缓存块的b_back指向当前缓存块
		this->bFreeList.b_forw = bp;//bFreeList的b_forw指向当前缓存块
		/* 初始化自由队列 */
		bp->b_flags = Buf::B_BUSY;//设置当前缓存正在使用中
		Brelse(bp);//释放缓存控制块
	}
	return;
}

/* 申请一块缓存，用于读写字符块blkno。*/
Buf* BufferManager::GetBlk(int blkno)
{
	Buf* headbp = &this->bFreeList;
	Buf *bp;
	/* 首先在该设备队列中搜索是否有相应的缓存 */
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_BUSY;
		return bp;
	}

	// /* 取自由队列第一个空闲块 */
	// bp = this->bFreeList.b_forw;
	// cout<<"&bp:"<<&bp<<endl;
	// if (bp->b_flags & Buf::B_DELWRI)
	// {
	// 	this->Bwrite(bp);
	// }
	// /* 注意: 这里清除了所有其他位，只设了B_BUSY */
	// bp->b_flags = Buf::B_BUSY;

	// /* 从原设备队列中抽出 */
	// bp->b_back->b_forw = bp->b_forw;
	// bp->b_forw->b_back = bp->b_back;

	// // 加入自由缓存队列
	// bp->b_back = this->bFreeList.b_back->b_forw;
	// this->bFreeList.b_back->b_forw = bp;
	// bp->b_forw = &this->bFreeList;
	// this->bFreeList.b_back = bp;

	// 没找到就去队头找
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

	// 已写回，清空其他标志
	bp->b_flags = Buf::B_BUSY;

	bp->b_blkno = blkno;
	// cout<<"blkno:"<<blkno<<endl;
	return bp;
} 

/* 释放缓存控制块buf */
void BufferManager::Brelse(Buf* bp)
{
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	//注：这里取消了对自由缓存队列的初始化和移动操作，系统中只有一个队列
	return;
}

/* 读一个磁盘块。blkno为目标磁盘块逻辑块号。 */
Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	// cout<<"blkno:"<<blkno<<endl;
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;
	//直接拷贝至缓存即可
	memcpy(bp->b_addr, &p[BufferManager::BUFFER_SIZE * blkno], BufferManager::BUFFER_SIZE);
	// cout<<"data:"<<p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	// cout<<"&data:"<<&p[BufferManager::BUFFER_SIZE*bp->b_blkno]<<endl;
	return bp;
}

/* 写一个磁盘块 */
void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	// if ((flags & Buf::B_ASYNC) == 0)
	// {
	// 	/* 同步写，需要等待I/O操作结束 */
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

/* 延迟写磁盘块 */
void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

/* 异步写磁盘块 */
void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

/* 清空缓冲区内容 */
void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

/* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */
void BufferManager::Bflush()
{
	Buf* bp;
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if ((bp->b_flags & Buf::B_DELWRI) )
		{
			/* 从原设备队列中抽出 */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			// 加入自由缓存队列
			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;

			this->Bwrite(bp);
		}
	}
	return;
}

/* 检查指定字符块是否已在缓存中 */
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

/* 获取自由缓存队列控制块Buf对象引用 */
Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

