#ifndef PRINTER_HH
#define PRINTER_HH
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

#include "../Include/color.h"
#include "../Include/util.hh"

enum printer_attributes {
#define VTCODES(attr, str) attr,
    VTCODES_ALL
#undef VTCODES
};
using p_attr = printer_attributes;
namespace sniffle {

class printer : virtual private std::basic_ostream<char> {
   private:
    static std::string do_get_attribute(int attr) {
        switch (attr) {
#define VTCODES(attri, str)         \
    case printer_attributes::attri: \
        return str;
            VTCODES_ALL
#undef VTCODES
        }
        return "";
    }
    std::ofstream _out_file;
    bool _file_set = false;
    bool _focus_disable_prettify = false;
    bool _enable_prettify = true;
    bool _copy_to_file = false;
    std::unique_ptr<FILE, std::function<void(FILE*)>> unique_fp;

    std::streambuf *stdout_buf, *stderr_buf, *file_buf;
    std::streambuf* cur_buf;

    std::vector<int> attr;
    friend printer& disable_pretiffy(printer&);
    friend printer& enable_pretiffy(printer&);

    std::string do_concat_attrs() {
        using namespace std::string_literals;
        std::string tmp = "\x1B[";
        int n = attr.size();
        for (int i = 0; i < n; ++i)
            tmp +=
                do_get_attribute(attr[i]) +
                ";mJ"[attr[i] != printer_attributes::a_CLEAR ? i == n - 1 : 2];
        return tmp;
    }

    template <typename... Attr>
    friend printer& set_attr(printer&, Attr&&...);

   public:
    printer& clear() {
        if (!_file_set) {
            this->rdbuf(cur_buf);
            this->write("\x1B[2J", sizeof("\x1B[2J"));
        }
        return *this;
    }
    printer& operator<<(printer& (*func)(printer&)) { return func(*this); }
    template <typename... Args>
    printer& printf(const char* format, Args&&... args) {
        std::string attr = do_concat_attrs();
        std::string res = format;
        if (_enable_prettify) {
            res = attr + format + "\x1B[0m";
        }
        std::string buffer = format_string(res.c_str(), std::forward<Args>(args)...);
        int len = buffer.length();
        this->rdbuf(cur_buf);
        this->write(buffer.c_str(), len);
        if (_enable_prettify) {
            this->write("\x1B[0m", sizeof("\x1B[0m"));
        }
        this->flush();

        if (_copy_to_file && cur_buf == stdout_buf && file_buf) {
            buffer = format_string(format, std::forward<Args>(args)...);
            len = buffer.length();
            this->rdbuf(file_buf);
            this->write(buffer.c_str(), len);
            this->flush();
            this->rdbuf(cur_buf);
        }
        // Reset
        clear_attr();
        return *this;
    }
    printer& open(const std::string& filename) {
        _out_file.open(filename, std::ios_base::openmode::_S_out);
        this->file_buf = _out_file.rdbuf();
        return *this;
    }
    printer& std_out() {
        if (!_focus_disable_prettify)  
            _enable_prettify = true;
        this->cur_buf = this->stdout_buf;
        return *this;
    }
    printer& std_err() {
        if (!_focus_disable_prettify)
            _enable_prettify = true;
        this->cur_buf = this->stderr_buf;
        return *this;
    }
    printer& file() {
        _enable_prettify = false;
        this->cur_buf = this->file_buf;
        return *this;
    }
    printer& clear_attr() {
        attr.clear();
        return *this;
    }
    printer& enable_copy_to_file() {
        _copy_to_file = true;
        return *this;
    }
    template <typename... attrs>
    printer& set_attr(attrs&&... x) {
        attr.clear();
        attr = {std::forward<attrs>(x)...};
        return *this;
    }
    static printer& get_instance() {
        static std::shared_ptr<printer> _printer_ptr{new printer};
        return *_printer_ptr;
    }
    printer(const printer&) = delete;
    printer(printer&&) = delete;
    printer& operator=(const printer&) = delete;
    printer& operator=(printer&&) = delete;

    printer()
        : std::basic_ostream<char>(),
          stdout_buf(std::cout.rdbuf()),
          stderr_buf(std::cerr.rdbuf()),
          cur_buf(stdout_buf) {}

    printer(const std::string& filename) : printer() {
        _out_file.open(filename, std::ios_base::openmode::_S_out);
        this->file_buf = _out_file.rdbuf();
    }
};

inline printer& disable_pretiffy(printer& out) {
    out._enable_prettify = false;
    out._focus_disable_prettify = true;
    return out;
}
inline printer& enable_pretiffy(printer& out) {
    out._enable_prettify = true;
    out._focus_disable_prettify = true;
    return out;
}

}  // namespace sniffle

extern sniffle::printer& aout;

#endif