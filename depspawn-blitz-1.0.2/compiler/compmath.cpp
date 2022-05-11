// BZ_HAVE_COMPLEX_MATH1
// Complex math functions, as per 26.2.7 of the Jan'96 draft standard
#include <complex>

#ifndef BZ_NO_NAMESPACES
using namespace std;
#endif

int main()
{
    complex<double> x(1.0, 1.0), y(1.0, 1.0);

    real(x);
    imag(x);
    abs(x);
    arg(x);
    norm(x);
    conj(x);
    polar(1.0,1.0);

    cos(x);
    cosh(x);
    exp(x);
    log(x);
    log10(x);
    pow(x,1);   
    pow(x,double(2.0));
    pow(x, y);
    pow(double(2.0), x);
    sin(x);
    sinh(x);
    sqrt(x);
    tan(x);
    tanh(x);

    return 0;
}

