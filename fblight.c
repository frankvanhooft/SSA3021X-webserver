//
// Takes an RGB565 format image file, assumed to be from a framebuffer, and "lightens" it, by
// flipping the brightness of any grey pixels it finds. So back becomes white, white becomes
// black, and intermediate shades of grey will lighten or darken. In addition, it will darken
// slightly any light colours it finds (so they contrast better against the now light background).
//
// Example: get the framebuffer file by: cp /dev/fb0 fbdump
//
// Quick build:
// gcc -Wall -O2 fblight.c -o fblight
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

#define GREY_SIMILARITY_THRESHOLD   5


void usage(void)
{
    printf("\n Lightens dark background of RGB565 image.\n");
    printf(" Usage: fblight numpixels filename_in filename_out\n");
    printf(" Example: fblight 614400 fbdump fbdump_light\n\n");
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
// This "corrects" the passed-in RGB pixel data. Each value occupies the low 6 bits of the int.
// If the pixel is a shade of grey, it changes the brightness. Otherwise it leaves it alone.
void correct_rgb_pixel(int *red, int *grn, int *blu)
{
    // check to see if this pixel is a grey value
    if ((abs(*grn - *red) < GREY_SIMILARITY_THRESHOLD)
        && (abs(*grn - *blu) < GREY_SIMILARITY_THRESHOLD))
    {
        // pixel is a shade of grey, so make a new shade of grey, of "inverted" brightness
        *grn = 0x3f - *grn;
        *red = *grn;
        *blu = *grn;
    }
    
    else
    {
        // Check if the pixel is yellow. If so, make it darker. 
        if ((*red > 0x20) && (*grn > 0x20) && (*blu < 0x08))
        {
            *red = *red >> 1;
            *grn = *grn >> 1;
            *blu = 0;
        }
    }
}

//
// Works through all the RGB565 pixels in the file, changing them if required,
// and writing to the output file.
void process_file(FILE* in_fp, FILE* out_fp, int numpixels)
{
    int16_t pixdata;
    int numread, red, grn, blu;

    fseek(in_fp, 0L, SEEK_SET);
    fseek(out_fp, 0L, SEEK_SET);                        
    while (numpixels--)
    {
        numread = fread(&pixdata, 2, 1, in_fp);     // read two bytes of RGB565 data
        if (numread == 0)
            exit (-1);

        red = (int)((pixdata & 0xf800) >> 10);      // extract R-G-B from RGB565
        grn = (int)((pixdata & 0x07e0) >> 5);
        blu = (int)((pixdata & 0x001f) << 1);

        correct_rgb_pixel(&red, &grn, &blu);        // adjust the R-G-B colour

        pixdata  = ((int16_t)(red & 0x3e)) << 10;
        pixdata |= (((int16_t)grn) << 5);
        pixdata |= (((int16_t)blu) >> 1);           // convert R-G-B back to RGB565

        fwrite(&pixdata, 2, 1, out_fp);             // write the new RGB565 pixel value
    }
}


int main(int argc, char** argv)
{
    FILE *in_fp, *out_fp;
    int numpixels;

    if (argc != 4)
    {
        usage();
        exit (-1);
    }

    numpixels  = atoi(argv[1]);

    // Open the input file
    in_fp = fopen(argv[2], "rb");
    if (in_fp == NULL)
    {
        printf("\n ERROR - input file does not exist.\n");
        usage();
        exit(-1);
    }

    // Open the output file
    out_fp = fopen(argv[3], "wb");
    if (out_fp == NULL)
    {
        printf("\n ERROR - Cannot write output file. Check permissions?\n");
        usage();
        fclose(in_fp);
        exit(-1);
    }

    check_input_filesize(in_fp, numpixels * 2);    // RGB565 means 2 bytes per pixel
    process_file(in_fp, out_fp, numpixels);        // process all the pixels in the file

    // cleanup
    fclose(in_fp);
    fclose(out_fp);
    return 0;
}

