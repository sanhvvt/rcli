/*
 *  write by chenshan@mchz.com.cn
 */
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string.h>

typedef std::function<void(int id, const char* str)> opt_func_t;

class OptionParser final {
public:
    OptionParser() = default;
    ~OptionParser() = default;

    void add_opt(const char* name, bool isval, opt_func_t handler) { 
        std::unique_ptr<otp_impl_t> impl(new otp_impl_t);
        impl->name_ = name;
        impl->isval_ = isval;
        impl->handler_ = handler;
        handler_map_.insert(std::make_pair(name, std::move(impl)));
    }

    int cmdline(int argc, char* argv[]) {
        char cmd_string[1024] = {0};
        char* cmd_argv[16] = {0};
        int cmd_argc = 0;
        int opt_c = 0;
        int cmd_nums = argc - 1;
        int cmd_index = 0;
        std::string cmd_buffer = "";
        std::string cmd_space = " ";

        app_name_ = argv[0];

        while (cmd_nums > 0) {
            cmd_index++;
            cmd_buffer += argv[cmd_index] + cmd_space;
            cmd_nums--;
        }

        if (cmd_buffer.length() >= 1024) {
            return 0;
        }
        memcpy(cmd_string, cmd_buffer.c_str(), cmd_buffer.length());

        str_to_args(cmd_string, 128, &cmd_argc, cmd_argv);

        while (cmd_argc) {
            cmd_argc--;
            otp_impl_t* impl = nullptr;
            auto item = handler_map_.find(cmd_argv[opt_c]);
            if (item != handler_map_.end()) {
                impl = item->second.get();
                if (!impl->isval_) {
                    impl->handler_(opt_c, cmd_argv[opt_c]);
                }
            }
            opt_c++;

            if (cmd_argc > 0 && impl->isval_) {
                 cmd_argc--;
                 impl->handler_(opt_c, cmd_argv[opt_c]);
                 opt_c++;
            }
        }

        return 0;
    }

private:
    char* split_argv(char** str, char** word) {
        constexpr char QUOTE = '\'';

        bool inquotes = false;
        if (**str == 0) {
            return NULL;
        }

        while (**str && isspace(**str))
            (*str)++;
        if (**str == '\0') {
            return NULL;
        }
        if (**str == QUOTE) {
            (*str)++;
            inquotes = true;
        }
        *word = *str;
        if (inquotes) {
            while (**str && **str != QUOTE)
                (*str)++;
        } else {
            while (**str && !isspace(**str))
                (*str)++;
        }

        if (**str) {
            *(*str)++ = '\0';
        }

        return *str;
    }

    char* str_to_args(char* str, const int argc_max, int* argc, char* argv[]) {
        *argc = 0;

        while (*argc < argc_max - 1 && split_argv(&str, &argv[*argc])) {
            ++(*argc);
            if (*str == '\0')
                break;
        }
        argv[*argc] = nullptr;
        return str;
    };

    std::string app_name_;

    typedef struct otp_impl {
        bool isval_ = false;
        std::string name_;
        opt_func_t handler_;
    } otp_impl_t;

    std::map<std::string, std::unique_ptr<otp_impl_t>> handler_map_;
};
