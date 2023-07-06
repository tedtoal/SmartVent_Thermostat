/*
  fontsAndColors.h

  Define color shortcut symbols and pointers to fonts imported from the library.
  Several fonts are included, even though we may not use most of them. Unused
  ones can be removed them later if needed.
*/
#include <Arduino.h>
#include <Adafruit_GFX.h>

// Define the colors without the ILI9341_ prefix.
#define BLACK         ILI9341_BLACK
#define NAVY          ILI9341_NAVY
#define DARKGREEN     ILI9341_DARKGREEN
#define DARKCYAN      ILI9341_DARKCYAN
#define MAROON        ILI9341_MAROON
#define PURPLE        ILI9341_PURPLE
#define OLIVE         ILI9341_OLIVE
#define LIGHTGREY     ILI9341_LIGHTGREY
#define DARKGREY      ILI9341_DARKGREY
#define BLUE          ILI9341_BLUE
#define GREEN         ILI9341_GREEN
#define CYAN          ILI9341_CYAN
#define RED           ILI9341_RED
#define MAGENTA       ILI9341_MAGENTA
#define YELLOW        ILI9341_YELLOW
#define WHITE         ILI9341_WHITE
#define ORANGE        ILI9341_ORANGE
#define GREENYELLOW   ILI9341_GREENYELLOW
#define PINK          ILI9341_PINK
#define CLEAR         TRANSPARENT_COLOR

// Pointers to imported fonts.
extern const GFXfont* mono12B;
extern const GFXfont* font9;
extern const GFXfont* font12;
extern const GFXfont* font18;
extern const GFXfont* font24;
extern const GFXfont* font9B;
extern const GFXfont* font12B;
extern const GFXfont* font18B;
extern const GFXfont* font24B;
//extern const GFXfont* fontOrg;
//extern const GFXfont* fontPico;
//extern const GFXfont* fontTiny;
extern const GFXfont* fontTom;

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
