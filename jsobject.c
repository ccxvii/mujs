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

static js_Property undefined = { "", &sentinel, &sentinel, 0, { {0}, JS_TUNDEFINED } };

static js_Property *newproperty(const char *key)
{
	js_Property *node = malloc(sizeof(js_Property));
	node->key = strdup(key);
	node->left = node->right = &sentinel;
	node->level = 1;
//	node->value = 0;
	return node;
}

static js_Property *lookup(js_Property *node, const char *key)
{
	while (node != &sentinel) {
		int c = strcmp(key, node->key);
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

static js_Property *insert(js_Property *node, const char *key, js_Property **result)
{
	if (node != &sentinel) {
		int c = strcmp(key, node->key);
		if (c < 0)
			node->left = insert(node->left, key, result);
		else if (c > 0)
			node->right = insert(node->right, key, result);
		else
			return *result = node;
		node = skew(node);
		node = split(node);
		return node;
	}
	return *result = newproperty(key);
}

js_Object *js_newobject(js_State *J)
{
	js_Object *obj = malloc(sizeof(js_Object));
	obj->properties = &sentinel;
	return obj;
}

js_Property *js_getproperty(js_State *J, js_Object *obj, const char *name)
{
	return lookup(obj->properties, name);
}

js_Property *js_setproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *result;
	obj->properties = insert(obj->properties, name, &result);
	return result;
}

static void js_dumpvalue(js_State *J, js_Value v)
{
	switch (v.type) {
	case JS_TUNDEFINED: printf("undefined"); break;
	case JS_TNULL: printf("null"); break;
	case JS_TBOOLEAN: printf(v.u.boolean ? "true" : "false"); break;
	case JS_TNUMBER: printf("%.9g", v.u.number); break;
	case JS_TSTRING: printf("'%s'", v.u.string); break;
	case JS_TOBJECT: printf("<object %p>", v.u.object); break;
	case JS_TCLOSURE: printf("<closure %p>", v.u.closure); break;
	case JS_TCFUNCTION: printf("<cfunction %p>", v.u.cfunction); break;
	}
}

static void js_dumpproperty(js_State *J, js_Property *node)
{
	if (node->left != &sentinel)
		js_dumpproperty(J, node->left);
	printf("\t%s: ", node->key);
	js_dumpvalue(J, node->value);
	printf(",\n");
	if (node->right != &sentinel)
		js_dumpproperty(J, node->right);
}

void js_dumpobject(js_State *J, js_Object *obj)
{
	printf("{\n");
	if (obj->properties != &sentinel)
		js_dumpproperty(J, obj->properties);
	printf("}\n");
}
