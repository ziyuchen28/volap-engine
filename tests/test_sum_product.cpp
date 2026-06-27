
#include<iostream>
#include<volap/kernels/sum_product.h>


void test_known_values()
{

    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};

    const float expected = 1.0f * 5.0f
                         + 2.0f * 6.0f
                         + 3.0f * 7.0f
                         + 4.0f * 8.0f;

    const float actual = volap::kernels::sum_product_f32(a, b, 4);
    std::cout << actual << "\n";
}



int main()
{
    test_known_values();

    std::cout << "test_sum_product: PASS\n";
    return 0;
}
