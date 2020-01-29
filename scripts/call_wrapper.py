#!/usr/bin/env python3

import traceback
import sys

class SkippedError(Exception):
     def __init__(self):
         self.message = "Skipped error"

def final_call_decorator(start_msg: str,
                         success_msg: str,
                         failure_msg: str):
    def f_decorator(f):
        def wrapper(*args, **kwds):
            print(start_msg)
            try:
                f(*args, **kwds)
                print(success_msg)
                sys.exit(0)
            except SkippedError:
                print(failure_msg, file=sys.stderr)
                sys.exit(1)
            except Exception:
                traceback.print_exc(file=sys.stderr)
                print(failure_msg, file=sys.stderr)
                sys.exit(1)

        return wrapper

    return f_decorator
