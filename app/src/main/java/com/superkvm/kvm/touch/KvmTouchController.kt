package com.superkvm.kvm.touch

import android.content.Context
import android.graphics.Matrix
import android.os.SystemClock
import android.util.Log
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.View
import android.view.ViewConfiguration
import android.view.TextureView
import androidx.core.view.GestureDetectorCompat
import com.superkvm.kvm.ch9329.VideoContentMapper
import com.superkvm.kvm.hid.KvmHidBridge


class KvmTouchController(
    context: Context,
    private val targetView: View,
    private val bridge: KvmHidBridge,
    private val videoSizeProvider: () -> Pair<Int, Int>,
    
    private val onLongPressMenuRequested: (anchorX: Float, anchorY: Float, x4095: Int, y4095: Int) -> Unit
) : View.OnTouchListener {

    private val touchSlop = ViewConfiguration.get(context).scaledTouchSlop
    

    private var downX = 0f
    private var downY = 0f
    private var longPressFired = false
    private var rightClickPending = false
    private var leftDragActive = false
    
    private var longPressLeftDownActive = false
    private var longPressDragging = false
    
    private var wheelPending = false
    private var wheelDirPending = 0

    
    private val nonZoomTouchLeftPressPatchEnabled = false

    
    private val textureView: TextureView? = targetView as? TextureView
    private var zoomScale = 1f
    private var panX = 0f
    private var panY = 0f
    private val minZoom = 1f
    
    private val maxZoom = 6.0f
    private val zoomedThreshold = 1.01f
    private var panActive = false
    private var panDownX = 0f
    private var panDownY = 0f
    private var lastPanX = 0f
    private var lastPanY = 0f
    private var suppressNextTap = false

    
    private var firstTap4095: Int? = null
    private var firstTap4095Y: Int? = null
    private var firstTapInside: Boolean = false
    private var firstTapAtMs: Long = 0L

    private val doubleTapTimeoutMs: Long =
        ViewConfiguration.getDoubleTapTimeout().toLong()

    private val scaleDetector = ScaleGestureDetector(
        context,
        object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
            override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
                
                panActive = false
                suppressNextTap = true
                return true
            }

            override fun onScale(detector: ScaleGestureDetector): Boolean {
                val w = targetView.width.toFloat()
                val h = targetView.height.toFloat()
                if (w <= 0f || h <= 0f) return true

                val oldScale = zoomScale
                val rawNewScale = oldScale * detector.scaleFactor
                val newScale = rawNewScale.coerceIn(minZoom, maxZoom)
                if (kotlin.math.abs(newScale - oldScale) < 1e-4f) return true

                val cx = w / 2f
                val cy = h / 2f
                val focusX = detector.focusX
                val focusY = detector.focusY

                
                
                if (oldScale > 0f) {
                    val ratio = newScale / oldScale
                    panX = (focusX - cx) - (focusX - cx - panX) * ratio
                    panY = (focusY - cy) - (focusY - cy - panY) * ratio
                } else {
                    panX = 0f
                    panY = 0f
                }

                zoomScale = newScale
                clampPan()
                applyTextureTransform()
                return true
            }

            override fun onScaleEnd(detector: ScaleGestureDetector) {
                
                if (zoomScale <= 1.001f) {
                    zoomScale = 1f
                    panX = 0f
                    panY = 0f
                    applyTextureTransform()
                }
            }
        }
    )

    private val gesture = GestureDetectorCompat(
        context,
        object : GestureDetector.SimpleOnGestureListener() {
            override fun onDown(e: MotionEvent): Boolean = true

            override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
                if (longPressFired || leftDragActive || longPressLeftDownActive) return false
                if (wheelPending) return false
                
                val (x4095, y4095, inside) = map(e.x, e.y)
                if (inside) {
                    bridge.moveAbsNoButtons(x4095, y4095)
                }
                firstTap4095 = null
                firstTap4095Y = null
                firstTapInside = false
                firstTapAtMs = 0L
                return true
            }

            override fun onSingleTapUp(e: MotionEvent): Boolean {
                if (longPressFired || leftDragActive || longPressLeftDownActive) return false
                if (wheelPending) return false

                val now = SystemClock.uptimeMillis()
                val (x4095, y4095, inside) = map(e.x, e.y)

                if (firstTap4095 == null || now - firstTapAtMs > doubleTapTimeoutMs) {
                    firstTap4095 = x4095
                    firstTap4095Y = y4095
                    firstTapInside = inside
                    firstTapAtMs = now
                    Log.d(
                        "superkvm",
                        "doubleTap path: first TAP_UP cached screen=(${e.x},${e.y}) hid=($x4095,$y4095) inside=$inside " +
                            "timeoutMs=$doubleTapTimeoutMs"
                    )
                } else {
                    Log.d(
                        "superkvm",
                        "doubleTap path: second TAP_UP (within timeout) screen=(${e.x},${e.y}) hid=($x4095,$y4095) inside=$inside " +
                            "firstCached=($firstTap4095,$firstTap4095Y) firstInside=$firstTapInside " +
                            "deltaMs=${now - firstTapAtMs}"
                    )
                }
                return true
            }

            override fun onDoubleTap(e: MotionEvent): Boolean {
                
                if (wheelPending) {
                    Log.d("superkvm", "doubleTap IGNORED: wheelPending")
                    return false
                }

                val (secondX4095, secondY4095, secondInside) = map(e.x, e.y)
                val firstX = firstTap4095
                val firstY = firstTap4095Y
                val firstOk = firstX != null && firstY != null && firstTapInside

                val branch = when {
                    firstOk && secondInside -> "firstOk+secondInside"
                    firstOk -> "firstOk_only"
                    secondInside -> "secondInside_only"
                    else -> "none_inside_skip_hid"
                }
                Log.d(
                    "superkvm",
                    "doubleTap RECOGNIZED: second DOWN screen=(${e.x},${e.y}) hid=($secondX4095,$secondY4095) inside=$secondInside " +
                        "firstOk=$firstOk first=($firstX,$firstY) firstInside=$firstTapInside branch=$branch"
                )

                when {
                    firstOk && secondInside -> {
                        val x0 = firstX!!
                        val y0 = firstY!!
                        bridge.moveAbsNoButtons(x0, y0)
                        bridge.moveAbsNoButtons(secondX4095, secondY4095)
                    }
                    firstOk -> {
                        val x0 = firstX!!
                        val y0 = firstY!!
                        bridge.moveAbsNoButtons(x0, y0)
                        bridge.moveAbsNoButtons(x0, y0)
                    }
                    secondInside -> {
                        bridge.moveAbsNoButtons(secondX4095, secondY4095)
                        bridge.moveAbsNoButtons(secondX4095, secondY4095)
                    }
                    else -> {
                        firstTap4095 = null
                        firstTap4095Y = null
                        firstTapInside = false
                        firstTapAtMs = 0L
                        Log.d("superkvm", "doubleTap done: no move (both outside content), no append click")
                        return true
                    }
                }

                firstTap4095 = null
                firstTap4095Y = null
                firstTapInside = false
                firstTapAtMs = 0L

                Log.d("superkvm", "doubleTap done: sent 2x moveAbsNoButtons branch=$branch")

                if (secondInside) {
                    bridge.leftClickAt(secondX4095, secondY4095)
                    Log.d(
                        "superkvm",
                        "doubleTap append: leftClickAt(second) hid=($secondX4095,$secondY4095)"
                    )
                }
                return true
            }

            override fun onDoubleTapEvent(e: MotionEvent): Boolean {
                when (e.actionMasked) {
                    MotionEvent.ACTION_DOWN ->
                        Log.d("superkvm", "doubleTapEvent DOWN (${e.x},${e.y})")
                    MotionEvent.ACTION_UP ->
                        Log.d("superkvm", "doubleTapEvent UP (${e.x},${e.y})")
                }
                return false
            }

            override fun onLongPress(e: MotionEvent) {
                longPressFired = true
                
                rightClickPending = false
                leftDragActive = false
                longPressLeftDownActive = false
                longPressDragging = false
                wheelPending = false
                wheelDirPending = 0

                val (x4095, y4095, inside) = map(e.x, e.y)
                
                Log.d(
                    "superkvm",
                    "longPress at (${e.x},${e.y}) -> 4095=($x4095,$y4095) inside=$inside"
                )
                onLongPressMenuRequested(e.x, e.y, x4095, y4095)

                firstTap4095 = null
                firstTap4095Y = null
                firstTapInside = false
                firstTapAtMs = 0L
            }

            override fun onScroll(
                e1: MotionEvent?,
                e2: MotionEvent,
                distanceX: Float,
                distanceY: Float
            ): Boolean {
                if (longPressFired || leftDragActive || longPressLeftDownActive) return false
                
                if (zoomScale > zoomedThreshold) return false
                
                if (kotlin.math.abs(distanceY) < kotlin.math.abs(distanceX)) return false
                
                wheelDirPending = if (distanceY > 0f) -1 else 1
                wheelPending = true
                Log.d("superkvm", "wheel(swipe) distanceY=$distanceY wheelDir=$wheelDirPending")
                return true
            }
        }
    ).apply {
        setIsLongpressEnabled(true)
    }

    private fun map(x: Float, y: Float): VideoContentMapper.Result {
        val (vw, vh) = videoSizeProvider()
        val w = targetView.width.toFloat()
        val h = targetView.height.toFloat()
        if (w <= 0f || h <= 0f) {
            return VideoContentMapper.Result(2047, 2047, false)
        }

        
        
        
        
        val cx = w / 2f
        val cy = h / 2f
        val invX = cx + (x - cx - panX) / zoomScale
        val invY = cy + (y - cy - panY) / zoomScale

        return VideoContentMapper.touchTo4095(
            targetView.width,
            targetView.height,
            vw,
            vh,
            invX,
            invY
        )
    }

    private fun clampPan() {
        val w = targetView.width.toFloat()
        val h = targetView.height.toFloat()
        if (w <= 0f || h <= 0f) return
        if (zoomScale <= 1f) {
            panX = 0f
            panY = 0f
            return
        }
        val maxPanX = (w / 2f) * (zoomScale - 1f)
        val maxPanY = (h / 2f) * (zoomScale - 1f)
        panX = panX.coerceIn(-maxPanX, maxPanX)
        panY = panY.coerceIn(-maxPanY, maxPanY)
    }

    private fun applyTextureTransform() {
        val tv = textureView ?: return
        val w = targetView.width.toFloat()
        val h = targetView.height.toFloat()
        if (w <= 0f || h <= 0f) return

        val cx = w / 2f
        val cy = h / 2f

        
        
        val dx = cx * (1f - zoomScale) + panX
        val dy = cy * (1f - zoomScale) + panY

        val m = Matrix()
        m.setScale(zoomScale, zoomScale)
        m.postTranslate(dx, dy)
        tv.setTransform(m)
    }

    private fun handleZoomPan(event: MotionEvent): Boolean {
        val zoomed = zoomScale > zoomedThreshold
        if (!zoomed) return false

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                panActive = false
                panDownX = event.x
                panDownY = event.y
                lastPanX = event.x
                lastPanY = event.y
                
                longPressFired = false
                rightClickPending = false
                leftDragActive = false
                longPressLeftDownActive = false
                longPressDragging = false
                wheelPending = false
                wheelDirPending = 0
            }
            MotionEvent.ACTION_MOVE -> {
                val dxDown = event.x - panDownX
                val dyDown = event.y - panDownY
                if (longPressLeftDownActive) {
                    
                    if (!longPressDragging && (dxDown * dxDown + dyDown * dyDown) > touchSlop * touchSlop) {
                        longPressDragging = true
                        rightClickPending = false
                        panActive = true
                        lastPanX = event.x
                        lastPanY = event.y
                    } else if (longPressDragging) {
                        val dx = event.x - lastPanX
                        val dy = event.y - lastPanY
                        lastPanX = event.x
                        lastPanY = event.y
                        panX += dx
                        panY += dy
                        clampPan()
                        applyTextureTransform()
                    }
                    return true
                }
                if (!panActive && (dxDown * dxDown + dyDown * dyDown) > touchSlop * touchSlop) {
                    panActive = true
                }
                if (panActive) {
                    val dx = event.x - lastPanX
                    val dy = event.y - lastPanY
                    lastPanX = event.x
                    lastPanY = event.y

                    panX += dx
                    panY += dy
                    clampPan()
                    applyTextureTransform()
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (longPressLeftDownActive) {
                    
                    if (!longPressDragging && rightClickPending && longPressFired) {
                        val (x4095, y4095, inside) = map(event.x, event.y)
                        if (inside) bridge.rightClickAt(x4095, y4095)
                    }
                    longPressLeftDownActive = false
                    longPressDragging = false
                }

                if (suppressNextTap) {
                    suppressNextTap = false
                }

                panActive = false
                wheelPending = false
                wheelDirPending = 0

                rightClickPending = false
                longPressFired = false
                leftDragActive = false
            }
        }
        return true
    }

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        
        scaleDetector.onTouchEvent(event)
        if (scaleDetector.isInProgress) {
            
            return true
        }

        
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                downX = event.x
                downY = event.y
                longPressFired = false
                rightClickPending = false
                leftDragActive = false
                longPressLeftDownActive = false
                longPressDragging = false
                wheelPending = false
                wheelDirPending = 0
                
                if (nonZoomTouchLeftPressPatchEnabled && zoomScale <= zoomedThreshold) {
                    val (x4095, y4095, inside) = map(event.x, event.y)
                    if (inside) bridge.leftPressOnlyAt(x4095, y4095)
                }
            }
        }

        
        gesture.onTouchEvent(event)

        
        if (zoomScale > zoomedThreshold) {
            return handleZoomPan(event)
        }

        
        when (event.actionMasked) {
            MotionEvent.ACTION_MOVE -> {
                if (longPressLeftDownActive) {
                    val dx = event.x - downX
                    val dy = event.y - downY
                    if (!longPressDragging && (dx * dx + dy * dy > touchSlop * touchSlop)) {
                        longPressDragging = true
                        rightClickPending = false
                        val (x4095, y4095, _) = map(event.x, event.y)
                        bridge.setLeftDown(true)
                        bridge.moveAbs(x4095, y4095)
                    } else if (longPressDragging) {
                        val (x4095, y4095, _) = map(event.x, event.y)
                        bridge.moveAbs(x4095, y4095)
                    }
                } else {
                    
                    if (longPressFired && rightClickPending && !leftDragActive) {
                        val dx = event.x - downX
                        val dy = event.y - downY
                        if (dx * dx + dy * dy > touchSlop * touchSlop) {
                            rightClickPending = false
                            leftDragActive = true
                            bridge.setLeftDown(true)
                        }
                    }
                    if (leftDragActive) {
                        val (x4095, y4095, _) = map(event.x, event.y)
                        bridge.moveAbs(x4095, y4095)
                    }
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (longPressLeftDownActive) {
                    if (longPressDragging) {
                        bridge.setLeftDown(false)
                    } else if (rightClickPending && longPressFired) {
                        val (x4095, y4095, inside) = map(event.x, event.y)
                        if (inside) bridge.rightClickAt(x4095, y4095)
                    }
                    longPressLeftDownActive = false
                    longPressDragging = false
                } else {
                    if (leftDragActive) {
                        bridge.setLeftDown(false)
                    } else if (rightClickPending && longPressFired) {
                        val (x4095, y4095, inside) = map(event.x, event.y)
                        if (inside) {
                            bridge.rightClickAt(x4095, y4095)
                        }
                    }
                }

                
                if (wheelPending && !leftDragActive && !longPressLeftDownActive && !longPressFired) {
                    
                    val wheelDelta = wheelDirPending * 5
                    bridge.scroll(wheelDelta)
                    Log.d("superkvm", "wheel(normal send) wheelDelta=$wheelDelta wheelDir=$wheelDirPending")
                }

                wheelPending = false
                wheelDirPending = 0
                rightClickPending = false
                leftDragActive = false
                longPressFired = false
            }
        }
        return true
    }
}
