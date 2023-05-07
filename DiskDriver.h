#ifndef DISKDRIVER_H
#define DISKDRIVER_H

//用于磁盘初始化
#include <iostream>
#include "Buf.h"
#include "FileSystem.h"
using namespace std;

class DiskDriver{
public:
    DiskDriver();
    ~DiskDriver();

    void InitialSuperBlock(SuperBlock &sb); //初始化SuperBlock
    void InitialDataBlock(char *data);      //初始化空闲盘块
    void InitialImg();     //初始化c.img
    void InitialSystem();  //初始化系统
    void Initial();        //初始化总函数

private:
    int fd;   //c.img的文件标识符
    char * addr = NULL; //mmap返回被映射区的指针地址
    int len;  //mmap映射长度
};

#endif