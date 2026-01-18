#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iomanip>
#define NOMINMAX
#include <windows.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

// ------------------- Исключения -------------------

class HotelException : public runtime_error {
public:
    explicit HotelException(const string& msg)
        : runtime_error(msg) {
    }
};

class InvalidValueException : public HotelException {
public:
    explicit InvalidValueException(const string& msg)
        : HotelException("Некорректное значение: " + msg) {
    }
};

class DuplicateRoomException : public HotelException {
public:
    explicit DuplicateRoomException(const string& msg)
        : HotelException("Дубликат номера: " + msg) {
    }
};

class EmptyRoomListException : public HotelException {
public:
    explicit EmptyRoomListException(const string& msg)
        : HotelException("Список номеров пуст: " + msg) {
    }
};

// ------------------- Стратегии скидки -------------------

class IDiscountStrategy {
public:
    virtual ~IDiscountStrategy() = default;
    // возвращает итоговую стоимость при заданной базовой цене
    virtual double computeCost(double baseCost) const = 0;
};

class NoDiscountStrategy : public IDiscountStrategy {
public:
    double computeCost(double baseCost) const override {
        return baseCost;
    }
};

class PercentageDiscountStrategy : public IDiscountStrategy {
private:
    double discountPercent; // >=0 и <100
public:
    explicit PercentageDiscountStrategy(double percent)
        : discountPercent(percent)
    {
        if (discountPercent < 0.0) {
            throw InvalidValueException("процент скидки должен быть >= 0");
        }
        if (discountPercent >= 100.0) {
            throw InvalidValueException("процент скидки должен быть < 100");
        }
    }

    double computeCost(double baseCost) const override {
        return baseCost * (1.0 - discountPercent / 100.0);
    }
};

// ------------------- Интерфейс номера и реализация -------------------

class IRoom {
public:
    virtual ~IRoom() = default;
    virtual string getNumber() const = 0;
    virtual double getBaseCost() const = 0;
    virtual double getFinalCost() const = 0;
};

class RoomBase : public IRoom {
private:
    string number;
    double baseCost;
    shared_ptr<IDiscountStrategy> discountStrategy;

public:
    RoomBase(const string& number_, double baseCost_, shared_ptr<IDiscountStrategy> strategy_)
        : number(number_), baseCost(baseCost_), discountStrategy(strategy_)
    {
        if (number.empty()) {
            throw InvalidValueException("номер комнаты не может быть пустым");
        }
        if (baseCost <= 0.0) {
            throw InvalidValueException("базовая стоимость должна быть > 0");
        }
        if (!discountStrategy) {
            throw InvalidValueException("стратегия скидки не может быть null");
        }
    }

    string getNumber() const override {
        return number;
    }

    double getBaseCost() const override {
        return baseCost;
    }

    double getFinalCost() const override {
        return discountStrategy->computeCost(baseCost);
    }
};

// ------------------- Класс гостиницы -------------------

class Hotel {
private:
    vector<shared_ptr<IRoom>> rooms;

    bool existsRoomNumber(const string& num) const {
        for (const auto& r : rooms) {
            if (r->getNumber() == num) return true;
        }
        return false;
    }

public:
    Hotel() = default;

    // Добавить комнату: number (строка), базовая стоимость, скидка в процентах (0 - без скидки)
    void addRoom(const string& number, double baseCost, double discountPercent = 0.0) {
        if (number.size() > 50) {
            cerr << "Предупреждение: обозначение номера слишком длинное\n";
        }

        if (existsRoomNumber(number)) {
            throw DuplicateRoomException("номер '" + number + "' уже существует");
        }


        shared_ptr<IDiscountStrategy> strat;
        if (discountPercent == 0.0) {
            strat = make_shared<NoDiscountStrategy>();
        }
        else {
            strat = make_shared<PercentageDiscountStrategy>(discountPercent);
        }

        auto room = make_shared<RoomBase>(number, baseCost, strat);
        rooms.push_back(room);
    }

    double calculateAverageCost() const {
        if (rooms.empty()) {
            throw EmptyRoomListException("нечего усреднять");
        }
        double sum = 0.0;
        for (const auto& r : rooms) {
            sum += r->getFinalCost();
        }
        return sum / static_cast<double>(rooms.size());
    }

    void printAll() const {
        if (rooms.empty()) {
            cout << "Список номеров пуст.\n";
            return;
        }
        cout << "Текущие номера:\n";
        cout << left << setw(12) << "Номер" << setw(14) << "Баз.стоимость" << setw(16) << "После скидки" << '\n';
        for (const auto& r : rooms) {
            cout << left << setw(12) << r->getNumber()
                << setw(14) << fixed << setprecision(2) << r->getBaseCost()
                << setw(16) << fixed << setprecision(2) << r->getFinalCost()
                << '\n';
        }
    }
};

// ------------------- Ввод / утилиты -------------------

void clearStdin() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string inputNonEmptyString(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);

        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == string::npos || end == string::npos) {
            cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
            continue;
        }
        s = s.substr(start, end - start + 1);
        if (!s.empty()) return s;
        cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
    }
}

double inputPositiveDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double x;
        if (!(cin >> x)) {
            cout << "Ошибка: введите число.\n";
            clearStdin();
            continue;
        }
        clearStdin();
        if (x <= 0.0) {
            cout << "Ошибка: значение должно быть больше 0. Попробуйте снова.\n";
            continue;
        }
        if (x > 1000000.0) {
            cout << "Ошибка: значение не должно превышать 1000000. Попробуйте снова.\n";
            continue;
        }
        return x;
    }
}

double inputNonNegativeDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double x;
        if (!(cin >> x)) {
            cout << "Ошибка: введите число.\n";
            clearStdin();
            continue;
        }
        clearStdin();
        if (x < 0.0) {
            cout << "Ошибка: значение не может быть отрицательным. Попробуйте снова.\n";
            continue;
        }
        if (x >= 100.0) {
            cout << "Ошибка: процент скидки должен быть меньше 100. Попробуйте снова.\n";
            continue;
        }
        return x;
    }
}

int inputMenuChoice(const string& prompt, int low, int high) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) {
            cin.clear();
            continue;
        }

        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == string::npos || end == string::npos) {
            cout << "Ошибка: введите число.\n";
            continue;
        }
        string s = line.substr(start, end - start + 1);

        bool allDigits = !s.empty() &&
            all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c); });

        if (!allDigits) {
            cout << "Ошибка: введите целое число.\n";
            continue;
        }


        int val = stoi(s);
        if (val < low || val > high) {
            cout << "Ошибка: число должно быть в диапазоне [" << low << ", " << high << "].\n";
            continue;
        }
        return val;
    }
}

// ------------------- main -------------------

int main() {
#ifdef _WIN32
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
#endif
    setlocale(LC_ALL, "Russian");

    Hotel hotel;

    while (true) {
        cout << "\n===== МЕНЮ СИСТЕМЫ ГОСТИНИЦЫ =====\n";
        cout << "1. Добавить информацию о номере\n";
        cout << "2. Показать все номера\n";
        cout << "3. Вычислить среднюю стоимость проживания (с учётом скидок)\n";
        cout << "0. Выход\n";
        cout << "===================================\n";

        int choice = inputMenuChoice("Ваш выбор: ", 0, 3);

        try {
            if (choice == 0) {
                cout << "Выход из программы.\n";
                break;
            }
            else if (choice == 1) {
                string number = inputNonEmptyString("Введите обозначение номера (например 101, A-12): ");
                double baseCost = inputPositiveDouble("Введите базовую стоимость за ночь: ");
                double discount = inputNonNegativeDouble("Введите процент скидки на проживание (0 если нет, <100): ");
                hotel.addRoom(number, baseCost, discount);
                cout << "Информация о номере добавлена.\n";
            }
            else if (choice == 2) {
                hotel.printAll();
            }
            else if (choice == 3) {
                double avg = hotel.calculateAverageCost();
                cout << fixed << setprecision(2);
                cout << "Средняя стоимость проживания (после скидок): " << avg << '\n';
            }
        }
        catch (const HotelException& ex) {
            cout << "Ошибка: " << ex.what() << '\n';
        }
        catch (const exception& ex) {
            cout << "Непредвиденная ошибка: " << ex.what() << '\n';
        }
    }

    return 0;
}
