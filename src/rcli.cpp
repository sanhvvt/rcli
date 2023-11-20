#include "rcli.h"
#include <hiredis/hiredis.h>

#ifdef _MSC_VER
#    include <winsock2.h>
#    ifndef strcasecmp
#        define strcasecmp stricmp
#    endif
#    ifndef strncasecmp
#        define strncasecmp strnicmp
#    endif
#endif

class RedisClientImpl {
public:
    bool connect(const std::string& host, uint32_t port);
    bool reconnect();
    redisContext* get_context() { return ctx_.get(); }
    int check_reply_type(const redisReply* reply);
    int get_reply_status(const redisReply* reply);
    int get_reply_integer(const redisReply* reply, int64_t& retval);
    int get_reply_string(const redisReply* reply, std::string& retval);
    int get_reply_vector(const redisReply* reply, std::vector<std::string>& retval);
    int get_reply_double(const redisReply* reply, double& retval);
    int cmp_reply_string(const redisReply* reply, const std::string& val);

    CSmartPtr<redisContext, redisFree> ctx_;
    std::string error_str_;
};

bool RedisClientImpl::connect(const std::string& host, uint32_t port) {
    if (ctx_) {
        return true;
    }
    redisOptions redis_opts = {0};
    REDIS_OPTIONS_SET_TCP(&redis_opts, host.c_str(), port);
    ctx_.reset(redisConnectWithOptions(&redis_opts));
    struct timeval timeout_val;
    timeout_val.tv_sec = 30;
    timeout_val.tv_usec = 0;
    redisSetTimeout(ctx_.get(), timeout_val);
    redisEnableKeepAlive(ctx_.get());
    if (ctx_ == nullptr) {
        error_str_ = "Redis Context nullptr!";
        return false;
    } else if (ctx_->err) {
        error_str_.assign(ctx_->errstr);
        return false;
    }
    return true;
}

bool RedisClientImpl::reconnect() {
    if (ctx_ == nullptr) {
        return false;
    } else {
        int ret = redisReconnect(ctx_.get());
        return ret == REDIS_OK;
    }
}

int RedisClientImpl::check_reply_type(const redisReply* reply) {
    error_str_.clear();
    if (NULL == reply) {
        if (ctx_->err) {
            error_str_.assign(ctx_->errstr);
        } else {
            error_str_ = "Redis reply is null!";
        }
        return RCLI_ERROR;
    }

    int err = RCLI_RET_UNKNOWN;
    switch (reply->type) {
        case REDIS_REPLY_STRING: err = RCLI_RET_OK; break;
        case REDIS_REPLY_ARRAY: err = RCLI_RET_OK; break;
        case REDIS_REPLY_INTEGER: err = RCLI_RET_OK; break;
        case REDIS_REPLY_STATUS: err = RCLI_RET_OK; break;
        case REDIS_REPLY_DOUBLE: err = RCLI_RET_OK; break;
        // error
        case REDIS_REPLY_NIL: {
            err = RCLI_RET_NIL;
            error_str_.assign("Redis reply nil");
            break;
        }
        case REDIS_REPLY_ERROR: {
            err = RCLI_RET_ERROR;
            error_str_.assign(reply->str, reply->len);
            break;
        }
        default: err = RCLI_RET_UNKNOWN; break;
    }
    return err;
}

int RedisClientImpl::get_reply_status(const redisReply* reply) {
    int ret = check_reply_type(reply);
    if (ret == RCLI_RET_OK) {
        if (REDIS_REPLY_STATUS == reply->type) {
            ret = RCLI_RET_OK;
        } else if (REDIS_REPLY_STRING == reply->type) {
            ret = reply->str && reply->len >= 2 && strcasecmp(reply->str, "OK") == 0 ? RCLI_RET_OK : RCLI_RET_FAIL;
        } else {
            ret = reply->integer == 1 ? RCLI_RET_OK : RCLI_RET_FAIL;
        }
    }
    return ret;
}

int RedisClientImpl::get_reply_integer(const redisReply* reply, int64_t& retval) {
    int err = check_reply_type(reply);
    if (err == RCLI_RET_OK) {
        retval = reply->integer;
    }
    return err;
}

int RedisClientImpl::get_reply_string(const redisReply* reply, std::string& retval) {
    int err = check_reply_type(reply);
    if (err == RCLI_RET_OK) {
        retval.assign(reply->str, reply->len);
    }
    return err;
}

int RedisClientImpl::get_reply_vector(const redisReply* reply, std::vector<std::string>& retval) {
    int err = check_reply_type(reply);
    if (err == RCLI_RET_OK) {
        retval.reserve(reply->elements);
        for (size_t i = 0; i < reply->elements; i++) {
            std::string val(reply->element[i]->str, reply->element[i]->len);
            retval.emplace_back(val);
        }
    }
    return err;
}

int RedisClientImpl::get_reply_double(const redisReply* reply, double& retval) {
    int err = check_reply_type(reply);
    if (err == RCLI_RET_OK) {
        retval = reply->dval;
    }
    return err;
}

int RedisClientImpl::cmp_reply_string(const redisReply* reply, const std::string& val) {
    int err = check_reply_type(reply);
    if (err == RCLI_RET_OK) {
        if (reply->str && reply->len > 0) {
            err = strcasecmp(reply->str, val.c_str()) == 0 ? RCLI_RET_OK : RCLI_RET_FAIL;
        }
    }
    return err;
}

RedisClient::RedisClient() { impl_ = new RedisClientImpl; }

RedisClient::~RedisClient() {
    if (impl_) {
        RedisClientImpl* cli = (RedisClientImpl*) impl_;
        delete cli;
    }
}

void RedisClient::init(const std::string& host, uint32_t port, const std::string& pwd) {
    host_ = host;
    port_ = port;
    pwd_ = pwd;
}

const std::string& RedisClient::get_last_error() {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    return cli->error_str_;
}

bool RedisClient::connect() {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    if (cli->connect(host_, port_)) {
        return auth();
    } else {
        return false;
    }
}

bool RedisClient::reconnect() {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    if (cli->reconnect()) {
        return auth();
    } else {
        return false;
    }
}

int RedisClient::command_for_status(const char* cmd, ...) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    va_list args;
    va_start(args, cmd);
    CSmartPtr<void, freeReplyObject> reply_sp(redisvCommand(cli->get_context(), cmd, args));
    va_end(args);
    return cli->get_reply_status((redisReply*) reply_sp.get());
}

int RedisClient::command_for_integer(int64_t& retval, const char* cmd, ...) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    va_list args;
    va_start(args, cmd);
    CSmartPtr<void, freeReplyObject> reply_sp(redisvCommand(cli->get_context(), cmd, args));
    va_end(args);
    return cli->get_reply_integer((redisReply*) reply_sp.get(), retval);
}

int RedisClient::command_for_double(double& retval, const char* cmd, ...) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    va_list args;
    va_start(args, cmd);
    CSmartPtr<void, freeReplyObject> reply_sp(redisvCommand(cli->get_context(), cmd, args));
    va_end(args);
    return cli->get_reply_double((redisReply*) reply_sp.get(), retval);
}

int RedisClient::command_for_string(std::string& retval, const char* cmd, ...) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    va_list args;
    va_start(args, cmd);
    CSmartPtr<void, freeReplyObject> reply_sp(redisvCommand(cli->get_context(), cmd, args));
    va_end(args);
    return cli->get_reply_string((redisReply*) reply_sp.get(), retval);
}

int RedisClient::command_for_vector(std::vector<std::string>& retval, const char* cmd, ...) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    va_list args;
    va_start(args, cmd);
    CSmartPtr<void, freeReplyObject> reply_sp(redisvCommand(cli->get_context(), cmd, args));
    va_end(args);
    return cli->get_reply_vector((redisReply*) reply_sp.get(), retval);
}

int RedisClient::commandv_for_status(const std::vector<std::string>& cmd) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    std::vector<const char*> argv(cmd.size());
    std::vector<size_t> argvlen(cmd.size());
    int n = 0;
    for (auto it = cmd.begin(); it != cmd.end(); ++it, ++n) {
        argv[n] = it->c_str();
        argvlen[n] = it->size();
    }
    CSmartPtr<void, freeReplyObject> reply_sp(
      redisCommandArgv(cli->get_context(), argv.size(), &(argv[0]), &(argvlen[0])));
    return cli->get_reply_status((redisReply*) reply_sp.get());
}

int RedisClient::commandv_for_integer(int64_t& retval, const std::vector<std::string>& cmd) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    std::vector<const char*> argv(cmd.size());
    std::vector<size_t> argvlen(cmd.size());
    int n = 0;
    for (auto it = cmd.begin(); it != cmd.end(); ++it, ++n) {
        argv[n] = it->c_str();
        argvlen[n] = it->size();
    }
    CSmartPtr<void, freeReplyObject> reply_sp(
      redisCommandArgv(cli->get_context(), argv.size(), &(argv[0]), &(argvlen[0])));
    return cli->get_reply_integer((redisReply*) reply_sp.get(), retval);
}

int RedisClient::commandv_for_string(std::string& retval, const std::vector<std::string>& cmd) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    std::vector<const char*> argv(cmd.size());
    std::vector<size_t> argvlen(cmd.size());
    int n = 0;
    for (auto it = cmd.begin(); it != cmd.end(); ++it, ++n) {
        argv[n] = it->c_str();
        argvlen[n] = it->size();
    }
    CSmartPtr<void, freeReplyObject> reply_sp(
      redisCommandArgv(cli->get_context(), argv.size(), &(argv[0]), &(argvlen[0])));
    return cli->get_reply_string((redisReply*) reply_sp.get(), retval);
}

int RedisClient::commandv_for_vector(std::vector<std::string>& retval, const std::vector<std::string>& cmd) {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    std::vector<const char*> argv(cmd.size());
    std::vector<size_t> argvlen(cmd.size());
    int n = 0;
    for (auto it = cmd.begin(); it != cmd.end(); ++it, ++n) {
        argv[n] = it->c_str();
        argvlen[n] = it->size();
    }
    CSmartPtr<void, freeReplyObject> reply_sp(
      redisCommandArgv(cli->get_context(), argv.size(), &(argv[0]), &(argvlen[0])));
    return cli->get_reply_vector((redisReply*) reply_sp.get(), retval);
}

bool RedisClient::auth() {
    if (pwd_.size() > 0) {
        return command_for_status("AUTH %s", pwd_.c_str()) == RCLI_RET_OK;
    } else {
        return true;
    }
}

bool RedisClient::ping() {
    RedisClientImpl* cli = (RedisClientImpl*) impl_;
    bool ret = false;
    CSmartPtr<void, freeReplyObject> reply_sp(redisCommand(cli->get_context(), "PING"));
    redisReply* reply = (redisReply*) reply_sp.get();
    return cli->cmp_reply_string((redisReply*) reply_sp.get(), "PONG") == RCLI_RET_OK;
}
