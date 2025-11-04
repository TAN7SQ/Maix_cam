# 环境搭建

- 在尝试过 vmware 虚拟机、ubuntu 物理机、wsl2 后，强烈推荐使用 wsl+vscode 作为开发方式，一方面是不需要配置这么多的环境（环境也更少 bug），另外一方面是 wsl 相比其他两个占用的内存更少，下载更加简单（仅需在 MicroSoft Store 下载 wsl2 和 ubuntu20.04 即可，20.04 是 maixcam 推荐的环境，其他的环境暂时没有试过），在 wsl 中翻墙也比在 vmware 和物理机中更加简单方便
  （这里仅介绍需要下载的基础工具和原理，并不介绍具体的安装下载方法，自行百度即可）

- 需要下载的软件：wsl、ubuntu20.04、vscode、filezilla

1. 在 MicroSoft Store 下载 wsl2 和 ubuntu20.04 后进行安装，能够在 powershell 中进入 ubuntu 即可
   - 注意 wsl 默认装在 c 盘，可以直接在设置中直接移动 wsl 到你需要的盘符，不推荐在 c 盘进行开发
2. 配置 samba 服务器，将 wsl 的根文件映射到 windows 的盘符（这里我映射到 z 盘，看个人喜好即可），使得我们可以在 windows 下直接看到 wsl 下的文件就像是在操作自己本地的盘符一样，由于是基于内部的网卡连接，所以极其稳定
3. 在 vscode 中下载 wsl 插件，即可实现在 vscode 启动 wsl，可直接在 vscode 的 terminal 中进行命令的输入，无需再麻烦的使用 ssh 登录 wsl（当然也是可以的）
4. 在 vscode 中登录 wsl，在第一次下载 maixcdk 的时候可能会遇到一些问题，如在使用 pip 的时候需要先创建虚拟环境，在虚拟环境中使用 pip(这是 ubuntu 20.04 的新特性，当然也可以直接使用 sudo 命令来强制下载，但为了不污染环境，尽量创建虚拟环境后再使用 python)，后续的所有操作都要在虚拟环境中进行
5. 在使用 wsl 的时候，由于其是 NAT 模式，可以直接共享主机的梯子，大多数情况下都可以直接下载（但速度较慢；，在使用虚拟机的时候需要配置梯子，如果不成功的话就直接手动下载对应的库（速度较快），下载完后不需要解压，只需要放到在 buildlog 中可以需要下载的位置即可，build 会自动解压，当全部需要的依赖库都下载完后，即可重新使用 build，这个过程一般需要重复几次，将所有库下载完即可。
6. 下载 filezilla，使用 fscp 登录到开发板（端口号 22），直接双击即可传输文件
7. 最后在 vscode 中使用 ssh 登录开发板（基于以太网连接会更加稳定快速，不过 ssh 也不占什么带宽）

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
```

4. 上传：

   1. 使用 filezilla 等基于 fscp 文件传输的工具，或使用 scp 命令进行传输
   2. 使用 ssh 登录 maixcam 后，

      - 首先关闭屏幕显示进程，使用`pa -a`命令查看所有的进程号，使用`kill <pid>`来删除进程（deamon），（一般 pid 是 296）
      - 然后进入 filezilla 传入的文件夹内，使用`chmod +x <execatue>`为编译好的程序增加可执行权限

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
