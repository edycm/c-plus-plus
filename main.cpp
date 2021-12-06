#include <iostream>
#include <math.h>
#include <thread>

#include "bitmap.hpp"

int main()
{
    BitMap bm;
    int value = 0;

    bm.Init(10000);
    bm.Set(30);
    bm.Set(90);

    for (int i = 0; i != 100; ++i) {
        std::cout << "index: " << i << ", value: " << bm.Get(i) << std::endl;
    }

    bm.UnInit();
}
