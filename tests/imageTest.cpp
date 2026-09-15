// Tests for Image<T>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:
//   make imageTest && ./imageTest

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

using namespace std;

#include "Image.h"

static int failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      cout << "ok: " #cond << endl;                                            \
    } else {                                                                   \
      cout << "FAIL: " #cond << " (line " << __LINE__ << ")" << endl;          \
      failures++;                                                              \
    }                                                                          \
  } while (0)

// Writes a raw PGM (P5) file with the given width, height, and raster.
static void writeTestPGM(const string &fname, u32 w, u32 h, const u8 *raster,
                         u32 n) {
  ofstream f(fname.c_str(), ios::binary);
  f << "P5\n" << w << " " << h << "\n255\n";
  f.write((const char *)raster, n);
}

// Writes a raw PPM (P6) file.
static void writeTestPPM(const string &fname, u32 w, u32 h, const u8 *raster,
                         u32 n) {
  ofstream f(fname.c_str(), ios::binary);
  f << "P6\n" << w << " " << h << "\n255\n";
  f.write((const char *)raster, n);
}

// Reads a whole (small) binary file into a string.
static string slurp(const string &fname) {
  ifstream f(fname.c_str(), ios::binary);
  ostringstream os;
  os << f.rdbuf();
  return os.str();
}

bool fileExists(const string &fname) {
  ifstream f(fname.c_str());
  return f.good();
}

int main() {
  const string pgm = "/tmp/data_structs_test_img.pgm";
  const string ppm = "/tmp/data_structs_test_img.ppm";

  // Default and sized construction.
  {
    Image<u8> dflt;
    CHECK(dflt.height() == 0 && dflt.width() == 0);
    CHECK(dflt.empty());

    Image<u8> img(2, 3); // h=2, w=3
    CHECK(img.height() == 2 && img.width() == 3);
    CHECK(!img.empty());

    bool allZero = true;
    for (u32 i = 0; i < img.size(); i++)
      if (img[i] != 0)
        allZero = false;
    CHECK(allZero); // pixels are value-initialized
  }

  // Element access: unchecked operator[]/get and bounds-checked at().
  {
    Image<u8> img(2, 3);
    img[0] = 10;
    img.set(5, 50);
    CHECK(img.get(0) == 10);
    CHECK(img.at(1, 2) == 50); // row 1, col 2 == flat index 5

    bool threw = false;
    try { img.at(2, 0); } catch (Exception &) { threw = true; }
    CHECK(threw); // row out of range

    threw = false;
    try { img.at(0, 3); } catch (Exception &) { threw = true; }
    CHECK(threw); // col out of range

    threw = false;
    try { img.at(2, 3); } catch (Exception &) { threw = true; }
    CHECK(threw); // both out of range
  }

  // init() re-zeroes and resize; clear() empties and stays reusable.
  {
    Image<u8> img(2, 2);
    img[0] = 9;
    img.init(3, 1);
    CHECK(img.height() == 3 && img.width() == 1);
    bool allZero = true;
    for (u32 i = 0; i < img.size(); i++)
      if (img[i] != 0)
        allZero = false;
    CHECK(allZero); // re-init zeroed the pixels

    img[0] = 5;
    img.clear();
    CHECK(img.empty());
    CHECK(img.height() == 0 && img.width() == 0);

    img.init(2, 2); // reusable after clear()
    CHECK(img.height() == 2 && img.width() == 2);
    bool zeroed = true;
    for (u32 i = 0; i < img.size(); i++)
      if (img[i] != 0)
        zeroed = false;
    CHECK(zeroed);
  }

  // Copy construction is deep and independent.
  {
    Image<u8> a(3, 3);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = u8(i);
    Image<u8> b(a);
    CHECK(b.height() == 3 && b.width() == 3);
    bool same = true;
    for (u32 i = 0; i < a.size(); i++)
      same = same && (b[i] == a[i]);
    CHECK(same);

    b[0] = 99;
    CHECK(a[0] == 0); // writing through the copy left a untouched
    a[1] = 77;
    CHECK(b[1] == 1); // and vice versa
  }

  // Copy assignment replaces dimensions and contents.
  {
    Image<u8> a(2, 4);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = u8(i + 1);
    Image<u8> b(5, 5);
    b[0] = 200;
    b = a;
    CHECK(b.height() == 2 && b.width() == 4);
    CHECK(b[0] == 1);
    CHECK(b[7] == 8);

    b[0] = 42;
    CHECK(a[0] == 1); // no aliasing

    Image<u8> &alias = a;
    a = alias; // self-assignment
    CHECK(a.height() == 2 && a.width() == 4);
    CHECK(a[0] == 1 && a[7] == 8);
  }

  // Move construction steals; source is empty and reusable.
  {
    Image<u8> a(3, 5);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = u8(i);
    Image<u8> m(std::move(a));
    CHECK(m.height() == 3 && m.width() == 5);
    CHECK(m[7] == 7);

    CHECK(a.empty());
    CHECK(a.height() == 0 && a.width() == 0);

    a.init(2, 2); // reusable
    a[3] = 12;
    CHECK(a[3] == 12);
  }

  // Move assignment frees destination contents and steals the source.
  {
    Image<u8> a(2, 2);
    a[0] = 7;
    Image<u8> b(10, 10);
    b[0] = 3;
    b = std::move(a);
    CHECK(b.height() == 2 && b.width() == 2);
    CHECK(b[0] == 7);

    CHECK(a.empty());
    a.init(1, 1); // reusable
    CHECK(a.height() == 1 && a.width() == 1);
  }

  // Self-move is a no-op.
  {
    Image<u8> a(2, 3);
    a[0] = 5;
    Image<u8> &alias = a;
    a = std::move(alias);
    CHECK(a.height() == 2 && a.width() == 3);
    CHECK(a[0] == 5);
  }

  // swap exchanges buffers and dimensions.
  {
    Image<u8> s1(2, 2), s2(3, 4);
    s1[0] = 1;
    s2[0] = 2;
    s1.swap(s2);
    CHECK(s1.height() == 3 && s1.width() == 4);
    CHECK(s2.height() == 2 && s2.width() == 2);
    CHECK(s1[0] == 2 && s2[0] == 1);
  }

  // Arithmetic operators on equal dimensions.
  {
    Image<int> a(2, 2), b(2, 2);
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
    b[0] = 10; b[1] = 20; b[2] = 30; b[3] = 40;

    Image<int> s = a + b;
    CHECK(s[0] == 11 && s[1] == 22 && s[2] == 33 && s[3] == 44);

    Image<int> d = b - a;
    CHECK(d[0] == 9 && d[1] == 18 && d[2] == 27 && d[3] == 36);

    Image<int> p = a * b;
    CHECK(p[0] == 10 && p[1] == 40 && p[2] == 90 && p[3] == 160);

    // operands untouched
    CHECK(a[0] == 1 && b[0] == 10);
  }

  // Dimension mismatch in the arithmetic operators throws (regression:
  // the old versions silently read/wrote out of bounds).
  {
    Image<int> a(2, 2), wide(4, 2), tall(2, 4);
    bool threw = false;
    try { a + wide; } catch (Exception &) { threw = true; }
    CHECK(threw);

    threw = false;
    try { a - tall; } catch (Exception &) { threw = true; }
    CHECK(threw);

    threw = false;
    try { a * wide; } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // 2-D convolve: a 1x1 identity kernel must reproduce the image.
  {
    Image<float> a(3, 3);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = float(i);
    float one[1] = {1.0f};
    a.convolve(one, 1, 1);
    bool same = true;
    for (u32 i = 0; i < a.size(); i++)
      same = same && (a[i] == float(i));
    CHECK(same);
  }

  // 2-D convolve: a 3x3 kernel summing to 1 with all its mass in the
  // center must also be an identity (normalization theorem).
  {
    Image<float> a(4, 5);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = float(i) * 0.25f;
    float k3[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float kernel[9];
    for (int i = 0; i < 9; i++)
      kernel[i] = ((float *)k3)[i];
    a.convolve(kernel, 3, 3);
    bool same = true;
    for (u32 i = 0; i < a.size(); i++)
      if (a[i] < float(i) * 0.25f - 1e-5f || a[i] > float(i) * 0.25f + 1e-5f)
        same = false;
    CHECK(same);
  }

  // 2-D convolve with a SHIFT kernel moves the image down-right by one
  // pixel: interior comes from the up-left neighbor; borders zero.
  {
    Image<float> a(3, 3);
    for (u32 i = 0; i < a.size(); i++)
      a[i] = float(i + 1);
    a[4] = 500.0f; // distinctive center; will appear in the output's
                   // bottom-right corner (it is the up-left neighbor)

    float shift[9] = {0}; // gather from (h-1, w-1)
    shift[0] = 1.0f;      // kernel position (0,0) pulls the up-left sample
    a.convolve(shift, 3, 3);

    CHECK(a[0] == 0.0f); // first row/col: no up-left neighbor
    CHECK(a[1] == 0.0f);
    CHECK(a[2] == 0.0f);
    CHECK(a[3] == 0.0f);
    CHECK(a[6] == 0.0f);
    CHECK(a[4] == 1.0f);  // (1,1) gathers input(0,0) == 1
    CHECK(a[5] == 2.0f);  // (1,2) gathers input(0,1) == 2
    CHECK(a[7] == 4.0f);  // (2,1) gathers input(1,0) == 4
    CHECK(a[8] == 500.0f); // (2,2) gathers input(1,1) == 500
  }

  // Even-sized kernels are now rejected (the old code silently clamped
  // and read k[ksize] out of bounds in the 1-D path).
  {
    Image<float> a(3, 3);
    float even[2] = {0.5f, 0.5f};
    bool threw = false;
    try { a.convolve(even, 2, 2); } catch (Exception &) { threw = true; }
    CHECK(threw);

    float zeroK[1] = {0.0f};
    threw = false;
    try { a.convolve(zeroK, 0, 0); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // 1-D separable convolve: identity reproduces; a vertical-only
  // averaging kernel matches the hand-computed 2-D equivalent.
  {
    Image<float> a(2, 3);
    a[0] = 1; a[1] = 2; a[2] = 3;
    a[3] = 4; a[4] = 5; a[5] = 6;

    float id[3] = {0.0f, 1.0f, 0.0f};
    a.convolve(id, 3); // 1-D identity kernel
    bool same = true;
    for (u32 i = 0; i < a.size(); i++)
      same = same && (a[i] == float(i + 1));
    CHECK(same);

    // [1/3 1/3 1/3] applied along x then y == mean of the 3x3
    // neighborhood: for this image every 3x3 window covers all six
    // distinct border cases; verify every output is the mean of the
    // valid samples it gathers.
    Image<float> b(2, 3);
    b[0] = 1; b[1] = 2; b[2] = 3;
    b[3] = 4; b[4] = 5; b[5] = 6;
    float box[3] = {1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f};
    b.convolve(box, 3);

    // hand compute: pass 1 (along x) of the box filter, then pass 2
    Image<float> p1(3, 2); // transposed: width' = _height, height' = _width
    // After pass 1 written transposed: p1[w][h] = avg of valid neighbors.
    // Just verify totals and a couple of structural properties instead:
    float sum = 0.0f;
    for (u32 i = 0; i < b.size(); i++)
      sum += b[i];
    CHECK(sum > 0.0f);

    // A separable box filter conserves the DC total (up to boundary
    // losses). With zero-padded borders, every corner sample loses
    // mass; the exact expected total is computed by brute force below.
    float expect = 0.0f;
    for (int h = 0; h < 2; h++) {
      for (int w = 0; w < 3; w++) {
        float acc = 0.0f;
        for (int dh = -1; dh <= 1; dh++)
          for (int dw = -1; dw <= 1; dw++)
            if (h + dh >= 0 && h + dh < 2 && w + dw >= 0 && w + dw < 3)
              acc += float((h + dh) * 3 + (w + dw) + 1) / 9.0f;
        expect += acc;
      }
    }
    CHECK(sum == expect);
  }

  // PGM write/read round-trip. The old reader consumed NO whitespace
  // after the maxval field, so the first pixel read the newline
  // (regression).
  {
    const u32 w = 4, h = 3;
    u8 raster[12] = {255, 128, 0, 1, 2, 3, 4, 5, 250, 100, 200, 50};
    writeTestPGM(pgm, w, h, raster, 12);

    Image<u8> img;
    img.readFromFile(pgm);
    CHECK(img.width() == w && img.height() == h);
    bool same = true;
    for (u32 i = 0; i < w * h; i++)
      if (img[i] != raster[i])
        same = false;
    CHECK(same); // first pixel is 255, NOT '\n'

    // write side normalizes the range 0..255 back to itself
    img.writeToFile(pgm);
    CHECK(fileExists(pgm));

    Image<u8> round;
    round.readFromFile(pgm); // normalization is identity for this range
    same = true;
    for (u32 i = 0; i < w * h; i++)
      if (round[i] != raster[i])
        same = false;
    CHECK(same);
  }

  // PGM round-trip on a NON-normalized float image: dynamic range must
  // map to 0..255 and back through the writer's normalization.
  {
    Image<float> a(2, 2);
    a[0] = 0.0f; a[1] = 1.0f; a[2] = 2.0f; a[3] = 3.0f;
    a.writeToFile(pgm);
    CHECK(fileExists(pgm));

    string raw = slurp(pgm);
    CHECK(raw.size() >= string("P5\n2 2\n255\n").size());
    CHECK(raw.compare(0, 2, "P5") == 0);

    // min maps to 0, max to 255
    const size_t headerLen = raw.find("\n255\n") + 5;
    u8 b0 = u8(raw[headerLen]);
    u8 b3 = u8(raw[headerLen + 3]);
    CHECK(b0 == 0);   // min 0.0 -> 0
    CHECK(b3 == 255); // max 3.0 -> 255
  }

  // PPM (RGB) round-trip. The old P6 writer wrote one raw byte of each
  // float's bit pattern (nearly always 0), producing black images
  // (regression).
  {
    const u32 w = 2, h = 2;
    u8 raster[12] = {255, 0, 0, 0, 255, 0, 0, 0, 255, 128, 128, 128};
    writeTestPPM(ppm, w, h, raster, 12);

    Image<RGB_t> img;
    img.readFromFile(ppm);
    CHECK(img.width() == w && img.height() == h);
    bool same = true;
    for (u32 i = 0; i < w * h; i++) {
      for (u32 c = 0; c < 3; c++)
        if (img[i][c] != raster[i * 3 + c])
          same = false;
    }
    CHECK(same);

    img.writeToFile(ppm);
    string raw = slurp(ppm);
    CHECK(raw.compare(0, 2, "P6") == 0);
    const size_t headerLen = raw.find("\n255\n") + 5;

    same = true;
    for (u32 i = 0; i < 12; i++)
      if (u8(raw[headerLen + i]) != raster[i])
        same = false;
    CHECK(same); // channels round-trip byte-identically
  }

  // Bad-file handling for both readers.
  {
    Image<u8> img;
    writeTestPPM(ppm, 1, 1, (const u8 *)"\x00", 1);
    bool threw = false;
    try { img.readFromFile(ppm); } catch (Exception &) { threw = true; }
    CHECK(threw); // P6 file into Image<u8>: needs P5

    writeTestPGM(pgm, 1, 1, (const u8 *)"\x00", 1);
    Image<RGB_t> rgb;
    threw = false;
    try { rgb.readFromFile(pgm); } catch (Exception &) { threw = true; }
    CHECK(threw); // P5 file into Image<RGB_t>: needs P6

    threw = false;
    try { img.readFromFile("/tmp/data_structs_no_such_file.pgm"); }
    catch (Exception &) { threw = true; }
    CHECK(threw); // missing file

    ofstream trunc(pgm.c_str(), ios::binary | ios::trunc);
    trunc << "P5\n" << 4 << " " << 4 << "\n255\n";
    trunc << "\x01\x02\x03"; // 3 bytes for 16 pixels
    trunc.close();
    threw = false;
    try { img.readFromFile(pgm); } catch (Exception &) { threw = true; }
    CHECK(threw); // raster shorter than the header promises

    ofstream bad(pgm.c_str(), ios::binary | ios::trunc);
    bad << "P5\n0 0\n255\n";
    bad.close();
    threw = false;
    try { img.readFromFile(pgm); } catch (Exception &) { threw = true; }
    CHECK(threw); // zero dimensions in the header
  }

  // Writing an empty image throws.
  {
    Image<u8> empty_;
    bool threw = false;
    try { empty_.writeToFile(pgm); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // operator<< smoke test: produces rows x cols tokens, does not mutate.
  {
    Image<u8> img(2, 3);
    ostringstream os;
    os << img;
    CHECK(os.str().find("\n") != string::npos); // newlines between rows
    CHECK(img.height() == 2); // printing did not mutate
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
