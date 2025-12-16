#pragma once


// ============================================================================
// ENUMS AND DEFINES
// ============================================================================

// Item types
#define ITEM_TYPE_TEXT              0   // Simple text
#define ITEM_TYPE_BUTTON            1   // Button control
#define ITEM_TYPE_RADIOBUTTON       2   // Radio button
#define ITEM_TYPE_CHECKBOX          3   // Check box
#define ITEM_TYPE_EDITFIELD         4   // Editable text field
#define ITEM_TYPE_COMBO             5   // Combo box (not used much)
#define ITEM_TYPE_LISTBOX           6   // List box
#define ITEM_TYPE_MODEL             7   // 3D model
#define ITEM_TYPE_OWNERDRAW         8   // Owner draw
#define ITEM_TYPE_NUMERICFIELD      9   // Numeric field
#define ITEM_TYPE_SLIDER            10  // Slider
#define ITEM_TYPE_YESNO             11  // Yes/No button
#define ITEM_TYPE_MULTI             12  // Multi-value
#define ITEM_TYPE_BIND              13  // Key bind

// Alignment
#define ITEM_ALIGN_LEFT             0
#define ITEM_ALIGN_CENTER           1
#define ITEM_ALIGN_RIGHT            2

// Window styles
#define WINDOW_STYLE_EMPTY          0
#define WINDOW_STYLE_FILLED         1
#define WINDOW_STYLE_GRADIENT       2
#define WINDOW_STYLE_SHADER         3
#define WINDOW_STYLE_TEAMCOLOR      4
#define WINDOW_STYLE_CINEMATIC      5

// Window flags
#define WINDOW_VISIBLE              0x00000001
#define WINDOW_BORDER               0x00000002
#define WINDOW_HORIZONTAL           0x00000004
#define WINDOW_AUTOWRAPPED          0x00000008
#define WINDOW_POPUP                0x00000010
#define WINDOW_FADEIN               0x00000020
#define WINDOW_FADEOUT              0x00000040
#define WINDOW_MODAL                0x00000080
#define WINDOW_HASFOCUS             0x00000100
#define WINDOW_DECORATION           0x00000200
#define WINDOW_MOUSEOVER            0x00000400
#define WINDOW_INTRANSITION         0x00000800
#define WINDOW_FORCED               0x00001000

// Text styles
#define ITEM_TEXTSTYLE_NORMAL       0
#define ITEM_TEXTSTYLE_BLINK        1
#define ITEM_TEXTSTYLE_PULSE        2
#define ITEM_TEXTSTYLE_SHADOWED     3
#define ITEM_TEXTSTYLE_OUTLINED     4
#define ITEM_TEXTSTYLE_OUTLINESHADOWED  5
#define ITEM_TEXTSTYLE_SHADOWEDMORE 6

// Common defines
#define MAX_LB_COLUMNS              16
#define MAX_MULTI_CVARS             32

// Vector type (if not already defined)
#ifndef vec4_t
typedef float vec4_t[4];
#endif

#ifndef vec3_t
typedef float vec3_t[3];
#endif

// Handle types (if not already defined)
#ifndef qhandle_t
typedef int qhandle_t;
#endif

#ifndef sfxHandle_t
typedef int sfxHandle_t;
#endif

// ============================================================================
// RECTANGLE
// ============================================================================
typedef struct {
    float x;
    float y;
    float w;
    float h;
} rectDef_t;

// ============================================================================
// WINDOW DEFINITION
// ============================================================================
typedef struct {
    rectDef_t   rect;                   // Rectangle
    rectDef_t   rectClient;             // Rectangle for client area
    const char* name;                  // Window name
    const char* group;                 // Group name
    int         style;                  // Window style
    int         border;                 // Border style
    int         ownerDraw;              // Owner draw type
    int         ownerDrawFlags;         // Owner draw flags
    float       borderSize;             // Border size
    int         flags;                  // Window flags
    rectDef_t   rectEffects;            // Effects rectangle
    rectDef_t   rectEffects2;           // Effects rectangle 2
    int         offsetTime;             // Offset time for effects
    int         nextTime;               // Next time for effects
    vec4_t      foreColor;              // Foreground color RGBA (float[4])
    vec4_t      backColor;              // Background color RGBA
    vec4_t      borderColor;            // Border color RGBA
    vec4_t      outlineColor;           // Outline color RGBA
    qhandle_t   background;             // Background shader handle
    qhandle_t   borderShader;           // Border shader
} windowDef_t;

// ============================================================================
// ITEM DEFINITION
// ============================================================================
#define MAX_COLOR_RANGES 10

typedef struct {
	vec4_t color;
	int type;
	float low;
	float high;
} colorRangeDef_t;

typedef struct itemDef_s {
    windowDef_t window;                  // 176 bytes (0x00-0xAF) - common positional, border, style, layout info
    rectDef_t textRect;                  // 16 bytes (0xB0-0xBF) - rectangle the text (if any) consumes
    int type;                            // 4 bytes (0xC0-0xC3) - text, button, radiobutton, checkbox, textfield, listbox, combo
    int alignment;                       // 4 bytes (0xC4-0xC7) - left center right
    int font;                            // 4 bytes (0xC8-0xCB) - font
    int textalignment;                   // 4 bytes (0xCC-0xCF) - (optional) alignment for text within rect based on text width
    float textalignx;                    // 4 bytes (0xD0-0xD3) - (optional) text alignment x coord
    float textaligny;                    // 4 bytes (0xD4-0xD7) - (optional) text alignment y coord
    float textscale;                     // 4 bytes (0xD8-0xDB) - scale percentage from 72pts
    int textStyle;                       // 4 bytes (0xDC-0xDF) - (optional) style, normal and shadowed are it for now
    char _padding1[12];                  // 12 bytes (0xE0-0xEB) - PADDING to align to 0xEC
    const char* text;                    // 4 bytes (0xEC-0xEF) - display text *** YOUR OFFSET IS HERE ***
    qboolean textSavegameInfo;           // 4 bytes (0xF0-0xF3)
    void* parent;                        // 4 bytes (0xF4-0xF7) - menu owner
    qhandle_t asset;                     // 4 bytes (0xF8-0xFB) - handle to asset
    const char* mouseEnterText;          // 4 bytes (0xFC-0xFF) - mouse enter script
    const char* mouseExitText;           // 4 bytes (0x100-0x103) - mouse exit script
    const char* mouseEnter;              // 4 bytes (0x104-0x107) - mouse enter script
    const char* mouseExit;               // 4 bytes (0x108-0x10B) - mouse exit script
    const char* action;                  // 4 bytes (0x10C-0x10F) - select script
    const char* onAccept;                // 4 bytes (0x110-0x113) - NERVE - SMF - run when the users presses the enter key
    const char* onFocus;                 // 4 bytes (0x114-0x117) - select script
    const char* leaveFocus;              // 4 bytes (0x118-0x11B) - select script
    const char* cvar;                    // 4 bytes (0x11C-0x11F) - associated cvar
    const char* cvarTest;                // 4 bytes (0x120-0x123) - associated cvar for enable actions
    const char* enableCvar;              // 4 bytes (0x124-0x127) - enable, disable, show, or hide based on value, this can contain a list
    int cvarFlags;                       // 4 bytes (0x128-0x12B) - what type of action to take on cvarenables
    sfxHandle_t focusSound;              // 4 bytes (0x12C-0x12F)
    int numColors;                       // 4 bytes (0x130-0x133) - number of color ranges
    colorRangeDef_t colorRanges[MAX_COLOR_RANGES];  // 240 bytes (0x134-0x223) - (24 bytes each * 10)
    int colorRangeType;                  // 4 bytes (0x224-0x227) - either
    float special;                       // 4 bytes (0x228-0x22B) - used for feeder id's etc.. diff per type
    int cursorPos;                       // 4 bytes (0x22C-0x22F) - cursor position in characters
    void* typeData;                      // 4 bytes (0x230-0x233) - type specific data ptr's
} itemDef_t;

// ============================================================================
// TYPE-SPECIFIC DATA STRUCTURES
// ============================================================================

// List box data
typedef struct {
    int         startPos;
    int         endPos;
    int         drawPadding;
    float       elementWidth;
    float       elementHeight;
    int         elementStyle;
    int         numColumns;
    vec4_t      columnInfo[MAX_LB_COLUMNS];
    const char* doubleClick;
    int         notselectable;
    int         noscrollbar;
    int         resetonfeeder;
} listBoxDef_t;

// Edit field data
typedef struct {
    float       minVal;
    float       maxVal;
    float       defVal;
    float       range;
    int         maxChars;
    int         maxPaintChars;
    int         paintOffset;
} editFieldDef_t;

// Multi value data
typedef struct {
    const char* cvarList[MAX_MULTI_CVARS];
    const char* cvarStr[MAX_MULTI_CVARS];
    float       cvarValue[MAX_MULTI_CVARS];
    int         count;
    int         strDef;
} multiDef_t;

// Model data
typedef struct {
    qhandle_t   model;
    vec3_t      origin;
    vec3_t      angles;
    vec3_t      rotation;
    float       fov_x;
    float       fov_y;
    int         rotationSpeed;
    int         animated;
    int         startframe;
    int         numframes;
    int         loopframes;
    int         fps;
    int         frame;
} modelDef_t;
