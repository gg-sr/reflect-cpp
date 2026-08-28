#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <type_traits>

namespace test_enum_name_transform {

enum class Color { Red, DarkGreen, Blue };

enum class Perm { CanRead = 1, CanWrite = 2, CanExec = 4 };

inline Perm operator|(Perm a, Perm b) noexcept {
  return static_cast<Perm>(static_cast<int>(a) | static_cast<int>(b));
}

struct Circle {
  float radius;
  Color color;
};

struct File {
  int id;
  Perm perm;
};

/// A processor that renames every enumerator from CamelCase to snake_case,
/// leaving the fields alone.
struct SnakeCaseEnums {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return std::forward<decltype(_named_tuple)>(_named_tuple);
  }

  template <rfl::internal::StringLiteral _name>
  static consteval auto transform_enum_name() {
    return rfl::internal::transform_camel_case<_name>();
  }
};

// A mis-detected transform is invisible at runtime - it looks exactly like
// "the processor did nothing" - so pin the trait itself.
static_assert(rfl::internal::enums::HasEnumNameTransform<SnakeCaseEnums>);
static_assert(std::is_same_v<
              rfl::internal::enums::enum_name_transform_t<SnakeCaseEnums>,
              SnakeCaseEnums>);
static_assert(
    std::is_same_v<rfl::internal::enums::enum_name_transform_t<
                       rfl::Processors<SnakeCaseEnums>>,
                   SnakeCaseEnums>);
static_assert(std::is_same_v<
              rfl::internal::enums::enum_name_transform_t<
                  rfl::Processors<rfl::Processors<SnakeCaseEnums>>>,
              SnakeCaseEnums>);
// A nested pack is not special to the head position: the trait peels one
// element at a time, so every argument takes its turn as the head.
static_assert(
    std::is_same_v<rfl::internal::enums::enum_name_transform_t<rfl::Processors<
                       rfl::SnakeCaseToCamelCase, rfl::Processors<SnakeCaseEnums>>>,
                   SnakeCaseEnums>);
static_assert(
    std::is_same_v<rfl::internal::enums::enum_name_transform_t<rfl::Processors<
                       rfl::Processors<rfl::SnakeCaseToCamelCase>, SnakeCaseEnums>>,
                   SnakeCaseEnums>);
static_assert(
    std::is_void_v<rfl::internal::enums::enum_name_transform_t<
        rfl::Processors<rfl::SnakeCaseToCamelCase>>>);
static_assert(
    std::is_void_v<rfl::internal::enums::enum_name_transform_t<
        rfl::Processors<rfl::Processors<rfl::SnakeCaseToCamelCase>>>>);

// A hook with the wrong shape must still be *detected*, so that it fails
// loudly where it is called rather than being silently skipped -- a skipped
// processor is indistinguishable from one that renames nothing. These are
// detected but deliberately never applied to an enum, which would not compile.
namespace malformed {
struct TakesAnArgument {
  template <class StructType>
  static auto process(auto&& _nt) { return std::forward<decltype(_nt)>(_nt); }
  template <rfl::internal::StringLiteral _name>
  static consteval auto transform_enum_name(int) { return _name; }
};
struct Inaccessible {
  template <class StructType>
  static auto process(auto&& _nt) { return std::forward<decltype(_nt)>(_nt); }

 private:
  template <rfl::internal::StringLiteral _name>
  static consteval auto transform_enum_name() { return _name; }
};
static_assert(rfl::internal::enums::HasEnumNameTransform<TakesAnArgument>);
static_assert(rfl::internal::enums::HasEnumNameTransform<Inaccessible>);
}  // namespace malformed

TEST(json, test_enum_name_transform_write) {
  EXPECT_EQ(rfl::json::write<SnakeCaseEnums>(Circle{2.0, Color::DarkGreen}),
            R"({"radius":2.0,"color":"dark_green"})");
}

TEST(json, test_enum_name_transform_read) {
  const auto res = rfl::json::read<Circle, SnakeCaseEnums>(
      R"({"radius":2.0,"color":"dark_green"})");
  ASSERT_TRUE(res && true) << "Test failed on read. Error: "
                           << res.error().what();
  EXPECT_EQ(res.value().color, Color::DarkGreen);
}

TEST(json, test_enum_name_transform_rejects_the_original_spelling) {
  // The schema advertises the transformed names, so the parser must reject the
  // C++ spelling. This is what catches a schema/parser mismatch.
  const auto res = rfl::json::read<Circle, SnakeCaseEnums>(
      R"({"radius":2.0,"color":"DarkGreen"})");
  EXPECT_FALSE(res && true);
}

TEST(json, test_enum_name_transform_in_schema) {
  const auto schema = rfl::json::to_schema<Circle, SnakeCaseEnums>();
  EXPECT_NE(schema.find(R"(["red","dark_green","blue"])"), std::string::npos)
      << schema;
}

TEST(json, test_enum_name_transform_flag_enum) {
  // Flag enums join the per-bit names with '|', so the per-bit lookup has to
  // use the transform too.
  EXPECT_EQ(rfl::json::write<SnakeCaseEnums>(File{1, Perm::CanRead | Perm::CanExec}),
            R"({"id":1,"perm":"can_read|can_exec"})");
  const auto res = rfl::json::read<File, SnakeCaseEnums>(
      R"({"id":1,"perm":"can_read|can_exec"})");
  ASSERT_TRUE(res && true) << "Test failed on read. Error: "
                           << res.error().what();
  EXPECT_EQ(res.value().perm, Perm::CanRead | Perm::CanExec);
}

// Regression: a processor nested inside a `Processors` pack must still be
// found. The entry points wrap their arguments unconditionally, so
// `write<Processors<P>>` reaches the trait as `Processors<Processors<P>>`.
TEST(json, test_enum_name_transform_nested_in_processors) {
  EXPECT_EQ(rfl::json::write<rfl::Processors<SnakeCaseEnums>>(
                Circle{2.0, Color::DarkGreen}),
            R"({"radius":2.0,"color":"dark_green"})");
  const auto res = rfl::json::read<Circle, rfl::Processors<SnakeCaseEnums>>(
      R"({"radius":2.0,"color":"dark_green"})");
  ASSERT_TRUE(res && true) << "Test failed on read. Error: "
                           << res.error().what();
  EXPECT_EQ(res.value().color, Color::DarkGreen);
}

TEST(json, test_enum_name_transform_alongside_a_field_processor) {
  const auto json =
      rfl::json::write<rfl::SnakeCaseToCamelCase, SnakeCaseEnums>(
          Circle{2.0, Color::DarkGreen});
  EXPECT_EQ(json, R"({"radius":2.0,"color":"dark_green"})");
}

TEST(json, test_enum_names_unchanged_without_the_processor) {
  EXPECT_EQ(rfl::json::write(Circle{2.0, Color::DarkGreen}),
            R"({"radius":2.0,"color":"DarkGreen"})");
  const auto schema = rfl::json::to_schema<Circle>();
  EXPECT_NE(schema.find(R"(["Red","DarkGreen","Blue"])"), std::string::npos)
      << schema;
}

}  // namespace test_enum_name_transform
