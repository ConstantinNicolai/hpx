#include <hpx/local/algorithm.hpp>
#include <hpx/local/future.hpp>
#include <hpx/local/init.hpp>
#include <hpx/iostream.hpp>

#include <benchmark/benchmark.h>

int hpx_main(hpx::program_options::variables_map& vm)
{
	// extract command line argument
    std::uint64_t n = vm["data-size"].as<std::uint64_t>();

    // Say hello to the world!
    std::cout << "Hello World! Command Line Input is number: " << n << std::flush;
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Configure application-specific options
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("data-size",
        hpx::program_options::value<std::uint64_t>()->default_value(100),
        "size of the data the opertions is to be performed on");

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
