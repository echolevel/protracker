#include <stdint.h>
#include "pt_palette.h"
#include "pt_header.h"
#include "pt_helpers.h"

uint32_t palette[PALETTE_NUM] =
{
    // -----------------------------
    0x00000000, // 00- PAL_BACKGRD
    0x00BBBBBB, // 01- PAL_BORDER
    0x00888888, // 02- PAL_GENBKG
    0x00555555, // 03- PAL_GENBKG2
    0x00FFDD00, // 04- PAL_QADSCP
    0x00DD0044, // 05- PAL_PATCURSOR
    0x00000000, // 06- PAL_GENTXT
    0x003344FF, // 07- PAL_PATTXT
    // -----------------------------
    0x0000FFFF, // 08- PAL_SAMPLLINE
    0x000000FF, // 09- PAL_LOOPPIN
    0x00770077, // 10- PAL_TEXTMARK
    0x00444444, // 11- PAL_MOUSE_1
    0x00777777, // 12- PAL_MOUSE_2
    0x00AAAAAA, // 13- PAL_MOUSE_3
    // -----------------------------
    0x00C0FFEE  // 14- PAL_COLORKEY
    // -----------------------------
};

void pointerErrorMode(void)
{
    palette[PAL_MOUSE_1] = 0x00770000;
    palette[PAL_MOUSE_2] = 0x00990000;
    palette[PAL_MOUSE_3] = 0x00CC0000;

    editor.ui.refreshMousePointer = true;
}

void setMsgPointer(void)
{
    editor.ui.pointerMode  = POINTER_MODE_READ_DIR;

    palette[PAL_MOUSE_1] = 0x00004400;
    palette[PAL_MOUSE_2] = 0x00007700;
    palette[PAL_MOUSE_3] = 0x0000AA00;

    editor.ui.refreshMousePointer = true;
}

void pointerSetMode(int8_t pointerMode, uint8_t carry)
{
    editor.ui.refreshMousePointer = true;

    switch (pointerMode)
    {
        case POINTER_MODE_IDLE:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00444444;
            palette[PAL_MOUSE_2] = 0x00777777;
            palette[PAL_MOUSE_3] = 0x00AAAAAA;
        }
        break;

        case POINTER_MODE_PLAY:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00444400;
            palette[PAL_MOUSE_2] = 0x00777700;
            palette[PAL_MOUSE_3] = 0x00AAAA00;
        }
        break;

        case POINTER_MODE_EDIT:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00000066;
            palette[PAL_MOUSE_2] = 0x00004499;
            palette[PAL_MOUSE_3] = 0x000055BB;
        }
        break;

        case POINTER_MODE_EDIT_PLAY:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00000066;
            palette[PAL_MOUSE_2] = 0x00004499;
            palette[PAL_MOUSE_3] = 0x000055BB;
        }
        break;

        case POINTER_MODE_MSG1:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00440044;
            palette[PAL_MOUSE_2] = 0x00770077;
            palette[PAL_MOUSE_3] = 0x00AA00AA;
        }
        break;

        case POINTER_MODE_READ_DIR:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x00004400;
            palette[PAL_MOUSE_2] = 0x00007700;
            palette[PAL_MOUSE_3] = 0x0000AA00;
        }
        break;

        case POINTER_MODE_LOAD:
        {
            editor.ui.pointerMode = pointerMode;

            if (carry)
                editor.ui.previousPointerMode = editor.ui.pointerMode;

            palette[PAL_MOUSE_1] = 0x000000AA;
            palette[PAL_MOUSE_2] = 0x00000077;
            palette[PAL_MOUSE_3] = 0x00000044;
        }
        break;

        default: break;
    }
}

void pointerSetPreviousMode(void)
{
    if (editor.ui.getLineFlag || editor.ui.askScreenShown || editor.ui.clearScreenShown)
        pointerSetMode(POINTER_MODE_MSG1, NO_CARRY);
    else
        pointerSetMode(editor.ui.previousPointerMode, NO_CARRY);
}
