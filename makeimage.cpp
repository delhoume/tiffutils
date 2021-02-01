#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

extern "C" {
#include <jpeglib.h>
#include <tiffio.h>
#include <zlib.h>
}

/* 
 cl /c /EHsc /DXMD_H /Ox /nologo /Ijpeg-6b /Izlib-1.2.11 /Itiff-3.8.2\libtiff makeimage.cpp 
 link makeimage.obj /out:makeimage.exe jpeg-6b/libjpeg.lib tiff-3.8.2\libtiff\*.obj zlib-1.2.11\zlib.lib /nodefaultlib:libcmt.lib

 
  cl /c /EHsc /DXMD_H /Ox /nologo /I../../src/jpeg-6bx /I../../src/zlib123 /I../../src/tiff-4b\libtiff makeimage.cpp 
 link makeimage.obj /out:makeimage.exe ../../src/jpeg-6bx/libjpeg.lib ../../src/tiff-4b\libtiff\libtiff.lib ../../src/zlib123\zlib.lib /nodefaultlib:libc.lib
*/

// makeimage yosemite.00.tif 0 340 13 145
// makeimage zion.00.tif 53 203 18 110
// makeimage boston.00.tif 15 311 0 163
// makeimage owens.00.tif 0 128 0 64
// makeimage elcapitan.00.tif 67 189 26 102
// makeimage.exe zion_ir.00.tif 0 78 7 32
// makeimage.exe chaco.00.tif 27 229 0 128
// makeimage.exe hawai.00.tif 52 204 36 93
// makeimage.exe oahu.00.tif 28 228 31 97
// makeimage.exe tahoe.00.tif 3 253 12 116
// makeimage.exe tahoe2.00.tif 8 120 5 59
// makeimage.exe yosemite_glacier.00.tif 58 198 14 113
// makeimage.exe macchupichu.00.tif 0 232 1 101
// ??
// makeimage.exe harlem.00.tif 0 1092 0 181 

// makeimage wtc.tif 1 256 1 64

//#define WTC

int main(int argc, char* argv[]) {
    int tilewidth = 512;
    int tileheight = 512;
    
    int startcol = atoi(argv[2]);
    int endcol = atoi(argv[3]);
    int startrow = atoi(argv[4]);
    int endrow = atoi(argv[5]);
    
    int numcols = endcol - startcol;
    int numrows = endrow - startrow;

	// can be customized when image is smaller because of tiling
    int imagewidth = numcols * tilewidth;
    int imageheight = numrows * tileheight;

    fprintf(stderr, "Input tiles: %d %d %d %d\n", startcol, endcol, startrow, endrow);
    fprintf(stderr, "Image width: %d Image height: %d\n", imagewidth, imageheight);
    fprintf(stderr, "Num rows: %d Num Cols: %d\n", numrows, numcols);

    TIFF* tifout = TIFFOpen(argv[1], "wb");
    FILE* jpegin = 0;
    TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, imagewidth);
    TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, imageheight);

    TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 3);

    TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_JPEG); 

	int compression = 80;
	if (argc == 7)
	   compression = atoi(argv[6]);
	fprintf(stderr, "JPEG Quality %d\n", compression);

    TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, compression); 
    TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

    int ntilesinimage = TIFFNumberOfTiles(tifout);
    fprintf(stderr, "ntilesinimage %d\n", ntilesinimage);

    unsigned char* imagedata = (unsigned char*)malloc(tileheight * tilewidth * 3);

    for (int y = 0; y < numrows; ++y) {
	fprintf(stderr, "\nsaving row %04d/%04d ->", y, numrows - 1);
	int row = startrow + y;
	//	fprintf(stderr, "\nreading row %03d\n", row);
	for (int x = 0; x < numcols; ++x) {
	    int col = startcol + x;
	    char buffer[100];
#if defined(WTC)
		if (x < 63)
			sprintf(buffer, "WTC_b\\Tiles\\%d_%d.jpg", row, (x % 63) + 1);
		else if (x < 126)
			sprintf(buffer, "WTC_l\\Tiles\\%d_%d.jpg", row, (x % 63) + 1);
		else if (x < 189)
			sprintf(buffer, "WTC_f\\Tiles\\%d_%d.jpg", row, (x % 63) + 1);
		else
			sprintf(buffer, "WTC_r\\Tiles\\%d_%d.jpg", row, (x % 63) + 1);
#else
		sprintf(buffer, "Tiles\\%04d_%04d.jpg", row, col); // order is row col for girl_with_pearl, others are reversed
#endif		
//		fprintf(stderr, "->> %s\n", buffer);;
	    memset(imagedata, 0, tileheight * tilewidth * 3);
	    jpegin = fopen(buffer, "rb");
	    if (!jpegin) {
			sprintf(buffer, "Tiles\\%03d_%d_%d.jpg", row, row, col); // order is row col for girl_with_pearl, others are reversed
//			fprintf(stderr, "--->> %s\n", buffer);;
			jpegin = fopen(buffer, "rb");
	    }
	    if (jpegin) {
			fprintf(stderr, "X");
			struct jpeg_decompress_struct cinfo;
			struct jpeg_error_mgr jerr;
			cinfo.err = jpeg_std_error(&jerr);
			jpeg_create_decompress(&cinfo);
			jpeg_stdio_src(&cinfo, jpegin);
			jpeg_read_header(&cinfo, TRUE);
			cinfo.out_color_space = JCS_RGB;
			jpeg_calc_output_dimensions(&cinfo);
			unsigned int imwidth = cinfo.output_width;
			unsigned int imheight = cinfo.output_height;
			jpeg_start_decompress(&cinfo);
			unsigned char* pos = imagedata;
			for (int idx = 0; idx < imheight; ++idx) {
				jpeg_read_scanlines(&cinfo, &pos, 1);
				 pos += 3 * tilewidth;
			}
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			fclose(jpegin);
	    } else {
			fprintf(stderr, "o");
	    }
	    int tilenum;
	    tilenum = TIFFComputeTile(tifout, x * tilewidth, y * tileheight, 0, 0); 
	    TIFFWriteEncodedTile(tifout, tilenum, imagedata, tilewidth * tileheight * 3);
	}
    }
    TIFFClose(tifout);
    free(imagedata);
}
