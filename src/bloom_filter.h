/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_BLOOM_FILTER_H_
#define DLT_MCP_SERVER_BLOOM_FILTER_H_

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

class BloomFilter {
 public:
  enum class Size : size_t {
    kNone = 0,
    k8KB = 8 * 1024,
    k16KB = 16 * 1024,
    k32KB = 32 * 1024,
  };

  class Key {
    friend class BloomFilter;

    enum class BlockState : uint8_t {
      kUnchecked = 0,
      kPositive = 1,
      kNegative = 2,
    };

    struct Probe {
      size_t byte_offset;
      uint8_t mask;
    };

    std::vector<Probe> probes_;
    std::vector<BlockState> block_states_;
  };

  explicit BloomFilter(Size filter_size);

  void add(size_t index, std::string_view text);

  Key makeKey(std::string_view text) const;

  bool contains(size_t index, Key& key) const;

 private:
  static constexpr size_t kBlockSize = 500;
  static constexpr size_t kHashCount = 4;

  size_t bytes_per_block_;
  size_t bit_mask_;

  std::vector<std::unique_ptr<std::vector<uint8_t>>> blocks_;

  static size_t blockId(size_t index);
  uint8_t* getBlock(size_t index);

  template <typename Op>
  void processTrigrams(std::string_view text, Op&& op) const;
};

#endif  // DLT_MCP_SERVER_BLOOM_FILTER_H_
