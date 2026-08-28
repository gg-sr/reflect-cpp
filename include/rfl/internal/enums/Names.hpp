#ifndef RFL_INTERNAL_ENUMS_NAMES_HPP_
#define RFL_INTERNAL_ENUMS_NAMES_HPP_

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../../Literal.hpp"
#include "../../Processors.hpp"
#include "../../make_named_tuple.hpp"
#include "../StringLiteral.hpp"

namespace rfl::internal::enums {

template <class EnumType, class LiteralType, size_t N, bool _is_flag,
          auto... _enums>
struct Names {
  /// Contains a collection of enums as compile-time strings.
  using Literal = LiteralType;

  /// The number of possible values
  constexpr static size_t size = N;

  /// A list of all the possible enums
  constexpr static auto enums_ = std::array<EnumType, N>{_enums...};
};

template <class EnumType, size_t N, bool _is_flag, StringLiteral... _names,
          auto... _enums>
auto names_to_enumerator_named_tuple(
    Names<EnumType, Literal<_names...>, N, _is_flag, _enums...>) {
  return make_named_tuple(Field<_names, EnumType>{_enums}...);
}

template <class EnumType, size_t N, bool _is_flag, StringLiteral... _names,
          auto... _enums>
auto names_to_underlying_enumerator_named_tuple(
    Names<EnumType, Literal<_names...>, N, _is_flag, _enums...>) {
  return make_named_tuple(Field<_names, std::underlying_type_t<EnumType>>{
      static_cast<std::underlying_type_t<EnumType>>(_enums)}...);
}

template <class EnumType, size_t N, bool _is_flag, StringLiteral... _names,
          auto... _enums>
constexpr std::array<std::pair<std::string_view, EnumType>, N>
names_to_enumerator_array(
    Names<EnumType, Literal<_names...>, N, _is_flag, _enums...>) {
  return {
      std::make_pair(LiteralHelper<_names>::name_.string_view(), _enums)...};
}

template <class EnumType, size_t N, bool _is_flag, StringLiteral... _names,
          auto... _enums>
constexpr std::array<
    std::pair<std::string_view, std::underlying_type_t<EnumType>>, N>
names_to_underlying_enumerator_array(
    Names<EnumType, Literal<_names...>, N, _is_flag, _enums...>) {
  return {
      std::make_pair(LiteralHelper<_names>::name_.string_view(),
                     static_cast<std::underlying_type_t<EnumType>>(_enums))...};
}

/// Satisfied by processors that rename enumerators. Such a processor renames
/// every enumerator of every enum it is applied to, the way the case-transform
/// processors rename every field.
///
/// This names `transform_enum_name` rather than calling it, on purpose. A
/// processor that fails this concept is silently left out, which is
/// indistinguishable from "no processor renames enumerators", so the concept
/// should ask for as little as possible and let a malformed hook fail loudly
/// at the call in `transform_enum_names` instead. Adding the `()` back would
/// turn a hook that takes an argument, or a private one, back into a silent
/// no-op.
template <class T>
concept HasEnumNameTransform =
    requires { T::template transform_enum_name<StringLiteral<2>("a")>; };

/// Yields the first processor in `P...` that renames enumerators, or `void`
/// if there is none, flattening `Processors<>`.
template <class... P>
struct enum_name_transform {
  using type = void;
};

template <HasEnumNameTransform T, class... Rest>
struct enum_name_transform<T, Rest...> {
  using type = T;
};

template <class T, class... Rest>
struct enum_name_transform<T, Rest...> : enum_name_transform<Rest...> {};

template <class... P, class... Rest>
struct enum_name_transform<Processors<P...>, Rest...> : enum_name_transform<P..., Rest...> {};

template <class T>
using enum_name_transform_t = typename enum_name_transform<
    std::remove_cvref_t<std::remove_pointer_t<T>>>::type;

/// Renames every enumerator in `Names` using `Transform`, or returns them
/// unchanged when `Transform` is `void`, which is the sentinel for "no
/// processor renames enumerators".
template <class Transform, class EnumType, size_t N, bool _is_flag,
          StringLiteral... _names, auto... _enums>
consteval auto transform_enum_names(
    Names<EnumType, Literal<_names...>, N, _is_flag, _enums...> _names_obj) {
  if constexpr (std::is_void_v<Transform>) {
    return _names_obj;
  } else {
    return Names<EnumType,
                 Literal<Transform::template transform_enum_name<_names>()...>,
                 N, _is_flag, _enums...>{};
  }
}

}  // namespace rfl::internal::enums

#endif
