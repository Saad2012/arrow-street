/// Copyright (c) 2012, 2013 by Pascal Costanza, Intel Corporation.

#ifndef SOA_REFERENCE_TYPE
#define SOA_REFERENCE_TYPE

#include <tuple>
#include <type_traits>

namespace soa {

  namespace {
	// support for complex data structures

	// how many leaf types are in a tuple-based tree?

	template<typename T, typename Enable = void> struct count_ref;

	template<typename T>
	struct count_ref<T, typename std::enable_if<
						  std::is_scalar<
							typename std::remove_reference<T>::type>::value>::type> {
	  static const int count = 1;
	};

	template<> struct count_ref<std::tuple<>> {
	  static const int count = 0;
	};

	template<typename Head, typename... Tail>
	struct count_ref<std::tuple<Head, Tail...>> {
	  static const int count =
		count_ref<Head>::count +
		count_ref<std::tuple<Tail...>>::count;
	};

	// construct the flattened reference type

	template<typename T, typename Enable = void> struct get_ref;

	template<typename T>
	struct get_ref<T, typename std::enable_if<std::is_scalar<T>::value>::type> {
	  typedef std::tuple<typename std::add_lvalue_reference<T>::type> type;
	  static const int count = 1;
	};

	template<typename T>
	struct get_ref<T, typename std::enable_if<std::is_class<T>::value>::type> {
	  typedef typename T::reference::type type;
	  static const int count = count_ref<type>::count;
	};

	// extract a subrange from a tuple

	template<int I, int J, typename T> struct tuple_range {
	  typedef tuple_range<I+1,J,T> next_range;
	  typedef decltype(std::tuple_cat(std::tie(std::get<I>(std::declval<T>())),
									  std::declval<typename next_range::result_type>())) result_type;

	  inline static result_type get(const T& tuple) {
		return std::tuple_cat(std::tie(std::get<I>(tuple)), next_range::get(tuple));
	  }
	};

	template<int I, typename T> struct tuple_range<I,I,T> {
	  typedef std::tuple<> result_type;

	  inline static std::tuple<> get(const T&) {
		return std::tie();
	  }
	};

	// match an entry in a flattened tuple tree against
	// the corresponding entry in the structured tuple tree

	template<int N, int Offset, typename T, typename Tuple> struct tuple_match;

	template<int N, int Offset, typename T, typename Head, typename... Tail>
	struct tuple_match<N, Offset, T, std::tuple<Head, Tail...>> {
	  typedef tuple_match<N-1, Offset+get_ref<Head>::count, T, std::tuple<Tail...>> next_match;
	  typedef decltype(next_match::get(std::declval<T>())) result_type;

	  inline static result_type get(const T& tuple) {
		return next_match::get(tuple);
	  }
	};

	template<int Offset, typename Head, typename T, typename... Tail>
	struct tuple_match<0, Offset, T, std::tuple<Head, Tail...>> {
	  typedef tuple_range<Offset, Offset+get_ref<Head>::count,T> match_range;
	  typedef decltype(match_range::get(std::declval<T>())) result_type;

	  inline static result_type get(const T& tuple) {
		return match_range::get(tuple);
	  }
	};
  }

  // user support for defining reference tuples

  template<typename... T> struct reference_type;

  template<> struct reference_type<> {
	typedef std::tuple<> type;

	template<int> static type match(const type&) {
	  return std::tie();
	}
  };

  template<typename Head, typename... Tail>
  struct reference_type<Head, Tail...> {
	typedef decltype(std::tuple_cat(std::declval<typename get_ref<Head>::type>(),
									std::declval<typename reference_type<Tail...>::type>())) type;

	typedef std::tuple<Head, Tail...> _tuple;

	template<int N> static auto match(const type& tuple)
	  -> decltype(tuple_match<N, 0, type, _tuple>::get(tuple))
	{
	  return tuple_match<N, 0, type, _tuple>::get(tuple);
	}

	template<int N> static auto get(const type& tuple)
	  -> decltype(std::get<0>(match<N>(tuple)))
	{
	  return std::get<0>(match<N>(tuple));
	}
  };
}

#endif