#ifndef WIN32_PORT_HH
#define WIN32_PORT_HH
#ifdef WIN32
// For Windows API Call
#include <windows.h>
#include <imagehlp.h>
#include <dbghelp.h>

// 设置 Win32 结构化异常处理; 捕获 Segmentation Fault 事件
void do_set_exception_filter();
const char* do_translate_addr_to_line(const char*, const DWORD64);
WINAPI LONG sniffle_unhandled_exception_filter(_EXCEPTION_POINTERS* );
#endif
#endif