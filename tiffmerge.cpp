/* 
 cl /EHsc /Ox /nologo /I../src/zlib123 /I../src/tiff-3.8.2\libtiff tiffmerge.cpp ../src/tiff-3.8.2\libtiff\libtiff.lib ../src/zlib123\zlib.lib ../src/jpeg-6bx\libjpeg.lib
*/

#include <iostream>
using namespace std;

extern "C" {
#include <tiffio.h>
}

#if defined(COLORMAP)
static const unsigned int COLSIZE = 256;
unsigned short r[COLSIZE];
unsigned short g[COLSIZE];
unsigned short b[COLSIZE];
#endif

int main(int argc, char* argv[]) {
    TIFF* tifout = TIFFOpen(argv[argc - 1], "wm");
    const char* description = "";
    const char* software = "Very Large Image Viewer";
    const char* copyright = "Frederic Delhoume";
    TIFFSetField(tifout, TIFFTAG_IMAGEDESCRIPTION, description);
    TIFFSetField(tifout, TIFFTAG_SOFTWARE, software);
    TIFFSetField(tifout, TIFFTAG_COPYRIGHT, copyright);
    unsigned int idx;
    cout << "Creating pyramid..." << endl
	 << "destination file: " << argv[argc - 1] << endl;

    for (idx = 1; idx < (argc - 1); ++idx) {
      unsigned int imagewidth;
      unsigned int imageheight;
      unsigned int tilewidth;
      unsigned int tileheight;

      unsigned int x, y;
      
      TIFF* tifin = TIFFOpen(argv[idx], "rm");
      tdata_t tilebuf = _TIFFmalloc(TIFFTileSize(tifin));
      cout << "  current file: " << argv[idx] << "(" << TIFFTileSize(tifin) << ")" << endl;
      TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
      TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);

      TIFFGetField(tifin, TIFFTAG_TILEWIDTH, &tilewidth);
      TIFFGetField(tifin, TIFFTAG_TILELENGTH, &tileheight);
      
      TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
      TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);
      
      TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, imagewidth);
      TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, imageheight);

      TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
      TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
      TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
#if defined(COLORMAP)
      for (unsigned int idxc = 0; idxc < COLSIZE; ++idxc) {
	  r[idxc] = 256 * idxc + idxc;
	  g[idxc] = 256 * idxc + idxc;
	  b[idxc] = 256 * idxc + idxc;
      }
      TIFFSetField(tifout, TIFFTAG_COLORMAP, r, g, b);
      TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE); 
      TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 1);
#else
      TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 3);
#endif
      if (idx != (argc - 2)) {
	  cout << "Reduced image" << endl;
	  TIFFSetField(tifout, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
      } else {
	  cout << "Full image" << endl;
      }
      
      unsigned int compression = 0;
      unsigned int quality = 0;
      unsigned int predictor = 0;
      TIFFGetField(tifin, TIFFTAG_COMPRESSION, &compression);
      TIFFSetField(tifout, TIFFTAG_COMPRESSION, compression);
      switch (compression) {
      case COMPRESSION_JPEG: {
	  TIFFGetFieldDefaulted(tifin, TIFFTAG_JPEGQUALITY, &quality);
	  TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, quality);
	  // TODO: copy JPEGTables
	  unsigned short size;
	  void* data;
	  TIFFGetFieldDefaulted(tifin, TIFFTAG_JPEGTABLES, &size, &data);
	  TIFFSetField(tifout, TIFFTAG_JPEGTABLES, size, data);
	  cout << "Jpeg compression: " << quality << endl;
	  break;
      }
      case COMPRESSION_DEFLATE: 
	  TIFFGetFieldDefaulted(tifin, TIFFTAG_ZIPQUALITY, &quality);
	  TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, quality);
	  TIFFGetFieldDefaulted(tifin, TIFFTAG_PREDICTOR, &predictor);
	  TIFFSetField(tifout, TIFFTAG_PREDICTOR, predictor);
	  cout << "Zip compression " << quality << " " << predictor << endl;
	  break;
      }
      for(y = 0; y < imageheight; y += tileheight) {
	  cout << "   row " << y << "/" << imageheight << endl;
	  for (x = 0; x < imagewidth; x += tilewidth) {
	      tsize_t numbytes = TIFFReadRawTile(tifin, 
						 TIFFComputeTile(tifin, x, y, 0, 0), 
						 tilebuf, 
						 TIFFTileSize(tifin));
	      //	      cout << "numbytes for " << x << ":" << y << " = " << numbytes << endl;
	      TIFFWriteRawTile(tifout, TIFFComputeTile(tifout, x, y, 0, 0), tilebuf, numbytes);
	  }
      }
      _TIFFfree(tilebuf);
      TIFFWriteDirectory(tifout);
      TIFFClose(tifin);
    }
    cout << "Done" << endl;
    TIFFClose(tifout);
}
