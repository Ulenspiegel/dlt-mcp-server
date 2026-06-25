/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DLT_MCP_SERVER_NGRAM_INDEX_H_
#define DLT_MCP_SERVER_NGRAM_INDEX_H_

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <memory>

#include "bloom_filter.h"
#include "index.h"
#include "utility/dlt_id.h"

namespace bmi = boost::multi_index;

class NgramIndex : public Index {
 public:
  NgramIndex(MessageSource& source, BloomFilter::Size filter_size);

  void reset() override;

  void add(int index, const QDltMsg& msg) override;
  std::optional<Message> get(int index) override;

  void search(const Query& params,
              std::function<bool(const Message&)> callback) override;

  int64_t firstTimestamp() const override;
  int64_t lastTimestamp() const override;

 private:
  struct IndexEntry {
    int index;
    int64_t timestamp;
    int64_t ecu_time;
    size_t ctid;
    size_t apid;
    size_t ecuid;
    int level;
  };

  struct by_timestamp {};
  struct by_index {};

  using IndexStorage = bmi::multi_index_container<
      IndexEntry,
      bmi::indexed_by<
          bmi::ordered_non_unique<
              bmi::tag<by_timestamp>,
              bmi::member<IndexEntry, int64_t, &IndexEntry::timestamp>>,
          bmi::ordered_unique<
              bmi::tag<by_index>,
              bmi::member<IndexEntry, int, &IndexEntry::index>>>>;

  MessageSource& source_;
  BloomFilter::Size filter_size_;
  IndexStorage storage_;

  DltID ctids_;
  DltID apids_;
  DltID ecuids_;

  std::unique_ptr<BloomFilter> filter_;
};

#endif  // DLT_MCP_SERVER_NGRAM_INDEX_H_
