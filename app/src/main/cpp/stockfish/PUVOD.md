# Původ zdrojů Stockfishe

| | |
|---|---|
| projekt | Stockfish, <https://github.com/official-stockfish/Stockfish> |
| tag | `sf_19` (lehký tag, ukazuje přímo na commit) |
| commit | `edb0d9db6731067ec50ce619ff372b463bc4dd5d` |
| datum commitu | 5. 9. 2026, 09:52:51 +0200 |
| `Bench:` ve zprávě commitu | **2497913** |
| síť (`EvalFileDefaultName` v `evaluate.h`) | `nn-1a298aa575a0.nnue` |
| SHA-256 sítě | `1a298aa575a085434d29027978dc36867fe9c5bcea9376654b7a8eba1e52dfc2` |
| velikost sítě | 98 511 183 bajtů (94,0 MiB) |
| odkud je síť | `https://tests.stockfishchess.org/api/nn/nn-1a298aa575a0.nnue`, shodná s `https://github.com/official-stockfish/networks/raw/master/nn-1a298aa575a0.nnue` (stejný SHA-256) |

Prvních 12 znaků SHA-256 sítě (`1a298aa575a0`) sedí se jménem souboru.
Stejně síť ověřuje `scripts/net.sh` Stockfishe.

## Co tu je

- Všech 25 zdrojů ze `SRCS` v `src/Makefile` kromě `main.cpp` a všech 41
  hlaviček z `HEADERS`, ve stejných podadresářích (`nnue/`,
  `nnue/features/`, `nnue/layers/`, `syzygy/`).
- `incbin/incbin.h` a `incbin/UNLICENCE`. `nnue/network.cpp` hlavičku
  vkládá, ale v `HEADERS` Makefile ji nemá.
- `Copying.txt` z kořene repozitáře Stockfishe (GPLv3).
- Síť `nn-1a298aa575a0.nnue`, ale **jen na disku, ne v gitu** (níž).
  Vkládá se do `libredcry.so` přes `incbin`, stejně jako v oficiálním buildu.

Chybí `OTHER_SRCS` (`universal/…`, jen pro univerzální binárky x86, arm64
a riscv64) a `main.cpp`. Ten je beze změny v `tools/stockfish/main.cpp`,
kde se z něj skládá konzolový Stockfish pro `bench`.

Soubory jsou zkopírované **bajt po bajtu** (ověřeno `cmp` proti klonu tagu).
Upraveno nic (úpravy `// RedCry:` nejsou žádné). Aby git na Windows
nepřepsal konce řádků, má adresář v `.gitattributes` `-text`.

## Síť: jak se získá a proč není v gitu

```
./gradlew stahniSitStockfish
```

Úloha v `app/build.gradle.kts` stáhne síť do tohoto adresáře: nejdřív
z `tests.stockfishchess.org`, a když to nejde, z `github.com/official-stockfish/networks`.
Stahuje do dočasného souboru `….nnue.stahuje` a ten přejmenuje, až když
sedí SHA-256. Při nesouhlasu dočasný soubor smaže a úloha selže. Když je
síť už na místě a hash sedí, nedělá nic. Na build pověšená není, build
nemá sám sahat na síť.

Jméno, velikost a SHA-256 jsou na jediném místě, v konstantách na začátku
`app/build.gradle.kts`. Musí sedět s tabulkou nahoře. CMake je dostane
jako `-D` argumenty a před překladem cíle `stockfish` ověří velikost
i `file(SHA256 …)`. Když síť chybí nebo nesedí, build skončí větou
„Chybí síť Stockfishe – spusť ./gradlew stahniSitStockfish“.

Proč ne v gitu: celý repozitář (`.git`) měl před krokem S1 asi 3 MB,
síť má 98,5 MB. Stala by se z ní 97 % historie a zůstala by tam napořád,
i kdyby se síť později vyměnila nebo přešla na asset. Na limit GitHubu
100 MiB na soubor by se vešla jen těsně. Identitu sítě stejně nedrží
git, ale SHA-256. Síť je u Stockfishe pojmenovaná podle svého hashe, takže
soubor se správným hashem je bajt po bajtu tentýž.
`app/src/main/cpp/stockfish/nn-*.nnue` je v `.gitignore`.

## Licence

Stockfish je pod **GNU GPL verze 3** (`Copying.txt`). Knihovna `incbin`
je pod Unlicense (`incbin/UNLICENCE`). Síť NNUE se do gitu nedává,
stahuje se z oficiálních serverů Stockfishe úlohou `stahniSitStockfish`
(viz oddíl o síti výš).
