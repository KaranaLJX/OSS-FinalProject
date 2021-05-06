#include <bits/stdc++.h>

#include "color.h"
#include "mINI.hh"
#include "packet_parser.hh"
#include "pcap.h"
#include "printer.hh"
#include "util.hh"

#ifdef WIN32
#include "win32_port.hh"
#endif
std::string err_buffer(1024, 0);
// filter 规则
std::string filter;
void my_packet_handler(u_char *, const pcap_pkthdr *, const u_char *);
#ifdef USE_JSON
void print_interface(const pcap_if_t *, json::Value &, json::Document &);
#else
void print_interface(
    const pcap_if_t *u);  // 最后一个参数用来传一个json对象；懒得写重载了
#endif
void loop_proc(pcap_t *);
void on_signal(int);
void read_config();
#ifdef WIN32
BOOL WINAPI on_signal_handler(DWORD dwCtrlType);
#endif

// atomic 标记；用于控制处理线程是否终止
std::atomic<bool> signal_stop;

int main(int argc, const char **argv) {
    do_set_exception_filter();
    read_config();
    // std::signal(SIGINT, on_signal);
#ifdef WIN32

    SetConsoleCtrlHandler(on_signal_handler, true);
#endif
    aout.open("sniffle.log");
    // 关闭 stdout 和 stderr 的缓冲
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    try {
        // json root
        pcap_if_t *all_devs;
        static auto free = [](pcap_if_t *ptr) { pcap_freealldevs(ptr); };
        static auto pcap_free = [](pcap_t *ptr) { pcap_close(ptr); };
        do_call_npcap(pcap_findalldevs_ex, PCAP_SRC_IF_STRING, nullptr,
                      &all_devs, err_buffer.data());
        std::unique_ptr<pcap_if_t, decltype(free)> devs_guard(all_devs, free);
        std::vector<pcap_if_t *> vec_if;

// json val
#ifdef USE_JSON
        json::Document doc;
        doc.SetObject();
        json::Value devices_list(json::Type::kArrayType);
#endif
        for (pcap_if_t *u = all_devs; u; u = u->next) vec_if.push_back(u);

        for (size_t i = 0; i < vec_if.size(); ++i) {
#ifdef USE_JSON
            json::Value dev(json::Type::kObjectType);

            dev.AddMember("id", json::Value(i), doc.GetAllocator());

            print_interface(vec_if[i], dev, doc);

            devices_list.PushBack(dev, doc.GetAllocator());
#else
            aout.std_err().set_attr(p_attr::a_GRN).printf("[%llu] ", i);
            print_interface(vec_if[i]);
#endif
        }

#ifdef USE_JSON

        doc.AddMember("devices_list", devices_list, doc.GetAllocator());

        json::StringBuffer sb;
        json::Writer writer(sb);
        // json::Writer writer(sb);
        doc.Accept(writer);

        // aout.std_err().printf("%s", sb.GetString());
        std::cerr << sb.GetString() << std::endl;

#endif
        int i_choose = 0;
        SNIFFLE_CHECK_AND_THROW(
            vec_if.size() > 0,
            "vec_if.size() == 0, failed to get lists of the devices!");
        do {
#ifndef USE_JSON
            aout.std_err().printf("\n");
            aout.std_err()
                .set_attr(p_attr::a_MAG)
                .printf("Type a number to choose (0 - %llu) >> ",
                        vec_if.size() - 1);
#endif
            std::cin >> i_choose;
            // scanf("%d", &i_choose);
        } while (i_choose < 0 || i_choose > static_cast<int>(vec_if.size()));

        pcap_if_t *choose = vec_if[i_choose];
        // aout.clear();
#ifndef USE_JSON
        aout.std_err()
            .set_attr(p_attr::a_GRN)
            .printf("[Info] Listening on `%s`...\n", choose->description);
#endif
        pcap_t *ad_handle =
            do_call_npcap(pcap_open,
                          choose->name,  // 设备名
                          65536,         // 端口号；这里为全部
                          PCAP_OPENFLAG_PROMISCUOUS,  // 混杂模式
                          1000,                       // 超时时间(ms)
                          nullptr,                    // 验证
                          err_buffer.data());         // 错误信息

        std::unique_ptr<pcap_t, decltype(pcap_free)> ad_handle_guard(ad_handle,
                                                                     pcap_free);

        // 检查链路层协议
        // https://blog.csdn.net/shenwansangz/article/details/47612505
        int link_type = do_call_npcap(pcap_datalink, ad_handle);
        // 以太网和 802.11 局域网
        SNIFFLE_CHECK_AND_THROW(
            link_type == DLT_EN10MB || link_type == DLT_IEEE802_11,
            "Unsupported network type!");

        u_int net_mask = 0xffffff;
        if (choose->addresses) {
            net_mask =
                (reinterpret_cast<sockaddr_in *>(choose->addresses->netmask))
                    ->sin_addr.S_un.S_addr;
        }
        char c;
        while (std::cin.get(c) && c != '^') {
            // 从 stdin 读取过滤器
            std::getline(std::cin, filter);
            if (c != '\n') filter.insert(filter.begin(), c);

            // 设置过滤器
            bpf_program fcode;
            // 字串格式: /npcap/docs/wpcap/pcap-filter.html
            // 测试抓 TCP 包
            // ip or udp or tcp
            auto packet_filter = filter;
#ifndef USE_JSON
            aout.std_err()
                .set_attr(p_attr::a_YEL)
                .printf("[Info] Filter: %s\n", packet_filter.c_str());
#endif
            do_call_npcap(pcap_compile, ad_handle, &fcode,
                          packet_filter.c_str(), 1, net_mask);
            do_call_npcap(pcap_setfilter, ad_handle, &fcode);

            // 启动处理线程
            // 设置标记; release-consume
            signal_stop.store(0, std::memory_order::memory_order_release);
            std::thread proc_thread(loop_proc, ad_handle);
            proc_thread.join();

            // 设置处理函数
            // do_call_npcap(pcap_loop, ad_handle, 0, my_packet_handler,
            // nullptr);
        }

    } catch (std::runtime_error &e) {
        const char *err_api = err_buffer.size() ? err_buffer.c_str() : "(none)";
        const char *exp_err = (*e.what()) ? e.what() : "(none)";
#ifndef USE_JSON
        aout.std_err()
            .set_attr(p_attr::a_BOLD, p_attr::a_BG_LRED, p_attr::a_WHT)
            .printf("[Fatal] Last API call error: %s, exception: %s\n", err_api,
                    exp_err);
#endif
        exit(-1);
    }
    return 0;
}
void loop_proc(pcap_t *ad_handle) {
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    while (1) {
        if (signal_stop.load(std::memory_order_consume)) {
            // 被终止
            aout.set_attr(p_attr::a_RED)
                .printf(
                    "\nPAUSE.\nInput another filter rule to continue or Press "
                    "^C again to exit.\n");
            break;
        }
        res = do_call_npcap(pcap_next_ex, ad_handle, &header, &pkt_data);
        if (!res) {
            // 超时
            continue;
        }
        my_packet_handler(nullptr, header, pkt_data);
    }
}
void my_packet_handler(u_char *param, const pcap_pkthdr *header,
                       const u_char *pkt_data) {
    // 测试解析 TCP
    // 以太网头： https://en.wikipedia.org/wiki/Ethernet_frame
    // 它抓到的 pkt_data 自动跳过了 preamble ，所以 MAC 帧头是14

    aout.std_out();

#ifdef USE_JSON
    json::Document doc;
    doc.SetObject();

    json::Value Timestamp, Type, Raw;
#endif

    //  lambda

    auto do_handle_transport = [&](std::string &data_type, const u_short t_len,
                                   const u_short ih_len, const void *ih,
                                   std::string &src_addr,
                                   std::string &dest_addr) {
        uint16_t sport, dport;

#ifdef USE_JSON
        Raw.SetString(
            get_hex_data(reinterpret_cast<const u_char *>(ih), t_len).c_str(),
            doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_BG_MAG)
            .printf("\n%s\n",
                    get_hex_data(reinterpret_cast<const u_char *>(ih), t_len)
                        .c_str());
#endif
        // 不优雅
        const udp_header *udp = nullptr;
        const tcp_header *tcp = nullptr;
        if (data_type == "UDP") {
            udp = reinterpret_cast<const udp_header *>(
                reinterpret_cast<const u_char *>(ih) + ih_len);
            sport = ntohs(udp->sport), dport = ntohs(udp->dport);
            // try_parse_DNS
            if (dport == 53 || sport == 53) data_type = "DNS";
            if (sport == 8000 || dport == 8000) {
                // 检查是否是 QQ 的包
                const u_char *header =
                    reinterpret_cast<const u_char *>(udp + 1);
                const u_short *option =
                    reinterpret_cast<const u_short *>(header + 3);
                u_short opt = ntohs(*option);

                if (*header == 0x02 && oicq_opt.count(opt)) {
                    data_type = "QQ";
                }
            }

        } else if (data_type == "TCP") {
            tcp = reinterpret_cast<const tcp_header *>(
                reinterpret_cast<const u_char *>(ih) + ih_len);
            sport = ntohs(tcp->s_port), dport = ntohs(tcp->d_port);
        }

        src_addr += ":" + std::to_string(sport),
            dest_addr += ":" + std::to_string(dport);

#ifdef USE_JSON
        Timestamp.SetString(do_get_timestamp(header).c_str(),
                            doc.GetAllocator());
        Type.SetString(data_type.c_str(), doc.GetAllocator());
        json::Value src, dest;

        // src.SetString(do_format_IP(&ih->saddr, sport).c_str(),
        // doc.GetAllocator()); dest.SetString(do_format_IP(&ih->daddr,
        // dport).c_str(), doc.GetAllocator());
        src.SetString(src_addr.c_str(), doc.GetAllocator());
        dest.SetString(dest_addr.c_str(), doc.GetAllocator());

        doc.AddMember("Timestamp", Timestamp, doc.GetAllocator());
        doc.AddMember("Type", Type, doc.GetAllocator());
        doc.AddMember("Src", src, doc.GetAllocator());
        doc.AddMember("Dest", dest, doc.GetAllocator());
        doc.AddMember("Raw", Raw, doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_GRN)
            .printf("[%s]", do_get_timestamp(header).c_str());
        aout.set_attr(p_attr::a_RED).printf("[%s]", data_type.c_str());

        aout.set_attr(p_attr::a_MAG)
            .printf("[%s -> %s] ", src_addr.c_str(), dest_addr.c_str());
#endif
        if (data_type == "QQ") {
#ifdef USE_JSON
            do_expand_QQ_packet(reinterpret_cast<const u_char *>(udp + 1), doc);
#else
            do_expand_QQ_packet(reinterpret_cast<const u_char *>(udp + 1));
#endif
        } else if (data_type == "DNS") {
#ifdef USE_JSON
            do_parse_DNS_header(reinterpret_cast<const dns *>(udp + 1), doc);
#else
            do_parse_DNS_header(reinterpret_cast<const dns *>(udp + 1));
#endif
        } else if (data_type == "TCP") {
#ifdef USE_JSON
            do_expand_TCP_segment(tcp, doc);
#else
            do_expand_TCP_segment(tcp);
#endif
        } else {
#ifdef USE_JSON
            json::Value msg;
            msg.SetString(format_string("Length: %3d", header->len).c_str(),
                          doc.GetAllocator());
            doc.AddMember("Msg", msg, doc.GetAllocator());
#else
            aout.set_attr(p_attr::a_YEL).printf("Length: %3d\n", header->len);
#endif
        }
    };

    const u_short ether_type =
        ntohs(*reinterpret_cast<const u_short *>(pkt_data + 12));

    auto ether_type_str = do_recognize_ether_frame(ether_type);
    if (ether_type_str == "ARP") {
        std::string _timestamp = do_get_timestamp(header);
        const char *timestamp = _timestamp.c_str();

#ifdef USE_JSON
        Timestamp.SetString(timestamp, doc.GetAllocator());
        Type.SetString(ether_type_str.c_str(), doc.GetAllocator());

        doc.AddMember("Timestamp", Timestamp, doc.GetAllocator());
        doc.AddMember("Type", Type, doc.GetAllocator());
        do_expand_ARP_frame(reinterpret_cast<const ARP_packet *>(pkt_data + 14),
                            doc);
        Raw.SetString(
            get_hex_data(reinterpret_cast<const u_char *>(pkt_data + 14),
                         sizeof(ARP_packet))
                .c_str(),
            doc.GetAllocator());
        doc.AddMember("Raw", Raw, doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_GRN).printf("[%s]", timestamp);
        aout.set_attr(p_attr::a_RED).printf("[%s]", ether_type_str.c_str());
        do_expand_ARP_frame(
            reinterpret_cast<const ARP_packet *>(pkt_data + 14));
#endif

    } else if (ether_type_str == "EIPv4") {
        const ip_header *ih =
            reinterpret_cast<const ip_header *>(pkt_data + 14);
        // 头部长度
        u_int ih_len = ((ih->ver_ihl & 0xf) << 2);

        std::string data_type = do_recognize_packet(ih->proto);

        std::string src = do_format_IP(&ih->saddr),
                    dest = do_format_IP(&ih->daddr);

        do_handle_transport(data_type, ntohs(ih->tlen), ih_len, ih, src, dest);

    } else if (ether_type_str == "IPv6") {
        const v6_header *ih =
            reinterpret_cast<const v6_header *>(pkt_data + 14);
        // 头部长度
        u_int ih_len = sizeof(v6_header);
        std::string data_type = do_recognize_packet(ih->next_header);

        std::string src = do_format_IP(&ih->source_addr),
                    dest = do_format_IP(&ih->dest_addr);
        do_handle_transport(data_type, ntohs(ih->payload_len) + ih_len, ih_len,
                            ih, src, dest);
    }
#ifdef USE_JSON
    json::StringBuffer sb;
    json::Writer writer(sb);
    doc.Accept(writer);

    std::cout << sb.GetString() << std::endl;

    // aout.std_out().printf("%s", sb.GetString());
    // fprintf(stdout, "%s\n", sb.GetString());
    // fflush(stdout);
#endif
}

void on_signal(int sig) {
    if (sig == SIGINT) {
        // 如果正在捕获 (signal_stop == 0) 就暂停...
        if (!signal_stop.load(std::memory_order_consume)) {
            signal_stop.store(1, std::memory_order_release);
            return;
        }
        aout.set_attr(p_attr::a_RED).printf("\nSTOP.\n");
        exit(0);
    }
}
#ifdef WIN32
BOOL WINAPI on_signal_handler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        std::cout.flush();
        on_signal(SIGINT);
    }
    return TRUE;
}
#endif
void print_interface(const pcap_if_t *u
#ifdef USE_JSON
                     ,
                     json::Value &obj, json::Document &doc
#endif
) {
    char ip6str[128];

#ifdef USE_JSON
    obj.AddMember("device_name", json::Value(u->name, strlen(u->name)),
                  doc.GetAllocator());
#else
    aout.std_err().set_attr(p_attr::a_GRN).printf("%s", u->name);
#endif

    if (u->description) {
#ifdef USE_JSON
        obj.AddMember("description",
                      json::Value(u->description, strlen(u->description)),
                      doc.GetAllocator());
#else
        aout.std_err().set_attr(p_attr::a_YEL).printf(" (%s)", u->description);
#endif
    }
#ifndef USE_JSON
    aout.std_err().printf("\n");
#endif
#ifdef USE_JSON
    obj.AddMember("loopback", json::Value(bool(u->flags & PCAP_IF_LOOPBACK)),
                  doc.GetAllocator());
    json::Value details(json::Type::kArrayType);
#else
    aout.std_err()
        .set_attr(p_attr::a_RED)
        .printf("\tLoopback: %s\n",
                (u->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");
#endif

    for (pcap_addr_t *addr = u->addresses; addr; addr = addr->next) {
#ifdef USE_JSON
        json::Value detail_tmp(json::Type::kObjectType);
#endif
        switch (addr->addr->sa_family) {
            case AF_INET:
#ifdef USE_JSON
                detail_tmp.AddMember(
                    "address_family_name",
                    json::Value("AF_INET", sizeof("AF_INET") - 1),
                    doc.GetAllocator());
#else
                aout.std_err()
                    .set_attr(p_attr::a_YEL)
                    .printf("\tAddress Family Name: AF_INET\n");
#endif

                if (addr->addr) {
                    const char *ip_addr = iptos(
                        ((struct sockaddr_in *)addr->addr)->sin_addr.s_addr);
#ifdef USE_JSON
                    detail_tmp.AddMember("address",
                                         json::Value(ip_addr, strlen(ip_addr)),
                                         doc.GetAllocator());
#else
                    aout.std_err()
                        .set_attr(p_attr::a_YEL)
                        .printf("\tAddress: %s\n", ip_addr);
#endif
                }
                if (addr->netmask) {
                    const char *mask = iptos(
                        ((struct sockaddr_in *)addr->netmask)->sin_addr.s_addr);
#ifdef USE_JSON
                    detail_tmp.AddMember("netmask",
                                         json::Value(mask, strlen(mask)),
                                         doc.GetAllocator());
#else
                    aout.std_err()
                        .set_attr(p_attr::a_YEL)
                        .printf("\tNetmask: %s\n", mask);
#endif
                }
                if (addr->broadaddr) {
                    const char *broadcast =
                        iptos(((struct sockaddr_in *)addr->broadaddr)
                                  ->sin_addr.s_addr);
#ifdef USE_JSON
                    detail_tmp.AddMember(
                        "broadcast_address",
                        json::Value(broadcast, strlen(broadcast)),
                        doc.GetAllocator());
#else
                    aout.std_err()
                        .set_attr(p_attr::a_YEL)
                        .printf("\tBroadcast Address: %s\n", broadcast);
#endif
                }
                if (addr->dstaddr) {
                    const char *dest = iptos(
                        ((struct sockaddr_in *)addr->dstaddr)->sin_addr.s_addr);
#ifdef USE_JSON
                    detail_tmp.AddMember("destination",
                                         json::Value(dest, strlen(dest)),
                                         doc.GetAllocator());
#else
                    aout.std_err()
                        .set_attr(p_attr::a_YEL)
                        .printf("\tDestination Address: %s\n", dest);
#endif
                }
                break;

            case AF_INET6:
#ifdef USE_JSON
                detail_tmp.AddMember(
                    "address_family_name",
                    json::Value("AF_INET6", sizeof("AF_INET6") - 1),
                    doc.GetAllocator());
#else
                aout.std_err()
                    .set_attr(p_attr::a_CYN)
                    .printf("\tAddress Family Name: AF_INET6\n");
#endif
                if (addr->addr) {
                    const char *v6_addr =
                        ip6tos(addr->addr, ip6str, sizeof(ip6str));
#ifdef USE_JSON
                    detail_tmp.AddMember("address",
                                         json::Value(v6_addr, strlen(v6_addr)),
                                         doc.GetAllocator());
#else
                    aout.std_err()
                        .set_attr(p_attr::a_MAG)
                        .printf("\tAddress: %s\n", v6_addr);
#endif
                }
                break;

            default:

#ifdef USE_JSON
                detail_tmp.AddMember("address_family_name",
                                     json::Value("unknown", strlen("unknown")),
                                     doc.GetAllocator());
#else
                aout.std_err()
                    .set_attr(p_attr::a_RED)
                    .printf("\tAddress Family Name: Unknown\n");
#endif
                break;
        }
#ifdef USE_JSON
        details.PushBack(detail_tmp, doc.GetAllocator());
#endif
    }

#ifdef USE_JSON
    obj.AddMember("details", details, doc.GetAllocator());
#endif
}

void read_config() {
    mINI::INIFile config_file("sniffle_config.ini");
    mINI::INIStructure config_ini;
    config_file.read(config_ini);
    filter = config_ini["sniffle"]["packet_filter"];
    std::string &op = config_ini["sniffle"]["pretty_print"];
    std::string &copy_file = config_ini["sniffle"]["copy_to_file"];

    if (!filter.size()) {
        filter = "\"ip or udp or tcp\"";
        config_file.write(config_ini, true);
    }
    if (!op.size()) {
        op = "1";
        config_file.write(config_ini, true);
    }
    if (!copy_file.size()) {
        copy_file = "0";
        config_file.write(config_ini, true);
    }
    filter.pop_back(), filter.erase(filter.begin());
    if (op == "1") {
        aout << sniffle::enable_pretiffy;
    } else
        aout << sniffle::disable_pretiffy;

    if (copy_file == "1") aout.enable_copy_to_file();
}