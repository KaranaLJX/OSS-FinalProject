#include "../Include/packet_parser.hh"

#include "../Include/printer.hh"
#include "../Include/util.hh"

const char *do_format_ip_header(const ip_header *ih) {
    static char packet_buffer[2048];
    memset(packet_buffer, 0, sizeof packet_buffer);
    /*
                                    IPv4 Header
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |  Ver  |  IHL  |     Service   |       Total length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    Identification             |     flags_fo(3 + 13offsets)   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |     TTL       |    Proto      |        CRC                    |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                             Source                            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                           Destination                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                            Option+padding                     |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    */
    sprintf(packet_buffer, R"(
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  %3d  |  %3d  |     %7u   |       %12d            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    %14d             |        %9d              |                                                 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     %3u       |    %5u      |  %21d        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      %3d.%3d.%3d.%3d                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      %3d.%3d.%3d.%3d                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Option+padding                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
)",
            ih->ver_ihl & (0xf0), (ih->ver_ihl & 0xf) << 2, ih->tos, ih->tlen,
            ih->identification, ih->flags_fo, ih->ttl, ih->proto, ih->crc,
            ih->saddr.byte1, ih->saddr.byte2, ih->saddr.byte3, ih->saddr.byte4,
            ih->daddr.byte1, ih->daddr.byte2, ih->daddr.byte3, ih->daddr.byte4);
    return packet_buffer;
}

void do_dump_packet(const u_char *pkt_data, int len) {
    SNIFFLE_HEXDUMP(pkt_data, len);
}

bool do_parse_DNS_header(const dns *dns_header
#ifdef USE_JSON
                         ,
                         json::Document &doc
#endif
) {

#ifdef USE_JSON
    json::Value Msg;
    std::string _msg;
#endif
    // check QR
    // 0 for query, 1 for response
    auto dns_query = reinterpret_cast<const u_char *>(dns_header + 1);
    /*
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    / QNAME                                         /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    | QTYPE                                         |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    | QCLASS                                        |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */
    static u_char qname[1024];

    // 记录回答的左端点和右端点. 注意字符串可能出现在 Query 部分的字符串里面
    std::vector<std::pair<const u_char *, const u_char *>> vec_answers;

    vec_answers.push_back({dns_query, nullptr});
    memset(qname, 0, sizeof qname);
    size_t tot = 0;
    while (*dns_query) {
        u_char len = *dns_query++;
        memcpy(qname + tot, dns_query, len);
        dns_query += len, tot += len;
        qname[tot++] = '.';
    }

    vec_answers.back().second = dns_query;

    qname[--tot] = '\0';
    ++dns_query;
    u_short qtype = *reinterpret_cast<const u_short *>(dns_query);
    dns_query += 2;
    u_short qclass = *reinterpret_cast<const u_short *>(dns_query);

    if (!(dns_header->flags & (1 << 15))) {
#ifdef USE_JSON
        _msg += format_string("Looking up %s", qname);
        Msg.SetString(_msg.c_str(), doc.GetAllocator());
        doc.AddMember("Msg", Msg, doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_WHT).printf("Looking up %s\n", qname);
#endif
    } else {
        /*
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                                               |
        /                                               /
        /                     NAME                      /
        |                                               |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | TYPE                                          |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | CLASS                                         |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | TTL                                           |
        |                                               |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | RDLENGTH                                      |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
        / RDATA                                         /
        /                                               /
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        */
        auto dns_answer = dns_query + 2;
        memset(qname, 0, sizeof qname);
        /*
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | 1 1 |               OFFSET                    |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        */
        // 处理头部的指针, 注意大小端
        auto dns_answer_data = dns_answer;
        const u_short ans_tot = htons(dns_header->an_count);

        for (int i = 1, tot = 0; i <= ans_tot; ++i, tot = 0) {
            u_short pointer =
                htons(*reinterpret_cast<const u_short *>(dns_answer_data));

            SNIFFLE_CHECK_AND_THROW(
                (pointer >> 14) == 3,
                "Failed to parse DNS response: pointer expected.");

            u_short offset = pointer & ((1 << 14) - 1);
            dns_answer_data += 2;
            u_short type =
                htons(*reinterpret_cast<const u_short *>(dns_answer_data));
            dns_answer_data += 2;
            u_short Class =
                htons(*reinterpret_cast<const u_short *>(dns_answer_data));
            dns_answer_data += 2;
            uint32_t ttl =
                htons(*reinterpret_cast<const uint32_t *>(dns_answer_data));
            dns_answer_data += 4;
            u_short data_length =
                htons(*reinterpret_cast<const u_short *>(dns_answer_data));
            dns_answer_data += 2;
            u_char response[1024] = {0};

            vec_answers.push_back(
                {dns_answer_data, dns_answer_data + data_length});

            // 记录当前的右边界
            const u_char *right_ptr = vec_answers.back().second;

            if (type == 0x0001 || type == 0x001c) {
                // 0x001c: RFC3596, AAAA for IPv6
                // ARecord, expecting IP address
                SNIFFLE_CHECK_AND_THROW(
                    !(data_length % 4),
                    "DNS Response parse failed: expecting "
                    "IP Address, but response is not, data_length is %d",
                    data_length);
                bool is_v6 = type == 0x001c;
                if (is_v6) {
                    for (int i = 0; i < 16; ++i)
                        response[tot++] = *dns_answer_data++;
                    response[tot] = '\0';
                    data_length -= 16;
                    std::string info =
                        "ARecord: " + do_format_IPv6(response) + "; ";
#ifdef USE_JSON
                    _msg += info;
#else
                    aout.set_attr(p_attr::a_CYN).printf(info.c_str());
#endif
                } else {
                    while (data_length >= 4) {
                        response[tot++] = *dns_answer_data++;
                        response[tot++] = *dns_answer_data++;
                        response[tot++] = *dns_answer_data++;
                        response[tot++] = *dns_answer_data++;
                        response[tot] = '\0';
                        data_length -= 4;

#ifdef USE_JSON
                        _msg += format_string("ARecord: %-3d.%-3d.%-3d.%-3d;",
                                              response[0], response[1],
                                              response[2], response[3]);
#else
                        aout.set_attr(p_attr::a_CYN)
                            .printf("ARecord: %-3d.%-3d.%-3d.%-3d; ",
                                    response[0], response[1], response[2],
                                    response[3]);
#endif
                    }
                }
            } else if (type == 0x0005) {
                // 0x0005: CNAME
                tot = 0;
                auto cur_ptr = dns_answer_data;
                // 某一个回答的长度
                std::stack<std::pair<const u_char *, const u_char *>> stack;

                while (1) {
                    // 这个回答结束了
                    if (!(*cur_ptr) || cur_ptr >= right_ptr) {
                        while (stack.size() && cur_ptr >= right_ptr) {
                            cur_ptr = stack.top().first,
                            right_ptr = stack.top().second;
                            stack.pop();
                        }
                        if (stack.empty()) break;
                    }
                    // 结尾回溯的时候会出问题：遇到了下一个答案的开头也是
                    // 0xc0 0xab
                    u_short offset = cur_ptr[0] * 256 + cur_ptr[1];
                    if (offset >> 14 == 3) {
                        // 遇到压缩的部分，跳转指针至开头部分
                        offset = (offset) & ((1 << 14) - 1);

                        SNIFFLE_CHECK_AND_THROW(
                            cur_ptr + 2 <= right_ptr,
                            "DNS parsing: push stack checker failed. "
                            "cur_ptr is %08x, right_ptr is %08x",
                            cur_ptr, right_ptr);
                        stack.push({cur_ptr + 2, right_ptr});

                        cur_ptr = reinterpret_cast<const u_char *>(dns_header) +
                                  offset;
                        right_ptr = [&] {
                            int l = 0, r = vec_answers.size() - 1;
                            while (l < r) {
                                int mid = (l + r + 1) >> 1;
                                if (cur_ptr >= vec_answers[mid].first)
                                    l = mid;
                                else
                                    r = mid - 1;
                            }
                            SNIFFLE_CHECK_AND_THROW(
                                l == (int)vec_answers.size() - 1 ||
                                    (vec_answers[l].first <= cur_ptr &&
                                     vec_answers[l + 1].first > cur_ptr),
                                "DNS parsing: jump pointer failed! "
                                "cur_ptr: %08x, pre_ptr: %08x",
                                cur_ptr, vec_answers[l].first);

                            return vec_answers[l].second;
                        }();
                        continue;
                    }
                    u_char len = *cur_ptr++;

                    memcpy(response + tot, cur_ptr, len);
                    cur_ptr += len, tot += len;

                    response[tot++] = '.';
                }
                response[--tot] = '\0';
                dns_answer_data = cur_ptr;
                if (!*(dns_answer_data)) ++dns_answer_data;
#ifdef USE_JSON
                _msg += format_string("CNAME: %s; ", response);
#else
                aout.set_attr(p_attr::a_CYN).printf("CNAME: %s; ", response);
#endif
            } else if (type == 0x0002) {
                // Name Server alias
                SNIFFLE_CHECK_AND_THROW(false,
                                        "DNS Response for Name Server is "
                                        "still under construction");
            } else if (type == 0x000f) {
                // Mail Server
                SNIFFLE_CHECK_AND_THROW(false,
                                        "DNS Response for Mail Server is "
                                        "still under construction");
            }
        }
        if (!ans_tot) {
#ifdef USE_JSON
            _msg += format_string("Server: No comment!");
#else
            aout.set_attr(p_attr::a_RED).printf("Server: No comment!");
#endif
        }
#ifndef USE_JSON
        aout.printf("\n");
#else
        Msg.SetString(_msg.c_str(), doc.GetAllocator());
        doc.AddMember("Msg", Msg, doc.GetAllocator());
#endif
    }
    return 1;
}
std::string do_recognize_packet(u_char ih_proto) {
    switch (ih_proto) {
#define PROTOCOL(Enum, String) \
    case protocol_id::Enum:    \
        return String;
        PROTOCOL_ID
#undef PROTOCOL
        default:
            return "UNKNOWN";
    }
}
std::string do_recognize_ether_frame(u_short type) {
    switch (type) {
#define ETHERTYPE(enum, id, str) \
    case ethertype_id::enum:     \
        return str;
        ETHERTYPE_ID
#undef ETHERTYPE
        default:
            SNIFFLE_CHECK_AND_THROW(false, "Ether frame unknown: %d", type);
            return "UNKNOWN";
    }
}
void do_expand_ARP_frame(const ARP_packet *arp
#ifdef USE_JSON
                         ,
                         json::Document &doc
#endif
) {
    std::string buffer(1024, 0);
    // 注意大小端
    std::string sender_MAC =
        do_format_MAC(arp->sender_hardware_address[0] & 0xff,
                      (arp->sender_hardware_address[0] & 0xff00) >> 8,
                      arp->sender_hardware_address[1] & 0xff,
                      (arp->sender_hardware_address[1] & 0xff00) >> 8,
                      arp->sender_hardware_address[2] & 0xff,
                      (arp->sender_hardware_address[2] & 0xff00) >> 8);
    std::string target_MAC =
        do_format_MAC(arp->target_hardware_address[0] & 0xff,
                      (arp->target_hardware_address[0] & 0xff00) >> 8,
                      arp->target_hardware_address[1] & 0xff,
                      (arp->target_hardware_address[1] & 0xff00) >> 8,
                      arp->target_hardware_address[2] & 0xff,
                      (arp->target_hardware_address[2] & 0xff00) >> 8);

#ifdef USE_JSON
    json::Value src, dest;
    src.SetString(sender_MAC.c_str(), doc.GetAllocator());
    dest.SetString(target_MAC.c_str(), doc.GetAllocator());
    doc.AddMember("Src", src, doc.GetAllocator());
    doc.AddMember("Dest", dest, doc.GetAllocator());

#else
    aout.set_attr(p_attr::a_MAG)
        .printf("[%s -> %s]", sender_MAC.c_str(), target_MAC.c_str());
#endif
    ip_address sender_protocol_address =

                   {u_char(arp->sender_protocol_address[0] &
                           0xff), /* -Wnarrowing */
                    u_char((arp->sender_protocol_address[0] & 0xff00) >> 8),
                    u_char(arp->sender_protocol_address[1] & 0xff),
                    u_char((arp->sender_protocol_address[1] & 0xff00) >> 8)},

               target_protocol_address = {
                   u_char(arp->target_protocol_address[0] & 0xff),
                   u_char((arp->target_protocol_address[0] & 0xff00) >> 8),
                   u_char(arp->target_protocol_address[1] & 0xff),
                   u_char((arp->target_protocol_address[1] & 0xff00) >> 8)};

    std::string target, sender;
    using namespace std::string_literals;
    if (ntohs(arp->operation) == 1) {
        target = do_format_IP(&target_protocol_address);
        sender = do_format_IP(&sender_protocol_address);
#ifdef USE_JSON
        json::Value msg;

        msg.SetString(
            ("ARP Request. Who has "s + target + "? Tell " + sender).c_str(),
            doc.GetAllocator());
        doc.AddMember("Msg", msg, doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_YEL)
            .printf(" ARP Request.")
            .set_attr(p_attr::a_GRN)
            .printf(" Who has %s ? Tell %s\n", target.c_str(), sender.c_str());
#endif
    } else {
        sender = do_format_IP(&sender_protocol_address).c_str();
#ifdef USE_JSON
        json::Value msg;

        msg.SetString(
            ("ARP Reply. "s + sender + " is here: " + sender_MAC).c_str(),
            doc.GetAllocator());
        doc.AddMember("Msg", msg, doc.GetAllocator());
#else
        aout.set_attr(p_attr::a_YEL)
            .printf(" ARP Reply.")
            .set_attr(p_attr::a_RED)
            .printf("%s is here: %s\n", sender.c_str(), sender_MAC.c_str());
#endif
    }
}
void do_expand_TCP_segment(const tcp_header *tcp
#ifdef USE_JSON
                           ,
                           json::Document &doc
#endif
) {
#ifdef USE_JSON
    json::Value msg;
    std::string _msg;
    _msg += format_string(
        "Ack Number: %u ; Seq Number: %u; Data offset: %u; Window size: %u",
        htonl(tcp->ack), htonl(tcp->seq), htonl(tcp->flags) & 0xf000,
        htonl(tcp->window));
    msg.SetString(_msg.c_str(), doc.GetAllocator());
    doc.AddMember("Msg", msg, doc.GetAllocator());
#else
    aout.set_attr(p_attr::a_BG_LBLU, p_attr::a_BOLD, p_attr::a_YEL)
        .printf("Ack Number: %u ; Seq Number: %u; ", htonl(tcp->ack),
                htonl(tcp->seq))
        .set_attr(p_attr::a_BG_GRN, p_attr::a_MAG)
        .printf("Data offset: %u; Window size: %u", htonl(tcp->flags) & 0xf000,
                htonl(tcp->window))
        .printf("\n");
#endif
}
void do_expand_QQ_packet(const u_char *qq
#ifdef USE_JSON
                         ,
                         json::Document &doc
#endif
) {
    // skip 0x02/0x00
    ++qq;
    u_short version = htons(*reinterpret_cast<const u_short *>(qq));
    qq += 2;
    u_short opt = htons(*reinterpret_cast<const u_short *>(qq));
    qq += 2;
    u_short seq = htons(*reinterpret_cast<const u_short *>(qq));
    qq += 2;

    u_int qq_id = htonl(*reinterpret_cast<const u_int *>(qq));

#ifdef USE_JSON
    json::Value msg;
    std::string _msg;
    _msg += format_string("QQ: %d is doing `%s`", qq_id, oicq_opt.at(opt).c_str());
    msg.SetString(_msg.c_str(), doc.GetAllocator());
    doc.AddMember("Msg", msg, doc.GetAllocator());
#else
    aout.set_attr(p_attr::a_BG_LBLU, p_attr::a_BOLD, p_attr::a_YEL)
        .printf("QQ: %d is doing ", qq_id)
        .set_attr(p_attr::a_BG_GRN, p_attr::a_MAG)
        .printf("`%s`", oicq_opt.at(opt).c_str())
        .printf("\n");

#endif
}