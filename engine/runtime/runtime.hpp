#pragma once

#include "renderer/snapshots.hpp"

#include <atomic>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace engine {

class Simulation;
namespace ecs { class World; }

class Runtime {
public:
    // функция, выполняющаюся перед каждым шагом симуляции
    // например обработка ввода из главного потока
    // надо будет назвать как-то по круче, по аналогии порядка отрисовки в directx
    using BeforeStep = std::function<void(ecs::World&)>;

    explicit Runtime(Simulation& simulation);
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    void start();
    void stop();

    bool isRunning() const;
    void setBeforeStep(BeforeStep action);
    RenderSnapshot getSnapshot() const;
    void rethrowIfFailed() const;

private:
    void run();

    // ссылка на существующую симуляцию (мы ее не создаем)
    Simulation& simulation;

    // atomic - позволяет безопасно читать и менять флаг из разных потоков
    std::atomic<bool> running{false};
    std::thread simulationThread;


    // отвечает сразу за beforeStep, snapshot и error
    mutable std::mutex mutex;

    BeforeStep beforeStep;
    RenderSnapshot snapshot;
    std::exception_ptr error;
};

}