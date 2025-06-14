#pragma once

#include "command.h"
#include <functional>
#include <utility> // For std::move

namespace undo {

/**
 * @brief A generic command implementation that uses callable objects (e.g., lambdas)
 *        for its execute and undo operations.
 *
 * This class allows for easy creation of commands without needing to define
 * a new class for each operation.
 */
class GenericCommand : public Command {
public:
    // Type alias for the callable functions
    using ExecuteFunc = std::function<void()>;
    using UndoFunc = std::function<void()>;

    /**
     * @brief Constructs a GenericCommand with specified execute and undo functions.
     *
     * @param execute_func The function to be called when execute() is invoked.
     * @param undo_func The function to be called when undo() is invoked.
     */
    GenericCommand(ExecuteFunc execute_func, UndoFunc undo_func)
        : execute_action_(std::move(execute_func)), undo_action_(std::move(undo_func)) {
    }

    // Non-copyable
    GenericCommand(const GenericCommand&) = delete;
    GenericCommand& operator=(const GenericCommand&) = delete;

    // Movable by default (due to std::function members being movable)
    GenericCommand(GenericCommand&&) = default;
    GenericCommand& operator=(GenericCommand&&) = default;

    /**
     * @brief Executes the command by invoking the stored execute function.
     */
    void execute() override {
        if (execute_action_) {
            execute_action_();
        }
    }

    /**
     * @brief Undoes the command by invoking the stored undo function.
     */
    void undo() override {
        if (undo_action_) {
            undo_action_();
        }
    }

private:
    ExecuteFunc execute_action_;
    UndoFunc undo_action_;
};

} // namespace undo
