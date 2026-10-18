#include "board.h"

int pawnValue   = 100;
int knightValue = 300;
int bishopValue = 300;
int rookValue   = 500;
int queenValue  = 900;

int CountMaterial(const Board *board, int color_request) {
    int material = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        int piece = board->sq[i];
        if (piece != NONE && get_color(piece) == color_request) {
            int piece_type = get_piece_type(piece);
            switch (piece_type) {
                case PAWN:   material += pawnValue;   break;
                case KNIGHT: material += knightValue; break;
                case BISHOP: material += bishopValue; break;
                case ROOK:   material += rookValue;   break;
                case QUEEN:  material += queenValue;  break;
            }
        }
    }
    return material;
}

int Evaluate(const Board *board){
    int whiteEval = CountMaterial(board, WHITE);
    int blackEval = CountMaterial(board, BLACK);
    int evaluation = whiteEval - blackEval;
    int perspective = (board->white_to_move) ? 1 : -1;
    return evaluation * perspective;
}