# RedCry

Chess app for Android with four engines: LightRed, RedMax, Crafty 18.15
and Stockfish 19. This repository contains the complete source code of
each published version of the app, one commit per version (tag vX.Y).

## Licences

- RedCry's own code, including the LightRed and RedMax engines:
  GNU GPL v3, see LICENSE.
- Stockfish 19 (app/src/main/cpp/stockfish): GNU GPL v3, © the Stockfish
  developers, unchanged from tag sf_19.
- Crafty 18.15 (app/src/main/cpp/crafty_*): © Robert M. Hyatt. Not
  licensed under the GPL; used under the author's terms reproduced at the
  top of each crafty_* file.
- RedMax is based on the program Minimax by Dieter Steinwender and
  Chrilly Donninger (D. Steinwender, F. Friedel: Schach am PC, 1995).
- Opening book data: lichess-org/chess-openings (CC0).
- incbin: The Unlicense.

## Building

Android Studio with NDK 27.2.12479018.

    ./gradlew stahniSitStockfish   # downloads the Stockfish NNUE network (98 MB)
    ./gradlew assembleRelease
