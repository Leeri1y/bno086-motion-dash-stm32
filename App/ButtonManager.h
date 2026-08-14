/*
 * ButtonManager.h
 * Button debounce + short/long press detection (EXTI driven).
 * The EXTI ISR calls onInterrupt(); short/long events are consumed by the
 * UI task. Same semantics as the original Arduino version.
 */
#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "main.h"
#include "Config.h"

class Button {
public:
    Button()
        : _port(0), _pin(0), _pressStart(0), _lastEdge(0),
          _isPressed(false), _pendingShort(false), _pendingLong(false) {}

    void begin(GPIO_TypeDef *port, uint16_t pin) {
        _port = port;
        _pin = pin;
    }

    /* Call from the EXTI ISR (lightweight: timestamp + debounce only). */
    void onInterrupt() {
        uint32_t now = HAL_GetTick();
        bool pressed = (HAL_GPIO_ReadPin(_port, _pin) == GPIO_PIN_RESET); /* INPUT_PULLUP */

        if (now - _lastEdge < BTN_DEBOUNCE_MS) {
            _lastEdge = now;
            return; /* inside debounce window: ignore bounce */
        }
        _lastEdge = now;

        if (pressed) {
            _pressStart = now;
            _isPressed = true;
        } else {
            /* release without matching press = noise, discard */
            if (!_isPressed) return;
            _isPressed = false;
            uint32_t duration = now - _pressStart;
            if (duration >= BTN_LONGPRESS_MS) _pendingLong = true;
            else _pendingShort = true;
        }
    }

    bool consumeShort() {
        if (!_pendingShort) return false;
        _pendingShort = false;
        return true;
    }

    bool consumeLong() {
        if (!_pendingLong) return false;
        _pendingLong = false;
        return true;
    }

    void clearPending() {
        _pendingShort = false;
        _pendingLong = false;
    }

private:
    GPIO_TypeDef *_port;
    uint16_t _pin;
    volatile uint32_t _pressStart;
    volatile uint32_t _lastEdge;
    volatile bool _isPressed;
    volatile bool _pendingShort;
    volatile bool _pendingLong;
};

#endif
