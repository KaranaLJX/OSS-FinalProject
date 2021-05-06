#ifndef COLOR_H
#define COLOR_H

// #define RED "\x1B[31m"
// #define GRN "\x1B[32m"
// #define LMAG "\x1B[95m"
// #define LGRN "\x1B[92m"
// #define YEL "\x1B[33m"
// #define BLU "\x1B[34m"
// #define MAG "\x1B[35m"
// #define CYN "\x1B[36m"
// #define WHT "\x1B[37m"
// #define RESET "\x1B[0m"
// #define BOLD "\x1B[1m"
// #define REVERSE "\x1B[7m"
// #define UNDERLINE "\x1B[4m"
// #define CLEAR "\x1B[2J"

// #define BG_RED "\x1B[41m"
// #define BG_GRN "\x1B[42m"
// #define BG_YEL "\x1B[43m"
// #define BG_MAG "\x1B[45m"
// #define BG_BLU "\x1B[44m"
// #define BG_LBLU "\x1B[104m"
// #define BG_LRED "\x1B[101m"
// #define BG_LGRN "\x1B[102m"
// #define BG_LYEL "\x1B[103m"
// #define BG_LMAG "\x1B[105m"

#define VTCODES_ALL             \
    VTCODES(a_RED, "31")      \
    VTCODES(a_GRN, "32")      \
    VTCODES(a_LMAG, "95")     \
    VTCODES(a_LGRN, "92")     \
    VTCODES(a_YEL, "33")      \
    VTCODES(a_BLU, "34")      \
    VTCODES(a_MAG, "35")      \
    VTCODES(a_CYN, "36")      \
    VTCODES(a_WHT, "37")      \
    VTCODES(a_RESET, "0")     \
    VTCODES(a_BOLD, "1")      \
    VTCODES(a_REVERSE, "7")   \
    VTCODES(a_UNDERLINE, "4") \
    VTCODES(a_CLEAR, "2")     \
    VTCODES(a_BG_RED, "41")   \
    VTCODES(a_BG_GRN, "42")   \
    VTCODES(a_BG_YEL, "43")   \
    VTCODES(a_BG_MAG, "45")   \
    VTCODES(a_BG_BLU, "44")   \
    VTCODES(a_BG_LBLU, "104") \
    VTCODES(a_BG_LRED, "101") \
    VTCODES(a_BG_LGRN, "102") \
    VTCODES(a_BG_LYEL, "103") \
    VTCODES(a_BG_LMAG, "105")

#endif