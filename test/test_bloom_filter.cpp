/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "bloom_filter.h"

#include <gtest/gtest.h>

class BloomFilterParamTest
    : public ::testing::TestWithParam<BloomFilter::Size> {
 protected:
  void SetUp() override {
    filter_ = std::make_unique<BloomFilter>(GetParam());
  }

  std::unique_ptr<BloomFilter> filter_;
};

INSTANTIATE_TEST_SUITE_P(AllSizes, BloomFilterParamTest,
                          ::testing::Values(BloomFilter::Size::k8KB,
                                            BloomFilter::Size::k16KB,
                                            BloomFilter::Size::k32KB));

TEST_P(BloomFilterParamTest, BasicContains) {
  filter_->add(0, "hello world");

  auto key_hello = filter_->makeKey("hello");
  EXPECT_TRUE(filter_->contains(0, key_hello));

  auto key_xyz = filter_->makeKey("xyznotfound999");
  EXPECT_FALSE(filter_->contains(0, key_xyz));
}

TEST_P(BloomFilterParamTest, CrossBlockReject) {
  for (int i = 0; i < 500; i++) {
    filter_->add(i, "this is TTSController log message " + std::to_string(i));
  }
  for (int i = 500; i < 1000; i++) {
    filter_->add(i, "this is SomeOtherModule log message " + std::to_string(i));
  }

  auto key_tts = filter_->makeKey("TTSController");

  for (int i = 0; i < 500; i++) {
    EXPECT_TRUE(filter_->contains(i, key_tts))
        << "Block 0 should contain 'TTSController' at index " << i;
  }
  for (int i = 500; i < 1000; i++) {
    EXPECT_FALSE(filter_->contains(i, key_tts))
        << "Block 1 should NOT contain 'TTSController' at index " << i;
  }
}

TEST_P(BloomFilterParamTest, UnknownKeywordScan) {
  for (int i = 0; i < 500; i++) {
    filter_->add(i, "log entry number " + std::to_string(i) + " with some text");
  }

  auto key_missing = filter_->makeKey("xyznotfound999");

  int false_positives = 0;
  for (int i = 0; i < 500; i++) {
    if (filter_->contains(i, key_missing)) {
      false_positives++;
    }
  }
  EXPECT_EQ(false_positives, 0)
      << "Keyword not in any block should produce 0 false positives, got "
      << false_positives;
}

TEST_P(BloomFilterParamTest, FalsePositiveRate) {
  for (int i = 0; i < 500; i++) {
    filter_->add(i, "message payload with unique content index " +
                        std::to_string(i));
  }

  double max_rate;
  switch (GetParam()) {
    case BloomFilter::Size::kNone:
      GTEST_SKIP() << "No filter";
      break;
    case BloomFilter::Size::k8KB:
      max_rate = 0.10;
      break;
    case BloomFilter::Size::k16KB:
      max_rate = 0.05;
      break;
    case BloomFilter::Size::k32KB:
      max_rate = 0.01;
      break;
  }

  int false_positives = 0;
  for (int i = 0; i < 1000; i++) {
    std::string probe = "probe_keyword_not_present_" + std::to_string(i);
    auto key = filter_->makeKey(probe);
    if (filter_->contains(0, key)) {
      false_positives++;
    }
  }

  double rate = false_positives / 1000.0;
  EXPECT_LT(rate, max_rate)
      << "False positive rate too high: " << (rate * 100) << "% ("
      << false_positives << "/1000), max allowed: " << (max_rate * 100) << "%";
}

TEST_P(BloomFilterParamTest, EmptyBlock) {
  for (int i = 0; i < 500; i++) {
    filter_->add(i, "message in block 0");
  }

  auto key = filter_->makeKey("message");
  EXPECT_TRUE(filter_->contains(0, key));

  auto key_missing = filter_->makeKey("xyznotfound");
  EXPECT_TRUE(filter_->contains(500, key_missing))
      << "Empty block returns true (no bloom data to check)";
}

TEST_P(BloomFilterParamTest, CaseInsensitive) {
  filter_->add(0, "HELLO WORLD TTSController");

  auto key = filter_->makeKey("hello");
  EXPECT_TRUE(filter_->contains(0, key));
}

TEST_P(BloomFilterParamTest, ShortKeyword) {
  filter_->add(0, "hello world");

  auto key = filter_->makeKey("ab");
  EXPECT_TRUE(filter_->contains(0, key))
      << "Short keyword (< 3 chars) has no trigrams, always returns true";
}
