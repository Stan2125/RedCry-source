import java.io.File
import java.io.IOException
import java.net.URI
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.security.MessageDigest

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
}

// Sit NNUE Stockfishe 19 (EvalFileDefaultName v evaluate.h). Neni v gitu,
// ziska se `./gradlew stahniSitStockfish`. Jedine misto, kde jsou jmeno,
// velikost a SHA-256 - CMake je dostane jako -D argumenty (nize) a overi.
// Hodnoty musi sedet s app/src/main/cpp/stockfish/PUVOD.md.
val sitStockfishe = "nn-1a298aa575a0.nnue"
val sitStockfisheVelikost = 98511183L
val sitStockfisheSha256 = "1a298aa575a085434d29027978dc36867fe9c5bcea9376654b7a8eba1e52dfc2"
val sitStockfisheZdroje = listOf(
    "https://tests.stockfishchess.org/api/nn/",
    "https://github.com/official-stockfish/networks/raw/master/",
)

android {
    namespace = "cz.redcry.chess"
    compileSdk = 36
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "cz.redcry.chess"
        minSdk = 26
        targetSdk = 36
        versionCode = 3
        versionName = "0.3"

        // ABI (arm64-v8a pro telefon, x86_64 pro emulator) urcuje jen
        // splits.abi nize. ndk.abiFilters tu byt nesmi: AGP ho vedle splits
        // odmitne ("Conflicting configuration ... cannot be present when
        // splits abi filters are set"). CMake se pousti pro ABI ze splits.

        externalNativeBuild {
            cmake {
                // Zarovnani segmentu .so na 16 KB stranky. Google Play to od
                // 11/2025 vyzaduje u kazde aplikace s targetSdk 35+, jinak
                // build neprojde revizi. NDK r27 to umi, ale defaultne to ma
                // vypnute - od r28 uz je to zapnute samo a tenhle radek zmizi.
                arguments += listOf("-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON")
                // Sit Stockfishe: CMake overi velikost a SHA-256 pred prekladem.
                arguments += listOf(
                    "-DSTOCKFISH_SIT=$sitStockfishe",
                    "-DSTOCKFISH_SIT_VELIKOST=$sitStockfisheVelikost",
                    "-DSTOCKFISH_SIT_SHA256=$sitStockfisheSha256",
                )
            }
        }
    }

    // Krok U8 - zvlastni APK pro kazde ABI. libredcry.so nese vlozenou sit
    // Stockfishe (98,5 MB, incbin), takze spolecne APK pro obe ABI melo
    // ~210 MB a telefon by v nem tahal i nepotrebnou x86_64 knihovnu.
    // Telefon dostane jen arm64-v8a, emulator jen x86_64.
    splits {
        abi {
            isEnable = true
            reset()
            include("arm64-v8a", "x86_64")
            isUniversalApk = false
        }
    }

    // Krok P1 - splits.abi plati jen pro APK, ne pro AAB (bundleRelease).
    // Do AAB by se dostala i armeabi-v7a a x86, protoze pro ne ma Compose
    // libandroidx.graphics.path.so. Google Play by pak aplikaci nabidl
    // i zarizenim, pro ktera libredcry.so neni, a ta by pri startu spadla.
    packaging {
        jniLibs {
            excludes += setOf("lib/armeabi-v7a/**", "lib/x86/**")
        }
    }

    // Krok 2 projektu - nativni vrstva.
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        compose = true
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.androidx.lifecycle.runtime.compose)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.ui)
    implementation(libs.androidx.ui.graphics)
    implementation(libs.androidx.ui.tooling.preview)
    implementation(libs.androidx.material3)
    implementation(libs.androidx.material.icons.core)
    debugImplementation(libs.androidx.ui.tooling)

    // JVM testy ViewModelu - bez Androidu a bez nativni knihovny.
    testImplementation(libs.junit)
    testImplementation(libs.kotlinx.coroutines.test)
}

// Stazeni site Stockfishe do app/src/main/cpp/stockfish/. Na build se
// schvalne nevesi: build nema tise sahat na sit, jen overi, ze sit je
// (CMakeLists.txt) a pripadne rekne, ze se ma pustit tahle uloha.
tasks.register("stahniSitStockfish") {
    group = "redcry"
    description = "Stahne sit NNUE Stockfishe a overi jeji SHA-256."
    val cil = layout.projectDirectory.file("src/main/cpp/stockfish/$sitStockfishe").asFile
    val jmeno = sitStockfishe
    val sha = sitStockfisheSha256
    val zdroje = sitStockfisheZdroje
    doLast {
        fun sha256(f: File): String {
            val md = MessageDigest.getInstance("SHA-256")
            f.inputStream().use { vstup ->
                val buf = ByteArray(1 shl 20)
                while (true) {
                    val n = vstup.read(buf)
                    if (n < 0) break
                    md.update(buf, 0, n)
                }
            }
            return md.digest().joinToString("") { "%02x".format(it) }
        }

        if (cil.isFile && sha256(cil) == sha) {
            logger.lifecycle("$jmeno: uz je na miste, SHA-256 sedi")
            return@doLast
        }

        val docasny = File(cil.parentFile, "$jmeno.stahuje")
        var chyba = ""
        for (zdroj in zdroje) {
            val url = zdroj + jmeno
            logger.lifecycle("$jmeno: stahuji z $url")
            try {
                URI(url).toURL().openStream().use { vstup ->
                    docasny.outputStream().use { vstup.copyTo(it, 1 shl 20) }
                }
            } catch (e: IOException) {
                docasny.delete()
                chyba = "$url: $e"
                logger.warn("$jmeno: stazeni selhalo ($e)")
                continue
            }
            val skutecny = sha256(docasny)
            if (skutecny != sha) {
                docasny.delete()
                throw GradleException(
                    "$jmeno ze $url ma SHA-256 $skutecny, ocekavano $sha - smazano")
            }
            Files.move(
                docasny.toPath(), cil.toPath(),
                StandardCopyOption.REPLACE_EXISTING,
                StandardCopyOption.ATOMIC_MOVE)
            logger.lifecycle("$jmeno: stazeno, SHA-256 sedi")
            return@doLast
        }
        throw GradleException("$jmeno se nepodarilo stahnout: $chyba")
    }
}
