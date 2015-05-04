**最终实现效果:**  
1. 无版本概念,任何本地文件均可增量升级到最新.服务器不用管理多版本  
2. 内存小,100M文件升级时只占用500KB内存.  
  
**使用流程:**  
1. 制作新版本,上传HTTP File Server.  
2. Client自动计算差异,下载差异,合并差异.  
3. done!  
  
#0.缘起  
目前主流的文件增量更新方案是**bs diff/patch**，以及google chrome Courgette改进方案。  
无尽之剑Android版最初使用该方案，在封测期间发现存在问题:  
  
1. 多版本运营繁琐。  
假设目前先后开发了5个版本，1.0～5.0，运营中需要制作  
1-2.patch  
2-3.patch  
3-4.patch  
4-5.patch  
**n个版本,n-1个patch包**  
或者  
1-2.patch  
1-3.patch 2-3.patch  
1-4.patch 2-4.patch 3-4.patch  
1-5.patch 2-5.patch 3-5.patch 4-5.patch  
**n个版本,(n-1)阶乘个patch包**  
  
2. patch依赖本地版本文件完整性  
如果本地文件损坏或被篡改，就无法增量升级，只能重新下载完整包.  
  
3. bs diff/patch算法性能和**内存**开销太高  
> from [bsdiff官网](http://www.daemonology.net/bsdiff/)  
> bsdiff is quite memory-hungry. It requires max(17*n,9*n+m)+O(1) bytes of memory, where n is the size of the old file and m is the size of the new file. bspatch requires **n+m+O(1)** bytes.  
> bsdiff runs in O((n+m) log n) time; on a 200MHz Pentium Pro, building a binary patch for a 4MB file takes about 90 seconds.  
> bspatch runs in O(n+m) time; on the same machine, applying that patch takes about two seconds.  
  
bsdiff执行慢,可以,200M文件制作patch包需要运行64位版本，可以.  
bspatch执行慢,可以。需要占用**n+m**内存,这点无法接受.  
无尽之剑Android版的资源包单个最大200M，在游戏运行同时进行200M+内存开销的patch操作，非常容易OOM.  
  
**那么问题来了：有没有一种版本无关，内容无关，内存开销小的增量升级方案呢?**  
这就是我的**crsync**:  
纯C语言实现,基于rsync rolling算法, Client Side计算diff和执行patch，通过curl进行HTTP range增量下载  
(同类方案有zsync,用于ubuntu package增量更新)
  
1. 客户端计算本地版本和服务器最新版本之间的diff，然后通过http range下载差异部分.  
  
2. 内存开销 = 文件size/分块size * 40字节 + HashTable开销.  
假设文件size 100M，分块size 16K,那么patch操作最多只需500KB内存。  
  
3. 版本无关, 本地文件是否改动无关，只关注本地和远端之间的内容差异.  
  
#1.rsync rolling  
检测2个文件的差异和重复数据，有2种策略:定长分块和变长分块.  
rsync算法属于前者,RDC算法属于后者,多轮rsync可以看作是对定长分块的变长优化.  
  
定长分块是将A文件按固定块大小切分,计算校验值。在B文件中遍历,计算所有相同块大小的校验值,相同即一致，不同即差异.  
以下假设文件size=100M, 分块size=1KB,测试数据基于公司开发机i7-3770 CPU.  
  
A文件切分,计算[0]~[1023], [1024]~[2047],[2048]~[3071]...的校验值，输出一张A表.  
B文件遍历,计算[0]~[1023], [1]~[1024],[2]~[1025],...的校验值,在A表中遍历比对.相同即重复,不同即差异.  
  
不足1KB的数据,rsync补齐0再计算校验值，与B文件遍历计算比对.  
crsync不计算,直接将之存入A表末尾,当作差异.  
  
校验算法很多,常用MD4,MD5,SHA-1,BLAKE2均可.  
这样明显很慢,如何优化?

rsync使用2个校验值,**弱校验**和**强校验**.  
弱校验是计算一个32位的CRC值,优点: 快; 缺点: 冲突率高;校验值相同时数据可能不同.  
强校验是上面说到的各种算法,优点: 冲突率低; 缺点: 慢.  
首先做弱校验,值相同再做强校验.以此保证快速与准确.  
遍历文件[0]~[1023], [1]~[1024],[2]~[1025],...需计算104856577次弱校验  
这样依然很慢，如何优化?  
  
CRC校验算法很多,rsync使用**adler32**算法,因此有了**rolling**的特性.  
alder32算法由Mark Adler于1995发明,用于zlib.
一个32位的adler32值由2个16位的A和B值构成。  
A值=buffer中所有byte相加,对2^16取模(65536).  
b值=buffer中所有A相加,对2^16取模(65536).  
公式如下:  
> A = 1 + D1 + D2 + ... + Dn (mod 65521)  
> B = (1 + D1) + (1 + D1 + D2) + ... + (1 + D1 + D2 + ... + Dn) (mod 65521)  
> = n×D1 + (n−1)×D2 + (n−2)×D3 + ... + Dn + n (mod 65521)  
> Adler-32(D) = B × 65536 + A  
  
65521是65536范围内的最大素数,因此mod=65521即可.  
librsync(rdiff)在A和B值计算时再偏移31,rsync偏移0,crsync偏移0.  

有趣的是,这个公式等价于:  
> A += data[i];  
> B += A;  
  
rolling是什么?  
计算出[0]~[1023]的校验值,再计算[1]~[1024]的校验值时,只需减去data[0],加上data[1024]即可!不需重新计算!  
> A -= data[0];  
> A += data[1024];  
> B -= 1024 * data[0];  
> B += A;  
  
如此滑动遍历,即可得到整个文件的所有分块的校验值.(大赞!)  
实测文件size 100M,分块size 2K,遍历计算仅耗时275ms.  
  
#2.rsync HashTable  
**rolling计算足够快了,用B文件每个块的校验值check A文件的校验表,如何优化?**  
  
A文件的校验表size = 102400,冒泡?快速?折半?  
rsync使用二段HashTable来存储校验值,即即H(A, H(B, MD5))  
用adler32的A做key, B+MD5做value, 其中再用alder32的B做key, MD5做value.  
表大小2^16,满足A的值范围.冲突值用LinkList存储.  
  
首先命中A,再命中B,最后顺序遍历LinkList命中MD5.完成!  
直接使用adler32值做key,MD5做value也可以,选择合适的散列算法即可.  

**HashTable解决了命中率的问题,但是实际中存在大量的missing查找.如何优化?**  
  
zsync在此基础上,使用一个bitbuffer来构建"缺失表",每个分块用3个bit表示,首先做位运算,若存在再访问HashTable,否则直接跳过.  
  
crsync使用2^20的Bloom filter做faster missing. 为什么是2^20? 兼顾速度提升和内存开销(2^20 = 128KB)  
  
#3.client-side rsync  
rsync是C/S架构,依赖rsyncd服务端存在,执行流程是:  

1. client生成本地校验表.发送给server  
2. server执行rsync rolling,发送相同块的index和差异块的数据.  
3. client接受指令,执行merge.  
  
是否可以去掉rsyncd server,只依赖HttpServer(CDN)存储文件和校验表,客户端执行rsync rolling?  
crsync和zsync为此而生.  
  
1. 制作新版本文件,生成校验表,一并上传CDN. 
2. client下载新版本的校验表.  
3. client根据本地文件和新校验表,执行rsync rolling,得到重复数据index和差异数据的块index.  
4. client重用本地数据,下载差异数据,合并写入新文件.  
5. done!  

#4.crsync VS bs diff/patch  

**测试对比1:**  
测试文件: 无尽之剑Android版资源包, base14009.obb ~ base14012.obb 92MB～93MB  
编译版本: MinGW 32bit release  
Bloom filter: 2^20  
纯本地差异查找与合并,无网络开销  

测试机: DELL PC, Intel i3 550 (4core 3.2GHz) 4GB RAM  

| 分块大小 | 校验表生成时间 | 校验块数量 | 差异查找时间 | 重复块数量 | 差异块大小 |
| :------- | :----------: | :-------: | :---------: | :------: | --------: |
| 2K      | 2182 ms      | 46328     |  3753 ms    | 40418    | 11820 KB  |
| 4K      | 2156 ms      | 23164     |  2914 ms    | 18960    | 16816 KB  |
| 8K      | 2154 ms      | 11582     |  2719 ms    | 9249     | 18664 KB  |

测试机 HP PC, Intel i7-3770 (8core 3.4GHz) 8GB RAM:  

| 分块大小 | 校验表生成时间 | 校验块数量 | 差异查找时间 | 重复块数量 | 差异块大小 |
| :------- | :----------: | :-------: | :---------: | :------: | --------: |
| 2K      | 1745 ms      | 46328     |  2974 ms    | 40418    | 11820 KB  |

**crsync VS bspatch**  

| 算法 | 差异量 | 内存开销 |
| :--- | :----------: | --------: |
| bsdiff | 16.7MB (diff文件)                         | 108.7MB (92MB+16.7MB) |
| crsync | **12.36MB**(校验表835KB + 差异数据11820KB) | **1.76MB**            |

**测试对比2:**  
测试文件: [我叫MT online Android标准版](http://update2.locojoy.com/android/IamMT/150413/IamMT_200100100_locojoy_standard_ac_4.5.4.0_4540.apk) 90.7MB ~ [我叫MT online Android高清版](http://update.locojoy.com/Client/Android/MT-A/0324/IamMT_200100100_locojoy_ac_hd_4.5.4.0_4540.apk) 102MB  
(测试版本基于2015.4.4官网下载,目前已变更)


测试机 HP PC, Intel i7-3770 (8core 3.4GHz) 8GB RAM:  

| 分块大小 | 校验表生成时间 | 差异查找时间 | merge时间  |
| :------- | :----------: | :---------: | --------: |
| 2K      | 1379 ms      |  2950 ms    | 90ms      |

**crsync VS bs diff/patch**  

|  算法  | 差异量                 | 内存开销                 |
| :------- | :----------: | --------: |
| bsdiff | **29.5MB** (diff文件) | 120.2MB (90.7MB+29.5MB) |
| crsync | 40.8MB               |  **1.7MB**              |

**这里发现, 为什么无尽之剑资源包使用crsync得到的差异量更小?**  
**那么问题来了, 如何设计出适合差异更新的文件格式**  
几种格式对比:  
  
1. 整个文件不压缩,完整下载量大,更新量小,bsdiff更优,因为diff文件使用bzip2压缩格式.crsync使用原始数据http range下载.  
2. 整个文件zip压缩,完整下载量大,更新量更大,这时bsdiff和crsync都无效.因为少量改动就会造成整个文件的大量差异.  
3. 文件内容用zip压缩,再序列化到一起,完整下载量小,更新量小.这时crsync更优,因为bsdiff对zip压缩过的数据处理会生成额外数据量.  
  
UnrealEngine3 Android使用的obb格式为:**文件头**(贴图原始大小, 压缩大小,文件体偏移位置)+**文件体**(贴图内容zip压缩).  
资源更改只影响zip压缩部分和文件头索引内容.对其他未更改资源无影响.  
  
Android APK是zip压缩格式,但是特定后缀名的文件不压缩,常见图片音视频文件为了支持流式read保持原始格式.  
因此crsync对比bsdiff的差异量大些.  
  
#5.源码  
纯C语言实现, 遵循C99, 核心部分仅2个文件 crsync.h crsync.c 880行代码.static-link libcurl.  
release版Android so 166KB, Windows mingw32 exe 299KB.  
API实现参考curl设计  

```c
CRSYNCcode crsync_global_init();  
crsync_handle_t* crsync_easy_init();  

CRSYNCcode crsync_easy_setopt(crsync_handle_t *handle, CRSYNCoption opt, ...);  

CRSYNCcode crsync_easy_perform_match(crsync_handle_t *handle);  
CRSYNCcode crsync_easy_perform_patch(crsync_handle_t *handle);  

void crsync_easy_cleanup(crsync_handle_t *handle);  
void crsync_global_cleanup();  
```

源码托管在[github crsync](https://github.com/9468305/crsync), MIT License, 目前暂时屏蔽访问.  
(上班时间所做,版权应该是属于公司,得到开源授权后再开放访问).  
  
#6.附录  
- [rsync官网](http://rsync.samba.org/)
- [rsync算法论文](http://samba.org/~tridge/phd_thesis.pdf) 
- [librsync](http://librsync.sourceforge.net/)
- [zsync](http://zsync.moria.org.uk/)
- [BS diff/patch](http://www.daemonology.net/bsdiff/)
- [Jarsync - a Java rsync implementation](http://sourceforge.net/projects/jarsync/)
- [rsync教程](http://tutorials.jenkov.com/rsync/index.html)
- [rsync java教程](http://javaforu.blogspot.com/2014/01/rsync-in-java-quick-and-partial-hack.html)
- [REBL算法](http://www.research.ibm.com/people/f/fdouglis/papers/rebl.pdf)
- [TAPER算法](http://www.usenix.org/events/fast05/tech/full_papers/jain/jain.pdf)
