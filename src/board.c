#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"

int fen_board[BOARD_SIZE] = {0};

int DirectionOffsets[8] = {-8, 8, -1, 1, -9, 9, -7, 7};
int NumbSquaresToEdge[64][8];

#define MAX_HISTORY 2048

static uint64_t History[MAX_HISTORY];   
static int HistoryCount = 0;

void ClearHistory(void) {
    HistoryCount = 0;
}

void PushHistory(uint64_t hash) {
    if (HistoryCount < MAX_HISTORY) {
        History[HistoryCount++] = hash;
    }
}

int HistorySize(void) {
    return HistoryCount;
}

void SetHistorySize(int size) {
    HistoryCount = size;
}

bool IsRepetition(const Board *board) {
    int oldest = HistoryCount - board->halfmove;
    if (oldest < 0) {
        oldest = 0;
    }

    for (int i = HistoryCount - 4; i >= oldest; i -= 2) {
        if (History[i] == board->hash) {
            return true;
        }
    }
    return false;
}

static bool InsufficientMaterial(const Board *board) {
    int minors = 0;

    for (int sq = 0; sq < 64; sq++) {
        int type = get_piece_type(board->sq[sq]);

        if (type == PAWN || type == ROOK || type == QUEEN) {
            return false;
        }
        if (type == KNIGHT || type == BISHOP) {
            minors++;
        }
    }

    return minors <= 1;
}

bool IsDraw(const Board *board) {
    if (board->halfmove >= 100) {
        return true;
    }
    if (IsRepetition(board)) {
        return true;
    }
    return InsufficientMaterial(board);
}

void PrecomputedMoveData(void) {
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            int numNorth = rank;
            int numSouth = 7 - rank;
            int numWest = file;
            int numEast = 7 - file;

            int squareIndex = rank * 8 + file;

            // Straight directions
            NumbSquaresToEdge[squareIndex][0] = numNorth;
            NumbSquaresToEdge[squareIndex][1] = numSouth;
            NumbSquaresToEdge[squareIndex][2] = numWest;
            NumbSquaresToEdge[squareIndex][3] = numEast;

            // Diagonal directions
            NumbSquaresToEdge[squareIndex][4] = Min(numNorth, numWest);
            NumbSquaresToEdge[squareIndex][5] = Min(numSouth, numEast);
            NumbSquaresToEdge[squareIndex][6] = Min(numNorth, numEast);
            NumbSquaresToEdge[squareIndex][7] = Min(numSouth, numWest);
        }
    }
}

void print_board(void) {
    for (int x = 0; x < 64; x++) {
        if (x % 8 == 0) {
            printf("%d  ", 8 - x / 8);
        }

        printf("%c ", fen_board[x] ? (char)fen_board[x] : '.');

        if (x % 8 == 7) {
            printf("\n");
        }
    }
    printf("\n   a b c d e f g h\n\n");
}

void print_uci_move(int startSq, int targetSq, int promo) {
    char file_start  = 'a' + (startSq % 8);
    char rank_start  = '8' - (startSq / 8);
    char file_target = 'a' + (targetSq % 8);
    char rank_target = '8' - (targetSq / 8);

    if (promo != NONE) {
        char promo_char = 'q';

        if (promo == ROOK) {
            promo_char = 'r';
        }
        if (promo == BISHOP) {
            promo_char = 'b';
        }
        if (promo == KNIGHT) {
            promo_char = 'n';
        }

        printf("bestmove %c%c%c%c%c\n", file_start, rank_start, file_target, rank_target, promo_char);
    } else {
        printf("bestmove %c%c%c%c\n", file_start, rank_start, file_target, rank_target);
    }
}

Board convert_to_move_board(int inference_board[64]) {
    Board b;

    for (int i = 0; i < 64; i++) {
        switch (inference_board[i]) {
            case 80:  b.sq[i] = WHITE | PAWN;   break;
            case 78:  b.sq[i] = WHITE | KNIGHT; break;
            case 66:  b.sq[i] = WHITE | BISHOP; break;
            case 82:  b.sq[i] = WHITE | ROOK;   break;
            case 81:  b.sq[i] = WHITE | QUEEN;  break;
            case 75:  b.sq[i] = WHITE | KING;   break;

            case 112: b.sq[i] = BLACK | PAWN;   break;
            case 110: b.sq[i] = BLACK | KNIGHT; break;
            case 98:  b.sq[i] = BLACK | BISHOP; break;
            case 114: b.sq[i] = BLACK | ROOK;   break;
            case 113: b.sq[i] = BLACK | QUEEN;  break;
            case 107: b.sq[i] = BLACK | KING;   break;

            default:  b.sq[i] = NONE;           break;
        }
    }

    return b;
}

Board fen_to_board(char fen[]) {
    for (int i = 0; i < 64; i++) {
        fen_board[i] = 0;
    }
    char black_pieces[6] = {'r', 'n', 'b', 'q', 'k', 'p'};
    char white_pieces[6] = {'R', 'N', 'B', 'Q', 'K', 'P'};

    Board b;

    for (int i = 0; i < 8; i++) {
        b.pins[i][0] = -1;
        b.pins[i][1] = -1;
    }
    b.enpassant_target = -1;

    b.castle_rights[0] = false;
    b.castle_rights[1] = false;
    b.castle_rights[2] = false;
    b.castle_rights[3] = false;

    int slant_up = 0;
    int space_up = 0;
    b.halfmove = 0;
    b.fullmove = 0;
    int sq = 0;

    bool space = false;
    b.white_to_move = true;
    b.enpassant = false;

    for (int x = 0; x < 100 && fen[x] != '\0'; x++) {
        if (fen[x] == '/') {
            slant_up++;
        }

        if (fen[x] == ' ') {
            space = true;
            space_up++;
            continue;
        }

        if (slant_up < 8 && space != true) {
            if (fen[x] >= '1' && fen[x] <= '8') {
                sq += fen[x] - '0';
            }
            for (int white_p = 0; white_p < 6; white_p++) {
                if (fen[x] == white_pieces[white_p]) {
                    if (fen[x] == white_pieces[4]) {
                        b.whiteKingLocation = sq;
                    }
                    fen_board[sq++] = fen[x];
                }
            }
            for (int black_p = 0; black_p < 6; black_p++) {
                if (fen[x] == black_pieces[black_p]) {
                    if (fen[x] == black_pieces[4]) {
                        b.blackKingLocation = sq;
                    }
                    fen_board[sq++] = fen[x];
                }
            }
        }
        else if (space) {
            if (space_up == 1) {
                if (fen[x] == 'b') {
                    b.white_to_move = false;
                } else if (fen[x] == 'w') {
                    b.white_to_move = true;
                }
            }
            else if (space_up == 2) {
                if (fen[x] == 'K') {
                    b.castle_rights[0] = true;
                } else if (fen[x] == 'Q') {
                    b.castle_rights[1] = true;
                } else if (fen[x] == 'k') {
                    b.castle_rights[2] = true;
                } else if (fen[x] == 'q') {
                    b.castle_rights[3] = true;
                }
            }
            else if (space_up == 3) {
                if (fen[x] != '-') {
                    b.enpassant = true;
                    char file_char = fen[x];
                    char rank_char = fen[x + 1];

                    int col = file_char - 'a';
                    int row = 8 - (rank_char - '0');

                    b.enpassant_target = row * 8 + col;
                    x++;
                }
            }
            else if (space_up == 4) {
                b.halfmove = b.halfmove * 10 + (fen[x] - '0');
            }
            else if (space_up == 5) {
                b.fullmove = b.fullmove * 10 + (fen[x] - '0');
            }
        }
    }

    Board converted = convert_to_move_board(fen_board);
    for (int i = 0; i < 64; i++) {
        b.sq[i] = converted.sq[i];
    }

    b.hash = ComputeHash(&b);

    return b;
}

void MakeMove(Board *board, int startSquare, int targetSquare, int promoPiece) {
    int piece    = board->sq[startSquare];
    int color    = get_color(piece);
    int type     = get_piece_type(piece);
    int captured = board->sq[targetSquare];

    uint64_t hash = board->hash ^ CastleKey(board) ^ EnPassantKey(board);

    // 50-move rule
    if (type == PAWN || captured != NONE) {
        board->halfmove = 0;
    } else {
        board->halfmove++;
    }

    bool prev_ep = board->enpassant;
    int prev_ep_target = board->enpassant_target;

    board->enpassant = false;
    board->enpassant_target = -1;

    if (type == PAWN && prev_ep && targetSquare == prev_ep_target) {
        int capSq = targetSquare + (color == WHITE ? 8 : -8);
        hash ^= PieceKey(board->sq[capSq], capSq);
        board->sq[capSq] = NONE;
    }

    if (type == PAWN && abs(startSquare - targetSquare) == 16) {
        board->enpassant = true;
        board->enpassant_target = startSquare + (color == WHITE ? -8 : 8);
    }

    if (type == KING) {
        if (color == WHITE) {
            board->whiteKingLocation = targetSquare;
            board->castle_rights[0] = false;
            board->castle_rights[1] = false;
        } else {
            board->blackKingLocation = targetSquare;
            board->castle_rights[2] = false;
            board->castle_rights[3] = false;
        }

        if (abs(startSquare - targetSquare) == 2) {
            if (targetSquare == 62) {
                // White O-O
                board->sq[63] = NONE;
                board->sq[61] = WHITE | ROOK;
                hash ^= PieceKey(WHITE | ROOK, 63) ^ PieceKey(WHITE | ROOK, 61);
            } else if (targetSquare == 58) {
                // White O-O-O
                board->sq[56] = NONE;
                board->sq[59] = WHITE | ROOK;
                hash ^= PieceKey(WHITE | ROOK, 56) ^ PieceKey(WHITE | ROOK, 59);
            } else if (targetSquare == 6) {
                // Black O-O
                board->sq[7] = NONE;
                board->sq[5] = BLACK | ROOK;
                hash ^= PieceKey(BLACK | ROOK, 7) ^ PieceKey(BLACK | ROOK, 5);
            } else if (targetSquare == 2) {
                // Black O-O-O
                board->sq[0] = NONE;
                board->sq[3] = BLACK | ROOK;
                hash ^= PieceKey(BLACK | ROOK, 0) ^ PieceKey(BLACK | ROOK, 3);
            }
        }
    }

    if (startSquare == 63 || targetSquare == 63) {
        board->castle_rights[0] = false;
    }
    if (startSquare == 56 || targetSquare == 56) {
        board->castle_rights[1] = false;
    }
    if (startSquare == 7 || targetSquare == 7) {
        board->castle_rights[2] = false;
    }
    if (startSquare == 0 || targetSquare == 0) {
        board->castle_rights[3] = false;
    }

    board->sq[startSquare] = NONE;
    hash ^= PieceKey(piece, startSquare);

    if (captured != NONE) {
        hash ^= PieceKey(captured, targetSquare);
    }

    if (promoPiece != NONE) {
        board->sq[targetSquare] = color | promoPiece;
    } else {
        board->sq[targetSquare] = piece;
    }
    hash ^= PieceKey(board->sq[targetSquare], targetSquare);

    if (!board->white_to_move) {
        board->fullmove++;
    }
    board->white_to_move = !board->white_to_move;

    board->hash = hash ^ CastleKey(board) ^ EnPassantKey(board) ^ ZobristSide;
}