`mkdir build && cd build`

# обычный режим:

`cmake .. && make && ./4`

# режим симуляции:

`cmake .. -DUSE_SIMULATION=ON && make && ./4`

# режим симуляции с ускорением времени в **k** раз:

`cmake .. -DUSE_SIMULATION=ON && make && ./4 k`