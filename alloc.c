#include "bsa.h"
#include "Alloc.h"

struct bsa {
    int* rows[BSA_ROWS];
    bool row_allocated[BSA_ROWS];
    bool* is_set;
    int max_index;
};


// Create an empty BSA
bsa* bsa_init(void){
    bsa* b = malloc(sizeof(bsa));
    if (b == NULL) {
        return NULL; // 内存分配失败
    }

    for (int i = 0; i < BSA_ROWS; i++) {
        b->rows[i] = NULL;            // 初始化所有行的指针为NULL
        b->row_allocated[i] = false;  // 初始化所有行为未分配
    }

    int total_elements = (1 << BSA_ROWS);
    b->is_set = malloc(total_elements * sizeof(bool));
    if (b->is_set == NULL) {
        free(b); // 不要忘记如果在这里失败了也需要释放之前分配的bsa结构体
        return NULL;
    }
    for (int i = 0; i < total_elements; ++i) {
        b->is_set[i] = false;
    }


    b->max_index = -1; // 初始化最大索引为-1，表示没有元素被设置
    return b;
}

int get_row_from_index(int index) {
    int row = 0;
    int row_end = 0;
    while (index >= row_end && row < BSA_ROWS) {
        row_end += (1 << row);
        row++;
    }
    return row - 1;
}

// Set element at index indx with value d i.e. b[i] = d;
// May require an allocation if it's the first element in that row
bool bsa_set(bsa* b, int indx, int d){
    if (b == NULL || indx < 0) {
        return false; // 检查有效的bsa指针和有效的索引
    }

    int row = get_row_from_index(indx);
    if (row >= BSA_ROWS) {
        return false; // 索引超出了bsa结构的容量
    }

    if (!b->row_allocated[row]) {
        // 分配内存
        b->rows[row] = (int*)calloc((1 << row), sizeof(int));
        if (b->rows[row] == NULL) {
            return false; // 内存分配失败
        }
        b->row_allocated[row] = true;
    }

    // 计算行内的实际位置
    int position = indx;
    for (int i = 0; i < row; ++i) {
        position -= (1 << i); // 减去前面行的总大小
    }

    b->rows[row][position] = d;
    b->is_set[indx] = true; // 标记元素为已设置

    if (indx > b->max_index) {
        b->max_index = indx; // 更新最大索引
    }
    return true;
}

// Return pointer to data at element b[i]
// or NULL if element is unset, or part of a row that hasn't been allocated.
int* bsa_get(bsa* b, int indx){
    if (b == NULL || indx < 0 || indx > b->max_index) {
        return NULL;
    }
    int row = get_row_from_index(indx);
    if (row >= BSA_ROWS || !b->row_allocated[row]) {
        return NULL;  // 如果行超出范围或该行尚未分配，则返回NULL
    }
    int position = indx;
    for (int i = 0; i < row; ++i) {
        position -= (1 << i);
    }
    return &(b->rows[row][position]);
}

bool is_row_empty(bsa* b, int row) {
    if (b == NULL || row < 0 || row >= BSA_ROWS || !b->row_allocated[row]) {
        return true; // 如果BSA无效，行号无效，或行未分配，则认为该行为空
    }
    for (int i = 0; i < (1 << row); ++i) {
        if (b->rows[row][i] != 0) {
            return false; // 找到非零元素，行不为空
        }
    }
    return true; // 所有元素都是零，行为空
}

// Delete element at index indx - forces a shrink
// if that was the only cell in the row occupied.
bool bsa_delete(bsa* b, int indx){
    if (b == NULL || indx < 0 || indx > b->max_index || !b->is_set[indx]) {
        return false; // 检查BSA指针是否有效，索引是否在有效范围内，以及索引是否已被赋值
    }

    int row = get_row_from_index(indx);
    if (row >= BSA_ROWS || !b->row_allocated[row]) {
        return false; // 如果行超出范围或该行尚未分配，则返回false
    }
    if (!b->is_set[indx]) {
        return false;
    }

    // 计算在行内的实际位置
    int position = indx;
    for (int i = 0; i < row; ++i) {
        position -= (1 << i);
    }

    // 删除元素：将元素值设置为0（或其他标识未使用的值）
    b->rows[row][position] = 0;
    b->is_set[indx] = false; // 标记该索引未被赋值

    if (is_row_empty(b, row)) {
        free(b->rows[row]);
        b->rows[row] = NULL;
        b->row_allocated[row] = false;
    }

    // 更新max_index为最后一个被标记的元素的索引
    if (indx == b->max_index) {
        while (indx >= 0 && !b->is_set[indx]) {
            indx--;
        }
        b->max_index = indx;
    }

    return true;
}

// Returns maximum index written to so far or
// -1 if no cells have been written to yet
int bsa_maxindex(bsa* b){
    if (b == NULL) {
        return -1; // 如果BSA指针无效，返回-1
    }
    return b->max_index; // 返回BSA中的最大索引
}

// Returns stringified version of structure
// Each row has its elements printed between {}, up to the maximum index.
// Rows after the maximum index are ignored.
bool bsa_tostring(bsa* b, char* str){
    if (b == NULL || str == NULL || b->is_set == NULL) {
        return false;
    }

    str[0] = '\0';  // 初始化字符串为空
    char buffer[100];  // 临时缓冲区用于格式化每个元素

    int currentIndex = 0;  // 当前处理的全局索引

    for (int row = 0; row < BSA_ROWS; ++row) {
        bool isRowPrinted = false;  // 标记当前行是否已打印

        if (b->row_allocated[row]) {
            strcat(str, isRowPrinted ? " {" : "{");  // 开始新行

            for (int pos = 0; pos < (1 << row); ++pos) {
                if (currentIndex > b->max_index) {
                    break;  // 如果超出最大索引，终止循环
                }

                if (b->is_set[currentIndex]) {
                    snprintf(buffer, sizeof(buffer), isRowPrinted ? " [%d]=%d" : "[%d]=%d", currentIndex, b->rows[row][pos]);
                    strcat(str, buffer);
                    isRowPrinted = true;
                }

                currentIndex++;  // 更新当前处理的索引
            }

            strcat(str, "}");  // 闭合已打印的行
        } else {
            if (currentIndex > b->max_index) {
                break;  // 如果超出最大索引，不再打印新行
            }
            strcat(str, "{}");  // 输出空行
            currentIndex += (1 << row);  // 更新索引以跳过未分配的行
        }
    }

    return true;
}

// Clears up all space used
bool bsa_free(bsa* b){
    if (b == NULL) {
        return false; // 如果BSA指针无效，不执行任何操作
    }
    // 释放每一行分配的内存
    for (int i = 0; i < BSA_ROWS; ++i) {
        if (b->row_allocated[i]) {
            free(b->rows[i]);  // 释放当前行
            b->rows[i] = NULL; // 将指针设置为NULL
        }
    }
    // 释放BSA结构体本身
    free(b);
    return true;
}

// Allow a user-defined function to be applied to each (valid) value
// in the array. The user defined 'func' is passed a pointer to an int,
// and maintains an accumulator of the result where required.
void bsa_foreach(void (*func)(int* p, int* n), bsa* b, int* acc){
    if (func == NULL || b == NULL || acc == NULL) {
        return; // 检查参数是否有效
    }

    for (int indx = 0; indx <= b->max_index; ++indx) {
        if (b->is_set[indx]) {
            int row = get_row_from_index(indx);
            int position = indx;
            for (int i = 0; i < row; ++i) {
                position -= (1 << i);
            }
            func(&(b->rows[row][position]), acc);
        }
    }

}

// You'll this to test the other functions you write
void test(void){
    bsa* b = bsa_init();
    assert(get_row_from_index(0) == 0);
    assert(get_row_from_index(1) == 1);
    assert(get_row_from_index(3) == 2);
    assert(get_row_from_index(7) == 3);
    assert(get_row_from_index(12) == 3);

    assert(is_row_empty(b, 0)== 1 );
    bsa_set(b, 0, 4);
    assert(is_row_empty(b, 0) == 0);

}