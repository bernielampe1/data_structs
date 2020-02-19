template <typename T> void Image<T>::convolve(const float *k, const u32 ksize) {
  Image<float> temp(_width, _height);
  u32 center = ksize >> 1;

  // convolve with 1-D kernel in the x direction
  for (s32 h = 0; h < _height; h++) {
    for (s32 w = 0; w < _width; w++) {
      float d = 0.0;
      for (s32 c = -center; c <= center; c++) {
        s32 wp = w + c;
        if (wp >= 0 && wp < _width)
          d += _data[h * _width + wp] * k[center + c];
      }

      temp._data[w * _height + h] = (T)d; // temp is flipped
    }
  }

  // convolve with 1-D kernel in the y direction
  for (s32 h = 0; h < temp._height; h++) {
    for (s32 w = 0; w < temp._width; w++) {
      float d = 0.0;
      for (s32 c = -center; c <= center; c++) {
        s32 wp = w + c;
        if (wp >= 0 && wp < temp._width)
          d += temp._data[h * temp._width + wp] * k[center + c];
      }

      _data[w * temp._height + h] = (T)d; // destination if flipped
    }
  }
}

template <typename T>
void Image<T>::convolve(const float *k, const u32 kheight, const u32 kwidth) {
  Image<float> temp(_height, _width);

  // special case if the ksize is even
  s32 kheightp = (kheight % 2) ? kheight : kheight - 1;
  s32 kwidthp = (kwidth % 2) ? kwidth : kwidth - 1;

  // convolve with 2-D kernel
  for (s32 h = 0; h < _height; h++) {
    for (s32 w = 0; w < _width; w++) {
      float d = 0.0; // kernel accumulator

      // loop over kernel
      for (s32 i = 0; i < kheight; i++) {
        for (s32 j = 0; j < kwidth; j++) {
          s32 hp = h + i - (kheightp >> 1);
          s32 wp = w + j - (kwidthp >> 1);

          if (hp >= 0 && hp < _height && wp >= 0 && wp < _width) {
            d += _data[hp * _width + wp] * k[i * kwidth + j];
          }
        }
      }

      temp._data[h * _width + w] = (T)d; // temp is flipped
    }
  }

  // copy convolved image to this image
  *this = temp;
}

template <typename T> Image<T> Image<T>::operator+(const Image<T> &im) const {
  Image<T> temp(_height, _width);

  for (u32 i = 0; i < _height * _width; i++) {
    temp._data[i] = _data[i] + im._data[i];
  }

  return (temp);
}

template <typename T> Image<T> Image<T>::operator-(const Image<T> &im) const {
  Image<T> temp(_height, _width);

  for (u32 i = 0; i < _height * _width; i++) {
    temp._data[i] = _data[i] - im._data[i];
  }

  return (temp);
}

template <typename T> Image<T> Image<T>::operator*(const Image<T> &im) const {
  Image<T> temp(_height, _width);

  for (u32 i = 0; i < _height * _width; i++) {
    temp._data[i] = _data[i] * im._data[i];
  }

  return (temp);
}

template <typename T> void Image<T>::readFromFile(const std::string &fname) {
  throw Exception("not implemented");
}

template <> void Image<u8>::readFromFile(const std::string &fname) {
  std::ifstream ifile;
  std::string magic, cols, rows, max;
  u8 p;

  ifile.open(fname.c_str());
  if (!ifile) {
    throw Exception("could not read from file");
  }

  ifile >> magic >> cols >> rows >> max;
  if (magic != "P5")
    throw Exception("image needs to be P5 pgm");

  init(atoi(rows.c_str()), atoi(cols.c_str()));
  for (u32 i = 0; i < _width * _height; i++) {
    ifile.read((s8 *)&p, sizeof(u8));

    _data[i] = p;
  }

  ifile.close();
}

template <> void Image<RGB_t>::readFromFile(const std::string &fname) {
  std::ifstream ifile;
  std::string magic, cols, rows, max;
  u8 r, g, b;

  ifile.open(fname.c_str());
  if (!ifile) {
    throw Exception("could not read from file");
  }

  ifile >> magic >> cols >> rows >> max;
  if (magic != "P6")
    throw Exception("image needs to be a P6 ppm");

  init(atoi(rows.c_str()), atoi(cols.c_str()));
  for (u32 i = 0; i < _width * _height; i++) {
    ifile.read((s8 *)&r, sizeof(u8));
    ifile.read((s8 *)&g, sizeof(u8));
    ifile.read((s8 *)&b, sizeof(u8));

    _data[i][0] = r;
    _data[i][1] = g;
    _data[i][2] = b;
  }

  ifile.close();
}

template <typename T>
void Image<T>::writeToFile(const std::string &fname) const {
  T minVal, maxVal;
  float scaleVal;
  std::ofstream ofile;
  u32 numElems = _height * _width;

  if (numElems <= 0) {
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
    u8 val = (u8)((_data[i] - minVal) * scaleVal + 0.5);
    ofile.write((s8 *)&val, sizeof(u8));
  }

  ofile.close();
}

template <> void Image<RGB_t>::writeToFile(const std::string &fname) const {
  std::ofstream ofile;
  u32 numElems = _height * _width;

  ofile.open(fname.c_str());
  if (!ofile) {
    throw(Exception("unable to open file for writing"));
  }

  ofile << "P6" << endl << _width << " " << _height << endl << "255" << endl;
  for (u32 i = 0; i < numElems; i++) {
    ofile.write((s8 *)&(_data[i][0]), sizeof(u8));
    ofile.write((s8 *)&(_data[i][1]), sizeof(u8));
    ofile.write((s8 *)&(_data[i][2]), sizeof(u8));
  }

  ofile.close();
}

template <> void Image<Vec2f_t>::writeToFile(const std::string &fname) const {
  std::ofstream ofile;
  s8 spac = 10;

  // allocate space
  u8 *img = new u8[_height * _width];
  memset(img, 0, _height * _width * sizeof(u8));

  // construct the graphical vector field
  for (s32 h = 0; h < (s32)_height; h += spac) {
    for (s32 w = 0; w < (s32)_width; w += spac) {
      s8 ex = w + _data[h * _width + w][0];
      s8 ey = h + _data[h * _width + w][1];

      if (ex >= 0 && ex < (s32)_width && ey >= 0 && ey < (s32)_height) {
        drawLine(w, h, ex, ey, (u8)255, _width, img);
      }
    }
  }

  // open file and check
  ofile.open(fname.c_str());
  if (!ofile) {
    throw(Exception("unable to open file for writing"));
  }

  // write to file
  ofile << "P5" << endl << _width << " " << _height << endl << "255" << endl;
  ofile.write((s8 *)img, _width * _height * sizeof(u8));
  ofile.close();

  delete[] img;
}
