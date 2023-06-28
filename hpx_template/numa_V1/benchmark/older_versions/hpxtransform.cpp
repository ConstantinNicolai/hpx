static void benchTransformNoArena(benchmark::State& state) {
    constexpr int numLocalities = 4; // Assuming 4 NUMA localities for illustration
    constexpr int dataSize = state.range(0);

    std::vector<numa_adaptor<ValueType, ContainerType>> X(numLocalities, {dataSize, 1});
    std::vector<numa_adaptor<ValueType, ContainerType>> Y(numLocalities, {dataSize, 1});

    constexpr ValueType alpha = 2;

    for (auto _ : state) {
        for (int i = 0; i < numLocalities; ++i) {
            hpx::transform(hpx::execution::par(hpx::execution::task),
                           X[i].begin(), X[i].end(), Y[i].begin(),
                           [alpha](ValueType x) { return alpha * x; });
        }
    }
}
