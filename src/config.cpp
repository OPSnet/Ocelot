// Copyright [2017-2024] Orpheus

#include <spdlog/spdlog.h>

#include <iostream>
#include <fstream>
#include <string>

#include "config.h"
#include "misc_functions.h"

confval::confval() {
    bool_val = 0;
    uint_val = 0;
    str_val = "";
    val_type = CONF_NONEXISTENT;
}

confval::confval(bool value) {
    bool_val = value;
    val_type = CONF_BOOL;
}

confval::confval(unsigned int value) {
    uint_val = value;
    val_type = CONF_UINT;
}

confval::confval(const char * value) {
    str_val = value;
    val_type = CONF_STR;
}

bool confval::get_bool() {
    return bool_val;
}

unsigned int confval::get_uint() {
    return uint_val;
}

std::string confval::get_str() {
    return str_val;
}

void confval::set(const std::string &value) {
    if (val_type == CONF_BOOL) {
        bool_val = value == "1" || value == "true" || value == "yes";
    } else if (val_type == CONF_UINT) {
        uint_val = strtoint32(value);
    } else if (val_type == CONF_STR) {
        str_val = value;
    }
}

config::config() {
    init();
    dummy_setting = new confval();  // Safety value to use if we're accessing nonexistent settings
}

void config::init() {
    // Internal stuff
    add("listen_port", 34000u);
    add("max_connections", 1024u);
    add("max_middlemen", 20000u);
    add("max_read_buffer", 4096u);
    add("connection_timeout", 10u);
    add("keepalive_timeout", 0u);
    add("daemonize", false);

    // Tracker requests
    add("announce_interval", 1800u);
    add("announce_jitter", 240u);
    add("max_request_size", 4096u);
    add("numwant_limit", 50u);
    add("request_log_size", 500u);

    // Timers
    add("del_reason_lifetime", 86400u);
    add("peers_timeout", 7200u);
    add("reap_peers_interval", 1800u);
    add("schedule_interval", 3u);

    // MySQL
    add("mysql_db", "gazelle");
    add("mysql_host", "localhost");
    add("mysql_port", 3306u);
    add("mysql_username", "");
    add("mysql_password", "");
    add("mysql_port", 3306u);

    // Site communication
    add("site_host", "127.0.0.1");
    add("site_service", "http");
    add("site_path", "");
    add("site_password", "00000000000000000000000000000000");
    add("report_password", "00000000000000000000000000000000");

    // Logging
    add("log", false);
    add("log_path", "ocelot");  // path to where to write log + name of log (don't need to put file extension)

    add("report_path", "/tmp");  // path for transfer to website (should be a tmpfs in production)

    // Debugging
    add("readonly", false);
}

confval * config::get(const std::string &setting_name) {
    const auto setting = settings.find(setting_name);
    if (setting == settings.end()) {
        spdlog::get("logger")->info("WARNING: Unrecognized setting '" + setting_name + "'");
        return dummy_setting;
    }
    return &setting->second;
}

bool config::get_bool(const std::string &setting_name) {
    return get(setting_name)->get_bool();
}

unsigned int config::get_uint(const std::string &setting_name) {
    return get(setting_name)->get_uint();
}

std::string config::get_str(const std::string &setting_name) {
    return get(setting_name)->get_str();
}

void config::set(const std::string &setting_name, const std::string &value) {
    get(setting_name)->set(value);
}

void config::load(const std::string &conf_file_path, std::istream &conf_file) {
    load(conf_file);
    add("conf_file_path", conf_file_path.c_str());
}

void config::load(std::istream &conf_file) {
    std::string line;
    while (getline(conf_file, line)) {
        size_t pos;
        if (line[0] != '#' && (pos = line.find('=')) != std::string::npos) {
            std::string key(trim(line.substr(0, pos)));
            std::string value(trim(line.substr(pos + 1)));
            set(key, value);
        }
    }
}

void config::reload() {
    const std::string conf_file_path(get_str("conf_file_path"));
    std::ifstream conf_file(conf_file_path);
    if (conf_file.fail()) {
        spdlog::get("logger")->error("Config file '" + conf_file_path + "' couldn't be opened");
    } else {
        init();
        load(conf_file);
    }
}

std::string config::trim(const std::string str) {
    size_t ltrim = str.find_first_not_of(" \t");
    if (ltrim == std::string::npos) {
        ltrim = 0;
    }
    size_t rtrim = str.find_last_not_of(" \t");
    if (ltrim != 0 || rtrim != str.length() - 1) {
        return str.substr(ltrim, rtrim - ltrim + 1);
    }
    return str;
}
