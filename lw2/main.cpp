#include <iostream>
#include <vector>
#include <pthread.h>
#include <math.h>
#include <unordered_map>
#include <chrono>

using namespace std;

pthread_mutex_t set_id_lock; // name?

pthread_mutex_t completed_lock;
pthread_cond_t completed_all;

vector<vector<double>> result;
vector<vector<double>> matrix;
vector<vector<double>> filter;
int thread_count, id = 0, completed_count = 0;
int size_x, size_y, filter_size_x, filter_size_y, k;


void scan(vector<vector<double>>& m, int x, int y) {
    for (int i = 0; i < x; i++) {
        for (int j = 0; j < y; j++) {
            cin >> m[i][j];
        }
    }
}

void print(const vector<vector<double>>& m, int x, int y) {
    for (int i = 0; i < x; i++) {
        for (int j = 0; j < y; j++) {
            cout.precision(2);
            cout << fixed << m[i][j] << " ";
        }
        cout << endl;
    }
}

void apply_filter(int i, int j) {
    int half_x = filter_size_x / 2, half_y = filter_size_y / 2;
    double sum = 0.0;

    for (int x = 0; x <= half_x; x++) {
        for (int y = 0; y <= half_y; y++) {
            // check indexes for out of range
            sum += matrix[(i - half_x + x >= 0) ? (i - half_x + x) : 0][(j - half_y + y >= 0) ? (j - half_y + y) : 0] * filter[x][y]; // left top corner
            if (y != half_y) {
                sum += matrix[(i - half_x + x >= 0) ? (i - half_x + x) : 0][(j + half_y - y < size_y) ? (j + half_y - y) : (size_y - 1)] * filter[x][filter_size_y - y - 1]; // right top
            }
            if (x != half_x) {
                sum += matrix[(i + half_x - x < size_x) ? (i + half_x - x) : (size_x - 1)][(j - half_y + y >= 0) ? (j - half_y + y) : 0] * filter[filter_size_x - x - 1][y]; // left bot
            }
            if (x != half_x && y != half_y) {
                sum += matrix[(i + half_x - x < size_x) ? (i + half_x - x) : (size_x - 1)][(j + half_y - y < size_y) ? (j + half_y - y) : (size_y - 1)] * filter[filter_size_x - x - 1][filter_size_y - y - 1]; // right bot
            }
        }
    }
    result[i][j] = sum / (filter_size_x * filter_size_y);
}

void apply_to_raws(int start, int finish) {
    for (int i = start; (i < size_x) && (i < finish); i++) {
        for (int j = 0; j < size_y; j++) {
            apply_filter(i, j);
        }
    }
}

void apply_to_cols(int start, int finish) {
    for (int i = 0; i < size_x; i++) {
        for (int j = start; (j < size_y) && (j < finish); j++) {
            apply_filter(i, j);
        }
    }
}


void* filter_thread(void* arg) {
    void(*apply_func)(int, int);
    apply_func = (size_x >= size_y) ? apply_to_raws : apply_to_cols;
    pthread_mutex_lock(&set_id_lock);
    int tid = id++;
    id %= thread_count;
    pthread_mutex_unlock(&set_id_lock);

    int start = (size_x / thread_count) * (tid);
    int finish = ceil((double) size_x / thread_count) * (tid + 1);

    for (int _ = 0; _ < k; ++_) {
        apply_func(start, finish);
        pthread_mutex_lock(&completed_lock);
        if (++completed_count == thread_count) {
            matrix = result;
            completed_count = 0;
            pthread_cond_broadcast(&completed_all);
        } else {
            pthread_cond_wait(&completed_all, &completed_lock);
        }
        pthread_mutex_unlock(&completed_lock);
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    freopen("in.txt", "r", stdin);

    // cout << "Enter matrix size: ";
    cin >> size_x >> size_y;
    matrix = result = vector<vector<double>>(size_x, vector<double>(size_y));
    // cout << "Enter matrix:" << endl;
    scan(matrix, size_x, size_y);

    // cout << "Enter filter window size (odd number of rows and cols): ";
    cin >> filter_size_x >> filter_size_y;
    if (!(filter_size_x & 1) || !(filter_size_y & 1)) {
        throw runtime_error("Even number of rows or cols in filter window");
    }
    filter = vector<vector<double>>(filter_size_x, vector<double>(filter_size_y));
    // cout << "Enter convolution matrix:" << endl;
    scan(filter, filter_size_x, filter_size_y);

    // cout << "K = ";
    cin >> k;

    thread_count = stoi(argv[1]);

    if (size_x > size_y && thread_count > size_x) {
        thread_count = size_x;
    } else if (size_x < size_y && thread_count > size_y) {
        thread_count = size_y;
    }

    pthread_t tid[thread_count];

    auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&tid[i], nullptr, filter_thread, nullptr);
    }
    for (int i = 0; i < thread_count; i++) {
        pthread_join(tid[i], nullptr);
    }
    auto end = std::chrono::steady_clock::now();

    cout.precision(3);

    cout << "Result:" << endl;
    print(matrix, size_x, size_y);
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "The time: " << elapsed_ms.count() << " ms\n";

}
