/*****************************************************************************
**	pngb.h
**
**	Data type and function definitions for the PNGB graphics converter.
**	
** 	Copyright (c) 2015 Elias Zacarias
**
** 	Permission is hereby granted, free of charge, to any person obtaining a
** 	copy of this software and associated documentation files (the "Software"),
** 	to deal in the Software without restriction, including without limitation
** 	the rights to use, copy, modify, merge, publish, distribute, sublicense,
** 	and/or sell copies of the Software, and to permit persons to whom the
** 	Software is furnished to do so, subject to the following conditions:
** 	
** 	The above copyright notice and this permission notice shall be included in
** 	all copies or substantial portions of the Software.

** 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** 	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** 	IN THE SOFTWARE.
** 	
*****************************************************************************/

#ifndef __PNGGB_H
#define __PNGGB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*###########################################################################
 ##                                                                        ##
 ##                              C O N S T A N T S                         ##
 ##                                                                        ##
 ###########################################################################*/
#define PNGB_URL				"https://github.com/battlecoder/pngb"
#define PNGB_VERSION_MAJOR		1
#define PNGB_VERSION_MINOR		0

#define BLACK_VAL				0
#define DARKGRAY_VAL			82
#define LIGHTGRAY_VAL			172
#define WHITE_VAL				255

/*###########################################################################
 ##                                                                        ##
 ##           D A T A   S T R U C T U R E S   A N D   T Y P E S            ##
 ##                                                                        ##
 ###########################################################################*/
typedef unsigned char BYTE;
typedef unsigned long DWORD;

typedef struct{
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE L;						/* "lightness". Computed and used for sorting the palette.              */
}RGB_PALETTE_ENTRY;

typedef enum{
	TARGET_BKG, TARGET_SPRITE, TARGET_WINDOW
}TARGET_TYPE;

typedef struct {
	TARGET_TYPE type;			/* Whether we are trying to generate SPRITES, BG tiles or WINDOW tiles. */
	long transparent;			/* Transparent color for sprites. Either a palette index or 24bit RGB.  */
	int grayscale;				/* Convert full palette to grayscale, then reduce to the 4 GB "shades". */
	int create_palette;			/* Set to != 0 for palette code output.                                 */
	int sort_palette;			/* Set to != 0 for the program to sort the palette from light to dark.  */
	int create_map;				/* Set to != 0 to include a tile map in the output.                     */
	BYTE palnumber;				/* Desired palette number. Affects sample code and "attribute" data.	*/
	int big_sprite;				/* Set to != 0 for 8x16 sprites.                                        */
	int test_code;				/* Set to != 0 to generate ready-to-compile test code.                  */
	int tile_reduction;			/* Set to != 0 to enable redundant tile detection and reduction.		*/
	BYTE baseindex;				/* Index of the first sprite/tile that will be defined.					*/
	int verbose;				/* Set to != 0 for detailed log output of the process.					*/
	char name[256];				/* Sprite/tileset name.                                                 */
} OPTIONS;

typedef struct{
	int w;						/* original width.                                                      */
	int h;						/* original height.                                                     */
	int cols; 					/* width in tiles.                                                      */
	int rows;					/* height in tiles.                                                     */
	int tileh;					/* either 8 or 16.                                                      */
	int total_tiles;			/* total tiles.                                                         */
	BYTE *tiles; 				/* Each tile is either 16 bytes in size (8x8 tiles) or 32 (8x16 tiles). */
	unsigned int *tilemap;		/* A tilemap of the image. this will be cols x rows in size.            */
	unsigned short int pal[4];	/* Each palette entry is 15 bits (For GBC).                             */
}PICDATA;

/*###########################################################################
 ##                                                                        ##
 ##                             F U N C T I O N S                          ##
 ##                                                                        ##
 ###########################################################################*/
void	error (const char * format, ...);
void	verbose (const char * format, ...);
PICDATA	*process_image (const char* filename);
void	free_gb_pict (PICDATA *data);
void	gb_check_warnings (PICDATA *gbpic);
void	gbdk_c_code_output (PICDATA *gbpic, FILE *f);
void	code_disclaimer_c (char *inputfile, char *outputfile, FILE *f);

/*###########################################################################
 ##                                                                        ##
 ##                           G L O B A L   D A T A                        ##
 ##                                                                        ##
 ###########################################################################*/
/* Global options. Affect the whole process */
extern OPTIONS globalOpts;

#endif
