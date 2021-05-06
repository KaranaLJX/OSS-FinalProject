#include "../Include/win32_port.hh"

#include <cstdio>
#include <memory>

#include "../Include/color.h"
#include "../Include/util.hh"
#include "../Include/printer.hh"

#ifdef __GNUG__
#include <cxxabi.h>
#endif
void do_set_exception_filter() {
    SetUnhandledExceptionFilter(sniffle_unhandled_exception_filter);
}
const char* do_recognize_type(DWORD code) {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "EXCEPTION_ACCESS_VIOLATION";

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";

        case EXCEPTION_BREAKPOINT:
            return "EXCEPTION_BREAKPOINT";

        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "EXCEPTION_DATATYPE_MISALIGNMENT";

        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "EXCEPTION_FLT_DENORMAL_OPERAND";

        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "EXCEPTION_FLT_DIVIDE_BY_ZERO";

        case EXCEPTION_FLT_INEXACT_RESULT:
            return "EXCEPTION_FLT_INEXACT_RESULT";

        case EXCEPTION_FLT_INVALID_OPERATION:
            return "EXCEPTION_FLT_INVALID_OPERATION";

        case EXCEPTION_FLT_OVERFLOW:
            return "EXCEPTION_FLT_OVERFLOW";

        case EXCEPTION_FLT_STACK_CHECK:
            return "EXCEPTION_FLT_STACK_CHECK";

        case EXCEPTION_FLT_UNDERFLOW:
            return "EXCEPTION_FLT_UNDERFLOW";

        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "EXCEPTION_ILLEGAL_INSTRUCTION";

        case EXCEPTION_IN_PAGE_ERROR:
            return "EXCEPTION_IN_PAGE_ERROR";

        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "EXCEPTION_INT_DIVIDE_BY_ZERO";

        case EXCEPTION_INT_OVERFLOW:
            return "EXCEPTION_INT_OVERFLOW";

        case EXCEPTION_INVALID_DISPOSITION:
            return "EXCEPTION_INVALID_DISPOSITION";

        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "EXCEPTION_NONCONTINUABLE_EXCEPTION";

        case EXCEPTION_PRIV_INSTRUCTION:
            return "EXCEPTION_PRIV_INSTRUCTION";

        case EXCEPTION_SINGLE_STEP:
            return "EXCEPTION_SINGLE_STEP";

        case EXCEPTION_STACK_OVERFLOW:
            return "EXCEPTION_STACK_OVERFLOW";

        default:
            return "Unrecognized Exception";
    }
}
WINAPI LONG
sniffle_unhandled_exception_filter(_EXCEPTION_POINTERS* _exception_info) {
    auto str_err_type =
        do_recognize_type(_exception_info->ExceptionRecord->ExceptionCode);
    aout.set_attr(p_attr::a_RED)
        .std_err()
        .printf("Fatal exception `%s` from here: \n", str_err_type);
    SymInitialize(GetCurrentProcess(), 0, true);

    STACKFRAME frame;
    memset(&frame, 0, sizeof frame);

    frame.AddrPC.Offset = _exception_info->ContextRecord->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = _exception_info->ContextRecord->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = _exception_info->ContextRecord->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;

    static char module_name[1024] = {0};

    GetModuleFileName(GetModuleHandle(nullptr), module_name,
                      sizeof module_name);

    while (StackWalk(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(),
                     GetCurrentThread(), &frame, _exception_info->ContextRecord,
                     0, SymFunctionTableAccess64, SymGetModuleBase, 0)) {
        aout.std_err()
            .set_attr(p_attr::a_YEL)
            .printf("At: 0x%012p ", frame.AddrPC.Offset);
        aout.std_err()
            .set_attr(p_attr::a_RED, p_attr::a_UNDERLINE)
            .printf("%s\n", do_translate_addr_to_line(module_name,
                                                      frame.AddrPC.Offset));
    }

    SymCleanup(GetCurrentProcess());
    // 获得 Console 的 Handle
    static char win_title[512];
    const char* new_title = "SniffleIsHereWithUniqueTitle";
    GetConsoleTitle(win_title, sizeof win_title);
    SetConsoleTitle(new_title);
    Sleep(20);
    HWND hwnd = FindWindow(nullptr, new_title);
    SetConsoleTitle(win_title);
    MessageBox(hwnd,
               format_string("Exception: %s\nAddress: 0x%012p\n\nPlease check the stack frame from `stderr` and make a bug report.", str_err_type,
                             _exception_info->ContextRecord->Rip)
                   .c_str(),
               "Unhandled Exception", MB_OK | MB_ICONERROR);
    // ----------------------
    exit(1);
    return EXCEPTION_EXECUTE_HANDLER;
}
const char* do_translate_addr_to_line(const char* name, const DWORD64 ptr) {
    auto format_str = format_string("addr2line -f -p -e %.256s %p", name, ptr);
    FILE* fp = popen(format_str.c_str(), "r");

    std::unique_ptr<FILE, void (*)(FILE*)> fp_guard(
        fp, [](FILE* ptr) { pclose(ptr); });

    static char buf[10240] = {0};
    fscanf(fp, "%s", buf);
#ifdef __GNUG__
    return abi::__cxa_demangle(buf, nullptr, nullptr, nullptr);
#else
    return buf;
#endif
}