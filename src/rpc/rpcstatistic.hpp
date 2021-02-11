#pragma once

#include "univalue.h"

#include <boost/thread.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <numeric>
#include <set>

namespace RpcStatistic {

using RequestKey = std::string;
using RequestTime = std::chrono::milliseconds;
using RequestIP = std::string;
using RequestPayloadSize = std::size_t;

struct RequestSample {
    RequestKey Key;
    RequestTime TimestampBegin;
    RequestTime TimestampEnd;
    RequestIP SourceIP;
    RequestPayloadSize InputSize;
    RequestPayloadSize OutputSize;
};

class RequestStatEngine
{
public:
    RequestStatEngine() = default;
    RequestStatEngine(const RequestStatEngine&) = delete;
    RequestStatEngine(RequestStatEngine&&) = default;

    void AddSample(const RequestSample& sample)
    {
        if (!LogAcceptCategory(BCLog::RPCSTAT) || sample.TimestampEnd < sample.TimestampBegin)
            return;

        std::lock_guard<std::mutex> lock{_samplesLock};
        _samples.push_back(sample);
    }

    std::size_t GetNumSamples() const
    {
        std::lock_guard<std::mutex> lock{_samplesLock};
        return _samples.size();
    }

    std::size_t GetNumSamplesBetween(RequestTime begin, RequestTime end) const
    {
        std::lock_guard<std::mutex> lock{_samplesLock};
        return std::count_if(
            _samples.begin(),
            _samples.end(),
            [=](const RequestSample& sample) {
                return sample.TimestampBegin >= begin && sample.TimestampEnd <= end;
            });
    }

    std::size_t GetNumSamplesBefore(RequestTime time) const
    {
        return GetNumSamplesBetween(RequestTime::min(), time);
    }

    std::size_t GetNumSamplesSince(RequestTime time) const
    {
        return GetNumSamplesBetween(time, RequestTime::max());
    }

    RequestTime GetAvgRequestTimeSince(RequestTime since) const
    {
        std::lock_guard<std::mutex> lock{_samplesLock};

        if (_samples.size() < 1)
            return {};

        RequestTime sum{};
        std::size_t count{};

        for (auto& sample : _samples) {
            if (sample.TimestampBegin >= since) {
                sum += sample.TimestampEnd - sample.TimestampBegin;
                count++;
            }
        }

        if (count <= 0) return {};
        return sum / count;
    }

    RequestTime GetAvgRequestTime() const
    {
        return GetAvgRequestTimeSince(RequestTime::min());
    }

    std::vector<RequestSample> GetTopHeavyTimeSamplesSince(std::size_t limit, RequestTime since) const
    {
        return GetTopTimeSamplesImpl(limit, since);
    }

    std::vector<RequestSample> GetTopHeavyTimeSamples(std::size_t limit) const
    {
        return GetTopHeavyTimeSamplesSince(limit, RequestTime::min());
    }

    std::vector<RequestSample> GetTopHeavyInputSamplesSince(std::size_t limit, RequestTime since) const
    {
        return GetTopSizeSamplesImpl(limit, &RequestSample::InputSize, since);
    }

    std::vector<RequestSample> GetTopHeavyInputSamples(std::size_t limit) const
    {
        return GetTopHeavyInputSamplesSince(limit, RequestTime::min());
    }

    std::vector<RequestSample> GetTopHeavyOutputSamplesSince(std::size_t limit, RequestTime since) const
    {
        return GetTopSizeSamplesImpl(limit, &RequestSample::OutputSize, since);
    }

    std::vector<RequestSample> GetTopHeavyOutputSamples(std::size_t limit) const
    {
        return GetTopHeavyOutputSamplesSince(limit, RequestTime::min());
    }

    std::set<RequestIP> GetUniqueSourceIPsSince(RequestTime since) const
    {
        std::set<RequestIP> result{};

        std::lock_guard<std::mutex> lock{_samplesLock};
        for (auto& sample : _samples)
            if (sample.TimestampBegin >= since)
                result.insert(sample.SourceIP);

        return result;
    }

    std::set<RequestIP> GetUniqueSourceIPs() const
    {
        return GetUniqueSourceIPsSince(RequestTime::min());
    }

    UniValue CompileStatsAsJsonSince(RequestTime since) const
    {
        constexpr auto top_limit = 5;
        const auto sample_to_json = [](const RequestSample& sample) {
            UniValue value{UniValue::VOBJ};

            value.pushKV("Key", sample.Key);
            value.pushKV("TimestampBegin", sample.TimestampBegin.count());
            value.pushKV("TimestampEnd", sample.TimestampEnd.count());
            value.pushKV("SourceIP", sample.SourceIP);
            value.pushKV("InputSize", (int)sample.InputSize);
            value.pushKV("OutputSize", (int)sample.OutputSize);

            return value;
        };

        auto unique_ips = GetUniqueSourceIPsSince(since);
        auto top_tm = GetTopHeavyTimeSamplesSince(top_limit, since);
        auto top_in = GetTopHeavyInputSamplesSince(top_limit, since);
        auto top_out = GetTopHeavyOutputSamplesSince(top_limit, since);

        UniValue result{UniValue::VOBJ};
        UniValue unique_ips_json{UniValue::VARR};
        UniValue top_tm_json{UniValue::VARR};
        UniValue top_in_json{UniValue::VARR};
        UniValue top_out_json{UniValue::VARR};

        for (auto& ip : unique_ips)
            unique_ips_json.push_back(ip);

        for (auto& sample : top_tm)
            top_tm_json.push_back(sample_to_json(sample));

        for (auto& sample : top_in)
            top_in_json.push_back(sample_to_json(sample));

        for (auto& sample : top_out)
            top_out_json.push_back(sample_to_json(sample));

        result.pushKV("NumSamples", (int)GetNumSamplesSince(since));
        result.pushKV("AverageTime", GetAvgRequestTimeSince(since).count());
        result.pushKV("UniqueIPs", unique_ips_json);
        result.pushKV("TopTime", top_tm_json);
        result.pushKV("TopInputSize", top_in_json);
        result.pushKV("TopOutputSize", top_out_json);

        return result;
    }

    UniValue CompileStatsAsJson() const
    {
        return CompileStatsAsJsonSince(RequestTime::min());
    }

    // Just a helper to prevent copypasta
    RequestTime GetCurrentSystemTime() const
    {
        return std::chrono::duration_cast<RequestTime>(std::chrono::system_clock::now().time_since_epoch());
    }

    void Run(boost::thread_group& threadGroup)
    {
        shutdown = false;
        threadGroup.create_thread(
            boost::bind(
                &RequestStatEngine::PeriodicStatLogger, this));
    }

    void Stop()
    {
        shutdown = true;
    }

    void PeriodicStatLogger()
    {
        auto statLoggerSleep = gArgs.GetArg("-rpcstatdepth", 60) * 1000;
        std::string msg = "RPC Statistic for last %lds:\n---------------------------------------\n%s\n---------------------------------------\n";

        while (!shutdown) {

            if (LogAcceptCategory(BCLog::RPCSTAT)) {
                auto chunkSize = GetCurrentSystemTime() - std::chrono::milliseconds(statLoggerSleep);
                LogPrint(BCLog::RPCSTAT, msg.c_str(), statLoggerSleep / 1000, CompileStatsAsJsonSince(chunkSize).write(1));

                RemoveSamplesBefore(chunkSize);
            }

            MilliSleep(statLoggerSleep);
        }
    }

private:
    std::vector<RequestSample> _samples;
    mutable std::mutex _samplesLock;
    bool shutdown = false;

    void RemoveSamplesBefore(RequestTime time)
    {
        std::lock_guard<std::mutex> lock{_samplesLock};
        _samples.erase(
            std::remove_if(
                _samples.begin(),
                _samples.end(),
                [time](const RequestSample& sample) {
                    return sample.TimestampBegin < time;
                }),
            _samples.end());

        LogPrint(BCLog::RPCSTAT, "Clear RPC Statistic cache: %d items after.", _samples.size());
    }

    std::vector<RequestSample> GetTopSizeSamplesImpl(std::size_t limit, RequestPayloadSize RequestSample::*size_field, RequestTime since) const
    {
        std::lock_guard<std::mutex> lock{_samplesLock};
        auto samples_copy = _samples;

        samples_copy.erase(
            std::remove_if(
                samples_copy.begin(),
                samples_copy.end(),
                [since](const RequestSample& sample) {
                    return sample.TimestampBegin < since;
                }),
            samples_copy.end());

        std::sort(
            samples_copy.begin(),
            samples_copy.end(),
            [limit, size_field](const RequestSample& left, const RequestSample& right) {
                return left.*size_field > right.*size_field;
            });

        if (samples_copy.size() > limit)
            samples_copy.resize(limit);

        return samples_copy;
    }

    std::vector<RequestSample> GetTopTimeSamplesImpl(std::size_t limit, RequestTime since) const
    {
        std::lock_guard<std::mutex> lock{_samplesLock};
        auto samples_copy = _samples;

        samples_copy.erase(
            std::remove_if(
                samples_copy.begin(),
                samples_copy.end(),
                [since](const RequestSample& sample) {
                    return sample.TimestampBegin < since;
                }),
            samples_copy.end());

        std::sort(
            samples_copy.begin(),
            samples_copy.end(),
            [limit](const RequestSample& left, const RequestSample& right) {
                return left.TimestampEnd - left.TimestampBegin > right.TimestampEnd - right.TimestampBegin;
            });

        if (samples_copy.size() > limit)
            samples_copy.resize(limit);

        return samples_copy;
    }
};

} // namespace RpcStatistic
