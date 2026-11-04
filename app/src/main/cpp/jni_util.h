// ---------------------------------------------------------------------------
//  Pomůcky pro mosty JNI
//
//  Vytaženo z jni_bridge.cpp (Light Red), když přibyl druhý most
//  (red_max_jni_bridge.cpp). Je tu JEN to, co je prokazatelně mechanické
//  a na motoru nezávislé: RAII kolem GetStringUTFChars, převod na jstring
//  a spojení řádků.
//
//  ZÁMĚRNĚ TU NENÍ nic dalšího. Vlastní most - držák instance, zámek,
//  odchytávání výjimek a jména exportovaných symbolů - je u každého motoru
//  vlastní a zůstává vlastní. Jméno javové třídy je zadrátované do jména
//  symbolu, takže společný most ani udělat nejde; a Crafty se Stockfishem
//  mluví UCI samy, takže je napojíme jinak. Zobecňovat se má podle druhého
//  a třetího případu, ne podle prvního.
//
//  Na co si dát pozor a co je tu ohlídané:
//   - každé GetStringUTFChars je spárované s ReleaseStringUTFChars,
//     i na chybových cestách (třída Retezec)
//   - veškerý výstup motorů je čisté ASCII, takže se modifikované UTF-8
//     neuplatní
// ---------------------------------------------------------------------------

#pragma once

#include <jni.h>

#include <string>
#include <vector>

namespace jniutil {

// RAII kolem GetStringUTFChars, aby nešlo zapomenout na uvolnění
class Retezec {
public:
    Retezec(JNIEnv* env, jstring s) : env_(env), js_(s) {
        if (js_) data_ = env_->GetStringUTFChars(js_, nullptr);
    }
    ~Retezec() {
        if (js_ && data_) env_->ReleaseStringUTFChars(js_, data_);
    }
    Retezec(const Retezec&) = delete;
    Retezec& operator=(const Retezec&) = delete;

    bool platny() const { return data_ != nullptr; }
    std::string text() const { return data_ ? std::string(data_) : std::string(); }

private:
    JNIEnv*     env_;
    jstring     js_;
    const char* data_ = nullptr;
};

inline jstring na_jstring(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

// spojí odpovědi do jednoho řetězce odděleného novým řádkem
inline std::string spoj(const std::vector<std::string>& radky) {
    std::string s;
    for (size_t i = 0; i < radky.size(); ++i) {
        if (i) s += '\n';
        s += radky[i];
    }
    return s;
}

}  // namespace jniutil
