#ifndef TIMER_WHEEL_HPP
#define TIMER_WHEEL_HPP

#include <vector>
#include <list>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <utility> // For std::move
#include <algorithm> // For std::max, std::min etc.
#include <cmath>     // For std::ceil
#include <any>       // For std::any

namespace cpp_utils {

// Added back the missing TimerType enum
enum class TimerType {
    OneShot,
    Periodic
};

using TimerCallback = std::function<void(std::any cookie)>;

class TimerWheel {
public:
    TimerWheel(size_t resolution_ms, size_t wheel_size)
        : resolution_ms_(resolution_ms),
          wheel_size_(wheel_size),
          current_tick_absolute_(0),
          next_timer_id_(0) {
        if (resolution_ms == 0) {
            throw std::invalid_argument("Timer resolution must be greater than 0.");
        }
        if (wheel_size == 0) {
            throw std::invalid_argument("Timer wheel size must be greater than 0.");
        }
        wheel_.resize(wheel_size_);
    }

    int addTimer(uint64_t delay_ms, TimerCallback cb, std::any cookie, TimerType type = TimerType::OneShot) {
        int timer_id = next_timer_id_++;

        auto placement = calculatePlacement(delay_ms);
        size_t target_slot_idx = placement.first;
        size_t rounds = placement.second;

        uint64_t interval = (type == TimerType::Periodic) ? delay_ms : 0;
        if (type == TimerType::Periodic && interval == 0) {
            interval = resolution_ms_;
        }

        auto timer = std::make_unique<Timer>(timer_id, std::move(cb), type, interval, rounds, std::move(cookie));
        timer->slot_idx = target_slot_idx;

        wheel_[target_slot_idx].push_front(timer_id);
        timer->wheel_iterator = wheel_[target_slot_idx].begin();

        timers_[timer_id] = std::move(timer);
        return timer_id;
    }

    int addTimer(uint64_t delay_ms, TimerCallback cb, TimerType type = TimerType::OneShot) {
        return addTimer(delay_ms, std::move(cb), std::any(), type);
    }

    bool cancelTimer(int timer_id) {
        auto map_iter = timers_.find(timer_id);
        if (map_iter == timers_.end()) {
            return false;
        }
        Timer* timer_ptr = map_iter->second.get();
        if (timer_ptr->slot_idx < wheel_.size()) {
            if (!wheel_[timer_ptr->slot_idx].empty()) {
                wheel_[timer_ptr->slot_idx].erase(timer_ptr->wheel_iterator);
            }
        }
        timers_.erase(map_iter);
        return true;
    }

    void tick() {
        size_t current_processing_slot_idx = current_tick_absolute_ % wheel_size_;
        std::list<int> ids_in_slot_copy = wheel_[current_processing_slot_idx];

        for (int timer_id : ids_in_slot_copy) {
            auto map_iter = timers_.find(timer_id);
            if (map_iter == timers_.end()) {
                continue;
            }

            Timer* timer_ptr = map_iter->second.get();
            if (timer_ptr->slot_idx != current_processing_slot_idx) {
                continue;
            }

            if (timer_ptr->remaining_rounds == 0) {
                TimerCallback callback_to_run = timer_ptr->callback;
                TimerType current_timer_type = timer_ptr->type; // Use a different name from member 'type'
                uint64_t periodic_interval = timer_ptr->interval_ms;
                int current_timer_id = timer_ptr->id;
                std::any current_cookie = timer_ptr->cookie;

                wheel_[current_processing_slot_idx].erase(timer_ptr->wheel_iterator);

                if (current_timer_type == TimerType::OneShot) {
                    timers_.erase(map_iter);
                }

                if (callback_to_run) {
                    callback_to_run(std::move(current_cookie));
                }

                if (current_timer_type == TimerType::Periodic) {
                    auto post_callback_map_iter = timers_.find(current_timer_id);
                    if (post_callback_map_iter != timers_.end()) {
                        Timer* periodic_timer_to_reschedule = post_callback_map_iter->second.get();

                        auto new_placement = calculatePlacement(periodic_interval);
                        periodic_timer_to_reschedule->slot_idx = new_placement.first;
                        periodic_timer_to_reschedule->remaining_rounds = new_placement.second;
                        // Cookie for periodic timer remains the same (it's already in periodic_timer_to_reschedule->cookie)

                        wheel_[periodic_timer_to_reschedule->slot_idx].push_front(current_timer_id);
                        periodic_timer_to_reschedule->wheel_iterator = wheel_[periodic_timer_to_reschedule->slot_idx].begin();
                    }
                }
            } else {
                timer_ptr->remaining_rounds--;
            }
        }
        current_tick_absolute_++;
    }

private:
    struct Timer {
        int id;
        TimerCallback callback;
        TimerType type; // This now refers to the enum
        uint64_t interval_ms;
        size_t remaining_rounds;
        std::list<int>::iterator wheel_iterator;
        size_t slot_idx;
        std::any cookie;

        Timer(int timer_id, TimerCallback cb, TimerType t, uint64_t interv, size_t rounds, std::any c)
            : id(timer_id),
              callback(std::move(cb)),
              type(t),
              interval_ms(interv),
              remaining_rounds(rounds),
              slot_idx(0),
              cookie(std::move(c))
              {}
    };

    size_t resolution_ms_;
    size_t wheel_size_;
    uint64_t current_tick_absolute_;
    int next_timer_id_;

    std::vector<std::list<int>> wheel_;
    std::unordered_map<int, std::unique_ptr<Timer>> timers_;

    std::pair<size_t, size_t> calculatePlacement(uint64_t delay_ms) {
        if (resolution_ms_ == 0) throw std::logic_error("Timer resolution cannot be zero.");
        if (wheel_size_ == 0) throw std::logic_error("Timer wheel size cannot be zero.");
        size_t num_total_ticks_to_wait;
        if (delay_ms == 0) {
            num_total_ticks_to_wait = 1;
        } else {
            num_total_ticks_to_wait = static_cast<size_t>(std::ceil(static_cast<double>(delay_ms) / resolution_ms_));
            if (num_total_ticks_to_wait == 0 && delay_ms > 0) {
                num_total_ticks_to_wait = 1;
            }
        }
        if (num_total_ticks_to_wait == 0) {
             num_total_ticks_to_wait = 1;
        }
        size_t rounds = (num_total_ticks_to_wait - 1) / wheel_size_;
        size_t slot_offset_from_current = (num_total_ticks_to_wait - 1) % wheel_size_;
        size_t current_wheel_pos_for_calc = current_tick_absolute_ % wheel_size_;
        size_t target_slot_idx = (current_wheel_pos_for_calc + slot_offset_from_current) % wheel_size_;
        return {target_slot_idx, rounds};
    }
};

} // namespace cpp_utils

#endif // TIMER_WHEEL_HPP
