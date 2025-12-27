#pragma once

#include <concepts>
#include <type_traits>

namespace refl::traits {

template <typename T>
concept aggregate_type = std::is_aggregate_v<T>;

template <typename T>
concept enum_type = std::is_enum_v<T>;

}  // namespace refl::traits
