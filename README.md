# Advanced-vector
Улучшенный контейнер вектор
***

## Развертывание
```
g++ main.cpp vector.h -o advanced_vector -std=c++17 -O3
./advanced_vector
```
# Формат входных данных
```
SimpleVector<int> v;   Создает вектор нужного типа
v.PushBack(i);         Добавляет элемент вектора
v.EmplaceBack(i);      Собирает элемент вектора 
v.Erase(v.begin());    Удаляет элемент вектора
v.Reserve(i);          Резервирует определенное кол-во элементов
v.Resize(i);           Изменяет размер вектора
```
# Формат выходных данных
```
v[i];                  Получает элемент вектора
v.Size();              Получает размер вектора
v.Capacity();          Получает вместимость вектора    
```  
## Использование
### Ввод
```
int main() {
        Vector<int> v;
        v.Reserve(10);
        for (size_t i = 0; i < 10; ++i) {
            v.EmplaceBack(i);
        }
        for (size_t i = 0; i < v.Size(); ++i) {
            assert(v[i] == i);
            std::cout << v[i] << std::endl;
        }
        auto it = v.Erase(v.begin());
}
```
### Вывод
```
0
1
2
3
4
5
6
7
8
9
```
