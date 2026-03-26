
package com.superkvm.kvm.hid

import android.view.KeyEvent

object KeyboardHidMapper {
    data class HidShift(val hid: Int, val needsShift: Boolean)

    fun keyCodeToHid(keyCode: Int): Int {
        return when (keyCode) {
            KeyEvent.KEYCODE_A -> 0x04
            KeyEvent.KEYCODE_B -> 0x05
            KeyEvent.KEYCODE_C -> 0x06
            KeyEvent.KEYCODE_D -> 0x07
            KeyEvent.KEYCODE_E -> 0x08
            KeyEvent.KEYCODE_F -> 0x09
            KeyEvent.KEYCODE_G -> 0x0a
            KeyEvent.KEYCODE_H -> 0x0b
            KeyEvent.KEYCODE_I -> 0x0c
            KeyEvent.KEYCODE_J -> 0x0d
            KeyEvent.KEYCODE_K -> 0x0e
            KeyEvent.KEYCODE_L -> 0x0f
            KeyEvent.KEYCODE_M -> 0x10
            KeyEvent.KEYCODE_N -> 0x11
            KeyEvent.KEYCODE_O -> 0x12
            KeyEvent.KEYCODE_P -> 0x13
            KeyEvent.KEYCODE_Q -> 0x14
            KeyEvent.KEYCODE_R -> 0x15
            KeyEvent.KEYCODE_S -> 0x16
            KeyEvent.KEYCODE_T -> 0x17
            KeyEvent.KEYCODE_U -> 0x18
            KeyEvent.KEYCODE_V -> 0x19
            KeyEvent.KEYCODE_W -> 0x1a
            KeyEvent.KEYCODE_X -> 0x1b
            KeyEvent.KEYCODE_Y -> 0x1c
            KeyEvent.KEYCODE_Z -> 0x1d
            KeyEvent.KEYCODE_1 -> 0x1e
            KeyEvent.KEYCODE_2 -> 0x1f
            KeyEvent.KEYCODE_3 -> 0x20
            KeyEvent.KEYCODE_4 -> 0x21
            KeyEvent.KEYCODE_5 -> 0x22
            KeyEvent.KEYCODE_6 -> 0x23
            KeyEvent.KEYCODE_7 -> 0x24
            KeyEvent.KEYCODE_8 -> 0x25
            KeyEvent.KEYCODE_9 -> 0x26
            KeyEvent.KEYCODE_0 -> 0x27
            KeyEvent.KEYCODE_ENTER -> 0x28
            KeyEvent.KEYCODE_ESCAPE -> 0x29
            KeyEvent.KEYCODE_DEL -> 0x2a
            KeyEvent.KEYCODE_TAB -> 0x2b
            KeyEvent.KEYCODE_SPACE -> 0x2c
            KeyEvent.KEYCODE_MINUS -> 0x2d
            KeyEvent.KEYCODE_EQUALS -> 0x2e
            KeyEvent.KEYCODE_LEFT_BRACKET -> 0x2f
            KeyEvent.KEYCODE_RIGHT_BRACKET -> 0x30
            KeyEvent.KEYCODE_BACKSLASH -> 0x31
            KeyEvent.KEYCODE_SEMICOLON -> 0x33
            KeyEvent.KEYCODE_APOSTROPHE -> 0x34
            KeyEvent.KEYCODE_GRAVE -> 0x35
            KeyEvent.KEYCODE_COMMA -> 0x36
            KeyEvent.KEYCODE_PERIOD -> 0x37
            KeyEvent.KEYCODE_SLASH -> 0x38
            KeyEvent.KEYCODE_CAPS_LOCK -> 0x39
            KeyEvent.KEYCODE_F1 -> 0x3a
            KeyEvent.KEYCODE_F2 -> 0x3b
            KeyEvent.KEYCODE_F3 -> 0x3c
            KeyEvent.KEYCODE_F4 -> 0x3d
            KeyEvent.KEYCODE_F5 -> 0x3e
            KeyEvent.KEYCODE_F6 -> 0x3f
            KeyEvent.KEYCODE_F7 -> 0x40
            KeyEvent.KEYCODE_F8 -> 0x41
            KeyEvent.KEYCODE_F9 -> 0x42
            KeyEvent.KEYCODE_F10 -> 0x43
            KeyEvent.KEYCODE_F11 -> 0x44
            KeyEvent.KEYCODE_F12 -> 0x45
            KeyEvent.KEYCODE_DPAD_RIGHT -> 0x4f
            KeyEvent.KEYCODE_DPAD_LEFT -> 0x50
            KeyEvent.KEYCODE_DPAD_DOWN -> 0x51
            KeyEvent.KEYCODE_DPAD_UP -> 0x52
            else -> 0x00
        }
    }

    fun keyCodeToModifierBit(keyCode: Int): Int? {
        return when (keyCode) {
            KeyEvent.KEYCODE_CTRL_LEFT -> 0
            KeyEvent.KEYCODE_SHIFT_LEFT -> 1
            KeyEvent.KEYCODE_ALT_LEFT -> 2
            KeyEvent.KEYCODE_META_LEFT -> 3
            KeyEvent.KEYCODE_CTRL_RIGHT -> 4
            KeyEvent.KEYCODE_SHIFT_RIGHT -> 5
            KeyEvent.KEYCODE_ALT_RIGHT -> 6
            KeyEvent.KEYCODE_META_RIGHT -> 7
            else -> null
        }
    }

    
    fun charToHid(c: Char): Int {
        return when (c) {
            in 'a'..'z' -> 0x04 + (c - 'a')
            in 'A'..'Z' -> 0x04 + (c.lowercaseChar() - 'a')
            in '1'..'9' -> 0x1e + (c - '1')
            '0' -> 0x27
            ' ' -> 0x2c
            '\n' -> 0x28
            '\t' -> 0x2b
            '-' -> 0x2d
            '=' -> 0x2e
            '[' -> 0x2f
            ']' -> 0x30
            '\\' -> 0x31
            ';' -> 0x33
            '\'' -> 0x34
            '`' -> 0x35
            ',' -> 0x36
            '.' -> 0x37
            '/' -> 0x38
            else -> 0x00
        }
    }

    
    fun charToHidWithShift(c: Char): HidShift {
        return when (c) {
            in 'a'..'z' -> HidShift(0x04 + (c - 'a'), false)
            in 'A'..'Z' -> HidShift(0x04 + (c.lowercaseChar() - 'a'), true)
            in '1'..'9' -> HidShift(0x1e + (c - '1'), false)
            '0' -> HidShift(0x27, false)

            
            ' ' -> HidShift(0x2c, false)
            '\n' -> HidShift(0x28, false)
            '\t' -> HidShift(0x2b, false)

            
            '-' -> HidShift(0x2d, false)
            '=' -> HidShift(0x2e, false)
            '[' -> HidShift(0x2f, false)
            ']' -> HidShift(0x30, false)
            '\\' -> HidShift(0x31, false)
            ';' -> HidShift(0x33, false)
            '\'' -> HidShift(0x34, false)
            '`' -> HidShift(0x35, false)
            ',' -> HidShift(0x36, false)
            '.' -> HidShift(0x37, false)
            '/' -> HidShift(0x38, false)

            
            '!' -> HidShift(0x1e, true)
            '@' -> HidShift(0x1f, true)
            '#' -> HidShift(0x20, true)
            '$' -> HidShift(0x21, true)
            '%' -> HidShift(0x22, true)
            '^' -> HidShift(0x23, true)
            '&' -> HidShift(0x24, true)
            '*' -> HidShift(0x25, true)
            '(' -> HidShift(0x26, true)
            ')' -> HidShift(0x27, true)
            '?' -> HidShift(0x38, true)
            ':' -> HidShift(0x33, true)
            '"' -> HidShift(0x34, true)
            '_' -> HidShift(0x2d, true)
            '+' -> HidShift(0x2e, true)
            '{' -> HidShift(0x2f, true)
            '}' -> HidShift(0x30, true)
            '|' -> HidShift(0x31, true)
            '~' -> HidShift(0x35, true)
            '<' -> HidShift(0x36, true)
            '>' -> HidShift(0x37, true)

            else -> HidShift(0x00, false)
        }
    }
}
