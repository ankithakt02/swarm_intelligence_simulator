/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Implementation of the CSV writer (see MetricsLogger.h).
*/

#include "bench/MetricsLogger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <cstdlib>
#else
#include <unistd.h>
#endif

namespace swarm
{

bool MetricsLogger::open(const std::string& path, const std::vector<std::string>& header, bool append)
{
    namespace fs = std::filesystem;
    m_path = path;
    m_columns = header.size();

    std::error_code ec;
    const fs::path parent = fs::path(path).parent_path();
    if (!parent.empty())
    {
        fs::create_directories(parent, ec);
    }

    const bool writeHeader = !append || !fs::exists(path) || fs::file_size(path, ec) == 0;
    m_out.open(path, append ? std::ios::app : std::ios::trunc);
    if (!m_out)
    {
        std::cerr << "error: cannot open " << path << " for writing\n";
        return false;
    }
    if (writeHeader)
    {
        for (std::size_t i = 0; i < header.size(); ++i)
        {
            m_out << (i ? "," : "") << escape(header[i]);
        }
        m_out << '\n';
    }
    return true;
}

MetricsLogger& MetricsLogger::add(const std::string& value)
{
    m_row.push_back(escape(value));
    return *this;
}

MetricsLogger& MetricsLogger::add(double value, int precision)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    m_row.push_back(ss.str());
    return *this;
}

MetricsLogger& MetricsLogger::add(long long value)
{
    m_row.push_back(std::to_string(value));
    return *this;
}

MetricsLogger& MetricsLogger::add(unsigned long long value)
{
    m_row.push_back(std::to_string(value));
    return *this;
}

void MetricsLogger::endRow()
{
    if (m_columns != 0 && m_row.size() != m_columns)
    {
        std::cerr << "warning: CSV row has " << m_row.size() << " cells, header has " << m_columns
                  << " (" << m_path << ")\n";
    }
    for (std::size_t i = 0; i < m_row.size(); ++i)
    {
        m_out << (i ? "," : "") << m_row[i];
    }
    m_out << '\n';
    m_out.flush();  // keep partial results if a PACE-ICE job hits its time limit
    m_row.clear();
}

std::string MetricsLogger::escape(const std::string& value)
{
    if (value.find_first_of(",\"\n") == std::string::npos)
    {
        return value;
    }
    std::string out = "\"";
    for (char c : value)
    {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

std::string hostName()
{
#if defined(_WIN32)
    const char* name = std::getenv("COMPUTERNAME");
    return name ? name : "unknown";
#else
    char buf[256] = {};
    if (gethostname(buf, sizeof(buf) - 1) == 0)
    {
        return buf;
    }
    return "unknown";
#endif
}

std::string timestampNow()
{
    const std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tmLocal{};
#if defined(_WIN32)
    localtime_s(&tmLocal, &t);
#else
    localtime_r(&t, &tmLocal);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmLocal, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

}  // namespace swarm
