#include "MetricsCollector.h"

#include <json/json.h>
#include <algorithm>
#include <chrono>
#include <cmath>

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

namespace {

	uint64_t toUInt64(const FILETIME& ft)
	{
		return (uint64_t(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
	}

	double round1(double v)
	{
		return std::round(v * 10.0) / 10.0;
	}

	std::string toUtf8(const wchar_t* wide)
	{
		const int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 1)
		{
			return {};
		}

		std::string result(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), len, nullptr, nullptr);
		result.resize(len - 1);//extract '\0' terminator
		return result;
	}
} //namespace

MetricsCollector& MetricsCollector::instance()
{
	static MetricsCollector collector;
	return collector;
}

std::string MetricsCollector::sample()
{
	// ---------- CPU total ----------
	FILETIME idle, kernel, user;
	GetSystemTimes(&idle, &kernel, &user);

	const uint64_t sysIdle = toUInt64(idle);
	const uint64_t sysTotal = toUInt64(kernel) + toUInt64(user); //kernel includes idle

	const uint64_t idleDiff = sysIdle - m_prevSysIdle;
	const uint64_t totalDiff = sysTotal - m_prevSysTotal;
	const bool havePrevious = m_prevSysTotal != 0 && totalDiff > 0;

	const double cpu = havePrevious ? 100.0 * double(totalDiff - idleDiff) / double(totalDiff) : 0.0;
	m_prevSysIdle = sysIdle;
	m_prevSysTotal = sysTotal;

	Json::Value root;
	root["timestamp"] = Json::Int64(
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	root["cpu"] = round1(cpu);

	// ---------- RAM ----------
	MEMORYSTATUSEX mem{};
	mem.dwLength = sizeof(mem);
	if (GlobalMemoryStatusEx(&mem)) {
		constexpr double GB = 1024.0 * 1024.0 * 1024.0;
		const double total = mem.ullTotalPhys / GB;
		const double used = (mem.ullTotalPhys - mem.ullAvailPhys) / GB;
		root["memory"]["totalGB"] = round1(total);
		root["memory"]["usedGB"] = round1(used);
		root["memory"]["percent"] = round1(total > 0 ? 100.0 * used / total : 0.0);
	}

	// ---------- Proccesses ----------
    Json::Value processes(Json::arrayValue);
    std::unordered_map<uint32_t, ProcTimes> currentTimes;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        if (Process32FirstW(snapshot, &entry)) {
            do {
                const DWORD pid = entry.th32ProcessID;
                if (pid == 0) // "System Idle Process"
                    continue;

                double procCpu = 0.0;
                double memoryMB = 0.0;

                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                if (process) {
                    FILETIME creation, exitTime, procKernel, procUser;
                    if (GetProcessTimes(process, &creation, &exitTime, &procKernel, &procUser)) {
                        const ProcTimes now{ toUInt64(creation),
                                             toUInt64(procKernel) + toUInt64(procUser) };
                        currentTimes[pid] = now;

                        auto it = m_prevProcTimes.find(pid);
                        if (havePrevious && it != m_prevProcTimes.end()
                            && it->second.creationTime == now.creationTime
                            && now.cpuTime >= it->second.cpuTime) {
                            procCpu = 100.0 * double(now.cpuTime - it->second.cpuTime) / double(totalDiff);
                        }
                    }

                    PROCESS_MEMORY_COUNTERS counters{};
                    if (GetProcessMemoryInfo(process, &counters, sizeof(counters)))
                        memoryMB = counters.WorkingSetSize / (1024.0 * 1024.0);

                    CloseHandle(process);
                }

                Json::Value p;
                p["pid"] = Json::UInt(pid);
                p["name"] = toUtf8(entry.szExeFile);
                p["cpu"] = round1(std::min(procCpu, 100.0));
                p["memoryMB"] = round1(memoryMB);
                processes.append(p);
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    m_prevProcTimes = std::move(currentTimes);
    root["processes"] = processes;

    // ---------- Serialization ----------
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string json = Json::writeString(writer, root);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latest = json;
    }
    return json;
}

std::string MetricsCollector::latest() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latest;
}