// based on code from https://github.com/DJayalath/GameOfLife


#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>  // for memset
#include "colors.h"
#include "bitmap_graphics.h"

#define WIDTH 320
#define HEIGHT 200


char msg[80] = {0};



// CELL STRUCTURE
/* 
Cells are stored in 8-bit chars where the 0th bit represents
the cell state and the 1st to 4th bit represent the number
of neighbours (up to 8). The 5th to 7th bits are unused.
Refer to this diagram: http://www.jagregory.com/abrash-black-book/images/17-03.jpg
*/


void CellMap(unsigned int w, unsigned int h);
void SetCell(unsigned int x, unsigned int y);
void ClearCell(unsigned int x, unsigned int y);
int CellState(int x, int y); // WHY NOT UNSIGNED?
void NextGen();
void Init();
unsigned char* cells;
unsigned char* temp_cells;
unsigned int width;
unsigned int height;
unsigned int length_in_bytes;


// Cell map dimensions
unsigned int cellmap_width = 140;
unsigned int cellmap_height = 140;

// offset for drawing
uint8_t x_offset = 90;
uint8_t y_offset = 30;


// Generation counter
unsigned int generation = 0;


// XRAM locations
#define KEYBORD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data

// HID keycodes that we care about for this demo
#define KEY_ESC 41 // Keyboard ESCAPE
#define KEY_SPC 44 // Keyboard SPACE

// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
bool handled_key = false;

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))


static inline void setPixel(int x, int y, bool enabled)
{
    uint8_t bit;
    RIA.addr0 = (x / 8) + (320 / 8 * y);
    RIA.step0 = 0;
    bit = 128 >> (x % 8);
    if (enabled) {
        RIA.rw0 |= bit;
    } else {
        RIA.rw0 &= ~bit;    
    }
    
}



void DrawCell(unsigned int x, unsigned int y, bool enabled)
{		
    setPixel(x+x_offset, y+y_offset, enabled);
	
}


void CellMap(unsigned int w, unsigned int h)
{
    width = w;
    height = h;
    length_in_bytes = w * h;

    // Use malloc for dynamic memory allocation
    cells = (unsigned char*)malloc(length_in_bytes);
    temp_cells = (unsigned char*)malloc(length_in_bytes);

    // Clear all cells to start
    memset(cells, 0, length_in_bytes);
}


void SetCell(unsigned int x, unsigned int y)
{
	int w = width, h = height;
	int xoleft, xoright, yoabove, yobelow;
	unsigned char *cell_ptr = cells + (y * w) + x;

	// Calculate the offsets to the eight neighboring cells,
	// accounting for wrapping around at the edges of the cell map
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;

	*(cell_ptr) |= 0x01; // Set first bit to 1

	// Change successive bits for neighbour counts
	*(cell_ptr + yoabove + xoleft) += 0x02;
	*(cell_ptr + yoabove) += 0x02;
	*(cell_ptr + yoabove + xoright) += 0x02;
	*(cell_ptr + xoleft) += 0x02;
	*(cell_ptr + xoright) += 0x02;
	*(cell_ptr + yobelow + xoleft) += 0x02;
	*(cell_ptr + yobelow) += 0x02;
	*(cell_ptr + yobelow + xoright) += 0x02;

    setPixel(x+x_offset, y+y_offset, true);
}

void ClearCell(unsigned int x, unsigned int y)
{
	int w = width, h = height;
	int xoleft, xoright, yoabove, yobelow;
	unsigned char *cell_ptr = cells + (y * w) + x;

	// Calculate the offsets to the eight neighboring cells,
	// accounting for wrapping around at the edges of the cell map
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;


	*(cell_ptr) &= ~0x01; // Set first bit to 0

	// Change successive bits for neighbour counts
	*(cell_ptr + yoabove + xoleft) -= 0x02;
	*(cell_ptr + yoabove) -= 0x02;
	*(cell_ptr + yoabove + xoright) -= 0x02;
	*(cell_ptr + xoleft) -= 0x02;
	*(cell_ptr + xoright) -= 0x02;
	*(cell_ptr + yobelow + xoleft) -= 0x02;
	*(cell_ptr + yobelow) -= 0x02;
	*(cell_ptr + yobelow + xoright) -= 0x02;

    setPixel(x+x_offset, y+y_offset, false);
}

int CellState(int x, int y)
{
	unsigned char *cell_ptr =
		cells + (y * width) + x;

	// Return first bit (LSB: cell state stored here)
	return *cell_ptr & 0x01;
}

void NextGen()
{
	unsigned int x, y, count;
	unsigned int h, w;
	unsigned char *cell_ptr;

    h = height;
    w = width;

	// Copy to temp map to keep an unaltered version
	memcpy(temp_cells, cells, length_in_bytes);

	// Process all cells in the current cell map
	cell_ptr = temp_cells;

	for (y = 0; y < h; y++) {

		x = 0;
		do {

			// Zero bytes are off and have no neighbours so skip them...
			while (*cell_ptr == 0) {
				cell_ptr++; // Advance to the next cell
				// If all cells in row are off with no neighbours go to next row
				if (++x >= w) goto RowDone;
			}

			// Remaining cells are either on or have neighbours
			count = *cell_ptr >> 1; // # of neighboring on-cells
			if (*cell_ptr & 0x01) {

				// On cell must turn off if not 2 or 3 neighbours
				if ((count != 2) && (count != 3)) {
					ClearCell(x, y);
					// setPixel(x+x_offset, y+y_offset, false);
				}
			}
			else {

				// Off cell must turn on if 3 neighbours
				if (count == 3) {
					SetCell(x, y);
					// setPixel(x+x_offset, y+y_offset, true);
				}
			}

			// Advance to the next cell byte
			cell_ptr++;

		} while (++x < w);
	RowDone:;
	}
}


static void setup()
{
    init_bitmap_graphics(0xFF00, 0x0000, 0, 1, 320, 240, 1, 0, HEIGHT);
    erase_canvas();
    xreg(1, 0, 1, 0, 1, HEIGHT, 240);

    CellMap(cellmap_width, cellmap_height);


    draw_hline(WHITE, x_offset, y_offset, cellmap_width+1);
    draw_hline(WHITE, x_offset, y_offset+cellmap_height, cellmap_width+1);
    draw_vline(WHITE, x_offset-1, y_offset, cellmap_height+1);
    draw_vline(WHITE, x_offset+cellmap_width, y_offset, cellmap_height+1);

    // glider
	// SetCell(11, 20);
	// SetCell(12, 21);
	// SetCell(10, 22);
	// SetCell(11, 22);
	// SetCell(12, 22);

	// glider generator
	// 4-8-12 diamond
	SetCell(68, 66);
	SetCell(69, 66);
	SetCell(70, 66);
	SetCell(71, 66);
	SetCell(66, 68);
	SetCell(67, 68);
	SetCell(68, 68);
	SetCell(69, 68);
	SetCell(70, 68);
	SetCell(71, 68);
	SetCell(72, 68);
	SetCell(73, 68);
	SetCell(64, 70);
	SetCell(65, 70);
	SetCell(66, 70);
	SetCell(67, 70);
	SetCell(68, 70);
	SetCell(69, 70);
	SetCell(70, 70);
	SetCell(71, 70);
	SetCell(72, 70);
	SetCell(73, 70);
	SetCell(74, 70);
	SetCell(75, 70);
	SetCell(66, 72);
	SetCell(67, 72);
	SetCell(68, 72);
	SetCell(69, 72);
	SetCell(70, 72);
	SetCell(71, 72);
	SetCell(72, 72);
	SetCell(73, 72);
	SetCell(68, 74);
	SetCell(69, 74);
	SetCell(70, 74);
	SetCell(71, 74);
}

void main()
{
    char msg[80] = {0};
    uint8_t i;

    puts("\n\n\n");
    puts("Press SPACE to start, ESC to exit");

    setup();

    // wait for a keypress
    xreg_ria_keyboard(KEYBORD_INPUT);
    RIA.addr0 = KEYBORD_INPUT;
    RIA.step0 = 0;
    while (1) {

        // fill the keystates bitmask array
        for (i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t j, new_keys;
            RIA.addr0 = KEYBORD_INPUT + i;
            new_keys = RIA.rw0;
///*
            // check for change in any and all keys
            for (j = 0; j < 8; j++) {
                uint8_t new_key = (new_keys & (1<<j));
                // if ((((i<<3)+j)>3) && (new_key != (keystates[i] & (1<<j)))) {
                    // printf( "key %d %s\n", ((i<<3)+j), (new_key ? "pressed" : "released"));
                // }
            }
//*/
            keystates[i] = new_keys;
        }

        // check for a key down
        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (key(KEY_SPC)) {
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }
    }


    while (1) {
        generation++;
        sprintf(msg, "generation %u", generation);
        puts(msg);
        NextGen();

        xreg_ria_keyboard(KEYBORD_INPUT);
        RIA.addr0 = KEYBORD_INPUT;
        RIA.step0 = 0;
        

        // fill the keystates bitmask array
        for (i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t j, new_keys;
            RIA.addr0 = KEYBORD_INPUT + i;
            new_keys = RIA.rw0;
///*
            // check for change in any and all keys
            for (j = 0; j < 8; j++) {
                uint8_t new_key = (new_keys & (1<<j));
                // if ((((i<<3)+j)>3) && (new_key != (keystates[i] & (1<<j)))) {
                //     printf( "key %d %s\n", ((i<<3)+j), (new_key ? "pressed" : "released"));
                // }
            }
//*/
            keystates[i] = new_keys;
        }

        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (key(KEY_ESC)) {
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }
    };
    
}
