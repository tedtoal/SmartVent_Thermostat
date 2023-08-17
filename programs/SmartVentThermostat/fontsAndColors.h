/*
  fontsAndColors.h

  Define color shortcut symbols.

  Define Font_TT objects pointing to fonts imported from the GFX font library.
  Several fonts are included, even though we may not use most of them. Unused
  ones can be removed them later if needed.
*/
#include <Arduino.h>
#include <Font_TT.h>

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

// Pointers to font objects for imported fonts.
extern Font_TT mono12B;
extern Font_TT font9;
extern Font_TT font12;
extern Font_TT font18;
extern Font_TT font24;
extern Font_TT font9B;
extern Font_TT font12B;
extern Font_TT font18B;
extern Font_TT font24B;
//extern Font_TT fontOrg;
//extern Font_TT fontPico;
//extern Font_TT fontTiny;
extern Font_TT fontTom;

/////////////////////////////////////////////////////////////////////////////////////////////
// End.
/////////////////////////////////////////////////////////////////////////////////////////////
