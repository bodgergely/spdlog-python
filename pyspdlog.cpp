
#include "spdlog/spdlog.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

namespace spd = spdlog;


namespace { // Avoid cluttering the global namespace.


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
    void info(const std::string& msg) const 
    {
        this->_logger->info(msg); 
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
    DailyLogger(const std::string& logger_name, const std::string& filename, bool multithreaded, int hour=0, int minute=0) 
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


void drop_all()
{
    spdlog::drop_all();
}



}

BOOST_PYTHON_MODULE(spdlog)
{
    using namespace boost::python;
    class_<Logger>("Logger")
        .def("info", &Logger::info)
        .def("name", &Logger::name)
        ;
    class_<ConsoleLogger>("ConsoleLogger", init<std::string, bool, bool, bool>())
        .def("info", &ConsoleLogger::info)
        .def("name", &ConsoleLogger::name)
        ;
    class_<FileLogger>("FileLogger", init<std::string, std::string, bool, bool>())
        .def("info", &FileLogger::info)
        .def("name", &Logger::name)
        ;
    class_<RotatingLogger>("RotatingLogger", init<std::string, std::string, bool, bool, bool>())
        .def("info", &RotatingLogger::info)
        .def("name", &RotatingLogger::name)
        ;
    class_<DailyLogger>("DailyLogger", init<std::string, std::string, bool, int, int>())
        .def("info", &DailyLogger::info)
        .def("name", &DailyLogger::name)
        ;


    def("get", get);
    def("drop_all", drop_all);
}
