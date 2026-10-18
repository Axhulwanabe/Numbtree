#include <stdio.h>
#include <stdlib.h>
#include "board.h"

bool IsSquareAttacked(int square, int attackerColor, const Board *board) {
    int pawn_offset_left  = (attackerColor == WHITE) ? 9 : -9;
    int pawn_offset_right = (attackerColor == WHITE) ? 7 : -7;
    int pawn_offsets[2]   = {pawn_offset_left, pawn_offset_right};

    for (int i = 0; i < 2; i++) {
        int attackerSq = square + pawn_offsets[i];
        if (attackerSq >= 0 && attackerSq < 64 && abs((square % 8) - (attackerSq % 8)) == 1) {
            int p = board->sq[attackerSq];
            if (p != NONE && get_color(p) == attackerColor && get_piece_type(p) == PAWN) {
                return true;
            }
        }
    }

    int knight_offsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    for (int i = 0; i < 8; i++) {
        int attackerSq = square + knight_offsets[i];
        if (attackerSq >= 0 && attackerSq < 64) {
            int colDiff = abs((square % 8) - (attackerSq % 8));
            if (colDiff == 1 || colDiff == 2) {
                int p = board->sq[attackerSq];
                if (p != NONE && get_color(p) == attackerColor && get_piece_type(p) == KNIGHT) {
                    return true;
                }
            }
        }
    }

    for (int i = 0; i < 8; i++) {
        int attackerSq = square + DirectionOffsets[i];
        if (attackerSq >= 0 && attackerSq < 64) {
            int colDiff = abs((square % 8) - (attackerSq % 8));
            int rowDiff = abs((square / 8) - (attackerSq / 8));
            if (colDiff <= 1 && rowDiff <= 1) {
                int p = board->sq[attackerSq];
                if (p != NONE && get_color(p) == attackerColor && get_piece_type(p) == KING) {
                    return true;
                }
            }
        }
    }

    for (int dir = 0; dir < 8; dir++) {
        for (int n = 0; n < NumbSquaresToEdge[square][dir]; n++) {
            int attackerSq = square + DirectionOffsets[dir] * (n + 1);
            int p = board->sq[attackerSq];

            if (p != NONE) {
                if (get_color(p) == attackerColor) {
                    int type = get_piece_type(p);
                    if (dir < 4 && (type == ROOK || type == QUEEN)) {
                        return true;
                    }
                    if (dir >= 4 && (type == BISHOP || type == QUEEN)) {
                        return true;
                    }
                }
                break;
            }
        }
    }

    return false;
}

void GenerateSlidingMoves(int startSquare, int piece, MoveList *moves, const Board *board) {
    int piece_type = get_piece_type(piece);
    int start_dir_index = (piece_type == BISHOP) ? 4 : 0;
    int end_dir_index   = (piece_type == ROOK)   ? 4 : 8;

    bool piece_pinned = false;
    int pin_dir = -999;

    for (int i = 0; i < 8; i++) {
        if (board->pins[i][0] == startSquare) {
            piece_pinned = true;
            pin_dir = board->pins[i][1];
            break;
        }
    }

    for (int directionIndex = start_dir_index; directionIndex < end_dir_index; directionIndex++) {
        if (piece_pinned && pin_dir != directionIndex && pin_dir != -directionIndex) {
            continue;
        }

        for (int n = 0; n < NumbSquaresToEdge[startSquare][directionIndex]; n++) {
            int targetSquare = startSquare + DirectionOffsets[directionIndex] * (n + 1);
            if (targetSquare < 0 || targetSquare >= 64) {
                break;
            }

            int pieceOnTargetSquare = board->sq[targetSquare];

            if (pieceOnTargetSquare == NONE) {
                moves->moves[moves->count][0] = startSquare;
                moves->moves[moves->count][1] = targetSquare;
                moves->moves[moves->count][2] = NONE;
                moves->count++;
            } else {
                if (get_color(piece) != get_color(pieceOnTargetSquare)) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = targetSquare;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
                break;
            }
        }
    }
}

void GenerateKnightMoves(int startSquare, int piece, MoveList *moves, const Board *board) {
    int knight_offsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    bool piece_pinned = false;

    for (int i = 0; i < 8; i++) {
        if (board->pins[i][0] == startSquare) {
            piece_pinned = true;
            break;
        }
    }

    if (piece_pinned) {
        return;
    }

    for (int i = 0; i < 8; i++) {
        int targetSquare = startSquare + knight_offsets[i];

        if (targetSquare >= 0 && targetSquare < 64) {
            int file_diff = abs((startSquare % 8) - (targetSquare % 8));
            if (file_diff > 2) {
                continue;
            }

            int pieceOnTargetSquare = board->sq[targetSquare];

            if (pieceOnTargetSquare == NONE || get_color(piece) != get_color(pieceOnTargetSquare)) {
                moves->moves[moves->count][0] = startSquare;
                moves->moves[moves->count][1] = targetSquare;
                moves->moves[moves->count][2] = NONE;
                moves->count++;
            }
        }
    }
}

void AddPawnMove(MoveList *moves, int startSquare, int targetSquare) {
    int targetRank = targetSquare / 8;

    if (targetRank == 0 || targetRank == 7) {
        int promoTypes[4] = {QUEEN, ROOK, BISHOP, KNIGHT};
        for (int i = 0; i < 4; i++) {
            moves->moves[moves->count][0] = startSquare;
            moves->moves[moves->count][1] = targetSquare;
            moves->moves[moves->count][2] = promoTypes[i];
            moves->count++;
        }
    } else {
        moves->moves[moves->count][0] = startSquare;
        moves->moves[moves->count][1] = targetSquare;
        moves->moves[moves->count][2] = NONE;
        moves->count++;
    }
}

void GeneratePawnMoves(int startSquare, int piece, MoveList *moves, const Board *board) {
    int piece_type = get_piece_type(piece);
    if (piece_type != PAWN) {
        return;
    }

    int forward, double_forward, left_capture, right_capture, start_row;
    int vertical_pin_dir;

    if (board->white_to_move) {
        forward = -8;
        double_forward = -16;
        left_capture = -9;
        right_capture = -7;
        start_row = 6;
        vertical_pin_dir = 0;
    } else {
        forward = 8;
        double_forward = 16;
        left_capture = 9;
        right_capture = 7;
        start_row = 1;
        vertical_pin_dir = 1;
    }

    bool piece_pinned = false;
    int pin_dir = -999;

    for (int i = 0; i < 8; i++) {
        if (board->pins[i][0] == startSquare) {
            piece_pinned = true;
            pin_dir = board->pins[i][1];
            break;
        }
    }

    // Forward pushes
    int targetSquare = startSquare + forward;
    if (targetSquare >= 0 && targetSquare < 64 && board->sq[targetSquare] == NONE) {
        if (!piece_pinned || pin_dir == vertical_pin_dir) {
            AddPawnMove(moves, startSquare, targetSquare);

            // Double forward push
            if (startSquare / 8 == start_row) {
                int doubleTarget = startSquare + double_forward;
                if (doubleTarget >= 0 && doubleTarget < 64 && board->sq[doubleTarget] == NONE) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = doubleTarget;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
            }
        }
    }

    // Captures and en passant
    int captures[2] = {left_capture, right_capture};
    int capture_pin_dirs[2] = {
        board->white_to_move ? 4 : 7,
        board->white_to_move ? 6 : 5
    };

    for (int i = 0; i < 2; i++) {
        int cap_offset = captures[i];
        int cap_pin_dir = capture_pin_dirs[i];
        int cap_target = startSquare + cap_offset;

        if (cap_target < 0 || cap_target >= 64) {
            continue;
        }
        if (abs((startSquare % 8) - (cap_target % 8)) != 1) {
            continue;
        }
        if (piece_pinned && pin_dir != cap_pin_dir) {
            continue;
        }

        int targetPiece = board->sq[cap_target];

        // Standard Capture
        if (targetPiece != NONE && get_color(targetPiece) != get_color(piece)) {
            AddPawnMove(moves, startSquare, cap_target);
        }
        // En Passant Capture
        else if (board->enpassant && cap_target == board->enpassant_target) {
            int kingSquare = board->white_to_move ? board->whiteKingLocation : board->blackKingLocation;
            int pawnRank = startSquare / 8;
            int kingRank = kingSquare / 8;

            bool ep_is_legal = true;

            if (kingRank == pawnRank) {
                int capturedPawnSq = cap_target + (board->white_to_move ? 8 : -8);
                int rankStart = pawnRank * 8;
                int rankEnd = rankStart + 8;

                bool checking_attacker_found = false;
                bool blocking_piece_found = false;

                int step = (kingSquare < startSquare) ? 1 : -1;
                for (int sq = kingSquare + step; sq >= rankStart && sq < rankEnd; sq += step) {
                    if (sq == startSquare || sq == capturedPawnSq) {
                        continue;
                    }

                    int pieceOnSq = board->sq[sq];
                    if (pieceOnSq != NONE) {
                        int pType = get_piece_type(pieceOnSq);
                        if (get_color(pieceOnSq) != get_color(piece) && (pType == ROOK || pType == QUEEN)) {
                            checking_attacker_found = true;
                        } else {
                            blocking_piece_found = true;
                        }
                        break;
                    }
                }

                if (checking_attacker_found && !blocking_piece_found) {
                    ep_is_legal = false;
                }
            }

            if (ep_is_legal) {
                moves->moves[moves->count][0] = startSquare;
                moves->moves[moves->count][1] = cap_target;
                moves->moves[moves->count][2] = NONE;
                moves->count++;
            }
        }
    }
}

void GenerateKingMoves(int startSquare, int piece, MoveList *moves, const Board *board) {
    for (int i = 0; i < 8; i++) {
        int targetSquare = startSquare + DirectionOffsets[i];

        if (targetSquare >= 0 && targetSquare < 64) {
            if (abs((startSquare % 8) - (targetSquare % 8)) > 1) {
                continue;
            }

            int pieceOnTargetSquare = board->sq[targetSquare];

            if (pieceOnTargetSquare == NONE || get_color(piece) != get_color(pieceOnTargetSquare)) {
                moves->moves[moves->count][0] = startSquare;
                moves->moves[moves->count][1] = targetSquare;
                moves->moves[moves->count][2] = NONE;
                moves->count++;
            }
        }
    }

    // Castling
    int enemyColor = board->white_to_move ? BLACK : WHITE;
    if (!IsSquareAttacked(startSquare, enemyColor, board)) {
        if (board->white_to_move) {
            // White Kingside
            if (board->castle_rights[0] && board->sq[61] == NONE && board->sq[62] == NONE) {
                if (!IsSquareAttacked(61, enemyColor, board) && !IsSquareAttacked(62, enemyColor, board)) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = 62;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
            }
            // White Queenside
            if (board->castle_rights[1] && board->sq[57] == NONE && board->sq[58] == NONE && board->sq[59] == NONE) {
                if (!IsSquareAttacked(58, enemyColor, board) && !IsSquareAttacked(59, enemyColor, board)) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = 58;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
            }
        } else {
            // Black Kingside
            if (board->castle_rights[2] && board->sq[5] == NONE && board->sq[6] == NONE) {
                if (!IsSquareAttacked(5, enemyColor, board) && !IsSquareAttacked(6, enemyColor, board)) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = 6;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
            }
            // Black Queenside
            if (board->castle_rights[3] && board->sq[1] == NONE && board->sq[2] == NONE && board->sq[3] == NONE) {
                if (!IsSquareAttacked(2, enemyColor, board) && !IsSquareAttacked(3, enemyColor, board)) {
                    moves->moves[moves->count][0] = startSquare;
                    moves->moves[moves->count][1] = 2;
                    moves->moves[moves->count][2] = NONE;
                    moves->count++;
                }
            }
        }
    }
}

void GenerateMoves(const Board *board, MoveList *legalMoves) {
    MoveList pseudoMoves;
    pseudoMoves.count = 0;

    legalMoves->count = 0;

    for (int startSquare = 0; startSquare < 64; startSquare++) {
        int piece_on_board = board->sq[startSquare];
        if (piece_on_board != NONE) {
            int color = get_color(piece_on_board);
            if ((color == WHITE && board->white_to_move) || (color == BLACK && !board->white_to_move)) {
                if (IsSlidingPiece(piece_on_board)) {
                    GenerateSlidingMoves(startSquare, piece_on_board, &pseudoMoves, board);
                }
                else if (get_piece_type(piece_on_board) == KNIGHT) {
                    GenerateKnightMoves(startSquare, piece_on_board, &pseudoMoves, board);
                }
                else if (get_piece_type(piece_on_board) == PAWN) {
                    GeneratePawnMoves(startSquare, piece_on_board, &pseudoMoves, board);
                }
                else if (get_piece_type(piece_on_board) == KING) {
                    GenerateKingMoves(startSquare, piece_on_board, &pseudoMoves, board);
                }
            }
        }
    }

    int currentSideColor = board->white_to_move ? WHITE : BLACK;
    int enemyColor       = board->white_to_move ? BLACK : WHITE;

    for (int i = 0; i < pseudoMoves.count; i++) {
        int startSq  = pseudoMoves.moves[i][0];
        int targetSq = pseudoMoves.moves[i][1];
        int promo    = pseudoMoves.moves[i][2];

        Board tempBoard = *board;
        MakeMove(&tempBoard, startSq, targetSq, promo);
        int kingSq = (currentSideColor == WHITE) ? tempBoard.whiteKingLocation : tempBoard.blackKingLocation;

        if (!IsSquareAttacked(kingSq, enemyColor, &tempBoard)) {
            legalMoves->moves[legalMoves->count][0] = startSq;
            legalMoves->moves[legalMoves->count][1] = targetSq;
            legalMoves->moves[legalMoves->count][2] = promo;
            legalMoves->count++;
        }
    }
}