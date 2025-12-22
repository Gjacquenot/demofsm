#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

namespace sc = boost::statechart;

struct Motor {
    double speed  = 0.0;
    double torque = 0.0;

    void update(bool running) {
        // Discrete first-order response:
        //   ds/dt = (s_target - s) / tau
        // Discretized with alpha = dt / (tau + dt):
        const double dt = 0.01;                 // seconds per step (matches previous simple integration)
        const double tau = 0.2;                 // time constant (s) — tune as needed
        const double speed_per_torque = 1.0;    // steady-state speed per unit torque

        torque = running ? 10.0 : 0.0;
        const double target_speed = torque * speed_per_torque;
        const double alpha = dt / (tau + dt);
        speed += (target_speed - speed) * alpha;
    }
};

struct EvStart : sc::event<EvStart> {};
struct EvStop  : sc::event<EvStop> {};
struct EvFail  : sc::event<EvFail> {};
struct EvReset : sc::event<EvReset> {};
struct EvTick  : sc::event<EvTick> {};

struct Idle;
struct Running;
struct Error;

struct MotorFSM : sc::state_machine<MotorFSM, Idle> {
    Motor motor;
    std::string state = "Unknown";
};

struct Running : sc::state<Running, MotorFSM> {
    using reactions = boost::mpl::list<
        sc::transition<EvStop, Idle>,
        sc::custom_reaction<EvTick>,
        sc::custom_reaction<EvFail>
    >;

    Running(my_context ctx) : my_base(ctx) {
        std::cout << "[STATE] Running\n";
        context<MotorFSM>().state = "Running";
    }

    sc::result react(const EvTick &) {
        auto& fsm = context<MotorFSM>();
        fsm.motor.update(true);
        return discard_event();
    }

    sc::result react(const EvFail &) {
        return transit<Error>();
    }
};

struct Idle : sc::state<Idle, MotorFSM> {
    using reactions = boost::mpl::list<
        sc::transition<EvStart, Running>,
        sc::custom_reaction<EvTick>
    >;

    Idle(my_context ctx) : my_base(ctx) {
        std::cout << "[STATE] Idle\n";
        context<MotorFSM>().state = "Idle";
    }

    sc::result react(const EvTick &) {
        context<MotorFSM>().motor.update(false);
        return discard_event();
    }
};

struct Error : sc::state<Error, MotorFSM> {
    using reactions = boost::mpl::list<
        sc::transition<EvReset, Idle>,
        sc::custom_reaction<EvTick>
    >;

    Error(my_context ctx) : my_base(ctx) {
        std::cout << "[STATE] Error\n";
        context<MotorFSM>().state = "Error";
    }

    sc::result react(const EvTick &) {
        context<MotorFSM>().motor.update(false);
        return discard_event();
    }
};

int main() {
    MotorFSM fsm;
    fsm.initiate();

    const auto dt = std::chrono::milliseconds(10);
    auto next = std::chrono::steady_clock::now();

    std::ofstream csv("log.csv");
    if (!csv) {
        std::cerr << "Failed to open log.csv for writing\n";
    } else {
        csv << "step,speed,state,events\n";
        csv << std::fixed << std::setprecision(6);
    }

    for (int step = 0; step < 1000; ++step) {

        // Events processed this iteration (joined with '|')
        std::string events;

        // Inject events
        if (step == 100) { fsm.process_event(EvStart()); if (!events.empty()) events += "|"; events += "EvStart"; }
        if (step == 500) { fsm.process_event(EvFail());  if (!events.empty()) events += "|"; events += "EvFail"; }
        if (step == 600) { fsm.process_event(EvReset()); if (!events.empty()) events += "|"; events += "EvReset"; }
        if (step == 650) { fsm.process_event(EvStart()); if (!events.empty()) events += "|"; events += "EvStart"; }

        // Tick
        fsm.process_event(EvTick());
        if (!events.empty()) events += "|"; events += "EvTick";

        // Observe
        std::cout << "step=" << std::setw(6) << std::left << step
                  << " state=" << std::setw(8) << std::left << fsm.state
                  << " speed=" << fsm.motor.speed;
        if (!events.empty()) std::cout << " events=" << events;
        std::cout << "\n";

        if (csv) {
            csv << step << "," << fsm.motor.speed << "," << fsm.state << "," << events << "\n";
        }

        // Real-time pacing
        next += dt;
        std::this_thread::sleep_until(next);
    }
}
