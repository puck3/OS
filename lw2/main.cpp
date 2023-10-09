#include <iostream>
#include <vector>
#include <pthread.h>
#include <math.h>
#include <time.h> 

using namespace std;


typedef struct args {
    int id;
    int threadCount;
    vector<vector<double>>* result;
    vector<vector<double>>* matrix;
    int sizeX, sizeY;
    vector<vector<double>>* filter;
    int filterSizeX, filterSizeY;
} args_t;

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

void* applyFilter(void* args) {
    args_t* arg = (args_t*) args;

    int id = arg->id;
    const int& threadCount = arg->threadCount;

    const vector<vector<double>>& matrix = *arg->matrix;
    const int& sizeX = arg->sizeX;
    const int& sizeY = arg->sizeY;

    const vector<vector<double>>& filter = *arg->filter;
    const int& filterSizeX = arg->filterSizeX;
    const int& filterSizeY = arg->filterSizeY;

    for (int i = (sizeX / threadCount) * (id); (i < sizeX) && (i < ceil((double) sizeX / threadCount) * (id + 1)); i++) {
        for (int j = 0; j < sizeY; j++) {
            double sum = 0.0;
            int halfX = filterSizeX / 2, halfY = filterSizeY / 2;

            for (int x = 0; x <= halfX; x++) {
                for (int y = 0; y <= halfY; y++) {
                    // check indexes for out of range
                    sum += matrix[(i - halfX + x >= 0) ? (i - halfX + x) : 0][(j - halfY + y >= 0) ? (j - halfY + y) : 0] * filter[x][y]; // left top corner
                    if (y != halfY) {
                        sum += matrix[(i - halfX + x >= 0) ? (i - halfX + x) : 0][(j + halfY - y < sizeY) ? (j + halfY - y) : (sizeY - 1)] * filter[x][filterSizeY - y - 1]; // right top
                    }
                    if (x != halfX) {
                        sum += matrix[(i + halfX - x < sizeX) ? (i + halfX - x) : (sizeX - 1)][(j - halfY + y >= 0) ? (j - halfY + y) : 0] * filter[filterSizeX - x - 1][y]; // left bot
                    }
                    if (x != halfX && y != halfY) {
                        sum += matrix[(i + halfX - x < sizeX) ? (i + halfX - x) : (sizeX - 1)][(j + halfY - y < sizeY) ? (j + halfY - y) : (sizeY - 1)] * filter[filterSizeX - x - 1][filterSizeY - y - 1]; // right bot
                    }
                }
            }
            arg->result->at(i)[j] = sum / (filterSizeX * filterSizeY);
        }
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    freopen("in.txt", "r", stdin);
    clock_t start = clock();
    int sizeX, sizeY;
    // cout << "Enter matrix size: ";
    cin >> sizeX >> sizeY;

    vector<vector<double>> matrix(sizeX, vector<double>(sizeY));
    // cout << "Enter matrix:" << endl;
    scan(matrix, sizeX, sizeY);

    int filterSizeX, filterSizeY;
    // cout << "Enter filter window size (odd number of rows and cols): ";
    cin >> filterSizeX >> filterSizeY;
    if (!(filterSizeX & 1) || !(filterSizeY & 1)) {
        throw runtime_error("Even number of rows or cols in filter window");
    }

    vector<vector<double>> filter(filterSizeX, vector<double>(filterSizeY));
    // cout << "Enter convolution matrix:" << endl;
    scan(filter, filterSizeX, filterSizeY);

    int k;
    // cout << "K = ";
    cin >> k;
    vector<vector<double>> result(sizeX, vector<double>(sizeY, 0));

    int threadCount = stoi(argv[1]);
    pthread_t tid[threadCount];
    args_t args[threadCount];
    for (int i = 0; i < threadCount; ++i) {
        args[i].id = i;
        args[i].threadCount = threadCount;
        args[i].result = &result;
        args[i].matrix = &matrix;
        args[i].sizeX = sizeX;
        args[i].sizeY = sizeY;
        args[i].filter = &filter;
        args[i].filterSizeX = filterSizeX;
        args[i].filterSizeY = filterSizeY;
    }


    for (int i = 0; i < k; ++i) {
        for (int i = 0; i < threadCount; i++) {

            pthread_create(&tid[i], NULL, applyFilter, (void*) &args[i]);
        }
        for (int i = 0; i < threadCount; i++) {
            pthread_join(tid[i], NULL);
        }
        swap(matrix, result);
        for (int i = 0; i < threadCount; ++i) {
            args[i].result = &result;
            args[i].matrix = &matrix;
        }
    }
    cout.precision(6);
    cout << "Result:" << endl;
    print(matrix, sizeX, sizeY);
    clock_t end = clock();
    double seconds = (double) (end - start) / CLOCKS_PER_SEC;
    cout.precision(6);
    cout << "took " << seconds << "s" << endl;
}
