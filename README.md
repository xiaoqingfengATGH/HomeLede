# HomeLede （Kernel 6.x) 版本说明
[1]: https://img.shields.io/badge/license-GPLV2-brightgreen.svg
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg
[4]: https://github.com/xiaoqingfengATGH/HomeLede/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg
[6]: https://github.com/xiaoqingfengATGH/HomeLede/issues/new
[7]: https://img.shields.io/badge/release-v2026.11.14-gold.svg?
[8]: https://github.com/xiaoqingfengATGH/HomeLede/releases
[10]: https://img.shields.io/badge/Contact-telegram-blue
[11]: https://t.me/t_homelede
[![license][1]][2]
[![PRs Welcome][3]][4]
[![Issue Welcome][5]][6]
[![Release Version][7]][8]
[![Contact Me][10]][11]
![visits since 2024.07.23](https://views.whatilearened.today/views/github/xiaoqingfengATGH/deplives.svg)

[固件使用说明](https://github.com/xiaoqingfengATGH/HomeLede/wiki) [版本下载](https://github.com/xiaoqingfengATGH/HomeLede/wiki/HomeLede%E7%89%88%E6%9C%AC%E5%8F%91%E5%B8%83)

+ 基于 LEDE / OpenWrt，集成 HomeLede 原创软件、深度定制的软件包及第三方 Feed
+ 面向家庭 x86_64 软路由定制，兼顾物理机与虚拟机部署，以及家庭存储、远程接入和容器应用场景
+ 按照家庭应用场景对固件及关键软件进行 x86_64 测试，通过验证后发布

重点验证家庭路由的高频功能，包括网络接入、文件共享、远程访问、VPN、端口转发及 Docker 等。多拨效果、远程接入能力和硬件兼容性仍取决于运营商、网络环境及设备条件。

## 固件内置功能

以下按当前 x86_64 默认选包介绍；自行编译时可调整软件组合，具体发布版本以固件内的软件及发布说明为准。内置软件不代表相关服务默认全部开启，使用前请按需配置。

### HomeLede 原创与定制功能

+ **HomeStatus 驾驶舱（原创）**：展示磁盘、分区和挂载点容量，监视关键应用运行状态，支持单项服务重启及网络唤醒，并可自定义监视项目
+ **HomeRedirect 2.0（原创）**：基于 socat 的 TCP / UDP 端口转发，支持 IPv4、IPv6 及 IPv6 入口转发至 IPv4 内网服务，适用于只有公网 IPv6 的家庭宽带；提供图形化配置及配套防火墙规则管理
+ **HomeTunnel（HomeLede 定制）**：基于 Cloudflare Tunnel 的内网穿透，无需公网 IP 即可发布家庭 Web 服务；支持常驻模式与 Worker 控制的按需模式、访问期间自动续期及空闲到期关闭，需要 Cloudflare 账号和托管于 Cloudflare 的域名
+ **HomeVPN（基于第三方项目扩展）**：基于 strongSwan 的 IKEv2 + EAP-MSCHAPv2 家庭 VPN 服务，提供用户管理、每用户固定 IP、客户端配置导出及证书管理；支持自签名、导入证书和 ACME 证书联动，VPN 客户端可通过 DHCP 获取家庭局域网地址
+ **HomeACME（基于 LuCI ACME 定制）**：提供证书申请、自动续期及 DNS API 验证配置，增加证书清单，便于查看同一域名的 RSA / ECC 等不同证书实例，并可供 HomeVPN 使用
+ **infinityfreedom-ng 原创主题**：默认集成适配现代 LuCI / ucode 的主题，配合 HomeStatus 提供家庭软路由驾驶舱

### 网络接入与访问管理

+ 支持 IPv4 / IPv6、Firewall4 / nftables 防火墙及 UPnP / NAT-PMP，为下载工具、游戏主机及需要自动端口映射的应用提供支持
+ 支持 syncdial 单线 / 多线多拨及 mwan3 多 WAN 负载均衡；能否增加带宽取决于运营商的拨号与带宽策略
+ 内置 PassWall2，以及 Xray、Sing-Box 等代理核心和分流配套工具，支持按设备、域名等规则管理代理与直连
+ 提供 dnsmasq、ChinaDNS-NG、mosdns 等 DNS 组件，可按需配置分流解析、抗污染与解析优化；广告过滤需要另行配置规则或安装相应服务
+ 内置 DDNS-GO 及图形化管理界面，支持动态更新域名的 IPv4 / IPv6 地址
+ 支持基于 MAC、网址及时间段的网络访问控制
+ 支持 WOL 远程唤醒及 Time WOL 定时唤醒，可配合终端自动关机实现家庭设备定时运行
+ 内置 Watchcat 网络连通性监测及 PushBot 消息推送工具，按需配置监测与通知

### 家庭存储与下载

+ 支持 CIFS / SMB 网络共享挂载，提供图形化工具，可将 NAS、Samba 或 Windows 共享目录挂载到路由器
+ 内置自动挂载组件与 Samba4 文件共享，支持磁盘、U 盘挂载及向局域网共享，并提供 Windows 网络发现相关组件
+ 提供 DiskMan 磁盘管理、Partexp 分区扩容及 hd-idle 硬盘休眠工具，便于管理软路由的存储空间
+ 内置 Aria2、AriaNg 及 LuCI 管理界面，支持 HTTP / FTP、磁力链接和 BitTorrent 下载，可将已挂载的 NAS 目录配置为下载目录
+ 提供 FileBrowser 文件管理、FTP 服务及 SFTP 文件传输，方便通过浏览器或常见客户端管理文件

### 系统管理与扩展

+ 内置 Docker 引擎及 Dockerman 图形化管理界面，可通过容器扩展家庭应用
+ 内置 Nginx HTTPS 前端及 PHP8 / FastCGI 组件，为 Web 管理与应用扩展提供基础环境
+ 支持 SSH 远程管理、端口转发及 ed25519 密钥，提供 ttyd Web 终端；从外网访问需配置 VPN、隧道或必要的防火墙规则
+ 预置 Open VM Tools，配套常见物理网卡与虚拟网卡驱动，以及 Intel / AMD CPU 微码，兼顾物理机和虚拟化部署
+ 保留 SoftEther VPN 组件，可按需配置 SSL-VPN / L2TP 等接入方式；家庭 VPN 的主要图形化方案为 HomeVPN IKEv2
+ 提供软件源与 Feed 扩展机制，包管理器可自动处理已配置软件源中的依赖；OpenClash、OpenVPN、WireGuard、ZeroTier 等在当前编译配置中作为可选软件包构建，不属于默认内置应用

------

## 编译说明

注意：
1. **不**要用 **root** 用户编译！！！
2. 国内用户编译前最好准备好梯子
3. 默认登陆IP 192.168.1.1, 密码 homelede（v2026.10.01 版本之前为 password）

## 编译命令

编译前：
1. 首先装好 Ubuntu 64bit，推荐  Ubuntu 24 LTS x64
2. 至少30G空闲硬盘空间
3. 16以上内存，建议24G

编译时:
1. 更新apt-get包信息，命令行输入
`sudo apt-get update`

2. 安装编译依赖包，命令行输入
   ```bash
   sudo apt update -y
   sudo apt full-upgrade -y
   sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
   bzip2 ccache clang cmake cpio curl device-tree-compiler flex gawk gcc-multilib g++-multilib gettext \
   genisoimage git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libfuse-dev libglib2.0-dev \
   libgmp3-dev libltdl-dev libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libpython3-dev \
   libreadline-dev libssl-dev libtool llvm lrzsz libnsl-dev ninja-build p7zip p7zip-full patch pkgconf \
   python3 python3-pyelftools python3-setuptools qemu-utils rsync scons squashfs-tools subversion \
   swig texinfo uglifyjs upx-ucl unzip vim wget xmlto xxd zlib1g-dev
   ```

3. `git clone https://github.com/xiaoqingfengATGH/HomeLede.git HomeLede`命令下载好源代码，然后 `cd HomeLede` 进入目录

4. `git checkout -b k5 origin/k5`

5.  `./prepareCompile.sh`

6. `make download V=s` 下载dl库（国内请尽量全局科学上网,如果程序下载失败，也可以提取网址自行下载后放入dl文件夹，此文件夹通常不需要删除）

7. `make menuconfig`  配置软件包

8. `make -j1 V=s` （-j1 后面是线程数。第一次编译推荐用单线程，国内请尽量全局科学上网）即可开始编译你要的固件了。

编译成功后，再次编译可以启动多线程编译。如4核心8线程i7上开启16线程使用`make -j16 V=sc`

------
使用 WSL/WSL2 进行编译
------
由于 WSL 的 PATH 中包含带有空格的 Windows 路径，有可能会导致编译失败，请在 `make` 前面加上：

```bash
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
```

由于默认情况下，装载到 WSL 发行版的 NTFS 格式的驱动器将不区分大小写，因此大概率在 WSL/WSL2 的编译检查中会返回以下错误：

```txt
Build dependency: OpenWrt can only be built on a case-sensitive filesystem
```

一个比较简洁的解决方法是，在 `git clone` 前先创建 Repository 目录，并为其启用大小写敏感：

```powershell
# 以管理员身份打开终端
PS > fsutil.exe file setCaseSensitiveInfo <your_local_lede_path> enable
# 将本项目 git clone 到开启了大小写敏感的目录 <your_local_lede_path> 中
PS > git clone https://github.com/coolsnowwolf/lede <your_local_lede_path>
```

> 对已经 `git clone` 完成的项目目录执行 `fsutil.exe` 命令无法生效，大小写敏感只对新增的文件变更有效。

------
macOS 原生系统进行编译：
------
1. 在 AppStore 中安装 Xcode

2. 安装 Homebrew：

   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. 使用 Homebrew 安装工具链、依赖与基础软件包：

   ```bash
   brew install coreutils diffutils findutils gawk gnu-getopt gnu-tar gnu-sed grep make \
     ncurses pkgconf wget quilt xz bash git rsync gpatch python3 python-setuptools go swig
   ```

   宿主编译器使用 Command Line Tools 提供的 Clang，目标工具链由构建系统生成。
   Apple Silicon 上编译 Go 软件包时需要已安装的 Go 进行自举，构建系统会自动从 `PATH` 中查找。
   `python-setuptools` 为仍使用 `distutils` 的软件包构建脚本提供兼容模块。
   `swig` 用于 U-Boot 等组件的 Python FDT 绑定生成。
   为 U-Boot 的 binman 镜像打包工具准备 Python 依赖：

   ```bash
   python3 -m venv --system-site-packages ~/.venvs/lede-build
   source ~/.venvs/lede-build/bin/activate
   python3 -m pip install pyelftools
   export PYTHONPATH="$(python3 -c 'import site; print(site.getsitepackages()[0])')${PYTHONPATH:+:$PYTHONPATH}"
   ```

   每次新开终端编译前需重新激活该环境并执行上述 `export`，使构建系统缓存的 Python 命令也能找到模块。
   `--system-site-packages` 保留 Homebrew setuptools 的可用性。

4. 准备**区分大小写**的构建文件系统。macOS 默认的 APFS 通常不区分大小写，不能直接在默认的家目录中编译。
   在“磁盘工具”中向现有 APFS 容器添加一个“APFS（区分大小写）”卷，例如 `LEDE-build`，然后在 `/Volumes/LEDE-build` 中存放源码和构建目录。
   建议至少保留 100 GB 可用空间；多个 target 的构建回归需要更多空间。APFS 卷默认共享容器空间，可不设置配额上限。

5. 进入 Bash，配置 GNU 工具路径（通过 Homebrew 前缀同时兼容 Intel 和 Apple Silicon）：

   ```bash
   # Intel Mac 的 Homebrew 通常位于 /usr/local，Apple Silicon 通常位于 /opt/homebrew
   eval "$(/opt/homebrew/bin/brew shellenv)" # Intel Mac 请改为 /usr/local/bin/brew
   bash
   export BREW_PREFIX="$(brew --prefix)"
   for tool in coreutils diffutils findutils gnu-tar gnu-sed grep make; do
     export PATH="$BREW_PREFIX/opt/$tool/libexec/gnubin:$PATH"
   done
   export PATH="$BREW_PREFIX/opt/gnu-getopt/bin:$PATH"
   umask 022
   ```

   每次新开终端编译前需重新加载以上环境；也可将 `export` 和循环部分保存到 `~/.bashrc`，进入 Bash 后执行 `source ~/.bashrc`。
   macOS 默认的 Zsh 不会自动加载 `~/.bashrc`。

6. 在上述大小写敏感卷内获取源码，按本文的 feeds 和配置步骤进行编译；使用普通用户执行，不要使用 `sudo make`。
   例如选择 `Target System: x86`、`Subtarget: x86_64` 后：

   ```bash
   make defconfig
   make download -j8
   make V=s -j$(nproc)
   # 如果失败，使用单线程详细输出定位首个错误
   make -j1 V=s
   ```

   长时间或通过 SSH 编译时，可使用 `caffeinate -i make V=s -j$(nproc)`，在构建任务运行期间防止 Mac 因空闲而睡眠。

   这是 macOS 原生交叉编译，Apple Silicon 主机也可构建 x86_64 固件，无需虚拟机或容器。
   Airoha AN7581/AN7583 的 BL2 构建会自动下载经过 SHA256 校验的 Arm 工具链；Apple Silicon 使用原生 macOS ARM64 版本。Intel Mac 需额外执行 `brew install arm-none-eabi-gcc`。

   UML（User Mode Linux）目标仅支持 Linux x86_64 主机构建，不适用于本节的 macOS 原生构建流程。

## 固件下载

当前正式版：**v2026.11.14**（x86_64）

文件：`Homelede_2026.11.14`

百度网盘：https://pan.baidu.com/s/1nG07VIKq8nyaS9HxYpsvjw?pwd=cv5x 提取码：`cv5x`

> HomeLede 2026 版本之间支持在系统内保留配置更新。升级前建议先备份配置；此说明不适用于从 2024 及更早版本跨代升级。

[版本发布合集与历史下载](https://github.com/xiaoqingfengATGH/HomeLede/wiki/HomeLede%E7%89%88%E6%9C%AC%E5%8F%91%E5%B8%83)

Google网盘：https://drive.google.com/open?id=1iUDsgh1y5qouP48V61aTsswi3IekscKk

## 交流
* [电报群](https://t.me/t_homelede)
* [QQ群1：1030484865](https://jq.qq.com/?_wv=1027&k=PtlQp9Z9)
* [QQ群2：807741215](https://jq.qq.com/?_wv=1027&k=z9phzgtx)
* [QQ群3：1001944162](https://jq.qq.com/?_wv=1027&k=gEADVcI5)

