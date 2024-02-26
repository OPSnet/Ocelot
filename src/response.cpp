// Copyright [2017-2024] Orpheus

#include <algorithm>
#include <sstream>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "misc_functions.h"
#include "response.h"

const std::string http_head(size_t content_length, client_opts_t &client_opts) {
    const std::string content_type = client_opts.html ? "text/html" : "text/plain";

    // rely on preprocessor string concatenation
    std::string head = "HTTP/1.1 200 OK\r\nServer: Ocelot " OCELOT_VERSION "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + inttostr(content_length) + "\r\n";
    if (client_opts.gzip) {
        head += "Content-Encoding: gzip\r\n";
    }
    if (client_opts.http_close) {
        head += "Connection: Close\r\n";
    }
    return head + "\r\n";
}

const std::string http_response(const std::string &body, client_opts_t &client_opts) {
    std::string out;
    bool processed = false;
    if (client_opts.html) {
        out = "<html><head><meta name=\"robots\" content=\"noindex, nofollow\" /></head><body>" + body + "</body></html>";
        processed = true;
    }
    if (client_opts.gzip) {
        std::stringstream ss, zss;
        ss << body;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_compressor());
        in.push(ss);
        boost::iostreams::copy(in, zss);
        out = zss.str();
        processed = true;
    }
    if (processed) {
        return http_head(out.length(), client_opts) + out;
    }
    return http_head(body.length(), client_opts) + body;
}

const std::string error(const std::string &message, client_opts_t &client_opts) {
    return http_response(
        "d14:failure reason" + inttostr(message.length()) + ':' + message
            + "12:min intervali5400e8:intervali5400ee",
        client_opts
    );
}
