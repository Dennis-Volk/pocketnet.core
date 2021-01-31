// Copyright (c) 2021 PocketNet developers
//-----------------------------------------------------
#include "benchmark.h"

std::unique_ptr<Benchmark> g_benchmark;

Benchmark::Benchmark() {
    _id = 0;
}

Benchmark::~Benchmark() = default;

int Benchmark::Begin() {
    // if benchmark allowed
    if (!gArgs.GetBoolArg("-benchmark", false))
        return 0;

    int id = 0;
    {
        LOCK(cs);
        id = _id;
        _id += 1;
    }

    if (_checkpoints.find(id) != _checkpoints.end()) {
        _checkpoints.erase(id);
    }

    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    _checkpoints.insert(std::make_pair(id, now));
    return id;
}

void Benchmark::End(int id, const string &checkpoint, const string &payload) {
    // if benchmark allowed
    if (!gArgs.GetBoolArg("-benchmark", false))
        return;

    // get end (now) time
    auto end = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // get saved begin time
    long long int begin = 0;
    {
        LOCK(cs);
        if (_checkpoints.find(id) != _checkpoints.end()) {
            begin = _checkpoints[id];
            _checkpoints.erase(id);
        }
    }

    // save in db
    g_pocket_repository->Add({
        begin,
        end,
        checkpoint,
        payload
    });
}


