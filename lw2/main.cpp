#include <iostream>
#include <vector>

using namespace std;

void scan(vector<vector<double>>& matrix, int sizeX, int sizeY) {
    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {
            cin >> matrix[i][j];
        }
    }
}

void print(const vector<vector<double>>& matrix, int sizeX, int sizeY) {
    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {
            cout.precision(2);
            cout << fixed << matrix[i][j] << " ";
        }
        cout << endl;
    }
}

vector<vector<double>> applyFilter(const vector<vector<double>>& matrix, int sizeX, int sizeY, const vector<vector<double>>& filter, int filterSizeX, int filterSizeY) {
    vector<vector<double>> result(sizeX, vector<double>(sizeY));

    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {
            double sum = 0.0;
            int halfX = filterSizeX / 2, halfY = filterSizeY / 2;

            for (int x = 0; x <= halfX; x++) {
                for (int y = 0; y <= halfY; y++) {
                    // Индексы задаются с проверкой на выход за границы матрицы в указанных местах
                    sum += matrix[(i - halfX + x >= 0) ? (i - halfX + x) : 0][(j - halfY + y >= 0) ? (j - halfY + y) : 0] * filter[x][y]; // Левый верхний угол
                    if (y != halfY) {
                        sum += matrix[(i - halfX + x >= 0) ? (i - halfX + x) : 0][(j + halfY - y < sizeY) ? (j + halfY - y) : (sizeY - 1)] * filter[x][filterSizeY - y - 1]; // Правый верхний угол
                    }
                    if (x != halfX) {
                        sum += matrix[(i + halfX - x < sizeX) ? (i + halfX - x) : (sizeX - 1)][(j - halfY + y >= 0) ? (j - halfY + y) : 0] * filter[filterSizeX - x - 1][y]; // Левый нижний угол
                    }
                    if (x != halfX && y != halfY) {
                        sum += matrix[(i + halfX - x < sizeX) ? (i + halfX - x) : (sizeX - 1)][(j + halfY - y < sizeY) ? (j + halfY - y) : (sizeY - 1)] * filter[filterSizeX - x - 1][filterSizeY - y - 1]; // Правый нижний угол
                    }
                }
            }
            result[i][j] = sum / (filterSizeX * filterSizeY);
        }
    }
    return result;
}

int main() {
    int sizeX, sizeY;
    cout << "Введите размер матрицы: ";
    cin >> sizeX >> sizeY;

    vector<vector<double>> matrix(sizeX, vector<double>(sizeY));
    cout << "Введите элементы матрицы:" << endl;
    scan(matrix, sizeX, sizeY);

    int filterSizeX, filterSizeY;
    cout << "Введите размер окна фильтра (нечетное количество строк и столбцов): ";
    cin >> filterSizeX >> filterSizeY;
    if (!(filterSizeX & 1) || !(filterSizeY & 1)) {
        throw runtime_error("Фильтр должен иметь нечетное количество строк и столбцов");
    }

    vector<vector<double>> filter(filterSizeX, vector<double>(filterSizeY));
    cout << "Введите элементы матрицы свертки:" << endl;
    scan(filter, filterSizeX, filterSizeY);

    int k;
    cout << "K = ";
    cin >> k;
    vector<vector<double>> result;

    for (int i = 0; i < k; ++i) {
        result = applyFilter(matrix, sizeX, sizeY, filter, filterSizeX, filterSizeY);
    }
    cout << "Результат:" << endl;
    print(result, sizeX, sizeY);
}
