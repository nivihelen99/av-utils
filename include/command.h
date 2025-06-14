#pragma once

namespace undo {

/**
 * @brief An abstract interface for commands that can be executed and undone.
 *
 * This interface forms the basis of the command pattern, allowing for
 * the encapsulation of operations that can be reversed.
 */
class Command {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived command objects.
     */
    virtual ~Command() = default;

    // Non-copyable
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;

    // Movable
    Command(Command&&) = default;
    Command& operator=(Command&&) = default;

    /**
     * @brief Executes the command.
     *
     * This method should apply the operation encapsulated by the command.
     */
    virtual void execute() = 0;

    /**
     * @brief Undoes the command.
     *
     * This method should revert the changes made by the `execute()` method.
     */
    virtual void undo() = 0;
};

} // namespace undo
