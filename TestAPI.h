#ifndef TESTAPI_H
#define TESTAPI_H

class TestAPI{
public:
    TestAPI();

    ~TestAPI();

    //显示当前目录列表
    void ls();

    //创建文件
    int fcreate(char *filename,int mode);

    //打开文件
    int fopen(char *pathname,int mode);

    //写文件
    int fwrite(int fd,char *src,int len);

    //读文件
    int fread(int fd,char *des,int len);

    //删除文件
    void fdelete(char* name);

    //调整读写指针位置
    int flseek(int fd,int position,int ptrname);

    //关闭文件
    void fclose(int fd);

    //创建文件夹
    void mkdir(char *dirname);

    //改变目录
    void cd(char *dirname);

    //将外部文件传入内部文件
    void fin(char *fileout, char *filein);

    //将内部文件传入外部文件
    void fout(char *filein, char *fileout);
    
    //退出系统
    void quit();

    //菜单
    void Menu();

private:
    //当前目录
    char curdir[28];

};

#endif
