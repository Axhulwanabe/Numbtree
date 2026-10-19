#ifndef BOARD_H
#define BOARD_H

#define BOARD_SIZE 64

#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>

enum PieceType {
    NONE = 0,
    KING = 1,
    PAWN = 2,
    KNIGHT = 3,
    BISHOP = 4,
    ROOK = 5,
    QUEEN = 6
};

enum Color {
    WHITE = 8,
    BLACK = 16
};


typedef struct{
    enum PieceType type;
    enum Color color;
} Piece;

static inline int IsSlidingPiece(int piece){
    int piece_type = piece & 7;
    return (piece_type == ROOK || piece_type == QUEEN || piece_type == BISHOP);
};

static inline int get_color(int piece_value){
    return piece_value & (WHITE | BLACK);
}

static inline int get_piece_type(int piece_value){
    return piece_value & 7;
}

static inline int is_type(int pieceOnSquare, int piece_value){
    return (pieceOnSquare & 7) == piece_value;
}

typedef struct {
    int sq[BOARD_SIZE];
    bool castle_rights[4];
    bool white_to_move;
    bool enpassant;
    int enpassant_target;
    int halfmove;
    int fullmove;
    bool checkmate;
    bool stalemate;
    bool in_check;
    int pins[8][2];
    int checks[2][2];
    int whiteKingLocation;
    int blackKingLocation;
    uint64_t hash;
} Board;

typedef struct {
    int moves[220][3]; // [0]: startSq, [1]: targetSq, [2]: promoPieceType
    int count;
} MoveList;

static inline int Min(int a, int b) { return a < b ? a : b; }
static inline int Max(int a, int b) { return a > b ? a : b; }

extern int fen_board[BOARD_SIZE];          /* board.c  */
extern int DirectionOffsets[8];            /* board.c  */
extern int NumbSquaresToEdge[64][8];       /* board.c  */

/* ---- board.c ---- */
void  PrecomputedMoveData(void);
void  print_board(void);
void  print_uci_move(int startSq, int targetSq, int promo);
Board convert_to_move_board(int inference_board[64]);
Board fen_to_board(char fen[]);
void  MakeMove(Board *board, int startSquare, int targetSquare, int promoPiece);
void  ClearHistory(void);
void  PushHistory(uint64_t hash);
bool  IsRepetition(const Board *board);
bool  IsDraw(const Board *board);
int   HistorySize(void);
void  SetHistorySize(int size);


/* ---- movegen.c ---- */
bool IsSquareAttacked(int square, int attackerColor, const Board *board);
void GenerateSlidingMoves(int startSquare, int piece, MoveList *moves, const Board *board);
void GenerateKnightMoves(int startSquare, int piece, MoveList *moves, const Board *board);
void AddPawnMove(MoveList *moves, int startSquare, int targetSquare);
void GeneratePawnMoves(int startSquare, int piece, MoveList *moves, const Board *board);
void GenerateKingMoves(int startSquare, int piece, MoveList *moves, const Board *board);
void GenerateMoves(const Board *board, MoveList *legalMoves);

/* ---- eval.c ---- */
int CountMaterial(const Board *board, int color_request);
int Evaluate(const Board *board);

/* ---- perft.c ---- */
unsigned long long Perft(int depth, const Board *board);
void PrintPerftTable(int maxDepth, const Board *board);

/* ---- search.c ---- */
void InitTree(int megabytes);
void ClearTree(void);
bool ExpandNode(uint32_t index, const Board *board);
double WinChance(const Board *board);
int  RunPlayout(const Board *root);
void Search(const Board *root, int timeMs, long long maxNodes);

/* ---- zobrist.c ---- */
extern uint64_t ZobristSide;
void     InitZobrist(void);
uint64_t ComputeHash(const Board *board);
uint64_t PieceKey(int piece, int sq);
uint64_t CastleKey(const Board *board);
uint64_t EnPassantKey(const Board *board);

/* ---- uci.c ---- */
void RunBench(int playouts);
 
#endif