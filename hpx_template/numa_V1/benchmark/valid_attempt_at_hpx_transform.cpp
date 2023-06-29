#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/util/high_resolution_timer.hpp>
#include <random>

void add_vectors(std::vector<double>& a, const std::vector<double>& b)
{
    hpx::transform(b.begin(), b.end(), a.begin(), a.begin(),
        [](double x, double y) { return x + y; });
}

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

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // Create a vector of vectors for each locality
    //new segmented vector hpx
    std::vector<std::vector<std::vector<double>>> a_vectors(localities.size());
    std::vector<std::vector<std::vector<double>>> b_vectors(localities.size());

    // Divide the work between localities
    std::size_t num_elements_per_locality = 1000000 / localities.size();

    // Initialize the vectors on each locality
    for (std::size_t i = 0; i < localities.size(); ++i)
    {
        a_vectors[i].resize(num_elements_per_locality);
        b_vectors[i].resize(num_elements_per_locality);
        
        for (std::size_t j = 0; j < num_elements_per_locality; ++j)
        {
            a_vectors[i][j].resize(num_elements_per_locality);
            b_vectors[i][j].resize(num_elements_per_locality);
            
            hpx::parallel::for_each(hpx::parallel::par, a_vectors[i][j].begin(), a_vectors[i][j].end(), 
                [&](double& element) {
                    element = dist(gen);
                });
                
            hpx::parallel::for_each(hpx::parallel::par, b_vectors[i][j].begin(), b_vectors[i][j].end(), 
                [&](double& element) {
                    element = dist(gen);
                });
        }
    }

    // Start the timer
    hpx::util::high_resolution_timer timer;

    // Perform the computation on each locality
    std::vector<hpx::future<void>> comp_futures;
    for (std::size_t i = 0; i < localities.size(); ++i)
    {
        comp_futures.push_back(hpx::async<add_vectors_action>(localities[i], std::move(a_vectors[i]), std::move(b_vectors[i])));
    }

    // Wait for the computations to finish
    hpx::wait_all(comp_futures);

    // Stop the timer and print the elapsed time
    double elapsed = timer.elapsed();
    std::cout << "Elapsed time: " << elapsed << " seconds" << std::endl;


    // Stop the HPX runtime
    hpx::runtime::stop();


    return 0;
}
