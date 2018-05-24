// Copyright 2009-2010 Jeroen Habraken.
// Copyright 2009-2018 Dean Michael Berris, Glyn Matthews.
// Copyright 2012 Google, Inc.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETWORK_URI_WHATWG_URL_INC
#define NETWORK_URI_WHATWG_URL_INC

/**
 * \file
 * \brief Contains the url class.
 */

#include <iterator>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <cstdlib>
#include <vector>
#include <network/string_view.hpp>
#include <network/optional.hpp>
#include <network/uri/config.hpp>
#include <network/uri/uri_errors.hpp>
#include <network/uri/detail/uri_parts.hpp>
#include <network/uri/detail/encode.hpp>
#include <network/uri/detail/decode.hpp>
#include <network/uri/detail/translate.hpp>
#include <network/uri/whatwg/url_search_parameters.hpp>

#ifdef NETWORK_URI_MSVC
#pragma warning(push)
#pragma warning(disable : 4251 4231 4660)
#endif

namespace network {
namespace whatwg {
/**
 * \ingroup url
 * \class url network/uri/url.hpp network/url.hpp
 * \brief A class that parses a URL (Uniform Resource Locator)
 *        into its component parts.
 *
 * According to IETF RFC 3986, a URI has the following generic syntax:
 *
 * \code
 * [scheme:][username@password][host][:port][path][?query][#fragment]
 * \endcode
 *
 * \code
 * [Constructor(USVString url, optional USVString base),
 * Exposed=(Window,Worker),
 * LegacyWindowAlias=webkitURL]
 * interface URL {
 *   stringifier attribute USVString href;
 *   readonly attribute USVString origin;
 *   attribute USVString protocol;
 *   attribute USVString username;
 *   attribute USVString password;
 *   attribute USVString host;
 *   attribute USVString hostname;
 *   attribute USVString port;
 *   attribute USVString pathname;
 *   attribute USVString search;
 *   [SameObject] readonly attribute URLSearchParams searchParams;
 *   attribute USVString hash;
 *
 *   USVString toJSON();
 * };
 * \endcode
 *
 * Example:
 *
 * \code
 * network::url instance("http://cpp-netlib.org/");
 * assert(instance.is_absolute());
 * assert(!instance.is_opaque());
 * assert(instance.has_scheme());
 * assert("http:" == *instance.scheme());
 * assert(instance.has_host());
 * assert("cpp-netlib.org" == *instance.host());
 * assert(instance.has_path());
 * assert("/" == *instance.path());
 * \endcode
 */
class url {
 public:
  /**
   * \brief The URL string_type.
   */
  using string_type = std::string;

  /**
   * \brief A reference to the underlying string_type parts.
   */
  using string_view = network::string_view;

  /**
   * \brief The char traits.
   */
  using traits_type = string_view::traits_type;

  /**
   * \brief The URL const_iterator type.
   */
  using const_iterator = string_view::const_iterator;

  /**
   * \brief The URL iterator type.
   */
  using iterator = const_iterator;

  /**
   * \brief The URL value_type.
   */
  using value_type = std::iterator_traits<iterator>::value_type;

  /**
   * \class path_iterator
   */
  class path_iterator {
  public:
    
    using value_type = string_view;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = const value_type &;
    using iterator_category = std::forward_iterator_tag;
    
    path_iterator() = default;
    
    explicit path_iterator(optional<detail::uri_part> path)
    : path_{path}, element_{} {
      if (path_ && path_->empty()) {
        path_ = nullopt;
      }
      else {
        // skip past '/'
        advance();
        assign();
      }
    }
    
    path_iterator(const path_iterator &) = default;
    
    path_iterator &operator=(const path_iterator &) = default;
    
    reference operator++() noexcept {
      increment();
      return element_;
    }
    
    value_type operator++(int) noexcept {
      auto original = element_;
      increment();
      return original;
    }
    
    reference operator*() const noexcept {
      return element_;
    }
    
    pointer operator->() const noexcept {
      return std::addressof(element_);
    }
    
    bool operator==(const path_iterator &other) const noexcept {
      if (!path_ && !other.path_) {
        return true;
      }
      else if (path_ && other.path_) {
        // since we're comparing substrings, the address of the first
        // element in each iterator must be the same
        return std::addressof(element_) == std::addressof(other.element_);
      }
      return false;
    }
    
    inline bool operator!=(const path_iterator &other) const noexcept {
      return !(*this == other);
    }
    
  private:

    void advance() noexcept {
      auto first = std::begin(*path_), last = std::end(*path_);
      
      auto sep_it =
      std::find_if(first, last, [](char c) -> bool { return c == '/'; });
      
      if (sep_it != last) {
        ++sep_it;  // skip next separator
      }
      
      // reassign path to the next element
      path_ = ::network::detail::uri_part(sep_it, last);
    }
    
    void assign() noexcept {
      auto first = std::begin(*path_), last = std::end(*path_);
      
      auto sep_it =
      std::find_if(first, last, [](char c) -> bool { return c == '/'; });
      
      element_ =
      string_view(std::addressof(*first), std::distance(first, sep_it));
    }
    
    void increment() noexcept {
      assert(path_);
      
      if (!path_->empty()) {
        advance();
        assign();
      }
      
      if (path_->empty()) {
        path_ = nullopt;
      }
    }
    
    optional<detail::uri_part> path_;
    value_type element_;
  };

  /**
   * \brief An iterator class that iterates through name-value pairs
   *        in a URL query.
   */
  class query_iterator {
  public:

    /**
     * \brief The value_type is a name-value pair.
     */
    using value_type = std::pair<string_view, string_view>;

    /**
     * \brief The iterator difference_type.
     */
    using difference_type = std::ptrdiff_t;

    /**
     *
     */
    using pointer = const value_type *;

    /**
     *
     */
    using reference = const value_type &;

    /**
     *
     */
    using iterator_category = std::forward_iterator_tag;

    /**
     * \brief Constructor.
     */
    query_iterator() = default;

    /**
     * \brief Constructor.
     */
    explicit query_iterator(optional<detail::uri_part> query)
      : query_(query)
      , nvp_() {
      auto first = std::begin(*query_), last = std::end(*query_);
      // skip ?
      ++first;
      query_ = ::network::detail::uri_part(first, last);
        
      if (query_ && query_->empty()) {
        query_ = nullopt;
      }
      else {
        assign();
      }
    }

    /**
     * \brief Copy constructor.
     */
    query_iterator(const query_iterator &) = default;

    /**
     * \brief Assignment operator.
     */
    query_iterator &operator=(const query_iterator &) = default;

    reference operator++() noexcept {
      increment();
      return nvp_;
    }

    value_type operator++(int) noexcept {
      auto original = nvp_;
      increment();
      return original;
    }

    reference operator*() const noexcept {
      return nvp_;
    }

    pointer operator->() const noexcept {
      return std::addressof(nvp_);
    }

    bool operator==(const query_iterator &other) const noexcept {
      if (!query_ && !other.query_) {
        return true;
      }
      else if (query_ && other.query_) {
        // since we're comparing substrings, the address of the first
        // element in each iterator must be the same
        return std::addressof(nvp_.first) == std::addressof(other.nvp_.first);
      }
      return false;
    }

    bool operator!=(const query_iterator &other) const noexcept {
      return !(*this == other);
    }

  private:

    void advance() noexcept {
      auto first = std::begin(*query_), last = std::end(*query_);

      auto sep_it = std::find_if(
          first, last, [](char c) -> bool { return c == '&' || c == ';'; });

      if (sep_it != last) {
        ++sep_it;  // skip next separator
      }

      // reassign query to the next element
      query_ = ::network::detail::uri_part(sep_it, last);
    }

    void assign() noexcept {
      auto first = std::begin(*query_), last = std::end(*query_);

      auto sep_it = std::find_if(
          first, last, [](char c) -> bool { return c == '&' || c == ';'; });
      auto eq_it =
          std::find_if(first, sep_it, [](char c) -> bool { return c == '='; });

      nvp_.first =
          string_view(std::addressof(*first), std::distance(first, eq_it));
      if (eq_it != sep_it) {
        ++eq_it;  // skip '=' symbol
      }
      nvp_.second =
          string_view(std::addressof(*eq_it), std::distance(eq_it, sep_it));
    }

    void increment() noexcept {
      assert(query_);

      if (!query_->empty()) {
        advance();
        assign();
      }

      if (query_->empty()) {
        query_ = nullopt;
      }
    }

    optional<detail::uri_part> query_;
    value_type nvp_;
  };

  /**
   * \brief Default constructor.
   */
  url();

  /**
   * \brief Constructor.
   * \param first The first element in a string sequence.
   * \param last The end + 1th element in a string sequence.
   * \throws uri_syntax_error if the sequence is not a valid URL.
   * \throws std::bad_alloc If the underlying string cannot be allocated.
   */
  template <class InputIter>
  url(InputIter first, InputIter last) {
    if (!initialize(string_type(first, last))) {
      throw uri_syntax_error();
    }
  }

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
  template <class InputIter>
  explicit url(InputIter first, InputIter last, std::error_code &ec) {
    if (!initialize(string_type(first, last))) {
      ec = make_error_code(uri_error::invalid_syntax);
    }
  }
#endif  // !defined(DOXYGEN_SHOULD_SKIP_THIS)

  /**
   * \brief Constructor.
   * \param source A source string that is to be parsed as a URL.
   * \throws uri_syntax_error if the source is not a valid URL.
   * \throws std::bad_alloc If the underlying string cannot be allocated.
   */
  template <class Source>
  explicit url(const Source &source) {
    if (!initialize(detail::translate(source))) {
      throw uri_syntax_error();
    }
  }
  
  template <class Source>
  url(const Source &source, const url &base) {
    auto tmp(base);
    tmp.swap(*this);

    auto parsed_source = url(source);
    if (parsed_source.has_query()) {
      url_parts_.query = parsed_source.url_parts_.query;
    }
  }

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
  template <class Source>
  url(const Source &source, std::error_code &ec) {
    if (!initialize(detail::translate(source))) {
      ec = make_error_code(uri_error::invalid_syntax);
    }
  }
#endif  // !defined(DOXYGEN_SHOULD_SKIP_THIS)

  /**
   * \brief Copy constructor.
   * \throws std::bad_alloc If the underlying string cannot be allocated.
   */
  url(const url &other);

  /**
   * \brief Move constructor.
   */
  url(url &&other) noexcept;

  /**
   * \brief Destructor.
   */
  ~url();

  /**
   * \brief Assignment operator.
   * \throws std::bad_alloc If the underlying string cannot be allocated.
   */
  url &operator=(url other);

  /**
   * \brief Swaps one url object with another.
   * \param other The other url object.
   */
  void swap(url &other) noexcept;

  /**
   * \brief Returns an iterator at the first element in the
   *        underlying sequence.
   * \return An iterator starting at the first element.
   */
  const_iterator begin() const noexcept;

  /**
   * \brief Returns an iterator at the end + 1th element in the
   *        underlying sequence.
   * \return An iterator starting at the end + 1th element.
   */
  const_iterator end() const noexcept;

  /**
   * \brief Tests whether this URL has a scheme component.
   * \return \c true if the URL has a scheme, \c false otherwise.
   */
  bool has_scheme() const noexcept;

  /**
   * \brief Returns the URL scheme.
   * \return The scheme.
   * \pre has_scheme()
   */
  string_view scheme() const noexcept;

  bool has_protocol() const noexcept {
    return has_scheme();
  }
  
  string_view protocol() const noexcept {
    return scheme();
  }
  
  /**
   * \brief Tests whether this URL has a user info component.
   * \return \c true if the URL has a user info, \c false otherwise.
   */
  bool has_user_info() const noexcept;

  /**
   * \brief Returns the URL user info.
   * \return The user info.
   * \pre has_user_info()
   */
  string_view user_info() const noexcept;

  string_view user_name() const noexcept;

  string_view password() const noexcept;

  /**
   * \brief Tests whether this URL has a host component.
   * \return \c true if the URL has a host, \c false otherwise.
   */
  bool has_host() const noexcept;

  /**
   * \brief Returns the URL host.
   * \return The host.
   * \pre has_host()
   */
  string_view host() const noexcept;

  /**
   * \brief Tests whether this URL has a port component.
   * \return \c true if the URL has a port, \c false otherwise.
   */
  bool has_port() const noexcept;

  /**
   * \brief Returns the URL port.
   * \return The port.
   * \pre has_port()
   */
  string_view port() const noexcept;

  /**
   * \brief Returns the URL port as an integer.
   * \return The port number.
   * \pre has_port()
   */
  template <typename intT>
  intT port(typename std::is_integral<intT>::type * = 0) const {
    assert(has_port());
    auto p = port();
    const char *port_first = std::addressof(*p.begin());
    char *port_last = 0;
    return static_cast<intT>(std::strtoul(port_first, &port_last, 10));
  }

  /**
   * \brief Tests whether this URL has a path component.
   * \return \c true if the URL has a path, \c false otherwise.
   */
  bool has_path() const noexcept;

  /**
   * \brief Returns the URL path.
   * \return The path.
   * \pre has_path()
   */
  string_view path() const noexcept;

  /**
   * \brief
   */
  path_iterator path_begin() const noexcept;

  /**
   * \brief
   */
  path_iterator path_end() const noexcept;

  /**
   * \brief Tests whether this URL has a query component.
   * \return \c true if the URL has a query, \c false otherwise.
   */
  bool has_query() const noexcept;

  /**
   * \brief Returns the URL query.
   * \return The query.
   * \pre has_query()
   */
  string_view query() const noexcept;

  /**
   * \brief Returns an iterator to the first key-value pair in the query
   *        component.
   *
   * \return query_iterator.
   */
  query_iterator query_begin() const noexcept;

  /**
   * \brief Returns an iterator to the last key-value pair in the query
   *        component.
   *
   * \return query_iterator.
   */
  query_iterator query_end() const noexcept;
  
  bool has_search() const noexcept {
    return has_query();
  }
  
  string_view search() const noexcept {
    return query();
  }
  
  using search_iterator = query_iterator;
  
  search_iterator search_begin() const noexcept {
    return query_begin();
  }
  
  search_iterator search_end() const noexcept {
    return query_end();
  }
  
//  url_search_parameters search_parameters() const;
  
//  void set_search_parameters(const url_search_parameters &params);

  /**
   * \brief Tests whether this URL has a fragment component.
   * \return \c true if the URL has a fragment, \c false otherwise.
   */
  bool has_fragment() const noexcept;

  /**
   * \brief Returns the URL fragment.
   * \return The fragment.
   * \pre has_fragment()
   */
  string_view fragment() const noexcept;
  
  bool has_hash() const noexcept {
    return has_fragment();
  }
  
  string_view hash() const noexcept {
    return fragment();
  }

  /**
   * \brief Returns the URL as a std::basic_string object.
   * \return A URL string.
   */
  template <class charT, class traits = std::char_traits<charT>,
            class Allocator = std::allocator<charT> >
  std::basic_string<charT, traits, Allocator> to_string(
      const Allocator &alloc = Allocator()) const {
    return std::basic_string<charT, traits, Allocator>(begin(), end());
  }

  /**
   * \brief Returns the URL as a std::string object.
   * \returns A URL string.
   */
  std::string string() const;

  /**
   * \brief Returns the URL as a std::wstring object.
   * \returns A URL string.
   */
  std::wstring wstring() const;

  /**
   * \brief Returns the URL as a std::u16string object.
   * \returns A URL string.
   */
  std::u16string u16string() const;

  /**
   * \brief Returns the URL as a std::u32string object.
   * \returns A URL string.
   */
  std::u32string u32string() const;

  /**
   * \brief Checks if the url object is empty, i.e. it has no parts.
   * \returns \c true if there are no parts, \c false otherwise.
   */
  bool empty() const noexcept;

  /**
   * \brief Checks if the url is absolute, i.e. it has a scheme.
   * \returns \c true if it is absolute, \c false if it is relative.
   */
  bool is_absolute() const noexcept;

  /**
   * \brief Checks if the url is opaque.
   * \returns \c true if it is opaque, \c false if it is hierarchical.
   */
  bool is_opaque() const noexcept;

  /**
   * \brief Checks if the url has a special scheme.
   */
  bool is_special() const noexcept;

  /**
   * \brief Returns the default port for the given scheme.
   */
  static optional<std::uint16_t> default_port(const url::string_type &scheme);

  /**
   * \brief Serializes a url object.
   * \returns A serialize url.
   * \throws std::bad_alloc
   */
  url serialize() const;

  /**
   * \brief Compares this URL against another.
   * \param other The other URL.
   * \returns \c 0 if the URLs are considered equal, \c -1 if this is
   *         less than other and and 1 if this is greater than
   *         other.
   */
  int compare(const url &other) const;
  
  // TODO: implement origin
  
  // TODO:: implement rendering

 private:
  bool initialize(const string_type &url);

  string_type url_;
  string_view url_view_;
  detail::uri_parts url_parts_;
  bool cannot_be_a_base_url_;

};

/**
 * \brief \c url factory function.
 * \param first The first element in a string sequence.
 * \param last The end + 1th element in a string sequence.
 * \param ec Error code set if the sequence is not a valid URL.
 */
template <class InputIter>
inline url make_url(InputIter first, InputIter last, std::error_code &ec) {
  return url(first, last, ec);
}

/**
 * \brief \c url factory function.
 * \param source A source string that is to be parsed as a URL.
 * \param ec Error code set if the source is not a valid URL.
 */
template <class Source>
inline url make_url(const Source &source, std::error_code &ec) {
  return url(source, ec);
}

/**
 * \brief Swaps one url object with another.
 */
void swap(url &lhs, url &rhs) noexcept;

/**
* \brief Equality operator for the \c url.
*/
bool operator==(const url &lhs, const url &rhs) noexcept;

/**
 * \brief Equality operator for the \c url.
 */
bool operator==(const url &lhs, const char *rhs) noexcept;

/**
 * \brief Equality operator for the \c url.
 */
inline bool operator==(const char *lhs, const url &rhs) noexcept {
  return rhs == lhs;
}

/**
 * \brief Inequality operator for the \c url.
 */
inline bool operator!=(const url &lhs, const url &rhs) noexcept {
  return !(lhs == rhs);
}

/**
 * \brief Less-than operator for the \c url.
 */
bool operator<(const url &lhs, const url &rhs) noexcept;

/**
 * \brief Greater-than operator for the \c url.
 */
inline bool operator>(const url &lhs, const url &rhs) noexcept {
  return rhs < lhs;
}

/**
 * \brief Less-than-or-equal-to operator for the \c url.
 */
inline bool operator<=(const url &lhs, const url &rhs) noexcept {
  return !(rhs < lhs);
}

/**
 * \brief Greater-than-or-equal-to operator for the \c url.
 */
inline bool operator>=(const url &lhs, const url &rhs) noexcept {
  return !(lhs < rhs);
}
}  // namespace whatwg
}  // namespace network

#if !defined(DOXYGEN_SHOULD_SKIP_THIS)
namespace std {
template <>
struct hash<::network::whatwg::url> {
  std::size_t operator()(const ::network::whatwg::url &url_) const {
    std::size_t seed = 0;
    std::for_each(std::begin(url_), std::end(url_),
                  [&seed](network::whatwg::url::value_type v) {
                    std::hash<::network::whatwg::url::value_type> hasher;
                    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                  });
    return seed;
  }
};
}  // namespace std
#endif  // !defined(DOXYGEN_SHOULD_SKIP_THIS)

#ifdef NETWORK_URL_MSVC
#pragma warning(pop)
#endif

#endif  // NETWORK_URI_WHATWG_URL_INC
