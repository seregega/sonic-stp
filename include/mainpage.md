# **Проект AVL Tree Library**

## Описание проекта

AVL Tree Library — это высокопроизводительная библиотека для работы с AVL-деревьями.  
Она предоставляет функционал для выполнения следующих операций:
- Вставка, удаление и поиск элементов.
- Балансировка дерева для оптимальной производительности.
- Поддержка итераторов для удобного обхода элементов.

Эта библиотека идеально подходит для использования в проектах, требующих структур данных с логарифмической сложностью операций.

---

## Особенности

- **Эффективная работа**: операции выполняются с временной сложностью \(O(\log n)\).
- **Поддержка пользовательских типов**: возможность работы с произвольными структурами через настраиваемую функцию сравнения.
- **Безопасность и устойчивость**: строгая проверка уникальности элементов и ассерты для предотвращения ошибок.
- **Удобство использования**: встроенные итераторы и функции для копирования деревьев.

---

## Основные компоненты

### Ключевые структуры
- **`struct avl_table`**: представляет AVL-дерево.
- **`struct avl_node`**: узел AVL-дерева, содержащий указатели на дочерние элементы и баланс-фактор.
- **`struct avl_traverser`**: объект для обхода дерева.

### Основные функции
- **Создание дерева**: `avl_create`
- **Поиск элемента**: `avl_find`
- **Вставка элемента**: `avl_probe`, `avl_replace`
- **Удаление элемента**: `avl_delete`
- **Обход дерева**: `avl_t_first`, `avl_t_next`

---

## Архитектура проекта

### Диаграмма структуры данных
@startuml
class AVL_Tree {
    +avl_create()
    +avl_insert()
    +avl_delete()
    +avl_find()
    +avl_destroy()
}
class AVL_Node {
    +balance_factor: int
    +left: AVL_Node
    +right: AVL_Node
}
AVL_Tree --> AVL_Node
@enduml

---

## Пример использования

```c
#include "avl.h"
#include <stdio.h>

int compare_ints(const void *a, const void *b, void *param) {
    return *(const int *)a - *(const int *)b;
}

int main() {
    struct avl_table *tree = avl_create(compare_ints, NULL, NULL);

    int value1 = 10, value2 = 20, value3 = 15;
    avl_probe(tree, &value1);
    avl_probe(tree, &value2);
    avl_probe(tree, &value3);

    int key = 15;
    int *result = avl_find(tree, &key);
    if (result) {
        printf("Найдено: %d\n", *result);
    }

    avl_destroy(tree, NULL);
    return 0;
}

## Автор
Картавцев Сергей sergega@me.com
Разработано в BULAT.

