/*
||
|| @file Keypad_Ted.h
|| @version 1.0
|| @author Ted Toal from code by Mark Stanley, Alexander Brevig
|| @contact ted@tedtoal.net
||
|| @description
|| | This library provides a simple interface for using matrix keypads.
|| | It supports single keypresses, there is no key buffering.
|| | It also supports user-definable pins and keymaps.
|| | It was created by simplifying the Keypad library of Stanley and Brevig.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/

#ifndef KEYPAD_TED_H
#define KEYPAD_TED_H

// Arduino versioning.
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define OPEN LOW
#define CLOSED HIGH

typedef enum { IDLE = 0, PRESSED = 1, HOLD = 2, RELEASED = 3 } KeyState;
#define NO_KEY '\0'

class Key {
    private:
        static const char* KeyStateStrings[4];

    public:

    char kchar;
    KeyState kstate;
    boolean haveNewState;

    Key() {
        kchar = NO_KEY;
        kstate = IDLE;
        haveNewState = false;
    }

    static const char* getKeyStateStr(KeyState state) { return KeyStateStrings[state]; }
};

// bperrybap - Thanks for a well reasoned argument and the following macro(s).
// See http://arduino.cc/forum/index.php/topic,142041.msg1069480.html#msg1069480
#ifndef INPUT_PULLUP
#warning "Using  pinMode() INPUT_PULLUP AVR emulation"
#define INPUT_PULLUP 0x2
#define pinMode(_pin, _mode) _mypinMode(_pin, _mode)
#define _mypinMode(_pin, _mode)  \
do {                             \
    if(_mode == INPUT_PULLUP)    \
        pinMode(_pin, INPUT);    \
        digitalWrite(_pin, 1);   \
    if(_mode != INPUT_PULLUP)    \
        pinMode(_pin, _mode);    \
}while(0)
#endif

// Made changes according to this post http://arduino.cc/forum/index.php?topic=58337.0
// by Nick Gammon. Thanks for the input Nick. It actually saved 78 bytes for me. :)
typedef struct {
    byte rows;
    byte columns;
} KeypadSize;

#define MAX_NUM_ROWS 10
#define makeKeymap(x) ((char*)x)

class Keypad : public Key {
public:

    // Constructor for initializing the keypad.
    Keypad(char* userKeymap, byte* row, byte* col, byte numRows, byte numCols, uint16_t holdTimeMS=500);

    // Return the last key press that occurred, or NO_KEY. *state is set to the
    // current state of the returned key, or to IDLE if NO_KEY is returned:
    //  IDLE: no key is pressed
    //  PRESSED: pressed
    //  HOLD: pressed and held for at least the hold time
    //  RELEASED: pressed and released but not yet returned by getNextKey().
    // A key is returned ANY TIME ITS STATE CHANGES (PRESS, HOLD, RELEASE), and
    // subsequent calls won't return it again until its state changes again, so
    // the state will transition from PRESS to RELEASE or PRESS to HOLD to
    // RELEASE. If this function is called too infrequently, it is possible that
    // key presses will be missed.
    char getKey(KeyState& state);

    // Call getKey() repeatedly until it returns a key rather than NO_KEY.
    // Return that key and set *state to the key state, which will transition
    // from PRESSED to RELEASED or PRESSED to HOLD to RELEASED in multiple calls.
    char waitForKey(KeyState& state);

    // Scan keypad. This function must be called periodically at a rate that
    // gives good keypad response without key bounce, 10 ms is a good choice.
    // It is recommended that you call this function from a timer interrupt
    // routine with interrupts disabled.
    // One call to this scans the entire keypad, and saves the key state for
    // processing by updateKeyState(). A single call to this will result in a
    // key transition to the PRESSED state if it is detected as DOWN when it was
    // UP in the previous call. No debouncing is done.
    void scanKeys();

    // Update key state using data from last call to scanKeys(). This must be
    // called immediately after calling scanKeys().
    // This function does not act on a new key press until getKey() has returned
    // the RELEASED state of the previous key press, so you must call getKey()
    // often to avoid missing any keys.
    // If you call scanKeys() from a timer interrupt service routine, it is
    // recommended that, since this function might take more time than an
    // interrupt routine should consume with interrupts disabled, set a blocking
    // semaphore in the ISR, enable interrupts, call this, disable interrupts,
    // and clear the blocking semaphore, like this:
    //      bool updateKeyStateIsActive = false;
    //      ISR(...) {
    //        scanKeys();
    //        if (!updateKeyStateIsActive) {
    //          updateKeyStateIsActive = true;
    //          interrupts();
    //          updateKeyState();
    //          noInterrupts();
    //          updateKeyStateIsActive = false;
    //        }
    //     }
    void updateKeyState();

private:
    char* keymap;
    byte* rowPins;
    byte* columnPins;
    KeypadSize sizeKpd;
    uint16_t holdTime;
    uint32_t holdTimer;
    Key key;
    uint16_t bitMap[MAX_NUM_ROWS];
    virtual void pin_mode(byte pinNum, byte mode) { pinMode(pinNum, mode); }
    virtual void pin_write(byte pinNum, boolean level) { digitalWrite(pinNum, level); }
    virtual int  pin_read(byte pinNum) { return digitalRead(pinNum); }
};

#endif

/*
|| @changelog
|| | 1.0 2022-12-16 - Ted Toal         : Initial coding from Stanley and Brevig version 3.2.
|| #
*/
