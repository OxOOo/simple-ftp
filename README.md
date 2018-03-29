# simple-ftp
simple ftp

### 架构

Master通过fork创建子进程.

### 命令

每行描述一个命令

* `ls` : 列出当前目录下的信息

* `cd` : 切换目录

* `mkdir` : 创建目录

* `cat` : 输出文件内容

* `recv` : 下载问家内容,发送完毕之后关闭端口

* `up` : 上传文件