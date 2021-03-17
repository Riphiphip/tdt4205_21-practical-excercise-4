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

void simplify_tree(node_t **simplified, node_t *root)
{
    // Depth first recursion
    node_t node = *root;
    for (int i = 0; i < node.n_children; i++)
    {
        if (node.children[i] != NULL && node.children[i])
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
    else
    {
        *simplified = root;
    }

    // Update working node
    node = **simplified;
}