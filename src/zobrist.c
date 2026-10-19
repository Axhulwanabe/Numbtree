#include <stdint.h>
#include "board.h"


uint64_t ZobristPieces[2][7][64];
uint64_t ZobristSide;
uint64_t ZobristCastle[4];
uint64_t ZobristEnPassant[8];

static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t RandomU64(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void InitZobrist(void) {
    for (int color = 0; color < 2; color++) {
        for (int type = 0; type < 7; type++) {
            for (int sq = 0; sq < 64; sq++) {
                ZobristPieces[color][type][sq] = RandomU64();
            }
        }
    }

    ZobristSide = RandomU64();

    for (int i = 0; i < 4; i++) {
        ZobristCastle[i] = RandomU64();
    }

    for (int file = 0; file < 8; file++) {
        ZobristEnPassant[file] = RandomU64();
    }
}

uint64_t PieceKey(int piece, int sq) {
    int color = (get_color(piece) == WHITE) ? 0 : 1;
    return ZobristPieces[color][get_piece_type(piece)][sq];
}

uint64_t CastleKey(const Board *board) {
    uint64_t key = 0;
    for (int i = 0; i < 4; i++) {
        if (board->castle_rights[i]) {
            key ^= ZobristCastle[i];
        }
    }
    return key;
}

uint64_t EnPassantKey(const Board *board) {
    if (!board->enpassant) {
        return 0;
    }

    int ep      = board->enpassant_target;
    int file    = ep % 8;
    int pushed  = board->white_to_move ? ep + 8 : ep - 8;   // the pawn that just moved two squares
    int ourPawn = (board->white_to_move ? WHITE : BLACK) | PAWN;

    if ((file > 0 && board->sq[pushed - 1] == ourPawn) ||
        (file < 7 && board->sq[pushed + 1] == ourPawn)) {
        return ZobristEnPassant[file];
    }
    return 0;
}

uint64_t ComputeHash(const Board *board) {
    uint64_t hash = 0;

    for (int sq = 0; sq < 64; sq++) {
        int piece = board->sq[sq];
        if (piece != NONE) {
            int color = (get_color(piece) == WHITE) ? 0 : 1;
            hash ^= ZobristPieces[color][get_piece_type(piece)][sq];
        }
    }

    if (!board->white_to_move) {
        hash ^= ZobristSide;
    }

    for (int i = 0; i < 4; i++) {
        if (board->castle_rights[i]) {
            hash ^= ZobristCastle[i];
        }
    }

    if (board->enpassant) {
        hash ^= ZobristEnPassant[board->enpassant_target % 8];
    }

    hash ^= EnPassantKey(board);

    return hash;
}