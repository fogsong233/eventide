#pragma once

#include <memory>

#include "kota/http/detail/curl.h"

namespace kota::http::detail {

struct request_runtime_state;
using request_runtime_ref = std::shared_ptr<request_runtime_state>;

curl::easy_error ensure_curl_runtime() noexcept;

request_runtime_ref make_request_runtime_state() noexcept;

void* request_runtime_opaque(const request_runtime_ref& runtime) noexcept;

request_runtime_ref retain_request_operation(void* opaque) noexcept;

void mark_request_operation_removed(const request_runtime_ref& runtime) noexcept;

void complete_request_operation(const request_runtime_ref& runtime,
                                curl::easy_error result,
                                bool resume_inline) noexcept;

}  // namespace kota::http::detail
