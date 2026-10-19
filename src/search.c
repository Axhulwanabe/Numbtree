#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"

#define CPUCT 1.5   // how much to explore

enum { ONGOING, DRAWN, LOST };   

typedef struct {
    double   score;        // sum of results, from the pov of the side that moved into this node
    uint32_t visits;
    uint32_t firstChild;   
    uint8_t  numChildren;
    uint8_t  from, to, promo;   // the move that got here
    uint8_t  state;             // ONGOING, DRAWN or LOST
} Node;

static Node    *Tree;
static uint32_t TreeSize;   // nodes that fit
static uint32_t TreeUsed;   

void InitTree(int megabytes) {
    free(Tree);
    TreeSize = (uint32_t)((size_t)megabytes * 1024 * 1024 / sizeof(Node));
    Tree = malloc((size_t)TreeSize * sizeof(Node));

    if (Tree == NULL) {
        printf("info string couldn't get %d MB for the tree\n", megabytes);
        exit(1);
    }
    ClearTree();
}

void ClearTree(void) {
    Tree[0] = (Node){0};
    TreeUsed = 1;
}


static uint32_t AllocNodes(int count) {
    if (TreeUsed + (uint32_t)count > TreeSize) {
        return 0;
    }
    uint32_t first = TreeUsed;
    TreeUsed += count;
    return first;
}

bool ExpandNode(uint32_t index, const Board *board) {
    MoveList moves;
    GenerateMoves(board, &moves);

    uint32_t first = AllocNodes(moves.count);
    if (first == 0) {
        return false;
    }

    for (int i = 0; i < moves.count; i++) {
        Tree[first + i] = (Node){
            .from  = moves.moves[i][0],
            .to    = moves.moves[i][1],
            .promo = moves.moves[i][2],
        };
    }

    Tree[index].firstChild  = first;
    Tree[index].numChildren = moves.count;
    return true;
}

// material eval squashed into a 0-1 win chance for the side to move
double WinChance(const Board *board) {
    return 1.0 / (1.0 + exp(-Evaluate(board) / 400.0));
}

static bool InCheck(const Board *board) {
    int king  = board->white_to_move ? board->whiteKingLocation : board->blackKingLocation;
    int enemy = board->white_to_move ? BLACK : WHITE;
    return IsSquareAttacked(king, enemy, board);
}

// first time we reach a node: is the game over here, and if not what's it worth
static double FirstVisit(Node *node, const Board *board, bool isRoot) {
    MoveList moves;
    GenerateMoves(board, &moves);

    if (moves.count == 0) {
        node->state = InCheck(board) ? LOST : DRAWN;
        return node->state == LOST ? 0.0 : 0.5;
    }

    // the game's still on at the root even if it's a repeat
    if (!isRoot && IsDraw(board)) {
        node->state = DRAWN;
        return 0.5;
    }

    return WinChance(board);
}

// best child by score so far, plus a bonus for moves that haven't had many visits
static uint32_t SelectChild(const Node *node) {
    double fpu     = 1.0 - node->score / node->visits;
    double prior   = 1.0 / node->numChildren;
    double explore = CPUCT * prior * sqrt(node->visits);

    uint32_t best = node->firstChild;
    double bestValue = -1.0;

    for (uint32_t i = node->firstChild; i < node->firstChild + node->numChildren; i++) {
        const Node *child = &Tree[i];
        double q = child->visits ? child->score / child->visits : fpu;
        double value = q + explore / (1 + child->visits);

        if (value > bestValue) {
            bestValue = value;
            best = i;
        }
    }
    return best;
}

static int PlayoutDepth;   

// returns the result for the side to move at this node: 0 loss, 0.5 draw, 1 win
static double Playout(uint32_t index, Board *board) {
    Node *node = &Tree[index];
    double result;

    PlayoutDepth++;

    if (node->visits == 0) {
        result = FirstVisit(node, board, index == 0);
    } else if (node->state == LOST) {
        result = 0.0;
    } else if (node->state == DRAWN) {
        result = 0.5;
    } else if (node->firstChild == 0 && !ExpandNode(index, board)) {
        result = WinChance(board);   // tree's full, just score it again
    } else {
        uint32_t child = SelectChild(node);
        PushHistory(board->hash);
        MakeMove(board, Tree[child].from, Tree[child].to, Tree[child].promo);
        result = 1.0 - Playout(child, board);
    }

    node->visits++;
    node->score += 1.0 - result;
    return result;
}


int RunPlayout(const Board *root) {
    Board board = *root;
    int historySize = HistorySize();

    PlayoutDepth = 0;
    Playout(0, &board);
    SetHistorySize(historySize);
    return PlayoutDepth;
}
static long long NowMs(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void MoveString(const Node *node, char *out) {
    out[0] = 'a' + node->from % 8;
    out[1] = '8' - node->from / 8;
    out[2] = 'a' + node->to % 8;
    out[3] = '8' - node->to / 8;
    out[4] = node->promo ? " kpnbrq"[node->promo] : '\0';
    out[5] = '\0';
}


void Search(const Board *root, int timeMs, long long maxNodes) {
    if (timeMs < 0 && maxNodes < 0) {
        maxNodes = 100000;
    }

    ClearTree();
    ExpandNode(0, root);
    Node *rootNode = &Tree[0];

    if (rootNode->numChildren == 0) {
        printf("bestmove 0000\n");
        fflush(stdout);
        return;
    }

    long long start = NowMs();
    long long playouts = 0;

    while (true) {
        RunPlayout(root);
        playouts++;

        if (maxNodes >= 0 && playouts >= maxNodes) {
            break;
        }
        if (timeMs >= 0 && playouts % 128 == 0 && NowMs() - start >= timeMs) {
            break;
        }
        if (TreeSize - TreeUsed < 256) {
            break;
        }
    }

    uint32_t best = rootNode->firstChild;
    for (uint32_t i = best + 1; i < rootNode->firstChild + rootNode->numChildren; i++) {
        if (Tree[i].visits > Tree[best].visits) {
            best = i;
        }
    }


    double q = Tree[best].visits ? Tree[best].score / Tree[best].visits : 0.5;
    q = fmin(fmax(q, 0.001), 0.999);
    int cp = (int)(400.0 * log(q / (1.0 - q)));

    long long elapsed = NowMs() - start;
    char move[6];
    MoveString(&Tree[best], move);

    printf("info nodes %lld time %lld nps %lld score cp %d pv %s\n",
           playouts, elapsed, playouts * 1000 / (elapsed > 0 ? elapsed : 1), cp, move);
    printf("bestmove %s\n", move);
    fflush(stdout);
}