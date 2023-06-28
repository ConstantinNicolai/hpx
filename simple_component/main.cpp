#include <benchmark/benchmark.h>
#include <hpx/hpx_main.hpp>
 // #include <hpx/runtime/serialization/detail/preprocess.hpp>
#include <hpx/include/runtime.hpp> 
#include <hpx/include/serialization.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

// #include <support/dummy.hpp>

struct simple_component
  : hpx::components::component_base<simple_component>
{
    int foo()
    {
        return 0;
    }
    HPX_DEFINE_COMPONENT_ACTION(simple_component, foo, foo_action);
};

typedef
  hpx::components::component<simple_component>
simple_component_server;
HPX_REGISTER_COMPONENT(simple_component_server, component_server);

static void simple_component_local(benchmark::State& state)
{
    hpx::id_type dest = hpx::find_here();
    for (auto _: state)
    {
        hpx::id_type id = hpx::new_<simple_component>(dest).get();
    }
}
BENCHMARK(simple_component_local)
    ->UseRealTime()
    ;

static void simple_component_remote(benchmark::State& state)
{
    hpx::id_type dest = hpx::find_all_localities()[1];
    for (auto _: state)
    {
        hpx::id_type id = hpx::new_<simple_component>(dest).get();
    }
}
BENCHMARK(simple_component_remote)
    ->UseRealTime()
    ;
BENCHMARK_MAIN();
