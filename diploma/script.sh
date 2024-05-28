#!/bin/bash

# Имя исполняемого файла вашей программы
PROGRAM="./test_parallel"

# Проверьте, существует ли файл программы и является ли он исполняемым
if [ ! -x "$PROGRAM" ]; then
    echo "Ошибка: Файл $PROGRAM не существует или не является исполняемым."
    exit 1
fi

# Имя выходного файла
OUTPUT_FILE="output.txt"

# Очистка выходного файла перед началом записи
> $OUTPUT_FILE

# Цикл для запуска программы 10 раз
for i in 1 2
do
    echo "Запуск $i:" >> $OUTPUT_FILE
    $PROGRAM >> $OUTPUT_FILE
    if [ $? -ne 0 ]; then
        echo "Ошибка: Программа завершилась с ошибкой на запуске $i." >> $OUTPUT_FILE
    fi
    echo "-------------------" >> $OUTPUT_FILE
done

echo "Все запуски завершены. Результаты сохранены в $OUTPUT_FILE."
