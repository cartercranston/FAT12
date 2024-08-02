void add_dir_to_stack(int val) {
    if (val == -1) {
        printf("Warning: Adding -1 to dir_list.\n");
    }
    if (dir_list->val == -1) {
        dir_list->val = val;
    } else {
        // push the value to the beginning of the stack, and move the rest of the values over
        stack_node * new = malloc(sizeof(stack_node));
        new->val = val;
        new->next = dir_list;
        dir_list = new;
    }
}

int get_dir_from_stack() {
    if (dir_list->val == -1) {
        return -1;
    } else if (dir_list->next == NULL) {
        // pop the sole value on the stack
        int val = dir_list->val;
        dir_list->val = -1;
        return val;
    } else {
        // pop the first value on the stack, and move the rest over
        stack_node * top = dir_list;
        dir_list = dir_list->next;
        int val = top->val;
        free(top);
        return val;
    }
}