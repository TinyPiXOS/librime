<meta charset="UTF-8">

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

RIME: Rime 输入法引擎; 

TinyPiXOS 基于 librime V1.5.3版本，移除了对boost库的依赖，使用原生C++11前版本语法及LinuxAPI代替boost。

[原始仓库地址](https://github.com/rime/librime.git)

安装
===

编译依赖
---

- compiler with C++11 support
- cmake>=2.8
- libboost>=1.48
- libglog (optional)
- libleveldb
- libmarisa
- libopencc>=1.0.2
- libyaml-cpp>=0.5
- libgtest (optional)
- tinyPiXOS 信号槽 V0.1.1

运行依赖
---

- libglog (optional)
- libleveldb
- libmarisa
- libopencc
- libyaml-cpp

Linux平台构件安装librime步骤
---

```bash
apt install libleveldb-dev libmarisa-dev libopencc-dev libyaml-cpp-dev libgoogle-glog-dev
make
sudo make install
```

使用librime安装依赖
---

```bash
apt install libleveldb-dev libmarisa-dev libopencc-dev libyaml-cpp-dev libgoogle-glog-dev
```

部署
---

TARGET_PATH = tinyPiXCore/src

- 拷贝install目录下include/* -> TARGET_PATH/include_p/Utils/rime
- 拷贝install目录下lib/librime.so.1.5.3 -> TARGET_PATH/depend_lib/dynamic/x86_x64或者arm_64
- 拷贝install目录下data/* -> TARGET_PATH/data/rime
- 第一次部署需要在data目录下执行 ./rime_deployer --build
