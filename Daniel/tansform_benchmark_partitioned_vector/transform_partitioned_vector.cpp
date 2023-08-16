#include <benchmark/benchmark.h>
#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/partitioned_vector.hpp>
 
#include <hpx/modules/program_options.hpp>
 
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <hpx/iostream.hpp>
 
unsigned int size ;
 
static void Args(benchmark::internal::Benchmark* b) {
  const int64_t lowerLimit = 15; //TODO: 15
  const int64_t upperLimit = 33; //TODO: 33
 
  for (auto x = lowerLimit; x <= upperLimit; ++x) {
        b->Args({int64_t{1} << x});
  }
}

///////////////////////////////////////////////////////////////////////////////
// Define the vector types to be used.
HPX_REGISTER_PARTITIONED_VECTOR(int)

hpx::init_params init_args;
 
///////////////////////////////////////////////////////////////////////////////
//
// Define a view for a partitioned vector which exposes the part of the vector
// which is located on the current locality.
//
// This view does not own the data and relies on the partitioned_vector to be
// available during the full lifetime of the view.
//
template <typename T>
struct partitioned_vector_view
{
private:
    typedef typename hpx::partitioned_vector<T>::iterator global_iterator;
    typedef typename hpx::partitioned_vector<T>::const_iterator
        const_global_iterator;
 
    typedef hpx::traits::segmented_iterator_traits<global_iterator> traits;
    typedef hpx::traits::segmented_iterator_traits<const_global_iterator>
        const_traits;
 
    typedef typename traits::local_segment_iterator local_segment_iterator;
 
public:
    typedef typename traits::local_raw_iterator iterator;
    typedef typename const_traits::local_raw_iterator const_iterator;
    typedef T value_type;
 
public:
    explicit partitioned_vector_view(hpx::partitioned_vector<T>& data)
      : segment_iterator_(data.segment_begin(hpx::get_locality_id()))
    {
    }
    iterator begin()
    {
        return traits::begin(segment_iterator_);
    }
    iterator end()
    {
        return traits::end(segment_iterator_);
    }
 
    const_iterator begin() const
    {
        return const_traits::begin(segment_iterator_);
    }
    const_iterator end() const
    {
        return const_traits::end(segment_iterator_);
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator cend() const
    {
        return end();
    }
 
    value_type& operator[](std::size_t index)
    {
        return (*segment_iterator_)[index];
    }
    value_type const& operator[](std::size_t index) const
    {
        return (*segment_iterator_)[index];
    }
 
    std::size_t size() const
    {
        return (*segment_iterator_).size();
    }
 
private:
    local_segment_iterator segment_iterator_;
};
 
///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
   std::cout << "using vector size: " << size << std::endl;
 
    char const* const vector_name_1 =
        "partitioned_vector_1";
    char const* const vector_name_2 =
        "partitioned_vector_2";
    char const* const latch_name = "latch_spmd_foreach";
 
    {
        // create vector on one locality, connect to it from all others
        hpx::partitioned_vector<int> v1;
        hpx::partitioned_vector<int> v2;
        hpx::distributed::latch l;
        
        if (0 == hpx::get_locality_id())
        {
            std::vector<hpx::id_type> localities = hpx::find_all_localities();
            
            v1 = hpx::partitioned_vector<int>(
                                              size, hpx::container_layout(localities));
            v1.register_as(vector_name_1);
            
            v2 = hpx::partitioned_vector<int>(
                                              size, hpx::container_layout(localities));
            v2.register_as(vector_name_2);
            
            l = hpx::distributed::latch(localities.size());
            l.register_as(latch_name);
        }
        else
        {
            hpx::future<void> f1 = v1.connect_to(vector_name_1);
            hpx::future<void> f2 = v2.connect_to(vector_name_2);
            l.connect_to(latch_name);
            f1.get();
            f2.get();
        }
        
        // fill the vector 1 with numbers 1
        partitioned_vector_view<int> view1(v1);
        hpx::generate(hpx::execution::par, view1.begin(), view1.end(),
                      [&]() { return 1; });
        
        // fill the vector 2 with numbers 2
        partitioned_vector_view<int> view2(v2);
        hpx::generate(hpx::execution::par, view2.begin(), view2.end(),
                      [&]() { return 2; });
        
        constexpr int alpha = 2;
        
        // Transform the values of view1 by adding the corresponding values from view2
        hpx::transform(hpx::execution::par, view1.begin(), view1.end(), view2.begin(), view1.begin(),
                   [](int v, int y) { return alpha * v + y; });
        
        /*
        //print the transformed vector partitions from the specific locality
        for(int i = 0; i<view1.size(); i++){
            hpx::cout << "locality: " << hpx::get_locality_id() <<  ", Addition: " << view1[i] << "\n" << std::flush;
        }
        
        //print the transformed vector1
        for(int i = 0; i<v1.size(); i++){
            hpx::cout << "v1: " << v1[i] << "\n" << std::flush;
        }
        */
        
        // Wait for all localities to reach this point.
        l.arrive_and_wait();
    }
         
    return hpx::finalize();
}

static void benchHpxTranform(benchmark::State& state)
{
    for (auto _: state)
    {
        size = state.range(0);
        hpx::init(init_args);
    }
}
 
BENCHMARK(benchHpxTranform)->Apply(Args)->UseRealTime();

int main(int argc, char* argv[])
{
    printf("You have entered %d arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
 
    // run hpx_main on all localities
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};
 
    // Initialize and run HPX
    //hpx::init_params init_args;
    init_args.cfg = cfg;
 
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
 
    return 0;
 
    //return hpx::init(argc, argv, init_args);
}
#endif
