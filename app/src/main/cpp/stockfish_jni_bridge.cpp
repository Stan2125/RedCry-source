// ---------------------------------------------------------------------------
//  Stockfish 19 - most JNI
//
//  Protějšek třídy cz.redcry.chess.engine.StockfishBridge. Opis
//  crafty_jni_bridge.cpp s jiným motorem (sfredcry::UciEngine) a jinými
//  jmény symbolů; pomůcky z jni_util.h, držák instance, zámek a odchytávání
//  výjimek vlastní (jméno javové třídy je zadrátované do jména symbolu).
//
//  NEJVÝŠ JEDNA INSTANCE NARÁZ, ale z jiného důvodu než u Crafty. Stockfish
//  globály sdílené mezi instancemi nemá (jen tabulky útoků a klíče Zobrist,
//  které se naplní jednou a pak se jen čtou). Jde o PAMĚŤ: každý
//  Stockfish::Engine si nese vlastní kopii sítě NNUE (desítky MB),
//  transpoziční tabulku (Hash, výchozí 16 MB, nejvýš 256 MB) a vlastní
//  vlákna hledání. Dvě instance by paměť zdvojnásobily a aplikace nikdy
//  dvě nepotřebuje - přepnutí motoru starý uvolní dřív, než založí nový.
//  Druhé nativeNew, dokud první instance žije, proto vrátí 0 a Kotlin z toho
//  udělá srozumitelnou chybu. Po nativeDelete lze založit novou.
//
//  VLÁKNA STOCKFISHE DO JVM NEVOLAJÍ, AttachCurrentThread tu proto není.
//  Ověřeno čtením kódu (krok S3/A):
//   - JNIEnv se používá jen v exportovaných funkcích níž, tedy na vlákně,
//     které z Javy volá; nikam se neukládá a nepředává,
//   - hledání Stockfishe běží na vláknech jeho ThreadPool (thread.cpp,
//     NativeThread = std::thread). Z nich se volají jen zpětná volání, která
//     nastavuje konstruktor sfredcry::UciEngine (stockfish_uci.cpp):
//     on_update_full zapíše řádek do vektoru info_radky a atomiky
//     PrubeznyStav, on_bestmove zapíše řetězec, on_update_no_moves, on_iter,
//     on_start a on_verify_network nedělají nic. Nic z toho nezná JNI,
//   - vektor a řetězec čte vlákno nativeCommand až po
//     wait_for_search_finished (podmínková proměnná = synchronizace),
//     atomiky čte nativeSearchInfo kdykoli.
//
//  ROZDÍL PROTI CRAFTY: nativeDelete je bezpečné zavolat dvakrát se stejným
//  handle. Druhé volání nic neudělá, protože handle už není živá instance
//  (porovná se s ukazatelem v `ziva`). Kotlinová strana to hlídá taky
//  (close() handle vynuluje); tady je to pojistka pro test mostu, který
//  dvojí close zkouší přímo. Ostatní funkce s handle po nativeDelete volat
//  nelze, stejně jako u Crafty.
//
//  Na co si dát pozor a co je tu ohlídané (stejně jako u Crafty):
//   - každé GetStringUTFChars je spárované s ReleaseStringUTFChars
//     (RAII třída jniutil::Retezec)
//   - veškerý výstup je čisté ASCII, modifikované UTF-8 se neuplatní
//   - nativeCommand je pod zámkem, nativeStop a nativeSearchInfo pod ním
//     být nesmí, jinak by se UI vlákno zablokovalo na běžícím hledání
//   - nativeSanVarianty (krok U14/B) je statická a bez handle i zámku:
//     instanci nepotřebuje a volá se i bez založeného Stockfishe
//   - žádná výjimka nepřeleze hranici JNI
// ---------------------------------------------------------------------------

#include <jni.h>

#include <atomic>
#include <mutex>
#include <new>
#include <string>

#include "jni_util.h"
#include "stockfish_uci.h"

namespace {

struct Instance {
    sfredcry::UciEngine engine;
    std::mutex          zamek;      // serializuje nativeCommand
};

// true od začátku nativeNew, dokud žije instance - hlídá "nejvýš jedna naráz"
// i během zakládání (to trvá, Engine nahrává síť)
std::atomic<bool> obsazeno{false};

// živá instance, nebo nullptr - kvůli dvojímu nativeDelete
std::atomic<Instance*> ziva{nullptr};

inline Instance* z_handle(jlong h) {
    return reinterpret_cast<Instance*>(static_cast<intptr_t>(h));
}

// provede příkaz pod zámkem a odchytí případnou výjimku
std::string proved(Instance* inst, const std::string& prikaz) {
    if (!inst) return "info string neplatny handle";
    try {
        std::lock_guard<std::mutex> zamek(inst->zamek);
        return jniutil::spoj(inst->engine.prikaz(prikaz));
    } catch (const std::exception& e) {
        return std::string("info string vyjimka: ") + e.what();
    } catch (...) {
        return "info string neznama vyjimka";
    }
}

}  // namespace

extern "C" {

// 0 = instance už existuje, nebo se ji nepodařilo založit.
JNIEXPORT jlong JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeNew(JNIEnv*, jclass) {
    bool ceka = false;
    if (!obsazeno.compare_exchange_strong(ceka, true)) return 0;
    Instance* inst = nullptr;
    try {
        inst = new (std::nothrow) Instance();
    } catch (...) {
        inst = nullptr;
    }
    if (!inst) {
        obsazeno.store(false);
        return 0;
    }
    ziva.store(inst);
    return static_cast<jlong>(reinterpret_cast<intptr_t>(inst));
}

JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeDelete(JNIEnv*, jclass, jlong handle) {
    Instance* inst = z_handle(handle);
    if (!inst) return;
    // Jen živou instanci a jen jednou: druhé volání se stejným handle
    // compare_exchange nevyhraje.
    Instance* cekana = inst;
    if (!ziva.compare_exchange_strong(cekana, nullptr)) return;
    delete inst;
    obsazeno.store(false);
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeCommand(JNIEnv* env, jclass,
                                                          jlong handle, jstring prikaz) {
    jniutil::Retezec vstup(env, prikaz);
    if (!vstup.platny()) return jniutil::na_jstring(env, "info string prazdny prikaz");
    return jniutil::na_jstring(env, proved(z_handle(handle), vstup.text()));
}

// Volá se z jiného vlákna než nativeCommand. Zámek nebere schválně:
// hledání zrovna běží a drží ho.
JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeStop(JNIEnv*, jclass, jlong handle) {
    Instance* inst = z_handle(handle);
    if (inst) inst->engine.zastav_hledani();
}

// Průběh běžícího hledání. Stejně jako nativeStop NEBERE zámek.
JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeSearchInfo(JNIEnv* env, jclass,
                                                             jlong handle) {
    Instance* inst = z_handle(handle);
    if (!inst) return jniutil::na_jstring(env, "");
    return jniutil::na_jstring(env, inst->engine.prubeh().text());
}

// --- tenké obálky nad tím, co UI potřebuje často ---------------------------

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeLegalMoves(JNIEnv* env, jclass,
                                                             jlong handle) {
    return jniutil::na_jstring(env, proved(z_handle(handle), "legalmoves"));
}

JNIEXPORT jboolean JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeApplyMove(JNIEnv* env, jclass,
                                                            jlong handle, jstring tah) {
    jniutil::Retezec vstup(env, tah);
    if (!vstup.platny()) return JNI_FALSE;
    const std::string odpoved = proved(z_handle(handle), "applymove " + vstup.text());
    return odpoved == "ok" ? JNI_TRUE : JNI_FALSE;
}

// Hlavní varianta v SAN pro všechny motory (krok U14/B). Statická, bez
// handle: nepotřebuje instanci Stockfishe ani zámek (stockfish_uci.h).
JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeSanVarianty(JNIEnv* env, jclass,
                                                              jstring fen, jstring tahy) {
    jniutil::Retezec f(env, fen);
    jniutil::Retezec t(env, tahy);
    if (!f.platny() || !t.platny()) return jniutil::na_jstring(env, "");
    std::string san;
    try {
        san = sfredcry::san_varianty(f.text(), t.text());
    } catch (...) {
        // Žádná výjimka nepřeleze hranici JNI; prázdná = záložní tvar v Kotlinu.
        san.clear();
    }
    return jniutil::na_jstring(env, san);
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_StockfishBridge_nativeFen(JNIEnv* env, jclass, jlong handle) {
    const std::string odpoved = proved(z_handle(handle), "d");
    const size_t konec = odpoved.find('\n');
    return jniutil::na_jstring(env, konec == std::string::npos ? odpoved
                                                               : odpoved.substr(0, konec));
}

}  // extern "C"
