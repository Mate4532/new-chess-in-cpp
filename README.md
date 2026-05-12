# High-Performance UCI Chess Engine (C++17)

![C++](https://img.shields.io/badge/Language-C++17-blue.svg)
![ELO](https://img.shields.io/badge/Estimated_ELO-3000+-brightgreen.svg)
![Status](https://img.shields.io/badge/Status-Work_in_Progress-orange.svg)

A highly optimized, UCI-compatible chess engine written from scratch in C++17. The engine focuses on raw performance and efficient hardware utilization, integrating both classical algorithmic search techniques and modern NNUE evaluation.

> **Graphical interface:**
> The engine is separated from the frontend. You can find the accompanying **Qt-based GUI repository here:** [[gui](https://github.com/Mate4532/new-chess-in-cpp-gui)]

## Performance Highlights
* **Speed:** Achieves a peak search speed of **40 Million Nodes Per Second (NPS)** on standard hardware (reaching ~25% of the speed of state-of-the-art engines like Stockfish on identical setups).
* **Strength:** Currently holds around **3000+ ELO** rating in local testing and Lichess bot pool evaluations.

## Testing & Benchmarking
To ensure consistent progress, the project includes a framework for automated testing and analysis:
* **Bot-vs-Bot Testing:** Support for running matches against earlier versions to evaluate improvements.
* **Version Control:** Systematic benchmarking of search/evaluation iterations.
* **PGN Export:** Automatically saves match results in organized **PGN format** for detailed post-game analysis.

## Technical Architecture
The engine relies on a highly optimized move generation and search architecture:
* **Board Representation:** Magic Bitboards for lightning-fast piece attack generation.
* **Search Algorithm:** Negamax search enhanced with Alpha-Beta pruning.
* **Search Optimizations:** Transposition Tables (TT), Quiescence Search, Lazy SMP, NMP, Futility pruning, and more.
* **Evaluation:** Hybrid approach utilizing NNUE (Efficiently Updatable Neural Networks) combined with optimized memory access patterns.

## Development Status & To-Do
*This project is actively maintained. The core logic and performance targets are achieved, but the codebase is currently undergoing a refactoring phase to improve readability and maintainability.*

## Usage (UCI Protocol)
The engine supports the core Universal Chess Interface (UCI) protocol. It can be loaded into any popular chess GUI (Arena, Cute Chess, etc.) or used via my custom Qt GUI.

Alternatively, you can run the executable directly from the command line (CMD/Terminal) and interact with it by manually entering standard UCI commands (e.g., `uci`, `isready`, `position startpos`, `go wtime 300000 btime 300000 winc 2000 binc 2000`).

> **Note:** For the evaluation to function, the **NNUE file** must be placed in the same directory as the executable. While essential UCI commands are functional, certain features like **pondering** are not yet supported.