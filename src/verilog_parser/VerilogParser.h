#pragma once
#include <QMap>
#include <QString>
#include <QStringList>
#include <QPair>

#include <string>
#include <vector>
#include <unordered_map>

// Hash and Equal function for std::pair<std::string, std::string>
struct PairHash {
    std::size_t operator()(const std::pair<std::string, std::string>& p) const {
        std::hash<std::string> hasher;
        return hasher(p.first) ^ (hasher(p.second) << 1);
    }
};

struct PairEqual {
    bool operator()(const std::pair<std::string, std::string>& lhs,
                    const std::pair<std::string, std::string>& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

class VerilogParser {
public:
    bool parseFile(const std::string& file_path);
    bool parseFileMultithreaded(const std::string& file_path, int num_threads);
    std::vector<std::string> get_ports() const;
    std::vector<std::string> get_cells() const;
    std::vector<std::string> get_nets() const;
    std::vector<std::string> get_pins(const std::string& cell) const;
    std::string get_net_for_pin(const std::string& cell, const std::string& pin) const;
    QMap<QString, QStringList> getPinsByCell() const;
    QMap<QPair<QString, QString>, QString> getNetByPin() const;

private:
    std::string strip_comments(const std::string& line);
    void parse_line(const std::string& line);
    void extract_identifiers(const std::string& text, std::vector<std::string>& target);

    bool inside_module_ = false;
    bool buffering_instance_ = false;
    std::string instance_buffer_;

    std::vector<std::string> ports_;
    std::vector<std::string> nets_;
    std::vector<std::string> cells_;
    std::unordered_map<std::string, std::vector<std::string>> pins_by_cell_;
    std::unordered_map<std::pair<std::string, std::string>, std::string, PairHash, PairEqual> net_by_pin_;
};

