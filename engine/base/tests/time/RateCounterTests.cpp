#include <TechEngine/base/time/RateCounter.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>

TEST_CASE("a rate sample needs one second of elapsed time", "[base][rate]") {
    TechEngine::RateCounter counter;
    for (int i = 0; i < 3; i++) {
        counter.advance(0.25, 15);
    }
    CHECK(counter.sampleIndex() == 0);
    CHECK(counter.rate() == 0.0);

    counter.advance(0.25, 15);
    CHECK(counter.sampleIndex() == 1);
    CHECK(counter.rate() == Catch::Approx(60.0));
}

TEST_CASE("a stall longer than the window lowers one sample instead of several", "[base][rate]") {
    TechEngine::RateCounter counter;
    counter.advance(3.0, 15);
    CHECK(counter.sampleIndex() == 1);
    CHECK(counter.rate() == Catch::Approx(5.0));

    counter.advance(1.0, 60);
    CHECK(counter.sampleIndex() == 2);
    CHECK(counter.rate() == Catch::Approx(60.0));
}

TEST_CASE("invalid elapsed time counts no work", "[base][rate]") {
    TechEngine::RateCounter counter;
    counter.advance(-1.0, 100);
    counter.advance(std::numeric_limits<double>::quiet_NaN(), 100);
    CHECK(counter.sampleIndex() == 0);

    counter.advance(1.0, 30);
    CHECK(counter.sampleIndex() == 1);
    CHECK(counter.rate() == Catch::Approx(30.0));
}
