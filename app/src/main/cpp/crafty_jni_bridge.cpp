// ---------------------------------------------------------------------------
//  Crafty - most JNI
//
//  Protějšek třídy cz.redcry.chess.engine.CraftyBridge. Opis
//  red_max_jni_bridge.cpp s jiným motorem a jinými jmény symbolů; pomůcky
//  z jni_util.h, držák instance, zámek a odchytávání výjimek vlastní (jméno
//  javové třídy je zadrátované do jména symbolu, společný most nejde).
//
//  JEDINÝ ROZDÍL PROTI REDMAXU: nejvýš jedna instance naráz. Stav Crafty jsou
//  globály (crafty_core, crafty_search), takže dvě instance by tiše sdílely
//  jednu pozici i hash tabulku. Druhé nativeNew, dokud první instance žije,
//  proto vrátí 0 (PORT-CRAFTY.md, oddíl 6a) a Kotlin z toho udělá
//  srozumitelnou chybu. Po nativeDelete lze založit novou; konstruktor
//  UciEngine uvede globály do čistého stavu.
//
//  Na co si dát pozor a co je tu ohlídané (stejně jako u RedMAXu):
//   - každé GetStringUTFChars je spárované s ReleaseStringUTFChars
//     (RAII třída jniutil::Retezec)
//   - veškerý výstup je čisté ASCII, modifikované UTF-8 se neuplatní
//   - nativeCommand je pod zámkem, nativeStop a nativeSearchInfo pod ním
//     být nesmí, jinak by se UI vlákno zablokovalo na běžícím hledání
//   - žádná výjimka nepřeleze hranici JNI
// ---------------------------------------------------------------------------

#include <jni.h>

#include <atomic>
#include <mutex>
#include <new>
#include <string>

#include "jni_util.h"
#include "crafty_uci.h"

namespace {

struct Instance {
    crafty::UciEngine engine;
    std::mutex        zamek;      // serializuje nativeCommand
};

// true, dokud žije instance - hlídá "nejvýš jedna naráz"
std::atomic<bool> obsazeno{false};

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
Java_cz_redcry_chess_engine_CraftyBridge_nativeNew(JNIEnv*, jclass) {
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
    return static_cast<jlong>(reinterpret_cast<intptr_t>(inst));
}

JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeDelete(JNIEnv*, jclass, jlong handle) {
    Instance* inst = z_handle(handle);
    if (!inst) return;
    delete inst;
    obsazeno.store(false);
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeCommand(JNIEnv* env, jclass,
                                                       jlong handle, jstring prikaz) {
    jniutil::Retezec vstup(env, prikaz);
    if (!vstup.platny()) return jniutil::na_jstring(env, "info string prazdny prikaz");
    return jniutil::na_jstring(env, proved(z_handle(handle), vstup.text()));
}

// Volá se z jiného vlákna než nativeCommand. Zámek nebere schválně:
// hledání zrovna běží a drží ho.
JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeStop(JNIEnv*, jclass, jlong handle) {
    Instance* inst = z_handle(handle);
    if (inst) inst->engine.zastav_hledani();
}

// Průběh běžícího hledání. Stejně jako nativeStop NEBERE zámek.
JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeSearchInfo(JNIEnv* env, jclass,
                                                          jlong handle) {
    Instance* inst = z_handle(handle);
    if (!inst) return jniutil::na_jstring(env, "");
    return jniutil::na_jstring(env, inst->engine.prubeh().text());
}

// --- tenké obálky nad tím, co UI potřebuje často ---------------------------

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeLegalMoves(JNIEnv* env, jclass,
                                                          jlong handle) {
    return jniutil::na_jstring(env, proved(z_handle(handle), "legalmoves"));
}

JNIEXPORT jboolean JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeApplyMove(JNIEnv* env, jclass,
                                                         jlong handle, jstring tah) {
    jniutil::Retezec vstup(env, tah);
    if (!vstup.platny()) return JNI_FALSE;
    const std::string odpoved = proved(z_handle(handle), "applymove " + vstup.text());
    return odpoved == "ok" ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_CraftyBridge_nativeFen(JNIEnv* env, jclass, jlong handle) {
    const std::string odpoved = proved(z_handle(handle), "d");
    const size_t konec = odpoved.find('\n');
    return jniutil::na_jstring(env, konec == std::string::npos ? odpoved
                                                               : odpoved.substr(0, konec));
}

}  // extern "C"
