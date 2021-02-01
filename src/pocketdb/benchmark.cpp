// Copyright (c) 2021 PocketNet developers
//-----------------------------------------------------
#include "benchmark.h"

std::unique_ptr<Benchmark> g_benchmark;

Benchmark::Benchmark() {
}

Benchmark::~Benchmark() = default;

long long int Benchmark::Begin() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void Benchmark::End(long long int begin, const string &checkpoint, const string &payload) {
    // if benchmark allowed
    if (!g_logger->WillLogCategory(BCLog::BENCHMARK))
        return;

    // get end (now) time
    auto end = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    auto sqlBegin = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // save in db
    g_pocket_repository->Add({
        begin,
        end,
        checkpoint,
        payload
    });

    auto sqlEnd = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    LogPrint(BCLog::BENCHMARK, "Benchmark time %s - %lldms (SQL Write: %lldms)\n", checkpoint, (end - begin) / 1000, (sqlEnd - sqlBegin) / 1000);
}


