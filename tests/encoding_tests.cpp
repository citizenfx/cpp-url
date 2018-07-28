// Copyright (c) Glyn Matthews 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <string>
#include <skyr/string_view.hpp>
#include <skyr/url/details/encode.hpp>


class encode_fragment_tests : public ::testing::TestWithParam<char> {};

INSTANTIATE_TEST_CASE_P(
    encode_fragment,
    encode_fragment_tests,
    ::testing::Values(' ', '\"', '<', '>', '`'));

TEST_P(encode_fragment_tests, encode_fragment_set) {
  auto str = std::string();
  skyr::details::pct_encode_char(GetParam(), std::back_inserter(str), " \"<>`");
  auto view = skyr::string_view(str);
  ASSERT_TRUE(skyr::details::is_pct_encoded(begin(view), end(view)));
}

class encode_path_tests : public ::testing::TestWithParam<char> {};

INSTANTIATE_TEST_CASE_P(
    encode_path,
    encode_path_tests,
    ::testing::Values(' ', '\"', '<', '>', '`', '#', '?', '{', '}'));

TEST_P(encode_path_tests, encode_path_set) {
  auto str = std::string();
  skyr::details::pct_encode_char(GetParam(), std::back_inserter(str), " \"<>`#?{}");
  auto view = skyr::string_view(str);
  ASSERT_TRUE(skyr::details::is_pct_encoded(begin(view), end(view)));
}

class encode_userinfo_tests : public ::testing::TestWithParam<char> {};

INSTANTIATE_TEST_CASE_P(
    encode_userinfo,
    encode_userinfo_tests,
    ::testing::Values(' ', '\"', '<', '>', '`', '#', '?', '{', '}', '/', ':', ';', '=', '@', '[', '\\', ']', '^', '|'));

TEST_P(encode_userinfo_tests, encode_userinfo_set) {
  auto str = std::string();
  skyr::details::pct_encode_char(GetParam(), std::back_inserter(str), " \"<>`#?{}/:;=@[\\]^|");
  auto view = skyr::string_view(str);
  ASSERT_TRUE(skyr::details::is_pct_encoded(begin(view), end(view)));
}