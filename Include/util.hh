#ifndef UTIL_H
#define UTIL_H
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <exception>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include "color.h"
#include "pcap.h"

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef USE_JSON
#include "include/rapidjson/document.h"
#include "include/rapidjson/filewritestream.h"
#include "include/rapidjson/prettywriter.h"
#include "include/rapidjson/stringbuffer.h"
#include "include/rapidjson/writer.h"
namespace json = rapidjson;
#endif

/*
                           TCP Header Format
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

From: https://www.freesoft.org/CIE/Course/Section4/8.htm
*/
struct tcp_header {
    u_short s_port;
    u_short d_port;
    u_int seq;
    u_int ack;
    u_short flags;
    u_short window;
    u_short check_sum;
    u_short urg_ptr;
    u_int options_padding;
} __attribute__((packed));
struct ip_address {
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;
};
struct v6_address {
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;
    u_char byte5;
    u_char byte6;
    u_char byte7;
    u_char byte8;
    u_char byte9;
    u_char byte10;
    u_char byte11;
    u_char byte12;
    u_char byte13;
    u_char byte14;
    u_char byte15;
    u_char byte16;
};
struct ip_header {
    u_char ver_ihl;  // Version (4 bits) + Internet header length (4 bits)
    u_char tos;      // Type of service
    int16_t tlen;    // Total length
    int16_t identification;  // Identification
    int16_t flags_fo;        // Flags (3 bits) + Fragment offset (13 bits)
    u_char ttl;              // Time to live
    u_char proto;            // Protocol
    int16_t crc;             // Header checksum
    ip_address saddr;        // Source address
    ip_address daddr;        // Destination address
    u_int op_pad;            // Option + Padding
} __attribute__((packed));

struct v6_header {
    u_int ver_traffic_flow;
    u_short payload_len;
    u_char next_header;
    u_char hop_limit;
    v6_address source_addr;
    v6_address dest_addr;
} __attribute__((packed));

/* UDP header*/
struct udp_header {
    int16_t sport;  // Source port
    int16_t dport;  // Destination port
    int16_t len;    // Datagram length
    int16_t crc;    // Checksum
};

// DNS header
// https://www2.cs.duke.edu/courses/fall16/compsci356/DNS/DNS-primer.pdf
struct dns {
    // header field
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
};

/*
0	0x00	HOPOPT	IPv6逐跳选项	RFC 2460
1	0x01	ICMP	互联网控制消息协议（ICMP）	RFC 792
2	0x02	IGMP	因特网组管理协议（IGMP）	RFC 1112
3	0x03	GGP	网关对网关协议	RFC 823
4	0x04	IPv4	IPv4 (封装) / IP-within-IP 封装协议（IPIP）	RFC 2003
5	0x05	ST	因特网流协议	RFC 1190, RFC 1819
6	0x06	TCP	传输控制协议（TCP）	RFC 793
7	0x07	CBT	有核树组播路由协议	RFC 2189
8	0x08	EGP	外部网关协议	RFC 888
9	0x09	IGP	内部网关协议（任意私有内部网关（用于思科的IGRP））
10	0x0A	BBN-RCC-MON	BBN RCC 监视
11	0x0B	NVP-II	网络语音协议	RFC 741
12	0x0C	PUP	Xerox PUP
13	0x0D	ARGUS	ARGUS
14	0x0E	EMCON	EMCON
15	0x0F	XNET	Cross Net Debugger	IEN 158
16	0x10	CHAOS	Chaos
17	0x11	UDP	用户数据报协议（UDP）	RFC 768
18	0x12	MUX	多路复用	IEN 90
19	0x13	DCN-MEAS	DCN Measurement Subsystems
20	0x14	HMP	Host Monitoring Protocol	RFC 869
21	0x15	PRM	Packet Radio Measurement
22	0x16	XNS-IDP	XEROX NS IDP
23	0x17	TRUNK-1	Trunk-1
24	0x18	TRUNK-2	Trunk-2
25	0x19	LEAF-1	Leaf-1
26	0x1A	LEAF-2	Leaf-2
27	0x1B	RDP	可靠数据协议	RFC 908
28	0x1C	IRTP	Internet Reliable Transaction Protocol	RFC 938
29	0x1D	ISO-TP4	ISO Transport Protocol Class 4	RFC 905
30	0x1E	NETBLT	Bulk Data Transfer Protocol	RFC 998
31	0x1F	MFE-NSP	MFE Network Services Protocol
32	0x20	MERIT-INP	MERIT Internodal Protocol
33	0x21	DCCP	Datagram Congestion Control Protocol	RFC 4340
34	0x22	3PC	Third Party Connect Protocol
35	0x23	IDPR	Inter-Domain Policy Routing Protocol	RFC 1479
36	0x24	XTP	Xpress Transport Protocol
37	0x25	DDP	Datagram Delivery Protocol
38	0x26	IDPR-CMTP	IDPR Control Message Transport Protocol
39	0x27	TP++	TP++ Transport Protocol
40	0x28	IL	IL Transport Protocol
41	0x29	IPv6	IPv6 封装	RFC 2473
42	0x2A	SDRP	Source Demand Routing Protocol	RFC 1940
43	0x2B	IPv6-Route	IPv6路由拓展头	RFC 2460
44	0x2C	IPv6-Frag	IPv6分片扩展头	RFC 2460
45	0x2D	IDRP	Inter-Domain Routing Protocol
46	0x2E	RSVP	资源预留协议	RFC 2205
47	0x2F	GRE	通用路由封装（GRE）	RFC 2784, RFC 2890
48	0x30	DSR	动态源路由
49	0x31	BNA	BNA
50	0x32	ESP	封装安全协议（ESP）	RFC 4303
51	0x33	AH	认证头协议（AH）	RFC 4302
52	0x34	I-NLSP	Integrated Net Layer Security Protocol	TUBA
53	0x35	SWIPE	SwIPe	IP with Encryption
54	0x36	NARP	NBMA Address Resolution Protocol	RFC 1735
55	0x37	MOBILE	IP Mobility (Min Encap)	RFC 2004
56	0x38	TLSP	传输层安全性协议（使用Kryptonet密钥管理）
57	0x39	SKIP	Simple Key-Management for Internet Protocol	RFC 2356
58	0x3A	IPv6-ICMP	互联网控制消息协议第六版（ICMPv6）	RFC 4443, RFC 4884
59	0x3B	IPv6-NoNxt	IPv6无负载头	RFC 2460
60	0x3C	IPv6-Opts	IPv6目标选项扩展头	RFC 2460
61	0x3D		任意的主机内部协议
62	0x3E	CFTP	CFTP
63	0x3F		任意本地网络
64	0x40	SAT-EXPAK	SATNET and Backroom EXPAK
65	0x41	KRYPTOLAN	Kryptolan
66	0x42	RVD	MIT远程虚拟磁盘协议
67	0x43	IPPC	Internet Pluribus Packet Core
68	0x44		Any distributed file system
69	0x45	SAT-MON	SATNET Monitoring
70	0x46	VISA	VISA协议
71	0x47	IPCV	Internet Packet Core Utility
72	0x48	CPNX	Computer Protocol Network Executive
73	0x49	CPHB	Computer Protocol Heart Beat
74	0x4A	WSN	Wang Span Network
75	0x4B	PVP	Packet Video Protocol
76	0x4C	BR-SAT-MON	Backroom SATNET Monitoring
77	0x4D	SUN-ND	SUN ND PROTOCOL-Temporary
78	0x4E	WB-MON	WIDEBAND Monitoring
79	0x4F	WB-EXPAK	WIDEBAND EXPAK
80	0x50	ISO-IP	国际标准化组织互联网协议
81	0x51	VMTP	Versatile Message Transaction Protocol	RFC 1045
82	0x52	SECURE-VMTP	Secure Versatile Message Transaction Protocol	RFC 1045
83	0x53	VINES	VINES
84	0x54	TTP	TTP
84	0x54	IPTM	Internet Protocol Traffic Manager
85	0x55	NSFNET-IGP	NSFNET-IGP
86	0x56	DGP	Dissimilar Gateway Protocol
87	0x57	TCF	TCF
88	0x58	EIGRP	增强型内部网关路由协议（EIGRP）
89	0x59	OSPF	开放式最短路径优先（OSPF）	RFC 1583
90	0x5A	Sprite-RPC	Sprite RPC Protocol
91	0x5B	LARP	Locus Address Resolution Protocol
92	0x5C	MTP	Multicast Transport Protocol
93	0x5D	AX.25	AX.25
94	0x5E	IPIP	IP-within-IP 封装协议	与4号协议重复
95	0x5F	MICP	Mobile Internetworking Control Protocol
96	0x60	SCC-SP	Semaphore Communications Sec. Pro
97	0x61	ETHERIP	Ethernet-within-IP 封装协议	RFC 3378
98	0x62	ENCAP	封装头部	RFC 1241
99	0x63		任意的加密模式
100	0x64	GMTP	GMTP
101	0x65	IFMP	Ipsilon Flow Management Protocol
102	0x66	PNNI	PNNI over IP
103	0x67	PIM	Protocol Independent Multicast
104	0x68	ARIS	IBM's ARIS (Aggregate Route IP Switching) Protocol
105	0x69	SCPS	空间通信协议规范	SCPS-TP[1]
106	0x6A	QNX	QNX
107	0x6B	A/N	Active Networks
108	0x6C	IPComp	IP负载压缩协议	RFC 3173
109	0x6D	SNP	Sitara Networks Protocol
110	0x6E	Compaq-Peer	Compaq Peer Protocol
111	0x6F	IPX-in-IP	封装于IP的IPX
112	0x70	VRRP	虚拟路由器备援协议、共享地址冗余协议（没在IANA注册）
VRRP:RFC 3768 113	0x71	PGM	实际通用多播	RFC 3208 114	0x72		Any
0-hop protocol 115	0x73	L2TP	第二层隧道协议第三版	RFC 3931 116	0x74
DDX	D-II Data Exchange (DDX) 117	0x75	IATP	Interactive Agent Transfer
Protocol 118	0x76	STP	Schedule Transfer Protocol
119	0x77	SRP	SpectraLink Radio Protocol
120	0x78	UTI	Universal Transport Interface Protocol
121	0x79	SMP	Simple Message Protocol
122	0x7A	SM	Simple Multicast Protocol draft-perlman-simple-multicast-03
页面存档备份，存于互联网档案馆 123	0x7B	PTP	Performance Transparency
Protocol 124	0x7C	IS-IS over IPv4	负载于IPv4的IS-IS	RFC 1142 and RFC
1195 125	0x7D	FIRE	Flexible Intra-AS Routing Environment 126	0x7E
CRTP	Combat Radio Transport Protocol 127	0x7F	CRUDP	Combat Radio User
Datagram
128	0x80	SSCOPMCE	Service-Specific Connection-Oriented Protocol in a
Multilink and Connectionless Environment	ITU-T Q.2111 (1999)
页面存档备份，存于互联网档案馆 129	0x81	IPLT 130	0x82	SPS	Secure
Packet Shield
131	0x83	PIPE	Private IP Encapsulation within IP	Expired I-D
draft-petri-mobileip-pipe-00.txt 页面存档备份，存于互联网档案馆 132	0x84 SCTP
Stream Control Transmission Protocol 133	0x85	FC	    光纤通道 134	0x86
RSVP-E2E-IGNORE	Reservation Protocol (RSVP) End-to-End Ignore	RFC 3175 135
0x87	Mobility Header	IPv6移动IP扩展头	RFC 6275 136	0x88	UDPLite
UDP-Lite	RFC 3828 137	0x89	MPLS-in-IP	封装于IP协议的多协议标签交换
RFC 4023 138	0x8A	manet	MANET Protocols	RFC 5498 139	0x8B	HIP	Host
Identity Protocol	RFC 5201 140	0x8C	Shim6	Site Multihoming by IPv6
Intermediation	RFC 5533 141	0x8D	WESP	包装过的封装安全协议（ESP）	RFC
5840 142	0x8E	ROHC
*/

// https://en.wikipedia.org/wiki/Address_Resolution_Protocol
struct ARP_packet {
    u_short
        hardware_type;  // This field specifies the network link protocol type.
    u_short protocol_type;  // This field specifies the internetwork protocol
                            // for which the ARP request is intended. For IPv4,
                            // this has the value 0x0800.
    u_short length;  // Length (in octets) of a hardware address. Ethernet
                     // address length is 6. Length (in octets) of internetwork
                     // addresses. The internetwork protocol is specified in
                     // PTYPE. Example: IPv4 address length is 4.

    u_short operation;  // Specifies the operation that the sender is
                        // performing: 1 for request, 2 for reply.
    u_short sender_hardware_address
        [3];  // Media address of the sender. In an ARP request this field is
              // used to indicate the address of the host sending the request.
              // In an ARP reply this field is used to indicate the address of
              // the host that the request was looking for.
    u_short sender_protocol_address[2];  // Internetwork address of the sender.
    u_short
        target_hardware_address[3];  // Media address of the intended receiver.
                                     // In an ARP request this field is ignored.
                                     // In an ARP reply this field is used to
                                     // indicate the address of the host that
                                     // originated the ARP request.
    u_short target_protocol_address[2];  // Internetwork address of the intended
                                         // receiver.
} __attribute__((packed));

// 协议号枚举
#define PROTOCOL_ID                          \
    PROTOCOL(HOPOPT, "HOPOPT")               \
    PROTOCOL(ICMP, "ICMP")                   \
    PROTOCOL(IGMP, "IGMP")                   \
    PROTOCOL(GGP, "GGP")                     \
    PROTOCOL(IPv4, "IPv4")                   \
    PROTOCOL(ST, "ST")                       \
    PROTOCOL(TCP, "TCP")                     \
    PROTOCOL(CBT, "CBT")                     \
    PROTOCOL(EGP, "EGP")                     \
    PROTOCOL(IGP, "IGP")                     \
    PROTOCOL(BBN_RCC_MON, "BBN_RCC_MON")     \
    PROTOCOL(NVP_II, "NVP_II")               \
    PROTOCOL(PUP_Xerox_PUP, "PUP_Xerox_PUP") \
    PROTOCOL(ARGUS, "ARGUS")                 \
    PROTOCOL(EMCON, "EMCON")                 \
    PROTOCOL(XNET, "XNET")                   \
    PROTOCOL(CHAOS, "CHAOS")                 \
    PROTOCOL(UDP, "UDP")                     \
    PROTOCOL(MUX, "MUX")                     \
    PROTOCOL(DCN_MEAS, "DCN-MEAS")           \
    PROTOCOL(HMP_Host, "HMP_Host")           \
    PROTOCOL(PRM_Packet, "PRM_Packet")       \
    PROTOCOL(XNS_IDP, "XNS-IDP")             \
    PROTOCOL(TRUNK_1, "TRUNK_1")             \
    PROTOCOL(TRUNK_2, "TRUNK_2")             \
    PROTOCOL(LEAF_1, "LEAF-1")               \
    PROTOCOL(LEAF_2, "LEAF-2")               \
    PROTOCOL(RDP, "RDP")                     \
    PROTOCOL(IRTP, "IRTP")                   \
    PROTOCOL(ISO_TP4, "ISO-TP4")             \
    PROTOCOL(NETBLT, "NETBLT")               \
    PROTOCOL(MFE_NSP, "MFE-NSP")             \
    PROTOCOL(MERIT_INP, "MERIT-INP")

enum protocol_id {
#define PROTOCOL(enum, str) enum,
    PROTOCOL_ID
#undef PROTOCOL
};

// `EIPv4` 是因为命名冲突所以加上了E
#define ETHERTYPE_ID                  \
    ETHERTYPE(EIPv4, 0x0800, "EIPv4") \
    ETHERTYPE(ARP, 0x0806, "ARP")     \
    ETHERTYPE(IPv6, 0x86DD, "IPv6")   \
    ETHERTYPE(RARP, 0x8035, "RARP")

enum ethertype_id {
#define ETHERTYPE(enum, id, str) enum = id,
    ETHERTYPE_ID
#undef ETHERTYPE
};

template <typename... Params>
inline std::string format_string(const char *fmt, Params &&... params) {
    int len = std::snprintf(nullptr, 0, fmt, std::forward<Params>(params)...);
    std::string buffer(len, 0);
    std::sprintf(buffer.data(), fmt, std::forward<Params>(params)...);
    return buffer;
}

// http://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html
// if the variable arguments are omitted or empty, the `##' operator causes the
// preprocessor to remove the comma before it.

#define SNIFFLE_FMT_THROW(fmt, ...) \
    (throw std::runtime_error(format_string(fmt, ##__VA_ARGS__)))

#define SNIFFLE_CHECK_AND_THROW(expr, fmt, ...)                     \
    do {                                                            \
        if (!(expr))                                                \
            SNIFFLE_FMT_THROW("\n%s, %d: " fmt, __FILE__, __LINE__, \
                              ##__VA_ARGS__);                       \
    } while (0)

#define SNIFFLE_HEXDUMP(ptr, size)                              \
    do {                                                        \
        aout.set_attr(p_attr::a_RED)                            \
            .std_err()                                          \
            .printf("\n%s\n", get_hex_data(ptr, size).c_str()); \
    } while (0)
extern std::string err_buffer;

template <
    typename API, typename... Args,
    typename std::enable_if_t<
        std::is_pointer_v<std::invoke_result_t<API &&, Args &&...>>, int> = 0>
std::invoke_result_t<API &&, Args &&...> _do_call_npcap(API &&api, int lineno,
                                                        const char *func,
                                                        const char *filename,
                                                        Args &&... args) {
    err_buffer.clear();
    auto res = (std::forward<API>(api))(std::forward<Args>(args)...);
    SNIFFLE_CHECK_AND_THROW(res, "`%s` failed with nullptr, called at %s: %d",
                            func, filename, lineno);
    return res;
}
template <
    typename API, typename... Args,
    typename std::enable_if_t<
        std::is_integral_v<std::invoke_result_t<API &&, Args &&...>>, int> = 0>
std::invoke_result_t<API &&, Args &&...> _do_call_npcap(API &&api, int lineno,
                                                        const char *func,
                                                        const char *filename,
                                                        Args &&... args) {
    err_buffer.clear();
    auto res = (std::forward<API>(api))(std::forward<Args>(args)...);
    SNIFFLE_CHECK_AND_THROW(res >= 0, "`%s` failed with %d, called at %s: %d",
                            func, res, filename, lineno);
    return res;
}
#define do_call_npcap(api, ...) \
    _do_call_npcap(api, __LINE__, __func__, __FILE__, ##__VA_ARGS__)

std::string do_get_timestamp(const pcap_pkthdr *);
std::string get_hex_data(const u_char *ptr, size_t len);
std::string do_format_IPv6(const u_char *);
std::string do_format_IP(const v6_address *);
std::string do_format_IP(const ip_address *);
std::string do_format_IP(const ip_address *, u_short);
std::string do_format_MAC(u_char, u_char, u_char, u_char, u_char, u_char);
const char *iptos(u_long in);
const char *ip6tos(sockaddr *, char *, int);
#endif