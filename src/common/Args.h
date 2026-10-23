/*
Author: Shravya Vasudeva
Class: ECE 6122
Last Date Modified: 10/23/2026

Description:
Tiny command-line helpers shared by the GUI and benchmark executables:
"--key value" lookup and comma-separated list parsing with validation.
*/

#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace swarm
{

class Args
{
public:
    Args(int argc, char** argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            m_args.emplace_back(argv[i]);
        }
    }

    bool has(const std::string& flag) const
    {
        for (const std::string& a : m_args)
        {
            if (a == flag) return true;
        }
        return false;
    }

    /// Value after "--key", or fallback if absent.
    std::string get(const std::string& key, const std::string& fallback) const
    {
        for (std::size_t i = 0; i + 1 < m_args.size(); ++i)
        {
            if (m_args[i] == key) return m_args[i + 1];
        }
        return fallback;
    }

    /// Flags that look like "--something" but are not in the known list.
    std::vector<std::string> unknown(const std::vector<std::string>& known) const
    {
        std::vector<std::string> bad;
        for (const std::string& a : m_args)
        {
            if (a.rfind("--", 0) != 0) continue;
            bool found = false;
            for (const std::string& k : known)
            {
                if (a == k) found = true;
            }
            if (!found) bad.push_back(a);
        }
        return bad;
    }

private:
    std::vector<std::string> m_args;
};

inline std::vector<std::string> splitList(const std::string& text)
{
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

/// Parses an unsigned integer; accepts suffix k/K (x1000) so "100k" works. Exits on error.
inline unsigned long long parseCount(const std::string& text, const std::string& what)
{
    std::string digits = text;
    unsigned long long scale = 1;
    if (!digits.empty() && (digits.back() == 'k' || digits.back() == 'K'))
    {
        scale = 1000;
        digits.pop_back();
    }
    char* end = nullptr;
    const unsigned long long v = std::strtoull(digits.c_str(), &end, 10);
    if (digits.empty() || end == nullptr || *end != '\0')
    {
        std::cerr << "error: invalid " << what << " '" << text << "'\n";
        std::exit(1);
    }
    return v * scale;
}

inline double parseReal(const std::string& text, const std::string& what)
{
    char* end = nullptr;
    const double v = std::strtod(text.c_str(), &end);
    if (text.empty() || end == nullptr || *end != '\0')
    {
        std::cerr << "error: invalid " << what << " '" << text << "'\n";
        std::exit(1);
    }
    return v;
}

}  // namespace swarm
