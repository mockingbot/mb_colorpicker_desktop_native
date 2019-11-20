#pragma once


// for load mask images

extern unsigned char RES_Circle_Mask[];
extern unsigned int RES_Circle_Mask_len;


/*
        /+/       <---------------------------
         +                          | GRID_NUMUBER_L*2 + 1
         +      /- GRID_NUMUBER_L   |
         +     /                    |
         +<---v-->|                 |
/+/+/+/+/#/+/+/+/+/                 |
         +                          |
         +                          |
         +                          |
         +                          |
        /+/       <---------------------------
^_________________^
|                 |
|  GRID_NUMUBER_L*2 + 1 |
*/

const int GRID_PIXEL = 9;
const int GRID_NUMUBER_L = 8;

const int GRID_NUMUBER = GRID_NUMUBER_L*2 + 1;

const int CAPTURE_WIDTH = GRID_NUMUBER;
const int CAPTURE_HEIGHT = GRID_NUMUBER;

#if defined(OS_MACOS)

const int UI_WINDOW_SIZE = 16 + // <- window shadow
                           GRID_PIXEL + 2 + // center pixel
                           (GRID_NUMUBER_L*GRID_PIXEL + GRID_NUMUBER_L)*2;

#elif defined(OS_WINDOWS)

const int UI_WINDOW_SIZE = 0 + // <- without window shadow
                           GRID_PIXEL + 2 + // center pixel
                           (GRID_NUMUBER_L*GRID_PIXEL + GRID_NUMUBER_L)*2;


#endif // defined(OS_WINDOWS)
