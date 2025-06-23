// File: src/verilog_parser/VerilogParser.cpp

#include "VerilogParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <iostream>
#include <cctype>
#include <thread>
#include <mutex>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QPair>



bool VerilogParser::parseFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: " << file_path << std::endl;
        return false;
    }

    std::string line;
    int line_num = 0;
    while (std::getline(file, line)) {
        ++line_num;
        std::cout << "[INFO] Line " << line_num << ": " << line << std::endl;
        parse_line(strip_comments(line));
    }
    std::cout << "[INFO] Parsing complete." << std::endl;
    return true;
}

bool VerilogParser::parseFileMultithreaded(const std::string& file_path, int num_threads) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: " << file_path << std::endl;
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(strip_comments(line));
    }

    std::vector<std::vector<std::string>> thread_ports(num_threads);
    std::vector<std::vector<std::string>> thread_cells(num_threads);
    std::vector<std::vector<std::string>> thread_nets(num_threads);
    std::mutex mutex_;

    std::vector<std::thread> threads;
    auto worker = [&](int id_thread, int start, int end) {
        std::regex module_decl(R"(^\s*module\s+(\w+)\s*\(([^;]*)\);)");
        std::regex wire_decl(R"(^\s*wire\s+(\S+);)");
        std::regex instance_decl(R"(^\s*(\w+)\s+(\w+)\s*\(.*\);)");
        std::regex instance_start(R"(^\s*(\w+)\s+(\w+)\s*\(\s*$)");
        std::regex pin_conn(R"(\.([A-Za-z0-9_]+)\s*\(\s*([A-Za-z0-9_]+)\s*\))");
        std::regex instance_full(R"(^\s*(\w+)\s+(\w+)\s*\(.*\)\s*;\s*$)");

        bool buffer_mode = false;
        std::string buffer, current_cell;

        for (int i = start; i < end; ++i) {
            const std::string& line = lines[i];
            std::smatch match;

            if (buffer_mode) {
                buffer += " " + line;
                if (line.find(");") != std::string::npos) {
                    if (std::regex_match(buffer, match, instance_full)) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        thread_cells[id_thread].push_back(match[2]);
                        current_cell = match[2];

                        std::sregex_iterator it(buffer.begin(), buffer.end(), pin_conn);
                        std::sregex_iterator end_it;
                        while (it != end_it) {
                            std::string pin = (*it)[1];
                            std::string net = (*it)[2];
                            pins_by_cell_[current_cell].push_back(pin);
                            net_by_pin_[{current_cell, pin}] = net;
                            ++it;
                        }
                    }
                    buffer.clear();
                    buffer_mode = false;
                }
                continue;
            }

            if (std::regex_search(line, match, module_decl)) {
                std::stringstream ss(match[2]);
                std::string port_id;
                while (std::getline(ss, port_id, ',')) {
                    port_id.erase(std::remove_if(port_id.begin(), port_id.end(), ::isspace), port_id.end());
                    if (!port_id.empty()) thread_ports[id_thread].push_back(port_id);
                }
            } else if (std::regex_search(line, match, wire_decl)) {
                thread_nets[id_thread].push_back(match[1]);
            } else if (std::regex_match(line, match, instance_decl)) {
                std::lock_guard<std::mutex> lock(mutex_);
                thread_cells[id_thread].push_back(match[2]);
                current_cell = match[2];

                std::sregex_iterator it(line.begin(), line.end(), pin_conn);
                std::sregex_iterator end_it;
                while (it != end_it) {
                    std::string pin = (*it)[1];
                    std::string net = (*it)[2];
                    pins_by_cell_[current_cell].push_back(pin);
                    net_by_pin_[{current_cell, pin}] = net;
                    ++it;
                }
            } else if (std::regex_match(line, match, instance_start)) {
                buffer = line;
                buffer_mode = true;
            }
        }
    };

    int chunk = lines.size() / num_threads;
    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk;
        int end = (i == num_threads - 1) ? lines.size() : (i + 1) * chunk;
        threads.emplace_back(worker, i, start, end);
    }

    for (auto& t : threads) t.join();

    for (int i = 0; i < num_threads; ++i) {
        ports_.insert(ports_.end(), thread_ports[i].begin(), thread_ports[i].end());
        nets_.insert(nets_.end(), thread_nets[i].begin(), thread_nets[i].end());
        cells_.insert(cells_.end(), thread_cells[i].begin(), thread_cells[i].end());
    }

    return true;
}

std::string VerilogParser::strip_comments(const std::string& line) {
    auto pos = line.find("//");
    return (pos != std::string::npos) ? line.substr(0, pos) : line;
}



void VerilogParser::parse_line(const std::string& line) {
    if (line.empty()) return;

    std::regex module_decl(R"(^\s*module\s+(\w+)\s*\(([^;]*)\);)");
    std::regex wire_decl(R"(^\s*wire\s+(\S+);)");
    std::regex instance_decl(R"(^\s*(\w+)\s+(\w+)\s*\(.*\);)");
    std::regex instance_start(R"(^\s*(\w+)\s+(\w+)\s*\(\s*$)");
    std::regex instance_full(R"(^\s*(\w+)\s+(\w+)\s*\(.*\)\s*;\s*$)");

    std::smatch match;

    if (buffering_instance_) {
        instance_buffer_ += " " + line;
        if (line.find(");") != std::string::npos) {
            if (std::regex_match(instance_buffer_, match, instance_full)) {
                cells_.push_back(match[2]);
            }
            buffering_instance_ = false;
            instance_buffer_.clear();
        }
        return;
    }

    if (std::regex_search(line, match, module_decl)) {
        extract_identifiers(match[2], ports_);
        inside_module_ = true;
    }
    else if (std::regex_search(line, match, wire_decl)) {
        nets_.push_back(match[1]);
    }
    else if (std::regex_match(line, match, instance_decl)) {
        cells_.push_back(match[2]);
    }
    else if (std::regex_match(line, match, instance_start)) {
        instance_buffer_ = line;
        buffering_instance_ = true;
    }
}

void VerilogParser::extract_identifiers(const std::string& text, std::vector<std::string>& target) {
    std::stringstream ss(text);
    std::string id;
    while (std::getline(ss, id, ',')) {
        id.erase(std::remove_if(id.begin(), id.end(), ::isspace), id.end());
        if (!id.empty()) target.push_back(id);
    }
}

std::vector<std::string> VerilogParser::get_ports() const {
    return ports_;
}

std::vector<std::string> VerilogParser::get_cells() const {
    return cells_;
}

std::vector<std::string> VerilogParser::get_nets() const {
    return nets_;
}

std::vector<std::string> VerilogParser::get_pins(const std::string& cell) const {
    auto it = pins_by_cell_.find(cell);
    if (it != pins_by_cell_.end()) return it->second;
    return {};
}

std::string VerilogParser::get_net_for_pin(const std::string& cell, const std::string& pin) const {
    auto it = net_by_pin_.find({cell, pin});
    return (it != net_by_pin_.end()) ? it->second : "";
}

QMap<QString, QStringList> VerilogParser::getPinsByCell() const {
    QMap<QString, QStringList> result;
    for (const auto& [cell, pins] : pins_by_cell_) {
        QString qcell = QString::fromStdString(cell);
        QStringList qpins;
        for (const auto& pin : pins) {
            qpins.append(QString::fromStdString(pin));
        }
        result[qcell] = qpins;
    }
    return result;
}

QMap<QPair<QString, QString>, QString> VerilogParser::getNetByPin() const {
    QMap<QPair<QString, QString>, QString> result;
    for (const auto& [pin_key, net] : net_by_pin_) {
        QString qcell = QString::fromStdString(pin_key.first);
        QString qpin  = QString::fromStdString(pin_key.second);
        QString qnet  = QString::fromStdString(net);
        result[{qcell, qpin}] = qnet;
    }
    return result;
}

