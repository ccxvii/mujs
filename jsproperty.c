#include "js.h"
#include "jsobject.h"

/*
	Use an AA-tree to quickly look up properties in objects:

	The level of every leaf node is one.
	The level of every left child is one less than its parent.
	The level of every right child is equal or one less than its parent.
	The level of every right grandchild is less than its grandparent.
	Every node of level greater than one has two children.

	A link where the child's level is equal to that of its parent is called a horizontal link.
	Individual right horizontal links are allowed, but consecutive ones are forbidden.
	Left horizontal links are forbidden.

	skew() fixes left horizontal links.
	split() fixes consecutive right horizontal links.
*/

static js_Property sentinel = { "", &sentinel, &sentinel, 0 };

static js_Property *newproperty(const char *name)
{
	js_Property *node = malloc(sizeof(js_Property));
	node->name = strdup(name);
	node->left = node->right = &sentinel;
	node->level = 1;
	node->value.type = JS_TUNDEFINED;
	node->value.u.number = 0;
	node->flags = 0;
	return node;
}

static js_Property *lookup(js_Property *node, const char *name)
{
	while (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c == 0)
			return node;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;
}

static inline js_Property *skew(js_Property *node)
{
	if (node->level != 0) {
		if (node->left->level == node->level) {
			js_Property *save = node;
			node = node->left;
			save->left = node->right;
			node->right = save;
		}
		node->right = skew(node->right);
	}
	return node;
}

static inline js_Property *split(js_Property *node)
{
	if (node->level != 0 && node->right->right->level == node->level) {
		js_Property *save = node;
		node = node->right;
		save->right = node->left;
		node->left = save;
		node->level++;
		node->right = split(node->right);
	}
	return node;
}

static js_Property *insert(js_Property *node, const char *name, js_Property **result)
{
	if (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c < 0)
			node->left = insert(node->left, name, result);
		else if (c > 0)
			node->right = insert(node->right, name, result);
		else
			return *result = node;
		node = skew(node);
		node = split(node);
		return node;
	}
	return *result = newproperty(name);
}

static js_Property *lookupfirst(js_Property *node)
{
	while (node != &sentinel) {
		if (node->left == &sentinel)
			return node;
		node = node->left;
	}
	return NULL;
}

static js_Property *lookupnext(js_Property *node, const char *name)
{
	js_Property *stack[100], *parent;
	int top = 0;

	stack[0] = NULL;
	while (node != &sentinel) {
		stack[++top] = node;
		int c = strcmp(name, node->name);
		if (c == 0)
			goto found;
		else if (c < 0)
			node = node->left;
		else
			node = node->right;
	}
	return NULL;

found:
	if (node->right != &sentinel)
		return lookupfirst(node->right);
	parent = stack[--top];
	while (parent && node == parent->right) {
		node = parent;
		parent = stack[--top];
	}
	return parent;
}

js_Object *jsR_newobject(js_State *J, js_Class type, js_Object *prototype)
{
	js_Object *obj = malloc(sizeof(js_Object));
	obj->type = type;
	obj->properties = &sentinel;
	obj->prototype = prototype;
	obj->primitive.number = 0;
	obj->scope = NULL;
	obj->function = NULL;
	obj->cfunction = NULL;
	return obj;
}

js_Property *jsR_getownproperty(js_State *J, js_Object *obj, const char *name)
{
	return lookup(obj->properties, name);
}

js_Property *jsR_getproperty(js_State *J, js_Object *obj, const char *name)
{
	do {
		js_Property *ref = lookup(obj->properties, name);
		if (ref)
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

js_Property *jsR_setproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *result;
	obj->properties = insert(obj->properties, name, &result);
	return result;
}

js_Property *jsR_nextproperty(js_State *J, js_Object *obj, const char *name)
{
	if (!name)
		return lookupfirst(obj->properties);
	return lookupnext(obj->properties, name);
}
