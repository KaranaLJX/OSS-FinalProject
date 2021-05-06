#ifndef PACKET_PARSER_HH
#define PACKET_PARSER_HH
#include "util.hh"

struct raw_packet {
    u_char *packet;
    protocol_id packet_type;
};
static const std::unordered_map<u_short, const std::string> oicq_opt = {
    {0x0001, "Log out"},
    {0x0002, "Heart Message"},
    {0x0004, "Update User information"},
    {0x0005, "Search user"},
    {0x0006, "Get User informationBroadcast"},
    {0x0009, "Add friend no auth"},
    {0x000a, "Delete user"},
    {0x000b, "Add friend by auth"},
    {0x000d, "Set status"},
    {0x0012, "Confirmation of receiving message from server"},
    {0x0016, "Send message"},
    {0x0017, "Receive message"},
    {0x0018, "Retrieve information"},
    {0x001a, "Reserved "},
    {0x001c, "Delete Me"},
    {0x001d, "Request KEY"},
    {0x0021, "Cell Phone"},
    {0x0022, "Log in"},
    {0x0026, "Get friend list"},
    {0x0027, "Get friend online"},
    {0x0029, "Cell PHONE"},
    {0x0030, "Operation on group"},
    {0x0031, "Log in test"},
    {0x003c, "Group name operation"},
    {0x003d, "Upload group friend"},
    {0x003e, "MEMO Operation"},
    {0x0058, "Download group friend"},
    {0x005c, "Get level"},
    {0x0062, "Request login"},
    {0x0065, "Request extra information"},
    {0x0067, "Signature operation"},
    {0x0080, "Receive system message"},
    {0x0081, "Get status of friend"},
    {0x00b5, "Get friend's status of group"}
};
std::string do_recognize_packet(u_char);
std::string do_recognize_ether_frame(u_short);
const char *do_format_ip_header(const ip_header *);
void do_dump_packet(const u_char *);
#ifdef USE_JSON
void do_expand_TCP_segment(const tcp_header *, json::Document &);
void do_expand_ARP_frame(const ARP_packet *, json::Document &);
bool do_parse_DNS_header(const dns *, json::Document &);
void do_expand_QQ_packet(const u_char*, json::Document &);
#else
void do_expand_TCP_segment(const tcp_header *);
void do_expand_ARP_frame(const ARP_packet *);
bool do_parse_DNS_header(const dns *);
void do_expand_QQ_packet(const u_char*);
#endif
#endif