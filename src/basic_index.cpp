/* Copyright (C) 2026, Mikhail Nikonov <michael.n.nikonov@gmail.com>
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "basic_index.h"

#include "utility/message.h"

BasicIndex::BasicIndex(MessageSource& source) : source_(source) {}

void BasicIndex::reset() {
  storage_.clear();
  ctids_.clear();
  apids_.clear();
  ecuids_.clear();
}

void BasicIndex::add(int index, const QDltMsg& msg) {
  storage_.insert({index, getWallClockNs(msg), getEcuTimeTicks(msg),
                   ctids_.insert(msg.getCtid().toStdString()),
                   apids_.insert(msg.getApid().toStdString()),
                   ecuids_.insert(msg.getEcuid().toStdString()),
                   msg.getSubtype()});
}

std::optional<Message> BasicIndex::get(int index) {
  auto& index_view = storage_.get<by_index>();
  const auto it = index_view.find(index);
  if (it == index_view.end()) {
    return std::nullopt;
  }

  const auto message = source_.get(it->index);
  if (!message) {
    return std::nullopt;
  }

  const auto payload = message->toStringPayload().toStdString();

  return Message{it->index,           it->timestamp,
                 it->ecu_time,        ctids_.at(it->ctid),
                 apids_.at(it->apid), ecuids_.at(it->ecuid),
                 it->level,           payload};
}

void BasicIndex::search(const Query& params,
                        std::function<bool(const Message&)> callback) {
  std::optional<size_t> ctid_filter;
  if (params.ctid) {
    ctid_filter = ctids_.find(*params.ctid);
    if (!ctid_filter) return;
  }

  std::optional<size_t> apid_filter;
  if (params.apid) {
    apid_filter = apids_.find(*params.apid);
    if (!apid_filter) return;
  }

  auto& index = storage_.get<by_timestamp>();
  auto it = params.min_timestamp ? index.lower_bound(*params.min_timestamp)
                                 : index.begin();
  auto end = params.max_timestamp ? index.upper_bound(*params.max_timestamp)
                                  : index.end();

  for (; it != end; ++it) {
    const auto& [idx, ts, ecu_time, ctid, apid, ecuid, level] = *it;

    if (ctid_filter && ctid != *ctid_filter) continue;
    if (apid_filter && apid != *apid_filter) continue;
    if (params.min_level && level < *params.min_level) continue;
    if (params.max_level && level > *params.max_level) continue;

    if (params.keyword) {
      auto msg_opt = source_.get(idx);
      if (!msg_opt) continue;
      QString payload = msg_opt->toStringPayload();
      Qt::CaseSensitivity cs =
          params.case_insensitive ? Qt::CaseInsensitive : Qt::CaseSensitive;
      QString keyword = QString::fromStdString(*params.keyword);
      if (payload.indexOf(keyword, cs) == -1) continue;
    }

    auto dlt_message = source_.get(idx);
    if (!dlt_message) continue;
    Message message{idx,
                    ts,
                    ecu_time,
                    ctids_.at(ctid),
                    apids_.at(apid),
                    ecuids_.at(ecuid),
                    level,
                    dlt_message->toStringPayload().toStdString()};

    if (!callback(message)) break;
  }
}

int64_t BasicIndex::firstTimestamp() const {
  if (auto& index = storage_.get<by_timestamp>(); !index.empty()) {
    return index.begin()->timestamp;
  }
  return 0;
}

int64_t BasicIndex::lastTimestamp() const {
  if (auto& index = storage_.get<by_timestamp>(); !index.empty()) {
    return index.rbegin()->timestamp;
  }
  return 0;
}
