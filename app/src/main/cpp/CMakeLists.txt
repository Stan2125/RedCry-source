# Nativní část RedCry - knihovna redcry
#
# Jedna knihovna pro všechny motory žebříčku: Light Red (namespace
# lightred), RedMAX (redmax) a Crafty 18.15 (crafty), každý s vlastní
# vrstvou UCI a mostem JNI. Stockfish 19 (Stockfish) je statická knihovna
# (krok S1) s vrstvou UCI stockfish_uci.cpp (namespace sfredcry, krok S2)
# a mostem JNI stockfish_jni_bridge.cpp (krok S3).

cmake_minimum_required(VERSION 3.22.1)
project(redcry LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(redcry SHARED
    light_red_core.cpp
    light_red_search.cpp
    red_max_core.cpp
    red_max_search.cpp
    red_max_uci.cpp
    uci.cpp
    jni_bridge.cpp
    red_max_jni_bridge.cpp
    # Crafty 18.15 (namespace crafty): pravidla, hledání a vrstva UCI.
    crafty_core.cpp
    crafty_search.cpp
    crafty_uci.cpp
    crafty_jni_bridge.cpp
    # Stockfish 19: vrstva UCI a most, motor je v knihovně stockfish níž.
    # Most includuje jen stockfish_uci.h, a ta nic ze Stockfishe, proto
    # nepotřebuje jeho definice (IS_64BIT, USE_NEON …).
    stockfish_uci.cpp
    stockfish_jni_bridge.cpp
)

target_include_directories(redcry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Vyjimky NEvypinat: oba mosty (jni_bridge.cpp a red_max_jni_bridge.cpp) je
# odchytavaji, aby zadna neprolezla pres hranici JNI do Kotlinu.
target_compile_options(redcry PRIVATE -O3 -Wall -Wextra)

# crafty_search.cpp je řádek po řádku z RedCrafty; po vyhozených výpisech do
# Memo a Label v něm zůstaly nepoužité proměnné a parametry (část už
# v originálu). Těla se kvůli varování neupravují.
set_source_files_properties(crafty_search.cpp PROPERTIES COMPILE_OPTIONS
    "-Wno-unused-parameter;-Wno-unused-variable;-Wno-unused-but-set-variable;-Wno-misleading-indentation")

# Stockfish 19 (namespace Stockfish): zdroje z tagu sf_19 beze změny, viz
# stockfish/PUVOD.md. Vlastní statická knihovna, aby jeho makra, volby
# a varování nezasáhla ostatní motory. Definice podle Makefile Stockfishe
# (ARCH=armv8 a ARCH=x86-64-sse41-popcnt, COMP=ndk).
#
# Síť NNUE není v gitu (98,5 MB). Jméno, velikost a SHA-256 přicházejí
# z app/build.gradle.kts jako -D argumenty; bez sítě se nepřekládá nic.
set(STOCKFISH_SIT_CESTA ${CMAKE_CURRENT_SOURCE_DIR}/stockfish/${STOCKFISH_SIT})
set(STOCKFISH_SIT_CHYBA "Chybí síť Stockfishe – spusť ./gradlew stahniSitStockfish")
if(NOT STOCKFISH_SIT OR NOT STOCKFISH_SIT_VELIKOST OR NOT STOCKFISH_SIT_SHA256)
    message(FATAL_ERROR "${STOCKFISH_SIT_CHYBA} (CMake nedostal jméno, "
        "velikost a SHA-256 sítě z app/build.gradle.kts)")
endif()
if(NOT EXISTS ${STOCKFISH_SIT_CESTA})
    message(FATAL_ERROR "${STOCKFISH_SIT_CHYBA} (${STOCKFISH_SIT_CESTA} neexistuje)")
endif()
file(SIZE ${STOCKFISH_SIT_CESTA} STOCKFISH_SIT_SKUTECNA_VELIKOST)
if(NOT STOCKFISH_SIT_SKUTECNA_VELIKOST EQUAL STOCKFISH_SIT_VELIKOST)
    message(FATAL_ERROR "${STOCKFISH_SIT_CHYBA} (${STOCKFISH_SIT} má "
        "${STOCKFISH_SIT_SKUTECNA_VELIKOST} B, očekáváno ${STOCKFISH_SIT_VELIKOST} B)")
endif()
file(SHA256 ${STOCKFISH_SIT_CESTA} STOCKFISH_SIT_SKUTECNY_SHA256)
if(NOT STOCKFISH_SIT_SKUTECNY_SHA256 STREQUAL STOCKFISH_SIT_SHA256)
    message(FATAL_ERROR "${STOCKFISH_SIT_CHYBA} (${STOCKFISH_SIT} má SHA-256 "
        "${STOCKFISH_SIT_SKUTECNY_SHA256}, očekáváno ${STOCKFISH_SIT_SHA256})")
endif()

add_library(stockfish STATIC
    stockfish/attacks.cpp
    stockfish/benchmark.cpp
    stockfish/bitboard.cpp
    stockfish/engine.cpp
    stockfish/evaluate.cpp
    stockfish/memory.cpp
    stockfish/misc.cpp
    stockfish/movegen.cpp
    stockfish/movepick.cpp
    stockfish/position.cpp
    stockfish/score.cpp
    stockfish/search.cpp
    stockfish/thread.cpp
    stockfish/timeman.cpp
    stockfish/tt.cpp
    stockfish/tune.cpp
    stockfish/uci.cpp
    stockfish/ucioption.cpp
    stockfish/syzygy/tbprobe.cpp
    stockfish/nnue/network.cpp
    stockfish/nnue/nnue_accumulator.cpp
    stockfish/nnue/nnue_misc.cpp
    stockfish/nnue/features/full_threats.cpp
    stockfish/nnue/features/half_ka_v2_hm.cpp
    stockfish/nnue/features/pp_3wide.cpp
)

set_target_properties(stockfish PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(STOCKFISH_DEFINICE IS_64BIT USE_POPCNT USE_NEON=8)
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(STOCKFISH_DEFINICE IS_64BIT USE_POPCNT USE_SSE2 USE_SSSE3 USE_SSE41)
else()
    message(FATAL_ERROR "Stockfish: nepodporované ABI ${ANDROID_ABI}")
endif()
target_compile_definitions(stockfish PRIVATE ${STOCKFISH_DEFINICE})

# stockfish_uci.cpp includuje hlavičky Stockfishe, takže se musí překládat
# se stejnými definicemi (jinak by se inline funkce a rozložení tříd lišily
# od knihovny). Jen tenhle soubor - ostatní motory makra Stockfishe nevidí.
# Varování zůstávají zapnutá, je to náš kód.
set_source_files_properties(stockfish_uci.cpp PROPERTIES
    COMPILE_DEFINITIONS "${STOCKFISH_DEFINICE};NDEBUG")

# -DNDEBUG jako oficiální build (debug=no). -w: varování jen na tomto cíli,
# kód Stockfishe se kvůli nim neupravuje. -Wa,-I: síť se vkládá do binárky
# přes incbin (.incbin v nnue/network.cpp) a integrovaný assembler ji hledá
# v pracovním adresáři, ne vedle zdroje.
target_compile_options(stockfish PRIVATE
    -O3 -DNDEBUG -w -Wa,-I${CMAKE_CURRENT_SOURCE_DIR}/stockfish)

# Změna sítě musí vynutit nový překlad network.cpp.
set_source_files_properties(stockfish/nnue/network.cpp PROPERTIES
    OBJECT_DEPENDS ${STOCKFISH_SIT_CESTA})

# Engine nepotřebuje nic než standardní knihovnu; log je jen pro ladění.
find_library(log-lib log)

# Na Stockfish odkazuje stockfish_uci.cpp, linker si z knihovny vezme, co
# potřebuje. (V kroku S1 tu bylo dočasně --whole-archive, viz deník.)
target_link_libraries(redcry stockfish ${log-lib})
