#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cctype>
#include <vector>
#include <map>
#include <algorithm>

namespace vns::utils {

// Time utilities
inline uint64_t current_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline std::string format_duration_ns(uint64_t ns) {
    if (ns < 1000) return std::to_string(ns) + " ns";
    if (ns < 1000000) return std::to_string(ns / 1000) + " us";
    if (ns < 1000000000) return std::to_string(ns / 1000000) + " ms";
    return std::to_string(ns / 1000000000) + " s";
}

inline std::string format_timestamp(uint64_t ns) {
    auto sec = ns / 1000000000;
    auto ms = (ns % 1000000000) / 1000000;
    auto time_t_sec = static_cast<std::time_t>(sec);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_sec));
    std::string result(buf);
    result += "." + std::to_string(ms);
    return result;
}

// String utilities
inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

inline std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(str.begin(), str.end(), result.begin(), ::tolower);
    return result;
}

inline std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(str.begin(), str.end(), result.begin(), ::toupper);
    return result;
}

template<typename T>
std::string join(const std::vector<T>& vec, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << vec[i];
    }
    return oss.str();
}

// Formatting
inline std::string format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

inline std::string format_number(uint64_t n) {
    std::string str = std::to_string(n);
    for (int i = str.length() - 3; i > 0; i -= 3) {
        str.insert(i, ",");
    }
    return str;
}

inline std::string format_percent(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (value * 100) << "%";
    return oss.str();
}

// Table formatting
class TableBuilder {
public:
    TableBuilder() = default;
    
    void add_column(const std::string& header, size_t width = 0, bool left_align = true) {
        headers_.push_back(header);
        widths_.push_back(width);
        alignments_.push_back(left_align);
    }
    
    void add_row(const std::vector<std::string>& row) {
        if (row.size() != headers_.size()) return;
        rows_.push_back(row);
        // Update widths
        for (size_t i = 0; i < row.size(); ++i) {
            if (widths_[i] < row[i].length()) {
                widths_[i] = row[i].length();
            }
        }
    }
    
    std::string build() const {
        std::ostringstream oss;
        
        // Header
        for (size_t i = 0; i < headers_.size(); ++i) {
            if (i > 0) oss << " | ";
            std::string h = headers_[i];
            if (widths_[i] > h.length()) h += std::string(widths_[i] - h.length(), ' ');
            oss << h;
        }
        oss << "\n";
        
        // Separator
        for (size_t i = 0; i < headers_.size(); ++i) {
            if (i > 0) oss << "-+-";
            oss << std::string(widths_[i], '-');
        }
        oss << "\n";
        
        // Rows
        for (const auto& row : rows_) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) oss << " | ";
                std::string cell = row[i];
                if (widths_[i] > cell.length()) {
                    cell += std::string(widths_[i] - cell.length(), ' ');
                }
                oss << cell;
            }
            oss << "\n";
        }
        
        return oss.str();
    }
    
    void clear_rows() { rows_.clear(); }
    
private:
    std::vector<std::string> headers_;
    std::vector<size_t> widths_;
    std::vector<bool> alignments_;
    std::vector<std::vector<std::string>> rows_;
};

} // namespace vns::utils