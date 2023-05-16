二级文件系统

操作系统：Ubuntu 64

运行方法：进入项目文件夹，make，生成可执行文件filesystem，运行filesystem即可使用。

使用方法：

运行filesystem后，会显示菜单，可以选择需要运行的指令。     

具体菜单如下所示（可以输入字母 a - l，q 来执行对应的指令）：      

```
===============================================================
请输入需要执行的API的对应编号:                  
a   ls()                                                  
b   fopen(char *name, int mode)                        
c   fclose(int fd)                                   
d   fread(int fd, int length)                         
e   fwrite(int fd, char *buffer, int length)          
f   flseek(int fd, int position, int ptrname)   
g   fcreat(char *name, int mode)                    
h   fdelete(char *name)                                 
i   mkdir(char* dirname)                             
j   cd(char* dirname)                                   
k   fin(char *fileout, char *filein)               
l   fout(char *filein, char *fileout)                
q   quit()                                            
===============================================================
```

下面具体展示每条命令的使用方法，有对应的输入提示：

a、ls();  显示当前目录

b、fopen(char *name, int mode); 打开文件

输入提示：

请输入第一个参数(文件名):

请输入第二个参数(打开方式)(r/w/rw):选择r/w/rw的一种

c、fclose(int fd); 关闭已打开的文件

输入提示：

请输入第一个参数(文件句柄):即open或create文件后返回的fd

d、fread(int fd, int len); 从文件中读取数据

输入提示：

请输入第一个参数(文件句柄):即open或create文件后返回的fd

请输入第二个参数(读出的数据长度):

e、fwrite(int fd, char *src, int len); 向文件中写入数据

输入提示：

请输入第一个参数(文件句柄):即open或create文件后返回的fd

请输入第二个参数(写入数据):

请输入第三个参数(写入数据的长度):

f、flseek(int fd, int position, int ptrname); 重定位文件当前读写指针

输入提示：

请输入第一个参数(文件句柄):即open或create文件后返回的fd

请输入第二个参数(移动位置):即offset

请输入第三个参数(移动方式):可输入0，1，2（0表示读写位置设置为offset，1表示读写位置加offset(可正可负)，2表示读写位置调整为文件长度加offset）

g、fcreat(char *filename, int mode);  创建文件

输入提示：

请输入第一个参数(文件名):

请输入第二个参数(创建模式)(r/w/rw):选择r/w/rw的一种

h、fdelete(char *name);  删除文件

输入提示：

请输入第一个参数(文件名):

i、mkdir(char *dirname);  创建目录文件

输入提示：

请输入第一个参数(目录名):

j、cd(char *dirname);  改变工作目录

输入提示：

请输入第一个参数(目录名):

k、fin(char *fileout, char *filein);  将外部文件传入内部文件（附带创建）

输入提示：

请输入第一个参数(外部文件路径):可以为绝对路径，也可以为相对路径（相对于TestAPI.cpp，如果放在SecondFileSystem目录下，即为./test.txt，test.txt为文件名）

请输入第二个参数(内部文件名，基于当前目录):

l、fout(char * filein, char * fileout);  将内部文件传到外部文件

输入提示：

请输入第一个参数(内部文件名，基于当前目录):

请输入第二个参数(外部文件路径):可以为绝对路径，也可以为相对路径（相对于TestAPI.cpp，如果放在SecondFileSystem目录下，即为./test.txt，test.txt为文件名）

q、quit(); 退出二级文件系统并保存



具体测试过程可以查看报告中的测试部分。