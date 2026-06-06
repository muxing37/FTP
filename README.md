# Linux FTP Server

## 环境配置与编译说明

### 编译器

* GCC/G++ 7.0 及以上版本
* 支持 C++17 标准

查看版本：

```bash
g++ --version
```

### CMake

本项目使用 CMake 构建，需要安装：

* CMake 3.10 及以上版本

查看版本：

```bash
cmake --version
```

### GNU Readline

本项目使用 GNU Readline 提供命令行编辑、历史记录等功能，需要安装 Readline 开发库。

---

## Ubuntu / Debian 环境配置

安装编译工具：

```bash
sudo apt update

sudo apt install build-essential
```

安装 CMake：

```bash
sudo apt install cmake
```

安装 Readline：

```bash
sudo apt install libreadline-dev
```

验证 Readline 是否安装成功：

```bash
ls /usr/include/readline
```

正常情况下应能看到：

```text
history.h
readline.h
...
```

---

## Arch Linux

安装所需依赖：

```bash
sudo pacman -S gcc cmake readline
```

---

## Fedora

安装所需依赖：

```bash
sudo dnf install gcc gcc-c++ cmake readline-devel
```

---

# 项目编译

进入项目根目录：

```bash
cd FTP
```

创建构建目录：

```bash
mkdir build
cd build
```

生成 Makefile：

```bash
cmake ..
```

编译：

```bash
make
```

编译完成后会生成可执行文件：

```text
ftp
```

运行：

```bash
./ftp server 
```
或
```bash
./ftp client
```

---


## 使用说明

### 启动服务器

```bash
./ftp server
```

---

### 启动客户端

```bash
./ftp client
```

---

### 支持命令

| 命令         | 说明     |
| ---------- | ------ |
| PASV       | 请求被动模式 |
| LIST        | 查看当前目录 |
| CWD <dir>   | 切换服务器目录   |
| RETR <file> | 下载文件(支持绝对路径和相对路径)  |
| STOR <file> | 上传文件(支持绝对路径和相对路径)  |
| exit       | 退出连接   |

其他说明：
客户端下载目录为
```text
~/Download
```

---

## 项目结构

```text
FTPProject/
├── include/
│   ├── socket.h
│   ├── ftpserver.h
│   ├── ftpclient.h
│   └── ftpctrl.h
│
├── src/
│   ├── main.cpp
│   ├── socket.cpp
│   ├── ftpserver.cpp
│   ├── ftpclient.cpp
│   ├── ftpctrl.cpp
│   └── ftpls.cpp
│
├── CMakeLists.txt
└── README.md
```
