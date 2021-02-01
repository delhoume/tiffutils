#include <iostream>
#include <fstream>

using namespace std;

extern "C" {
#include <tiffio.h>
#include <omp.h>
}


/* 
cl /EHsc /O2 /nologo /I../../src/zlib123 /I../../src/tiff-4b\libtiff halftiff.cpp ../../src/tiff-4b\libtiff\libtiff.lib ../../src/zlib123\zlib.lib ../../src/jpeg-6b\libjpeg.lib
*/

static void HalfNormal(unsigned char* dst, unsigned char* src, unsigned int srcsize, 
		       unsigned int bpp, int offset) {
    unsigned char* dstptr;
    unsigned char* srcptr;
    unsigned int dstsize = srcsize / 2;
    for (unsigned int y = 0; y < dstsize; ++y) {
	srcptr = src + y * srcsize * 2 * 3;
	dstptr = dst + y * offset * 3;
	for (unsigned int x = 0; x < dstsize; ++x) {
	    for (unsigned int bpp = 0; bpp < 3; ++bpp) {
		unsigned int val = srcptr[x * 3 * 2 + bpp] + 
		    srcptr[x * 3 * 2 + 3 + bpp] + 
		    srcptr[3 * srcsize + x * 3 * 2 + bpp] + 
		    srcptr[3 * srcsize + x * 3 * 2 + 3 + bpp];
		dstptr[x * 3 + bpp] = (unsigned char)(val / 4);
	    }
	} 
    }
}

static void HalfNormalBPP3(unsigned char* dst, unsigned char* src, unsigned int srcsize, 
			   unsigned int bpp, int offset) {
    unsigned char* dstptr;
    unsigned char* srcptr;
    unsigned int dstsize = srcsize / 2;
    for (unsigned int y = 0; y < dstsize; ++y) {
	srcptr = src + y * srcsize * 2 * 3;
	dstptr = dst + y * offset * 3;
	for (unsigned int x = 0; x < dstsize; ++x) {
	    unsigned int val = srcptr[x * 3 * 2] + 
		srcptr[x * 3 * 2 + 3] + 
		srcptr[3 * srcsize + x * 3 * 2] + 
		srcptr[3 * srcsize + x * 3 * 2 + 3];
	    dstptr[x * 3] = (unsigned char)(val / 4);
	    val = srcptr[x * 3 * 2 + 1] + 
		srcptr[x * 3 * 2 + 3 + 1] + 
		srcptr[3 * srcsize + x * 3 * 2 + 1] + 
		srcptr[3 * srcsize + x * 3 * 2 + 3 + 1];
	    dstptr[x * 3 + 1] = (unsigned char)(val / 4);
	    val = srcptr[x * 3 * 2 + 2] + 
		srcptr[x * 3 * 2 + 3 + 2] + 
		srcptr[3 * srcsize + x * 3 * 2 + 2] + 
		srcptr[3 * srcsize + x * 3 * 2 + 3 + 2];
	    dstptr[x * 3 + 2] = (unsigned char)(val / 4);
	} 
    }
}

static void HalfNormalBPP1(unsigned char* dst, unsigned char* src, unsigned int srcsize, 
			   unsigned int bpp, int offset) {
    unsigned char* dstptr;
    unsigned char* srcptr;
    unsigned int dstsize = srcsize / 2;
    for (unsigned int y = 0; y < dstsize; ++y) {
	srcptr = src + y * srcsize * 2;
	dstptr = dst + y * offset;
	for (unsigned int x = 0; x < dstsize; ++x) {
	    unsigned int val = srcptr[x * 2] + 
		srcptr[x * 2 + 1] + 
		srcptr[srcsize + x * 2] + 
		srcptr[srcsize + x * 2 + 1];
	    dstptr[x] = (unsigned char)(val / 4);
	} 
    }
}

static const unsigned int COLSIZE = 256;
unsigned short r[COLSIZE];
unsigned short g[COLSIZE];
unsigned short b[COLSIZE];
#if defined(COLORMAP)
#define Half HalfNormalBPP1
#else
#define Half HalfNormalBPP3
#endif

int main(int argc, char* argv[]) {
    TIFF* tifin = TIFFOpen(argv[1], "rb");

    unsigned int imagewidth = 0;
    unsigned int imageheight = 0;
    unsigned int tilewidth = 0;
    unsigned int tileheight = 0;
    unsigned int bpp = 0;
	unsigned int bps = 0;

    TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
    TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
    TIFFGetField(tifin, TIFFTAG_TILEWIDTH, &tilewidth);
    TIFFGetField(tifin, TIFFTAG_TILELENGTH, &tileheight);
   	TIFFGetField(tifin, TIFFTAG_BITSPERSAMPLE, &bps);

    cout << "original size:" << imagewidth << "x" << imageheight << endl;
    if (!TIFFIsTiled(tifin)) {
	cout << "image is not tiled, please convert using strips2tiled" << endl;
	return 1;
    }
    cout << "Tile size: " << tilewidth << "x" << tileheight << endl;
    unsigned int numtilesx = imagewidth / tilewidth;
    if (imagewidth % tilewidth)
	++numtilesx;

    unsigned int numtilesy = imageheight / tileheight;
    if (imageheight % tileheight)
	++numtilesy;

    unsigned int newwidth = imagewidth / 2;
    unsigned int newheight = imageheight / 2;
    unsigned int newtilesx = newwidth / tilewidth;
    if (newwidth % tilewidth)
	++newtilesx;

    unsigned int newtilesy = newheight / tileheight;
    if (newheight % tileheight)
	++newtilesy;

cout << "bits per sample: " << bps << endl;
    cout << "original tiles: " << numtilesx << "x" << numtilesy << endl;
    cout << "new size: " << newwidth << "x" << newheight << endl;
    cout << "new tiles: " << newtilesx << "x" << newtilesy << endl;

    TIFF* tifout = TIFFOpen(argv[2], "wb");
    TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, newwidth);
    TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, newheight);

    TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    unsigned int photometric = 0;
    TIFFGetField(tifin, TIFFTAG_PHOTOMETRIC, &photometric);
    switch (photometric) {
    case PHOTOMETRIC_PALETTE:
	for (unsigned int idx = 0; idx < COLSIZE; ++idx) {
	    r[idx] = 256 * idx + idx;
	    g[idx] = 256 * idx + idx;
	    b[idx] = 256 * idx + idx;
	}
	TIFFSetField(tifout, TIFFTAG_COLORMAP, r, g, b);
	bpp = 1;
	break;
    case PHOTOMETRIC_RGB:
	bpp = 3;
	cout << "RGB image source" << endl;
	break;
	default:
		cout << bpp << " image source" << endl;
		bpp = 3; // defaults to RGB
		photometric = PHOTOMETRIC_RGB;
    }
TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, bpp);

    unsigned int compression = 0;
    unsigned int quality = 0;
    unsigned int predictor = 0;
    TIFFGetField(tifin, TIFFTAG_COMPRESSION, &compression);
	if (argc == 4)
	compression = atoi(argv[3]);
    TIFFSetField(tifout, TIFFTAG_COMPRESSION, compression);
    switch (compression) {
    case COMPRESSION_JPEG: 
	TIFFGetField(tifin, TIFFTAG_JPEGQUALITY, &quality);
	TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, quality);
	cout << "Jpeg compression: " << quality << endl;
	break;
    case COMPRESSION_DEFLATE: 
	TIFFGetField(tifin, TIFFTAG_ZIPQUALITY, &quality);
	TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, quality);
	TIFFGetField(tifin, TIFFTAG_PREDICTOR, &predictor);
	TIFFSetField(tifout, TIFFTAG_PREDICTOR, predictor);
	cout << "Zip compression" << endl;
	break;
    }
    TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

    unsigned char* newtiledata = new unsigned char[tilewidth * tileheight * bpp];
    unsigned int rowsize = tilewidth * bpp;
    
    unsigned char* topleft = new unsigned char[tilewidth * tileheight * bpp];
    unsigned char* topright = new unsigned char[tilewidth * tileheight * bpp];
    unsigned char* bottomleft = new unsigned char[tilewidth * tileheight * bpp];
    unsigned char* bottomright = new unsigned char[tilewidth * tileheight * bpp];

    for (unsigned int row = 0; row < newtilesy; ++row) {
	cout << "writing tiles for row " << row << " (of " << (newtilesy - 1) << ")" << endl;
	for (unsigned int col = 0; col < newtilesx; ++col) {
	    // read all 4 corners
	    if (((col * 2) < numtilesx) && ((row * 2) < numtilesy))
		TIFFReadEncodedTile(tifin, 
				    TIFFComputeTile(tifin, 
						    col * 2 * tilewidth, 
						    row * 2 * tileheight,
						    0, 
						    0), 
				    topleft, 
				    tilewidth * tileheight * bpp);
	    if (((col * 2 + 1) < numtilesx) && ((row * 2) < numtilesy))
		TIFFReadEncodedTile(tifin, 
				    TIFFComputeTile(tifin, 
						    (col * 2 + 1) * tilewidth, 
						    row * 2 * tileheight, 
						    0, 
						    0), 
				    topright, 
				    tilewidth * tileheight* bpp);
	    if (((col * 2) < numtilesx) && ((row * 2 + 1) < numtilesy))
		TIFFReadEncodedTile(tifin, 
				    TIFFComputeTile(tifin, 
						    col * 2 * tilewidth, 
						    (row * 2 + 1) * tileheight, 
						    0, 
						    0), 
				    bottomleft, 
				    tilewidth * tileheight* bpp);
	    if (((col * 2 + 1) < numtilesx) && ((row * 2 + 1) < numtilesy))
		TIFFReadEncodedTile(tifin, 
				    TIFFComputeTile(tifin, 
						    (col * 2 + 1) * tilewidth, 
						    (row * 2 + 1) * tileheight, 
						    0, 
						    0), 
				    bottomright, 
				    tilewidth * tileheight * bpp);
	    
	    // now scale them into newtiledata
	    //memset(newtiledata, 255, tilewidth * tileheight * bpp);
	    // make // !!
#pragma omp sections 
	    {
#pragma omp section 
		{
		Half(newtiledata, topleft, tilewidth, bpp, tilewidth);
		}
#pragma omp section 
		{
		Half(newtiledata + (tilewidth / 2) * bpp, topright, tilewidth, bpp, tilewidth);
		}
#pragma omp section 
		{
		Half(newtiledata + tilewidth * bpp * (tilewidth / 2), bottomleft, tilewidth, bpp, tilewidth);
		}
#pragma omp section 
		{
		Half(newtiledata + tilewidth * bpp * (tilewidth / 2) + (tilewidth / 2) * bpp, bottomright, tilewidth, bpp, tilewidth);
		}
	    }
	    TIFFWriteEncodedTile(tifout, 
				 TIFFComputeTile(tifout,
						 col * tilewidth, 
						 row * tileheight, 
						 0, 
						 0), 
				 newtiledata,
				 tilewidth * tileheight * bpp);
	}
    }
    TIFFClose(tifout);
    TIFFClose(tifin);
    delete [] newtiledata;
    delete [] topleft;
    delete [] topright;
    delete [] bottomleft;
    delete [] bottomright;
    return 1;
}
