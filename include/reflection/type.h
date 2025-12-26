#pragma once

#include <string_view>

namespace refl {

template <typename T>
consteval std::string_view type_name() {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view name = __PRETTY_FUNCTION__;
    constexpr std::string_view key = "T =";
    auto start = name.find(key);
    if(start == std::string_view::npos) {
        return "UNKNOWN_TYPE";
    }
    start += key.size();
    if(start < name.size() && name[start] == ' ') {
        ++start;
    }
    auto end = name.find_first_of(";]", start);
    if(end == std::string_view::npos) {
        end = name.size();
    }
    if(end > start && name[end - 1] == ' ') {
        --end;
    }
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    constexpr std::string_view name = __FUNCSIG__;
    constexpr std::string_view key = "type_name<";
    auto start = name.find(key);
    if(start == std::string_view::npos) {
        return "UNKNOWN_TYPE";
    }
    start += key.size();
    auto end = name.find('>', start);
    if(end == std::string_view::npos) {
        end = name.size();
    }
    auto value = name.substr(start, end - start);
    constexpr std::string_view class_prefix = "class ";
    constexpr std::string_view struct_prefix = "struct ";
    constexpr std::string_view union_prefix = "union ";
    constexpr std::string_view enum_prefix = "enum ";
    if(value.starts_with(class_prefix)) {
        value.remove_prefix(class_prefix.size());
    } else if(value.starts_with(struct_prefix)) {
        value.remove_prefix(struct_prefix.size());
    } else if(value.starts_with(union_prefix)) {
        value.remove_prefix(union_prefix.size());
    } else if(value.starts_with(enum_prefix)) {
        value.remove_prefix(enum_prefix.size());
    }
    return value;
#else
    return "UNKNOWN_TYPE";
#endif
}

}  // namespace refl
