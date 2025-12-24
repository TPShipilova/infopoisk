# Лабораторные работы по курсу "Информационный поиск"

Как запустить поискового робота:

```
docker run --name fashion-corpus -d -p 27017:27017 mongo
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 main.py
```

Далее для остальных лаб написан общий makefile, конкретно CMakeLists.txt

```
mkdir build && cd build
cmake ..
make
```

Работа с булевым поиском осуществляется через терминал, просто пишется запрос. Примеры запросов представлены в отчете.
