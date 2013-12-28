#include "js.h"

/* Use an AA-tree to quickly look up interned strings. */

struct js_StringNode
{
	const char *string;
	js_StringNode *left, *right;
	int level;
};

static js_StringNode sentinel = { "", &sentinel, &sentinel, 0 };

static js_StringNode *makestringnode(const char *string, const char **out)
{
	js_StringNode *node = malloc(sizeof(js_StringNode));
	node->string = *out = strdup(string);
	node->left = node->right = &sentinel;
	node->level = 1;
	return node;
}

static const char *lookup(js_StringNode *node, const char *string)
{
	if (node && node != &sentinel) {
		int c = strcmp(string, node->string);
		if (c == 0)
			return node->string;
		else if (c < 0)
			return lookup(node->left, string);
		else
			return lookup(node->right, string);
	}
	return NULL;
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

static js_StringNode *insert(js_StringNode *node, const char *string, const char **out)
{
	if (node && node != &sentinel) {
		int c = strcmp(string, node->string);
		if (c < 0)
			node->left = insert(node->left, string, out);
		else
			node->right = insert(node->right, string, out);
		node = skew(node);
		node = split(node);
		return node;
	} else {
		return makestringnode(string, out);
	}
}

static void printstringnode(js_StringNode *node, int level)
{
	int i;
	if (node->left != &sentinel)
		printstringnode(node->left, level + 1);
	for (i = 0; i < level; i++)
		putchar(' ');
	printf("'%s' (%d)\n", node->string, node->level);
	if (node->right != &sentinel)
		printstringnode(node->right, level + 1);
}

void js_printstringtree(js_State *J)
{
	js_StringNode *root = J->strings;
	printf("--- string dump ---\n");
	if (root && root != &sentinel)
		printstringnode(root, 0);
	printf("---\n");
}

const char *js_intern(js_State *J, const char *s)
{
	const char *a = lookup(J->strings, s);
	if (!a)
		J->strings = insert(J->strings, s, &a);
	return a;
}
