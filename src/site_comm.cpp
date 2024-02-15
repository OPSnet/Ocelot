// Copyright [2017-2024] Orpheus

#include <spdlog/spdlog.h>

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <queue>
#include <mutex>
#include <thread>

#include <boost/asio.hpp>

#include "config.h"
#include "site_comm.h"

using boost::asio::ip::tcp;

site_comm::site_comm(config * conf) : t_active(false) {
    logger = spdlog::get("logger");
    load_config(conf);
}

void site_comm::load_config(config * conf) {
    site_host = conf->get_str("site_host");
    site_service = conf->get_str("site_service");
    site_path = conf->get_str("site_path");
    site_password = conf->get_str("site_password");
    readonly = conf->get_bool("readonly");
}

void site_comm::reload_config(config * conf) {
    load_config(conf);
}

bool site_comm::all_clear() {
    return (token_queue.size() == 0);
}

void site_comm::expire_token(int torrent, int user) {
    std::stringstream token_pair;
    token_pair << user << ':' << torrent;
    if (!expire_token_buffer.empty()) {
        expire_token_buffer += ",";
    }
    expire_token_buffer += token_pair.str();
    if (expire_token_buffer.length() > 350) {
        logger->info("Flushing overloaded token buffer");
        if (!readonly) {
            std::lock_guard<std::mutex> lock(expire_queue_lock);
            token_queue.push(expire_token_buffer);
        }
        expire_token_buffer.clear();
    }
}

void site_comm::flush_tokens()
{
    if (readonly) {
        expire_token_buffer.clear();
        return;
    }
    std::lock_guard<std::mutex> lock(expire_queue_lock);
    size_t qsize = token_queue.size();
    if (verbose_flush || qsize > 0) {
        logger->info("Token expire queue size: " + std::to_string(qsize));
    }
    if (expire_token_buffer.empty()) {
        return;
    }
    token_queue.push(expire_token_buffer);
    expire_token_buffer.clear();
    if (!t_active) {
        std::thread thread(&site_comm::do_flush_tokens, this);
        thread.detach();
    }
}

void site_comm::do_flush_tokens()
{
    t_active = true;
    try {
        while (token_queue.size() > 0) {
            boost::asio::io_service io_service;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(site_host, site_service);
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            tcp::resolver::iterator end;

            tcp::socket socket(io_service);
            boost::system::error_code error = boost::asio::error::host_not_found;
            while (error && endpoint_iterator != end) {
                socket.close();
                socket.connect(*endpoint_iterator++, error);
            }
            if (error) {
                throw boost::system::system_error(error);
            }

            boost::asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << "GET " << site_path << "/tools.php?key=" << site_password
                << "&type=expiretoken&action=ocelot&tokens=" << token_queue.front() << " HTTP/1.0\r\n"
                << "Host: " << site_host << "\r\n"
                << "Accept: */*\r\n"
                << "Connection: close\r\n\r\n";

            boost::asio::write(socket, request);

            boost::asio::streambuf response;
            boost::asio::read_until(socket, response, "\r\n");

            std::istream response_stream(&response);
            std::string http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);

            if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                logger->info("Invalid response");
                continue;
            }

            if (status_code == 200) {
                std::lock_guard<std::mutex> lock(expire_queue_lock);
                token_queue.pop();
            } else {
                logger->error("Response returned with status code " + std::to_string(status_code) + " when trying to expire a token!");
                // TODO: add exponential backoff
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    } catch (std::exception &er) {
        logger->error("Exception: " + std::string(er.what()));
    }
    t_active = false;
}

site_comm::~site_comm()
{
}
