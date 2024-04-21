// You are free to use this in any way you like, as long as you mention:
// ------------------------------------------------------------------------------
// Copyright (c) 2024 superKoder (github.com/superKoder/)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The ABOVE COPYRIGHT notice and this permission notice SHALL BE INCLUDED in
// all copies or substantial portions of the Software.
//
// The software is provided "as is", without warranty of any kind, express or
// implied, including but not limited to the warranties of merchantability,
// fitness for a particular purpose and noninfringement. In no event shall the
// authors or copyright holders be liable for any claim, damages or other
// liability, whether in an action of contract, tort or otherwise, arising from,
// out of or in connection with the software or the use or other dealings in the
// software.
// ------------------------------------------------------------------------------

#ifndef SUPERKODER_COMLYNX_H
#define SUPERKODER_COMLYNX_H
#pragma once

#if __cplusplus < 201703L
#error Requires C++17.
#endif

#include <array>
#include <cassert>
#include <chrono>
#include <deque>
#include <vector>

/// The Handy emulator uses this too and we want to live in it.
#ifndef UBYTE
#define UBYTE uint8_t
#endif  // UBYTE

#define COMLYNX_ASSERT(cond)                                      \
  if (!(cond)) {                                                  \
    std::fprintf(stderr, "\n-assert: %s (%s @ line %d)\n", #cond, \
                 __PRETTY_FUNCTION__, __LINE__);                  \
    std::abort();                                                 \
  }

/// The checksum used by most ComLynx games (e.g. Slime World)
constexpr inline UBYTE ComLynxCommonChecksum(
    std::initializer_list<UBYTE> const &bytes) {
  UBYTE sum = 0;
  for (UBYTE byte : bytes) {
    sum += byte;
  }
  return 255 - sum;
}

constexpr inline bool CalculateEvenParity(UBYTE byte) {
  byte ^= byte >> 4;
  byte ^= byte >> 2;
  byte ^= byte >> 1;
  return byte & 1;
}

constexpr inline bool CalculateOddParity(UBYTE byte) {
  return !CalculateEvenParity(byte);
}

constexpr inline bool CalculateParity(bool even_parity, UBYTE byte) {
  return (even_parity ? CalculateEvenParity(byte) : CalculateOddParity(byte));
}

/**
 * Class to replicate the Atari Lynx ComLynx UART.
 */
class ComLynx {
 public:
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;
  using ReadReceipt = uint32_t;
  using Player = int;

  enum class ParityConfig {
    kOdd,
    kEven,
    kSpace,
    kMark,
  };

  enum class TxNotReadyReason {
    kNone = 0,
    kOverrun,
    kFrame,
  };

  struct Error {
    bool overrun = {};
    bool parity = {};
    bool frame = {};

    void Reset() {
      overrun = false;
      parity = false;
      frame = false;
    }
  };

  struct ByteMessage {
    static ReadReceipt s_read_receipt_complete;
    Player const sender;
    TimePoint const time_point;
    UBYTE const data;
    bool const parity;
    ReadReceipt read_receipt = {};

    ByteMessage(Player player, UBYTE data, bool parity)
        : sender{player}
        , data{data}
        , time_point{Clock::now()}
        , read_receipt{0}
        , parity{parity} {
      MarkRead(player);
    }

    constexpr inline bool HasRead(Player player) const {
      return (read_receipt & (1 << player));
    }

    constexpr inline void MarkRead(Player player) {
      read_receipt |= (1 << player);
    }

    inline bool AllHaveRead() const {
      return read_receipt == s_read_receipt_complete;
    }

    constexpr inline void ResetRead() {
      read_receipt = 0;
    }
  };

  using Buffer = std::deque<ByteMessage>;

  inline ComLynx(Player n_players)
      : n_players_{n_players} {
    ByteMessage::s_read_receipt_complete = (1u << n_players) - 1u;
    errors_.resize(n_players);
    breaks_.resize(n_players);
    rx_int_en_.resize(n_players);
    tx_int_en_.resize(n_players);
  }

  constexpr void Configure(bool enable_parity, bool even_parity) {
    enable_parity_ = enable_parity;
    even_parity_ = even_parity;
    configured_ = true;
  }

  constexpr void Configure(ParityConfig config) {
    switch (config) {
      case ParityConfig::kOdd:
        Configure(true, false);
        return;
      case ParityConfig::kEven:
        Configure(true, true);
        return;
      case ParityConfig::kSpace:
        Configure(false, false);
        return;
      case ParityConfig::kMark:
        Configure(false, true);
        return;
    }
  }

  constexpr inline void EnableRxIRQ(Player player, bool value) {
    COMLYNX_ASSERT(configured_);
    rx_int_en_[player] = value;
  }

  constexpr inline void EnableTxIRQ(Player player, bool value) {
    COMLYNX_ASSERT(configured_);
    tx_int_en_[player] = value;
  }

  constexpr inline bool Send(Player player, UBYTE data) {
    COMLYNX_ASSERT(configured_);
    TxNotReadyReason reason = TxNotReadyReason::kNone;
    if (!IsTxReady(player, reason)) {
      switch (reason) {
        case TxNotReadyReason::kNone:
          COMLYNX_ASSERT("?");
          break;
        case TxNotReadyReason::kFrame:
          errors_[player].frame = true;
          break;
        case TxNotReadyReason::kOverrun:
          errors_[player].overrun = true;
          break;
      }
      return false;
    }

    buffer_.emplace_back(player, data, ParityFor(data));
    return true;
  }

  inline UBYTE Recv(Player player) {
    COMLYNX_ASSERT(configured_);
    COMLYNX_ASSERT(!buffer_.empty());

    auto msg_ptr = FirstUnreadMessage(player);
    COMLYNX_ASSERT(msg_ptr);

    // Mark as read and return for this player.
    auto &curr_msg = *msg_ptr;
    COMLYNX_ASSERT(!curr_msg.HasRead(player));
    curr_msg.MarkRead(player);

    // If this was the last reader.
    if (buffer_.front().AllHaveRead()) {
      buffer_.pop_front();
    }

    return curr_msg.data;
  }

  constexpr inline void SendBreak() {
    COMLYNX_ASSERT(configured_);

    // TODO: maybe not set it for the player themselves?

    for (size_t i = 0u; i < n_players_; ++i) {
      breaks_[i] = true;
    }
  }

  /// Player can only read when something new is available.
  inline bool IsRxReady(Player player) {
    COMLYNX_ASSERT(configured_);

    auto msg_ptr = FirstUnreadMessage(player);
    if (nullptr == msg_ptr) {
      return false;
    }
    if (msg_ptr->parity != CalculateParity(even_parity_, msg_ptr->data)) {
      errors_[player].parity = true;
    }
    return true;
  }

  inline ByteMessage *FirstUnreadMessage(Player player) {
    COMLYNX_ASSERT(configured_);

    for (auto &msg : buffer_) {
      if (!msg.HasRead(player)) {
        return &msg;
      }
    }
    return nullptr;
  }

  /// Player can only write after everything has been read.
  inline bool IsTxReady(Player player, TxNotReadyReason &reason) const {
    COMLYNX_ASSERT(configured_);

    auto const len = buffer_.size();
    if (len == 0) return true;

    if (len >= 32) {
      reason = TxNotReadyReason::kOverrun;
      return false;
    }

    return true;
  }

  inline bool IsTxEmpty(Player player) const {
    COMLYNX_ASSERT(configured_);

    for (auto const &msg : buffer_) {
      if (msg.sender == player) {
        return false;
      }
    }
    return true;
  }

  inline bool IsRxBrk(Player player) {
    COMLYNX_ASSERT(configured_);
    if (breaks_[player]) {
      breaks_[player] = false;
      return true;
    }
    return false;
  }

  inline bool IsIRQ(Player player) {
    if (rx_int_en_[player] && IsRxReady(player)) {
      return true;
    }
    TxNotReadyReason reason = {};
    if (tx_int_en_[player] && IsTxReady(player, reason)) {
      return true;
    }
    return false;
  }

  constexpr inline bool HasFrameError(Player player) const {
    COMLYNX_ASSERT(configured_);
    return errors_[player].frame;
  }

  constexpr inline bool HasOverrunError(Player player) const {
    COMLYNX_ASSERT(configured_);
    return errors_[player].overrun;
  }

  constexpr inline bool HasParityError(Player player) const {
    COMLYNX_ASSERT(configured_);
    return errors_[player].parity;
  }

  constexpr inline bool HasAnyError(Player player) const {
    COMLYNX_ASSERT(configured_);
    if (HasFrameError(player)) return true;
    if (HasOverrunError(player)) return true;
    if (HasParityError(player)) return true;
    return false;
  }

  constexpr inline void ResetErrors(Player player) {
    COMLYNX_ASSERT(configured_);
    errors_[player].Reset();
  }

  constexpr inline UBYTE GetSERCTL(Player player) {
    COMLYNX_ASSERT(configured_);

    TxNotReadyReason reason = {};
    auto const tx_ready = IsTxReady(player, reason);
    auto const rx_ready = IsRxReady(player);
    auto const tx_empty = IsTxEmpty(player);
    auto const &errors = errors_[player];
    auto const parity_bit = GetParityOfNextByte(player);

    return PackSERCTL(tx_ready, rx_ready, tx_empty, errors.parity,
                      errors.overrun, errors.frame, breaks_[player],
                      parity_bit);
  }

 private:
  int const n_players_;
  bool configured_ = false;
  bool enable_parity_ = {};
  bool even_parity_ = {};
  Buffer buffer_;
  std::vector<Error> errors_;
  std::vector<bool> breaks_;
  std::vector<bool> rx_int_en_;
  std::vector<bool> tx_int_en_;

  constexpr inline bool ParityFor(UBYTE byte) const {
    return (enable_parity_ ? CalculateParity(even_parity_, byte)
                           : even_parity_);
  }

  constexpr inline UBYTE PackSERCTL(bool tx_ready, bool rx_ready, bool tx_empty,
                                    bool parity_err, bool overrun_err,
                                    bool frame_err, bool rx_break,
                                    bool parity_bit) {
    UBYTE byte = {};
    byte |= tx_ready ? 0x80 : 0x00;
    byte |= rx_ready ? 0x40 : 0x00;
    byte |= tx_empty ? 0x20 : 0x00;
    byte |= parity_err ? 0x10 : 0x00;
    byte |= overrun_err ? 0x08 : 0x00;
    byte |= frame_err ? 0x04 : 0x00;
    byte |= rx_break ? 0x02 : 0x00;
    byte |= parity_bit ? 0x01 : 0x00;
    return byte;
  }

  inline bool PeekNextByte(Player player, UBYTE &byte) const {
    for (const auto &msg : buffer_) {
      if (!msg.HasRead(player)) {
        byte = msg.data;
        return true;
      }
    }
    return false;
  }

  constexpr inline bool GetParityOfNextByte(Player player) const {
    // TODO: maybe it just be of the previous byte

    UBYTE byte = 0;
    if (PeekNextByte(player, byte)) {
      return ParityFor(byte);
    }
    return false;
  }
};

class ComLynxClient {
 public:
  ComLynxClient(ComLynx &comlynx, ComLynx::Player player)
      : comlynx_{comlynx}
      , player_{player} {}

  constexpr void Configure(bool enable_parity, bool even_parity) {
    comlynx_.Configure(enable_parity, even_parity);
  }

  constexpr inline void EnableRxIRQ(bool value) {
    comlynx_.EnableRxIRQ(player_, value);
  }

  constexpr inline void EnableTxIRQ(bool value) {
    comlynx_.EnableTxIRQ(player_, value);
  }

  constexpr inline bool Send(UBYTE data) {
    return comlynx_.Send(player_, data);
  }

  constexpr inline UBYTE Recv() {
    return comlynx_.Recv(player_);
  }

  constexpr inline void SendBreak() {
    comlynx_.SendBreak();
  }

  /// Player can only read when something new is available.
  constexpr inline bool IsRxReady() {
    return comlynx_.IsRxReady(player_);
  }

  /// Player can only write after everything has been read.
  inline bool IsTxReady(ComLynx::TxNotReadyReason &reason) const {
    return comlynx_.IsTxReady(player_, reason);
  }

  inline bool IsTxEmpty() const {
    return comlynx_.IsTxEmpty(player_);
  }

  inline bool IsRxBrk() {
    return comlynx_.IsRxBrk(player_);
  }

  inline bool IsIRQ() {
    return comlynx_.IsIRQ(player_);
  }

  constexpr inline bool HasFrameError() const {
    return comlynx_.HasFrameError(player_);
  }

  constexpr inline bool HasOverrunError() const {
    return comlynx_.HasOverrunError(player_);
  }

  constexpr inline bool HasParityError() const {
    return comlynx_.HasParityError(player_);
  }

  constexpr inline bool HasAnyError() const {
    return comlynx_.HasAnyError(player_);
  }

  constexpr inline void ResetErrors() {
    comlynx_.ResetErrors(player_);
  }

  constexpr inline UBYTE GetSERCTL() {
    return comlynx_.GetSERCTL(player_);
  }

  constexpr inline ComLynx::Player GetPlayer() const {
    return player_;
  }

 private:
  ComLynx &comlynx_;
  ComLynx::Player const player_;
};

#endif  // SUPERKODER_COMLYNX_H
