package cz.redcry.chess.ui

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Info
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import cz.redcry.chess.model.PieceType
import cz.redcry.chess.model.Side
import cz.redcry.chess.texty.LocalTexty

// Mensi prvky obrazovky: dlazdice, tlacitka, menu Hra, dialog promeny
// a od kroku U15 prepinac jazyka.
// Vzniklo v kroku R1 presunem z GameScreen.kt.

@Composable
internal fun LabelledValue(
    label: String,
    value: String,
    modifier: Modifier = Modifier,
    /** Krok J2: tucna hodnota pred popiskem ("White to move"). Mezera je porad v popisku. */
    hodnotaPrvni: Boolean = false,
) {
    Row(modifier = modifier, verticalAlignment = Alignment.CenterVertically) {
        if (hodnotaPrvni) {
            Text(value, fontSize = 14.sp, fontWeight = FontWeight.Medium)
            Text(" " + label, fontSize = 14.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        } else {
            Text(label + " ", fontSize = 14.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Text(value, fontSize = 14.sp, fontWeight = FontWeight.Medium)
        }
    }
}

@Composable
internal fun MetricCard(label: String, value: String, modifier: Modifier = Modifier) {
    // Box kvuli svislemu vycentrovani v pevne vysce z navrhu. Row uvnitr
    // zarovnava na zakladni linku, at popisek a hodnota sedi na jedne lince.
    Box(
        modifier = modifier
            .background(MaterialTheme.colorScheme.surfaceVariant, RoundedCornerShape(8.dp))
            .padding(horizontal = 12.dp),
        contentAlignment = Alignment.CenterStart,
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            Text(
                text = label,
                fontSize = 13.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.alignByBaseline(),
            )
            Text(
                text = value,
                fontSize = 13.sp,
                fontWeight = FontWeight.Medium,
                color = MaterialTheme.colorScheme.onSurface,
                modifier = Modifier.alignByBaseline(),
            )
        }
    }
}

/**
 * Vyber figury pri promene: ctyri figury vedle sebe v barve strany, ktera
 * tahne. Kresli je tytez [drawPiece] jako deska, zadna druha kopie.
 *
 * Zavreni mimo dialog nebo Zrusit tah nezahraje a zrusi vyber na desce.
 */
@Composable
internal fun PromenaDialog(
    strana: Side,
    onVyber: (PieceType) -> Unit,
    onDismiss: () -> Unit,
) {
    val texty = LocalTexty.current
    // Figury, na ktere se da promenit, v obvyklem poradi D, V, S, J.
    val figuryPromeny = listOf(
        PieceType.QUEEN to texty.dama,
        PieceType.ROOK to texty.vez,
        PieceType.BISHOP to texty.strelec,
        PieceType.KNIGHT to texty.jezdec,
    )
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(texty.promena) },
        text = {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
            ) {
                figuryPromeny.forEach { (typ, nazev) ->
                    Canvas(
                        modifier = Modifier
                            .size(56.dp)
                            .background(
                                MaterialTheme.colorScheme.surfaceVariant,
                                RoundedCornerShape(8.dp),
                            )
                            .clickable(onClickLabel = nazev) { onVyber(typ) },
                    ) {
                        drawPiece(typ, strana, x = 0f, y = 0f, cell = size.width)
                    }
                }
            }
        },
        confirmButton = {},
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(texty.zrusit) }
        },
    )
}

/**
 * Tlacitko "Hra" s rozbalovacim menu (krok U6/A). Menu je tentyz
 * DropdownMenu se stejnym pismem jako vyber motoru; pod Novou partii
 * je oddelovac. Co ktera polozka udela, rozhoduje [provedPolozkuHry].
 */
@Composable
internal fun HraMenu(
    analyzaPovolena: Boolean,
    /** Fajfka u polozky Knihovna zahajeni (krok K1/B). */
    knihovnaZapnuta: Boolean,
    onOtevrit: () -> Boolean,
    onPolozka: (PolozkaHry) -> Unit,
    modifier: Modifier = Modifier,
) {
    val texty = LocalTexty.current
    var expanded by remember { mutableStateOf(false) }
    Box(modifier = modifier) {
        OutlinedButton(
            // Za analyzy se menu neotevre (A1/B), ViewModel rekne proc.
            onClick = { if (onOtevrit()) expanded = true },
            modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(horizontal = 4.dp),
        ) {
            Text(texty.hra, fontSize = 13.sp, fontWeight = FontWeight.Bold, maxLines = 1)
            Icon(
                imageVector = Icons.Default.ArrowDropDown,
                contentDescription = null,
                modifier = Modifier.size(16.dp),
            )
        }
        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
        ) {
            val itemStyle = MaterialTheme.typography.bodyMedium
            PolozkaHry.entries.forEach { polozka ->
                DropdownMenuItem(
                    text = { Text(texty.polozkaHry(polozka), style = itemStyle) },
                    // Analyza partie bez tahu nema co analyzovat (A1/B).
                    enabled = polozka != PolozkaHry.ANALYZA || analyzaPovolena,
                    // Krok K1/B: menu se po klepnuti zavre jako u ostatnich
                    // polozek, novy stav potvrdi hlaska.
                    trailingIcon = if (polozka == PolozkaHry.KNIHOVNA && knihovnaZapnuta) {
                        { Icon(Icons.Default.Check, contentDescription = texty.zapnuto) }
                    } else {
                        null
                    },
                    onClick = {
                        expanded = false
                        onPolozka(polozka)
                    },
                )
                if (polozka == PolozkaHry.NOVA_PARTIE) HorizontalDivider()
            }
        }
    }
}

/**
 * Tlacitko v rade pod ovladanim. [zvyrazneny] (krok U12/C, jen ⟳): aktivni
 * tlacitko je plne v hlavni barve motivu, aby bylo na prvni pohled videt,
 * ze se partie od nactene odchylila - obrysove aktivni a obrysove sede se
 * od sebe lisily jen odstinem. Neaktivni zustava obrysove jako ostatni.
 * Button a OutlinedButton maji stejny tvar i minimalni vysku, takze rada
 * zustava stejna.
 */
@Composable
internal fun StepButton(
    icon: ImageVector,
    contentDescription: String,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    enabled: Boolean = true,
    zvyrazneny: Boolean = false,
) {
    if (zvyrazneny && enabled) {
        Button(
            onClick = onClick,
            modifier = modifier,
            contentPadding = PaddingValues(0.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.primary,
                contentColor = MaterialTheme.colorScheme.onPrimary,
            ),
        ) {
            Icon(icon, contentDescription = contentDescription, modifier = Modifier.size(20.dp))
        }
        return
    }
    OutlinedButton(
        onClick = onClick,
        enabled = enabled,
        modifier = modifier,
        contentPadding = PaddingValues(0.dp),
    ) {
        Icon(icon, contentDescription = contentDescription, modifier = Modifier.size(20.dp))
    }
}

/**
 * Prepinac jazyka (krok U15): neutralni draha s koleckem v hlavni barve,
 * na kolecku zkratka jazyka. CZ je kolecko vlevo, EN vpravo.
 *
 * Vlastni prvek, ne Material Switch: ten ma pevnych 52 x 32 dp a roztazeny
 * na velikost tlacitek by z kolecka udelal elipsu. Tvar a pomer kolecka
 * k vysce drahy (24 : 32) jsou prevzate ze Switch, obrys z OutlinedButton,
 * aby prepinac vedle (i) vypadal jako jeden z rady.
 *
 * Vzdycky povoleny: na partii nema vliv, takze ho hledani nezamyka.
 */
@Composable
internal fun PrepinacJazyka(
    jazyk: Jazyk,
    onZmena: (Jazyk) -> Unit,
    modifier: Modifier = Modifier,
) {
    val texty = LocalTexty.current
    BoxWithConstraints(
        modifier = modifier
            .clip(CircleShape)
            .background(MaterialTheme.colorScheme.surfaceContainerHighest)
            .border(1.dp, MaterialTheme.colorScheme.outline, CircleShape)
            .semantics {
                contentDescription = texty.jazyk
                stateDescription =
                    if (jazyk == Jazyk.CESTINA) texty.nazevCestiny else texty.nazevAnglictiny
            }
            .toggleable(
                value = jazyk == Jazyk.ANGLICTINA,
                role = Role.Switch,
                onValueChange = { onZmena(jazyk.druhy()) },
            ),
    ) {
        val vyska = maxHeight
        val prumer = vyska * 24 / 32
        // Stred kolecka je vyska / 2 od leveho (CZ) nebo praveho (EN) kraje.
        val stred = if (jazyk == Jazyk.CESTINA) vyska / 2 else maxWidth - vyska / 2
        val posun by animateDpAsState(stred - prumer / 2, label = "prepinacJazyka")
        Box(
            modifier = Modifier
                .offset(x = posun)
                .align(Alignment.CenterStart)
                .size(prumer)
                .background(MaterialTheme.colorScheme.primary, CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = jazyk.zkratka,
                fontSize = 13.sp,
                fontWeight = FontWeight.Bold,
                color = MaterialTheme.colorScheme.onPrimary,
                maxLines = 1,
            )
        }
    }
}

/**
 * Horni radek leveho sloupce ovladani (krok U15): prepinac jazyka a (i).
 * Oba maji velikost tlacitek ‹ a › z rady dole a stoji presne nad nimi.
 * (i) se sem presunulo z rady dole, kde ho zakryvala hlaska Snackbaru.
 */
@Composable
internal fun RadekJazyka(
    n: Navrh,
    jazyk: Jazyk,
    onJazyk: (Jazyk) -> Unit,
    onInfo: () -> Unit,
) {
    val texty = LocalTexty.current
    Row(
        modifier = Modifier.height(n.px(Navrh.RADEK_JAZYKA)),
        horizontalArrangement = Arrangement.spacedBy(n.px(Navrh.MEZERA_SIPEK)),
    ) {
        val sirka = Modifier.width(n.px(Navrh.SIRKA_TLACITKA)).fillMaxHeight()
        PrepinacJazyka(jazyk = jazyk, onZmena = onJazyk, modifier = sirka)
        StepButton(
            icon = Icons.Default.Info,
            contentDescription = texty.zaznamPartiePopis,
            onClick = onInfo,
            modifier = sirka,
        )
    }
}
