// Copyright (c) 2021 PocketNet developers
//-----------------------------------------------------

#ifndef POCKETNET_CORE_BENCHMARK_H
#define POCKETNET_CORE_BENCHMARK_H

#include "sync.h"
#include "util.h"
#include "pocketdb/pocketrepository.h"

#include <string>
#include <map>

using namespace std;

#include <chrono>

using namespace std::chrono;


class Benchmark {
private:

public:
    Benchmark();

    ~Benchmark();

    long long int Begin();

    void End(long long int begin, const string& checkpoint, const string& payload);
};

extern std::unique_ptr<Benchmark> g_benchmark;

#endif //POCKETNET_CORE_BENCHMARK_H
