#include <source_location>

#include "../async/io/awaiter.h"
#include "kota/http/detail/client.h"
#include "kota/http/detail/manager.h"
#include "kota/http/detail/prepared_request.h"

namespace kota::http {

namespace {

struct request_op : uv::await_op<request_op> {
    using promise_t = task<response, error>::promise_type;
    using result_type = outcome<response, error, cancellation>;

    manager* mgr = nullptr;
    detail::prepared_request* prepared = nullptr;
    bool added = false;
    bool completed = false;

    explicit request_op(manager& manager) : mgr(&manager) {}

    void attach(detail::prepared_request& request) noexcept {
        prepared = &request;
    }

    void mark_removed() noexcept {
        added = false;
    }

    void complete_with(error err, bool resume) noexcept {
        if(completed) {
            return;
        }

        completed = true;
        if(prepared) {
            prepared->result = std::move(err);
        }
        added = false;
        if(resume) {
            this->complete();
        }
    }

    void complete_with(curl::easy_error code, bool resume) noexcept {
        complete_with(error::from_curl(code), resume);
    }

    static void on_cancel(system_op* op) {
        uv::await_op<request_op>::complete_cancel(op, [](request_op& aw) {
            if(aw.added && aw.mgr && aw.prepared && aw.prepared->easy) {
                aw.mgr->remove_request(aw.prepared->easy.get());
                aw.added = false;
            }
            if(aw.prepared) {
                aw.prepared->easy.reset();
            }
            aw.completed = true;
        });
    }

    void start() noexcept {
        if(completed || !prepared) {
            return;
        }

        if(!prepared->ready()) {
            completed = true;
            return;
        }

        if(!prepared->runtime_bound) {
            prepared->fail(error::invalid_request("request runtime binding failed"));
            completed = true;
            return;
        }

        if(auto err = mgr->add_request(prepared->easy.get()); !curl::ok(err)) {
            prepared->fail(error::from_curl(curl::to_easy_error(err)));
            completed = true;
            return;
        }

        added = true;
        mgr->drive_timeout_arming(this);
    }

    bool await_ready() const noexcept {
        return completed;
    }

    std::coroutine_handle<>
        await_suspend(std::coroutine_handle<promise_t> waiting,
                      std::source_location loc = std::source_location::current()) noexcept {
        return this->link_continuation(&waiting.promise(), loc);
    }

    result_type await_resume() noexcept {
        if(added && mgr && prepared && prepared->easy) {
            mgr->remove_request(prepared->easy.get());
            added = false;
        }

        if(static_cast<async_node&>(*this).state == async_node::Cancelled) {
            return result_type(outcome_cancel(cancellation("http request cancelled")));
        }

        if(!prepared) {
            return result_type(outcome_error(error::invalid_request("missing prepared request")));
        }

        return prepared->finish();
    }

    void finish(curl::easy_error code) noexcept {
        complete_with(code, true);
    }

    void finish_inline(curl::easy_error code) noexcept {
        complete_with(code, false);
    }
};

}  // namespace

namespace detail {

void mark_request_operation_removed(void* opaque) noexcept {
    auto* req = static_cast<request_op*>(opaque);
    if(!req) {
        return;
    }
    req->mark_removed();
}

void complete_request_operation(void* opaque,
                                curl::easy_error result,
                                bool resume_inline) noexcept {
    auto* req = static_cast<request_op*>(opaque);
    if(!req) {
        return;
    }

    if(resume_inline) {
        req->finish_inline(result);
        return;
    }

    req->finish(result);
}

task<response, error> execute_with_state(request req,
                                         event_loop& loop,
                                         std::shared_ptr<client_state> owner) {
    detail::prepared_request prepared(std::move(req), owner);
    request_op op(manager::for_loop(loop));
    op.attach(prepared);
    if(!prepared.bind_runtime(&op) && prepared.result.kind == error_kind::curl &&
       curl::ok(prepared.result.curl_code)) {
        prepared.result = error::invalid_request("failed to bind request to runtime");
    }

    op.start();
    auto result = co_await op;

    if(result.is_cancelled()) {
        co_await cancel();
    }

    if(result.has_error()) {
        co_await fail(std::move(result).error());
    }

    co_return std::move(*result);
}

}  // namespace detail

}  // namespace kota::http
