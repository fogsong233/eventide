#pragma once

#include "kota/http/detail/curl.h"

namespace kota::http::detail {

curl::easy_error ensure_curl_runtime() noexcept;

void mark_request_operation_removed(void* opaque) noexcept;

void complete_request_operation(void* opaque,
                                curl::easy_error result,
                                bool resume_inline) noexcept;

}  // namespace kota::http::detail
