#include <iostream>
#include <vector>
#include <pthread.h>
#include <math.h>
#include <chrono>

using namespace std;

const double eps{1e-5};
pthread_mutex_t completed_lock;
pthread_cond_t completed_all;
int completed_count{0};

typedef struct {
    int thread_count, id;
    vector<vector<double>>* result;
    vector<vector<double>>* matrix;
    vector<vector<double>>* filter;
    int size_x, size_y, filter_size_x, filter_size_y, k;
    double div;
} arg_t;

void throw_if_empty(int& size_x, int& size_y) {
    if (size_x == 0 || size_y == 0) {
        throw logic_error("Matrix is empty");
    }
}

double scan(vector<vector<double>>& matrix, int& size_x, int& size_y) {
    double sum{0};
    for (int i{0}; i < size_x; ++i) {
        for (int j{0}; j < size_y; ++j) {
            cin >> matrix[i][j];
            sum += matrix[i][j];
        }
    }
    return sum;
}

void print(const vector<vector<double>>& matrix, int& size_x, int& size_y) {
    for (int i{0}; i < size_x; ++i) {
        for (int j{0}; j < size_y; ++j) {
            cout.precision(2);
            cout << fixed << matrix[i][j] << " ";
        }
        cout << endl;
    }
}

void input(arg_t args[], int argc, char* argv[], int& thread_count, int& size_x, int& size_y, int& filter_size_x, int& filter_size_y, int& div, int& k,
    vector<vector<double>>& matrix, vector<vector<double>>& result, vector<vector<double>>& filter) {
    if (!freopen("in.txt", "r", stdin)) {
        throw runtime_error("File error");
    }
    // cout << "Enter matrix size: ";
    cin >> size_x >> size_y;
    throw_if_empty(size_x, size_y);

    matrix = result = vector<vector<double>>(size_x, vector<double>(size_y));
    // cout << "Enter matrix:" << endl;
    scan(matrix, size_x, size_y);

    // cout << "Enter filter window size (odd number of rows and cols): ";
    cin >> filter_size_x >> filter_size_y;
    throw_if_empty(filter_size_x, filter_size_y);
    if (!(filter_size_x & 1) || !(filter_size_y & 1)) {
        throw logic_error("Even number of rows or cols in filter window");
    }

    filter = vector<vector<double>>(filter_size_x, vector<double>(filter_size_y));
    // cout << "Enter convolution matrix:" << endl;
    div = scan(filter, filter_size_x, filter_size_y);
    if (div < eps) div = 1;

    // cout << "K = ";
    cin >> k;

    fclose(stdin);

    if (size_x >= size_y && thread_count > size_x) {
        thread_count = size_x;
    } else if (size_x < size_y && thread_count > size_y) {
        thread_count = size_y;
    }

    for (int i{0}; i < thread_count; ++i) {
        args[i].thread_count = thread_count;
        args[i].id = i;
        args[i].size_x = size_x;
        args[i].size_y = size_y;
        args[i].matrix = &matrix;
        args[i].filter_size_x = filter_size_x;
        args[i].filter_size_y = filter_size_y;
        args[i].filter = &filter;
        args[i].result = &result;
        args[i].div = div;
        args[i].k = k;
    }
}

void apply_filter(int& i, int& j, arg_t* args) {
    const int& size_x{args->size_x}, & size_y{args->size_y}, & filter_size_x{args->filter_size_x}, & filter_size_y{args->filter_size_y};

    const vector<vector<double>>& matrix{*args->matrix}, & filter{*args->filter};

    int half_x{filter_size_x / 2}, half_y{filter_size_y / 2};
    double sum{0};

    for (int x{0}; x <= half_x; ++x) {
        for (int y{0}; y <= half_y; ++y) {
            int top, bot, left, right;
            top = (i - half_x + x >= 0) ? (i - half_x + x) : 0;
            bot = (i + half_x - x < size_x) ? (i + half_x - x) : (size_x - 1);
            left = (j - half_y + y >= 0) ? (j - half_y + y) : 0;
            right = (j + half_y - y < size_y) ? (j + half_y - y) : (size_y - 1);

            sum += matrix[top][left] * filter[x][y];
            if (y != half_y) {
                sum += matrix[top][right] * filter[x][filter_size_y - y - 1];
            }
            if (x != half_x) {
                sum += matrix[bot][left] * filter[filter_size_x - x - 1][y];
            }
            if (x != half_x && y != half_y) {
                sum += matrix[bot][right] * filter[filter_size_x - x - 1][filter_size_y - y - 1];
            }
        }
    }
    sum / args->div;
    args->result->at(i)[j] = sum / args->div;
}

void apply_to_raws(int start, int finish, arg_t* args) {
    for (int i{start}; (i < args->size_x) && (i < finish); ++i) {
        for (int j{0}; j < args->size_y; ++j) {
            apply_filter(i, j, args);
        }
    }
}

void apply_to_cols(int start, int finish, arg_t* args) {
    for (int i{0}; i < args->size_x; i++) {
        for (int j{start}; (j < args->size_y) && (j < finish); ++j) {
            apply_filter(i, j, args);
        }
    }
}

void* filter_thread(void* arg) {
    arg_t* args{static_cast<arg_t*>(arg)};
    void(*apply_func)(int, int, arg_t*);
    apply_func = (args->size_x >= args->size_y) ? apply_to_raws : apply_to_cols;

    int start{static_cast<int>(ceil(static_cast<double>(args->size_x) / args->thread_count)) * args->id};
    int finish{static_cast<int>(ceil(static_cast<double>(args->size_x) / args->thread_count)) * (args->id + 1)};
    int status;

    for (int _{0}; _ < args->k; ++_) {
        apply_func(start, finish, args);
        pthread_mutex_lock(&completed_lock);
        if (++completed_count == args->thread_count) {
            completed_count = 0;
            pthread_cond_broadcast(&completed_all);
        } else {
            pthread_cond_wait(&completed_all, &completed_lock);
        }
        pthread_mutex_unlock(&completed_lock);
        args->matrix = args->result;
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    int thread_count, size_x, size_y, filter_size_x, filter_size_y, div, k;
    vector<vector<double>> matrix, result, filter;

    if (argc == 1) {
        cerr << "Usage: ./main_exe n" << endl;
        cerr << "n - number of threads" << endl;
        throw logic_error("Number of threads not specified");
    }

    thread_count = stoi(argv[1]);
    if (!thread_count) {
        throw logic_error("At least 1 thread must exist");
    }

    arg_t args[thread_count];
    input(args, argc, argv, thread_count, size_x, size_y, filter_size_x, filter_size_y, div, k, matrix, result, filter);

    auto begin{chrono::steady_clock::now()};
    pthread_t tid[thread_count];
    for (int i{0}; i < thread_count; ++i) {
        pthread_create(&tid[i], nullptr, filter_thread, static_cast<void*>(&args[i]));
    }
    for (int i{0}; i < thread_count; ++i) {
        pthread_join(tid[i], nullptr);
    }
    auto end{chrono::steady_clock::now()};

    // cout << "Result:" << endl;
    // print(*args[0].matrix, size_x, size_y);
    auto elapsed_ms{chrono::duration_cast<chrono::milliseconds>(end - begin)};
    cout << "The time: " << elapsed_ms.count() << " ms" << endl;
}
