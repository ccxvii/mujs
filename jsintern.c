#include "jsi.h"

/* Use an AA-tree to quickly look up interned strings. */

struct js_StringNode
{
	js_StringNode *left, *right;
	int level;
	char string[1];
};

static js_StringNode sentinel = { &sentinel, &sentinel, 0, ""};

static js_StringNode *newstringnode(const char *string, const char **result)
{
	int n = strlen(string);
	js_StringNode *node = malloc(offsetof(js_StringNode, string) + n + 1);
	node->left = node->right = &sentinel;
	node->level = 1;
	strcpy(node->string, string);
	return *result = node->string, node;
}

static js_StringNode *skew(js_StringNode *node)
{
	if (node->level != 0) {
		if (node->left->level == node->level) {
			js_StringNode *save = node;
			node = node->left;
			save->left = node->right;
			node->right = save;
		}
		node->right = skew(node->right);
	}
	return node;
}

static js_StringNode *split(js_StringNode *node)
{
	if (node->level != 0 && node->right->right->level == node->level) {
		js_StringNode *save = node;
		node = node->right;
		save->right = node->left;
		node->left = save;
		node->level++;
		node->right = split(node->right);
	}
	return node;
}

static js_StringNode *insert(js_StringNode *node, const char *string, const char **result)
{
	if (node != &sentinel) {
		int c = strcmp(string, node->string);
		if (c < 0)
			node->left = insert(node->left, string, result);
		else if (c > 0)
			node->right = insert(node->right, string, result);
		else
			return *result = node->string, node;
		node = skew(node);
		node = split(node);
		return node;
	}
	return newstringnode(string, result);
}

static void dumpstringnode(js_StringNode *node, int level)
{
	int i;
	if (node->left != &sentinel)
		dumpstringnode(node->left, level + 1);
	printf("%d: ", node->level);
	for (i = 0; i < level; ++i)
		putchar('\t');
	printf("'%s'\n", node->string);
	if (node->right != &sentinel)
		dumpstringnode(node->right, level + 1);
}

void jsS_dumpstrings(js_State *J)
{
	js_StringNode *root = J->strings;
	printf("interned strings {\n");
	if (root && root != &sentinel)
		dumpstringnode(root, 1);
	printf("}\n");
}

static void js_freestringnode(js_State *J, js_StringNode *node)
{
	if (node->left != &sentinel) js_freestringnode(J, node->left);
	if (node->right != &sentinel) js_freestringnode(J, node->right);
	free(node);
}

void jsS_freestrings(js_State *J)
{
	if (J->strings && J->strings != &sentinel)
		js_freestringnode(J, J->strings);
}

const char *js_intern(js_State *J, const char *s)
{
	const char *result;
	if (!J->strings)
		J->strings = &sentinel;
	J->strings = insert(J->strings, s, &result);
	return result;
}
