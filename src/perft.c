#include <stdio.h>
#include "board.h"

unsigned long long Perft(int depth, const Board *board) {
    if (depth == 0) {
        return 1;
    }

    unsigned long long nodes = 0;
    MoveList moves;
    GenerateMoves(board, &moves);

    for (int i = 0; i < moves.count; i++) {
        int startSq  = moves.moves[i][0];
        int targetSq = moves.moves[i][1];
        int promo    = moves.moves[i][2];

        Board nextBoard = *board;
        MakeMove(&nextBoard, startSq, targetSq, promo);
        nodes += Perft(depth - 1, &nextBoard);
    }

    return nodes;
}

void PrintPerftTable(int maxDepth, const Board *board) {
    printf("Depth   Nodes\n");
    printf("---------------------\n");

    for (int d = 0; d <= maxDepth; d++) {
        printf("%-7d %llu\n", d, Perft(d, board));
    }
}