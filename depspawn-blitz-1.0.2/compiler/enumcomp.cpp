// BZ_ENUM_COMPUTATIONS

struct foo {
    enum { a = 5, b = 7, c = 2 };
};

struct bar {
    enum { a = 1, b = 6, c = 9 };
};

template<class T1, class T2>
struct Z {
    enum { a = (T1::a > T2::a) ? T1::a : T2::b,
           b = T1::b + T2::b,
           c = (T1::c * T2::c + T2::a + T1::a)
    };
};

int main()
{
    if (((int)Z<foo,bar>::a == 5) && ((int)Z<foo,bar>::b == 13) 
      && ((int)Z<foo,bar>::c == 24))
        return 0;
    else
        return 1;
}

