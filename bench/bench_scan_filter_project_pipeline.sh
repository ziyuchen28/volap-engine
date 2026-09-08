# for chunk in 256 512 1024 2048 4096 8192; do
#     ./build/bench_scan_filter_project_pipeline \
#         --rows 1048576 \
#         --chunk-size $chunk \
#         --iters 20 \
#         --warmup 3 \
#         --threshold 50
# done

./build/bench_scan_filter_project_pipeline \
    --rows 1048576 \
    --chunk-size 2048 \
    --iters 20 \
    --warmup 3 \
    --threshold 50
