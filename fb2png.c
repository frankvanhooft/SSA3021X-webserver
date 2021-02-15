//
// Converts an incoming file of RGB565 from a framebuffer, into an output PNG file
// so that it can be displayed by an image viewer, web browser, etc.
// Example: get the framebuffer file by: cp /dev/fb0 fbdump
//
// Uses lodepng for the PNG file creation.
//
// Quick build:
// gcc -Wall -O2 -DLODEPNG_NO_COMPILE_DECODER -DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS 
// -DLODEPNG_NO_COMPILE_ERROR_TEXT fb2png.c lodepng.c -o fb2png
//
// (c) Frank Van Hooft 2021
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lodepng.h"

#define MAX_PALETTE_SIZE 256        // 8 bit palette


void usage(void)
{
    printf("\n FrameBuffer RGB565 to PNG file Converter.\n");
    printf(" Usage: fb2png width height input-filename output-filename\n");
    printf(" Example: fb2png 1024 600 fbdump screen.png\n\n");
}

//
// Check the input file size - if it's too small then error message & exit
void check_input_filesize(FILE* in_fptr, int datasize)
{
    fseek(in_fptr, 0L, SEEK_END);
    int filesize = ftell(in_fptr);
    fseek(in_fptr, 0L, SEEK_SET);
    if (filesize < datasize)
    {
        printf("\n ERROR - input filesize is too small for stated image size.\n");
        fclose(in_fptr);
        usage();
        exit(-1);
    }
}

//
// Try to open & close the output (PNG) file, just as a sanity-check.
// That way if we're in a read-only directory or something, the user will get
// a meaningful error message.
void check_output_file_can_be_written(char* filename)
{
    FILE* out_fptr;

    out_fptr = fopen(filename, "w");
    if (out_fptr == NULL)
    {
        printf("\n ERROR - cannot create output file. Check permissions?\n");
        usage();
        exit(-1);
    }
    fclose(out_fptr);
}

//
// Reads through the input file and creates an array of unique colours in the RGB565 source image file.
// Returns the number of unique colours in the array. 
int create_unique_colors_array(FILE *inptr, int16_t* colour_array, int num_pixels)
{
    int16_t datain;
    int i, numread;
    int num_colours = 0;

    fseek(inptr, 0L, SEEK_SET);                     // ensure we're at start of file
    while ((num_pixels--) && (num_colours < MAX_PALETTE_SIZE))
    {   
        // Read an RGB565 input pixel colour. If it's not in the array then add it
        numread = fread(&datain, 2, 1, inptr);     // read two bytes of RGB565 data
        if (numread == 0)
            exit (-1);

        for (i = 0; i < num_colours; i++)
            if (datain == colour_array[i])
                break;                              // exit for() loop if colour already in array

        // If datain pixel colour already in the array, i will be less than num_colours
        // If datain pixel colour not in the array, i will have incremented to equal num_colours
        if (i == num_colours)
            colour_array[num_colours++] = datain;   // store this new colour in the array
    }

    return num_colours;
}

//
// Loads the palette colors into the PNG palette. By reading the red, green, blue
// components of the RGB565 colours in the "unique colors" array. Alpha always 255.
void load_png_palettes(LodePNGState* state, int16_t* color_array, int num_colors)
{
    int i;
    unsigned char red, grn, blu;
    unsigned err;

    for (i = 0; i < num_colors; i++)
    {
        red = (unsigned char)((color_array[i] & 0xf800) >> 8);
        grn = (unsigned char)((color_array[i] & 0x07e0) >> 3);
        blu = (unsigned char)((color_array[i] & 0x001f) << 3);
        err = lodepng_palette_add(&state->info_png.color, red, grn, blu, 255);
        if (err)
            printf(" ERROR loadpng_palette_add 1, code %d\n", err);
        err = lodepng_palette_add(&state->info_raw, red, grn, blu, 255);
        if (err)
            printf(" ERROR loadpng_palette_add 2, code %d\n", err);
    }

    // tell nodepng about the color palettes being used
    state->info_png.color.colortype = LCT_PALETTE;
    state->info_png.color.bitdepth = 8;
    state->info_raw.colortype = LCT_PALETTE;
    state->info_raw.bitdepth = 8;
    state->encoder.auto_convert = 0;    // not auto, we've specified the color mode
}

//
// Fill a PNG "raw image" array by reading through the framebuffer RGB565 pixel data, and for
// each pixel matching it to an entry in the "unique colors" array. The resulting "raw image" array
// is a list of 8-bit indexes into the unique colors array. Which is the same as a list of indexes
// into the PNG palette.
void fill_raw_image_array(FILE *inptr, int16_t* colour_array, unsigned char* rawimage_array, 
                            int num_pixels, int num_colours)
{
    int i, numread;
    int16_t datain;

    fseek(inptr, 0L, SEEK_SET);                         // ensure we're at start of file
    while (num_pixels--)
    {
        numread = fread(&datain, 2, 1, inptr);          // read RGB565 pixel
        if (numread == 0)
            exit (-1);
        
        for (i = 0; i < num_colours; i++)               // match pixel to colour in the color_array
            if (datain == colour_array[i])
                break;                                  // exit for() loop when pixel color found

        // safety check - if pixel colour not found in the array then i == num_colours. In which case
        // set i to 0, so this "unknown colour" pixel is allocated to the first colour in the array.
        if (i == num_colours) 
            i = 0;

        *rawimage_array++ = i;
    }
}


int main(int argc, char** argv)
{
    FILE *in_fptr;
    unsigned char* raw_image;
    int16_t* color_array;
    int num_colors;
    unsigned width, height, lodepngerror;
    LodePNGState png_state;
    size_t pngsize;
    unsigned char* png;


    if (argc != 5)
    {
        usage();
        exit (-1);
    }

    width  = atoi(argv[1]);
    height = atoi(argv[2]);

    check_output_file_can_be_written(argv[4]);

    // Open the input (raw framebuffer) file
    in_fptr = fopen(argv[3], "rb");
    if (in_fptr == NULL)
    {
        printf("\n ERROR - input file does not exist.\n");
        usage();
        exit(-1);
    }

    // Check the input file size - RGB565 data means 2 bytes per pixel
    check_input_filesize(in_fptr, width * height * 2);
   
    // Allocate space for the unique colors array, then fill it
    color_array = (int16_t*)malloc(MAX_PALETTE_SIZE * 2);
    if (color_array == NULL)
    {
        printf("/n ERROR - unable to allocate space for color array.\n");
        fclose(in_fptr);
        exit(-1);
    }
    num_colors = create_unique_colors_array(in_fptr, color_array, width * height);

    // Initialize the png_state structure, then load it with the palettes & color information
    lodepng_state_init(&png_state);
    load_png_palettes(&png_state, color_array, num_colors);

    // Create a "raw image" array, which is the input file where each pixel is converted into
    // an index into the palette. We have an 8-bit palette, hence the raw_image array size
    // is simply width * height. Fill the array.
    raw_image = (unsigned char *)malloc(width * height);
    if (raw_image == NULL)
    {
        printf("/n ERROR - unable to allocate space for buffer.\n");
        fclose(in_fptr);
        free(color_array);
        exit(-1);
    }
    fill_raw_image_array(in_fptr, color_array, raw_image, width * height, num_colors);

    // Encode the raw image into a PNG in memory, then save PNG to file
    lodepngerror = lodepng_encode(&png, &pngsize, raw_image, width, height, &png_state);
    if (lodepngerror)
    {
        printf("\n ERROR - unable to create PNG file - code %d\n", lodepngerror);
        free(raw_image);
        free(color_array);
        fclose(in_fptr);
        exit(-1);
    }

    lodepngerror = lodepng_save_file(png, pngsize, argv[4]);
    if (lodepngerror)
    {
        printf("\n ERROR - unable to write PNG file - code %d\n", lodepngerror);
        free(raw_image);
        free(color_array);
        fclose(in_fptr);
        exit(-1);
    }
       
    // clean up
    lodepng_state_cleanup(&png_state);
    free(color_array);
    free(raw_image);
    free(png);
    fclose(in_fptr);
    return 0;	
}
