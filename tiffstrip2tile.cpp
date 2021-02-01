#include <tiffio.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

unsigned int tilewidth = 256;
unsigned int tileheight = 256;

/* 
 cl /Ox /nologo /I../../src/zlib123 /I../../src/tiff-3.8.2\libtiff convtiffone.c /link ../../src/tiff-3.8.2\libtiff\libtiff.lib ../../src/zlib123\zlib.lib ../../src/jpeg-6b\libjpeg.lib
*/

int main(int argc, char* argv[]) {
    TIFF* tifin = TIFFOpen(argv[1], "r");
    TIFF* tifout = TIFFOpen(argv[2], "w");
    unsigned int imagewidth;
    unsigned int imageheight;
    unsigned int rows_per_strip;

    unsigned int numtilesx;
    unsigned int numtilesy;

    unsigned int full_tile_width;
    
    unsigned char* full_tile_data;
    unsigned char* tile_data;

    unsigned int col, row;
    unsigned int line;
    unsigned int current_line;

    TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
    TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
    TIFFGetField(tifin, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);

    TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, imagewidth);
    TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, imageheight);

    TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 3);

 //   TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE); 
  //  TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, Z_BEST_COMPRESSION);

  TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);

    TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

    numtilesx = imagewidth / tilewidth;
    if (imagewidth % tilewidth)
	++numtilesx;
    full_tile_width = numtilesx * tilewidth;

    numtilesy = imageheight / tileheight;
    if (imageheight % tileheight)
	++numtilesy;

    fprintf(stderr, "final size : %dx%d\n", imagewidth, imageheight);
    fprintf(stderr, "num tiles : %dx%d\n", numtilesx, numtilesy);

    fprintf(stderr, "rows_per_strip : %d\n", rows_per_strip);

    full_tile_data = (unsigned char*)malloc(tileheight * full_tile_width * 4);
    memset(full_tile_data, 255, tileheight * full_tile_width * 4);
    tile_data = (unsigned char*)malloc(tilewidth * tileheight * 3);
    memset(tile_data, 0, tilewidth * tileheight * 3);

    current_line = 0;
    for (row = 0; row < numtilesy; ++row) {
	unsigned char* currentpos = full_tile_data;
	for (line = 0; line < tileheight; ++line, ++current_line) {
	    if (current_line < imageheight) {
		TIFFReadRGBAStrip(tifin, current_line, (uint32*)currentpos);
		currentpos += full_tile_width * 4;
	    } 
	}
	for (col = 0; col < numtilesx; ++col) {
	    // now copy
	    unsigned char* startx = full_tile_data + col * tilewidth * 4;
	    unsigned int offset = full_tile_width * 4;
	    unsigned int y;
	    for (y = 0; y < tileheight; ++y) {
		unsigned int x;
		for (x = 0; x < tilewidth; ++x) {
		    memcpy(tile_data + 3 * tilewidth * y + x * 3,
			   startx + offset * y + x * 4,
			   3);
		}
	    }
	    // write the tile
	    TIFFWriteEncodedTile(tifout, 
				 TIFFComputeTile(tifout, 
						 col * tilewidth, 
						 row * tileheight, 
						 0, 0), 
				 tile_data, 3 * tilewidth * tileheight);
	}
	fprintf(stderr, "saving tiles (row %d/%d)\n", row, numtilesy);
    }
    TIFFClose(tifout);
    TIFFClose(tifin);
    free(full_tile_data);
    free(tile_data);
  
}
