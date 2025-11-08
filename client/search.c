#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 100
#define ROWS 5
#define COLS 5

typedef struct {
    int matrix[ROWS][COLS];
} grid_t;

typedef struct {
    int row;
    int col;
} point_t;

typedef struct {
    int f, g, h;
    point_t parent;
    int is_closed;
} cell_t;

cell_t cell_details[ROWS][COLS];

int is_valid(int row, int col)
{
    return (row >= 0) && (row < ROWS) && (col >= 0) && (col < COLS);
}

int is_destination(int row, int col, point_t dest)
{
    return row == dest.row && col == dest.col;
}

int calculate_h_value(int row, int col, point_t dest)
{
    return abs(row - dest.row) + abs(col - dest.col);
}

void trace_path(point_t dest)
{
    printf("The path is: \n");
    int row = dest.row;
    int col = dest.col;
    while (!(cell_details[row][col].parent.row == row && cell_details[row][col].parent.col == col)) {
        printf("(%d, %d) <- ", row, col);
        int temp_row = cell_details[row][col].parent.row;
        int temp_col = cell_details[row][col].parent.col;
        row = temp_row;
        col = temp_col;
    }
    printf("(%d, %d)\n", row, col);
}

void initialize_cell_details()
{
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            cell_details[i][j].f = INT_MAX;
            cell_details[i][j].g = INT_MAX;
            cell_details[i][j].h = INT_MAX;
            cell_details[i][j].parent.row = -1;
            cell_details[i][j].parent.col = -1;
            cell_details[i][j].is_closed = 0;
        }
    }
}

void add_to_open_set(int open_set[], int *open_set_count, int row, int col)
{
    open_set[(*open_set_count)++] = row * COLS + col;
}

void remove_from_open_set(int open_set[], int *open_set_count, int index)
{
    open_set[index] = open_set[--(*open_set_count)];
}

void process_neighbor(grid_t grid, point_t dest, int open_set[], int *open_set_count, int row, int col, int new_row, int new_col)
{
    if (is_valid(new_row, new_col)) {
        if (is_destination(new_row, new_col, dest)) {
            cell_details[new_row][new_col].parent.row = row;
            cell_details[new_row][new_col].parent.col = col;
            trace_path(dest);
            exit(0);
        } else if (!cell_details[new_row][new_col].is_closed && grid.matrix[new_row][new_col] == 1) {
            int g_new = cell_details[row][col].g + 1;
            int h_new = calculate_h_value(new_row, new_col, dest);
            int f_new = g_new + h_new;

            if (cell_details[new_row][new_col].f == INT_MAX || cell_details[new_row][new_col].f > f_new) {
                add_to_open_set(open_set, open_set_count, new_row, new_col);
                cell_details[new_row][new_col].f = f_new;
                cell_details[new_row][new_col].g = g_new;
                cell_details[new_row][new_col].h = h_new;
                cell_details[new_row][new_col].parent.row = row;
                cell_details[new_row][new_col].parent.col = col;
            }
        }
    }
}

void a_star_search(grid_t grid, point_t src, point_t dest)
{
    if (!is_valid(src.row, src.col) || !is_valid(dest.row, dest.col)) {
        printf("Source or destination is invalid\n");
        return;
    }

    if (grid.matrix[src.row][src.col] == 0 || grid.matrix[dest.row][dest.col] == 0) {
        printf("Source or destination is blocked\n");
        return;
    }

    if (is_destination(src.row, src.col, dest)) {
        printf("We are already at the destination\n");
        return;
    }

    initialize_cell_details();

    int open_set[MAX_SIZE];
    int open_set_count = 0;

    int row = src.row;
    int col = src.col;

    cell_details[row][col].f = 0;
    cell_details[row][col].g = 0;
    cell_details[row][col].h = 0;
    cell_details[row][col].parent.row = row;
    cell_details[row][col].parent.col = col;

    add_to_open_set(open_set, &open_set_count, row, col);

    int row_num[] = { -1, 0, 0, 1 };
    int col_num[] = { 0, -1, 1, 0 };

    while (open_set_count > 0) {
        int min_f = INT_MAX;
        int min_index = -1;

        for (int i = 0; i < open_set_count; i++) {
            int idx = open_set[i];
            int r = idx / COLS;
            int c = idx % COLS;
            if (cell_details[r][c].f < min_f) {
                min_f = cell_details[r][c].f;
                min_index = i;
            }
        }

        int idx = open_set[min_index];
        row = idx / COLS;
        col = idx % COLS;

        remove_from_open_set(open_set, &open_set_count, min_index);
        cell_details[row][col].is_closed = 1;

        for (int i = 0; i < 4; i++) {
            int new_row = row + row_num[i];
            int new_col = col + col_num[i];
            process_neighbor(grid, dest, open_set, &open_set_count, row, col, new_row, new_col);
        }
    }

    printf("Failed to find the destination cell\n");
}

int main()
{
    grid_t grid = {
        .matrix = {
            { 1, 1, 1, 1, 1 },
            { 0, 1, 0, 0, 1 },
            { 0, 1, 0, 1, 1 },
            { 1, 1, 0, 1, 0 },
            { 1, 1, 1, 1, 1 } }
    };

    point_t src = { 0, 0 };
    point_t dest = { 4, 4 };

    a_star_search(grid, src, dest);

    return 0;
}
