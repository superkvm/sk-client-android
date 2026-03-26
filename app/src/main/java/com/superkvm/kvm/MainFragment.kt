
package com.superkvm.kvm

import android.content.Context
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.hardware.usb.UsbConstants
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.net.Uri
import android.media.AudioManager
import android.media.ToneGenerator
import android.os.SystemClock
import android.util.Log
import android.view.Gravity
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import android.widget.TextView
import android.view.ViewConfiguration
import android.widget.FrameLayout
import androidx.appcompat.app.AlertDialog
import com.superkvm.ausbc.MultiCameraClient
import com.superkvm.ausbc.base.CameraFragment
import com.superkvm.ausbc.callback.ICameraStateCallBack
import com.superkvm.ausbc.camera.bean.CameraRequest
import com.superkvm.ausbc.render.env.RotateType
import com.superkvm.ausbc.utils.*
import com.superkvm.ausbc.utils.bus.BusKey
import com.superkvm.ausbc.utils.bus.EventBus
import com.superkvm.ausbc.widget.*
import com.superkvm.kvm.databinding.FragmentDemoBinding
import com.superkvm.kvm.hid.KeyboardHidMapper
import com.superkvm.kvm.hid.KvmHidBridge
import com.superkvm.kvm.serial.SkUartDeviceHelper
import com.superkvm.kvm.serial.SkUsbSerialTransport
import com.superkvm.kvm.serial.UartPreferences
import com.superkvm.kvm.touch.KvmTouchController


class MainFragment : CameraFragment() {
    private lateinit var mViewBinding: FragmentDemoBinding
    private lateinit var skTransport: SkUsbSerialTransport
    private lateinit var kvmBridge: KvmHidBridge
    private var kvmTouch: KvmTouchController? = null
    private var kvmPreviewTextureView: TextureView? = null

    private var overlayFps: Int? = null
    private var directPreviewFrameCount = 0
    private var directPreviewFpsWindowStart = 0L
    private var selectedMenuType = DeviceMenuType.VIDEO

    private enum class SideKeyboardMode { QWERTY, SYMBOL }
    private var sideKeyboardMode = SideKeyboardMode.QWERTY
    private var sideKeyboardExpanded = false
    private var sideKeyboardPanel: View? = null
    private var keyboardKeysContainer: LinearLayout? = null
    private var sideKeyboardShift = false
    private var keyboardCardView: View? = null

    
    private var longPressMenuOverlay: FrameLayout? = null
    private var longPressMenuCard: View? = null
    
    private var lastLongPressMenuTriggerX4095: Int = 0
    private var lastLongPressMenuTriggerY4095: Int = 0
    
    private var forwardingOutsideMenuToTexture: Boolean = false

    private var toneGenerator: ToneGenerator? = null
    private var mouseJitterEnabled: Boolean = false

    override fun initView() {
        skTransport = SkUsbSerialTransport(requireContext().applicationContext)
        kvmBridge = KvmHidBridge(skTransport)
        mouseJitterEnabled = readMouseJitterEnabled()
        super.initView()
        setupDeviceMenu()
        initSideKeyboardPanelIfNeeded()
        initLongPressMenuOverlayIfNeeded()
        setupKeyboardToggle()
        if (DIRECT_UVC_TEXTURE_PREVIEW) {
            attachDirectPreviewFpsMeter()
        }
        attachKvmTouchToPreview()
    }

    override fun initData() {
        super.initData()
        if (!DIRECT_UVC_TEXTURE_PREVIEW) {
            EventBus.with<Int>(BusKey.KEY_FRAME_RATE).observe(this, { fps ->
                overlayFps = fps
                refreshInfoWatermark()
            })
        }
    }

    override fun onDestroyView() {
        if (::skTransport.isInitialized) {
            skTransport.shutdown()
        }
        kvmTouch = null
        kvmPreviewTextureView = null
        try {
            toneGenerator?.release()
        } catch (_: Exception) {
        }
        toneGenerator = null
        super.onDestroyView()
    }

    override fun onCameraState(
        self: MultiCameraClient.ICamera,
        code: ICameraStateCallBack.State,
        msg: String?
    ) {
        when (code) {
            ICameraStateCallBack.State.OPENED -> handleCameraOpened()
            ICameraStateCallBack.State.CLOSED -> handleCameraClosed()
            ICameraStateCallBack.State.ERROR -> handleCameraError(msg)
        }
    }

    private fun handleCameraError(msg: String?) {
        resetDirectPreviewFpsMeter()
        overlayFps = null
        mViewBinding.connectTipTv.visibility = View.VISIBLE
        mViewBinding.frameRateTv.visibility = View.GONE
        skTransport.close()
        ToastUtils.show("camera opened error: $msg")
    }

    private fun handleCameraClosed() {
        resetDirectPreviewFpsMeter()
        overlayFps = null
        mViewBinding.connectTipTv.visibility = View.VISIBLE
        mViewBinding.frameRateTv.visibility = View.GONE
        skTransport.close()
        kvmBridge.resetKeyboard()
        ToastUtils.show("camera closed success")
    }

    private fun handleCameraOpened() {
        resetDirectPreviewFpsMeter()
        mViewBinding.connectTipTv.visibility = View.GONE
        mViewBinding.frameRateTv.visibility = View.VISIBLE
        refreshInfoWatermark()
        attachKvmTouchToPreview()
        tryConnectSkUart()
        kvmBridge.setMouseAutoMoveEnabled(mouseJitterEnabled)
        ToastUtils.show("camera opened success")
    }

    private fun tryConnectSkUart() {
        if (!::skTransport.isInitialized) return
        val preferred = UartPreferences.getPreferredKey(requireContext())
        val dev = SkUartDeviceHelper.pickDevice(requireContext(), preferred) ?: return
        skTransport.requestOpen(dev) { ok ->
            activity?.runOnUiThread {
                if (ok) {
                    ToastUtils.show("SK UART OK @ 2M")
                } else {
                    ToastUtils.show("SK UART failed")
                }
            }
        }
    }

    private fun attachKvmTouchToPreview() {
        mViewBinding.cameraViewContainer.post {
            val tv = mViewBinding.cameraViewContainer.getChildAt(0) as? TextureView ?: return@post
            kvmPreviewTextureView = tv
            if (kvmTouch == null) {
                kvmTouch = KvmTouchController(
                    requireContext(),
                    tv,
                    kvmBridge,
                    videoSizeProvider = {
                    val sz = getCurrentPreviewSize()
                    (sz?.width ?: 1920) to (sz?.height ?: 1080)
                    },
                    onLongPressMenuRequested = { anchorX, anchorY, x4095, y4095 ->
                        showLongPressMenu(tv, anchorX, anchorY, x4095, y4095)
                    }
                )
            }
            tv.setOnTouchListener(kvmTouch)
        }
    }

    private fun setupKeyboardToggle() {
        mViewBinding.keyboardBtn.setOnClickListener {
            val panel = sideKeyboardPanel ?: return@setOnClickListener
            val showing = panel.visibility == View.VISIBLE
            if (showing) {
                sideKeyboardExpanded = false
                panel.visibility = View.GONE
            } else {
                sideKeyboardExpanded = true
                sideKeyboardShift = false
                sideKeyboardMode = SideKeyboardMode.QWERTY
                rebuildSideKeyboard()
                panel.visibility = View.VISIBLE
                keyboardCardView?.translationX = 0f
                keyboardCardView?.translationY = 0f
            }
        }
    }

    private fun initSideKeyboardPanelIfNeeded() {
        sideKeyboardPanel = mViewBinding.root.findViewById(R.id.sideKeyboardPanel)
        keyboardKeysContainer = mViewBinding.root.findViewById(R.id.keyboardKeysContainer)
        keyboardCardView = mViewBinding.root.findViewById(R.id.keyboardCard)

        
        sideKeyboardExpanded = false
        sideKeyboardShift = false
        sideKeyboardMode = SideKeyboardMode.QWERTY
        sideKeyboardPanel?.visibility = View.GONE
        mViewBinding.keyboardBtn.visibility = View.VISIBLE
        mViewBinding.cameraViewContainer.requestLayout()

        
        val card = keyboardCardView ?: return
        val overlay = sideKeyboardPanel ?: return
        val touchSlop = ViewConfiguration.get(requireContext()).scaledTouchSlop
        val dragThreshold = touchSlop * 2
        var downRawX = 0f
        var downRawY = 0f
        var startTranslationX = 0f
        var startTranslationY = 0f
        var dragging = false
        var tapCandidate = true

        card.setOnTouchListener { v, event ->
            val parentW = overlay.width.toFloat().coerceAtLeast(1f)
            val parentH = overlay.height.toFloat().coerceAtLeast(1f)
            val cardW = v.width.toFloat().coerceAtLeast(1f)
            val cardH = v.height.toFloat().coerceAtLeast(1f)
            val baseLeft = v.left.toFloat()
            val baseTop = v.top.toFloat()

            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    dragging = false
                    tapCandidate = true
                    downRawX = event.rawX
                    downRawY = event.rawY
                    startTranslationX = v.translationX
                    startTranslationY = v.translationY
                    return@setOnTouchListener true
                }
                MotionEvent.ACTION_MOVE -> {
                    val dx = event.rawX - downRawX
                    val dy = event.rawY - downRawY
                    if (tapCandidate && (dx * dx + dy * dy) > (dragThreshold * dragThreshold)) {
                        dragging = true
                        tapCandidate = false
                    }
                    if (dragging) {
                        
                        val minTx = -baseLeft
                        val maxTx = parentW - baseLeft - cardW
                        val minTy = -baseTop
                        val maxTy = parentH - baseTop - cardH
                        val newX = (startTranslationX + dx).coerceIn(minTx, maxTx)
                        val newY = (startTranslationY + dy).coerceIn(minTy, maxTy)
                        v.translationX = newX
                        v.translationY = newY
                    }
                    return@setOnTouchListener true
                }
                MotionEvent.ACTION_UP -> {
                    if (tapCandidate) {
                        val keyText = findKeyTagAt(v as ViewGroup, event.x, event.y)
                        if (keyText != null) handleSideKey(keyText)
                    }
                    return@setOnTouchListener true
                }
                MotionEvent.ACTION_CANCEL -> return@setOnTouchListener true
                else -> return@setOnTouchListener true
            }
        }
    }

    private fun initLongPressMenuOverlayIfNeeded() {
        longPressMenuOverlay = mViewBinding.root.findViewById(R.id.longPressMenuOverlay)
        longPressMenuCard = mViewBinding.root.findViewById(R.id.longPressMenuCard)
        if (longPressMenuOverlay == null || longPressMenuCard == null) return

        val overlay = longPressMenuOverlay ?: return

        overlay.setOnTouchListener { ov, ev ->
            if (forwardingOutsideMenuToTexture) {
                forwardLongPressMenuTouchToPreview(ev)
                if (ev.actionMasked == MotionEvent.ACTION_UP ||
                    ev.actionMasked == MotionEvent.ACTION_CANCEL
                ) {
                    forwardingOutsideMenuToTexture = false
                    hideLongPressMenu()
                }
                return@setOnTouchListener true
            }
            if (ov.visibility != View.VISIBLE) return@setOnTouchListener false

            val card = longPressMenuCard
            if (card != null && card.visibility == View.VISIBLE) {
                val r = Rect()
                card.getHitRect(r)
                if (r.contains(ev.x.toInt(), ev.y.toInt())) {
                    return@setOnTouchListener false
                }
            }

            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    forwardingOutsideMenuToTexture = true
                    card?.visibility = View.INVISIBLE
                    forwardLongPressMenuTouchToPreview(ev)
                    return@setOnTouchListener true
                }
                else -> return@setOnTouchListener true
            }
        }
    }

    
    private fun bindLongPressMenuActions(triggerX4095: Int, triggerY4095: Int) {
        val x = triggerX4095.coerceIn(0, 4095)
        val y = triggerY4095.coerceIn(0, 4095)
        lastLongPressMenuTriggerX4095 = x
        lastLongPressMenuTriggerY4095 = y
        fun bind(id: Int, action: () -> Unit) {
            mViewBinding.root.findViewById<View>(id)?.setOnClickListener {
                hideLongPressMenu()
                action()
            }
        }
        
        bind(R.id.longPressMenuLeftSingle) { kvmBridge.leftClickAt(x, y) }
        bind(R.id.longPressMenuLeftDouble) { kvmBridge.leftDoubleClickBurstAt(x, y) }
        bind(R.id.longPressMenuRight) { kvmBridge.rightClickAt(x, y) }
        bind(R.id.longPressMenuMiddle) { kvmBridge.middleClickAt(x, y) }
    }

    private fun forwardLongPressMenuTouchToPreview(ev: MotionEvent) {
        val tv = kvmPreviewTextureView ?: return
        val listener = kvmTouch ?: return
        val locTv = IntArray(2)
        tv.getLocationOnScreen(locTv)
        val copy = MotionEvent.obtain(ev)
        copy.setLocation(ev.rawX - locTv[0], ev.rawY - locTv[1])
        listener.onTouch(tv, copy)
        copy.recycle()
    }

    private fun showLongPressMenu(
        targetView: TextureView,
        anchorXInTarget: Float,
        anchorYInTarget: Float,
        x4095: Int,
        y4095: Int
    ) {
        val overlay = longPressMenuOverlay
        val card = longPressMenuCard
        if (overlay == null || card == null) {
            Log.w("superkvm", "longPressMenu: overlay or card null")
            return
        }

        
        val triggerHidX = x4095.coerceIn(0, 4095)
        val triggerHidY = y4095.coerceIn(0, 4095)

        val root = mViewBinding.root
        val margin = dpToPx(10)

        fun layoutAndShow() {
            val overlayW = root.width
            val overlayH = root.height
            if (overlayW <= 0 || overlayH <= 0) {
                root.post { layoutAndShow() }
                return
            }

            val wSpec = View.MeasureSpec.makeMeasureSpec(
                (overlayW - 2 * margin).coerceAtLeast(1),
                View.MeasureSpec.AT_MOST
            )
            val hSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
            card.measure(wSpec, hSpec)
            val cardW = card.measuredWidth.coerceAtLeast(1)
            val cardH = card.measuredHeight.coerceAtLeast(1)

            val tvLoc = IntArray(2)
            val rootLoc = IntArray(2)
            targetView.getLocationInWindow(tvLoc)
            root.getLocationInWindow(rootLoc)

            val anchorX = tvLoc[0] + anchorXInTarget - rootLoc[0]
            val anchorY = tvLoc[1] + anchorYInTarget - rootLoc[1]

            val desiredX = if (anchorX < overlayW / 2f) {
                anchorX + margin
            } else {
                anchorX - cardW - margin
            }
            val desiredY = if (anchorY < overlayH / 2f) {
                anchorY + margin
            } else {
                anchorY - cardH - margin
            }

            val minX = margin.toFloat()
            val minY = margin.toFloat()
            val maxX = (overlayW - cardW - margin).coerceAtLeast(margin).toFloat()
            val maxY = (overlayH - cardH - margin).coerceAtLeast(margin).toFloat()

            card.x = desiredX.coerceIn(minX, maxX)
            card.y = desiredY.coerceIn(minY, maxY)

            card.visibility = View.VISIBLE
            forwardingOutsideMenuToTexture = false
            overlay.visibility = View.VISIBLE
            bindLongPressMenuActions(triggerHidX, triggerHidY)
            Log.d(
                "superkvm",
                "longPressMenu show root=${overlayW}x${overlayH} anchor=($anchorX,$anchorY) " +
                    "card=${cardW}x${cardH} at (${card.x},${card.y}) " +
                    "triggerHid=($triggerHidX,$triggerHidY)"
            )
        }

        
        overlay.visibility = View.INVISIBLE
        root.post { layoutAndShow() }
    }

    private fun hideLongPressMenu() {
        forwardingOutsideMenuToTexture = false
        longPressMenuCard?.visibility = View.VISIBLE
        longPressMenuOverlay?.visibility = View.GONE
    }

    private fun rebuildSideKeyboard() {
        val container = keyboardKeysContainer ?: return
        container.removeAllViews()
        if (sideKeyboardMode == SideKeyboardMode.QWERTY) {
            buildQwertyPage(container)
        } else {
            buildSymbolPage(container)
        }
    }

    private fun buildQwertyPage(container: LinearLayout) {
        
        val rows = listOf(
            listOf("1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "BS"),
            listOf("q", "w", "e", "r", "t", "y", "u", "i", "o", "p"),
            listOf("a", "s", "d", "f", "g", "h", "j", "k", "l", ";"),
            listOf("z", "x", "c", "v", "b", "n", "m", ",", ".", "/"),
            listOf("SHIFT", "SPACE", "LANG", "ENT")
        )
        buildRows(container, rows)
    }

    private fun buildSymbolPage(container: LinearLayout) {
        val rows = listOf(
            listOf("!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "BS"),
            listOf("-", "=", "[", "]", "\\", ";", "'", "`", "/", ""),
            listOf(",", ".", "?", ":", "\"", "_", "+", "{", "}", "|"),
            listOf("SHIFT", "SPACE", "LANG", "ENT")
        )
        buildRows(container, rows)
    }

    private fun buildRows(container: LinearLayout, rows: List<List<String>>) {
        val baseKeyHdp = 38
        val isLandscape = resources.configuration.orientation == android.content.res.Configuration.ORIENTATION_LANDSCAPE
        
        val dpKeyH = if (isLandscape) kotlin.math.round(baseKeyHdp * 1.3f).toInt() else baseKeyHdp
        val keyH = dpToPx(dpKeyH)
        rows.forEach { rowSpec ->
            val row = LinearLayout(requireContext()).apply {
                orientation = LinearLayout.HORIZONTAL
                layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                )
            }
            rowSpec.forEach { cell ->
                if (cell.isBlank()) {
                    val spacer = View(requireContext()).apply {
                        layoutParams = LinearLayout.LayoutParams(0, keyH, 1f)
                    }
                    row.addView(spacer)
                } else {
                    val key = createKeyView(cell, keyH)
                    row.addView(key)
                }
            }
            container.addView(row)
        }
    }

    private fun createKeyView(text: String, keyHpx: Int): View {
        val isShiftKey = text == "SHIFT"
        val isLetterKey = text.length == 1 && text[0] in 'a'..'z'

        val displayText = when (text) {
            "SP", "SPACE" -> "Space"
            "BS" -> "Del"
            "ENT" -> "Enter"
            "SHIFT" -> "Shift"
            "SYM" -> "SYM"
            "LANG" -> "Lang"
            else -> {
                
                if (isLetterKey && sideKeyboardShift) text.uppercase() else text
            }
        }

        if (!isShiftKey) {
            return TextView(requireContext()).apply {
                layoutParams = LinearLayout.LayoutParams(0, keyHpx, 1f)
                gravity = Gravity.CENTER
                
                setBackgroundColor(0x55FFFFFF)
                setTextColor(0xFF000000.toInt())
                textSize = 14f
                setPadding(4, 4, 4, 4)
                setLines(1)
                this.text = displayText
                tag = text
                isClickable = false
                isFocusable = false
            }
        }

        val container = FrameLayout(requireContext()).apply {
            layoutParams = LinearLayout.LayoutParams(0, keyHpx, 1f)
            
            tag = text
            
            setBackgroundColor(0x55FFFFFF)
        }

        val title = TextView(requireContext()).apply {
            layoutParams = FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            ).apply {
                gravity = Gravity.CENTER
            }
            gravity = Gravity.CENTER
            
            setBackgroundColor(0x00000000)
            setTextColor(0xFF000000.toInt())
            textSize = 14f
            setPadding(4, 4, 4, 4)
            setLines(1)
            this.text = displayText
            isClickable = false
            isFocusable = false
        }

        container.addView(title)

        if (sideKeyboardShift) {
            val dotSize = dpToPx(4)
            val dotPad = dpToPx(2)
            val dotColor = 0xFF00C853.toInt()
            val positions = listOf(
                Pair(dotPad, dotPad),
                Pair(dotPad + dpToPx(6), dotPad),
                Pair(dotPad, dotPad + dpToPx(6))
            )
            positions.forEach { (x, y) ->
                val dot = View(requireContext()).apply {
                    layoutParams = FrameLayout.LayoutParams(dotSize, dotSize).apply {
                        gravity = Gravity.TOP or Gravity.START
                        leftMargin = x
                        topMargin = y
                    }
                    setBackgroundColor(dotColor)
                }
                container.addView(dot)
            }
        }

        return container
    }

    private fun handleSideKey(key: String) {
        playKeySound(key)
        when (key) {
            "SYM" -> {
                sideKeyboardMode = if (sideKeyboardMode == SideKeyboardMode.QWERTY) SideKeyboardMode.SYMBOL else SideKeyboardMode.QWERTY
                rebuildSideKeyboard()
                mViewBinding.root.post { highlightKey("SYM") }
                return
            }
            "SHIFT" -> {
                sideKeyboardShift = !sideKeyboardShift
                rebuildSideKeyboard()
                mViewBinding.root.post { highlightKey("SHIFT") }
                return
            }
            "LANG" -> {
                
                sideKeyboardMode =
                    if (sideKeyboardMode == SideKeyboardMode.QWERTY) SideKeyboardMode.SYMBOL else SideKeyboardMode.QWERTY
                sideKeyboardShift = false
                rebuildSideKeyboard()
                mViewBinding.root.post { highlightKey("LANG") }
                return
            }
            "BS" -> {
                highlightKey("BS")
                kvmBridge.tapKey(0, 0x2a)
                return
            }
            "ENT" -> {
                highlightKey("ENT")
                kvmBridge.tapKey(0, 0x28)
                return
            }
            "SPACE" -> {
                highlightKey("SPACE")
                kvmBridge.tapKey(0, 0x2c)
                return
            }
        }
        if (key.length == 1) {
            highlightKey(key)
            val ch = key[0]
            val pressed = KeyboardHidMapper.charToHidWithShift(ch)
            if (pressed.hid != 0) {
                val shiftMask = if (pressed.needsShift || (sideKeyboardShift && ch.isLetter())) (1 shl 1) else 0 
                kvmBridge.tapKey(shiftMask, pressed.hid)
            }
        }
    }

    private fun highlightKey(keyTag: String) {
        val container = keyboardKeysContainer ?: return
        val keyView = findKeyViewByTag(container, keyTag) ?: return
        val defaultBg = 0x55FFFFFF.toInt()
        val pressedBg = 0xB300C853.toInt() 
        val defaultElevation = keyView.elevation
        val pressedElevation = dpToPx(5).toFloat()

        val defaultTextColor = 0xFF000000.toInt()
        val pressedTextColor = 0xFFFFFFFF.toInt()
        try {
            keyView.setBackgroundColor(pressedBg)
            keyView.elevation = pressedElevation
            setTextColorRecursive(keyView, pressedTextColor)
            keyView.animate().scaleX(0.96f).scaleY(0.96f).setDuration(60).start()
        } catch (_: Exception) {
        }
        keyView.postDelayed({
            try {
                keyView.setBackgroundColor(defaultBg)
                keyView.elevation = defaultElevation
                setTextColorRecursive(keyView, defaultTextColor)
                keyView.animate().scaleX(1f).scaleY(1f).setDuration(80).start()
            } catch (_: Exception) {
            }
        }, 180)
    }

    private fun findKeyViewByTag(root: ViewGroup, tag: String): View? {
        for (i in 0 until root.childCount) {
            val child = root.getChildAt(i)
            val t = child.tag as? String
            if (t == tag) return child
            val vg = child as? ViewGroup ?: continue
            val hit = findKeyViewByTag(vg, tag)
            if (hit != null) return hit
        }
        return null
    }

    private fun setTextColorRecursive(root: View, color: Int) {
        if (root is TextView) {
            root.setTextColor(color)
        } else if (root is ViewGroup) {
            for (i in 0 until root.childCount) {
                setTextColorRecursive(root.getChildAt(i), color)
            }
        }
    }

    private fun playKeySound(key: String) {
        try {
            if (toneGenerator == null) {
                
                toneGenerator = ToneGenerator(AudioManager.STREAM_MUSIC, 80)
            }
            val toneType = when (key) {
                "BS" -> ToneGenerator.TONE_PROP_BEEP2
                "ENT" -> ToneGenerator.TONE_PROP_ACK
                "SPACE" -> ToneGenerator.TONE_PROP_NACK
                else -> ToneGenerator.TONE_PROP_BEEP
            }
            toneGenerator?.startTone(toneType, 35)
        } catch (_: Exception) {
            
        }
    }

    private fun dpToPx(dp: Int): Int {
        return (dp * resources.displayMetrics.density).toInt()
    }

    private fun findKeyTagAt(root: ViewGroup, x: Float, y: Float): String? {
        
        for (i in 0 until root.childCount) {
            val child = root.getChildAt(i)
            val left = child.left.toFloat()
            val top = child.top.toFloat()
            val right = child.right.toFloat()
            val bottom = child.bottom.toFloat()
            if (x < left || x > right || y < top || y > bottom) continue

            val tag = child.tag as? String
            if (tag != null) return tag

            val vg = child as? ViewGroup ?: continue
            val hit = findKeyTagAt(vg, x - left, y - top)
            if (hit != null) return hit
        }
        return null
    }

    private fun refreshInfoWatermark() {
        if (!::mViewBinding.isInitialized) return
        val size = getCurrentPreviewSize()
        val resText = size?.let { "${it.width}x${it.height}" } ?: "--"
        val fpsPart = overlayFps?.let { "$it fps" } ?: "…"
        mViewBinding.frameRateTv.text = if (DIRECT_UVC_TEXTURE_PREVIEW) {
            "$resText · $fpsPart · Direct"
        } else {
            "$resText · $fpsPart"
        }
    }

    private fun resetDirectPreviewFpsMeter() {
        directPreviewFrameCount = 0
        directPreviewFpsWindowStart = 0L
    }

    private fun setupDeviceMenu() {
        mViewBinding.settingsBtn.setOnClickListener {
            val shouldShow = mViewBinding.deviceMenuPanel.visibility != View.VISIBLE
            if (shouldShow) {
                mViewBinding.deviceMenuPanel.visibility = View.VISIBLE
                mViewBinding.menuDismissOverlay.visibility = View.VISIBLE
                selectMenu(DeviceMenuType.VIDEO)
            } else {
                hideDeviceMenu()
            }
        }
        mViewBinding.menuDismissOverlay.setOnClickListener { hideDeviceMenu() }
        mViewBinding.menuVideoDeviceTv.setOnClickListener { selectMenu(DeviceMenuType.VIDEO) }
        mViewBinding.menuUartTv.setOnClickListener { selectMenu(DeviceMenuType.UART) }
        mViewBinding.menuPasteTv.setOnClickListener {
            clearMenuSelection()
            handlePasteClipboard()
        }
        mViewBinding.menuMouseJitterTv.setOnClickListener { selectMenu(DeviceMenuType.MOUSE_JITTER) }
        mViewBinding.menuOperationGuideTv.setOnClickListener {
            clearMenuSelection()
            hideDeviceMenu()
            openOperationWiki()
        }
        mViewBinding.menuAboutTv.setOnClickListener {
            clearMenuSelection()
            hideDeviceMenu()
            showAboutDialog()
        }
    }

    private fun hideDeviceMenu() {
        mViewBinding.deviceMenuPanel.visibility = View.GONE
        mViewBinding.menuDismissOverlay.visibility = View.GONE
    }

    private fun selectMenu(type: DeviceMenuType) {
        selectedMenuType = type
        val selectedColor = 0xFF2196F3.toInt()
        val normalColor = 0xFF000000.toInt()
        mViewBinding.menuVideoDeviceTv.setTextColor(if (type == DeviceMenuType.VIDEO) selectedColor else normalColor)
        mViewBinding.menuUartTv.setTextColor(if (type == DeviceMenuType.UART) selectedColor else normalColor)
        mViewBinding.menuPasteTv.setTextColor(normalColor)
        mViewBinding.menuMouseJitterTv.setTextColor(if (type == DeviceMenuType.MOUSE_JITTER) selectedColor else normalColor)
        mViewBinding.menuOperationGuideTv.setTextColor(normalColor)
        mViewBinding.menuLevelTwoContainer.translationY =
            when (type) {
                
                DeviceMenuType.VIDEO -> dp(57 * 2).toFloat()
                DeviceMenuType.UART -> dp(57 * 3).toFloat()
                DeviceMenuType.MOUSE_JITTER -> dp(57).toFloat()
            }
        when (type) {
            DeviceMenuType.VIDEO -> {
                val items = getVideoDeviceNames()
                renderVideoDeviceList(items, "No video device")
            }
            DeviceMenuType.UART -> renderUartDeviceList()
            DeviceMenuType.MOUSE_JITTER -> renderMouseJitterToggleList()
        }
    }

    private fun renderMouseJitterToggleList() {
        val container = mViewBinding.deviceListContainer
        container.removeAllViews()
        container.addView(
            makeSecondaryRow("On${if (mouseJitterEnabled) "  ✓" else ""}") {
                setMouseJitterEnabled(true)
                renderMouseJitterToggleList()
            }
        )
        container.addView(
            makeSecondaryRow("Off${if (!mouseJitterEnabled) "  ✓" else ""}") {
                setMouseJitterEnabled(false)
                renderMouseJitterToggleList()
            }
        )
    }

    private fun clearMenuSelection() {
        val normalColor = 0xFF000000.toInt()
        mViewBinding.menuVideoDeviceTv.setTextColor(normalColor)
        mViewBinding.menuUartTv.setTextColor(normalColor)
        mViewBinding.menuPasteTv.setTextColor(normalColor)
        mViewBinding.menuMouseJitterTv.setTextColor(normalColor)
        mViewBinding.menuOperationGuideTv.setTextColor(normalColor)
        mViewBinding.menuAboutTv.setTextColor(normalColor)
    }

    private fun handlePasteClipboard() {
        val cm = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as? android.content.ClipboardManager
        val text = cm?.primaryClip?.getItemAt(0)?.coerceToText(requireContext())?.toString().orEmpty()
        if (text.isBlank()) {
            ToastUtils.show("Clipboard empty")
            return
        }
        hideDeviceMenu()
        kvmBridge.pasteText(text, 5L)
        ToastUtils.show("Pasted ${text.length} chars")
    }

    private fun openOperationWiki() {
        val uri = Uri.parse("https://wiki.superkvm.com/software/androidclient")
        val intent = Intent(Intent.ACTION_VIEW, uri)
        runCatching { startActivity(intent) }
            .onFailure { ToastUtils.show("Open link failed") }
    }

    private fun showAboutDialog() {
        val versionText =
            "Version: v${BuildConfig.APP_VERSION_FROM_FILE}-${BuildConfig.GIT_SHORT4}-${BuildConfig.BUILD_DATETIME}"
        AlertDialog.Builder(requireContext())
            .setTitle("About")
            .setMessage(
                "Source: https://github.com/superkvm/sk-client-android\n" +
                    "Support: support@superkvm.com\n" +
                    versionText
            )
            .setPositiveButton("OK", null)
            .show()
    }

    private fun setMouseJitterEnabled(enabled: Boolean) {
        mouseJitterEnabled = enabled
        requireContext().getSharedPreferences(PREF_NAME_SETTING, Context.MODE_PRIVATE)
            .edit()
            .putBoolean(KEY_MOUSE_JITTER_ENABLED, enabled)
            .apply()
        kvmBridge.setMouseAutoMoveEnabled(enabled)
    }

    private fun readMouseJitterEnabled(): Boolean {
        return requireContext().getSharedPreferences(PREF_NAME_SETTING, Context.MODE_PRIVATE)
            .getBoolean(KEY_MOUSE_JITTER_ENABLED, false)
    }

    private fun renderVideoDeviceList(items: List<String>, emptyText: String) {
        val container = mViewBinding.deviceListContainer
        container.removeAllViews()
        val target = if (items.isEmpty()) listOf(emptyText) else items
        target.forEach { text ->
            container.addView(makeSecondaryRow(text, null))
        }
    }

    private fun renderUartDeviceList() {
        val container = mViewBinding.deviceListContainer
        container.removeAllViews()
        val devices = SkUartDeviceHelper.listSkUartCandidates(requireContext())
        if (devices.isEmpty()) {
            container.addView(makeSecondaryRow("No uart device", null))
            return
        }
        devices.forEach { dev ->
            val name = SkUartDeviceHelper.buildDisplayName(dev)
            container.addView(
                makeSecondaryRow(name) {
                    UartPreferences.setPreferredKey(requireContext(), dev)
                    hideDeviceMenu()
                    tryConnectSkUart()
                }
            )
        }
    }

    private fun makeSecondaryRow(text: String, onClick: (() -> Unit)?): View {
        val row = TextView(requireContext()).apply {
            val rowH = dp(SECONDARY_MENU_ROW_HEIGHT_DP)
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                rowH
            )
            minHeight = rowH
            gravity = Gravity.CENTER
            setTextColor(0xFF000000.toInt())
            textSize = 12f
            setTypeface(typeface, android.graphics.Typeface.NORMAL)
            this.text = text
            setBackgroundColor(0xCCBDBDBD.toInt())
            if (onClick != null) {
                isClickable = true
                setOnClickListener { onClick() }
            }
        }
        val wrap = android.widget.LinearLayout(requireContext()).apply {
            orientation = android.widget.LinearLayout.VERTICAL
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            )
            addView(row)
            addView(View(requireContext()).apply {
                layoutParams = ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(1)
                )
                setBackgroundColor(0x55FFFFFF)
            })
        }
        return wrap
    }

    private fun getVideoDeviceNames(): List<String> {
        return getUsbDevices()
            .map { it to buildDeviceDisplayName(it) }
            .filter { (_, name) ->
                val lower = name.lowercase()
                lower.contains("usb") && lower.contains("video")
            }
            .filter { (device, _) -> isVideoDevice(device) }
            .map { (_, name) -> name }
            .distinct()
    }

    private fun getUsbDevices(): List<UsbDevice> {
        val manager = requireContext().getSystemService(Context.USB_SERVICE) as? UsbManager ?: return emptyList()
        return manager.deviceList.values.sortedBy { it.deviceName ?: "" }
    }

    private fun isVideoDevice(device: UsbDevice): Boolean {
        if (device.deviceClass == UsbConstants.USB_CLASS_VIDEO) return true
        for (i in 0 until device.interfaceCount) {
            if (device.getInterface(i).interfaceClass == UsbConstants.USB_CLASS_VIDEO) {
                return true
            }
        }
        return false
    }

    private fun buildDeviceDisplayName(device: UsbDevice): String {
        val product = device.productName?.takeIf { it.isNotBlank() }
        val manufacturer = device.manufacturerName?.takeIf { it.isNotBlank() }
        val deviceName = device.deviceName.takeIf { it.isNotBlank() } ?: "unknown"
        return when {
            manufacturer != null && product != null -> "$manufacturer $product"
            product != null -> product
            manufacturer != null -> manufacturer
            else -> deviceName
        }
    }

    private fun dp(value: Int): Int {
        return (value * resources.displayMetrics.density).toInt()
    }

    private fun attachDirectPreviewFpsMeter() {
        val tv = mViewBinding.cameraViewContainer.getChildAt(0) as? TextureView ?: return
        val base = tv.surfaceTextureListener ?: return
        tv.surfaceTextureListener = object : TextureView.SurfaceTextureListener {
            override fun onSurfaceTextureAvailable(surface: SurfaceTexture?, width: Int, height: Int) {
                base.onSurfaceTextureAvailable(surface, width, height)
            }

            override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture?, width: Int, height: Int) {
                base.onSurfaceTextureSizeChanged(surface, width, height)
            }

            override fun onSurfaceTextureDestroyed(surface: SurfaceTexture?): Boolean {
                return base.onSurfaceTextureDestroyed(surface)
            }

            override fun onSurfaceTextureUpdated(surface: SurfaceTexture?) {
                base.onSurfaceTextureUpdated(surface)
                if (!DIRECT_UVC_TEXTURE_PREVIEW) return
                directPreviewFrameCount++
                val now = SystemClock.elapsedRealtime()
                if (directPreviewFpsWindowStart == 0L) {
                    directPreviewFpsWindowStart = now
                }
                if (now - directPreviewFpsWindowStart >= 1000L) {
                    overlayFps = directPreviewFrameCount
                    directPreviewFrameCount = 0
                    directPreviewFpsWindowStart = now
                    refreshInfoWatermark()
                }
            }
        }
    }

    override fun getCameraView(): IAspectRatio {
        return AspectRatioTextureView(requireContext())
    }

    override fun getCameraViewContainer(): ViewGroup {
        return mViewBinding.cameraViewContainer
    }

    override fun getRootView(inflater: LayoutInflater, container: ViewGroup?): View {
        mViewBinding = FragmentDemoBinding.inflate(inflater, container, false)
        return mViewBinding.root
    }

    override fun getGravity(): Int = Gravity.CENTER

    override fun getCameraRequest(): CameraRequest {
        return CameraRequest.Builder()
            .setPreviewWidth(1920)
            .setPreviewHeight(1080)
            .setRenderMode(
                if (DIRECT_UVC_TEXTURE_PREVIEW) CameraRequest.RenderMode.NORMAL
                else CameraRequest.RenderMode.OPENGL
            )
            .setDefaultRotateType(RotateType.ANGLE_0)
            .setAudioSource(CameraRequest.AudioSource.SOURCE_SYS_MIC)
            .setPreviewFormat(CameraRequest.PreviewFormat.FORMAT_MJPEG)
            .setAspectRatioShow(true)
            .setCaptureRawImage(false)
            .setRawPreviewData(false)
            .create()
    }

    companion object {
        private const val DIRECT_UVC_TEXTURE_PREVIEW = true
        private const val SECONDARY_MENU_ROW_HEIGHT_DP = 28
        private const val PREF_NAME_SETTING = "sk_kvm_setting"
        private const val KEY_MOUSE_JITTER_ENABLED = "mouse_jitter_enabled"
    }

    private enum class DeviceMenuType {
        VIDEO, UART, MOUSE_JITTER
    }
}
