#ifndef LCDELEMENTS_H_
#define LCDELEMENTS_H_

#include <cstdint>

//******************************************************************************
//
// LCD Element Outlines
//
//******************************************************************************
//
// The LCD elements on the display are described by vector outlines.  Each
// element (object) is described by one or more outlines.  In the case of
// the juggler's torso, three outlines are used one for the overall shape and
// then two smaller unfilled areas for the eye.  Standard winding mode is used
// to determine the filled and unfilled areas of an object.
//
// Each outline is a connected path that is comprised of straight line segments
// and/or cubic Bezier segments. An array of OUTLINE_NODE(s) is used to
// describe this path.
//
// The OUTLINE_NODE holds the x,y position and a flag that tells us what type
// of point it is as well as tell us when a new outline ends and/or when the
// object ends.  The x and y positions are integer values such that the
// whole area is framed between 0 and 65535 (XOutlineMax in LCDElements.inc)
// and y uses the same scaling.  The y axis is downwards +ve (just like the
// GDI).
//
// The type of node is one of OUTLINE_MOVE, _LINE or _CONTROL.  While these
// flags are given separate bits, it's not valid to combine any or these three
// flags.  _MOVE moves to a new point, _LINE draws a line from the last point
// to this one, _CONTROL is a control point on a Bezier spline.  Three
//_CONTROL nodes must follow in succession.
//
// Additional information may be given, _CLOSE indicates that the outline
// should be closed with a straight line to the first point on the
// outline, _FILL indicates that this is the last node in the object.

#define OUTLINE_MOVE 0x01
#define OUTLINE_LINE 0x02
#define OUTLINE_CONTROL 0x04

#define OUTLINE_CLOSE 0x10
#define OUTLINE_FILL 0x20

struct OUTLINE_NODE {
    uint16_t x;
    uint16_t y;
    uint8_t flags;
};

extern const OUTLINE_NODE LCDOutlineNodes[];

// This array holds the starting node for each LCD element.  The array
// is ordered according to the LCD_XXXX constants in LCDElements.h.
// Hence LCDElements[LCD_RIGHTCRUSH] gives the index into
// LCDOutlineNodes for the "Crush!" text on the right side.
extern const uint16_t LCDElements[];

extern const uint16_t XOutlineMax;
extern const uint16_t YOutlineMax;

// The number of elements
#define LCD_NO_ELEMENTS 76

//******************************************************************************
//
// LCD Elements Constants
//
//******************************************************************************
//
// These constants are objects indices for the various LCD elements on the
// screen.

// LCD_INNER0 to LCD_INNER7 are the eight ball positions for the inner track
// of balls.  The go in order from the right side all the way around to the
// left.
#define LCD_INNER0 0
#define LCD_INNER1 1
#define LCD_INNER2 2
#define LCD_INNER3 3
#define LCD_INNER4 4
#define LCD_INNER5 5
#define LCD_INNER6 6
#define LCD_INNER7 7

// Similarly LCD_MID0 to LCD_MID9 are the ten ball positions for the
// mid trak.
#define LCD_MID0 8
#define LCD_MID1 9
#define LCD_MID2 10
#define LCD_MID3 11
#define LCD_MID4 12
#define LCD_MID5 13
#define LCD_MID6 14
#define LCD_MID7 15
#define LCD_MID8 16
#define LCD_MID9 17

// and then for the twelve outer track positions
#define LCD_OUTER0 18
#define LCD_OUTER1 19
#define LCD_OUTER2 20
#define LCD_OUTER3 21
#define LCD_OUTER4 22
#define LCD_OUTER5 23
#define LCD_OUTER6 24
#define LCD_OUTER7 25
#define LCD_OUTER8 26
#define LCD_OUTER9 27
#define LCD_OUTER10 28
#define LCD_OUTER11 29

// Forearm positions.  The ordering is important, positions that occur
// in pairs (e.g. the left is in the outer track when the right is in the
// inner) are 3 apart.
#define LCD_LEFTOUTER 30
#define LCD_LEFTMID 31
#define LCD_LEFTINNER 32
#define LCD_RIGHTINNER 33
#define LCD_RIGHTMID 34
#define LCD_RIGHTOUTER 35

// Legs.
#define LCD_LEFTLEGUP 36
#define LCD_LEFTLEGDOWN 37
#define LCD_RIGHTLEGDOWN 38
#define LCD_RIGHTLEGUP 39

// Number elements.
// I'm sure there's a standard way of descibing the seven segment
// numbers, but I use the following the lettering system
//
//        -A-
//       |   |
//       B   F
//       |   |
//        -G-
//       |   |
//       C   E
//       |   |
//        -D-
//
// From this the table below shows how the elements are enabled for each
// number
//
//        |  A  |  B  |  C  |  D  |  E  |  F  |  G  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     0  |  1  |  1  |  1  |  1  |  1  |  1  |  0  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     1  |  0  |  0  |  0  |  0  |  1  |  1  |  0  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     2  |  1  |  0  |  1  |  1  |  0  |  1  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     3  |  1  |  0  |  0  |  1  |  1  |  1  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     4  |  0  |  1  |  0  |  0  |  1  |  1  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     5  |  1  |  1  |  0  |  1  |  1  |  0  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     6  |  1  |  1  |  1  |  1  |  1  |  0  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     7  |  1  |  1  |  0  |  0  |  1  |  1  |  0  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     8  |  1  |  1  |  1  |  1  |  1  |  1  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//     9  |  1  |  1  |  0  |  1  |  1  |  1  |  1  |
//        +-----+-----+-----+-----+-----+-----+-----+
//

// Left most (thousands) number digit
#define LCD_THOUA 40
#define LCD_THOUB 41
#define LCD_THOUC 42
#define LCD_THOUD 43
#define LCD_THOUE 44
#define LCD_THOUF 45
#define LCD_THOUG 46

// digit giving hundreds
#define LCD_HUNDA 47
#define LCD_HUNDB 48
#define LCD_HUNDC 49
#define LCD_HUNDD 50
#define LCD_HUNDE 51
#define LCD_HUNDF 52
#define LCD_HUNDG 53

// digit giving tens
#define LCD_TENSA 54
#define LCD_TENSB 55
#define LCD_TENSC 56
#define LCD_TENSD 57
#define LCD_TENSE 58
#define LCD_TENSF 59
#define LCD_TENSG 60

// digit for units
#define LCD_UNITA 61
#define LCD_UNITB 62
#define LCD_UNITC 63
#define LCD_UNITD 64
#define LCD_UNITE 65
#define LCD_UNITF 66
#define LCD_UNITG 67

// The torso and head of the juggler
#define LCD_BODY 68

// The upper arm elements
#define LCD_LEFTARM 69
#define LCD_RIGHTARM 70

// The "CRUSH!" text on the left hand side
#define LCD_LEFTCRUSH 71

// The splattered ball on the left
#define LCD_LEFTSPLAT 72

#define LCD_RIGHTCRUSH 73
#define LCD_RIGHTSPLAT 74

// Outline that frames the area.  Strictly speaking this isn't an LCD element
// but we treat it as one.  On the actual game this is a bit of plastic that
// sits in front of the LCD screen.
#define LCD_FRAME 75

// The colours of the background and enabled LCD element.
//#define LCD_COL_BK RGB(100, 100, 90)
//#define LCD_COL_ON RGB(50, 50, 50)

//#define LCD_COL_ON PALETTERGB(64, 64, 64)

// these values work well

/*
#define LCD_COL_BK PALETTERGB(128, 128, 112)
#define LCD_COL_ON PALETTERGB(66, 66, 66)
*/

//#define LCD_COL_BK PALETTERGB(255, 255, 255)
//#define LCD_COL_ON PALETTERGB(0, 0, 0)

// these two are from the web safe palette
//#define LCD_COL_BK PALETTERGB(128, 128, 85)
//#define LCD_COL_ON PALETTERGB(42, 42, 42)

//#define LCD_COL_BK RGB(255, 255, 255)
//#define LCD_COL_ON RGB(0, 0, 0)

#endif
