[![Build Status](https://travis-ci.org/bodgergely/spdlog-python.svg?branch=master)](https://travis-ci.org/bodgergely/spdlog-python)

spdlog-python
=============

python wrapper around the fast C++ logger called [spdlog](https://github.com/gabime/spdlog)


Introduction
============

Python wrapper (pybind11) around the C++ spdlog logging library. 

Why choose [spdlog](https://github.com/gabime/spdlog)?

https://kjellkod.wordpress.com/2015/06/30/the-worlds-fastest-logger-vs-g3log/

Try running [tests/spdlog_vs_logging.py](https://github.com/bodgergely/spdlog-python/blob/master/tests/test_spdlog.py) and see what results you get on your system.

spdlog-python vs logging (standard lib)
---------------------------------------

How many microseconds it takes on average to complete a log function (info(), debug() etc) using a FileLogger.
On reasonable sized log messages spdlog takes **4% (async mode enabled)** and **6% (sync mode)** of the time it would take to complete using the standard logging module.

Async mode with 8MB queue with blocking mode.

| msg len (bytes)   | spdlog **sync** (microsec)| spdlog **async** (microsec)| logging (microsec)   |
| -------           | :--------:      | :--------:      | :--------:                                |
|  10               |  1.2            |  0.87           |   24.6                                    |
|  100              |  1.2            |  1.03           |   24.6                                    |
|  300              |  1.5            |  1.07           |   24.9                                    |
|  1000             |  2.4            |  1.16           |   26.8                                    |
|  5000             |  6.2            |  2.31           |   31.7                                    |
|  20000            |  15.3           |  7.51           |   48.0                                    |

Installation
============

1) `pip install spdlog` will get a distribution from pypi.org

or

2) from github: 

`pip install pybind11` - if missing

```bash
git clone https://github.com/bodgergely/spdlog-python.git
cd spdlog-python
git submodule update --init --recursive
python setup.py install
```

Usage
=====

```python
./python
import spdlog as spd
logger = spd.FileLogger('fast_logger', '/tmp/spdlog_example.log')
logger.set_level(spd.LogLevel.INFO)
logger.info('Hello World!')
logger.debug('I am not so important.')
```

To run the speed test:

```bash
python ./tests/spdlog_vs_logging.py
```

