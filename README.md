# Numbtree

A CPU-based MCTS chess engine written in C. Work in progress.

## Building

    make          # builds ./Numbtree
    make debug    # builds ./Numbtree-debug with sanitizers

## Commands

- Speaks UCI.
- `./Numbtree bench` prints a deterministic node count and nps (used by OpenBench).
- `perft N` in UCI mode counts move-generator nodes to depth N.
