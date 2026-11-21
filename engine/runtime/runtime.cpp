#include "runtime.hpp"
#include "simulation/simulation.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace engine {

Runtime::Runtime(Simulation& simulation)
    : simulation(simulation) {
    snapshot = buildRenderSnapshot(simulation.getWorld());
}

Runtime::~Runtime() {
    stop();
}

void Runtime::start() {
    if (running.load()) {
        return;
    }

    {
        std::lock_guard lock(mutex);
        error = nullptr;
    }

    running.store(true);

    try {
        simulationThread = std::thread(&Runtime::run, this);
    } catch (...) {
        running.store(false);
        throw;
    }
}

void Runtime::stop() {
    running.store(false);

    if (simulationThread.joinable()) {
        simulationThread.join();
    }
}

bool Runtime::isRunning() const {
    return running.load();
}

void Runtime::setBeforeStep(BeforeStep action) {
    std::lock_guard lock(mutex);
    beforeStep = std::move(action);
}

RenderSnapshot Runtime::getSnapshot() const {
    std::lock_guard lock(mutex);
    return snapshot;
}

void Runtime::rethrowIfFailed() const {
    std::exception_ptr savedError;

    {
        std::lock_guard lock(mutex);
        savedError = error;
    }

    if (savedError) {
        std::rethrow_exception(savedError);
    }
}

void Runtime::run() {
    try {
        auto previous = std::chrono::steady_clock::now();
        // накопитель прошедшего, не обработанного времени
        double accumulator = 0.0;
        constexpr double stepDuration = Simulation::getFixedDt();

        while (running.load()) {
            const auto now = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(now - previous).count();
            previous = now;

            // после долгой паузы не догоняем всё пропущенное время, а четверть секунды
            accumulator += std::min(elapsed, 0.25);
            bool updated = false;

            while (running.load() && accumulator >= stepDuration) {
                BeforeStep action;

                {
                    std::lock_guard lock(mutex);
                    action = beforeStep;
                }

                if (action) {
                    action(simulation.getWorld());
                }

                simulation.step();

                accumulator -= stepDuration;
                updated = true;
            }

            if (updated) {
                auto newSnapshot = buildRenderSnapshot(simulation.getWorld());

                std::lock_guard lock(mutex);
                snapshot = std::move(newSnapshot);
            }

            // между шагами поток спит, а не занимает процессор пустым циклом
            if (running.load()) {
                std::this_thread::sleep_for(std::chrono::duration<double>(stepDuration - accumulator));
            }
        }
    } catch (...) {
        {
            std::lock_guard lock(mutex);
            error = std::current_exception();
        }

        running.store(false);
    }
}

}