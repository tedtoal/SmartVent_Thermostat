/*
||
|| @file Keypad_Ted.cpp
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
#include <Keypad_Ted.h>

static const char* Key::KeyStateStrings[4] = { "IDLE", "PRESSED", "HOLD", "RELEASED" };

// Constructor. Allows custom keymap, pin configuration, and keypad sizes.
Keypad::Keypad(char* userKeymap, byte* row, byte* col, byte numRows, byte numCols, uint16_t holdTimeMS=500) {
    rowPins = row;
    columnPins = col;
    sizeKpd.rows = numRows;
    sizeKpd.columns = numCols;
    keymap = userKeymap;
    holdTime = holdTimeMS;
}

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
char Keypad::getKey(KeyState& state) {
    // Note that the following should be safe with regard to calling updateKeyState()
    // from an ISR. The only non-ISR variables accessed by scanKeys() and updateKeyState()
    // are "key".  key.haveNewState is a boolean variable using one byte, so accesses
    // to it are atomic. This doesn't touch "key" as long as key.haveNewState is false,
    // and updateKeyState() doesn't touch "key" as long as key.haveNewState is true.
    if (key.haveNewState) {
        state = key.kstate;
        char kchar = key.kchar;
        if (state == RELEASED)
            key.kstate = IDLE; // Return to IDLE state after RELEASED returned.
        key.haveNewState = false;
        return kchar;
    }
    return NO_KEY;
}

// Call getKey() repeatedly until it returns a key rather than NO_KEY.
// Return that key and set *state to the key state, which will transition
// from PRESSED to RELEASED or PRESSED to HOLD to RELEASED in multiple calls.
char Keypad::waitForKey(KeyState& state) {
    char waitKey;
    while( (waitKey = getKey(state)) == NO_KEY )
        ;    // Block everything while waiting for a keypress.
    return waitKey;
}


// Scan keypad.
void Keypad::scanKeys() {
    // Re-intialize the row pins. Allows sharing these pins with other hardware.
    for (byte r=0; r<sizeKpd.rows; r++)
        pin_mode(rowPins[r], INPUT_PULLUP);

    // bitMap stores ALL the keys that are being pressed.
    for (byte c=0; c<sizeKpd.columns; c++) {
        pin_mode(columnPins[c], OUTPUT);
        pin_write(columnPins[c], LOW);  // Begin column pulse output.
        for (byte r=0; r<sizeKpd.rows; r++)
            bitWrite(bitMap[r], c, !pin_read(rowPins[r]));  // keypress is active low so invert to high.
        // Set pin to high impedance input. Effectively ends column pulse.
        pin_write(columnPins[c], HIGH);
        pin_mode(columnPins[c], INPUT);
    }
}

// Update the key state.
void Keypad::updateKeyState() {
    // Ignore state changes of still-active key as long as haveNewState is true
    // (until getKey() is called to clear it).
    if (!key.haveNewState) {
    	for (byte r=0; r<sizeKpd.rows; r++) {
            for (byte c=0; c<sizeKpd.columns; c++) {
                boolean button = bitRead(bitMap[r],c);
                char keyChar = keymap[r * sizeKpd.columns + c];
                // If a previous key is still active, ignore any state change unless
                // it is for the same key.
                if (key.kstate == IDLE || key.kchar == keyChar) {
                    key.kchar = keyChar;
                    switch (key.kstate) {
                    case IDLE:
                        if (button == CLOSED) {
                            key.kstate = PRESSED;
                            key.haveNewState = true;
                            holdTimer = millis(); // Start timing possible HOLD state.
                        }
                        break;
                    case PRESSED:
                        if (button == OPEN) {
                            key.kstate = RELEASED;
                            key.haveNewState = true;
                        } else if ((millis()-holdTimer) > holdTime) { // Check for HOLD
                            key.kstate = HOLD;
                            key.haveNewState = true;
                        }
                        break;
                    case HOLD:
                        if (button == OPEN) {
                            key.kstate = RELEASED;
                            key.haveNewState = true;
                        }
                        break;
                    case RELEASED:
                        if (!key.haveNewState) { // Also done in getKey().
                            key.kchar = NO_KEY;
                            key.kstate = IDLE;
                            key.haveNewState = false;
                        }
                        break;
                    }
                }
            }
        }
    }
}

/*
|| @changelog
|| | 1.0 2022-12-16 - Ted Toal         : Initial coding from Stanley and Brevig version 3.2.
|| #
*/
