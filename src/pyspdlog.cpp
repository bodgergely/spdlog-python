#ifndef _WIN32
#define SPDLOG_ENABLE_SYSLOG
#endif

#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
//#include <pybind11/chrono.h>

using namespace pybind11::literals;

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#ifndef _WIN32
#include <spdlog/sinks/syslog_sink.h>
#endif

#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace spd = spdlog;
namespace py = pybind11;

namespace { // Avoid cluttering the global namespace.

class Logger;

bool g_async_mode_on = false;
auto g_async_overflow_policy = spdlog::async_overflow_policy::block;

std::unordered_map<std::string, Logger*> g_loggers;
std::mutex mutex_loggers;

void register_logger(const std::string& name, Logger* logger)
{
    std::lock_guard<std::mutex> lck(mutex_loggers);
    g_loggers[name] = logger;
}

Logger* access_logger(const std::string& name)
{
    std::lock_guard<std::mutex> lck(mutex_loggers);
    return g_loggers[name];
}

void remove_logger(const std::string& name)
{
    std::lock_guard<std::mutex> lck(mutex_loggers);
    g_loggers[name] = nullptr;
    g_loggers.erase(name);
}

void remove_logger_all()
{
    std::lock_guard<std::mutex> lck(mutex_loggers);
    g_loggers.clear();
}

class LogLevel {
public:
    const static int trace{ (int)spd::level::trace };
    const static int debug{ (int)spd::level::debug };
    const static int info{ (int)spd::level::info };
    const static int warn{ (int)spd::level::warn };
    const static int err{ (int)spd::level::err };
    const static int critical{ (int)spd::level::critical };
    const static int off{ (int)spd::level::off };
};

class Sink {
public:
    Sink() {}
    Sink(const spd::sink_ptr& sink)
        : _sink(sink)
    {
    }
    virtual ~Sink() {}
    virtual void log(const spd::details::log_msg& msg)
    {
        _sink->log(msg);
    }
    bool should_log(int msg_level) const
    {
        return _sink->should_log((spd::level::level_enum)msg_level);
    }
    void set_level(int log_level)
    {
        _sink->set_level((spd::level::level_enum)log_level);
    }
    int level() const
    {
        return (int)_sink->level();
    }

    spd::sink_ptr get_sink() const { return _sink; }

protected:
    spd::sink_ptr _sink{ nullptr };
};

// template <class sink_type>
// class generic_sink : public Sink {
// public:
//     generic_sink() {
//         _sink = std::make_shared<sink_type>();
//     }
// };

// class stdout_sink_st : public generic_sink<spdlog::sinks::stdout_sink_st> { };
// class stdout_sink_mt : public generic_sink<spdlog::sinks::stdout_sink_mt> { };

class stdout_sink_st : public Sink {
public:
    stdout_sink_st()
    {
        _sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
    }
};

class stdout_sink_mt : public Sink {
public:
    stdout_sink_mt()
    {
        _sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    }
};

class stdout_color_sink_st : public Sink {
public:
    stdout_color_sink_st()
    {
        _sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    }
};

class stdout_color_sink_mt : public Sink {
public:
    stdout_color_sink_mt()
    {
        _sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    }
};

class stderr_sink_st : public Sink {
public:
    stderr_sink_st()
    {
        _sink = std::make_shared<spdlog::sinks::stderr_sink_st>();
    }
};

class stderr_sink_mt : public Sink {
public:
    stderr_sink_mt()
    {
        _sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
    }
};

class stderr_color_sink_st : public Sink {
public:
    stderr_color_sink_st()
    {
        _sink = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
    }
};

class stderr_color_sink_mt : public Sink {
public:
    stderr_color_sink_mt()
    {
        _sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    }
};

class basic_file_sink_st : public Sink {
public:
    basic_file_sink_st(const std::string& base_filename, bool truncate)
    {
        _sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(base_filename, truncate);
    }
};

class basic_file_sink_mt : public Sink {
public:
    basic_file_sink_mt(const std::string& base_filename, bool truncate)
    {
        _sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(base_filename, truncate);
    }
};

class daily_file_sink_mt : public Sink {
public:
    daily_file_sink_mt(const std::string& base_filename, int rotation_hour, int rotation_minute)
    {
        _sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(base_filename, rotation_hour, rotation_minute);
    }
};

class daily_file_sink_st : public Sink {
public:
    daily_file_sink_st(const std::string& base_filename, int rotation_hour, int rotation_minute)
    {
        _sink = std::make_shared<spdlog::sinks::daily_file_sink_st>(base_filename, rotation_hour, rotation_minute);
    }
};

class rotating_file_sink_mt : public Sink {
public:
    rotating_file_sink_mt(const std::string& filename, size_t max_file_size, size_t max_files)
    {
        _sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, max_file_size, max_files);
    }
};

class rotating_file_sink_st : public Sink {
public:
    rotating_file_sink_st(const std::string& filename, size_t max_file_size, size_t max_files)
    {
        _sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(filename, max_file_size, max_files);
    }
};

class null_sink_st : public Sink {
public:
    null_sink_st()
    {
        _sink = std::make_shared<spdlog::sinks::null_sink_st>();
    }
};

class null_sink_mt : public Sink {
public:
    null_sink_mt()
    {
        _sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    }
};

#ifdef SPDLOG_ENABLE_SYSLOG
class syslog_sink_st : public Sink {
public:
    syslog_sink_st(const std::string& ident = "", int syslog_option = 0, int syslog_facility = (1 << 3))
    {
        _sink = std::make_shared<spdlog::sinks::syslog_sink_st>(ident, syslog_option, syslog_facility);
    }
};

class syslog_sink_mt : public Sink {
public:
    syslog_sink_mt(const std::string& ident = "", int syslog_option = 0, int syslog_facility = (1 << 3))
    {
        _sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(ident, syslog_option, syslog_facility);
    }
};
#endif

class Logger {
public:
    using async_factory_nb = spdlog::async_factory_impl<spdlog::async_overflow_policy::overrun_oldest>;

    Logger(const std::string& name, bool async_mode)
        : _name(name)
        , _async(async_mode)
    {
        register_logger(name, this);
    }

    virtual ~Logger() {}
    std::string name() const
    {
        if (_logger)
            return _logger->name();
        else
            return "NULL";
    }
    void log(int level, const std::string& msg) const { this->_logger->log((spd::level::level_enum)level, msg); }
    void trace(const std::string& msg) const { this->_logger->trace(msg); }
    void debug(const std::string& msg) const { this->_logger->debug(msg); }
    void info(const std::string& msg) const { this->_logger->info(msg); }
    void warn(const std::string& msg) const { this->_logger->warn(msg); }
    void error(const std::string& msg) const { this->_logger->error(msg); }
    void critical(const std::string& msg) const { this->_logger->critical(msg); }

    bool should_log(int level) const
    {
        return _logger->should_log((spd::level::level_enum)level);
    }

    void set_level(int level)
    {
        _logger->set_level((spd::level::level_enum)level);
    }

    int level() const
    {
        return (int)_logger->level();
    }

    void set_pattern(const std::string& pattern, spd::pattern_time_type type = spd::pattern_time_type::local)
    {
        _logger->set_pattern(pattern, type);
    }

    // automatically call flush() if message level >= log_level
    void flush_on(int log_level)
    {
        _logger->flush_on((spd::level::level_enum)log_level);
    }

    void flush()
    {
        _logger->flush();
    }

    bool async()
    {
        return _async;
    }

    void close()
    {
        remove_logger(_name);
        _logger = nullptr;
        spdlog::drop(_name);
    }

    std::vector<Sink> sinks() const
    {
        std::vector<Sink> snks;
        for (const spd::sink_ptr& sink : _logger->sinks()) {
            snks.push_back(Sink(sink));
        }
        return snks;
    }

    void set_error_handler(spd::log_err_handler handler)
    {
        _logger->set_error_handler(handler);
    }

    spd::log_err_handler error_handler()
    {
        return _logger->error_handler();
    }

    std::shared_ptr<spdlog::logger> get_underlying_logger() {
        return _logger;
    }

protected:
    const std::string _name;
    bool _async;
    std::shared_ptr<spdlog::logger> _logger{ nullptr };
};

class ConsoleLogger : public Logger {
public:
    ConsoleLogger(const std::string& logger_name, bool multithreaded, bool standard_out, bool colored, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (standard_out) {
            if (multithreaded) {
                if (colored) {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stdout_color_mt<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stdout_color_mt<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stdout_color_mt(logger_name);
                    }
                } else {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stdout_logger_mt<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stdout_logger_mt<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stdout_logger_mt(logger_name);
                    }
                }
            } else {
                if (colored) {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stdout_color_st<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stdout_color_st<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stdout_color_st(logger_name);
                    }
                } else {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stdout_logger_st<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stdout_logger_st<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stdout_logger_st(logger_name);
                    }
                }
            }

        } else {
            if (multithreaded) {
                if (colored) {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stderr_color_mt<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stderr_color_mt<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stderr_color_mt(logger_name);
                    }
                } else {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stderr_logger_mt<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stderr_logger_mt<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stderr_logger_mt(logger_name);
                    }
                }
            } else {
                if (colored) {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stderr_color_st<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stderr_color_st<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stderr_color_st(logger_name);
                    }
                } else {
                    if (async_mode) {
                        if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                            _logger = spd::stderr_logger_st<async_factory_nb>(logger_name);
                        } else {
                            _logger = spd::stderr_logger_st<spdlog::async_factory>(logger_name);
                        }
                    } else {
                        _logger = spd::stderr_logger_st(logger_name);
                    }
                }
            }
        }
    }
};

class FileLogger : public Logger {
public:
    FileLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, bool truncate = false, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (multithreaded) {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::basic_logger_mt<async_factory_nb>(logger_name, filename, truncate);
                } else {
                    _logger = spd::basic_logger_mt<spdlog::async_factory>(logger_name, filename, truncate);
                }
            } else {
                _logger = spd::basic_logger_mt(logger_name, filename, truncate);
            }
        } else {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::basic_logger_st<async_factory_nb>(logger_name, filename, truncate);
                } else {
                    _logger = spd::basic_logger_st<spdlog::async_factory>(logger_name, filename, truncate);
                }
            } else {
                _logger = spd::basic_logger_st(logger_name, filename, truncate);
            }
        }
    }
};

class RotatingLogger : public Logger {
public:
    RotatingLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, size_t max_file_size, size_t max_files, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (multithreaded) {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::rotating_logger_mt<async_factory_nb>(logger_name, filename, max_file_size, max_files);
                } else {
                    _logger = spd::rotating_logger_mt<spdlog::async_factory>(logger_name, filename, max_file_size, max_files);
                }
            } else {
                _logger = spd::rotating_logger_mt(logger_name, filename, max_file_size, max_files);
            }
        } else {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::rotating_logger_st<async_factory_nb>(logger_name, filename, max_file_size, max_files);
                } else {
                    _logger = spd::rotating_logger_st<spdlog::async_factory>(logger_name, filename, max_file_size, max_files);
                }
            } else {
                _logger = spd::rotating_logger_st(logger_name, filename, max_file_size, max_files);
            }
        }
    }
};

class DailyLogger : public Logger {
public:
    DailyLogger(const std::string& logger_name, const std::string& filename, bool multithreaded = false, int hour = 0, int minute = 0, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (multithreaded) {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::daily_logger_mt<async_factory_nb>(logger_name, filename, hour, minute);
                } else {
                    _logger = spd::daily_logger_mt<spdlog::async_factory>(logger_name, filename, hour, minute);
                }
            } else {
                _logger = spd::daily_logger_mt(logger_name, filename, hour, minute);
            }
        } else {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::daily_logger_st<async_factory_nb>(logger_name, filename, hour, minute);
                } else {
                    _logger = spd::daily_logger_st<spdlog::async_factory>(logger_name, filename, hour, minute);
                }
            } else {
                _logger = spd::daily_logger_st(logger_name, filename, hour, minute);
            }
        }
    }
};

#ifdef SPDLOG_ENABLE_SYSLOG
class SyslogLogger : public Logger {
public:
    SyslogLogger(const std::string& logger_name, bool multithreaded = false, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1 << 3), bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (multithreaded) {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::syslog_logger_mt<async_factory_nb>(logger_name, ident, syslog_option, syslog_facilty);
                } else {
                    _logger = spd::syslog_logger_mt<spdlog::async_factory>(logger_name, ident, syslog_option, syslog_facilty);
                }
            } else {
                _logger = spd::syslog_logger_mt(logger_name, ident, syslog_option, syslog_facilty);
            }
        } else {
            if (async_mode) {
                if (g_async_overflow_policy == spdlog::async_overflow_policy::overrun_oldest) {
                    _logger = spd::syslog_logger_st<async_factory_nb>(logger_name, ident, syslog_option, syslog_facilty);
                } else {
                    _logger = spd::syslog_logger_st<spdlog::async_factory>(logger_name, ident, syslog_option, syslog_facilty);
                }
            } else {
                _logger = spd::syslog_logger_st(logger_name, ident, syslog_option, syslog_facilty);
            }
        }
    }
};
#endif

class AsyncOverflowPolicy {
public:
    const static int block{ (int)spd::async_overflow_policy::block };
    const static int overrun_oldest{ (int)spd::async_overflow_policy::overrun_oldest };
};

void set_async_mode(size_t queue_size = spdlog::details::default_async_q_size, size_t thread_count = 1, int async_overflow_policy = AsyncOverflowPolicy::block) {
    // Initialize/replace the global spdlog thread pool.
    auto& registry = spdlog::details::registry::instance();
    std::lock_guard<std::recursive_mutex> tp_lck(registry.tp_mutex());
    auto tp = std::make_shared<spd::details::thread_pool>(queue_size, thread_count);
    registry.set_tp(tp);

    g_async_overflow_policy = static_cast<spd::async_overflow_policy>(async_overflow_policy);
    g_async_mode_on = true;
}

std::shared_ptr<spdlog::details::thread_pool> thread_pool() {
    auto& registry = spdlog::details::registry::instance();
    std::lock_guard<std::recursive_mutex> tp_lck(registry.tp_mutex());
    auto tp = registry.get_tp();
    if(tp == nullptr) {
        set_async_mode();
        auto tp = registry.get_tp();
    }

    return tp;
}

class SinkLogger : public Logger {
public:
    SinkLogger(const std::string& logger_name, const Sink& sink, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        if (async_mode) {
            _logger = std::shared_ptr<spd::async_logger>(new spd::async_logger(logger_name, sink.get_sink(), thread_pool(), g_async_overflow_policy));
        } else {
            _logger = std::shared_ptr<spd::logger>(new spd::logger(logger_name, sink.get_sink()));
        }
    }
    SinkLogger(const std::string& logger_name, const std::vector<Sink>& sink_list, bool async_mode = g_async_mode_on)
        : Logger(logger_name, async_mode)
    {
        std::vector<spd::sink_ptr> sinks;
        for (auto sink : sink_list)
            sinks.push_back(sink.get_sink());

        if (async_mode) {
            _logger = std::shared_ptr<spd::async_logger>(new spd::async_logger(logger_name, sinks.begin(), sinks.end(), thread_pool(), g_async_overflow_policy));
        } else {
            _logger = std::shared_ptr<spd::logger>(new spd::logger(logger_name, sinks.begin(), sinks.end()));
        }
    }
};

Logger get(const std::string& name)
{
    Logger* logger = access_logger(name);
    if (logger)
        return *logger;
    else
        throw std::runtime_error(std::string("Logger name: " + name + " could not be found"));
}

void drop(const std::string& name)
{
    remove_logger(name);
    spdlog::drop(name);
}

void drop_all()
{
    remove_logger_all();
    spdlog::drop_all();
}

}

PYBIND11_MODULE(spdlog, m)
{
    m.doc() = R"pbdoc(
        spdlog module
        -----------------------

        .. currentmodule:: spdlog

        .. autosummary::
           :toctree: _generate

           LogLevel
           Logger
    )pbdoc";

    py::class_<spd::logger, std::shared_ptr<spd::logger>>(m, "_spd_logger");

    m.def("set_async_mode", set_async_mode,
        py::arg("queue_size") = 1 << 16,
        py::arg("thread_count") = 1,
        py::arg("overflow_policy") = 0);

    py::class_<Sink>(m, "Sink")
        .def(py::init<>())
        .def("set_level", &Sink::set_level);

    py::class_<stdout_sink_st, Sink>(m, "stdout_sink_st")
        .def(py::init<>());

    py::class_<stdout_sink_mt, Sink>(m, "stdout_sink_mt")
        .def(py::init<>());

    py::class_<stdout_color_sink_st, Sink>(m, "stdout_color_sink_st")
        .def(py::init<>());

    py::class_<stdout_color_sink_mt, Sink>(m, "stdout_color_sink_mt")
        .def(py::init<>());

    py::class_<stderr_sink_st, Sink>(m, "stderr_sink_st")
        .def(py::init<>());

    py::class_<stderr_sink_mt, Sink>(m, "stderr_sink_mt")
        .def(py::init<>());

    py::class_<stderr_color_sink_st, Sink>(m, "stderr_color_sink_st")
        .def(py::init<>());

    py::class_<stderr_color_sink_mt, Sink>(m, "stderr_color_sink_mt")
        .def(py::init<>());

    py::class_<basic_file_sink_st, Sink>(m, "basic_file_sink_st")
        .def(py::init<std::string, bool>(), py::arg("filename"), py::arg("truncate") = false);

    py::class_<basic_file_sink_mt, Sink>(m, "basic_file_sink_mt")
        .def(py::init<std::string, bool>(), py::arg("filename"), py::arg("truncate") = false);

    py::class_<daily_file_sink_st, Sink>(m, "daily_file_sink_st")
        .def(py::init<std::string, int, int>(), py::arg("filename"),
            py::arg("rotation_hour"),
            py::arg("rotation_minute"));

    py::class_<daily_file_sink_mt, Sink>(m, "daily_file_sink_mt")
        .def(py::init<std::string, int, int>(), py::arg("filename"),
            py::arg("rotation_hour"),
            py::arg("rotation_minute"));

    py::class_<rotating_file_sink_st, Sink>(m, "rotating_file_sink_st")
        .def(py::init<std::string, int, int>(), py::arg("filename"),
            py::arg("max_size"),
            py::arg("max_files"));

    py::class_<rotating_file_sink_mt, Sink>(m, "rotating_file_sink_mt")
        .def(py::init<std::string, int, int>(), py::arg("filename"),
            py::arg("max_size"),
            py::arg("max_files"));

    py::class_<null_sink_st, Sink>(m, "null_sink_st")
        .def(py::init<>());

    py::class_<null_sink_mt, Sink>(m, "null_sink_mt")
        .def(py::init<>());

    py::class_<LogLevel>(m, "LogLevel")
        .def_property_readonly_static("TRACE", [](py::object) { return LogLevel::trace; })
        .def_property_readonly_static("DEBUG", [](py::object) { return LogLevel::debug; })
        .def_property_readonly_static("INFO", [](py::object) { return LogLevel::info; })
        .def_property_readonly_static("WARN", [](py::object) { return LogLevel::warn; })
        .def_property_readonly_static("ERR", [](py::object) { return LogLevel::err; })
        .def_property_readonly_static("CRITICAL", [](py::object) { return LogLevel::critical; })
        .def_property_readonly_static("OFF", [](py::object) { return LogLevel::off; });

    py::class_<AsyncOverflowPolicy>(m, "AsyncOverflowPolicy")
        .def_property_readonly_static("BLOCK", [](py::object) { return AsyncOverflowPolicy::block; })
        .def_property_readonly_static("OVERRUN_OLDEST", [](py::object) { return AsyncOverflowPolicy::overrun_oldest; });

    py::enum_<spdlog::pattern_time_type>(m, "PatternTimeType")
        .value("local", spdlog::pattern_time_type::local)
        .value("utc", spdlog::pattern_time_type::utc)
        .export_values();

    py::class_<Logger>(m, "Logger")
        .def("log", &Logger::log)
        .def("trace", &Logger::trace)
        .def("debug", &Logger::debug)
        .def("info", &Logger::info)
        .def("warn", &Logger::warn)
        .def("error", &Logger::error)
        .def("critical", &Logger::critical)
        .def("name", &Logger::name)
        .def("should_log", &Logger::should_log)
        .def("set_level", &Logger::set_level)
        .def("level", &Logger::level)
        .def("set_pattern", &Logger::set_pattern,
            py::arg("pattern"), py::arg("type") = spd::pattern_time_type::local)
        .def("flush_on", &Logger::flush_on)
        .def("flush", &Logger::flush)
        .def("close", &Logger::close)
        .def("async_mode", &Logger::async)
        .def("sinks", &Logger::sinks)
        .def("set_error_handler", &Logger::set_error_handler)
        .def("error_handler", &Logger::error_handler)
        .def("get_underlying_logger", &Logger::get_underlying_logger);

    py::class_<SinkLogger, Logger>(m, "SinkLogger")
    .def(py::init<const std::string&, const std::vector<Sink>&>(),
        py::arg("name"),
        py::arg("sinks"))
    .def(py::init<const std::string&, const std::vector<Sink>&, bool>(),
        py::arg("name"),
        py::arg("sinks"),
        py::arg("async_mode"));

py::class_<ConsoleLogger, Logger>(m, "ConsoleLogger")
    .def(py::init<std::string, bool, bool, bool>(),
        py::arg("name"),
        py::arg("multithreaded") = false,
        py::arg("stdout") = true,
        py::arg("colored") = true)
    .def(py::init<std::string, bool, bool, bool, bool>(),
        py::arg("name"),
        py::arg("multithreaded") = false,
        py::arg("stdout") = true,
        py::arg("colored") = true,
        py::arg("async_mode"));

py::class_<FileLogger, Logger>(m, "FileLogger")
    .def(py::init<std::string, std::string, bool, bool>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded") = false,
        py::arg("truncate") = false)
    .def(py::init<std::string, std::string, bool, bool, bool>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded") = false,
        py::arg("truncate") = false,
        py::arg("async_mode"));
py::class_<RotatingLogger, Logger>(m, "RotatingLogger")
    .def(py::init<std::string, std::string, bool, int, int>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded"),
        py::arg("max_file_size"),
        py::arg("max_files"))
    .def(py::init<std::string, std::string, bool, int, int, bool>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded"),
        py::arg("max_file_size"),
        py::arg("max_files"),
        py::arg("async_mode"));
py::class_<DailyLogger, Logger>(m, "DailyLogger")
    .def(py::init<std::string, std::string, bool, int, int>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded") = false,
        py::arg("hour") = 0,
        py::arg("minute") = 0)
    .def(py::init<std::string, std::string, bool, int, int, bool>(),
        py::arg("name"),
        py::arg("filename"),
        py::arg("multithreaded") = false,
        py::arg("hour") = 0,
        py::arg("minute") = 0,
        py::arg("async_mode"));

//SyslogLogger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1<<3))
#ifdef SPDLOG_ENABLE_SYSLOG
py::class_<syslog_sink_st, Sink>(m, "syslog_sink_st")
    .def(py::init<std::string, int, int>(),
        py::arg("ident") = "",
        py::arg("syslog_option") = 0,
        py::arg("syslog_facility") = (1 << 3));
py::class_<syslog_sink_mt, Sink>(m, "syslog_sink_mt")
    .def(py::init<std::string, int, int>(),
        py::arg("ident") = "",
        py::arg("syslog_option") = 0,
            py::arg("syslog_facility") = (1 << 3));
    py::class_<SyslogLogger, Logger>(m, "SyslogLogger")
        .def(py::init<std::string, bool, std::string, int, int>(),
            py::arg("name"),
            py::arg("multithreaded") = false,
            py::arg("ident") = "",
            py::arg("syslog_option") = 0,
            py::arg("syslog_facility") = (1 << 3))
        .def(py::init<std::string, bool, std::string, int, int, bool>(),
            py::arg("name"),
            py::arg("multithreaded") = false,
            py::arg("ident") = "",
            py::arg("syslog_option") = 0,
            py::arg("syslog_facility") = (1 << 3),
            py::arg("async_mode"));
#endif
    m.def("get", get, py::arg("name"), py::return_value_policy::copy);
    m.def("drop", drop, py::arg("name"));
    m.def("drop_all", drop_all);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
