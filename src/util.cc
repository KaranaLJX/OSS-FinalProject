#include "../Include/util.hh"

std::string get_hex_data(const u_char *ptr, size_t len) {
    const int HEXDUMP_COLS = 16;
    std::string buffer;
// 见 https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#pragma GCC warning \
    "Note that your architecture is little endian. You need to read the bytes printed in reverse order.\n";
#endif
#else
#pragma GCC warning "Your compiler doesn't support __BYTE_ORDER__ macro."
#endif
    // buffer += format_string("At 0x%016x:\n", ptr);
    for (u_int i = 0;
         i <
         len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0);
         i++) {
        /* print offset */
        if (i % HEXDUMP_COLS == 0) {
            buffer += format_string("0x%06x: ", i);
        }
        /* print hex data */
        if (i < len) {
            buffer += format_string("%02x ", 0xFF & ptr[i]);
            // printf("%02x ", 0xFF & (ptr)[i]);
        } else /* end of block, just aligning for ASCII dump */
        {
            buffer += format_string("   ");
        }

        /* print ASCII dump */
        if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
            for (u_int j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
                if (j >= len) /* end of block, not really printing */
                {
                    buffer += format_string(" ");
                } else if (isprint((ptr)[j])) /* printable char */
                {
                    buffer += format_string("%c", 0xFF & ptr[j]);
                } else /* other char */
                {
                    buffer += format_string(".");
                }
            }
            buffer += format_string("\n");
        }
    }
    return buffer;
}

const char *iptos(u_long in) {
    const int IPTOSBUFFERS = 12;
    static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
    static short which;
    u_char *p;
    p = (u_char *)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    _snprintf_s(output[which], sizeof(output[which]), sizeof(output[which]),
                "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}
const char *ip6tos(sockaddr *sockaddr, char *address, int addrlen) {
    socklen_t sockaddrlen;
    memset(address, 0, addrlen);
#ifdef WIN32
    sockaddrlen = sizeof(struct sockaddr_in6);
#else
    sockaddrlen = sizeof(struct sockaddr_storage);
#endif
    getnameinfo(sockaddr, sockaddrlen, address, addrlen, nullptr, 0,
                NI_NUMERICHOST);
    return !strlen(address) ? "(none)" : address;
}

std::string do_get_timestamp(const pcap_pkthdr *header) {
    tm ltime;
    static char _time_str[50], time_str[50];
    time_t local_tv_sec;

    local_tv_sec = header->ts.tv_sec;
    localtime_s(&ltime, &local_tv_sec);
    // ISO8601
    std::strftime(_time_str, sizeof _time_str, "%Y-%m-%dT%H:%M:%S", &ltime);
    sprintf(time_str, "%s.%.3ld", _time_str, header->ts.tv_usec / 1000);
    return time_str;
}
std::string do_format_IP(const v6_address *addr) {
    std::string source;
    source = format_string(
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%"
        "02x",
        addr->byte1, addr->byte2, addr->byte3, addr->byte4, addr->byte5,
        addr->byte6, addr->byte7, addr->byte8, addr->byte9, addr->byte10,
        addr->byte11, addr->byte12, addr->byte13, addr->byte14, addr->byte15,
        addr->byte16);
    static unsigned char buffer[sizeof(in6_addr)];
    std::string res(INET6_ADDRSTRLEN, 0);
    inet_pton(AF_INET6, source.data(), buffer);
    inet_ntop(AF_INET6, buffer, res.data(), INET6_ADDRSTRLEN);
    return res.empty() ? "(none)" : res; 
}
std::string do_format_IPv6(const u_char *addr) {
    std::string source = format_string(
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%"
        "02x",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7],
        addr[8], addr[9], addr[10], addr[11], addr[12], addr[13], addr[14],
        addr[15]);
    static unsigned char buffer[sizeof(in6_addr)];
    std::string res(INET6_ADDRSTRLEN, 0);
    inet_pton(AF_INET6, source.data(), buffer);
    inet_ntop(AF_INET6, buffer, res.data(), INET6_ADDRSTRLEN);
    return res.empty() ? "(none)" : res;   
}
std::string do_format_IP(const ip_address *addr) {
    return format_string("%-3d.%-3d.%-3d.%-3d", addr->byte1, addr->byte2,
                         addr->byte3, addr->byte4);
}
std::string do_format_IP(const ip_address *addr, u_short port) {
    return format_string("%-3d.%-3d.%-3d.%-3d:%-5d", addr->byte1, addr->byte2,
                         addr->byte3, addr->byte4, port);
}
std::string do_format_MAC(u_char a, u_char b, u_char c, u_char d, u_char e,
                          u_char f) {
    return format_string("0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X", a, b, c,
                         d, e, f);
}
