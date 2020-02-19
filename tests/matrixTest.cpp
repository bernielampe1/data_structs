#include<iostream>

using namespace std;

#include "Matrix.h"

int main()
{
    float c = 3.1415927;
    float a[] = {1, 2, 3, 4, 5, 6, 7, 8, -9, -10, -11, -12, -13, 14, 15, 16};
    float b[] = {1, 2, 3, 4, 6, 5, 7, 8, -2, -45, -14, -12, -13, 41, 15, 61};

    Vec<float> v0(a, 4);
    Vec<float> v1(a, 3);
    Vec<u32> v2(5);

    Matrix<float> m0(a, 4, 4);
    Matrix<float> m1(a, 4, 4);
    Matrix<float> m2(a, 4, 4);

    Matrix<float> m3(a, 4, 3);
    Matrix<float> m4(a, 4, 3);

    Matrix<float> m5(b, 4, 4);

    cout << m0 << endl;

    // simple matrix compositon operations
    m2 = m0 + m1; cout << m2 << endl;
    m2 = m0 - m1; cout << m2 << endl;
    m2 = m0 * m1; cout << m2 << endl;
    m2 = m0 / m1; cout << m2 << endl;

    m2 += m1; cout << m2 << endl;
    m2 -= m1; cout << m2 << endl;
    m2 *= m1; cout << m2 << endl;
    m2 /= m1; cout << m2 << endl;

    // scalar operations
    m2 = m0 + c; cout << m2 << endl;
    m2 = m0 - c; cout << m2 << endl;
    m2 = m0 * c; cout << m2 << endl;
    m2 = m0 / c; cout << m2 << endl;

    m2 += c; cout << m2 << endl;
    m2 -= c; cout << m2 << endl;
    m2 *= c; cout << m2 << endl;
    m2 /= c; cout << m2 << endl;

    // more advanced operations
    cout << m2.transpose() << endl;
    cout << m2.diag() << endl;

    cout << m0.dot(v0) << endl;
    cout << m3.dot(v1) << endl;
    cout << m4.transpose().dot(v0) << endl;

    cout << m0.dot(m1) << endl;
    cout << m3.dot(m4.transpose()) << endl;
    cout << m4.dot(m3.transpose()) << endl;

    cout << m5.determinant_1() << endl;
    cout << m5.inverse_1() << endl;
    cout << m5.solve_1(v0) << endl;

    cout << m5.determinant_2() << endl << endl;
    cout << m5.inverse_2() << endl << endl;
    cout << m5.solve_2(v0) << endl << endl;

    return 0;
}
