#include "opt_parser.h"
#include "redis_test.h"
#include <iostream>
#include <memory>

void split(const std::string& s, std::string delim, std::vector<std::string>* ret) {
    size_t last = 0;
    size_t index = s.find_first_of(delim, last);
    while (index != std::string::npos) {
        ret->push_back(s.substr(last, index - last));
        last = index + 1;
        index = s.find_first_of(delim, last);
    }
    if (index - last > 0) {
        ret->push_back(s.substr(last, index - last));
    }
}

std::unique_ptr<RedisClient> create_redis_cli(const std::string& redis_host) {
    std::unique_ptr<RedisClient> redis_cli = nullptr;

    // 127.0.0.1:6380:hzmcdba
    std::vector<std::string> host_vec;
    split(redis_host, ":", &host_vec);
    if (host_vec.size() != 3) {
        fprintf(stderr, "RedisClient host error!\n");
        return nullptr;
    }
    redis_cli.reset(new RedisClient);
    int port = atoi(host_vec[1].c_str());
    redis_cli->init(host_vec[0], port, host_vec[2]);
    fprintf(stdout, "[RedisClient] connect: start %s\n", redis_host.c_str());

    bool connected = redis_cli->connect();
    for (int n = 0; !connected && n < 3; n++) {
        fprintf(stderr, "[RedisClient] connect error: %s, trying %d ...\n", redis_cli->get_last_error().c_str(), n);
        if (redis_cli->reconnect()) {
            connected = true;
            break;
        }
    }

    if (!connected) {
        return nullptr;
    }

    bool check_alive = redis_cli->ping();
    if (!check_alive) {
        fprintf(stderr, "[RedisClient] check: %s, closeing ...\n", redis_cli->get_last_error().c_str());
        return nullptr;
    }

    fprintf(stdout, "[RedisClient] connect: success!\n");
    return redis_cli;
}

void run_test(RedisClient* rcli, const std::string& cmd) {
    if (cmd == "*" || cmd == "hash") {
        test_hash(rcli);
    }
    if (cmd == "*" || cmd == "zset") {
        test_set(rcli);
    }
    if (cmd == "*" || cmd == "expire") {
        test_expire_key(rcli);
    }
    if (strncasecmp(cmd.c_str(), "exist", 5) == 0) {
        test_exist(rcli, cmd.c_str() + 6);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Hello world!" << std::endl;
    std::string host_;
    bool test_ = false;

    OptionParser optr;
    optr.add_opt("-h", true, [&](int id, const char* str) { host_.assign(str); });
    optr.add_opt("-t", false, [&](int id, const char* str) { test_ = true; });
    optr.cmdline(argc, argv);

    auto rcli = create_redis_cli(host_);
    if (rcli == nullptr) {
        fprintf(stderr, "host error: %s\n", host_.c_str());
        return 0;
    }

    if (test_) {
        run_test(rcli.get(), "*");
        std::cout << "Bye!" << std::endl;
        return 0;
    }

    while(1) {
        std::string cmd = "";
        std::getline(std::cin, cmd);
        if (cmd == "q") {
            break;
        } else {
            run_test(rcli.get(), cmd);
        }
    }
    std::cout << "Bye!" << std::endl;
    return 0;
}
