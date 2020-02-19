#include<iostream>

using namespace std;

#include "Vec.h"

int main()
{
    float c = 3.1415927;
    float a[] = {1, 2, 3, 4, 5, 6, 7, 8, -9, -10, -11, -12};
    Vec<float> v0(a, 12);
    Vec<float> v1(a, 12);
    Vec<float> v2(a, 12);

    cout << v0 << endl;

    // simple vector compositon operations
    v2 = v0 + v1; cout << v2 << endl;
    v2 = v0 - v1; cout << v2 << endl;
    v2 = v0 * v1; cout << v2 << endl;
    v2 = v0 / v1; cout << v2 << endl;

    v2 += v1; cout << v2 << endl;
    v2 -= v1; cout << v2 << endl;
    v2 *= v1; cout << v2 << endl;
    v2 /= v1; cout << v2 << endl;

    cout << v2.sum() << endl;
    cout << v2.prod() << endl;
    cout << v2.abs() << endl;

    // scalar operations
    v2 = v0 + c; cout << v2 << endl;
    v2 = v0 - c; cout << v2 << endl;
    v2 = v0 * c; cout << v2 << endl;
    v2 = v0 / c; cout << v2 << endl;

    v2 += c; cout << v2 << endl;
    v2 -= c; cout << v2 << endl;
    v2 *= c; cout << v2 << endl;
    v2 /= c; cout << v2 << endl;

    return 0;
}
