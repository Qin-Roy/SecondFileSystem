#include <iostream>
#include <limits>
#include <string.h>
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/mman.h>  
#include <sys/stat.h> 

#include "TestAPI.h"
#include "Buf.h"
#include "BufferManager.h"
#include "File.h"
#include "FileSystem.h"
#include "INode.h"
#include "Kernel.h"
#include "OpenFileManager.h"
#include "User.h"

using namespace std;

TestAPI::TestAPI(){
	strcpy(this->curdir,"/");
}

TestAPI::~TestAPI(){
	//nothing to do here
}

//显示当前目录列表
void TestAPI::ls()
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	int fd = fopen(u.u_curdir, (File::FREAD) );
	char tempFilename[32] = { 0 };
	for (;;)
	{
		if (fread(fd, tempFilename, 32) == 0) {
			return;
		}
		else
		{
			DirectoryEntry *mm = (DirectoryEntry*)tempFilename;
			if (mm->m_ino == 0)
				continue;
			cout << mm->m_name << endl;
			memset(tempFilename, 0, 32);
		}
	}
	fclose(fd);
}

//创建文件
int TestAPI::fcreate(char *filename,int mode)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = filename;
	u.u_arg[1] = Inode::IRWXU;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Creat();
	return u.u_ar0;
}

//打开文件
int TestAPI::fopen(char *pathname,int mode)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = pathname;
	u.u_arg[1] = mode;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Open();
	return u.u_ar0;
}

//写文件
int TestAPI::fwrite(int fd,char *src,int len)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = (long long)(src);
	u.u_arg[2] = len;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Write();
	return u.u_ar0;
}

//读文件
int TestAPI::fread(int fd,char *des,int len)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = (long long)(des);
	u.u_arg[2] = len;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Read();
	// cout<<"u.u_ar0:"<<u.u_ar0<<endl;
	return u.u_ar0;
}

//删除文件
void TestAPI::fdelete(char* name)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = name;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.UnLink();
}

//调整读写指针位置
int TestAPI::flseek(int fd,int position,int ptrname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = position;
	u.u_arg[2] = ptrname;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.Seek();
	return u.u_ar0;
}

//关闭文件
void TestAPI::fclose(int fd)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	FileManager &fm = Kernel::Instance().GetFileManager();
	u.u_arg[0] = fd;
	fm.Close();
	cout<<"文件关闭成功！"<<endl;
}

//创建文件夹
void TestAPI::mkdir(char *dirname)
{
	int defaultmode = 040755;
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_dirp = dirname;
	u.u_arg[1] = defaultmode;
	u.u_arg[2] = 0;
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.MkNod();

}

//改变目录
void TestAPI::cd(char *dirname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	char ppp[10] = { 0 };
	strcpy(ppp, dirname);
	u.u_dirp = ppp;
	u.u_arg[0] = (long long)(ppp);
	FileManager &fm = Kernel::Instance().GetFileManager();
	fm.ChDir();
	strcpy(this->curdir,u.u_curdir);
}

//将外部文件传入内部文件
void TestAPI::fin(char *fileout, char *filein){
	// 打开外部文件
    int ofd = open(fileout, O_RDONLY); //只读方式打开外部文件
    if(ofd < 0){
        cout << "Fail to open file(out)" << endl;
        return;
    }
	// 创建内部文件
	fcreate(filein,511);
	int ifd = fopen(filein,511);
	if (ifd < 0){
		cout << "Fail to open file(in)" << endl;
		return;
	}
	// 开始拷贝，一次 256 字节
    char buf[256];
    int all_read_num = 0;
    int all_write_num = 0;
    while(true){
        memset(buf, 0, sizeof(buf));
        int read_num = read(ofd, buf, 256);
		// for(int i=0;i<sizeof(buf);i++){
		// 	cout<<buf[i]<<" ";
		// }
        if(read_num <= 0){
            break;
        }
        all_read_num += read_num;
        int write_num = fwrite(ifd, buf, read_num);
        if(write_num <= 0){
            cout << "Fail to write in" << endl;
            break;
        }
        all_write_num += write_num;
    }
	cout << "read bytes：" << all_read_num << " write bytes：" << all_write_num << endl;
    close(ofd);
	fclose(ifd);
}

//将内部文件传入外部文件
void TestAPI::fout(char *filein, char *fileout){
	// 创建外部文件
    int ofd = open(fileout, O_RDWR | O_TRUNC | O_CREAT); //截断写入方式打开外部文件
    if(ofd < 0){
        cout << "Fail to create file(out)" << endl;
        return;
    }
	// 打开内部文件
	int ifd = fopen(filein,511);
	if (ifd < 0){
		cout << "Fail to open file(in)" << endl;
		return;
	}
	// 开始拷贝，一次 256 字节
    char buf[256];
    int all_read_num = 0;
    int all_write_num = 0;
	flseek(ifd, 0, 0);
    while(true){
        memset(buf, 0, sizeof(buf));
        int read_num = fread(ifd, buf, 256);
		// for(int i=0;i<sizeof(buf);i++){
		// 	cout<<buf[i]<<" ";
		// }
        if(read_num <= 0){
            break;
        }
        all_read_num += read_num;
        int write_num = write(ofd, buf, read_num);
        if(write_num <= 0){
            cout << "Fail to write in" << endl;
            break;
        }
        all_write_num += write_num;
    }
	cout << "read bytes：" << all_read_num << " write bytes：" << all_write_num << endl;
    close(ofd);
	fclose(ifd);
}

//退出系统
void TestAPI::quit()
{
	BufferManager &bm = Kernel::Instance().GetBufferManager();
	bm.Bflush();
	InodeTable *mit = Kernel::Instance().GetFileManager().m_InodeTable;
	mit->UpdateInodeTable();
}

//菜单
void TestAPI::Menu(){
	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
    cout << "                   Secondary File System in Ubuntu                  " << endl;
	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
	while (true)
	{
		cout << endl;
		cout << "===============================================================" << endl;
		cout << "请输入需要执行的API的对应编号:                               " << endl;
		cout << "a   ls()                                                   " << endl;
		cout << "b   fopen(char *name, int mode)                            " << endl;
		cout << "c   fclose(int fd)                                         " << endl;
		cout << "d   fread(int fd, int length)                              " << endl;
		cout << "e   fwrite(int fd, char *buffer, int length)               " << endl;
		cout << "f   flseek(int fd, int position, int ptrname)              " << endl;
		cout << "g   fcreat(char *name, int mode)                           " << endl;
		cout << "h   fdelete(char *name)                                    " << endl;
		cout << "i   mkdir(char* dirname)                                   " << endl;
		cout << "j   cd(char* dirname)                                      " << endl;
		cout << "k   fin(char *fileout, char *filein)                       " << endl;
		cout << "l   fout(char *filein, char *fileout)                       " << endl;
		cout << "q   quit()                                                 " << endl;
		cout << "===============================================================" << endl;
		cout << endl;
		cout << "[root@SecondaryFileSystem "<< this->curdir << "]# ";

		char choice = 0;
		string filename;
		string filename2;
		string inBuf;
		string dirname;
		int tempfd; 
		int outSeek;
		int inLen;
		int mode;
		int openfd;
		int tempPtrname;
		int outLen;
		int tempPosition;
		int readNum;
		int creatfd;
		int writeNum = 0;
		char c;
		char *tempInBuf, *tempDes, *tempFilename,*tempFilename2,*tempDirname;
		cin >> choice;
		switch (choice)
		{
		case 'a'://ls
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 当前目录列表如下:" << endl;
			ls();
			break;
		case 'b'://fopen
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件名):";
			cin >> filename;
			tempFilename = new char[filename.length()+1];
			strcpy(tempFilename, filename.c_str());
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(打开方式):";
			cin >> mode;
			openfd = fopen(tempFilename, mode);
			if (openfd < 0)
				cout << "[root@SecondaryFileSystem "<< this->curdir << "]# open失败" << endl;
			else
				cout << "[root@SecondaryFileSystem "<< this->curdir << "]# open 返回fd=" << openfd << endl;
			delete tempFilename;
			break;
		case 'c'://fclose
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件句柄):";
			cin >> tempfd;
			fclose(tempfd);
			break;
		case 'd'://fread
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件句柄):";
			cin >> tempfd;
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(读出的数据长度):";
			cin >>outLen;
			tempDes = new char[1+outLen];
			memset(tempDes, 0, outLen + 1);
			readNum = fread(tempfd, tempDes, outLen);
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# read返回:" << readNum << endl;
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 读出数据为:" << endl;
			cout << tempDes << endl;
			break;
		case 'e'://fwrite
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件句柄):";
			cin >> tempfd;
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(写入数据):";
			cin >> inBuf;
			tempInBuf = new char[inBuf.length() + 1];
			strcpy(tempInBuf, inBuf.c_str());
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第三个参数(写入数据的长度):";
			cin >>inLen;
			writeNum = fwrite(tempfd, tempInBuf, inLen);
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# write返回:" << writeNum << endl;
			break;
		case 'f'://flseek
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件句柄):";
			cin >> tempfd;
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(移动位置):";
			cin >> tempPosition;
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第三个参数(移动方式):";
			cin >> tempPtrname;
			outSeek = flseek(tempfd, tempPosition, tempPtrname);
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# fseek返回:" << outSeek << endl;
			break;
		case 'g'://fcreat
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件名):";
			cin >> filename;
			tempFilename = new char[filename.length() + 1];
			strcpy(tempFilename, filename.c_str());
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(创建模式):";
			cin >> mode;
			creatfd = fcreate(tempFilename, mode);
			if (creatfd < 0)
				cout << "[root@SecondaryFileSystem "<< this->curdir << "]# create失败" << endl;
			else
				cout << "[root@SecondaryFileSystem "<< this->curdir << "]# create成功 返回fd=" << creatfd << endl;
			delete tempFilename;
			break;
		case 'h'://fdelete
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(文件名):";
			cin >> filename;
			tempFilename = new char[filename.length() + 1];
			strcpy(tempFilename, filename.c_str());
			fdelete(tempFilename);
			delete tempFilename;
			break;
		case 'i'://mkdir
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(目录名):";
			cin >> dirname;
			tempDirname = new char[dirname.length() + 1];
			strcpy(tempDirname, dirname.c_str());
			mkdir(tempDirname);
			delete tempDirname;
			break;
		case 'j'://cd
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(目录名):";
			cin >> dirname;
			tempDirname = new char[dirname.length() + 1];
			strcpy(tempDirname, dirname.c_str());
			cd(tempDirname);
			delete tempDirname;
			break;
		case 'k'://fin
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(外部文件路径):";
			cin >> filename;
			tempFilename = new char[filename.length() + 1];
			strcpy(tempFilename, filename.c_str());
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(内部文件名，基于当前目录):";
			cin >> filename2;
			tempFilename2 = new char[filename2.length() + 1];
			strcpy(tempFilename2, filename2.c_str());
			fin(tempFilename,tempFilename2);
			delete tempFilename;
			delete tempFilename2;
			break;
		case 'l'://fout
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第一个参数(内部文件名，基于当前目录):";
			cin >> filename;
			tempFilename = new char[filename.length() + 1];
			strcpy(tempFilename, filename.c_str());
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 请输入第二个参数(外部文件路径):";
			cin >> filename2;
			tempFilename2 = new char[filename2.length() + 1];
			strcpy(tempFilename2, filename2.c_str());
			fout(tempFilename,tempFilename2);
			delete tempFilename;
			delete tempFilename2;
			break;
		case 'q':
			quit();
			return;
			break;
		default:
			cout << "[root@SecondaryFileSystem "<< this->curdir << "]# 输入不合法，请重新输入" << endl;
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			break;
		}
	}

}