import spdlog
import unittest

from spdlog import ConsoleLogger, FileLogger, RotatingLogger, DailyLogger, LogLevel
    
def set_log_level(logger, level):
    print("Setting Log level to %d" % level)
    logger.set_level(level)


def log_msg(logger):
    logger.trace('I am Trace')
    logger.debug('I am Debug')
    logger.info('I am Info')
    logger.warn('I am Warning')
    logger.error('I am Error')
    logger.critical('I am Critical')


class SpdLogTest(unittest.TestCase):
    def test_console_logger(self):
        name = 'Console Logger'
        tf = (True, False)
        for multithreaded in tf:
            for stdout in tf:
                for colored in tf:
                    logger = ConsoleLogger(name, multithreaded, stdout, colored) 
                    logger.info('I am a console log test.')
                    spdlog.drop(name)

    def test_drop(self):
        name = 'Console Logger'
        for i in range(10):
            tf = (True, False)
            for multithreaded in tf:
                for stdout in tf:
                    for colored in tf:
                        logger = ConsoleLogger(name, multithreaded, stdout, colored) 
                        spdlog.drop(logger.name())
    def test_log_level(self):
        logger = ConsoleLogger('Logger', False, True, True)
        for level in (LogLevel.TRACE, LogLevel.DEBUG, LogLevel.INFO, LogLevel.WARN,
                LogLevel.ERR, LogLevel.CRITICAL):
            set_log_level(logger, level)
            log_msg(logger)
    
    
       
if __name__ == "__main__":
    unittest.main()

