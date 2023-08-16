cmake_minimum_required( VERSION 3.14 )
project(NUMA CXX)

find_package( Threads REQUIRED )
find_package( TBB REQUIRED )
find_package( benchmark REQUIRED )
find_package( xsimd REQUIRED)
find_package( OpenMP REQUIRED COMPONENTS CXX)

function( rome_build targetname )
	target_compile_features( ${targetname} PRIVATE cxx_std_20 )
	target_compile_options( ${targetname} PRIVATE -march=core-avx2 -mtune=core-avx2 -qopenmp -qopenmp-simd -O3 -mfma ) 
	target_include_directories( ${targetname} PRIVATE include )
	target_link_libraries( ${targetname} PRIVATE benchmark::benchmark TBB::tbb Threads::Threads OpenMP::OpenMP_CXX xsimd )
endfunction()

function( qdr_build targetname )
	target_compile_features( ${targetname} PRIVATE cxx_std_20 )
	target_compile_options( ${targetname} PRIVATE -march=core-avx-i -mtune=core-avx-i -qopenmp -qopenmp-simd -O3 ) 
	target_include_directories( ${targetname} PRIVATE include )
	target_link_libraries( ${targetname} PRIVATE benchmark::benchmark TBB::tbb Threads::Threads OpenMP::OpenMP_CXX xsimd )
endfunction()

function( skylake_build targetname )
	target_compile_features( ${targetname} PRIVATE cxx_std_20 )
	target_compile_options( ${targetname} PRIVATE -march=skylake-avx512 -mtune=skylake-avx512 -qopt-zmm-usage=high -qopenmp -qopenmp-simd -O3 -mfma ) 
	target_include_directories( ${targetname} PRIVATE include )
	target_link_libraries( ${targetname} PRIVATE benchmark::benchmark TBB::tbb Threads::Threads OpenMP::OpenMP_CXX xsimd )
endfunction()


add_executable( transform-benchmark_rome benchmark/transform.cpp )
rome_build( transform-benchmark_rome )

add_executable( reduction-benchmark_rome benchmark/reduction.cpp )
rome_build( reduction-benchmark_rome )

add_executable( scan-benchmark_rome benchmark/scan.cpp )
rome_build( scan-benchmark_rome )

add_executable( transform-benchmark_qdr benchmark/transform.cpp )
qdr_build( transform-benchmark_qdr )

add_executable( reduction-benchmark_qdr benchmark/reduction.cpp )
qdr_build( reduction-benchmark_qdr )

add_executable( scan-benchmark_qdr benchmark/scan.cpp )
qdr_build( scan-benchmark_qdr )

add_executable( transform-benchmark_skylake benchmark/transform.cpp )
skylake_build( transform-benchmark_skylake )

add_executable( reduction-benchmark_skylake benchmark/reduction.cpp )
skylake_build( reduction-benchmark_skylake )

add_executable( scan-benchmark_skylake benchmark/scan.cpp )
skylake_build( scan-benchmark_skylake )


