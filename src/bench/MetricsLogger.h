/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Minimal CSV writer used by the benchmark, the validation run, and the GUI FPS
sweep. Creates parent directories as needed. In append mode the header is only
written when the file is new, so several PACE-ICE jobs can add to one file.
*/

#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace swarm
{

class MetricsLogger
{
public:
    /// Opens the file. Returns false (and prints an error) if it cannot be opened.
    bool open(const std::string& path, const std::vector<std::string>& header, bool append = false);
    bool isOpen() const { return m_out.is_open(); }

    /// Starts a row; add cells with add(), finish with endRow().
    MetricsLogger& add(const std::string& value);
    MetricsLogger& add(const char* value) { return add(std::string(value)); }
    MetricsLogger& add(double value, int precision = 4);
    MetricsLogger& add(long long value);
    MetricsLogger& add(unsigned long long value);
    MetricsLogger& add(int value) { return add(static_cast<long long>(value)); }
    MetricsLogger& add(unsigned value) { return add(static_cast<unsigned long long>(value)); }
    MetricsLogger& add(unsigned long value) { return add(static_cast<unsigned long long>(value)); }
    MetricsLogger& add(long value) { return add(static_cast<long long>(value)); }
    void endRow();

    const std::string& path() const { return m_path; }

private:
    static std::string escape(const std::string& value);

    std::ofstream m_out;
    std::string m_path;
    std::vector<std::string> m_row;
    std::size_t m_columns = 0;
};

/// Hostname of this machine (so CSV rows from different PACE-ICE nodes can be told apart).
std::string hostName();
/// Current local time as "YYYY-MM-DD HH:MM:SS".
std::string timestampNow();

}  // namespace swarm
