#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "kota/http/detail/common.h"

namespace kota::http {

struct request {
    std::string method = std::string(http::method::get);
    std::string url;
    std::vector<header> headers;
    std::string cookie;
    std::vector<query_param> query;
    std::string body;
    std::string user_agent;
    std::optional<proxy> proxy_config;
    redirect_policy redirect = redirect_policy::limited();
    tls_options tls{};
    std::optional<std::chrono::milliseconds> timeout;
    std::vector<curl_option_hook> curl_options;
    bool record_cookie = true;
    bool disable_proxy = false;
};

}  // namespace kota::http
