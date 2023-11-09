/*
 *  write by chenshan@mchz.com.cn
 */
#pragma once

#include "rcli.h"
#include <chrono>
#include <cstdio>
#include <thread>

#define T_HASH_KEY "cs_test_hash"
#define T_ZSET_KEY "cs_test_zset"
#define T_EXPIRE_KEY "cs_test_expire"

static void test_exist(RedisClient* rcli, const char* key) {
    if (rcli->exist(key)) {
        fprintf(stdout, "[exist  ] %s, true\n", key);
    } else {
        const std::string& err = rcli->get_last_error();
        if (err.size() > 0) {
            fprintf(stderr, "[exist  ] error: %s\n", err.c_str());
        } else {
            fprintf(stdout, "[exist  ] %s, false\n", key);
        }
    }
}

static void test_hash(RedisClient* rcli) {
    const char key[] = T_HASH_KEY;
    fprintf(stdout, "================[%s]================\n", key);

    if (rcli->exist(key)) {
        if (rcli->del(key)) {
            fprintf(stdout, "[del    ] %s\n", key);
        } else {
            fprintf(stderr, "[del    ] error: %s\n", rcli->get_last_error().c_str());
        }
    }

    for (int i = 0; i < 10; i++) {
        int64_t out = 0;
        std::string field = "field" + std::to_string(i);
        std::string val = std::to_string(i);
        if (rcli->hset(key, field, val, out)) {
            fprintf(stdout, "[hset   ] %s : %s, ret = %lld\n", field.c_str(), val.c_str(), out);
        } else {
            fprintf(stderr, "[hset   ] error: %s\n", rcli->get_last_error().c_str());
        }
        if (rcli->hincrby(key, field, 10, out)) {
            fprintf(stdout, "[hincrby] %s + 10, ret = %lld\n", field.c_str(), out);
        } else {
            fprintf(stderr, "[hincrby] error: %s\n", rcli->get_last_error().c_str());
        }
    }

    std::vector<std::string> field_vec;
    if (rcli->hkeys(key, field_vec)) {
        fprintf(stdout, "[hkeys  ] count: %zu\n", field_vec.size());
    } else {
        fprintf(stderr, "[hkeys  ] error: %s\n", rcli->get_last_error().c_str());
    }
    for (auto& k : field_vec) {
        std::string out_val;
        if (rcli->hget(key, k, out_val)) {
            fprintf(stdout, "[hget   ] %s : %s\n", k.c_str(), out_val.c_str());
        } else {
            fprintf(stderr, "[hget   ] error: %s\n", rcli->get_last_error().c_str());
        }
        if (rcli->hdel(key, k)) {
            fprintf(stdout, "[hdel   ] %s\n", k.c_str());
        } else {
            fprintf(stderr, "[hdel   ] error: %s\n", rcli->get_last_error().c_str());
        }
    }

    field_vec.clear();
    if (rcli->hkeys(key, field_vec)) {
        fprintf(stdout, "[hkeys  ] count: %zu\n", field_vec.size());
    } else {
        fprintf(stderr, "[hkeys  ] error: %s\n", rcli->get_last_error().c_str());
    }
}

static void test_set(RedisClient* rcli) {
    const char key[] = T_ZSET_KEY;
    fprintf(stdout, "================[%s]================\n", key);

    if (rcli->exist(key)) {
        if (rcli->del(key)) {
            fprintf(stdout, "[del    ] %s\n", key);
        } else {
            fprintf(stderr, "[del    ] error: %s\n", rcli->get_last_error().c_str());
        }
    }

    std::vector<RedisClient::score_member_t> score_member_vec;
    std::string log_members;
    for (int i = 0; i < 10; i++) {
        std::string member = "m" + std::to_string(i);
        double score = i * 1000.0;
        score_member_vec.emplace_back(std::make_pair(score, member));
        log_members += member + "=" + std::to_string(score) + " ";
    }
    int64_t ret = 0;
    if (rcli->zadd(key, score_member_vec, ret)) {
        fprintf(stdout, "[zadd   ] %s, ret = %lld\n", log_members.c_str(), ret);
    } else {
        fprintf(stderr, "[zadd   ] error: %s\n", rcli->get_last_error().c_str());
    }

    ret = 0;
    if (rcli->zadd(key, std::make_pair(99.0, std::string("mm")), ret)) {
        fprintf(stdout, "[zadd   ] mm, ret = %lld\n", ret);
    } else {
        fprintf(stderr, "[zadd   ] error: %s\n", rcli->get_last_error().c_str());
    }

    std::vector<std::string> member_vec;
    if (rcli->zrange(key, 0, -1, member_vec)) {
        fprintf(stdout, "[zrange ] count: %zu\n", member_vec.size());
    } else {
        fprintf(stderr, "[zrange ] error: %s\n", rcli->get_last_error().c_str());
    }

    for (auto& mem : member_vec) {
        double score;
        if (rcli->zscore(key, mem, score)) {
            fprintf(stdout, "[zscore ] %s = %f\n", mem.c_str(), score);
        } else {
            fprintf(stderr, "[zscore ] error: %s\n", rcli->get_last_error().c_str());
        }
        ret = 0;
        if (rcli->zrem(key, mem, ret)) {
            fprintf(stdout, "[zrem   ] %s, ret = %lld\n", mem.c_str(), ret);
        } else {
            fprintf(stderr, "[zrem   ] error: %s\n", rcli->get_last_error().c_str());
        }
    }

    int64_t count = 0;
    if (rcli->zcard(key, count)) {
        fprintf(stdout, "[zcard  ] count: %lld\n", count);
    } else {
        fprintf(stderr, "[zcard  ] error: %s\n", rcli->get_last_error().c_str());
    }
}

static void test_expire_key(RedisClient* rcli) {
    const char key[] = T_EXPIRE_KEY;
    fprintf(stdout, "================[%s]================\n", key);

    if (rcli->set(key, "aaaa")) {
        fprintf(stdout, "[set    ] %s = aaaa\n", key);
    } else {
        fprintf(stderr, "[set    ] error: %s\n", rcli->get_last_error().c_str());
    }

    if (rcli->expire(key, 5)) {
        fprintf(stdout, "[expire ] %s, 5s\n", key);
    } else {
        fprintf(stderr, "[expire ] error: %s\n", rcli->get_last_error().c_str());
    }

    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fprintf(stdout, "waiting %ds ...\n", i + 1);
    }

    if (rcli->exist(key)) {
        fprintf(stdout, "[exist    ] %s, ret = true\n", key);
    } else {
        fprintf(stdout, "[exist    ] %s, ret = false\n", key);
    }
}