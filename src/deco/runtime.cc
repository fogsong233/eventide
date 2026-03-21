#include <utility>

#include "eventide/deco/deco.h"

namespace deco::util {

std::vector<std::string> argvify(int argc, const char* const* argv, unsigned skip_num) {
    std::vector<std::string> res;
    for(unsigned i = skip_num; i < static_cast<unsigned>(argc); ++i) {
        res.emplace_back(argv[i]);
    }
    return res;
}

}  // namespace deco::util

namespace deco::cli {

auto SubCommander::command_of(const decl::SubCommand& subcommand) -> std::string {
    if(subcommand.command.has_value()) {
        return std::string(*subcommand.command);
    }
    return std::string(subcommand.name);
}

auto SubCommander::display_name_of(const decl::SubCommand& subcommand, std::string_view command)
    -> std::string {
    if(!subcommand.name.empty()) {
        return std::string(subcommand.name);
    }
    return std::string(command);
}

SubCommander::SubCommander(std::string_view command_overview, std::string_view overview) :
    commandOverview(command_overview), overview(overview) {}

auto SubCommander::add(const decl::SubCommand& subcommand, SubCommander::handler_fn_t handler)
    -> SubCommander& {
    std::string command = command_of(subcommand);
    if(command.empty()) {
        errorHandler(
            {SubCommandError::Type::Internal, "subcommand name/command must not be empty"});
        return *this;
    }

    std::string name = display_name_of(subcommand, command);
    std::string description(subcommand.description);

    if(auto it = commandToHandler.find(command); it != commandToHandler.end()) {
        auto& target = handlers[it->second];
        target.name = std::move(name);
        target.description = std::move(description);
        target.command = std::move(command);
        target.handler = std::move(handler);
        return *this;
    }

    commandToHandler[command] = handlers.size();
    handlers.push_back({
        .name = std::move(name),
        .description = std::move(description),
        .command = std::move(command),
        .handler = std::move(handler),
    });
    return *this;
}

auto SubCommander::add(SubCommander::handler_fn_t default_handler) -> SubCommander& {
    defaultHandler = std::move(default_handler);
    return *this;
}

auto SubCommander::when_err(SubCommander::error_fn_t error_handler) -> SubCommander& {
    errorHandler = std::move(error_handler);
    return *this;
}

auto SubCommander::when_err(std::ostream& os) -> SubCommander& {
    errorHandler = [&os](const SubCommandError& err) {
        os << err.message << "\n";
    };
    return *this;
}

void SubCommander::usage(std::ostream& os) const {
    if(!overview.empty()) {
        os << overview << "\n\n";
    }

    if(defaultHandler.has_value()) {
        os << "usage: " << commandOverview << "\n";
        if(!handlers.empty()) {
            os << "\n";
        }
    }

    if(handlers.empty()) {
        return;
    }

    std::size_t max_name_len = 0;
    for(const auto& item: handlers) {
        if(item.name.size() > max_name_len) {
            max_name_len = item.name.size();
        }
    }

    os << "Subcommands:\n";
    for(const auto& item: handlers) {
        os << "  " << item.name;
        if(!item.description.empty()) {
            os << std::string(max_name_len - item.name.size() + 2, ' ') << item.description;
        }
        if(item.command != item.name) {
            os << " (" << item.command << ")";
        }
        os << "\n";
    }
}

auto SubCommander::match(std::span<std::string> argv) const
    -> std::expected<SubCommander::match_t, SubCommandError> {
    if(!argv.empty()) {
        if(auto it = commandToHandler.find(argv.front()); it != commandToHandler.end()) {
            const auto& handler = handlers[it->second];
            return match_t{
                .kind = match_t::Kind::Command,
                .original_argv = argv,
                .remaining_argv = argv.subspan(1),
                .token = argv.front(),
                .name = handler.name,
                .command = handler.command,
            };
        }
    }

    if(defaultHandler.has_value()) {
        return match_t{
            .kind = match_t::Kind::Default,
            .original_argv = argv,
            .remaining_argv = argv,
            .token = argv.empty() ? std::string_view{} : std::string_view(argv.front()),
        };
    }

    if(argv.empty()) {
        return std::unexpected(
            SubCommandError{SubCommandError::Type::MissingSubCommand, "subcommand is required"});
    }

    return std::unexpected(SubCommandError{
        SubCommandError::Type::UnknownSubCommand,
        std::format("unknown subcommand '{}'", argv.front()),
    });
}

void SubCommander::parse(std::span<std::string> argv) {
    auto matched = match(argv);
    if(!matched.has_value()) {
        errorHandler(std::move(matched.error()));
        return;
    }

    if(matched->is_command()) {
        if(auto it = commandToHandler.find(matched->command); it != commandToHandler.end()) {
            handlers[it->second].handler(std::move(*matched));
            return;
        }
        errorHandler({SubCommandError::Type::Internal,
                      std::format("missing handler for subcommand '{}'", matched->command)});
        return;
    }

    if(defaultHandler.has_value()) {
        (*defaultHandler)(std::move(*matched));
        return;
    }

    errorHandler({SubCommandError::Type::Internal, "default route resolved without handler"});
}

void SubCommander::operator()(std::span<std::string> argv) {
    parse(argv);
}

}  // namespace deco::cli
