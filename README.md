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
How many microseconds does it take on average to complete a log function (info(), debug() etc) using a FileLogger.
On reasonable sized log messages spdlog takes 1/10th of the time it would take to complete using the standard logging module.

| msg len (bytes)   | spdlog    | logging   |
| -------           | --------  | --------  |
|  10               |  6.2      |   84.2    |
|  20               |  7.5      |   82.2    |
|  40               |  10.3     |   85.2    |
|  100              |  20.9     |   92.9    |
|  300              |  53.3     |   112.1   |
|  1000             |  144.43   |   202.6   |

Installation
============

Either clone spdlog directly into the top directory or use git submodule update as I show below.

```
git submodule update --init --recursive
python setup.py install
```

Usage
=====
```
./python
import spdlog
```

