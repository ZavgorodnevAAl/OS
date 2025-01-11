# Запуск сервера

`mkdir build && cd build`

обычный режим:

`cmake .. && make && ./5`

режим симуляции:

`cmake .. -DUSE_SIMULATION=ON && make && ./5`

режим симуляции с ускорением времени в **k** раз:

`cmake .. -DUSE_SIMULATION=ON && make && ./5 k`

# Запуск веб-приложения

Установка библиотек

Для красоты веб приложения использовал python библиотеку streamlit.

`pip install streamlit requests pandas plotly`

Запуск

`streamlit run app.py`