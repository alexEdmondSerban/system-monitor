#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

//Collect system metrics using windows api.
//sample() - called from a single thread
//latest() - called from any thread

class MetricsCollector
{
public:
	static MetricsCollector& instance();

	//new measurement and return JSON snapshot
	std::string sample();

	//last calculated snapshot
	std::string latest() const;

private:
	MetricsCollector() = default;

	struct ProcTimes {
		uint64_t creationTime = 0;//to detect PIDs reused
		uint64_t cpuTime = 0;//kernel + user
	};

	uint64_t m_prevSysIdle = 0;
	uint64_t m_prevSysTotal = 0;
	std::unordered_map<uint32_t, ProcTimes> m_prevProcTimes;

	mutable std::mutex m_mutex;
	std::string m_latest = "{}";
};