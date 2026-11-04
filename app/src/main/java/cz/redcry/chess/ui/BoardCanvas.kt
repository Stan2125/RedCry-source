package cz.redcry.chess.ui

import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.keyframes
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.FilterQuality
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale
import androidx.compose.ui.graphics.drawscope.translate
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.imageResource
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import cz.redcry.chess.R
import cz.redcry.chess.model.PieceType
import cz.redcry.chess.model.Position
import cz.redcry.chess.model.Side
import cz.redcry.chess.model.Square
import androidx.compose.runtime.getValue

private val CursorColor = Color(0xFF00FFFF)
private val WhiteFill = Color(0xFFFFFFFF)
private val BlackFill = Color(0xFF141414)
private val TargetDot = Color(0x4D000000)

/**
 * Cela deska je jedno platno, ne 64 komponent. Prekresluje se jen pri zmene
 * stavu; blikani kurzoru bezi jako animace pruhlednosti a nesaha na zbytek.
 */
@Composable
fun BoardCanvas(
    position: Position,
    selected: Square?,
    targets: List<Square>,
    flipped: Boolean,
    onSquareClick: (Square) -> Unit,
    modifier: Modifier = Modifier,
) {
    val light = ImageBitmap.imageResource(R.drawable.wood_light)
    val dark = ImageBitmap.imageResource(R.drawable.wood_dark)

    val blink = rememberInfiniteTransition(label = "cursor")
    val cursorAlpha by blink.animateFloat(
        initialValue = 1f,
        targetValue = 0f,
        animationSpec = infiniteRepeatable(
            animation = keyframes {
                durationMillis = 900
                1f at 0
                1f at 449
                0f at 450
                0f at 899
            },
            repeatMode = RepeatMode.Restart,
        ),
        label = "cursorAlpha",
    )

    val targetSet = remember(targets) { targets.toHashSet() }

    Canvas(
        modifier = modifier
            .fillMaxWidth()
            .aspectRatio(1f)
            .pointerInput(flipped) {
                detectTapGestures { offset ->
                    val cell = size.width / 8f
                    val file = (offset.x / cell).toInt().coerceIn(0, 7)
                    val rank = (offset.y / cell).toInt().coerceIn(0, 7)
                    val visual = rank * 8 + file
                    onSquareClick(if (flipped) 63 - visual else visual)
                }
            }
    ) {
        val cell = size.width / 8f

        for (visual in 0 until 64) {
            val square = if (flipped) 63 - visual else visual
            val col = visual % 8
            val row = visual / 8
            val x = col * cell
            val y = row * cell

            val isLight = jeSvetle(square)
            drawImage(
                image = if (isLight) light else dark,
                dstOffset = IntOffset(x.toInt(), y.toInt()),
                dstSize = IntSize(cell.toInt() + 1, cell.toInt() + 1),
                filterQuality = FilterQuality.Low,
            )

            position[square]?.let { piece ->
                drawPiece(piece.type, piece.side, x = x, y = y, cell = cell)
            }

            if (square in targetSet) {
                drawCircle(
                    color = TargetDot,
                    radius = cell * 0.14f,
                    center = androidx.compose.ui.geometry.Offset(x + cell / 2f, y + cell / 2f),
                )
            }
        }

        selected?.let { sq ->
            val visual = if (flipped) 63 - sq else sq
            val stroke = cell * 0.08f
            translate(left = (visual % 8) * cell, top = (visual / 8) * cell) {
                drawRect(
                    color = CursorColor.copy(alpha = cursorAlpha),
                    topLeft = androidx.compose.ui.geometry.Offset(stroke / 2f, stroke / 2f),
                    size = androidx.compose.ui.geometry.Size(cell - stroke, cell - stroke),
                    style = Stroke(width = stroke),
                )
            }
        }
    }
}

/**
 * Jedna figura do ctverce o strane [cell] s levym hornim rohem v [x], [y].
 *
 * Internal kvuli dialogu pro vyber promeny, ktery kresli tytez figury -
 * druha kopie by se od desky casem rozesla v barvach i obrysu.
 */
internal fun DrawScope.drawPiece(
    type: PieceType,
    side: Side,
    x: Float,
    y: Float,
    cell: Float,
) {
    val path = PieceShapes.path(type)
    val fill = if (side == Side.WHITE) WhiteFill else BlackFill
    val outline = if (side == Side.WHITE) BlackFill else WhiteFill
    val factor = cell / PieceShapes.VIEWPORT
    translate(left = x, top = y) {
        scale(scaleX = factor, scaleY = factor, pivot = androidx.compose.ui.geometry.Offset.Zero) {
            drawPath(path = path, color = fill)
            drawPath(path = path, color = outline, style = Stroke(width = 1.2f))
        }
    }
}

/**
 * Je pole [square] svetle? a1 je tmave, h1 svetle.
 *
 * Ciste, bez Compose - v Canvasu se to jako vyraz nedalo otestovat a prave
 * ta jednicka/nula v parite byla spatne. Cislovani poli je stejne jako
 * u [cz.redcry.chess.model.squareName]: 0 = a8, 63 = h1.
 *
 * Plati `jeSvetle(sq) == jeSvetle(63 - sq)`, takze otoceni desky o 180
 * stupnu barvy poli nemeni.
 */
fun jeSvetle(square: Square): Boolean = ((square / 8) + (square % 8)) % 2 == 0
