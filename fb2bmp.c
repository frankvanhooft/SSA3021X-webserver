//
// Converts an incoming file of RGB565 from a framebuffer, into an output file
// of an RGB555 BMP file, so that it can be displayed by an image viewer, web browser, etc.
// Example: get the framebuffer file by: cp /dev/fb0 fbdump
//
// See wikipedia for BMP file format description
//
// (c) Frank Van Hooft 2021
// License: GPL
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define BMP_HEADER_SIZE     14
#define DIB_HEADER_SIZE     40


void usage(void)
{
	printf("\n FrameBuffer RGB565 to BMP RGB555 Converter.\n");
	printf(" Usage: fb2bmp width height input-filename output-filename\n");
	printf(" Example: fb2bmp 1024 600 fbdump screen.bmp\n\n");
}

// 
// Outputs the BMP header, which is the first of the two headers
void write_bmp_header(FILE *fp, int datasize)
{
    const unsigned char firstpart[] = {0x42, 0x4d};
    const unsigned char secondpart[] = {0,0,0,0,0x36,0,0,0};
    int bmpfilesize = BMP_HEADER_SIZE + DIB_HEADER_SIZE + datasize;
    unsigned char c;

    fwrite(&firstpart, 1, sizeof(firstpart), fp);
    
    // write the bmpfilesize, 4 bytes, least-significant byte first
    c = bmpfilesize & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (bmpfilesize >> 8) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (bmpfilesize >> 16) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (bmpfilesize >> 24) & 0xff;
    fwrite(&c, 1, 1, fp);

    fwrite(&secondpart, 1, sizeof(secondpart), fp);
}

// 
// Outputs the DIB header, which is the second of the two headers
void write_dib_header(FILE *fp, int width, int height)
{
    const unsigned char partone[] = {1,0,16,0,0,0,0,0};
    const unsigned char parttwo[] = {0x13,0x0b,0,0,0x13,0x0b,0,0,0,0,0,0,0,0,0,0};
    unsigned char c;

    // first output the header size, 32 bits, least significant byte first
    c = DIB_HEADER_SIZE;
    fwrite(&c, 1, 1, fp);
    c = 0;
    fwrite(&c, 1, 1, fp);
    fwrite(&c, 1, 1, fp);
    fwrite(&c, 1, 1, fp);

    // then the image width, least significant byte first
    c = width & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (width >> 8) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (width >> 16) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = (width >> 24) & 0xff;
    fwrite(&c, 1, 1, fp);

    // then the image height, as a negative number (so it displays right-way up), LSB first
    c = (-1 * height) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((-1 * height) >> 8) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((-1 * height) >> 16) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((-1 * height) >> 24) & 0xff;
    fwrite(&c, 1, 1, fp);

    fwrite(&partone, 1, sizeof(partone), fp);

    // raw bitmap datasize = width * height * 2, LSB first
    c = (width * height * 2) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((width * height * 2) >> 8) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((width * height * 2) >> 16) & 0xff;
    fwrite(&c, 1, 1, fp);
    c = ((width * height * 2) >> 24) & 0xff;
    fwrite(&c, 1, 1, fp);

    fwrite(&parttwo, 1, sizeof(parttwo), fp);
}

//
// Writes the output data, by reading 2 bytes from the input file (RGB565),
// converts them to 2 bytes of RGB555, writes them to the output file,
// and repeats for the required number of pixels.
void write_image_data(FILE *inptr, FILE *outptr, int num_pixels)
{
    int16_t datain, dataout;

    while (num_pixels--)
    {   
        fread(&datain, 2, 1, inptr);            // format is RGB565
        dataout = 0;
        dataout |= ((datain & 0xf800) >> 1);    // red    
        dataout |= ((datain & 0x07c0) >> 1);    // green
        dataout |= (datain & 0x001f);           // blue
        fwrite(&dataout, 2, 1, outptr);         // write RGB555
    }
}


int main(int argc, char** argv)
{
    int width, height, filesize, datasize;
    FILE *in_fptr, *out_fptr;


	if (argc != 5)
	{
		usage();
		exit (-1);
	}

    width  = atoi(argv[1]);
    height = atoi(argv[2]);

    // Open the input (raw framebuffer) file
    in_fptr = fopen(argv[3], "rb");
    if (in_fptr == NULL)
    {
        printf("\n ERROR - input file does not exist.\n");
        usage();
        exit(-1);
    }

    // Check the input file size - if it's too small then error out
    fseek(in_fptr, 0L, SEEK_END);
    filesize = ftell(in_fptr);
    datasize = width * height * 2;
    fseek(in_fptr, 0L, SEEK_SET);
    if (filesize < datasize)
    {
        printf("\n ERROR - input filesize is too small.");
        fclose(in_fptr);
        usage();
        exit(-1);
    }
   
    // Open the output (BMP) file
    out_fptr = fopen(argv[4], "wb");
    if (out_fptr == NULL)
    {
        printf("\n ERROR - cannot write output file.\n");
        fclose(in_fptr);
        usage();
        exit(-1);
    }

    // Output the BMP headers to the output file
    write_bmp_header(out_fptr, datasize);    
    write_dib_header(out_fptr, width, height);

    // write the converted image data to the output file
    write_image_data(in_fptr, out_fptr, width * height);

    // wrap up
    fclose(in_fptr);
    fclose(out_fptr);
	return 0;	
}
