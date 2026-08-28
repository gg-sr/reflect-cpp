#ifndef RFL_INTERNAL_ENUMS_GET_ENUM_NAMES_HPP_
#define RFL_INTERNAL_ENUMS_GET_ENUM_NAMES_HPP_

#include <type_traits>

#include "Names.hpp"

#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/get_enum_names.hpp"
#else
#include "cpp26/get_enum_names.hpp"
#endif

namespace rfl::internal::enums {

template <class EnumType>
consteval auto get_enum_names() {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
  return cpp20::get_enum_names<EnumType>();
#else
  return cpp26::get_enum_names<EnumType>();
#endif
}

/// Same as `get_enum_names`, but with the enumerators renamed by `Transform`,
/// which is the processor-provided transformation obtained through
/// `enum_name_transform_t`, or `void` for no renaming.
template <class EnumType, class Transform>
consteval auto get_transformed_enum_names() {
  return transform_enum_names<Transform>(get_enum_names<EnumType>());
}

}  // namespace rfl::internal::enums

#endif
