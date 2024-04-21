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
// The ABOVE COPYRIGHT notice and this permission notice SHALL BE INCLUDED in all
// copies or substantial portions of the Software.
//
// The software is provided "as is", without warranty of any kind, express or
// implied, including but not limited to the warranties of merchantability,
// fitness for a particular purpose and noninfringement. In no event shall the
// authors or copyright holders be liable for any claim, damages or other
// liability, whether in an action of contract, tort or otherwise, arising from,
// out of or in connection with the software or the use or other dealings in the
// software.
// ------------------------------------------------------------------------------ 

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::ElementsAre;

#include "comlynx.h"

std::vector<UBYTE> ReadAllSuccessfully(ComLynx &comlynx, ComLynx::Player player) {
    std::vector<UBYTE> ret;
    while (comlynx.IsRxReady(player)) {
        ret.push_back(comlynx.Recv(player));
    }
    return ret;
}

TEST(ComLynxTest, test_2p_simpleSend_p1_to_p2) {
    ComLynx comlynx(2);
    ComLynx::TxNotReadyReason reason = ComLynx::TxNotReadyReason::kNone;

    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    auto const sender = 0;
    auto const receiver = 1;
    
    EXPECT_TRUE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10100000);
    comlynx.Send(sender, 'A');
    
    EXPECT_FALSE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10000000);
    EXPECT_EQ(comlynx.GetSERCTL(receiver), 0b11100001);
    comlynx.Send(sender, 'B');

    EXPECT_FALSE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10000000);
    EXPECT_EQ(comlynx.GetSERCTL(receiver), 0b11100001);
    comlynx.Send(sender, 'C');

    EXPECT_FALSE(comlynx.HasAnyError(sender));
    EXPECT_FALSE(comlynx.IsRxReady(sender));
    
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'A');
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10000000);
    EXPECT_EQ(comlynx.GetSERCTL(receiver), 0b11100001);
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'B');
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10000000);
    EXPECT_EQ(comlynx.GetSERCTL(receiver), 0b11100000);
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'C');
    EXPECT_EQ(comlynx.GetSERCTL(sender), 0b10100000);
    EXPECT_EQ(comlynx.GetSERCTL(receiver), 0b10100000);

    EXPECT_FALSE(comlynx.IsRxReady(sender));
    EXPECT_FALSE(comlynx.IsRxReady(receiver));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(sender));
}

TEST(ComLynxTest, test_2p_simpleSend_p2_to_p1) {
    ComLynx comlynx(2);
    ComLynx::TxNotReadyReason reason = ComLynx::TxNotReadyReason::kNone;
    
    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    auto const sender = 1;
    auto const receiver = 0;
    
    EXPECT_TRUE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    comlynx.Send(sender, 'A');
    
    EXPECT_FALSE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    comlynx.Send(sender, 'B');

    EXPECT_FALSE(comlynx.IsTxEmpty(sender));
    EXPECT_TRUE(comlynx.IsTxReady(sender, reason));
    comlynx.Send(sender, 'C');

    EXPECT_FALSE(comlynx.HasAnyError(sender));
    EXPECT_FALSE(comlynx.IsRxReady(sender));
    
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'A');
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'B');
    EXPECT_TRUE(comlynx.IsRxReady(receiver));
    EXPECT_EQ(comlynx.Recv(receiver), 'C');
    
    EXPECT_FALSE(comlynx.IsRxReady(sender));
    EXPECT_FALSE(comlynx.IsRxReady(receiver));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(sender));
}

TEST(ComLynxTest, test_3p_roundRobin) {
    ComLynx comlynx(3);
    ComLynx::TxNotReadyReason reason = ComLynx::TxNotReadyReason::kNone;
    
    comlynx.Configure(ComLynx::ParityConfig::kOdd);
    
    auto const P1 = 0;
    auto const P2 = 1;
    auto const P3 = 2;
    
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_FALSE(comlynx.IsRxReady(P2));
    EXPECT_FALSE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxReady(P1, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
    
    // P1 talks
    comlynx.Send(P1, 'A');
    comlynx.Send(P1, 'B');
    EXPECT_FALSE(comlynx.HasAnyError(P1));
    
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxReady(P1, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
    
    EXPECT_FALSE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
    
    // P2 & P3 read 'A'
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_EQ(comlynx.Recv(P2), 'A');
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_EQ(comlynx.Recv(P3), 'A');
    
    // P2 & P3 read 'B'
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_EQ(comlynx.Recv(P2), 'B');
    EXPECT_FALSE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_EQ(comlynx.Recv(P3), 'B');
    
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_FALSE(comlynx.IsRxReady(P2));
    EXPECT_FALSE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxReady(P1, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
    
    // P2 talks
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    comlynx.Send(P2, 'C');
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    comlynx.Send(P2, 'D');
    EXPECT_FALSE(comlynx.HasAnyError(P2));
    
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_FALSE(comlynx.IsRxReady(P2));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_FALSE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
    
    // P1 & P3 read 'C'
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_EQ(comlynx.Recv(P3), 'C');
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_EQ(comlynx.Recv(P1), 'C');
    
    // P1 & P3 read 'D'
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_EQ(comlynx.Recv(P1), 'D');
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P3));
    EXPECT_EQ(comlynx.Recv(P3), 'D');
    
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
    
    // P3 talks
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
    comlynx.Send(P3, 'E');
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
    comlynx.Send(P3, 'F');
    EXPECT_FALSE(comlynx.HasAnyError(P3));
    
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_FALSE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_FALSE(comlynx.IsTxEmpty(P3));
    
    // P1 & P2 read 'E'
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_EQ(comlynx.Recv(P1), 'E');
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_EQ(comlynx.Recv(P2), 'E');
    
    // P1 & P2 read 'F'
    EXPECT_TRUE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_EQ(comlynx.Recv(P1), 'F');
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_TRUE(comlynx.IsRxReady(P2));
    EXPECT_EQ(comlynx.Recv(P2), 'F');
    
    EXPECT_FALSE(comlynx.HasAnyError(P1));
    EXPECT_FALSE(comlynx.HasAnyError(P2));
    EXPECT_FALSE(comlynx.HasAnyError(P3));
    
    EXPECT_FALSE(comlynx.IsRxReady(P1));
    EXPECT_FALSE(comlynx.IsRxReady(P2));
    EXPECT_FALSE(comlynx.IsRxReady(P3));
    
    EXPECT_TRUE(comlynx.IsTxReady(P1, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P2, reason));
    EXPECT_TRUE(comlynx.IsTxReady(P3, reason));
 
    EXPECT_TRUE(comlynx.IsTxEmpty(P1));
    EXPECT_TRUE(comlynx.IsTxEmpty(P2));
    EXPECT_TRUE(comlynx.IsTxEmpty(P3));
}

TEST(ComLynxTest, test_parity) {
    EXPECT_EQ(CalculateOddParity (0), true);
    EXPECT_EQ(CalculateEvenParity(0), false);
    
    EXPECT_EQ(CalculateOddParity (1), false);
    EXPECT_EQ(CalculateEvenParity(1), true);
    
    EXPECT_EQ(CalculateOddParity (0b11111111), true);
    EXPECT_EQ(CalculateEvenParity(0b11111111), false);
    
    EXPECT_EQ(CalculateOddParity (0b10101010), true);
    EXPECT_EQ(CalculateEvenParity(0b10101010), false);
    
    EXPECT_EQ(CalculateOddParity (0b10101011), false);
    EXPECT_EQ(CalculateEvenParity(0b10101011), true);
}

TEST(ComLynxTest, test_common_checksum) {
    
    // Slime World:
    EXPECT_EQ(ComLynxCommonChecksum({0x05, 0x00, 0x00, 0x01, 0x05, 0x00}), 0xF4);
    EXPECT_EQ(ComLynxCommonChecksum({0x05, 0x00, 0x01, 0x03, 0x05, 0x00}), 0xF1);
    
    // Gauntlet The Third Encounter:
    EXPECT_EQ(ComLynxCommonChecksum({5, 0, 0, 1, 1, 0}), 0xF8);
    EXPECT_EQ(ComLynxCommonChecksum({5, 0, 1, 3, 1, 0}), 0xF5);
}

TEST(ComLynxTest, test_handshake_slime_world) {
    ComLynx comlynx(2);
    ComLynx::TxNotReadyReason reason = {};
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    // Slime World: 
    //  - P1: { 05 00 00 01 05 00 F4 }
    //  - P2: { 05 00 01 03 05 00 F1 }
    // (form https://github.com/superKoder/lynx_game_info)
    
    // L1 wants to be P1, L2 wants to be P2
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x05));
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x00));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));  // !!!
    EXPECT_TRUE(comlynx.Send(L2, 0x05));         // !!!
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x00));
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x01));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));  // !!!
    EXPECT_TRUE(comlynx.Send(L2, 0x00));         // !!!
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x05));
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0x00));
    EXPECT_TRUE(comlynx.IsTxReady(L1, reason));
    EXPECT_TRUE(comlynx.Send(L1, 0xF4));
    
    EXPECT_THAT(ReadAllSuccessfully(comlynx, L2), ElementsAre(0x05, 0x00, 0x00, 0x01, 0x05, 0x00, 0xF4));
    
    // continuation of L2 wanting to be P2
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));
    EXPECT_TRUE(comlynx.Send(L2, 0x01));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));
    EXPECT_TRUE(comlynx.Send(L2, 0x03));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));
    EXPECT_TRUE(comlynx.Send(L2, 0x05));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));
    EXPECT_TRUE(comlynx.Send(L2, 0x00));
    EXPECT_TRUE(comlynx.IsTxReady(L2, reason));
    EXPECT_TRUE(comlynx.Send(L2, 0xF1));
    
    EXPECT_THAT(ReadAllSuccessfully(comlynx, L1), ElementsAre(0x05, 0x00, 0x01, 0x03, 0x05, 0x00, 0xF1));
}

TEST(ComLynxTest, test_parity_even) {
    ComLynx comlynx(2);
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    comlynx.Configure(ComLynx::ParityConfig::kEven);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101011)); // even parity = 1
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101010)); // even parity = 0
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);
}

TEST(ComLynxTest, test_parity_odd) {
    ComLynx comlynx(2);
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101011)); // odd parity = 0
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101010)); // odd parity = 1
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);
}

TEST(ComLynxTest, test_parity_mark) {
    ComLynx comlynx(2);
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    comlynx.Configure(ComLynx::ParityConfig::kMark);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101011)); // even parity = 1
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101010)); // even parity = 0
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_TRUE(comlynx.HasAnyError(L2));
    EXPECT_TRUE(comlynx.HasParityError(L2));
    comlynx.ResetErrors(L2);
    comlynx.Recv(L2);
}

TEST(ComLynxTest, test_parity_space) {
    ComLynx comlynx(2);
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    comlynx.Configure(ComLynx::ParityConfig::kSpace);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101011)); // odd parity = 0
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_FALSE(comlynx.HasAnyError(L2));
    comlynx.Recv(L2);

    EXPECT_TRUE(comlynx.Send(L1, 0b10101010)); // odd parity = 1
    EXPECT_FALSE(comlynx.HasAnyError(L1));
    
    EXPECT_TRUE(comlynx.IsRxReady(L2));
    EXPECT_TRUE(comlynx.HasAnyError(L2));
    EXPECT_TRUE(comlynx.HasParityError(L2));
    comlynx.ResetErrors(L2);
    comlynx.Recv(L2);
}

TEST(ComLynxTest, test_break) {
    ComLynx comlynx(3);
    ComLynx::Player const L1 = 0;
    ComLynx::Player const L2 = 1;
    ComLynx::Player const L3 = 2;
    comlynx.Configure(ComLynx::ParityConfig::kSpace);
    
    EXPECT_FALSE(comlynx.IsRxBrk(L1));
    EXPECT_FALSE(comlynx.IsRxBrk(L2));
    EXPECT_FALSE(comlynx.IsRxBrk(L3));

    comlynx.SendBreak();
    EXPECT_TRUE(comlynx.IsRxBrk(L1));
    EXPECT_TRUE(comlynx.IsRxBrk(L2));
    EXPECT_TRUE(comlynx.IsRxBrk(L3));

    // only set once
    EXPECT_FALSE(comlynx.IsRxBrk(L1));
    EXPECT_FALSE(comlynx.IsRxBrk(L2));
    EXPECT_FALSE(comlynx.IsRxBrk(L3));
}

TEST(ComLynxTest, test_interrupt) {
    ComLynx comlynx(2);
    ComLynx::TxNotReadyReason reason = {};
    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    ComLynxClient sender(comlynx, 0);
    ComLynxClient recver(comlynx, 1);

    EXPECT_FALSE(sender.IsIRQ());
    EXPECT_FALSE(recver.IsIRQ());
    sender.EnableRxIRQ(true);
    sender.EnableTxIRQ(true);
    recver.EnableRxIRQ(true);
    recver.EnableTxIRQ(true);
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());

    sender.Send('A');
    
    EXPECT_FALSE(sender.IsTxEmpty());
    EXPECT_TRUE(sender.IsTxReady(reason));
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());
    sender.Send('B');

    EXPECT_FALSE(sender.IsTxEmpty());
    EXPECT_TRUE(sender.IsTxReady(reason));
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());
    sender.Send('C');

    EXPECT_FALSE(sender.HasAnyError());
    EXPECT_FALSE(sender.IsRxReady());
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());

    EXPECT_TRUE(recver.IsRxReady());
    EXPECT_EQ(recver.Recv(), 'A');
    EXPECT_TRUE(recver.IsRxReady());
    EXPECT_EQ(recver.Recv(), 'B');
    EXPECT_TRUE(recver.IsRxReady());
    EXPECT_EQ(recver.Recv(), 'C');

    EXPECT_FALSE(sender.IsRxReady());
    EXPECT_FALSE(recver.IsRxReady());
    EXPECT_TRUE(sender.IsTxReady(reason));
    EXPECT_TRUE(recver.IsTxReady(reason));
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());

    EXPECT_FALSE(sender.IsRxReady());
    EXPECT_FALSE(recver.IsRxReady());
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());

    EXPECT_TRUE(sender.IsTxEmpty());
    EXPECT_TRUE(sender.IsIRQ());
    EXPECT_TRUE(recver.IsIRQ());
}

TEST(ComLynxTest, test_lynxbug_recv_own_sent) {
    ComLynx comlynx(2);
    ComLynx::TxNotReadyReason reason = {};
    comlynx.Configure(ComLynx::ParityConfig::kOdd);

    ComLynxClient P1(comlynx, 0);
    ComLynxClient P2(comlynx, 1);

    EXPECT_TRUE(P1.IsTxEmpty());
    EXPECT_TRUE(P2.IsTxEmpty());
    EXPECT_TRUE(P1.IsTxReady(reason));
    EXPECT_TRUE(P2.IsTxReady(reason));
    EXPECT_FALSE(P1.IsRxReady());
    EXPECT_FALSE(P2.IsRxReady());
    
    P1.Send('A');
    EXPECT_FALSE(P1.IsRxReady());
    EXPECT_TRUE(P2.IsRxReady());

    P2.Send('B');
    EXPECT_TRUE(P1.IsRxReady());
    EXPECT_TRUE(P2.IsRxReady());

    P2.Recv();  // P1 reads without RxReady!
}
