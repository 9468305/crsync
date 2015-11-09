# crsync
a C library of Client-side rsync over HTTP via curl

## Guide
基于rsync算法的客户端二进制增量差异更新组件  
1. 纯C语言实现, 使用libcurl HTTP通讯，跨平台兼容性良好。
2. 客户端使用rsync算法计算与服务器新版本的二进制差异，并增量下载热更新。  
3. 服务器仅需支持HTTP File传输，普通CDN即可，无须搭建rsyncd。  
4. 目前已支持Android，Windows，理论上支持所有平台。  
5. 源码简单易懂，核心部分仅1000行，可随意扩展修改。  

## Workflow
+ 客户端使用libcurl下载资源，断点续传。
+ 客户端计算文件版本差异，增量下载差异内容并合并到本地。
+ 轻量级版本控制策略组件。