#include <iostream>
#include <vector>
#include <pthread.h>
#include <math.h>
#include <chrono>

using namespace std;

const double eps{1e-5};

pthread_mutex_t set_id_lock;

pthread_mutex_t completed_lock;
pthread_cond_t completed_all;

int thread_count, id{0}, completed_count{0};

typedef struct {
    vector<vector<double>> result;
    vector<vector<double>> matrix;
    vector<vector<double>> filter;
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

void input(arg_t& args) {
    // cout << "Enter matrix size: ";
    cin >> args.size_x >> args.size_y;
    throw_if_empty(args.size_x, args.size_y);

    args.matrix = args.result = vector<vector<double>>(args.size_x, vector<double>(args.size_y));
    // cout << "Enter matrix:" << endl;
    scan(args.matrix, args.size_x, args.size_y);

    // cout << "Enter filter window size (odd number of rows and cols): ";
    cin >> args.filter_size_x >> args.filter_size_y;
    throw_if_empty(args.filter_size_x, args.filter_size_y);
    if (!(args.filter_size_x & 1) || !(args.filter_size_y & 1)) {
        throw logic_error("Even number of rows or cols in filter window");
    }

    args.filter = vector<vector<double>>(args.filter_size_x, vector<double>(args.filter_size_y));
    // cout << "Enter convolution matrix:" << endl;
    args.div = scan(args.filter, args.filter_size_x, args.filter_size_y);
    if (args.div < eps) args.div = 1;

    // cout << "K = ";
    cin >> args.k;
}

void apply_filter(int& i, int& j, arg_t* args) {
    const int& size_x{args->size_x}, & size_y{args->size_y}, & filter_size_x{args->filter_size_x}, & filter_size_y{args->filter_size_y};

    int half_x{filter_size_x / 2}, half_y{filter_size_y / 2};
    double sum{0};

    for (int x{0}; x <= half_x; ++x) {
        for (int y{0}; y <= half_y; ++y) {
            // check indexes for out of range
            sum += args->matrix[(i - half_x + x >= 0) ? (i - half_x + x) : 0][(j - half_y + y >= 0) ? (j - half_y + y) : 0] * args->filter[x][y]; // left top corner
            if (y != half_y) {
                sum += args->matrix[(i - half_x + x >= 0) ? (i - half_x + x) : 0][(j + half_y - y < size_y) ? (j + half_y - y) : (size_y - 1)] * args->filter[x][filter_size_y - y - 1]; // right top
            }
            if (x != half_x) {
                sum += args->matrix[(i + half_x - x < size_x) ? (i + half_x - x) : (size_x - 1)][(j - half_y + y >= 0) ? (j - half_y + y) : 0] * args->filter[filter_size_x - x - 1][y]; // left bot
            }
            if (x != half_x && y != half_y) {
                sum += args->matrix[(i + half_x - x < size_x) ? (i + half_x - x) : (size_x - 1)][(j + half_y - y < size_y) ? (j + half_y - y) : (size_y - 1)] * args->filter[filter_size_x - x - 1][filter_size_y - y - 1]; // right bot
            }
        }
    }
    sum / args->div;

    args->result[i][j] = sum / args->div;
}

void apply_to_raws(int& start, int& finish, arg_t* args) {
    for (int i{start}; (i < args->size_x) && (i < finish); ++i) {
        for (int j{0}; j < args->size_y; ++j) {
            apply_filter(i, j, args);
        }
    }
}

void apply_to_cols(int& start, int& finish, arg_t* args) {
    for (int i{0}; i < args->size_x; i++) {
        for (int j{start}; (j < args->size_y) && (j < finish); ++j) {
            apply_filter(i, j, args);
        }
    }
}

void* filter_thread(void* arg) {
    arg_t* args{static_cast<arg_t*>(arg)};

    void(*apply_func)(int&, int&, arg_t*);
    apply_func = (args->size_x >= args->size_y) ? apply_to_raws : apply_to_cols;

    pthread_mutex_lock(&set_id_lock);
    int current_id{id++};
    id %= thread_count;
    pthread_mutex_unlock(&set_id_lock);

    int start{static_cast<int>(ceil(static_cast<double>(args->size_x) / thread_count)) * (current_id)};
    int finish{static_cast<int>(ceil(static_cast<double>(args->size_x) / thread_count)) * (current_id + 1)};

    for (int _{0}; _ < args->k; ++_) {
        apply_func(start, finish, args);
        pthread_mutex_lock(&completed_lock);
        if (++completed_count == thread_count) {
            args->matrix = args->result;
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
    if (argc == 1) {
        cerr << "Usage: ./main_exe n" << endl;
        cerr << "n - number of threads" << endl;
        throw logic_error("Number of threads not specified");
    }

    thread_count = stoi(argv[1]);
    if (!thread_count) {
        throw logic_error("At least 1 thread must exist");
    }

    arg_t args;
    if (!freopen("in.txt", "r", stdin)) {
        throw runtime_error("File error");
    }
    input(args);
    fclose(stdin);

    if (args.size_x >= args.size_y && thread_count > args.size_x) {
        thread_count = args.size_x;
    } else if (args.size_x < args.size_y && thread_count > args.size_y) {
        thread_count = args.size_y;
    }
    pthread_t tid[thread_count];

    auto begin{chrono::steady_clock::now()};

    for (int i{0}; i < thread_count; ++i) {
        pthread_create(&tid[i], nullptr, filter_thread, static_cast<void*>(&args));
    }
    for (int i{0}; i < thread_count; ++i) {
        pthread_join(tid[i], nullptr);
    }

    auto end{chrono::steady_clock::now()};

    cout.precision(3);
    cout << "Result:" << endl;
    print(args.matrix, args.size_x, args.size_y);

    auto elapsed_ms{chrono::duration_cast<chrono::milliseconds>(end - begin)};
    cout << "The time: " << elapsed_ms.count() << " ms" << endl;
}
