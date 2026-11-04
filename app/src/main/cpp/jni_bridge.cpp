// ---------------------------------------------------------------------------
//  Light Red - most JNI
//
//  Protějšek třídy cz.redcry.chess.engine.LightRedBridge.
//
//  Rozhraní je záměrně úzké: jedna metoda posílá dovnitř textový příkaz UCI
//  a dostává zpátky odpovědi. Vedle ní stojí tenké obálky pro to, co UI
//  potřebuje často (legalMoves, applyMove), a nativeStop, které se jako
//  jediné volá z jiného vlákna.
//
//  Na co si dát pozor a co je tu ohlídané:
//   - každé GetStringUTFChars je spárované s ReleaseStringUTFChars,
//     i na chybových cestách (RAII třída jniutil::Retezec v jni_util.h)
//   - veškerý výstup je čisté ASCII, takže se modifikované UTF-8 neuplatní
//   - nativeCommand je pod zámkem, nativeStop pod ním být nesmí, jinak
//     by se UI vlákno zablokovalo na běžícím hledání
// ---------------------------------------------------------------------------

#include <jni.h>

#include <mutex>
#include <new>
#include <string>

#include "jni_util.h"
#include "uci.h"

namespace {

struct Instance {
    lightred::UciEngine engine;
    std::mutex          zamek;      // serializuje nativeCommand
};

inline Instance* z_handle(jlong h) {
    return reinterpret_cast<Instance*>(static_cast<intptr_t>(h));
}

// Retezec, na_jstring a spoj jsou v jni_util.h - sdili je oba mosty.
using jniutil::Retezec;
using jniutil::na_jstring;
using jniutil::spoj;

// provede příkaz pod zámkem a odchytí případnou výjimku
std::string proved(Instance* inst, const std::string& prikaz) {
    if (!inst) return "info string neplatny handle";
    try {
        std::lock_guard<std::mutex> zamek(inst->zamek);
        return spoj(inst->engine.prikaz(prikaz));
    } catch (const std::exception& e) {
        return std::string("info string vyjimka: ") + e.what();
    } catch (...) {
        return "info string neznama vyjimka";
    }
}

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeNew(JNIEnv*, jclass) {
    Instance* inst = new (std::nothrow) Instance();
    return static_cast<jlong>(reinterpret_cast<intptr_t>(inst));
}

JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeDelete(JNIEnv*, jclass, jlong handle) {
    delete z_handle(handle);
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeCommand(JNIEnv* env, jclass,
                                                         jlong handle, jstring prikaz) {
    Retezec vstup(env, prikaz);
    if (!vstup.platny()) return na_jstring(env, "info string prazdny prikaz");
    return na_jstring(env, proved(z_handle(handle), vstup.text()));
}

// Jediná metoda volaná z jiného vlákna než nativeCommand. Zámek nebere
// schválně: hledání zrovna běží a drží ho.
JNIEXPORT void JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeStop(JNIEnv*, jclass, jlong handle) {
    Instance* inst = z_handle(handle);
    if (inst) inst->engine.zastav_hledani();
}

// Průběh běžícího hledání. Stejně jako nativeStop NEBERE zámek: čte se
// z UI vlákna, zatímco hledání drží zámek a počítá.
JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeSearchInfo(JNIEnv* env, jclass,
                                                            jlong handle) {
    Instance* inst = z_handle(handle);
    if (!inst) return na_jstring(env, "");
    return na_jstring(env, inst->engine.prubeh().text());
}

// --- tenké obálky nad tím, co UI potřebuje často ---------------------------

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeLegalMoves(JNIEnv* env, jclass,
                                                            jlong handle) {
    return na_jstring(env, proved(z_handle(handle), "legalmoves"));
}

JNIEXPORT jboolean JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeApplyMove(JNIEnv* env, jclass,
                                                           jlong handle, jstring tah) {
    Retezec vstup(env, tah);
    if (!vstup.platny()) return JNI_FALSE;
    const std::string odpoved = proved(z_handle(handle), "applymove " + vstup.text());
    return odpoved == "ok" ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_cz_redcry_chess_engine_LightRedBridge_nativeFen(JNIEnv* env, jclass, jlong handle) {
    const std::string odpoved = proved(z_handle(handle), "d");
    const size_t konec = odpoved.find('\n');
    return na_jstring(env, konec == std::string::npos ? odpoved
                                                      : odpoved.substr(0, konec));
}

}  // extern "C"
