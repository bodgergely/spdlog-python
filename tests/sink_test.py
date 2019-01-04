import spdlog
import time
import os

sinks = [
    spdlog.stdout_sink_st(),
    spdlog.stdout_sink_mt(),
    spdlog.stderr_sink_st(),
    spdlog.stderr_sink_mt(),
    spdlog.daily_file_sink_st("DailySinkSt.log", 0, 0),
    spdlog.daily_file_sink_mt("DailySinkMt.log", 0, 0),
    spdlog.rotating_file_sink_st("RotSt.log", 1024, 1024),
    spdlog.rotating_file_sink_mt("RotMt.log", 1024, 1024),
]


logger = spdlog.SinkLogger("Hello", sinks)
logger.info("Kukucs")
logger.info("Alma")