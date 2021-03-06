#include <vslc.h>

void node_print(node_t *root, int nesting)
{
    if (root != NULL)
    {
        printf("%*c%s", nesting, ' ', node_string[root->type]);
        if (root->type == IDENTIFIER_DATA ||
            root->type == STRING_DATA ||
            root->type == RELATION ||
            root->type == EXPRESSION)
            printf("(%s)", (char *)root->data);
        else if (root->type == NUMBER_DATA)
            printf("(%ld)", *((int64_t *)root->data));
        putchar('\n');
        for (int64_t i = 0; i < root->n_children; i++)
            node_print(root->children[i], nesting + 1);
    }
    else
        printf("%*c%p\n", nesting, ' ', root);
}

void node_init(node_t *nd, node_index_t type, void *data, uint64_t n_children, ...)
{
    va_list child_list;
    *nd = (node_t){
        .type = type,
        .data = data,
        .entry = NULL,
        .n_children = n_children,
        .children = (node_t **)malloc(n_children * sizeof(node_t *))};
    va_start(child_list, n_children);
    for (uint64_t i = 0; i < n_children; i++)
        nd->children[i] = va_arg(child_list, node_t *);
    va_end(child_list);
}

void node_finalize(node_t *discard)
{
    if (discard != NULL)
    {
        free(discard->data);
        free(discard->children);
        free(discard);
    }
}

void destroy_subtree(node_t *discard)
{
    if (discard != NULL)
    {
        for (uint64_t i = 0; i < discard->n_children; i++)
            destroy_subtree(discard->children[i]);
        node_finalize(discard);
    }
}

// Flattens a node by taking the nth grandchildren as its own children and killing the nth child
// No breaches of the Geneva convention here, officer, just move along
void flatten_child(node_t *node, int n)
{
    node_t *child = node->children[n];
    // Attach the nested list's children to this node instead
    int old_n_children = node->n_children;
    // We're going to remove the child, so we need to reduce the amount of children by 1
    node->n_children = old_n_children + child->n_children - 1;

    // A S S I M I L A T E  C H I L D R E N
    node_t **new_children = calloc(node->n_children, sizeof(node_t *));
    for (int j = 0; j < child->n_children; j++)
    {
        new_children[j] = child->children[j];
    }
    int k = child->n_children;
    for (int j = 0; j < old_n_children; j++)
    {
        // Don't keep the child we're getting rid of
        if (n == j)
            continue;
        new_children[k++] = node->children[j];
    }
    free(node->children);
    node->children = new_children;

    // K I L L  C H I- no wait
    // "F I N A L I Z E"  C H I L D
    // :)
    // ;)
    node_finalize(child);
}

long apply_unary(char op, long param)
{
    switch (op)
    {
    case '-':
        return -param;
    case '~':
        return ~param;
    default:
        perror("Invalid unary operator");
    }
}

long apply_binary(char op, long param1, long param2)
{
    switch (op)
    {
    case '+':
        return param1 + param2;
    case '-':
        return param1 - param2;
    case '*':
        return param1 * param2;
    case '/':
        return param1 / param2;
    case '<':
        return param1 << param2;
    case '>':
        return param1 >> param2;
    case '|':
        return param1 | param2;
    case '&':
        return param1 & param2;
    case '^':
        return param1 ^ param2;
    default:
        perror("Invalid binary operator");
    }
}

void simplify_tree(node_t **simplified, node_t *root)
{
    // Depth first recursion
    node_t node = *root;
    for (int i = 0; i < node.n_children; i++)
    {
        if (node.children[i] != NULL)
        {
            simplify_tree(&node.children[i], node.children[i]);
        }
    }

    // Remove purely syntactic nodes
    if (root->data == NULL &&
        root->n_children == 1 &&
        root->type != DECLARATION &&
        root->type != PRINT_STATEMENT &&
        root->type != RETURN_STATEMENT)
    {
        *simplified = root->children[0];
        node_finalize(root);
    }

    root = *simplified;
    // Simplify list structures (task 2)
    if (root->type == GLOBAL_LIST ||
        root->type == STATEMENT_LIST ||
        root->type == PRINT_LIST ||
        root->type == EXPRESSION_LIST ||
        root->type == VARIABLE_LIST ||
        root->type == ARGUMENT_LIST ||
        root->type == PARAMETER_LIST ||
        root->type == DECLARATION_LIST)
    {
        for (int i = 0; i < root->n_children; i++)
        {
            // Nested list of the same type
            if (root->children[i]->type == root->type)
            {
                flatten_child(root, i);
            }
        }
    }

    // Special case: print lists can directly associate their children with the print statement node
    if (root->type == PRINT_STATEMENT && root->n_children > 0 && root->children[0]->type == PRINT_LIST)
    {
        flatten_child(root, 0);
    }

    if (root->type == EXPRESSION)
    {
        bool is_constant = true;
        for (int i = 0; i < root->n_children; i++)
        {
            if (root->children[i]->type != NUMBER_DATA)
            {
                is_constant = false;
                break;
            }
        }
        if (is_constant)
        {
            long number_value;
            long param1 = *((long *)root->children[0]->data);
            long param2;
            if (root->n_children > 1)
            {
                param2 = *((long *)root->children[1]->data);
                number_value = apply_binary(((char *)root->data)[0], param1, param2);
            }
            else
            {
                number_value = apply_unary(((char *)root->data)[0], param1);
            }
            destroy_subtree(root);
            root = malloc(sizeof(node_t));
            node_init(root, NUMBER_DATA, malloc(sizeof(long)), 0);
            *(long *)root->data = number_value;
            *simplified = root;
        }
    }
}
