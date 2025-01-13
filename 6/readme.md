![result](/6/result.png)

# Сборка

`qmake && make`

Режим симуляции:

`./temperature_monitor`

Режим симуляции с ускорением времени в **k** раз:

`./temperature_monitor k`

Чтобы включить обычный режим, надо в `./temperature_monitor` закомментировать `QMAKE_CXXFLAGS += -DUSE_SIMULATION`.