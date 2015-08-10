/*****************************************************************************
**	pngb.c
**
**	Code for the PNG GB Converter.
**	Currently this module contains all the code for the converter except for
**	the PNG loading routines (We are using LodePNG for that) and the program
**  entry point (and command line parsing), which are in main.c.
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
#include <time.h>
#include "lodepng.h"
#include "pngb.h"

#define MIN(a, b) (a < b ? a : b)

OPTIONS globalOpts;
/*###########################################################################
 ##                                                                        ##
 ##                     M I S C   F U N C T I O N S                        ##
 ##                                                                        ##
 ###########################################################################*/
/*** error ******************************************************************
 * Prints a message and exits.                                              *
 ****************************************************************************/
void error(const char * format, ...){
	va_list args;
	va_start (args, format);
	vprintf (format, args);
	puts("\n\n");
	va_end (args);
	exit(0);
}

/*** verbose ****************************************************************
 * Outputs a message to the console only if verbose mode is enabled.        *
 ****************************************************************************/
void verbose(const char * format, ...){
	if (!globalOpts.verbose) return;
	va_list args;
	va_start (args, format);
	vprintf (format, args);
	va_end (args);
}

/*** sanitize_var_name ******************************************************
 * Normalizes a string so it becomes a valid variable name (ensures it does *
 * not start with a number and replaces non-alphanumeric chars with "_".    *
 ****************************************************************************/
void sanitize_var_name(unsigned char *var, int len){
	int c;
	if (!isalpha(var[0])) var[0] = '_';
	for (c=1; c<len; c++) if (!isalnum(var[c])) var[c] = '_';
}

/*###########################################################################
 ##                                                                        ##
 ##                         A U X    F U N C T I O N S                     ##
 ##                                                                        ##
 ###########################################################################*/
/*** is8x16Mode *************************************************************
 * Returns non-zero if we are supposed to output 8x16 data instead of 8x8.  *
 ****************************************************************************/
int is8x16Mode(){
	return (globalOpts.big_sprite && globalOpts.type == TARGET_SPRITE);
}

/*###########################################################################
 ##                                                                        ##
 ##          P A L E T T E   A N D   C O L O R   H A N D L I N G           ##
 ##                                                                        ##
 ###########################################################################*/
/*** color_light_val *********************************************************
 * Classic RGB to "grayscale" conversion.                                    *
 ****************************************************************************/
BYTE color_light_val(BYTE r, BYTE g, BYTE b){
	return 0.2989*r + 0.5870*g + 0.1140*b;
}

/*** set_palette_color ******************************************************
 * Provides a convenient way of filling RGPB_PALETTE_ENTRY structures       *
 ****************************************************************************/
void set_palette_color(RGB_PALETTE_ENTRY *pal, BYTE r, BYTE g, BYTE b){
	pal->r = r;
	pal->g = g;
	pal->b = b;
	pal->L = color_light_val(r, g, b);
}

/*** find_palette_color *****************************************************
 * Finds a given color in the palette.                                      *
 ****************************************************************************/
int find_palette_color(BYTE r, BYTE g, BYTE b, RGB_PALETTE_ENTRY *palette, BYTE tcolors){
	int i;

	for (i=0; i < tcolors; i++){
		if ((palette[i].r == r) && (palette[i].g == g) && (palette[i].b == b)){
			return i;
		}
	}
	return -1;
}

/*** create_gb_gray_pal *****************************************************
 * Create a "GB-compatible" 4-shade grayscale palette.                      *
 ****************************************************************************/
RGB_PALETTE_ENTRY *create_gb_gray_pal(){
	RGB_PALETTE_ENTRY *pal = (RGB_PALETTE_ENTRY *)malloc(4*sizeof(RGB_PALETTE_ENTRY));

	set_palette_color (&pal[0], WHITE_VAL		, WHITE_VAL		, WHITE_VAL);
	set_palette_color (&pal[1], LIGHTGRAY_VAL	, LIGHTGRAY_VAL	, LIGHTGRAY_VAL);
	set_palette_color (&pal[2], DARKGRAY_VAL	, DARKGRAY_VAL	, DARKGRAY_VAL);
	set_palette_color (&pal[3], BLACK_VAL		, BLACK_VAL		, BLACK_VAL);
	return pal;
}

/*** match_lightness ********************************************************
 * Returns the entry from an "intensity set" that is closer to a certain    *
 * "lightness" value.                                                       *
 ****************************************************************************/
BYTE match_lightness(BYTE lightness, BYTE *intensity_set, BYTE set_count){
	BYTE nearest = 0;
	int shortestdist = abs(intensity_set[0] - lightness);
	BYTE c, d;

	for (c=1; c<set_count; c++){
		d = abs(intensity_set[c] - lightness);
		if (d < shortestdist){
			shortestdist = d;
			nearest = c;
		}
	}
	return nearest;
}

/*** intensity_to_shades ****************************************************
 * Returns the closest "gameboy" palette value that closests match a given  *
 * level of lightness. If sprite output is set, we will rule out white as   *
 * candidate.                                                               *
 ****************************************************************************/
BYTE intensity_to_shades(BYTE lightness){
	BYTE intensities[4] = {WHITE_VAL, LIGHTGRAY_VAL, DARKGRAY_VAL, BLACK_VAL};
	if (globalOpts.type == TARGET_SPRITE) return match_lightness(lightness, &intensities[1], 3) + 1;
	return match_lightness(lightness, intensities, 4);
}

/*** copy_palette ***********************************************************
 * Copies a palette entry into another.                                     *
 ****************************************************************************/
void copy_palette (RGB_PALETTE_ENTRY *to, RGB_PALETTE_ENTRY *from){
	memcpy(to, from, sizeof(RGB_PALETTE_ENTRY));
}

/*** swap_palette_entries ***************************************************
 * Swaps two palette entries (in probably the most basic way).              *
 ****************************************************************************/
void swap_palette_entries (RGB_PALETTE_ENTRY *a, RGB_PALETTE_ENTRY *b){
	if (a == b) return;
	RGB_PALETTE_ENTRY temp;
	copy_palette(&temp, b);
	copy_palette(b, a);
	copy_palette(a, &temp);
}

/*** swap_palette_indexes ***************************************************
 * Updtes a palete map swaping all the references to a given pair of colors.*
 ****************************************************************************/
void swap_palette_indexes(BYTE *palette_map, BYTE a, BYTE b, BYTE tColors){
	if (a == b) return;
	int c;
	for (c=0;c<tColors;c++) {
		if (palette_map[c] == a){
			palette_map[c] = b;
		}else if (palette_map[c] == b){
			palette_map[c] = a;
		}
	}
}

/*** sort_palette ***********************************************************
 * Sorts a palette from lighter to darkess creating a palette_map that will *
 * keep track of the equivalence with the former colors.                    *
 ****************************************************************************/
void sort_palette(RGB_PALETTE_ENTRY *palette, BYTE *palette_map, BYTE tColors, BYTE startAt){
	/* We will re-arrange the palette but we will also keep track of the new
	positions of every entry in the sorted palette via the palette map. */

	/* Since we are not going for efficiency nor real-time application we will
	use bubble-sort (also palettes will normally be really small in size) */
	unsigned char i,A, B, temp;
	unsigned char newn, n = tColors;
	do{
		newn = startAt;
		for (i=startAt+1; i<n; i++){
			if (palette[i-1].L < palette[i].L){
				/* Swap */
				swap_palette_indexes (palette_map, i-1, i, tColors);
				swap_palette_entries (&palette[i-1], &palette[i]);
				newn = i;
			}
		}
      	n = newn;
    }while (n > startAt);
}

/*###########################################################################
 ##                                                                        ##
 ##                   G B   P I C D A T A   H A N D L I N G                ##
 ##                                                                        ##
 ###########################################################################*/
/*** allocate_gb_pict *******************************************************
 * Creates a basic structure to hold GB picture data (palette, tiles, etc). *
 ****************************************************************************/
PICDATA *allocate_gb_pict(int w, int h, int _16hMode){
	int t;

	PICDATA *picd = (PICDATA *)malloc(sizeof(PICDATA));
	picd->tileh	= (_16hMode? 16 : 8);
	picd->w = w;
	picd->h = h;
	/* Round to nearest 8x[tileh] block */
	picd->cols	= (w % 8 == 0			? w/8 			: w/8 + 1);
	picd->rows	= (h % picd->tileh == 0	? h/picd->tileh	: h/picd->tileh + 1);
	/* Without optimization the tile list will be cols x rows entries in size */
	picd->total_tiles = picd->cols*picd->rows;
	/* Each byte contains 4 pixels of data so it's 2 bytes per row */
	int tBytes = picd->total_tiles*picd->tileh*2;
	picd->tiles = (unsigned char *)malloc(tBytes);
	memset (picd->tiles, 0, tBytes);

	/* Tilemap will always be cols x rows */
	picd->tilemap = (unsigned int *)malloc(picd->total_tiles*sizeof(unsigned int));

	/* Generate a default non-optimized tilemap for this picture */
	for (t=0; t<picd->total_tiles; t++) picd->tilemap[t] = t;

	return picd;
}

/*** free_gb_pict ***********************************************************
 * The opposite from the function above, I guess.                           *
 ****************************************************************************/
void free_gb_pict(PICDATA *data){
	if (!data) return;
	free (data->tiles);
	free (data->tilemap);
	free (data);
}

/*** set_tile_pixel *********************************************************
 * Sets the color of a pixel in a given tile (from a PICDATA structure).    *
 ****************************************************************************/
void set_tile_pixel(PICDATA *data, unsigned int tile, unsigned char x, unsigned char y, unsigned char color){
	if (!data || tile >= data->cols*data->rows || x >= 8 ||  y >= data->tileh || color >= 4) return;
	int base = tile*data->tileh*2 + y*2;

	BYTE mask			= 0x80 >> x;
	/* Clear the pixel at that location first */
	data->tiles[base] 	&= ~mask;
	data->tiles[base+1] &= ~mask;
	/* Set bits if required */
	if (color & 1) data->tiles[base] 	|= mask;
	if (color & 2) data->tiles[base+1]  |= mask;
}

/*** get_tile_row ***********************************************************
 * Returns a row from a tile. Sounds trivial but it's not. For some reason  *
 * row data is stored in an "interleaved" fashion in the gameboy hardware.  *
 ****************************************************************************/
unsigned short get_tile_row(PICDATA *data, unsigned int tile, unsigned int row){
	if (!data || row >= data->tileh || tile >= data->cols*data->rows) return 0;
	int base =tile*data->tileh*2 + row*2;
	return (unsigned short)((unsigned char)data->tiles[base]<<8 | (unsigned char)data->tiles[base+1]&0xff);
}

/*** set_gb_pal_entry *******************************************************
 * Sets a palette entry to a given RGB color, perfoming bits reduction.     *
 ****************************************************************************/
void set_gb_pal_entry(PICDATA *data, unsigned int index, BYTE r, BYTE g, BYTE b){
	if (!data || index >= 4) return;
	/* very basic conversion to 15 bits here. */
	data->pal[index] = (unsigned short) ((r>>3) | ((g>>3)<<5) | ((b>>3)<<10));
}

/*###########################################################################
 ##                                                                        ##
 ##                   I M A G E   P R O C E S S I N G                      ##
 ##                                                                        ##
 ###########################################################################*/
/*** compare_gb_tiles *******************************************************
 * Compares two GB tiles. Returns zero if equal.                            *
 ****************************************************************************/
int compare_gb_tiles(PICDATA *pic, unsigned int t0, unsigned int t1){
	if (!pic || t0 >= pic->cols*pic->rows || t1 >= pic->cols*pic->rows) return 1;
	if (t0 == t1) return 1;

	int tdatasize = pic->tileh*2;
	return memcmp (&pic->tiles[t0*tdatasize], &pic->tiles[t1*tdatasize], tdatasize);
}

/*** copy_gb_tile ***********************************************************
 * Copies a tile inside PICDATA, from 'src' to 'dest'.                      *
 ****************************************************************************/
void copy_gb_tile(PICDATA *pic, unsigned int dest, unsigned int src){
	if (!pic || dest >= pic->cols*pic->rows || src >= pic->cols*pic->rows) return;
	int tdatasize = pic->tileh*2;
	memcpy (&pic->tiles[dest*tdatasize], &pic->tiles[src*tdatasize], tdatasize);
}

/*** replace_in_tilemap *****************************************************
 * Replaces any reference in PICDATA to tile 'told' with 'tnew'.            *
 ****************************************************************************/
void replace_in_tilemap(PICDATA *pic, unsigned int told, unsigned int tnew){
	if (!pic) return;
	int t, mapsize = pic->cols*pic->rows;
	for (t=0; t<mapsize; t++) {
		if (pic->tilemap[t] == told) pic->tilemap[t] = tnew;
	}
}

/*** do_tile_reduction *****************************************************
 * Searches for -and removes- redundant (identical) tiles in PICDATA.      *
 ****************************************************************************/
void do_tile_reduction(PICDATA *pic){
	if (!pic) return;

	int t2, t1 = 0;
	int old_tTiles = pic->total_tiles;

	for (t1=0; t1<pic->total_tiles; t1++){
		t2 = t1+1;
		while (t2 < pic->total_tiles){
			if (compare_gb_tiles(pic, t1, t2) == 0){
				/* t1 and t2 are the same. Let's remove t2 and replace all references to t2 for t1. */
				replace_in_tilemap (pic, t2, t1);
				/* Now to "delete" it we will copy the current last tile into t2 and then lower the "total_tiles" count. */
				copy_gb_tile(pic, t2, pic->total_tiles-1);
				/* All the references to the "last tile" we just swapped positions with,
				should be replaced with t2, which is the new position it occupies. */
				replace_in_tilemap (pic, pic->total_tiles-1, t2);
				pic->total_tiles--;
			}else{
				t2++;
			}
		}
	}
	verbose ("-- %d tiles reduced. New tile count: %d\n", old_tTiles - pic->total_tiles, pic->total_tiles);
}
	
/*###########################################################################
 ##                                                                        ##
 ##                        I M A G E   L O A D I N G                       ##
 ##                                                                        ##
 ###########################################################################*/
/*** process_image *********************************************************
 * Loads and process a PNG, generating the tile and palette data that will *
 * be later output in code.                                                *
 ****************************************************************************/
PICDATA *process_image(const char* filename){
	unsigned int err, c;
	unsigned char* image, *png;
	unsigned int width, height;
	int baseColor = 0;
	long rgb;
	PICDATA *result;
	size_t pngsize;
	LodePNGState state;

	lodepng_state_init(&state);

	/* ~~~~~~~~~~~~~~ STEP 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* customize the PNG state to disable conversion and load the image. */
	state.decoder.color_convert = 0;
	lodepng_load_file(&png, &pngsize, filename);
	err = lodepng_decode(&image, &width, &height, &state, png, pngsize);
	free(png);
	if(err) error("ERROR %u: %s\n", err, lodepng_error_text(err));


	/*We can now make use of both state and image data */
	int tColors = state.info_png.color.palettesize;
	/*
	verbose("image size  : %dx%d\n", width, height);
	verbose("color type  : %d\n", state.info_png.color.colortype);
	verbose("color depth : %d\n", state.info_png.color.bitdepth);
	verbose("color count : %d\n", tColors);
	*/

	if (state.info_png.color.colortype != LCT_PALETTE) error ("ERROR: PNG colortype 3 (indexed, 256 colors max) expected!");

	if (tColors > 4 && !globalOpts.grayscale){
		error ("ERROR: PNG has more than 4 colors! Select grayscale conversion (-g) and try again.\n\n");
	}

	/* ~~~~~~~~~~~~~~ STEP 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* palette[] will contain a copy of the image palette but with additional
	info such as the light intensity of  each color. This data will later be
	used to sort the palette or perform grayscale conversion */
	RGB_PALETTE_ENTRY *palette = (RGB_PALETTE_ENTRY *)malloc(tColors*sizeof(RGB_PALETTE_ENTRY));

	/* palette_map[] on the other hand will map any color from the full palette
	to the first 4 entries. It will be initialized first with a 1:1 mapping
	but will be later edited for color sorting and grayscale conversion */
	unsigned char *palette_map = (unsigned char *)malloc(tColors);

	verbose ("\n<ANALYZING COLORS>\n");
	for (c=0; c<tColors; c++){
		set_palette_color(&palette[c],
			(unsigned char)state.info_png.color.palette[c*4], 	/* R */
			(unsigned char)state.info_png.color.palette[c*4+1],	/* G */
			(unsigned char)state.info_png.color.palette[c*4+2]	/* B */
		);
		/* initial 1:1 mapping */
		palette_map[c] = c;
	}

	/* In the case of sprites, we need to make sure that the transparent color
	   is valid. If it's an RGB value we have to find it in the palette */
	if (globalOpts.type == TARGET_SPRITE){
		if (globalOpts.transparent < 0){
			rgb = -(globalOpts.transparent+1);
			globalOpts.transparent = find_palette_color((rgb>>16) & 0xff, (rgb>>8) & 0xff, (rgb) & 0xff, palette, tColors);
			if (globalOpts.transparent < 0){
				verbose("WARNING: RGB Color #%02x%02x%02x Not Found. Defaulting to color 0.\n", (rgb>>16) & 0xff, (rgb>>8) & 0xff, (rgb) & 0xff);
				globalOpts.transparent = 0;
			}
		}else if (globalOpts.transparent >= tColors) {
			verbose("WARNING: The selected transparent color (#%03d) is invalid!\n         The image has %d colors only. Defaulting to Color 0!\n\n", globalOpts.transparent, tColors);
			globalOpts.transparent = 0;
		}
	}

	/* ~~~~~~~~~~~~~~ STEP 3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/*	Having grayscale enabled is the only way an image with a large number
		of colors could have passed the test and reach this point. Whatever
		the case we need to generate a standard grayscale palette AND map
		every color from the full original palette to a grayscale one before
		proceeding. */

	if (globalOpts.grayscale) {
		verbose ("-- Mapping to a grayscale palette.\n");
		/* Map each color from the full palette to 4-shades-of-gray.*/
		for (c=0; c<tColors; c++){
			/* In the case of sprites, we want to make sure that the entry
			that corresponds to "transparent" points to 0.*/
			if (globalOpts.type == TARGET_SPRITE && c == globalOpts.transparent) {
				palette_map[c] = 0;
			}else {
				palette_map[c] = intensity_to_shades(palette[c].L);
			}
		}
		/* Overwrite the existing palette with a GB-compatible one */
		free(palette);
		palette = create_gb_gray_pal();
		tColors = 4;
	}else {
		/*	For sprites, we need to do the equivalent of what we did for
			grayscale, basically moving the "transparent" color to the top of
			the palette, because color 0 is always transparent in the case of
			sprites.
			Also we will set the "baseColor" to 1, so the rest of this block
			win't mess with the first color of the palette. */
		if (globalOpts.type == TARGET_SPRITE) {
			swap_palette_entries (&palette[0], &palette[globalOpts.transparent]);
			swap_palette_indexes(palette_map, 0, globalOpts.transparent, tColors);
			baseColor = 1;
		}
		/*	NOTE: Palette sorting is only needed in non-grayscale mode.
			Our grayscale palette is already sorted */
		if (globalOpts.sort_palette) {	
			verbose ("-- RE-ARRANGING THE PALETTE FROM LIGHT TO DARK\n");
			if (tColors < 4){
				/* Resize the palette to 4 colors */
				palette = (RGB_PALETTE_ENTRY *)realloc (palette, 4*sizeof(RGB_PALETTE_ENTRY));
	
				/* fill the new entries with a dummy color (let's use black) */
				memset (&palette[tColors], 0, sizeof(RGB_PALETTE_ENTRY)*(4-tColors));

				/* Move the existing entries to a position that makes sense
					(sorted from light to dark).*/
				int newPos = 0;
				for (c = baseColor; c < tColors; c++){
					newPos = intensity_to_shades(palette[c].L);
					swap_palette_entries (&palette[c], &palette[newPos]);
					swap_palette_indexes(palette_map, c, newPos, tColors);
				}
				tColors = 4;
				for (c=0; c<tColors; c++){
					verbose("[%02x] --> [%02x] L: %03d\n", c, palette_map[c], palette[palette_map[c]].L);
				}
			}else{
				sort_palette(palette, palette_map, tColors, baseColor);
			}
			for (c = 0; c < tColors; c++){
				verbose("[%02x] --> [%02x] L: %03d\n", c, palette_map[c], palette[palette_map[c]].L);
			}
			verbose("\n");
		}
	}

	/* ~~~~~~~~~~~~~~ STEP 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* if we got to this point we can safely assume we have a palette of no
	more than 4 colors stored in RGBA format AND pixel data in no more than 8
	bits per pixel */
	verbose ("\n<ALLOCATING PICTURE DATA>\n");
	result = allocate_gb_pict (width, height, (is8x16Mode() ? 1 : 0));
	verbose("input tiles: %d (%dx%d map)\n\n", result->rows*result->cols, result->cols, result->rows);

	if (globalOpts.create_palette){
		verbose ("\n<GENERATING OUTPUT PALETTE>\n");
		verbose ("-- Palette Data\n");
		verbose ("  IN  R  G  B      15B\n");
		RGB_PALETTE_ENTRY *pal;
		for (c=0; c<tColors; c++){
			pal = &palette[c];
			set_gb_pal_entry (result, c, pal->r, pal->g, pal->b);
			verbose (" [%02x] %02x %02x %02x --> %04x\n", c, pal->r, pal->g, pal->b, result->pal[c]);
		}
	}

	/* ~~~~~~~~~~~~~~ STEP 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* go over the pixel data */
	unsigned char pdata;
	int x, y, tx, ty, b, pix;
	int ppb = 8/state.info_png.color.bitdepth; /* pixels per byte */
	long tBytes = width*height/ppb;
	int tile_n = 0;

	for (b=0; b<tBytes; b++){
		pdata = image[b];
		for (pix=0; pix<ppb; pix++){
			x = (b*ppb + pix) % width; /* Current pixel in the picture = b*ppb + pix */
			y = (b*ppb + pix) / width;
			tx = x / 8;
			ty = y / result->tileh;
			tile_n = ty*result->cols + tx;
			set_tile_pixel (result, tile_n, x % 8, y % result->tileh, palette_map[pdata>>(8-state.info_png.color.bitdepth)]);
			pdata <<= state.info_png.color.bitdepth;
		}
	}

	if (globalOpts.tile_reduction){
		verbose ("\n<PERFORMING TILE REDUCTION>\n");
		do_tile_reduction(result);
	}

	/* ~~~~~~~~~~~~~~ STEP 6 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	lodepng_state_cleanup(&state);
	free(image);
	free(palette);
	free(palette_map);
	return result;
}

/*###########################################################################
 ##                                                                        ##
 ##                   O U T P U T   G E N E R A T I O N                    ##
 ##                                                                        ##
 ###########################################################################*/
/*** gb_check_warnings ******************************************************
 * Checks that the generated data and selected options are well within the  *
 * limits of the Gameboy. Will adjust values if possible.                   *
 ****************************************************************************/
void gb_check_warnings(PICDATA *gbpic){
	/* Isolating the warnings here will allow me to add "output targets" like
	ASM or other GB toolkits without having to rewrite the GB-specific warnings
	that would apply to all forms of output (like memory limits, sprite-count
	limitations, etc) */
	if (globalOpts.palnumber > 7) {
		printf("\nWARNING: Palette Number can't be > 7. This will be corrected.\n");
		globalOpts.palnumber = 7;
	}

	if (globalOpts.big_sprite && (globalOpts.baseindex & 1)){
		globalOpts.baseindex &= 0xfe;
		printf("\nNOTICE: In 8x16 mode base index must be even. Base will be rounded to %d.\n", globalOpts.baseindex);
	}

	if (globalOpts.test_code && !globalOpts.create_map){
		printf("\nNOTICE: For the test code to work, the tilemap output option has been\n\tactivated despite not being selected.\n");
		globalOpts.create_map = 1;
	}

	if (globalOpts.sort_palette && !globalOpts.create_palette){
		printf("\nNOTICE: Palette sorting is activated but palette output is\n\tdisabled, so it will be enabled now.\n");
		globalOpts.create_map = 1;
	}

	if (globalOpts.test_code){
		if (globalOpts.type == TARGET_BKG || globalOpts.type == TARGET_WINDOW){
			char func_name[4];
			strcpy(func_name, (globalOpts.type == TARGET_BKG ? "bkg": "win"));
			if (gbpic->cols > 32 || gbpic->rows > 32) printf("\nWARNING: The image is more than 32x32 tiles in size.\n\tThe set_%s_tiles() calls will most probably\n\toverflow.\n", func_name);
			if (gbpic->total_tiles + globalOpts.baseindex > 256) printf("\nWARNING: There are more than 256 tiles in %s_dat[]\n\tor the chosen base index is too high. This may\n\tcause problems with set_%s_data().\n", globalOpts.name, func_name);
		} else {
			if (gbpic->total_tiles + globalOpts.baseindex > 40) printf("\nWARNING: There are more than 40 frames in %s_dat[]\n\tor the chosen base index is too high. This may\n\tcause problems with set_sprite_data().\n", globalOpts.name);
			if (gbpic->cols*gbpic->rows > 40 || gbpic->cols > 10) printf("\nWARNING: The picture is more than 40 sprites in size or\n\tmore than 10 sprites wide. The sample code won't\n\tdisplay correctly.\n", globalOpts.name);
		}
	}
}

/*** code_disclaimer_c ******************************************************
 * Outputs the PNGB disclaimer to a file.                                   *
 ****************************************************************************/
void code_disclaimer_c(char *inputfile, char *outputfile, FILE *f){
	time_t t		= time(NULL);
	struct tm tm	= *localtime(&t);

	fputs	("/*********************************************************************\n", f);
	fprintf	(f, " **  <%s>\n",outputfile);
	fputs	(" *********************************************************************\n", f);
	fprintf	(f, " **   Code generated with PNGB v%d.%02d\n", PNGB_VERSION_MAJOR, PNGB_VERSION_MINOR);
	fprintf	(f, " **   %s\n", PNGB_URL);
	fputs	(" **\n", f);
	fprintf	(f, " ** Date:\t%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf	(f, " ** Source:\t%s\n", inputfile);
	fputs	(" *********************************************************************/\n\n", f);
}

/*** gbdk_c_code_output *****************************************************
 * Outputs the GB Picture and palette data according to the selected        *
 * options, in GBDK-compatible C Code.                                      *
 ****************************************************************************/
void gbdk_c_code_output(PICDATA *gbpic, FILE *f){
	int x, y, t;
	unsigned short rowword;
	int tdat = gbpic->cols*gbpic->rows;
	int tattr = (globalOpts.type == TARGET_SPRITE ? gbpic->total_tiles : tdat);
	verbose ("\n<GENERATING CODE>\n");

	sanitize_var_name(globalOpts.name, strlen(globalOpts.name));
	/* ~~~~~~~~~~~~~~ STEP 1 (PRELUDE) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	if (globalOpts.test_code) fprintf (f, "#include <gb/gb.h>\n\n");

	fprintf (f, "#define %s_cols\t%d\n", globalOpts.name, gbpic->cols);
	fprintf (f, "#define %s_rows\t%d\n", globalOpts.name, gbpic->rows);
	fprintf (f, "#define %s_base\t%d\n", globalOpts.name, globalOpts.baseindex);
	fprintf (f, "#define %s_tsize\t%s_cols*%s_rows\n", globalOpts.name, globalOpts.name, globalOpts.name);
	fprintf (f, "#define %s_tiles\t%d\n\n", globalOpts.name, gbpic->total_tiles);

	/* ~~~~~~~~~~~~~~ STEP 2 (PALETTE) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	if (globalOpts.create_palette){
		int c;
		fprintf (f, "const unsigned int %s_pal[] = {", globalOpts.name);
		for (c=0; c<4; c++){
			fprintf (f, " 0x%04x%c", (unsigned short)gbpic->pal[c], (c<3? ',' : ' '));
		}
		fputs ("};\n\n", f);
	}

	/* ~~~~~~~~~~~~~~ STEP 3 (TILES) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	fprintf (f, "const unsigned char %s_dat[] = {\n", globalOpts.name);
	for (t=0; t<gbpic->total_tiles; t++){
		fputs ("\t", f);
		for (y=0;y<gbpic->tileh;y++){
			rowword = get_tile_row(gbpic, t, y);
			fprintf (f, "0x%02x, 0x%02x", (rowword>>8), (rowword & 0xff));
			if (y < gbpic->tileh-1) fputs (", ", f);
		}
		fputs ((t<gbpic->total_tiles-1? ",\n" : "\n};\n\n"), f);
	}
	/* ~~~~~~~~~~~~~~ STEP 3 (TILE/SPRITE ATTRIBUTES) ~~~~~~~~~~~~~~~~~~~~~~*/
	/*	For the most of it, the "attributes" are the palette, which is the
		lowest 3 bits for both sprites and BG/WIN tiles. */
	fprintf (f, "const unsigned char %s_att[] = {", globalOpts.name);
	for (t=0; t < tattr; t++){
		if (t % gbpic->cols == 0) fputs("\n\t", f);
		fprintf (f, "0x%02x", globalOpts.palnumber);
		if (t < tattr-1) fputs (", ", f);
	}
	fputs("\n};\n\n", f);

	/* ~~~~~~~~~~~~~~ STEP 4 (TILE/SPRITE MAP) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	if (globalOpts.create_map){
		fprintf (f, "const unsigned char %s_map[] = {", globalOpts.name);
		for (t=0; t < tdat; t++){
			if (t % gbpic->cols == 0) fputs("\n\t", f);
			fprintf (f, "0x%02x", globalOpts.baseindex+(gbpic->tilemap[t]));
			if (t < tdat-1) fputs (", ", f);
		}
		fputs("\n};\n\n", f);
	}

	/* ~~~~~~~~~~~~~~ STEP 5 (SAMPLE CODE) ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	if (globalOpts.test_code){
		if (globalOpts.type == TARGET_SPRITE){
			/* Aux function for sprites */
			fprintf (f, "\n/* This function sets a sprite tile, attributes (palette) and position. It's just for demo purposes, this is NOT efficient at ALL! */\n");
			fprintf (f, "void set_%s_sprite(unsigned char index, unsigned char tile, unsigned char attr, unsigned char x, unsigned char y) {\n", globalOpts.name);
			fprintf (f, "\tif (index >= 40) return;\n");
			fprintf (f, "\tset_sprite_tile (index, tile);\n");
			fprintf (f, "\tset_sprite_prop (index, attr);\n");
			fprintf (f, "\tmove_sprite (index, x, y);\n");
			fprintf (f, "}\n");
		}

		fprintf (f, "\n\nint main(void) {\n");
		if (globalOpts.type == TARGET_BKG || globalOpts.type == TARGET_WINDOW){
			char func_name[4];
			int dx = (globalOpts.type == TARGET_BKG ? -(160-gbpic->w)/2 : (160-gbpic->w)/2 + 7);
			int dy = (globalOpts.type == TARGET_BKG ? -(144-gbpic->h)/2 : (144-gbpic->h)/2 );
			strcpy(func_name, (globalOpts.type == TARGET_BKG ? "bkg": "win"));
			if (globalOpts.create_palette){
				fprintf (f, "\tset_bkg_palette(%d, 1, %s_pal);\n", globalOpts.palnumber, globalOpts.name);
			}
			fprintf (f, "\tset_%s_data(0x%02x, %s_tiles, %s_dat);\n", func_name, globalOpts.baseindex, globalOpts.name, globalOpts.name);
			fprintf (f, "\tVBK_REG = 1;\n");
			fprintf (f, "\tset_%s_tiles(0, 0, %s_cols, %s_rows, %s_att);\n", func_name, globalOpts.name, globalOpts.name, globalOpts.name);
			fprintf (f, "\tVBK_REG = 0;\n");
			fprintf (f, "\tset_%s_tiles(0, 0, %s_cols, %s_rows, %s_map);\n", func_name, globalOpts.name, globalOpts.name, globalOpts.name);
			fprintf (f, "\tmove_%s (%d, %d);\n", func_name, dx, dy);
			fprintf (f, "\n\tSHOW_%s;\n", (globalOpts.type == TARGET_BKG ? "BKG" : "WIN"));
		}else{
			int dx = (160-gbpic->w)/2 + 8;
			int dy = (144-gbpic->h)/2 + 16;
			fprintf (f, "\tunsigned char x, y, xt, yt, i=0;\n");
			if (globalOpts.big_sprite) fprintf (f, "\tSPRITES_8x16;\n");
			if (globalOpts.create_palette){
				fprintf (f, "\tset_sprite_palette(%d, 1, %s_pal);\n", globalOpts.palnumber, globalOpts.name);
			}
			fprintf (f, "\tset_sprite_data(0x%02x, %s_tiles%s, %s_dat);\n", globalOpts.baseindex, globalOpts.name, (globalOpts.big_sprite? "*2" : ""), globalOpts.name);
			fprintf (f, "\tVBK_REG = 0;\n\n");
			fprintf (f, "\tfor(y=0; y< %s_rows; y++){\n", globalOpts.name);
			fprintf (f, "\t\tyt=y*%dU;\n", gbpic->tileh);
			fprintf (f, "\t\tfor(x=0; x < %s_cols; x++){\n", globalOpts.name);
			fprintf (f, "\t\t\txt=x*8;\n");
			fprintf (f, "\t\t\tif (i >= %s_tsize) break;\n", globalOpts.name);
			fprintf (f, "\t\t\tset_%s_sprite (i, %s_map[i]%s, %s_att[%s_map[i]-%s_base], xt+%dU, yt+%dU);\n", globalOpts.name, globalOpts.name, (is8x16Mode() ? "*2" : ""), globalOpts.name, globalOpts.name, globalOpts.name, dx, dy);
			fprintf (f, "\t\t\ti++;\n");
			fprintf (f, "\t\t}\n", globalOpts.name);
			fprintf (f, "\t}\n", globalOpts.name);
			fprintf (f, "\n\tSHOW_SPRITES;\n");
		}

		fprintf (f, "\tenable_interrupts();\n");
		fprintf (f, "\tDISPLAY_ON;\n");
		fputs ("\n\treturn 0;\n}\n", f);
	}
	verbose ("-- Done\n\n");
}
	
