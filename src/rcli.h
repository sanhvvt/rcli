/*
 *  write by chenshan@mchz.com.cn
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string.h>
#include <vector>

template <class T, void (*deleter)(T*)>
class CSmartPtr : public std::unique_ptr<T, void (*)(T*)> {
public:
    CSmartPtr() : std::unique_ptr<T, void (*)(T*)>(nullptr, deleter) {}
    CSmartPtr(T* object) : std::unique_ptr<T, void (*)(T*)>(object, deleter) {}
};

#define RCLI_RET_FAIL 1
#define RCLI_RET_OK 0
#define RCLI_RET_NIL -1
#define RCLI_RET_ERROR -2
#define RCLI_RET_UNKNOWN -3
#define RCLI_ERROR -4

#define RCLI_TRY_COUNT 3

#define BEGIN_CHECK_ALIVE()                                                                                            \
    int n = RCLI_TRY_COUNT;                                                                                            \
    do {
#define END_CHECK_ALIVE()                                                                                              \
    }                                                                                                                  \
    while (n-- && err == RCLI_ERROR && !check_alive())

class RedisClient {
public:
    RedisClient();
    virtual ~RedisClient();

    void init(const std::string& host, uint32_t port, const std::string& pwd);
    const std::string& get_last_error();

    bool connect();
    bool reconnect();
    bool auth();
    bool ping();

    int command_for_status(const char* cmd, ...);
    int command_for_integer(int64_t& retval, const char* cmd, ...);
    int command_for_double(double& retval, const char* cmd, ...);
    int command_for_string(std::string& retval, const char* cmd, ...);
    int command_for_vector(std::vector<std::string>& retval, const char* cmd, ...);

    int commandv_for_status(const std::vector<std::string>& cmd);
    int commandv_for_integer(int64_t& retval, const std::vector<std::string>& cmd);
    int commandv_for_string(std::string& retval, const std::vector<std::string>& cmd);
    int commandv_for_vector(std::vector<std::string>& retval, const std::vector<std::string>& cmd);

    bool check_alive() {
        if (!ping()) {
            fprintf(stdout, "ping error, reconnect...\n");
            return reconnect();
        }
        return true;
    }

    // keys

    bool exist(const std::string& key) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("EXISTS %s", key.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool get(const std::string& key, std::string& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_string(out, "GET %s", key.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool set(const std::string& key, const std::string& in) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("SET %s %s", key.c_str(), in.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool del(const std::string& key) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("DEL %s", key.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool expire(const std::string& key, uint32_t second) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("EXPIRE %s %u", key.c_str(), second);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool expireat(const std::string& key, uint32_t timestamp) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("EXPIREAT %s %u", key.c_str(), timestamp);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool pexpire(const std::string& key, uint32_t milliseconds) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("PEXPIRE %s %u", key.c_str(), milliseconds);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    // hash map

    bool hexist(const std::string& key, const std::string& field) {
        int err = RCLI_ERROR;
        std::vector<std::string> cmdv{"HEXISTS", key, field};
        BEGIN_CHECK_ALIVE();
        err = commandv_for_status(cmdv);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool hget(const std::string& key, const std::string& field, std::string& out) {
        int err = RCLI_ERROR;
        std::vector<std::string> cmdv{"HGET", key, field};
        BEGIN_CHECK_ALIVE();
        err = commandv_for_string(out, cmdv);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool hset(const std::string& key, const std::string& field, const std::string& in, int64_t& out) {
        int err = RCLI_ERROR;
        std::vector<std::string> cmdv{"HSET", key, field, in};
        BEGIN_CHECK_ALIVE();
        err = commandv_for_integer(out, cmdv);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool hincrby(const std::string& key, const std::string& field, const int64_t& in, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "HINCRBY %s %s %lld", key.c_str(), field.c_str(), in);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool hdel(const std::string& key, const std::string& field) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_status("HDEL %s %s", key.c_str(), field.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool hkeys(const std::string& key, std::vector<std::string>& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_vector(out, "HKEYS %s", key.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    // sorted set

    typedef std::pair<double, std::string> score_member_t;
    typedef std::pair<double, std::string> incre_member_t;

    bool zadd(const std::string& key, const score_member_t& in, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "ZADD %s %f %s", key.c_str(), in.first, in.second.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zadd(const std::string& key, const std::vector<score_member_t>& in, int64_t& out) {
        int err = RCLI_ERROR;
        std::vector<std::string> cmdv{"ZADD", key};
        for (auto& kv : in) {
            cmdv.emplace_back(std::to_string(kv.first));
            cmdv.emplace_back(kv.second);
        }
        BEGIN_CHECK_ALIVE();
        err = commandv_for_integer(out, cmdv);
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zcard(const std::string& key, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "ZCARD %s", key.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zincrby(const std::string& key, const incre_member_t& in, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "ZINCRBY %s %f %s", key.c_str(), in.first, in.second.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zrange(const std::string& key, int32_t start, int32_t stop, std::vector<std::string>& out,
                bool withscore = false) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        if (withscore) {
            err = command_for_vector(out, "ZRANGE %s %d %d WITHSCORES", key.c_str(), start, stop);
        } else {
            err = command_for_vector(out, "ZRANGE %s %d %d", key.c_str(), start, stop);
        }
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zrangebyscore(const std::string& key, double min, double max, std::vector<std::string>& out,
                       bool withscore = false) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        if (withscore) {
            err = command_for_vector(out, "ZRANGEBYSCORE %s %f %f WITHSCORES", key.c_str(), min, max);
        } else {
            err = command_for_vector(out, "ZRANGEBYSCORE %s %f %f", key.c_str(), min, max);
        }
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zrank(const std::string& key, const std::string& member, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "ZRANK %s %s", key.c_str(), member.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zscore(const std::string& key, const std::string& member, double& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        std::string out_str;
        err = command_for_string(out_str, "ZSCORE %s %s", key.c_str(), member.c_str());
        if (err == RCLI_RET_OK) {
            out = std::stod(out_str);
        }
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

    bool zrem(const std::string& key, const std::string& member, int64_t& out) {
        int err = RCLI_ERROR;
        BEGIN_CHECK_ALIVE();
        err = command_for_integer(out, "ZREM %s %s", key.c_str(), member.c_str());
        END_CHECK_ALIVE();
        return err == RCLI_RET_OK;
    }

private:
    void* impl_ = nullptr;
    std::string host_;
    uint32_t port_;
    std::string pwd_;
};