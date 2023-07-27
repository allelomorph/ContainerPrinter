#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "container_stream_io.hh"

#include <algorithm>
#include <functional>

#include <array>
#include <vector>
#include <tuple>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <stack>
#include <queue>

#include "TypeName.hh"  // testing

#if (__cplusplus < 201103L)
#error "csio_unit_tests.cpp only supports C++11 and above"
#endif  // pre-C++11

namespace
{

/**
 * @brief An RAII wrapper that will execute an action when the wrapper falls out of scope, or is
 * otherwise destroyed.
 */
template <typename LambdaType>
class scope_exit
{
public:
    scope_exit(LambdaType&& lambda) noexcept : m_lambda{ std::move(lambda) }
    {
        static_assert(
#if __cplusplus >= 201703L
            std::is_nothrow_invocable<LambdaType>::value,
#else
            noexcept(lambda),
#endif
            "Since the callable type is invoked from the destructor, exceptions are not allowed.");
    }

    ~scope_exit() noexcept
    {
        m_lambda();
    }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    scope_exit(scope_exit&&) = default;
    scope_exit& operator=(scope_exit&&) = default;

private:
    LambdaType m_lambda;
};

template <typename Type>
class vector_wrapper : public std::vector<Type>
{};

template <typename Type>
class stack_wrapper : public std::stack<Type>
{};

/**
 * @brief Custom formatting struct.
 */
struct custom_formatter
{
    template <typename StreamType> void print_prefix(StreamType& stream) const noexcept
    {
        stream << L"$$ ";
    }

    template <typename StreamType, typename ElementType>
    void print_element(StreamType& stream, const ElementType& element) const noexcept
    {
        stream << element;
    }

    template <typename StreamType> void print_separator(StreamType& stream) const noexcept
    {
        stream << L" | ";
    }

    template <typename StreamType> void print_suffix(StreamType& stream) const noexcept
    {
        stream << L" $$";
    }
};

template <typename CStrTypeA, typename CStrTypeB>
auto idiomatic_strcmp(const CStrTypeA s1, const CStrTypeB s2
    ) -> std::enable_if_t<
        std::is_pointer<typename std::decay<CStrTypeA>::type>::value &&
        std::is_pointer<typename std::decay<CStrTypeB>::type>::value,
        bool>
{
    using char_type = typename std::remove_const<
        typename std::pointer_traits<
        typename std::decay<CStrTypeB>::type >::element_type >::type;
#if __cplusplus >= 201703L
    return s1 == std::basic_string_view<char_type>(s2);
#else
    return s1 == std::basic_string<char_type>(s2);
#endif
}

template<typename CharType>
void reset_ostringstream(std::basic_ostringstream<CharType>& oss)
{
    CharType s[1] { 0 };  // char type-agnostic ""
    oss.str(s);
    oss.clear();
}

template<typename CharType>
void reset_istringstream(std::basic_istringstream<CharType>& iss)
{
    CharType s[1] { 0 };  // char type-agnostic ""
    iss.str(s);
    iss.clear();
}

} // namespace

TEST_CASE("Traits: detect parseable (input stream extractable) container types",
          "[traits],[input]")
{
    using namespace container_stream_io;

    SECTION("Detect most STL containers as being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::array<int, 5>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::vector<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::pair<int, double>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::tuple<int, double, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::deque<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::forward_list<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::list<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::set<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::multiset<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::map<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::multimap<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_set<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_multiset<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_map<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_multimap<int, float>>::value == true);
    }

    SECTION("Detect C-like array of non-char type as being a parseable container type")
    {
        REQUIRE(traits::is_parseable_as_container<int[5]>::value == true);
    }

    SECTION("Detect inherited parseable container type.")
    {
        REQUIRE(traits::is_parseable_as_container<vector_wrapper<int>>::value == true);
    }
}

TEST_CASE("Traits: detect types not parseable (input stream extractable) as containers",
          "[traits],[input]")
{
    using namespace container_stream_io;

    SECTION("Detect C-like array of char type as not being a parseable container type")
    {
        REQUIRE(traits::is_parseable_as_container<char[5]>::value == false);
    }

    SECTION("Detect STL strings as not being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_parseable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("Detect STL stacks and single-ended queues as not being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("Detect inherited unparseable container type.")
    {
        REQUIRE(traits::is_parseable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect printable (output stream insertable) container types",
          "[traits],[output]")
{
    using namespace container_stream_io;

    SECTION("Detect most STL containers as being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::array<int, 5>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::vector<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::pair<int, double>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::tuple<int, double, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::deque<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::forward_list<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::list<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::set<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::multiset<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::map<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::multimap<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_set<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_multiset<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_map<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_multimap<int, float>>::value == true);
    }

    SECTION("Detect C-like array of non-char type as being a printable container type")
    {
        REQUIRE(traits::is_printable_as_container<int[5]>::value == true);
    }

    SECTION("Detect inherited printable container type.")
    {
        REQUIRE(traits::is_printable_as_container<vector_wrapper<int>>::value == true);
    }
}

TEST_CASE("Traits: detect types not printable (output stream insertable) as containers",
          "[traits],[output]")
{
    using namespace container_stream_io;

    SECTION("Detect C-like array of char type as not being a printable container type")
    {
        REQUIRE(traits::is_printable_as_container<char[5]>::value == false);
    }

    SECTION("Detect STL strings as not being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_printable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("Detect STL stacks and single-ended queues as not being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("Detect inherited unprintable container type")
    {
        REQUIRE(traits::is_printable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect char types", "[traits]")
{
    using namespace container_stream_io;

    REQUIRE(traits::is_char_variant<int>::value == false);
    REQUIRE(traits::is_char_variant<std::vector<int>>::value == false);

    REQUIRE(traits::is_char_variant<char>::value == true);
    REQUIRE(traits::is_char_variant<wchar_t>::value == true);
#if __cplusplus > 201703L
    REQUIRE(traits::is_char_variant<char8_t>::value == true);
#endif
    REQUIRE(traits::is_char_variant<char16_t>::value == true);
    REQUIRE(traits::is_char_variant<char32_t>::value == true);
}

TEST_CASE("Traits: detect string types", "[traits]")
{
    using namespace container_stream_io;

    SECTION("Detect non-char types and C arrays as not being string types")
    {
        REQUIRE(traits::is_string_variant<int>::value == false);
        REQUIRE(traits::is_string_variant<int[5]>::value == false);
    }

    SECTION("Fail to detect char type C arrays as strings (expecting their decay to pointers)")
    {
        REQUIRE(traits::is_string_variant<char[5]>::value == false);
        REQUIRE(traits::is_string_variant<const char[5]>::value == false);
    }

    SECTION("Detect char pointers and STL strings as strings")
    {
        REQUIRE(traits::is_string_variant<char*>::value == true);
        REQUIRE(traits::is_string_variant<const char*>::value == true);
        REQUIRE(traits::is_string_variant<std::basic_string<char>>::value == true);
        REQUIRE(traits::is_string_variant<const std::basic_string<char>>::value == true);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_string_variant<std::basic_string_view<char>>::value == true);
        REQUIRE(traits::is_string_variant<const std::basic_string_view<char>>::value == true);
#endif
    }
}

TEST_CASE("Traits: detect emplace methods", "[traits]")
{
    using namespace container_stream_io;

    REQUIRE(traits::has_iterless_emplace<std::vector<int>>::value == false);
    REQUIRE(traits::has_emplace_back<std::vector<int>>::value == true);

    REQUIRE(traits::has_iterless_emplace<std::set<int>>::value == true);
    REQUIRE(traits::has_emplace_back<std::set<int>>::value == false);

    REQUIRE(traits::has_iterless_emplace<int>::value == false);
    REQUIRE(traits::has_emplace_back<int>::value == false);
}

TEST_CASE("Strings: printing char/string types as escaped literals",
          "[literal],[strings],[output]")
{
    using namespace container_stream_io;

    std::ostringstream oss;
    std::wostringstream woss;

    SECTION("Char/string types outside supported containers can be printed as "
            "literals using literal()")
    {
        oss << '\t';
        REQUIRE(oss.str() == "\t");

        reset_ostringstream(oss);
        oss << strings::literal('\t');
        REQUIRE(oss.str() == "'\\t'");

        reset_ostringstream(oss);
        oss << "tes\t";
        REQUIRE(oss.str() == "tes\t");

        reset_ostringstream(oss);
        oss << strings::literal("tes\t");
        REQUIRE(oss.str() == "\"tes\\t\"");

        std::string s { "tes\t" };
        reset_ostringstream(oss);
        oss << s;
        REQUIRE(oss.str() == "tes\t");

        reset_ostringstream(oss);
        oss << strings::literal(s);
        REQUIRE(oss.str() == "\"tes\\t\"");

#if (__cplusplus >= 201703L)
        std::string_view sv { "tes\t" };
        reset_ostringstream(oss);
        oss << sv;
        REQUIRE(oss.str() == "tes\t");

        reset_ostringstream(oss);
        oss << strings::literal(sv);
        REQUIRE(oss.str() == "\"tes\\t\"");
#endif
    }

    SECTION("Char/string types inside supported containers printed as literals by default")
    {
        std::vector<char> cv { 't', 'e', 's', '\t', '\0' };
        std::vector<const char*> cpv { "tes\t" };
        std::vector<std::string> sv { { "tes\t" } };
        oss << cv;
        REQUIRE(oss.str() == "['t', 'e', 's', '\\t', '\\0']");

        reset_ostringstream(oss);
        oss << cpv;
        REQUIRE(oss.str() == "[\"tes\\t\"]");

        reset_ostringstream(oss);
        oss << sv;
        REQUIRE(oss.str() == "[\"tes\\t\"]");

#if (__cplusplus >= 201703L)
        std::vector<std::string_view> svv { { "tes\t" } };

        reset_ostringstream(oss);
        oss << svv;
        REQUIRE(oss.str() == "[\"tes\\t\"]");
#endif
    }

    SECTION("Default behavior of printing char/string types inside containers "
        " can be set with I/O manipulator literalrepr")
    {
        std::vector<char> cv { 't', 'e', 's', '\t', '\x7f' };
        std::vector<const char*> cpv { "tes\t" };

        oss << strings::quotedrepr;
        oss << cv;
        REQUIRE(oss.str() == "['t', 'e', 's', '\t', '\x7f']");

        reset_ostringstream(oss);
        oss << cpv;
        REQUIRE(oss.str() == "[\"tes\t\"]");

        oss << strings::literalrepr;
        reset_ostringstream(oss);
        oss << cv;
        REQUIRE(oss.str() == "['t', 'e', 's', '\\t', '\\x7f']");

        reset_ostringstream(oss);
        oss << cpv;
        REQUIRE(oss.str() == "[\"tes\\t\"]");
    }

    SECTION("With different stream and element char types, literal representation "
            "hex escapes all chars")
    {
        std::string s { "test" };
        std::wstring ws { L"test" };
        std::vector<std::string> sv { { "test" } };
        std::vector<std::wstring> wsv { { L"test" } };

        oss << strings::literal(ws);
        REQUIRE(oss.str() == "L\"\\x00000074\\x00000065\\x00000073\\x00000074\"");

        reset_ostringstream(oss);
        oss << wsv;
        REQUIRE(oss.str() == "[L\"\\x00000074\\x00000065\\x00000073\\x00000074\"]");

        woss << strings::literal(s);
        REQUIRE(woss.str() == L"\"\\x74\\x65\\x73\\x74\"");

        reset_ostringstream(woss);
        woss << sv;
        REQUIRE(woss.str() == L"[\"\\x74\\x65\\x73\\x74\"]");
    }

    SECTION("With same stream and element char types, literal representation "
            "escapes the delimiter, escape, and standard ASCII-7 escaped chars, "
            " hex escaping all other unprintable ASCII-7")
    {
        std::string s { "t\\\"\t\x7f" };
        std::wstring ws { L"t\\\"\t\x7f" };
        std::vector<std::string> sv { { "t\\\"\t\x7f" } };
        std::vector<std::wstring> wsv { { L"t\\\"\t\x7f" } };

        oss << strings::literal(ws);
        REQUIRE(oss.str() == "L\"\\x00000074\\x0000005c\\x00000022\\x00000009\\x0000007f\"");

        reset_ostringstream(oss);
        oss << wsv;
        REQUIRE(oss.str() == "[L\"\\x00000074\\x0000005c\\x00000022\\x00000009\\x0000007f\"]");

        woss << strings::literal(s);
        REQUIRE(woss.str() == L"\"\\x74\\x5c\\x22\\x09\\x7f\"");

        reset_ostringstream(woss);
        woss << sv;
        REQUIRE(woss.str() == L"[\"\\x74\\x5c\\x22\\x09\\x7f\"]");
    }
}

TEST_CASE("Strings: parsing chars/STL strings as escaped literals",
          "[literal],[strings],[input]")
{
    SECTION("Chars and STL strings outside containers can be parsed as "
            "literals using literal()")
    {
    }

    SECTION("Chars and STL strings inside containers parsed as literals by default")
    {
    }

    SECTION("Default behavior of parsing chars/STL strings inside containers "
        " can be set with I/O manipulator literalrepr")
    {
    }

    SECTION("With different stream and element char types, literal representation "
            "hex escapes all chars")
    {
    }

    SECTION("With same stream and element char types, literal representation "
            "escapes the delimiter, escape, and standard ASCII-7 escaped chars, "
            " hex escaping all other unprintable ASCII-7")
    {
    }
}

TEST_CASE("Strings: printing char/string types as quoted strings",
          "[quoted],[strings],[output]")
{
    SECTION("Char/string types outside containers can be printed as quoted using quoted()")
    {
    }

    SECTION("Char/string types inside containers are not printed as quoted by default")
    {
    }

    SECTION("Default behavior of printing char/string types inside containers "
        " can be set with I/O manipulator quotedrepr")
    {
    }

    SECTION("With different stream and element char types, quoted representation "
            "hex escapes all chars")
    {
    }

    SECTION("With same stream and element char types, quoted representation "
            "escapes only the delimiter and escape chars, inserting the rest directly")
    {
    }

#if (__cplusplus >= 201402L)
    SECTION("With same stream and element char types, quoted representation "
            "should match std::quoted")
    {
    }
#endif
}

TEST_CASE("Strings: parsing chars/STL strings as quoted strings",
          "[quoted],[strings],[input]")
{
    SECTION("Chars/STL strings outside containers can be parsed as quoted using quoted()")
    {
    }

    SECTION("Chars/STL strings inside containers not parsed as quoted by default")
    {
    }

    SECTION("Default behavior of parsing chars/STL strings inside containers "
        " can be set with I/O manipulator quotedrepr")
    {
    }

    SECTION("With different stream and element char types, quoted representation "
            "hex escapes all chars")
    {
    }

    SECTION("With same stream and element char types, quoted representation "
            "escapes only the delimiter and escape chars, extracting the rest directly")
    {
    }

#if (__cplusplus >= 201402L)
    SECTION("With same stream and element char types, quoted representation "
            "should match std::quoted")
    {
    }
#endif
}

TEST_CASE("Delimiters: validate char defaults", "[decorator]")
{
    SECTION("Verify char delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "]" ));
    }

    SECTION("Verify char delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "}" ));
    }

    SECTION("Verify char delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ")" ));
    }

    SECTION("Verify char delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ">" ));
    }
}

TEST_CASE("Delimiters: validate wchar_t defaults", "[decorator]")
{
    SECTION("Verify wchar_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"]" ));
    }

    SECTION("Verify wchar_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"}" ));
    }

    SECTION("Verify wchar_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L")" ));
    }

    SECTION("Verify wchar_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L">" ));
    }
}

#if __cplusplus > 201703L
TEST_CASE("Delimiters: validate char8_t defaults", "[decorator]")
{
    SECTION("Verify char8_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"]" ));
    }

    SECTION("Verify char8_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"}" ));
    }

    SECTION("Verify char8_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8")" ));
    }

    SECTION("Verify char8_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8">" ));
    }
}
#endif  // above C++17

TEST_CASE("Delimiters: validate char16_t defaults", "[decorator]")
{
    SECTION("Verify char16_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"]" ));
    }

    SECTION("Verify char16_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"}" ));
    }

    SECTION("Verify char16_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u")" ));
    }

    SECTION("Verify char16_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u">" ));
    }
}

TEST_CASE("Delimiters: validate char32_t defaults", "[decorator]")
{
    SECTION("Verify char32_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"]" ));
    }

    SECTION("Verify char32_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"}" ));
    }

    SECTION("Verify char32_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U")" ));
    }

    SECTION("Verify char32_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U">" ));
    }
}

TEST_CASE("Printing of Raw Arrays")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a narrow character array.")
    {
        const auto& array { "Hello" };
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == "Hello");
    }

    SECTION("Printing a wide character array.")
    {
        const auto& array { L"Hello" };
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == L"Hello");
    }

    SECTION("Printing an array of ints to a narrow stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == "[1, 2, 3, 4, 5]");
    }

    SECTION("Printing an array of ints to a wide stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == L"[1, 2, 3, 4, 5]");
    }
}


TEST_CASE("Printing of Standard Library Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a std::pair<...> to a narrow stream.")
    {
        const auto pair { std::make_pair(10, 100) };
        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == "(10, 100)");
    }

    SECTION("Printing a std::pair<...> to a wide stream.")
    {
        const auto pair { std::make_pair(10, 100) };
        std::wcout << pair << std::flush;

        REQUIRE(wide_buffer.str() == L"(10, 100)");
    }

    SECTION("Printing an empty std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector{};
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[]");
    }

    SECTION("Printing a populated std::vector<...> to a narrow stream.")
    {
        const std::vector<int> vector { 1, 2, 3, 4 };
        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == "[1, 2, 3, 4]");
    }

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector { 1, 2, 3, 4 };
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[1, 2, 3, 4]");
    }

    SECTION("Printing an empty std::set<...> to a narrow stream.")
    {
        const std::set<int> set {};
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == "{}");
    }

    SECTION("Printing an empty std::set<...> to a wide stream.")
    {
        const std::set<int> set {};
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == L"{}");
    }

    SECTION("Printing a populated std::set<...> to a narrow stream.")
    {
        const std::set<int> set { 1, 2, 3, 4 };
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == "{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::set<...> to a wide stream.")
    {
        const std::set<int> set { 1, 2, 3, 4 };
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == L"{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::multiset<...> to a narrow stream.")
    {
        const std::multiset<int> multiset { 1, 2, 3, 4 };
        std::cout << multiset << std::flush;

        REQUIRE(narrow_buffer.str() == "{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::multiset<...> to a wide stream.")
    {
        const std::multiset<int> multiset { 1, 2, 3, 4 };
        std::wcout << multiset << std::flush;

        REQUIRE(wide_buffer.str() == L"{1, 2, 3, 4}");
    }

    SECTION("Printing an empty std::tuple<...> to a narrow stream.")
    {
        const auto tuple { std::make_tuple() };
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == "<>");
    }

    SECTION("Printing an empty std::tuple<...> to a wide stream.")
    {
        const auto tuple { std::make_tuple() };
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == L"<>");
    }

    SECTION("Printing a populated std::tuple<...> to a narrow stream.")
    {
        const auto tuple { std::make_tuple(1, 2, 3, 4, 5) };
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == "<1, 2, 3, 4, 5>");
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto tuple { std::make_tuple(1, 2, 3, 4, 5) };
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == L"<1, 2, 3, 4, 5>");
    }
}

TEST_CASE("Printing of Nested Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a populated std::map<...> to a narrow stream.")
    {
        const auto map { std::map<int, std::string> {
                { 1, "Template" }, { 2, "Meta" }, { 3, "Programming" } } };

        std::cout << map << std::flush;

        REQUIRE(narrow_buffer.str() == "[(1, \"Template\"), (2, \"Meta\"), (3, \"Programming\")]");
    }

    SECTION("Printing a populated std::map<...> to a wide stream.")
    {
        const auto map { std::map<int, std::wstring>{
                { 1, L"Template" }, { 2, L"Meta" }, { 3, L"Programming" } } };

        std::wcout << map << std::flush;

        REQUIRE(wide_buffer.str() == L"[(1, L\"Template\"), (2, L\"Meta\"), (3, L\"Programming\")]");
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
    {
        const auto vector { std::vector<std::tuple<int, double, std::string>>{
                std::make_tuple(1, 0.1, "Hello"), std::make_tuple(2, 0.2, "World") } };

        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == "[<1, 0.1, \"Hello\">, <2, 0.2, \"World\">]");
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a wide stream.")
    {
        const auto vector { std::vector<std::tuple<int, double, std::wstring>>{
                std::make_tuple(1, 0.1, L"Hello"), std::make_tuple(2, 0.2, L"World") } };

        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[<1, 0.1, L\"Hello\">, <2, 0.2, L\"World\">]");
    }

    SECTION("Printing a populated std::pair<int, std::vector<std::pair<std::string, std::string>>>")
    {
        const auto pair { std::make_pair(
                10, std::vector<std::pair<std::string, std::string>> {
                    std::make_pair("Why", "Not?"), std::make_pair("Someone", "Might!") }) };

        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == "(10, [(\"Why\", \"Not?\"), (\"Someone\", \"Might!\")])");
    }
}

TEST_CASE("Printing with Custom Formatters")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const auto container { std::vector<int>{ 1, 2, 3, 4 } };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 | 3 | 4 $$");
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto container { std::make_tuple(1, 2, 3, 4) };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 | 3 | 4 $$");
    }

    SECTION("Printing a populated std::pair<...> to a wide stream.")
    {
        const auto container { std::make_pair(1, 2) };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 $$");
    }
}
