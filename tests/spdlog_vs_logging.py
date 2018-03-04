import spdlog
import logging
import time
import statistics
import random
import numpy as np
from functools import partial

MICROSEC_IN_SEC = 1e6

def timed(func):
    def wrapper(*args):
        start = time.perf_counter()
        func(*args)
        return (time.perf_counter() - start) * MICROSEC_IN_SEC
    return wrapper


def generate_message(msg_len):
    msg = ""
    for i in range(msg_len):
        c = random.randint(ord('a'), ord('z'))
        msg += chr(c)
    return msg

def generate_numpy_array(array_len):
    return np.random.rand(array_len)

def generate_numpy_array_str(array_len):
    return f'{np.random.rand(array_len)}'



@timed
def do_logging(logger, message, count):
    for i in range(count):
        logger.info(message)



def build_timings_per_len(message_lengths):
    timings = {"spdlog" : {}, "logging" : {}}
    for msg_len in message_lengths:
        timings["spdlog"][msg_len] = []
        timings["logging"][msg_len] = []
    return timings


def candidate_logger(logger, name, epochs, sub_epochs,repeat_cnt, message_lengths, message_generator, worker, timings):
    for epoch in range(epochs):
        for msg_len in message_lengths:
            msg = message_generator(msg_len)
            for _ in range(sub_epochs):
                took = logger(msg, repeat_cnt)
                timings[name][msg_len].append(took)
                worker()

def lets_do_some_work():
    x = [i for i in range(1 << 5)]
    y = [i for i in range(1 << 5)]
    result = []
    for i, j in zip(x,y):
        z = x + y
        result.append(z)



def generate_stats(timings):
    d = {"spdlog": {}, "logging" : {}}
    for logger, time_per_msg_len in timings.items():
        for msg_len, times in time_per_msg_len.items():
           mean = statistics.mean(times)
           d[logger][msg_len] = mean
    return d

def calculate_ratio(timings, logger1, logger2):
    t1,t2 = timings[logger1], timings[logger2]
    msg_lens = t1.keys()
    return { ml : t1[ml]/t2[ml] for ml in msg_lens }





def run_test(async):
    
    message_lengths = [10, 20, 40, 100, 300, 1000]
    repeat_cnt = 5
    epochs = 20
    sub_epochs = 10
    if async:
        spdlog.set_async_mode(queue_size=1 << 20)
    else: 
        spdlog.set_sync_mode()


    spd_logger = spdlog.FileLogger(name='speedlogger', filename='speedlog.log', multithreaded=False, truncate=False)
    if spd_logger.async() != async:
        print(f"spdlog should be in {async} mode but is in {spd_logger.async()}")

    standard_logger = logging.getLogger('logging')
    fh = logging.FileHandler('logging.log')
    fh.setLevel(logging.DEBUG)
    standard_logger.addHandler(fh)
    standard_logger.setLevel(logging.DEBUG)

    timings = build_timings_per_len(message_lengths)

    candidate_logger(partial(do_logging, spd_logger), 'spdlog', epochs, sub_epochs, repeat_cnt, message_lengths,generate_numpy_array_str, lets_do_some_work, timings)
    candidate_logger(partial(do_logging, standard_logger), 'logging', epochs,sub_epochs, repeat_cnt, message_lengths, generate_numpy_array_str, lets_do_some_work, timings)


    final = generate_stats(timings)
    print("Message len -> Avg time microsec")
    print(final)

    ratios = calculate_ratio(final, 'spdlog', 'logging')

    for msg_len, ratio in ratios.items():
        print(f"spdlog takes {ratio * 100}% of logging at message len: {msg_len}")

    if async:
        sleeptime = 4
        print(f"Sleeping for secs: {sleeptime}")
        time.sleep(sleeptime)
    spd_logger.close()


if __name__ == "__main__":
    print("Running in spdlog in sync mode")
    run_test(False)
    print("Running in spdlog in async mode")
    run_test(True)




