


void test_known_values()
{
    volap::core::ColumnVector<float> a {1.0f, 2.0f, 3.0f, 4.0f};
    volap::core::ColumnVector<float> b {5.0f, 6.0f, 7.0f, 8.0f};

    const float expected = 1.0f * 5.0f
                         + 2.0f * 6.0f
                         + 3.0f * 7.0f
                         + 4.0f * 8.0f;

    const float actual = volap::kernels::sum_product_f32(a, b);

    check_near(actual, expected, 1e-6f, "known values");
}



int main()
{
    test_column_vector_basic();
    test_empty_input();
    test_known_values();
    test_size_mismatch_throws();
    test_scalar_auto_and_requested_avx2_fma_match();

    std::cout << "test_sum_product: PASS\n";
    return 0;
}
