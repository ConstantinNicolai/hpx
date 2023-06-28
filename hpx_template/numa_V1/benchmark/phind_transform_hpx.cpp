#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/util/high_resolution_timer.hpp>
#include <vector>


void add_vectors(std::vector<double>& a, const std::vector<double>& b)
{
    hpx::transform(b.begin(), b.end(), a.begin(), a.begin(),
        [](double x, double y) { return x + y; });
}


std::vector<double> a = {1.0, 2.0, 3.0};
std::vector<double> b = {4.0, 5.0, 6.0};


struct add_vectors_action : hpx::actions::make_action<
    void (*)(std::vector<double>&, const std::vector<double>&),
    &add_vectors
>::type {};

HPX_REGISTER_ACTION(add_vectors_action);


int main()
{
    // Initialize HPX runtime
    hpx::runtime::start();

    // Create two localities
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    // Create the vectors on each locality
    std::vector<hpx::future<void>> futures;

    // Start the timer
    hpx::util::high_resolution_timer timer;    

    for (const auto& locality : localities)
    {
        futures.push_back(hpx::async<add_vectors_action>(locality, a, b));
    }

    // Wait for the computations to finish
    hpx::wait_all(futures);

    // Stop the timer and print the elapsed time
    double elapsed = timer.elapsed();
    std::cout << "Elapsed time: " << elapsed << " seconds" << std::endl;


    // Stop the HPX runtime
    hpx::runtime::stop();

    return 0;
}
