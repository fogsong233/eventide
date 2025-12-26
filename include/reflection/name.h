#pragma once

#include <source_location>
#include <string_view>

#include "traits.h"

namespace refl {

template <typename T>
consteval auto type_name(bool qualified = false) {
    std::string_view name = std::source_location::current().function_name();
#if __GNUC__ || __clang__
    std::size_t start = name.rfind("T =") + 3;
    if(name[start] == ' ') {
        start += 1;
    }
    std::size_t end = name.find_first_of(";]", start);
    if(end == std::string_view::npos) {
        end = name.size();
    }
    if(name[end - 1] == ' ') {
        end -= 1;
    }
    name = std::string_view(name.data() + start, end - start);
#elif _MSC_VER
    std::size_t start = name.find('<') + 1;
    std::size_t end = name.rfind(">(");
    name = std::string_view{name.data() + start, end - start};
    start = name.find(' ');
    name = start == std::string_view::npos
               ? name
               : std::string_view(name.data() + start + 1, name.size() - start - 1);
#endif
    if(!qualified) {
        auto pos = name.rfind("::");
        if(pos != std::string_view::npos) {
            name = name.substr(pos + 2);
        }
    }
    return name;
}

template <auto*>
consteval auto field_name() {
    std::string_view name = std::source_location::current().function_name();
#if __GNUC__ || __clang__ && (!_MSC_VER)
    std::size_t start = name.rfind("::") + 2;
    std::size_t end = name.rfind(')');
    name = name.substr(start, end - start);
#elif _MSC_VER
    std::size_t start = name.rfind("->") + 2;
    std::size_t end = name.rfind('}');
    name = name.substr(start, end - start);
#else
#error "UNKNOWN COMPILER";
#endif
    if(name.rfind("::") != std::string_view::npos) {
        name = name.substr(name.rfind("::") + 2);
    }
    return name;
}

}  // namespace refl
