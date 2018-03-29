# simple-ftp
simple ftp

### 编译&运行

```sh
> mkdir build
> cd build
> cmake ..
> make
```

```sh
> ./sftp
```

### 架构

Master进程在`0.0.0.0:4000`和`0.0.0.0:4001`地址监听TCP链接,然后fork创建一共8个子进程.

`0.0.0.0:4000`和`0.0.0.0:4001`的功能完全一样,区别只在于`4000`端口会输出当前命令行信息,`4001`端口不会.

Master进程监听子进程的推出情况,在必要时重启子进程.

每个子进程使用`poll`检测事件,可以同时处理`1024-2`个链接. 不会在想一个链接发送或接收大数据的时候卡死其他链接.

### 命令

每行描述一个命令

* `ls` : 列出当前目录下的信息

* `cd` : 切换目录

* `mkdir` : 创建目录

* `cat` : 输出文件内容

* `recv` : 下载问家内容,发送完毕之后关闭端口

* `up` : 上传文件

### 文件相关

1. 下载服务器上的`file.txt`到本地`download.txt`

```sh
> echo "recv file.txt" | nc localhost 4001 > download.txt
```

2. 上传本地的`file.txt`到服务器`upload.txt`

```sh
> echo "up upload.txt" | cat - file.txt | nc localhost 4000(1)
```
