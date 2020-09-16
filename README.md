TODO
--------

V0.1.0 可用，但性能有问题  
V0.2.0 Commits on Jan 8, 2016。未打Tag，可用，性能优化。  
V0.3.0 计划把整个工程从QMake切换至CMake，完善可用性。未完成。  

crsync
--------

A library and CLI tool of Client-side rsync over HTTP via curl, implemented by pure C.

[Guide](./doc/README.md)
--------

基于rsync算法的客户端二进制增量差异更新组件。

1. 纯C语言实现, 使用libcurl HTTP通讯，跨平台兼容性良好。  
2. 客户端使用rsync算法计算与服务器新版本的二进制差异，并增量下载更新数据。  
3. 服务器仅需支持HTTP File传输，普通CDN即可，无须搭建rsyncd。  
4. 目前已支持Android，Windows，理论上支持所有平台。  
5. 源码简单易懂，核心部分仅1000行，可随意扩展修改。  

[Workflow](./doc/workflow.md)
--------

+ 服务端生成新版本的摘要文件, 并部署到HTTP FileServer
+ 客户端使用libcurl下载资源，断点续传。
+ 客户端计算文件版本差异，增量下载差异内容并合并到本地。
+ 客户端附带一个轻量级版本控制策略组件。

Android Build
--------

Run `src/crsync-build.cmd`, Output `src/libs/TARGET_ARCH_ABI/libcrsync.so`.

Qt MinGW Build
--------

Build `src/crsync.pro`, Output `src/m32/crsync.exe`

Patent  
--------

[一种数据文件的增量更新方法和服务器、客户端以及系统](https://www.google.com.pg/patents/CN106528125A)，由原团队成员 @nwpu053741 撰写并申请。

License
--------

```txt
   Copyright 2015 ChenQi

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
```
