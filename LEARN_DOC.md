# 环境搭建

- 在尝试过 vmware 虚拟机、ubuntu 物理机、wsl2 后，强烈推荐使用 wsl+vscode 作为开发方式，一方面是不需要配置这么多的环境（环境也更少 bug），另外一方面是 wsl 相比其他两个占用的内存更少，下载更加简单（仅需在 MicroSoft Store 下载 wsl2 和 ubuntu20.04 即可，20.04 是 maixcam 推荐的环境，其他的环境暂时没有试过）
  （这里仅介绍需要下载的基础工具和原理，并不介绍具体的安装下载方法，自行百度即可）

- 需要下载的软件：wsl、ubuntu20.04、vscode、filezilla

  1. 在 MicroSoft Store 下载 wsl2 和 ubuntu20.04 后进行安装，能够在 powershell 中进入 ubuntu 即可

  - 注意 wsl 默认装在 c 盘，可以直接在设置中直接移动 wsl 到你需要的盘符，不推荐在 c 盘进行开发

  2. [samba 安装教程](https://wiki.lckfb.com/zh-hans/tspi-rk3566/project-case/fat-little-cell-phone/programming-environment-setup.html#samba%E6%90%AD%E5%BB%BA)，将 wsl 的根文件映射到 windows 的盘符（这里我映射到 z 盘，看个人喜好即可），使得我们可以在 windows 下直接看到 wsl 下的文件就像是在操作自己本地的盘符一样，由于是基于内部的网卡连接，所以极其稳定
  3. 在 vscode 中下载 wsl 插件，即可实现在 vscode 启动 wsl，可直接在 vscode 的 terminal 中进行命令的输入，无需再麻烦的使用 ssh 登录 wsl（当然也是可以的）
  4. 在 vscode 中登录 wsl，在第一次下载 maixcdk 的时候可能会遇到一些问题，如在使用 pip 的时候需要先创建虚拟环境，在虚拟环境中使用 pip(这是 ubuntu 20.04 的新特性，当然也可以直接使用 sudo 命令来强制下载，但为了不污染环境，尽量创建虚拟环境后再使用 python)，后续的所有操作都要在虚拟环境中进行
  5. 在使用 wsl 的时候，由于其是 NAT 模式，可以直接共享主机的梯子，大多数情况下都可以直接下载（但速度较慢，在使用虚拟机的时候需要配置梯子，如果不成功的话就直接手动下载对应的库（速度较快），下载完后不需要解压，只需要放到在 buildlog 中可以需要下载的位置即可，build 会自动解压，当全部需要的依赖库都下载完后，即可重新使用 build，这个过程一般需要重复几次，将所有库下载完即可，[纯命令行翻墙教程](https://github.com/Xizhe-Hao/Clash-for-RaspberryPi-4B?tab=readme-ov-file)。
  6. 下载 filezilla，使用 fscp 登录到开发板（端口号 22），直接右键即可传输文件
  7. 最后在 vscode/powershell 中使用 ssh 登录开发板（基于以太网连接会更加稳定快速，不过 ssh 也不占什么带宽），这些端口在 win 和在 wsl 都可以访问的

## 推荐在 vscode 下载的插件：

1. cmake、cmake tools
2. c/cpp、c/cpp externsion、python
3. wsl、remote ssh
4. prettier、markdown all in one、vscode icons

(只要配置好 cmake 和 cpp 文件，就能实现在 wsl 中代码跳转)

# 基础编译操作

1. 首先激活 python 环境以使用 maixcdk，激活方法：
   `source ./activate_python_venv.sh"`

2. 创建新任务：
   `maixcdk new`

3. 编译：
   编译好的程序一般放置在 dist 文件夹下，因为可能会有动态库的文件，在文件传输的时候需要将 dist 目录下的所有文件都传输到目标板子上

```bash
maixcdk distclean # 清除编译中间文件
maixcdk build     # 完全编译
maixcdk build2    # 这个不会检测增删的文件，只对文件内部代码的增删做编译

==================================
build time: 5.05s (2025-12-08 11:20:16)
platform  : maixcam
build type: Release(MinSizeRel)
toolchain : musl_t-head
==================================
有输出上述字样，才能算真正编译完成
```

4. 上传：

   1. 使用 filezilla 等基于 fscp 文件传输的工具，或使用 scp 命令进行传输
   2. 使用 ssh 登录 maixcam 后，

      - 首先关闭屏幕显示进程，使用`pa -a`命令查看所有的进程号，使用`kill <pid>`来删除进程（deamon），（一般 pid 是 296 和 354，对应 deamon 和 maix_vison_server，这两个都会占用 cpu 和 mem 资源）
      - 然后进入 filezilla 传入的文件夹内，使用`chmod +x <execatue>`为编译好的程序增加可执行权限
      - 如果想要开机自启动的话可以通过配置` vi /etc/rc.local`来实现（包括联网等操作）

   3. 使用`top -d 1`可以实现 1s 更新一次，查看当前系统的 CPU 和内存的占用率（top 会相对比 htop 占用更少的资源）

---

# 远程调试 STM32

## 前言

- 由于 Maixcam 固件本身没有加载 Rt-Linux 的补丁，且 sg2002 的 Rtos 核很多外设无法使用（目前官方只有 uart 和 i2c 的 demo ———— 25.11.20），且 sg2002 本身的外设就不多，除去 mipi-csi，sdio0，sdio1，eth，uart，emmc，实则剩下的可用引脚不多，所以想要全部使用 sg2002 来控制不仅难度大、sdk 不完善，而且本身可拓展性也不强，故舍弃掉
  - 综上所述，在现在这个时间点来看，往前看往后看，使用 stm32/esp32 + sg2002 无疑是一个更佳的选择（这里先不讨论 esp32 的情况）
- 显然，假如只使用 sg2002 的控制+视觉，那么可以很轻易的实现基于 ssh 的无线调试方式，想要使用 sg2002+stm32，很自然的需要解决烧录的问题，以下是几个解决方法

  1. 静态烧录口烧录 + sg2002 转发串口调试
  2. 板载装甲板无线烧录器（基于 esp32s3）
  3. link + sg2002 + openocd server

  - 方案一的坏处是无法远程烧录，必须使用烧录器连接烧录，且调试也不方便；方案二基于 esp32s3 其本身的 2.4Ghz 的频段调试，也很容易受到干扰，且不仅需要无线板载烧录器，其还需要搭配主从烧录器来使用，不够稳定；方案三的好处是直接使用 openocd 开启 gdb server ，在同一局域网的情况下就能实现烧录与调试（但目前似乎因为 ozone 使用的是拓展的 gdb，openocd 的 gdb 是标准的 gdb，不能兼容 ozone ，后续尝试编译更加完整的 openocd ），方案三不需要编写其他额外的代码，可显著避免其他的问题，只要电脑开热点或者电脑使用网线连接 sg2002、开启 openocd 即可，是最方便的方案。

- 为什么不使用 jlink remote server？因为 sg2002 的大核虽然是异构设计，可自行切换 riscV 架构或 arm 架构，但 maixcam 固件本身使用的是 riscV 架构，所以完全不能兼容。实测在 rk3506 上可以完美兼容 jlink remote server 和 openocd server

## 操作

- 在尝试了无数次自行构建源码后，我发现使用官方 buildroot-sdk 编译是最方便的，直接下载 LicheeRvNano

```shell
cd ./buildroot
make menuconfig
# 输入/查找openocd，在hardware什么的
# 选择jlink，stlink，daplink，实测三个都可以使用
make openocd -j$(nproc)
# 选择最终编译出来的openocd，以及相应的openocd scrips，最后使用scp把所有需要用到的文件传输到板子上即可，在板子上编写.cfg文件，使用 openocd -f xxx.cfg (-d)即可
openocd -f xxx.cfg
```

# QNA

## 1. 在使用屏幕的时候没有/dev/fb0

- 原因：
  这是可能因为在启动 linux 的时候忘记把文件的可执行权限打开，我们一般可以重新执行初始化程序，然后编写脚本，在每次启动 linux 的时候进行自动初始化
- 解决方法：
  1. 编辑系统自启脚本（通常是 /etc/rc.local，若没有则创建）：

```bash
vi /etc/rc.local
```

```sh
# 开机自动加载 soph_fb 驱动
if ! lsmod | grep -q "soph_fb"; then
   insmod /mnt/system/ko/soph_fb.ko
fi
# 开机自动创建 /dev/fb0 节点（若不存在）
if [ ! -c "/dev/fb0" ]; then
   mknod /dev/fb0 c 29 0
   chmod 666 /dev/fb0
fi
# （可选）若想开机自动运行程序，加这行（注意：程序会在后台运行，屏幕直接显示画面）
# /path/to/your/camera_display &
```

2. 给 rc.local 加执行权限（确保开机能运行）：

```bash
chmod +x /etc/rc.local
```

## 2. 在使用 maixcdk build 的时候无法下载对应的库

- 在使用虚拟机的时候需要配置梯子，如果不成功的话就直接手动下载对应的库，下载完后不需要解压，只需要放到在 buildlog 中可以需要下载的位置即可，build 会自动解压，当全部需要的依赖库都下载完后，即可重新使用 build，这个过程一般需要重复几次，将所有库下载完即可

## 3. 查看系统核心温度

在`sys/class/thermal/thermal_zone0 `目录下，有 temp 文件，直接`cat temp`即可查看当前温度

- 注意开启屏幕后会特别烫，注意散热

```bash
# 可从任意目录执行
temp=$(cat /sys/class/thermal/thermal_zone0/temp)
echo "Zone 0: $(echo "scale=1; $temp/1000" | bc)°C"
```

## 4. 使用 opencv

实测发现使用 opencv 帧率会大幅度下降，能使用 maix 的模块尽量不使用 opencv，maix 的函数经过硬件加速

## 5. 在 licheerv nano buildroot 上，使用 maixcdk

获取完整的 maixcam 的固件，比较与 licheervnano 的区别
移动

1. board
2. hostname.prefix
3. ver
4. uEnv.txt
5. resolv.conf

在内部的线程上，两者的主要区别是缺少了 `maixapp` 相关的进程

```bash
/maixapp/apps/launcher/launcher_daemon（PID 296     maixapp 的启动器守护进程，负责管理 maixvision_server 等核心应用的启动
/maixapp/apps/launcher/launcher daemon（PID 297） 启动器的辅助进程，保障 maixapp 相关服务稳定运行
/usr/bin/maixvision_server（PID 354） maixvision 识别设备的核心服务，负责对外提供 AI 视觉能力和设备识别接口

/usr/bin上的差异
app_store_cli：应用商店命令行工具
lt9611：可能是 LT9611 芯片相关工具
maix-resize：Maix 平台图像缩放工具
maixvision_server：Maix 视觉服务程序
cvimodel_tool：CVIModel 模型操作工具
sync_time：时间同步工具
time_sync_daemon：时间同步守护进程
uptime_us：微秒级系统运行时间查询工具
usb_util：USB 设备操作工具
wifi_util：Wi-Fi 相关工具
hexdump：十六进制 dump 工具（第一份虽有但第二份重复列出，实际为新增有效条目）
pyrcc5：PyQt 资源文件编译工具（第一份有，第二份重复列出，补充完整）
zstdgrep：ZSTD 压缩文件搜索工具
zstdless：ZSTD 压缩文件查看工具

/usr/lib上的差异
libmaixcam_lib.so：Maix 平台摄像头相关核心库
libmaixcam_lib.so.1.23.0：Maix 摄像头库的具体版本文件
libcnpy.so：CNPY 库（用于 NumPy 数组与 C++ 数据交互）
libcvikernel.so、libcvimath.so、libcviruntime.so：CVIModel 相关运行时、内核及数学计算库（3 个关联库，配套提供 CV 模型运行支持）
```

如果想要正常启动，还需要运行以下的线程
/maixapp/apps/launcher/launcher_daemon（Maix 启动器守护进程）
/maixapp/apps/launcher/launcher daemon（Maix 启动器主进程）
/usr/bin/maixvision_server（Maix 视觉服务，核心视觉处理进程，启动该线程后可以正常连接 maixvision，但还不能运行程序）
time_sync_daemon（时间同步守护进程，配套 Maix 功能运行）

还需要把 python 脚本搞上去

在文件构成上，maixcam 固件在 usr/bin，/usr/lib，/bin 目录下有所有的静态库文件，包括但不限于 yolo\lvgl\nnmodel

| 由于过于复杂，笔者最终没能移植完所有的动态库（确实太多了，估计后期得大文件夹的移动会比较方便，可能后期会尝试自己移植，自己构建这个 buildroot 系统

## 6. 有时候无法使用 ssh 登录

具体的显示如下

```powershell
PS C:\Users\xxx> ssh root@10.218.35.1
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!
Someone could be eavesdropping on you right now (man-in-the-middle attack)!
It is also possible that a host key has just been changed.
The fingerprint for the ED25519 key sent by the remote host is
SHA256:xxx
Please contact your system administrator.
Add correct host key in C:\\Users\\xxx/.ssh/known_hosts to get rid of this message.
Offending ED25519 key in C:\\Users\\xxx/.ssh/known_hosts:25
Host key for 10.218.35.1 has changed and you have requested strict checking.
Host key verification failed.

```

核心原因：连接的这个板子（10.218.35.1）可能重新安装了操作系统，或者重新配置了 SSH 服务。这导致它的主机密钥发生了变化。

执行`ssh-keygen -R "ssh的ip地址"`重置主机密钥，再次连接即可

## 7. 与 milkv duo 的差异

milk-v duo 与 licheerv nano 在硬件上具有些许差异，导致其固件并不通用，但可以参考其 freertos 的使用经验，以及如何编译内核，不过对于小核的外设，两者的兼容性都不太好，很多外设都缺失，但如果使用 arduino 的话，其启用的是小小核，希望后期官方能够多开发一下实时核小核吧 ————25.11.20

## 8. 使用 nn 与 vision 模块

在 cmakelist 中，nn 模块必须在 vision 模块之前，否则会编译不过
