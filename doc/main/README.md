# DocGenerator (Qt5 + QXlsx)

Генератор документов ГОСТ из BOM (XLSX).

## Возможности
- Выбор типа документа: ПЭ3 / ВП / Спецификация (пока реализован только ПЭ3)
- Чтение XLSX через QXlsx
- Группировка Designator: C1, C2, C3, C4 → C1-C4
- Сортировка по Designator (естественный порядок)
- Заполнение HTML-шаблона (первый лист уникальный, далее повторяется второй)
- Проверка вместимости текста в ячейки
- Вывод HTML + PDF

## Сборка
```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/msys64/mingw64 ..
cmake --build . -j
```

## Маппинг (ПЭ3)
- Поз. обозначение ← сгруппированный Designator
- Наименование ← Type + Comment + Manufacturer
- Кол. ← Quantity
- Примечание ← пусто
