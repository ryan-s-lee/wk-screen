import ctypes
import os

test_lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), "radixtree.so"))
test_lib.test()
