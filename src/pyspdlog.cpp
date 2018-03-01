#ifndef _WIN32
#define SPDLOG_ENABLE_SYSLOG
#endif

#include <pybind11/pybind11.h>

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

namespace spd = spdlog;
namespace py = pybind11;

namespace { // Avoid cluttering the global namespace.

class LogLevel
{
public:
    const static int trace {(int)spd::level::trace};
    const static int debug { (int)spd::level::debug };
    const static int info { (int)spd::level::info };
    const static int warn { (int)spd::level::warn };
    const static int err { (int)spd::level::err };
    const static int critical { (int)spd::level::critical };
    const static int off { (int)spd::level::off };
};


class Sink
{
public:
    Sink(){}
    Sink(const spd::sink_ptr& sink) : _sink(sink) {}
    virtual ~Sink() {}
    virtual void log(const spd::details::log_msg& msg)
    {
        _sink->log(msg);
    }
    bool should_log(spd::level::level_enum msg_level) const
    {
        return _sink->should_log(msg_level);
    }
    void set_level(spd::level::level_enum log_level)
    {
        _sink->set_level(log_level);
    }
    spd::level::level_enum level() const
    {
        return _sink->level();
    }

    spd::sink_ptr get_sink() const { return _sink; }

protected:
    spd::sink_ptr _sink{nullptr};
};


class Logger
{
public:
    Logger() {}
    Logger(const std::shared_ptr<spd::logger>& logger) : _logger(logger) {}
    virtual ~Logger() {}
    std::string name() const 
    {
        if(_logger) 
            return _logger->name(); 
        else
            return "NULL";
    }
    void log(int level, const std::string& msg) const { this->_logger->log((spd::level::level_enum)level ,msg); }
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

    std::vector<Sink> sinks() const
    {
        std::vector<Sink> snks;
        for(const spd::sink_ptr& sink : _logger->sinks())
        {
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
    std::shared_ptr<spdlog::logger> _logger{nullptr};
};


class ConsoleLogger : public Logger
{
public:
    ConsoleLogger(const std::string& logger_name, bool multithreaded, bool stdout, bool colored) 
    {
        if(stdout)
        {
            if(multithreaded)
            {
                if(colored)
                    _logger = spd::stdout_color_mt(logger_name);
                else
                    _logger = spd::stdout_logger_mt(logger_name);
            }
            else
            {
                if(colored)
                    _logger = spd::stdout_color_st(logger_name);
                else
                    _logger = spd::stdout_logger_st(logger_name);

            }

        }
        else
        {
            if(multithreaded)
            {
                if(colored)
                    _logger = spd::stderr_color_mt(logger_name);
                else
                    _logger = spd::stderr_logger_mt(logger_name);
            }
            else
            {
                if(colored)
                    _logger = spd::stderr_color_st(logger_name);
                else
                    _logger = spd::stderr_logger_st(logger_name);

            }

        }

    }
};

class FileLogger : public Logger
{
public:
    FileLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, bool truncate = false) 
    {
        if(multithreaded)
        {
            _logger = spd::basic_logger_mt(logger_name, filename, truncate);
        }
        else
        {
            _logger = spd::basic_logger_st(logger_name, filename, truncate);
        }
    }
};

class RotatingLogger : public Logger
{
public:
    RotatingLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, size_t max_file_size, size_t max_files) 
    {
        if(multithreaded)
        {
            _logger = spd::rotating_logger_mt(logger_name, filename, max_file_size, max_files);
        }
        else
        {
            _logger = spd::rotating_logger_st(logger_name, filename, max_file_size, max_files);
        }
    }
};

class DailyLogger : public Logger
{
public:
    DailyLogger(const std::string& logger_name, const std::string& filename, bool multithreaded=false, int hour=0, int minute=0) 
    {
        if(multithreaded)
        {
            _logger = spd::daily_logger_mt(logger_name, filename, hour, minute);
        }
        else
        {
            _logger = spd::daily_logger_st(logger_name, filename, hour, minute);
        }
    }
};


#ifdef SPDLOG_ENABLE_SYSLOG
class SyslogLogger : public Logger
{
public:
    SyslogLogger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1<<3))
    {
       _logger = spd::syslog_logger(logger_name, ident, syslog_option, syslog_facilty);
    }
};
#endif



class SinkLogger : public Logger
{
public:
    SinkLogger(const std::string& logger_name, const Sink& sink) : Logger(std::shared_ptr<spd::logger>( new spd::logger(logger_name, sink.get_sink())))
    {}
    SinkLogger(const std::string& logger_name, const std::vector<Sink>& sink_list) : Logger()
    {
        std::vector<spd::sink_ptr> sinks;
        for(const Sink& sink : sink_list)
            sinks.push_back(sink.get_sink());
        _logger = std::shared_ptr<spd::logger>(
                new spd::logger(logger_name, sinks.begin(), sinks.end()));
    }
};


Logger get(const std::string& name)
{
    Logger logger(spd::get(name));
    return logger;
}


void drop(const std::string& name)
{
    spdlog::drop(name);
}


void drop_all()
{
    spdlog::drop_all();
}



}

PYBIND11_MODULE(spdlog, m) {
    m.doc() = R"pbdoc(
        spdlog module
        -----------------------

        .. currentmodule:: spdlog

        .. autosummary::
           :toctree: _generate

           LogLevel
           Logger
    )pbdoc";

    py::class_<LogLevel>(m, "LogLevel")
        .def_property_readonly_static("TRACE", [](py::object) {return LogLevel::trace;})
        .def_property_readonly_static("DEBUG", [](py::object) {return LogLevel::debug;})
        .def_property_readonly_static("INFO", [](py::object) {return LogLevel::info;})
        .def_property_readonly_static("WARN", [](py::object) {return LogLevel::warn;})
        .def_property_readonly_static("ERR", [](py::object) {return LogLevel::err;})
        .def_property_readonly_static("CRITICAL", [](py::object) {return LogLevel::critical;})
        .def_property_readonly_static("OFF", [](py::object) {return LogLevel::off;})
        ;

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
        .def("set_pattern", &Logger::set_pattern)
        .def("flush_on", &Logger::flush_on)
        .def("flush", &Logger::flush)
        .def("sinks", &Logger::sinks)
        .def("set_error_handler", &Logger::set_error_handler)
        .def("error_handler", &Logger::error_handler)
        ;


    py::class_<ConsoleLogger, Logger>(m, "ConsoleLogger")
        .def(py::init<std::string, bool, bool, bool>(), 
                py::arg("name"),
                py::arg("multithreaded") = false, 
                py::arg("stdout") = true, 
                py::arg("colored") = true
            )
        ;

    py::class_<FileLogger, Logger>(m, "FileLogger") 
        .def(py::init<std::string, std::string, bool, bool>(),
                py::arg("name"),
                py::arg("filename"), 
                py::arg("multithreaded") = false, 
                py::arg("truncate") = false
            )
        ;
    py::class_<RotatingLogger, Logger>(m, "RotatingLogger")
        .def(py::init<std::string, std::string, bool, bool, bool>(),
                py::arg("name"),
                py::arg("filename"), 
                py::arg("multithreaded"), 
                py::arg("max_file_size"),
                py::arg("max_files")
            )
        ;
    py::class_<DailyLogger, Logger>(m, "DailyLogger")
        .def(py::init<std::string, std::string, bool, int, int>(),
                py::arg("name"),
                py::arg("filename"), 
                py::arg("multithreaded") = false, 
                py::arg("hour") = 0,
                py::arg("minute") = 0
            )
       ;
//SyslogLogger(const std::string& logger_name, const std::string& ident = "", int syslog_option = 0, int syslog_facilty = (1<<3))
#ifdef SPDLOG_ENABLE_SYSLOG
    py::class_<SyslogLogger, Logger>(m, "SyslogLogger")
       .def(py::init<std::string, std::string, int, int>(),
                py::arg("name"),
                py::arg("ident") = "", 
                py::arg("syslog_option") = 0, 
                py::arg("syslog_facility") = (1<<3)
            )
       ;
#endif
    m.def("get", get, py::arg("name"));
    m.def("drop", drop, py::arg("name"));
    m.def("drop_all", drop_all);

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}


/*
BOOST_PYTHON_MODULE(spdlog)
{
    bp::class_<LogLevel>("LogLevel")
        .add_static_property("TRACE", &LogLevel::trace)  
        .add_static_property("DEBUG", &LogLevel::debug)  
        .add_static_property("INFO", &LogLevel::info)  
        .add_static_property("WARN", &LogLevel::warn)  
        .add_static_property("ERR", &LogLevel::err)  
        .add_static_property("CRITICAL", &LogLevel::critical)  
        .add_static_property("OFF", &LogLevel::off)  
        ;
    bp::class_<Logger>("Logger")
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
        .def("set_pattern", &Logger::set_pattern)
        .def("flush_on", &Logger::flush_on)
        .def("flush", &Logger::flush)
        .def("sinks", &Logger::sinks)
        .def("set_error_handler", &Logger::set_error_handler)
        .def("error_handler", &Logger::error_handler)
        ;
    bp::class_<ConsoleLogger, bp::bases<Logger>>("ConsoleLogger", bp::init<std::string, bool, bool, bool>())
        ;
    bp::class_<FileLogger, bp::bases<Logger>>("FileLogger", bp::init<std::string, std::string, bool, bool>())
        ;
    bp::class_<RotatingLogger, bp::bases<Logger>>("RotatingLogger", bp::init<std::string, std::string, bool, bool, bool>())
        ;
    bp::class_<DailyLogger, bp::bases<Logger>>("DailyLogger", bp::init<std::string, std::string, bool, int, int>())
        ;

#ifdef SPDLOG_ENABLE_SYSLOG
    bp::class_<SyslogLogger, bp::bases<Logger>>("SyslogLogger", bp::init<std::string, std::string, int, int>())
        ;
#endif
    bp::def("get", get);
    bp::def("drop_all", drop_all);
}

*/
