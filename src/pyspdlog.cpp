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
//#include <spdlog/sinks/stdout_sinks.h>
#ifdef WIN32
#include <spdlog/sinks/wincolor_sink.h>
#else
#include <spdlog/sinks/ansicolor_sink.h>
#endif
////#include <spdlog/sinks/simple_file_sink.h>
////#include <spdlog/sinks/daily_file_sink.h>
//#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/null_sink.h>
//#include <spdlog/sinks/syslog_sink.h>
//#include <spdlog/async_logger.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace spd = spdlog;
namespace py = pybind11;

namespace spdlog {
namespace sinks {
#ifdef WIN32
    using stdout_color_sink_st = wincolor_stdout_sink_st;
    using stdout_color_sink_mt = wincolor_stdout_sink_mt;
    using stderr_color_sink_st = wincolor_stderr_sink_st;
    using stderr_color_sink_mt = wincolor_stderr_sink_mt;
#else
    using stdout_color_sink_st = ansicolor_stdout_sink_st;
    using stdout_color_sink_mt = ansicolor_stdout_sink_mt;
    using stderr_color_sink_st = ansicolor_stderr_sink_st;
    using stderr_color_sink_mt = ansicolor_stderr_sink_mt;
#endif
}
}

namespace { // Avoid cluttering the global namespace.

class Logger;

bool g_async_mode_on = false;
std::mutex mutex_async_mode_on;

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
bool is_async_mode_on()
{
    std::lock_guard<std::mutex> lck(mutex_async_mode_on);
    return g_async_mode_on;
}

void change_async_mode_on(bool val)
{
    std::lock_guard<std::mutex> lck(mutex_async_mode_on);
    g_async_mode_on = val;
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

class simple_file_sink_st : public Sink {
public:
    simple_file_sink_st(const std::string& base_filename, bool truncate)
    {
        _sink = std::make_shared<spdlog::sinks::simple_file_sink_st>(base_filename, truncate);
    }
};

class simple_file_sink_mt : public Sink {
public:
    simple_file_sink_mt(const std::string& base_filename, bool truncate)
    {
        _sink = std::make_shared<spdlog::sinks::simple_file_sink_mt>(base_filename, truncate);
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
class syslog_sink : public Sink {
public:
    syslog_sink(const std::string& ident = "", int syslog_option = 0, int syslog_facility = (1 << 3))
    {
        _sink = std::make_shared<spdlog::sinks::syslog_sink>(ident, syslog_option, syslog_facility);
    }
};
#endif

class Logger {
public:
    Logger(const std::string& name)
        : _name(name)
        , _async(is_async_mode_on())
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
        if (!_async)
            _logger->flush_on((spd::level::level_enum)log_level);
        else
            throw std::runtime_error("Can only flush explicitly on sync logger!");
    }

    void flush()
    {
        if (!_async)
            _logger->flush();
        else
            throw std::runtime_error("Can only flush explicitly on sync logger!");
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

protected:
    const std::string _name;
    bool _async;
    std::shared_ptr<spdlog::logger> _logger{ nullptr };
};

class ConsoleLogger : public Logger {
public:
    ConsoleLogger(const std::string& logger_name, bool multithreaded, bool stdout, bool colored)
        : Logger(logger_name)
    {
        if (stdout) {
            if (multithreaded) {
                if (colored)
                    _logger = spd::stdout_color_mt(logger_name);
                else
                    _logger = spd::stdout_logger_mt(logger_name);
            } else {
                if (colored)
                    _logger = spd::stdout_color_st(logger_name);
                else
                    _logger = spd::stdout_logger_st(logger_name);
            }

        } else {
            if (multithreaded) {
                if (colored)
                    _logger = spd::stderr_color_mt(logger_name);
                else
                    _logger = spd::stderr_logger_mt(logger_name);
            } else {
                if (colored)
                    _logger = spd::stderr_color_st(logger_name);
                else
                    _logger = spd::stderr_logger_st(logger_name);
            }
        }
    }
};

class FileLogger : public Logger {
public:
    FileLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, bool truncate = false)
        : Logger(logger_name)
    {
        if (multithreaded) {
            _logger = spd::basic_logger_mt(logger_name, filename, truncate);
        } else {
            _logger = spd::basic_logger_st(logger_name, filename, truncate);
        }
    }
};

class RotatingLogger : public Logger {
public:
    RotatingLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, size_t max_file_size, size_t max_files)
        : Logger(logger_name)
    {
        if (multithreaded) {
            _logger = spd::rotating_logger_mt(logger_name, filename, max_file_size, max_files);
        } else {
            _logger = spd::rotating_logger_st(logger_name, filename, max_file_size, max_files);
        }
    }
};

class DailyLogger : public Logger {
public:
    DailyLogger(const std::string& logger_name, const std::string& filename, bool multithreaded = false, int hour = 0, int minute = 0)
        : Logger(logger_name)
    {
        if (multithreaded) {
            _logger = spd::daily_logger_mt(logger_name, filename, hour, minute);
        } else {
            _logger = spd::daily_logger_st(logger_name, filename, hour, minute);
        }
    }
};

#ifdef SPDLOG_ENABLE_SYSLOG
class SyslogLogger : public Logger {
public:
    SyslogLogger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1 << 3))
        : Logger(logger_name)
    {
        _logger = spd::syslog_logger(logger_name, ident, syslog_option, syslog_facilty);
    }
};
#endif

class SinkLogger : public Logger {
public:
    SinkLogger(const std::string& logger_name, const Sink& sink)
        : Logger(logger_name)
    {
        _logger = std::shared_ptr<spd::logger>(new spd::logger(logger_name, sink.get_sink()));
    }
    SinkLogger(const std::string& logger_name, const std::vector<Sink>& sink_list)
        : Logger(logger_name)
    {
        std::vector<spd::sink_ptr> sinks;
        for (auto sink : sink_list)
            sinks.push_back(sink.get_sink());
        _logger = std::shared_ptr<spd::logger>(
            new spd::logger(logger_name, sinks.begin(), sinks.end()));
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

class AsyncOverflowPolicy {
public:
    const static int block_retry{ (int)spd::async_overflow_policy::block_retry };
    const static int discard_log_msg{ (int)spd::async_overflow_policy::discard_log_msg };
};

void set_async_mode(size_t queue_size, int policy = 0, const std::function<void()>& worker_warmup_cb = nullptr, size_t flush_interval_ms = 0, const std::function<void()>& worker_teardown_cb = nullptr)
{
    if (flush_interval_ms == 0)
        throw std::runtime_error("Zero is invalid value for flush interval millisec!");
    spd::set_async_mode(queue_size, (spd::async_overflow_policy)policy, worker_warmup_cb, std::chrono::milliseconds(flush_interval_ms), worker_teardown_cb);
    change_async_mode_on(true);
}

// Turn off async mode
void set_sync_mode()
{
    spd::set_sync_mode();
    change_async_mode_on(false);
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

    m.def("set_async_mode", set_async_mode,
        py::arg("queue_size") = 1 << 16,
        py::arg("async_overflow_policy") = 0,
        py::arg("worker_warmup_cb") = nullptr,
        py::arg("flush_interval_ms") = 10,
        py::arg("worker_teardown_cb") = nullptr);

    m.def("set_sync_mode", set_sync_mode);

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

    py::class_<simple_file_sink_st, Sink>(m, "simple_file_sink_st")
        .def(py::init<std::string, bool>(), py::arg("filename"), py::arg("truncate") = false);

    py::class_<simple_file_sink_mt, Sink>(m, "simple_file_sink_mt")
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
        .def_property_readonly_static("BLOCK", [](py::object) { return AsyncOverflowPolicy::block_retry; })
        .def_property_readonly_static("DISCARD_LOG_MSG", [](py::object) { return AsyncOverflowPolicy::discard_log_msg; });

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
        .def("error_handler", &Logger::error_handler);

    py::class_<SinkLogger, Logger>(m, "SinkLogger")
        .def(py::init<const std::string&, const std::vector<Sink>&>());

    py::class_<ConsoleLogger, Logger>(m, "ConsoleLogger")
        .def(py::init<std::string, bool, bool, bool>(),
            py::arg("name"),
            py::arg("multithreaded") = false,
            py::arg("stdout") = true,
            py::arg("colored") = true);

    py::class_<FileLogger, Logger>(m, "FileLogger")
        .def(py::init<std::string, std::string, bool, bool>(),
            py::arg("name"),
            py::arg("filename"),
            py::arg("multithreaded") = false,
            py::arg("truncate") = false);
    py::class_<RotatingLogger, Logger>(m, "RotatingLogger")
        .def(py::init<std::string, std::string, bool, int, int>(),
            py::arg("name"),
            py::arg("filename"),
            py::arg("multithreaded"),
            py::arg("max_file_size"),
            py::arg("max_files"));
    py::class_<DailyLogger, Logger>(m, "DailyLogger")
        .def(py::init<std::string, std::string, bool, int, int>(),
            py::arg("name"),
            py::arg("filename"),
            py::arg("multithreaded") = false,
            py::arg("hour") = 0,
            py::arg("minute") = 0);

//SyslogLogger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1<<3))
#ifdef SPDLOG_ENABLE_SYSLOG
    py::class_<syslog_sink, Sink>(m, "syslog_sink")
        .def(py::init<std::string, int, int>(),
            py::arg("ident") = "",
            py::arg("syslog_option") = 0,
            py::arg("syslog_facility") = (1 << 3));
    py::class_<SyslogLogger, Logger>(m, "SyslogLogger")
        .def(py::init<std::string, std::string, int, int>(),
            py::arg("name"),
            py::arg("ident") = "",
            py::arg("syslog_option") = 0,
            py::arg("syslog_facility") = (1 << 3));
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
