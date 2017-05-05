//
// Created by Shawn Roach on 3/1/2017.
//
#include <string>
#include <iostream>
#include <assert.h>
#include <bitset>
#include "BitMath.h"


#include <stdio.h>
#include <stdlib.h>

namespace BitBoard {
    u64 toBitIndex(const std::string &file, const int rank) {
        //row = rank - 1;//start at 0 instead of 1
        //col = file[0] - 'a';
        return 1ull << ((rank - 1) * 8 + (file[0] - 'a'));
    }

    u64 toBitIndex(int row, int col) {
        return 1ull << (row * 8 + col);
    }


    void printAsBoard(const std::string &boardTitle, const u64 board) {
        std::cout << boardTitle << '\n';
        printAsBoard(board);
    }

    uint32_t toTableIndex(const u64 bitIndex) {
        return __builtin_ctzll(bitIndex);
    }


    void printAsBoard(const u64 board) {
        unsigned count = 0;
        std::cout << "   ";
        for (unsigned x = 0; x < 8; x++)
            printf("%*c", -2, x + 'a');
        std::cout << '\n';
        for (int x = 8 - 1; x >= 0; x--) {
            printf("%*u ", 2, 8 - count++);
            for (int q = 0; q < 8; q++) {
                u64 index = 1ull << (x * 8 + q);
                if (board & index)
                    std::cout << '1';
                else
                    std::cout << '.';
                std::cout << ' ';
            }
            std::cout << '\n';
        }
    }

    void printAsBoard(const std::vector<std::string> &boardTitle, const std::vector<u64> &board) {
        unsigned count = 0;
        std::cout << "   ";
        for (int x = 0; x < boardTitle.size(); x++)
            printf("%-17s", boardTitle[x].c_str());
        std::cout << "\n";
        std::cout << "   ";
        for (int a = 0; a < board.size(); a++) {

            for (unsigned x = 0; x < 8; x++)
                printf("%*c", -2, x + 'a');
            std::cout << " ";
        }
        std::cout << '\n';
        for (int x = 8 - 1; x >= 0; x--) {
            printf("%*u ", 2, 8 - count++);
            for (int a = 0; a < board.size(); a++) {
                for (int q = 0; q < 8; q++) {
                    u64 index = 1ull << (x * 8 + q);
                    if (board[a] & index)
                        std::cout << '1';
                    else
                        std::cout << '.';
                    std::cout << ' ';
                }
                std::cout << " ";
            }
            std::cout << '\n';
        }
    }


    unsigned toRankIndex(const u64 board) {
        return __builtin_ctzll(board) / 8;
    }

    unsigned toFileIndex(const u64 board) {
        return __builtin_ctzll(board) % 8;
    }

    std::string toFile(const u64 i) {
        return std::string(1, (char) (toFileIndex(i) + 'a'));
    }

    int toRank(const u64 i) {
        return toRankIndex(i) + 1;
    }

    //http://chessprogramming.wikispaces.com/magic+bitboards#cite_note-13
    //http://www.talkchess.com/forum/viewtopic.php?t=60065&start=14
    //http://www.open-aurec.com/wbforum/viewtopic.php?f=4&t=51162
    u64 magicLookUp[89524];

    constexpr fixedShiftFancyMagic rookMagics[64] = {
            {magicLookUp + 41305, 0x101010101017eULL,    0x280077ffebfffeULL},
            {magicLookUp + 14326, 0x202020202027cULL,    0x2004010201097fffULL},
            {magicLookUp + 24477, 0x404040404047aULL,    0x10020010053fffULL},
            {magicLookUp + 8223,  0x8080808080876ULL,    0x30002ff71ffffaULL},
            {magicLookUp + 49795, 0x1010101010106eULL,   0x7fd00441ffffd003ULL},
            {magicLookUp + 60546, 0x2020202020205eULL,   0x4001d9e03ffff7ULL},
            {magicLookUp + 28543, 0x4040404040403eULL,   0x4000888847ffffULL},
            {magicLookUp + 79282, 0x8080808080807eULL,   0x6800fbff75fffdULL},
            {magicLookUp + 6457,  0x1010101017e00ULL,    0x28010113ffffULL},
            {magicLookUp + 4125,  0x2020202027c00ULL,    0x20040201fcffffULL},
            {magicLookUp + 81021, 0x4040404047a00ULL,    0x7fe80042ffffe8ULL},
            {magicLookUp + 42341, 0x8080808087600ULL,    0x1800217fffe8ULL},
            {magicLookUp + 14139, 0x10101010106e00ULL,   0x1800073fffe8ULL},
            {magicLookUp + 19465, 0x20202020205e00ULL,   0x7fe8009effffe8ULL},
            {magicLookUp + 9514,  0x40404040403e00ULL,   0x1800602fffe8ULL},
            {magicLookUp + 71090, 0x80808080807e00ULL,   0x30002fffffa0ULL},
            {magicLookUp + 75419, 0x10101017e0100ULL,    0x300018010bffffULL},
            {magicLookUp + 33476, 0x20202027c0200ULL,    0x3000c0085fffbULL},
            {magicLookUp + 27117, 0x40404047a0400ULL,    0x4000802010008ULL},
            {magicLookUp + 85964, 0x8080808760800ULL,    0x2002004002002ULL},
            {magicLookUp + 54915, 0x101010106e1000ULL,   0x2002020010002ULL},
            {magicLookUp + 36544, 0x202020205e2000ULL,   0x1002020008001ULL},
            {magicLookUp + 71854, 0x404040403e4000ULL,   0x4040008001ULL},
            {magicLookUp + 37996, 0x808080807e8000ULL,   0x802000200040ULL},
            {magicLookUp + 30398, 0x101017e010100ULL,    0x40200010080010ULL},
            {magicLookUp + 55939, 0x202027c020200ULL,    0x80010040010ULL},
            {magicLookUp + 53891, 0x404047a040400ULL,    0x4010008020008ULL},
            {magicLookUp + 56963, 0x8080876080800ULL,    0x40020200200ULL},
            {magicLookUp + 77451, 0x1010106e101000ULL,   0x10020020020ULL},
            {magicLookUp + 12319, 0x2020205e202000ULL,   0x10020200080ULL},
            {magicLookUp + 88500, 0x4040403e404000ULL,   0x8020200040ULL},
            {magicLookUp + 51405, 0x8080807e808000ULL,   0x200020004081ULL},
            {magicLookUp + 72878, 0x1017e01010100ULL,    0xfffd1800300030ULL},
            {magicLookUp + 676,   0x2027c02020200ULL,    0x7fff7fbfd40020ULL},
            {magicLookUp + 83122, 0x4047a04040400ULL,    0x3fffbd00180018ULL},
            {magicLookUp + 22206, 0x8087608080800ULL,    0x1fffde80180018ULL},
            {magicLookUp + 75186, 0x10106e10101000ULL,   0xfffe0bfe80018ULL},
            {magicLookUp + 681,   0x20205e20202000ULL,   0x1000080202001ULL},
            {magicLookUp + 36453, 0x40403e40404000ULL,   0x3fffbff980180ULL},
            {magicLookUp + 20369, 0x80807e80808000ULL,   0x1fffdff9000e0ULL},
            {magicLookUp + 1981,  0x17e0101010100ULL,    0xfffeebfeffd800ULL},
            {magicLookUp + 13343, 0x27c0202020200ULL,    0x7ffff7ffc01400ULL},
            {magicLookUp + 10650, 0x47a0404040400ULL,    0x408104200204ULL},
            {magicLookUp + 57987, 0x8760808080800ULL,    0x1ffff01fc03000ULL},
            {magicLookUp + 26302, 0x106e1010101000ULL,   0xfffe7f8bfe800ULL},
            {magicLookUp + 58357, 0x205e2020202000ULL,   0x8001002020ULL},
            {magicLookUp + 40546, 0x403e4040404000ULL,   0x3fff85fffa804ULL},
            {magicLookUp + 0,     0x807e8080808000ULL,   0x1fffd75ffa802ULL},
            {magicLookUp + 14967, 0x7e010101010100ULL,   0xffffec00280028ULL},
            {magicLookUp + 80361, 0x7c020202020200ULL,   0x7fff75ff7fbfd8ULL},
            {magicLookUp + 40905, 0x7a040404040400ULL,   0x3fff863fbf7fd8ULL},
            {magicLookUp + 58347, 0x76080808080800ULL,   0x1fffbfdfd7ffd8ULL},
            {magicLookUp + 20381, 0x6e101010101000ULL,   0xffff810280028ULL},
            {magicLookUp + 81868, 0x5e202020202000ULL,   0x7ffd7f7feffd8ULL},
            {magicLookUp + 59381, 0x3e404040404000ULL,   0x3fffc0c480048ULL},
            {magicLookUp + 84404, 0x7e808080808000ULL,   0x1ffffafd7ffd8ULL},
            {magicLookUp + 45811, 0x7e01010101010100ULL, 0xffffe4ffdfa3baULL},
            {magicLookUp + 62898, 0x7c02020202020200ULL, 0x7fffef7ff3d3daULL},
            {magicLookUp + 45796, 0x7a04040404040400ULL, 0x3fffbfdfeff7faULL},
            {magicLookUp + 66994, 0x7608080808080800ULL, 0x1fffeff7fbfc22ULL},
            {magicLookUp + 67204, 0x6e10101010101000ULL, 0x20408001001ULL},
            {magicLookUp + 32448, 0x5e20202020202000ULL, 0x7fffeffff77fdULL},
            {magicLookUp + 62946, 0x3e40404040404000ULL, 0x3ffffbf7dfeecULL},
            {magicLookUp + 17005, 0x7e80808080808000ULL, 0x1ffff9dffa333ULL},
    };

    constexpr fixedShiftFancyMagic bishopMagics[64] = {
            {magicLookUp + 33104, 0x40201008040200ULL, 0x404040404040ULL},
            {magicLookUp + 4094,  0x402010080400ULL,   0xa060401007fcULL},
            {magicLookUp + 24764, 0x4020100a00ULL,     0x401020200000ULL},
            {magicLookUp + 13882, 0x40221400ULL,       0x806004000000ULL},
            {magicLookUp + 23090, 0x2442800ULL,        0x440200000000ULL},
            {magicLookUp + 32640, 0x204085000ULL,      0x80100800000ULL},
            {magicLookUp + 11558, 0x20408102000ULL,    0x104104004000ULL},
            {magicLookUp + 32912, 0x2040810204000ULL,  0x20020820080ULL},
            {magicLookUp + 13674, 0x20100804020000ULL, 0x40100202004ULL},
            {magicLookUp + 6109,  0x40201008040000ULL, 0x20080200802ULL},
            {magicLookUp + 26494, 0x4020100a0000ULL,   0x10040080200ULL},
            {magicLookUp + 17919, 0x4022140000ULL,     0x8060040000ULL},
            {magicLookUp + 25757, 0x244280000ULL,      0x4402000000ULL},
            {magicLookUp + 17338, 0x20408500000ULL,    0x21c100b200ULL},
            {magicLookUp + 16983, 0x2040810200000ULL,  0x400410080ULL},
            {magicLookUp + 16659, 0x4081020400000ULL,  0x3f7f05fffc0ULL},
            {magicLookUp + 13610, 0x10080402000200ULL, 0x4228040808010ULL},
            {magicLookUp + 2224,  0x20100804000400ULL, 0x200040404040ULL},
            {magicLookUp + 60405, 0x4020100a000a00ULL, 0x400080808080ULL},
            {magicLookUp + 7983,  0x402214001400ULL,   0x200200801000ULL},
            {magicLookUp + 17,    0x24428002800ULL,    0x240080840000ULL},
            {magicLookUp + 34321, 0x2040850005000ULL,  0x18000c03fff8ULL},
            {magicLookUp + 33216, 0x4081020002000ULL,  0xa5840208020ULL},
            {magicLookUp + 17127, 0x8102040004000ULL,  0x58408404010ULL},
            {magicLookUp + 6397,  0x8040200020400ULL,  0x2022000408020ULL},
            {magicLookUp + 22169, 0x10080400040800ULL, 0x402000408080ULL},
            {magicLookUp + 42727, 0x20100a000a1000ULL, 0x804000810100ULL},
            {magicLookUp + 155,   0x40221400142200ULL, 0x100403c0403ffULL},
            {magicLookUp + 8601,  0x2442800284400ULL,  0x78402a8802000ULL},
            {magicLookUp + 21101, 0x4085000500800ULL,  0x101000804400ULL},
            {magicLookUp + 29885, 0x8102000201000ULL,  0x80800104100ULL},
            {magicLookUp + 29340, 0x10204000402000ULL, 0x400480101008ULL},
            {magicLookUp + 19785, 0x4020002040800ULL,  0x1010102004040ULL},
            {magicLookUp + 12258, 0x8040004081000ULL,  0x808090402020ULL},
            {magicLookUp + 50451, 0x100a000a102000ULL, 0x7fefe08810010ULL},
            {magicLookUp + 1712,  0x22140014224000ULL, 0x3ff0f833fc080ULL},
            {magicLookUp + 78475, 0x44280028440200ULL, 0x7fe08019003042ULL},
            {magicLookUp + 7855,  0x8500050080400ULL,  0x202040008040ULL},
            {magicLookUp + 13642, 0x10200020100800ULL, 0x1004008381008ULL},
            {magicLookUp + 8156,  0x20400040201000ULL, 0x802003700808ULL},
            {magicLookUp + 4348,  0x2000204081000ULL,  0x208200400080ULL},
            {magicLookUp + 28794, 0x4000408102000ULL,  0x104100200040ULL},
            {magicLookUp + 22578, 0xa000a10204000ULL,  0x3ffdf7f833fc0ULL},
            {magicLookUp + 50315, 0x14001422400000ULL, 0x8840450020ULL},
            {magicLookUp + 85452, 0x28002844020000ULL, 0x20040100100ULL},
            {magicLookUp + 32816, 0x50005008040200ULL, 0x7fffdd80140028ULL},
            {magicLookUp + 13930, 0x20002010080400ULL, 0x202020200040ULL},
            {magicLookUp + 17967, 0x40004020100800ULL, 0x1004010039004ULL},
            {magicLookUp + 33200, 0x20408102000ULL,    0x40041008000ULL},
            {magicLookUp + 32456, 0x40810204000ULL,    0x3ffefe0c02200ULL},
            {magicLookUp + 7762,  0xa1020400000ULL,    0x1010806000ULL},
            {magicLookUp + 7794,  0x142240000000ULL,   0x8403000ULL},
            {magicLookUp + 22761, 0x284402000000ULL,   0x100202000ULL},
            {magicLookUp + 14918, 0x500804020000ULL,   0x40100200800ULL},
            {magicLookUp + 11620, 0x201008040200ULL,   0x404040404000ULL},
            {magicLookUp + 15925, 0x402010080400ULL,   0x6020601803f4ULL},
            {magicLookUp + 32528, 0x2040810204000ULL,  0x3ffdfdfc28048ULL},
            {magicLookUp + 12196, 0x4081020400000ULL,  0x820820020ULL},
            {magicLookUp + 32720, 0xa102040000000ULL,  0x10108060ULL},
            {magicLookUp + 26781, 0x14224000000000ULL, 0x84030ULL},
            {magicLookUp + 19817, 0x28440200000000ULL, 0x1002020ULL},
            {magicLookUp + 24732, 0x50080402000000ULL, 0x40408020ULL},
            {magicLookUp + 25468, 0x20100804020000ULL, 0x4040404040ULL},
            {magicLookUp + 10186, 0x40201008040200ULL, 0x404040404040ULL},
    };

    u64 getLegalRookAttacks(const u64 occ, const u64 bitIndex) {
        const auto &e = rookMagics[toTableIndex(bitIndex)];
        return e.resultTableOffset[(((occ & e.mask) * e.magic) >> (64 - 12))];
    }

    u64 getLegalBishopAttacks(const u64 occ, const u64 bitIndex) {
        const auto &e = bishopMagics[toTableIndex(bitIndex)];
        return e.resultTableOffset[(((occ & e.mask) * e.magic) >> (64 - 9))];
    }


    //http://chessprogramming.wikispaces.com/Looking+for+Magics

    int count_1s(u64 b) {
        int r;
        for (r = 0; b; r++, b &= b - 1);
        return r;
    }

    const int BitTable[64] = {63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2, 51, 21, 43, 45, 10, 18, 47, 1, 54,
                              9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52, 26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38,
                              28, 58, 20, 37, 17, 36, 8};

    int pop_1st_bit(u64 *bb) {
        u64 b = *bb ^(*bb - 1);
        unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
        *bb &= (*bb - 1);
        return BitTable[(fold * 0x783a9b23) >> 26];
    }

    u64 index_to_uint64(int index, int bits, u64 m) {
        int i, j;
        u64 result = 0ULL;
        for (i = 0; i < bits; i++) {
            j = pop_1st_bit(&m);
            if (index & (1 << i)) result |= (1ULL << j);
        }
        return result;
    }

    u64 ratt(int sq, u64 block) {
        u64 result = 0ULL;
        int rk = sq / 8, fl = sq % 8, r, f;
        for (r = rk + 1; r <= 7; r++) {
            result |= (1ULL << (fl + r * 8));
            if (block & (1ULL << (fl + r * 8))) break;
        }
        for (r = rk - 1; r >= 0; r--) {
            result |= (1ULL << (fl + r * 8));
            if (block & (1ULL << (fl + r * 8))) break;
        }
        for (f = fl + 1; f <= 7; f++) {
            result |= (1ULL << (f + rk * 8));
            if (block & (1ULL << (f + rk * 8))) break;
        }
        for (f = fl - 1; f >= 0; f--) {
            result |= (1ULL << (f + rk * 8));
            if (block & (1ULL << (f + rk * 8))) break;
        }
        return result;
    }

    u64 batt(int sq, u64 block) {
        u64 result = 0ULL;
        int rk = sq / 8, fl = sq % 8, r, f;
        for (r = rk + 1, f = fl + 1; r <= 7 && f <= 7; r++, f++) {
            result |= (1ULL << (f + r * 8));
            if (block & (1ULL << (f + r * 8))) break;
        }
        for (r = rk + 1, f = fl - 1; r <= 7 && f >= 0; r++, f--) {
            result |= (1ULL << (f + r * 8));
            if (block & (1ULL << (f + r * 8))) break;
        }
        for (r = rk - 1, f = fl + 1; r >= 0 && f <= 7; r--, f++) {
            result |= (1ULL << (f + r * 8));
            if (block & (1ULL << (f + r * 8))) break;
        }
        for (r = rk - 1, f = fl - 1; r >= 0 && f >= 0; r--, f--) {
            result |= (1ULL << (f + r * 8));
            if (block & (1ULL << (f + r * 8))) break;
        }
        return result;
    }

    int transform(u64 b, u64 magic, int bits) {
        return (int) ((b * magic) >> (64 - bits));
    }

    void createMagicLookupTable() {
        //for (int x = 0; x < 89524; x++)magicLookUp[x] = 0ull;

        for (int bishop = 0; bishop <= 1; bishop++) {
            for (int sq = 0; sq < 64; sq++) {
                const fixedShiftFancyMagic &e = bishop ? bishopMagics[sq] : rookMagics[sq];
                u64 *offset = e.resultTableOffset;
                u64 mask = e.mask;
                int popCount = count_1s(mask);
                for (int i = 0; i < (1 << popCount); i++) {
                    u64 magic = e.magic;
                    u64 b = index_to_uint64(i, popCount, mask);
                    u64 a = bishop ? batt(sq, b) : ratt(sq, b);
                    int j = transform(b, magic, bishop ? 9 : 12);
                    //if (offset[j] == 0ULL)
                    offset[j] = a;
                    //else if (offset[j] != a) { std::cout << "error!\n"; }
                    /*
                    u64 q = bishop ? getLegalBishopAttacks(b, 1ull << sq) : getLegalRookAttacks(b, 1ull << sq);
                    if (a != q) {
                        std::cout << "fail " << bishop << "\n";
                        std::cout << sq << '\n';
                        std::cout << toTableIndex(1<<sq)<<'\n';
                        printAsBoard("block", b);
                        printAsBoard("mask", mask);
                        printAsBoard("attackable", a);
                        printAsBoard("aquired result", q);
                    }
                     */

                }
            }
        }
    }


}
