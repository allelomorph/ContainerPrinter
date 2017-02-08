#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace ContainerPrinter
{
   namespace Traits
   {
      /**
      * @brief A neat SFINAE trick.
      *
      * @see N3911
      */
      template<class...>
      using void_t = void;

      /**
      * @brief Base case for the testing of STD compatible container types.
      */
      template<
         typename /*Type*/,
         typename = void
      >
      struct is_printable_as_container : std::false_type
      {
      };

      /**
      * @brief Specialization to ensure that Standard Library compatible containers that have
      * `begin()`, `end()`, and `empty()` member functions, as well as the `value_type` and
      * `iterator` typedefs, are treated as printable containers.
      */
      template<typename Type>
      struct is_printable_as_container<
         Type,
         void_t<
            decltype(std::declval<Type&>().begin()),
            decltype(std::declval<Type&>().end()),
            decltype(std::declval<Type&>().empty()),
            typename Type::value_type,
            typename Type::iterator
         >
      > : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat std::pair<...> as a printable container type.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct is_printable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat std::tuple<...> as a printable container type.
      */
      template<typename... DataTypes>
      struct is_printable_as_container<std::tuple<DataTypes...>> : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat arrays as printable container types.
      */
      template<
         typename ArrayType,
         std::size_t ArraySize
      >
      struct is_printable_as_container<ArrayType[ArraySize]> : public std::true_type
      {
      };

      /**
      * @brief Narrow character array specialization in order to ensure that we print character
      * arrays as string and not as delimiter containers.
      */
      template<std::size_t ArraySize>
      struct is_printable_as_container<char[ArraySize]> : public std::false_type
      {
      };

      /**
      * @brief Wide character array specialization in order to ensure that we print character arrays
      * as string and not as delimiter containers.
      */
      template<std::size_t ArraySize>
      struct is_printable_as_container<wchar_t[ArraySize]> : public std::false_type
      {
      };

      /**
      * @brief String specialization in order to ensure that we treat strings as nothing more than
      * strings.
      */
      template<
         typename CharacterType,
         typename CharacterTraitsType,
         typename AllocatorType
      >
      struct is_printable_as_container<
         std::basic_string<
            CharacterType,
            CharacterTraitsType,
            AllocatorType
         >
      > : public std::false_type
      {
      };

      /**
      * @brief Helper variable template.
      */
      template<typename Type>
      constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;

      template<typename>
      struct is_a_pair : public std::false_type
      {
      };

      template<
         typename FirstType,
         typename SecondType
      >
      struct is_a_pair<std::pair<FirstType, SecondType>> : public std::true_type
      {
      };

      template<typename>
      struct is_a_tuple : public std::false_type
      {
      };

      template<typename... Args>
      struct is_a_tuple<std::tuple<Args...>> : public std::true_type
      {
      };

      template<typename>
      struct is_not_a_tuple : public std::true_type
      {
      };

      template<typename... Args>
      struct is_not_a_tuple<std::tuple<Args...>> : public std::false_type
      {
      };
   }

   namespace Decorator
   {
      /**
      * @brief Struct to neatly wrap up all the additional characters we'll need in order to
      * print out the containers.
      */
      template<typename CharacterType>
      struct wrapper
      {
         using type = CharacterType;

         const type* prefix;
         const type* separator;
         const type* suffix;
      };

      /**
      * @brief Additional wrapper around the `delimiter_values` struct for added convenience.
      */
      template<
         typename /*ContainerType*/,
         typename CharacterType
      >
      struct delimiters
      {
         using type = wrapper<CharacterType>;

         static const type values;
      };

      /**
      * @brief Default narrow delimiters for any container type that isn't specialized.
      */
      template<typename ContainerType>
      struct delimiters<ContainerType, char>
      {
         using type = wrapper<char>;

         static constexpr type values = { "[", ", ", "]" };
      };

      /**
      * @brief Default wide delimiters for any container type that isn't specialized.
      */
      template<typename ContainerType>
      struct delimiters<ContainerType, wchar_t>
      {
         using type = wrapper<wchar_t>;

         static constexpr type values = { L"[", L", ", L"]" };
      };

      /**
      * @brief Narrow character specialization for std::set<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, char>
      {
         static constexpr wrapper<char> values = { "{", ", ", "}" };
      };

      /**
      * @brief Wide character specialization for std::set<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
      };

      /**
      * @brief Narrow character specialization for std::multiset<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, char>
      {
         static constexpr wrapper<char> values = { "{", ", ", "}" };
      };

      /**
      * @brief Wide character specialization for std::multiset<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
      };

      /**
      * @brief Narrow character specialization for std::pair<...>.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct delimiters<std::pair<FirstType, SecondType>, char>
      {
         static constexpr wrapper<char> values = { "(", ", ", ")" };
      };

      /**
      * @brief Wide character specialization for std::pair<...>.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct delimiters<std::pair<FirstType, SecondType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"(", L", ", L")" };
      };

      /**
      * @brief Narrow character specialization for std::tuple<...>.
      */
      template<typename... DataType>
      struct delimiters<std::tuple<DataType...>, char>
      {
         static constexpr wrapper<char> values = { "<", ", ", ">" };
      };

      /**
      * @brief Wide character specialization for std::tuple<...>.
      */
      template<typename... DataType>
      struct delimiters<std::tuple<DataType...>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"<", L", ", L">" };
      };
   }

   /**
   * @brief Default container formatter that will be used to print prefix, element, separator, and
   * suffix strings to an output stream.
   */
   template<
      typename ContainerType,
      typename StreamType
   >
   struct default_formatter
   {
      static constexpr auto decorators =
         ContainerPrinter::Decorator::delimiters<ContainerType, StreamType::char_type>::values;

      static void print_prefix(StreamType& stream)
      {
         stream << decorators.prefix;
      }

      template<typename ElementType>
      static void print_element(
         StreamType& stream,
         const ElementType& element)
      {
         stream << element;
      }

      static void print_delimiter(StreamType& stream)
      {
         stream << decorators.separator;
      }

      static void print_suffix(StreamType& stream)
      {
         stream << decorators.suffix;
      }
   };

   /**
   * @brief Helper function to determine if a container is empty.
   */
   template<typename ContainerType>
   inline bool is_empty(const ContainerType& container)
   {
      return container.empty();
   };

   /**
   * @brief Helper function to test arrays for emptiness.
   */
   template<
      typename ArrayType,
      std::size_t ArraySize
   >
   constexpr bool is_empty(const ArrayType(&)[ArraySize])
   {
      return !static_cast<bool>(ArraySize);
   }

   template<typename...>
   struct tuple_handler
   {
      template<
         typename StreamType,
         typename TupleType
      >
      inline static void print(
         StreamType& /*stream*/,
         const TupleType& /*tuple*/);
   };

   /**
   * @brief Specialization of tuple helper to handle empty std::tuple<...> objects.
   */
   template<>
   struct tuple_handler<std::tuple<>>
   {
      template<
         typename StreamType,
         typename TupleType
      >
      inline static void print(
         StreamType& /*stream*/,
         const TupleType& /*tuple*/) noexcept
      {
      };
   };

   /**
   * @brief Specialization of tuple helper for the first index of a std::tuple<...>
   */
   template<typename SingleArg>
   struct tuple_handler<std::tuple<SingleArg>>
   {
      template<
         typename StreamType,
         typename TupleType
      >
      inline static void print(
         StreamType& stream,
         const TupleType& tuple)
      {
         stream << std::get<0>(tuple);
      }
   };

   /**
   * @brief Recursive tuple handler struct meant to unpack and print std::tuple<...> elements.
   */
   template<
      typename FirstElement,
      typename... Args
   >
   struct tuple_handler<std::tuple<FirstElement, Args...>>
   {
      template<
         typename StreamType,
         typename TupleType
      >
      inline static void print(
         StreamType& stream,
         const TupleType& container)
      {
         tuple_handler<std::tuple<Args...>>::print(stream, container);

         static constexpr auto decorators =
            ContainerPrinter::Decorator::delimiters<TupleType, StreamType::char_type>::values;

         stream
            << decorators.separator
            << std::get<sizeof...(Args)>(container);
      }
   };

   /**
   * @brief Overload meant to handle std::tuple<...> objects.
   *
   * @todo Figure out how to add a defaultable FormatterType template parameter into the mix.
   * Without it, it won't be possible to customize std::tuple<...>'s printing format.
   */
   template<
      typename StreamType,
      typename TupleType,
      //typename FormatterType = default_formatter<TupleType, StreamType>,
      typename = std::enable_if_t<Traits::is_a_tuple<TupleType>::value>
   >
   static void ToStream(
      StreamType& stream,
      const TupleType& container)
   {
      using ContainerType = std::decay_t<decltype(container)>;
      using Formatter = default_formatter<ContainerType, StreamType>;

      Formatter::print_prefix(stream);
      tuple_handler<ContainerType>::print(stream, container);
      Formatter::print_suffix(stream);
   }

   /**
   * @brief Overload meant to handle std::pair<...> objeccts.
   */
   template<
      typename StreamType,
      typename FirstType,
      typename SecondType,
      typename FormatterType = default_formatter<std::pair<FirstType, SecondType>, StreamType>
   >
   static void ToStream(
      StreamType& stream,
      const std::pair<FirstType, SecondType>& container)
   {
      using Formatter = FormatterType;

      Formatter::print_prefix(stream);
      Formatter::print_element(stream, container.first);
      Formatter::print_delimiter(stream);
      Formatter::print_element(stream, container.second);
      Formatter::print_suffix(stream);
   }

   /**
   * @brief Overload meant to handle containers that support the notion of "emptiness".
   */
   template<
      typename StreamType,
      typename ContainerType,
      typename FormatterType = default_formatter<ContainerType, StreamType>,
      typename = std::enable_if_t<Traits::is_not_a_tuple<ContainerType>::value>
   >
   static void ToStream(
      StreamType& stream,
      const ContainerType& container)
   {
      using Formatter = FormatterType;

      Formatter::print_prefix(stream);

      if (is_empty(container))
      {
         Formatter::print_suffix(stream);
         return;
      }

      auto begin = std::begin(container);
      Formatter::print_element(stream, *begin);

      std::advance(begin, 1);

      std::for_each(begin, std::end(container),
         [&stream](const auto& element)
      {
         Formatter::print_delimiter(stream);
         Formatter::print_element(stream, element);
      });

      Formatter::print_suffix(stream);
   }
}

/**
* @brief Overload of the stream output operator for compatible containers.
*/
template<
   typename StreamType,
   typename ContainerType,
   typename = std::enable_if_t<ContainerPrinter::Traits::is_printable_as_container_v<ContainerType>>
>
auto& operator<<(
   StreamType& stream,
   const ContainerType& container)
{
   ContainerPrinter::ToStream(stream, container);

   return stream;
}
