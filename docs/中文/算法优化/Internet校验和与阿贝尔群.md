## 缘由

在知名的TCP详解三卷中的卷一中，关于IP协议章节中，作者介绍了Internet校验和算法，不过在介绍该算法时，作者也简单聊了一下该算法的数学性质，这些信息在原书128页（中文翻译版本）。

作者的描述十分简短，就几行，只是证明了{0001,.....FFFF}这个集合是一个阿贝尔群，甚至都看不出与Internet校验和算法有什么关系，因此笔者还是简单补充一下。



## 理论

Internet校验和算法如下，在笔者编写的TCP/IP协议栈SKRTOS_tcp中使用了这段代码：

其实就是将所有内容看成是十六位的字，将它们相加，然后进行循环进位，最后返回其反码。

```
unsigned short in_checksum(void *b, int len) 
{
    unsigned short *addr = (unsigned short *)b;
    long sum = 0;

    for (len; len > 1; len -= 2) {
        sum += *(unsigned short *)addr++;
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    if (len) {
        sum += (unsigned short)(*(unsigned char *)addr);
    }
    while(sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }

    return ~sum;
}
```

## 性质

对于{0001,.....FFFF}这个集合V是一个阿贝尔群，证明如下：

1.对于V中的任何X,Y,（X + Y）都在V中 （封闭性）

2.对于V中的任何X,Y,Z, X + (Y + Z）= (X + Y) + Z     (结合性)

3.对于V中的任何X,（e + X）= X + e = X, 其中e = FFFF （单位元）

4.对于V中的任何X,Y, 有一个X'在V中，使得X + X' = e （逆元）

5.对于V中的任何X,Y, X  + Y = Y + X  （可交换）

那么Internet校验和是否符合上面的性质呢？

Internet校验和算法的运算确实是阿贝尔群。

因为在Internet计算中，{0000,.....FFFF}集合P 与{0001,.....FFFF}集合V完全等价。

### 思考

对于集合V，如果引入0000这个元素，那么是不是就有两个单位元了，这个集合是不是不会构成阿贝尔群呢？

其实不是的，因为在该运算中，0000与FFFF其实是等价的，它们只是不同的表示方法。

## 数学运算

将internet校验和归纳为数学运算，其实就是：

```
while(x 运算 y) = (x + y) % 0xFFFF
```

### 单个运算等价性证明

1.基数定义：设 M = 2^16 ，N = 65535 = M - 1

2.高低位分解：对任意非负整数 s，存在唯一整数 q, r 使得 
s = q · M + r,   0 ≤ r < M 
其中 
q = s >> 16 
r = s & 0xFFFF

3.一次回卷：定义算子 E(s) = r + q

**差值计算：** 
E(s) - s = (r + q) - (qM + r) 
         = q(1 - M) 
         = -q(M - 1) 
         = -qN

**同余结论：** 由于 E(s) - s = -qN，故 E(s) ≡ s (mod N) 
即 ：E(s) ≡ s (mod 0xFFFF)

### 多次回卷的同余性

因为每次回卷都满足E(s) ≡ s (mod 0xFFFF)，

所以无论你回卷多少次，最终结果都与原始 s mod 0xFFFF 同余

### 证明最终结果

- 每次回卷都会把高 16 位折叠到低 16 位，数值严格减小（除非已经在 16 位范围内）。
- 因此经过有限次回卷，必然得到一个小于 0xFFFF的数。
- 记最终结果为 Ek(s)，那么既然Ek(s)要满足Ek(s) ≡ s (mod 0xFFFF) ，而且它还小于0xFFFF，那么必然有Ek(s) = s mod 0xFFFF成立。

## 性质

那么，也就是说，现在的Internet校验和算法中的内容是可交换的，只要我没有更改其中的值，而只是替换十六位字内容的顺序，Internet校验和算法的结果不会有任何影响。

如何破坏该算法的性质呢？如果我们想检验内容中的字的顺序，那么肯定不能让该集合构成一个阿贝尔群了。

### 可行性？

如果不使用long类型存储求和结合，而是使用short类型，那么，看起来此时FFFF也成为了单位元，因为此时有X + 0xFFFF = X，向高位进的一又被加回来了。

但是，由于上面笔者说过，0和0xFFFF是等价的，让我们尝试证明运算结果不变：

short数据类型相加，也就是说，两个数据相加时，只要超过0xFFFF，都会触发mod 0xFFFF操作。

由于(A mod B) mod B 等价于A mod B，且运算本身满足交换律与结合律，所以，

设x1、x2、x3......这些都是数据包中的内容。

我们有(x1 + x2）mod 0xFFFF = x1 mod 0xFFFF + x2 mod 0xFFFF ，

不管由于触发多少次mod运算，我们都可以将其转化为x1 mod 0xFFFF + x2 mod 0xFFFF这种形式，而x1、x2这些本身又小于0xFFFF，例如：

```
((x1 + x2) % 0xFFFF + x3) % 0xFFFF = ((x1 + x2) % 0xFFFF) % 0xFFFF + x3 % 0xFFFF = x1 % 0xFFFF + x2 %0xFFFF + x3 % 0xFFFF
```

**先模再加** 与 **先加再模** 是等价的，因此结果不变。

从这里可以看出，我们的集合貌似还是一个阿贝尔群，也就是说，0和0xFFFF就是同一个数！

那么要怎么破坏性质呢？

## 定义新运算

在每次对内容加法前，对左操作数做向左移动1 位，如果溢出则环回。

通过这种方法，不同位置的数字会有不同的操作，这样就破坏了交换性。

也就是说：1 + 2 + 3与 3 + 2 + 1的结果是不同的，因为我们每次都会对左操作数进行左移运算。

# 验证

为了验证我们的结论，让我们修改校验和算法，检查协议栈是否能正常运行：

```
unsigned short in_checksum(void *b, int len) 
{
    unsigned short *addr = (unsigned short *)b;
    long sum = 0;

    for (len; len > 1; len -= 2) {
        sum += *(unsigned short *)addr++;
        if (sum & 0x80000000) {
            //sum = (sum & 0xFFFF) + (sum >> 16);
            sum = sum % 0xFFFF;
        }
    }
    if (len) {
        sum += (unsigned short)(*(unsigned char *)addr);
    }
    /*
    while(sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }
    */
    sum = sum % 0xFFFF;
    
    return ~sum;
}
```

尝试ping一下：

```
skaiuijing@ubuntu:~/NetStack/testdoc$ ping 192.168.1.200
PING 192.168.1.200 (192.168.1.200) 56(84) bytes of data.
64 bytes from 192.168.1.200: icmp_seq=1 ttl=64 time=0.177 ms
64 bytes from 192.168.1.200: icmp_seq=2 ttl=64 time=0.196 ms
64 bytes from 192.168.1.200: icmp_seq=3 ttl=64 time=0.189 ms
64 bytes from 192.168.1.200: icmp_seq=4 ttl=64 time=0.168 ms
64 bytes from 192.168.1.200: icmp_seq=5 ttl=64 time=0.192 ms
64 bytes from 192.168.1.200: icmp_seq=6 ttl=64 time=0.169 ms
^C
--- 192.168.1.200 ping statistics ---
6 packets transmitted, 6 received, 0% packet loss, time 5122ms
rtt min/avg/max/mdev = 0.168/0.181/0.196/0.020 ms
```

尝试发送消息给Linux tcp服务端：

```
skaiuijing@ubuntu:~/NetStack/testdoc$ ./handle
Server listening on port 8080...
Connection from 192.168.1.200:1234
Received: Hello,linux Server
Client sent FIN.
Closed connection properly (FIN sent).
```

一切正常，但是，在笔者编写的网络协议栈netstack的打印中，有这么一句报错：

```
IPv4
Error: ip input check sum error!
 | File: /home/skaiuijing/NetStack/net/source/ip_input.c | Function: ip_input | Line: 53
tcp input!
csum:65535
Error: tcp input check sum error!
 | File: /home/skaiuijing/NetStack/net/source/tcp_input.c | Function: tcp_input | Line: 80
tp_stat:5
flags:17
FIN_WAIT1
FLAGS:16
```

由于笔者并没有检查到校验错误后就回收数据包，仅仅是打印报错。但是观察linux 服务端，却仍然是正常运行，再次完成了三次握手和四次挥手，这说明其实我们的校验和算法仍然是正确的：

```
skaiuijing@ubuntu:~/NetStack/testdoc$ ./handle
Server listening on port 8080...
Connection from 192.168.1.200:1234
Received: Hello,linux Server
Client sent FIN.
Closed connection properly (FIN sent).
//上面完成了第一次测试，下面是第二次
Connection from 192.168.1.200:1234
Received: Hello,linux Server
Client sent FIN.
Closed connection properly (FIN sent).
```

这是为什么呢？

## 单位元的等价性

在前面笔者说过，在internet校验和运算中，0与0xFFFF是等价的，是同一个单位元的不同表示，所以让我们验证这个结论，再次修改校验和算法：

```
unsigned short in_checksum(void *b, int len) 
{
    unsigned short *addr = (unsigned short *)b;
    long sum = 0;

    for (len; len > 1; len -= 2) {
        sum += *(unsigned short *)addr++;
        if (sum & 0x80000000) {
            //sum = (sum & 0xFFFF) + (sum >> 16);
            sum = sum % 0xFFFF;
        }
    }
    if (len) {
        sum += (unsigned short)(*(unsigned char *)addr);
    }
    /*
    while(sum >> 16) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }
    */
    sum = sum % 0xFFFF;
    if (sum == 0) {
        return sum;
    } 
    
    return ~sum;
}
```

通过之前的运算，我们计算出来的校验和是0xFFFF和0，但是本质上它们是等价的，所以其实不影响Linux协议栈的正常校验运算，这也是为什么三次握手四次挥手能正常运行的原因：

但是由于我们这边的协议栈使用的是取模运算，所以计算结果往往是0xFFFF，而代码逻辑又都是判断是否为0，因此导致了报错；

现在让我们再次运行，报错消失了，而ping这些命令仍然可以正常运行：

```
skaiuijing@ubuntu:~/NetStack/testdoc$ ./handle
Server listening on port 8080...
Connection from 192.168.1.200:1234
Received: Hello,linux Server
Client sent FIN.
Closed connection properly (FIN sent).
Connection from 192.168.1.200:1234
Received: Hello,linux Server
Client sent FIN.
Closed connection properly (FIN sent).
^C
skaiuijing@ubuntu:~/NetStack/testdoc$ ping 192.168.1.200
PING 192.168.1.200 (192.168.1.200) 56(84) bytes of data.
64 bytes from 192.168.1.200: icmp_seq=1 ttl=64 time=0.097 ms
64 bytes from 192.168.1.200: icmp_seq=2 ttl=64 time=0.112 ms
64 bytes from 192.168.1.200: icmp_seq=3 ttl=64 time=0.132 ms
64 bytes from 192.168.1.200: icmp_seq=4 ttl=64 time=0.104 ms
64 bytes from 192.168.1.200: icmp_seq=5 ttl=64 time=0.129 ms
^C
--- 192.168.1.200 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 4081ms
rtt min/avg/max/mdev = 0.097/0.114/0.132/0.019 ms
```

这再次验证了我们的结论：

对于{0000,.....FFFF}这个集合，它仍然是一个阿贝尔群，因为0000和FFFF本来就是等价的。

## 结语

从上面的论证，我们知道internet校验和的数学性质其实可以等价于取模运算，反过来看，我们得到了运算优化相关算法：

```
X % 2^16 = X & (2^16 - 1) 
X % (2^16 - 1) = while(x高16位 回卷运算 x低16位)
```

