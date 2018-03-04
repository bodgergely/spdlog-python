pyspdlog
==========
python wrapper around C++ spdlog (git@github.com:gabime/spdlog.git)

Introduction
============

Python wrapper (pybind11) around the C++ spdlog logging library. 

Why choose spdlog?

https://kjellkod.wordpress.com/2015/06/30/the-worlds-fastest-logger-vs-g3log/

Try running [tests/spdlog_vs_logging.py](https://github.com/bodgergely/pyspdlog/blob/master/tests/test_spdlog.py) and see what results you get on your system.

pyspdlog vs logging (standard lib)
--------------------------------------------------
How many microseconds it takes on average to complete a log function (info(), debug() etc) using a FileLogger.
On reasonable sized log messages spdlog takes 1/10th of the time it would take to complete using the standard logging module.

Async mode with 8MB queue with blocking mode.

| msg len (bytes)   | spdlog **async** (microsec)| spdlog **sync** (microsec)| logging (microsec)  |
| -------           | :--------:      | :--------:      | :--------:          |
|  10               |  4.2            |  6.1            |   87.1              |
|  20               |  4.6            |  5.9            |   86.3              |
|  40               |  4.9            |  6.2            |   87.6              |
|  100              |  4.7            |  6.3            |   86.7              |
|  300              |  5.1            |  7.3            |   87.8              |
|  1000             |  5.8            |  12.4           |   93.1              |
|  5000             |  11.8           |  30.9           |   107.7             |
|  20000            |  39.9           |  76.9           |   150.3             |

Installation
============

1) `pip install spdlog` will get a distribution from pypi.org

or 

2) from github: 

```
git clone git@github.com:bodgergely/pyspdlog.git
cd pyspdlog 

pip install pybind11
git submodule update --init --recursive
python setup.py install
```

Usage
=====
```
./python
import spdlog as spd
logger = spd.FileLogger('fast_logger', '/tmp/spdlog_example.log')
logger.set_level(spd.LogLevel.INFO)
logger.info('Hello World!')
logger.debug('I am not so important.')
```

To run the speed test:
```
python ./tests/spdlog_vs_logging.cpp
```

