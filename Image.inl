template <typename T> void Image<T>::convolve(const float *k, const u32 ksize) {
  if (ksize < 1)
    throw Exception("convolve: kernel size must be positive");

  u32 center = ksize >> 1;

  // Phase 1: 1-D convolution along x, written TRANSPOSED into temp
  // (temp holds width x height), so phase 2 can run along its rows.
  // Phase 2 then transposes back, leaving *this convolved in both
  // directions with the original orientation restored.
  Image<float> temp(_width, _height);
  for (u32 h = 0; h < _height; h++) {
    for (u32 w = 0; w < _width; w++) {
      float d = 0.0;
      for (u32 c = 0; c < ksize; c++) {
        s32 wp = s32(w + center) - s32(c); // w + (center - c) signed
        if (wp >= 0 && wp < s32(_width))
          d += _data[h * _width + wp] * k[c];
      }
      temp._data[w * _height + h] = d;
    }
  }

  // Phase 2: 1-D convolution along the vertical axis of the
  // transposed buffer.
  for (u32 h = 0; h < temp._height; h++) {
    for (u32 w = 0; w < temp._width; w++) {
      float d = 0.0;
      for (u32 c = 0; c < ksize; c++) {
        s32 wp = s32(w + center) - s32(c);
        if (wp >= 0 && wp < s32(temp._width))
          d += temp._data[h * temp._width + wp] * k[c];
      }
      _data[w * temp._height + h] = (T)d;
    }
  }
}

template <typename T>
void Image<T>::convolve(const float *k, const u32 kheight, const u32 kwidth) {
  if (kheight < 1 || kwidth < 1 || (kheight % 2) == 0 || (kwidth % 2) == 0)
    throw Exception("convolve: kernel dimensions must be odd and positive");

  Image<float> temp(_height, _width);

  const s32 kh = s32(kheight), kw = s32(kwidth);

  // 2-D kernel convolution; borders are a zero-padded implicit: samples
  // outside the image contribute nothing.
  for (s32 h = 0; h < s32(_height); h++) {
    for (s32 w = 0; w < s32(_width); w++) {
      float d = 0.0; // kernel accumulator

      // loop over kernel
      for (s32 i = 0; i < kh; i++) {
        for (s32 j = 0; j < kw; j++) {
          s32 hp = h + i - (kh >> 1);
          s32 wp = w + j - (kw >> 1);

          if (hp >= 0 && hp < s32(_height) && wp >= 0 && wp < s32(_width)) {
            d += _data[hp * _width + wp] * k[i * kwidth + j];
          }
        }
      }

      temp._data[h * _width + w] = d;
    }
  }

  // copy convolved image back; float -> T conversion per element
  for (u32 i = 0; i < _height * _width; i++)
    _data[i] = (T)temp._data[i];
}

template <typename T> Image<T> Image<T>::operator+(const Image<T> &im) const {
  if (im._height != _height || im._width != _width)
    throw Exception("image addition requires equal dimensions");

  Image<T> temp(_height, _width);
  for (u32 i = 0; i < _height * _width; i++)
    temp._data[i] = _data[i] + im._data[i];

  return (temp);
}

template <typename T> Image<T> Image<T>::operator-(const Image<T> &im) const {
  if (im._height != _height || im._width != _width)
    throw Exception("image subtraction requires equal dimensions");

  Image<T> temp(_height, _width);
  for (u32 i = 0; i < _height * _width; i++)
    temp._data[i] = _data[i] - im._data[i];

  return (temp);
}

template <typename T> Image<T> Image<T>::operator*(const Image<T> &im) const {
  if (im._height != _height || im._width != _width)
    throw Exception("image multiplication requires equal dimensions");

  Image<T> temp(_height, _width);
  for (u32 i = 0; i < _height * _width; i++)
    temp._data[i] = _data[i] * im._data[i];

  return (temp);
}

/* The white space following the maxval field of a PGM/PPM header is a
 * single whitespace character, part of the raster. operator>> stops at
 * (but leaves) that character, so it must be consumed once before the
 * binary read; otherwise the FIRST PIXEL is read as the newline and the
 * whole raster shifts by one byte (the old readers had exactly that
 * defect). */
static void consumeRasterWhitespace(std::ifstream &ifile) {
  ifile.get();
}

template <> void Image<u8>::readFromFile(const std::string &fname) {
  std::ifstream ifile;
  std::string magic, cols, rows, max;

  ifile.open(fname.c_str());
  if (!ifile) {
    throw Exception("could not read from file");
  }

  ifile >> magic >> cols >> rows >> max;
  if (magic != "P5")
    throw Exception("image needs to be P5 pgm");

  const u32 h = atoi(rows.c_str());
  const u32 w = atoi(cols.c_str());
  if (h == 0 || w == 0)
    throw Exception("image header holds an invalid dimension");

  init(h, w);
  consumeRasterWhitespace(ifile);
  for (u32 i = 0; i < _width * _height; i++) {
    u8 p;
    ifile.read((char *)&p, sizeof(u8));
    _data[i] = p;
  }

  if (!ifile)
    throw Exception("image file ended before the raster did");

  ifile.close();
}

template <> void Image<RGB_t>::readFromFile(const std::string &fname) {
  std::ifstream ifile;
  std::string magic, cols, rows, max;

  ifile.open(fname.c_str());
  if (!ifile) {
    throw Exception("could not read from file");
  }

  ifile >> magic >> cols >> rows >> max;
  if (magic != "P6")
    throw Exception("image needs to be a P6 ppm");

  const u32 h = atoi(rows.c_str());
  const u32 w = atoi(cols.c_str());
  if (h == 0 || w == 0)
    throw Exception("image header holds an invalid dimension");

  init(h, w);
  consumeRasterWhitespace(ifile);
  for (u32 i = 0; i < _width * _height; i++) {
    u8 r, g, b;
    ifile.read((char *)&r, sizeof(u8));
    ifile.read((char *)&g, sizeof(u8));
    ifile.read((char *)&b, sizeof(u8));

    _data[i][0] = r;
    _data[i][1] = g;
    _data[i][2] = b;
  }

  if (!ifile)
    throw Exception("image file ended before the raster did");

  ifile.close();
}

/* Generic PGM writer (any arithmetic pixel type): the dynamic range of
 * the sample is normalized to 0..255. */
template <typename T>
void Image<T>::writeToFile(const std::string &fname) const {
  double minVal, maxVal;
  double scaleVal;
  std::ofstream ofile;
  u32 numElems = _height * _width;

  if (numElems == 0) {
    throw(Exception("cannot write a null image object to file"));
  }

  minVal = maxVal = _data[0];
  for (u32 i = 1; i < numElems; i++) {
    if (minVal > _data[i])
      minVal = _data[i];
    if (maxVal < _data[i])
      maxVal = _data[i];
  }

  scaleVal = 255.0 / (minVal < maxVal ? maxVal - minVal : 1.0);

  ofile.open(fname.c_str());
  if (!ofile) {
    throw(Exception("could not open image file for writing"));
  }

  ofile << "P5\n" << _width << " " << _height << "\n255\n";
  for (u32 i = 0; i < numElems; i++) {
    long val = lround((double(_data[i]) - minVal) * scaleVal);
    if (val < 0)
      val = 0;
    if (val > 255)
      val = 255;
    u8 b = u8(val);
    ofile.write((char *)&b, sizeof(u8));
  }

  ofile.flush();
  if (!ofile)
    throw(Exception("error while writing image file"));

  ofile.close();
}

/* PPM writer specialization: pixel type is Vec3f_t (float RGB triple).
 * The old version wrote one byte of each float's raw bit pattern,
 * producing an all-black file; each channel is instead clamped to
 * 0..255 and stored as its own byte so the image round-trips. */
template <> void Image<RGB_t>::writeToFile(const std::string &fname) const {
  std::ofstream ofile;
  u32 numElems = _height * _width;

  if (numElems == 0)
    throw(Exception("cannot write a null image object to file"));

  ofile.open(fname.c_str());
  if (!ofile) {
    throw(Exception("unable to open image file for writing"));
  }

  ofile << "P6\n" << _width << " " << _height << "\n255\n";
  for (u32 i = 0; i < numElems; i++) {
    for (u32 c = 0; c < 3; c++) {
      long v = lround(_data[i][c]);
      if (v < 0)
        v = 0;
      if (v > 255)
        v = 255;
      u8 b = u8(v);
      ofile.write((char *)&b, sizeof(u8));
    }
  }

  ofile.flush();
  if (!ofile)
    throw(Exception("error while writing image file"));

  ofile.close();
}

/* Vector-field writer (pixel type Vec2f_t): renders each pixel's
 * vector as a bright line from (w, h) to (w + v0, h + v1). Kept from
 * the original; a missing	drawLine.h made this translation unit
 * unbuildable before. */
template <> void Image<Vec2f_t>::writeToFile(const std::string &fname) const {
  std::ofstream ofile;
  u8 spac = 10;

  // allocate space
  u8 *img = new u8[_height * _width]();
  std::unique_ptr<u8[]> imgGuard(img);

  // construct the graphical vector field
  for (u32 h = 0; h < _height; h += spac) {
    for (u32 w = 0; w < _width; w += spac) {
      s32 ex = s32(w) + s32(_data[h * _width + w][0]);
      s32 ey = s32(h) + s32(_data[h * _width + w][1]);

      if (ex >= 0 && ey >= 0 && ex < s32(_width) && ey < s32(_height)) {
        drawLine(s32(w), s32(h), ex, ey, (u8)255, _width, _height, img);
      }
    }
  }

  // open file and check
  ofile.open(fname.c_str());
  if (!ofile) {
    throw(Exception("unable to open file for writing"));
  }

  // write to file
  ofile << "P5\n" << _width << " " << _height << "\n255\n";
  ofile.write((char *)img, _width * _height * sizeof(u8));
  ofile.flush();
  if (!ofile)
    throw(Exception("error while writing image file"));

  ofile.close();
}
