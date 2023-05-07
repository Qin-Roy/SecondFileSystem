#ifndef DISKDRIVER_H
#define DISKDRIVER_H

//���ڴ��̳�ʼ��
#include <iostream>
#include "Buf.h"
#include "FileSystem.h"
using namespace std;

class DiskDriver{
public:
    DiskDriver();
    ~DiskDriver();

    void InitialSuperBlock(SuperBlock &sb); //��ʼ��SuperBlock
    void InitialDataBlock(char *data);      //��ʼ�������̿�
    void InitialImg();     //��ʼ��c.img
    void InitialSystem();  //��ʼ��ϵͳ
    void Initial();        //��ʼ���ܺ���

private:
    int fd;   //c.img���ļ���ʶ��
    char * addr = NULL; //mmap���ر�ӳ������ָ���ַ
    int len;  //mmapӳ�䳤��
};

#endif