#ifndef INCLUDE_ARC_TRAITS_SCHEDULER_HPP
#define INCLUDE_ARC_TRAITS_SCHEDULER_HPP

#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <exception>
#include <string>
#endif

#include "arc/traits/scheduler.hxx"

namespace arc {

ARC_MODULE_EXPORT
struct TerminateSchedulerThreadException : std::exception
{
    explicit TerminateSchedulerThreadException(std::string msg) : msg(std::move(msg)) {}
    char const* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

ARC_MODULE_EXPORT
struct StopSchedulerException : std::exception
{
    explicit StopSchedulerException(std::string msg) : msg(std::move(msg)) {}
    char const* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

} // namespace arc

#endif // INCLUDE_ARC_TRAITS_SCHEDULER_HPP
