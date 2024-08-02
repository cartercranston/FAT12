typedef struct stack_node {
    int val;
    struct stack_node * next;
} stack_node;

void add_dir_to_stack(int val);

int get_dir_from_stack();

stack_node * dir_list;