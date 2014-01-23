#include "jsi.h"
#include "jsvalue.h"

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

static js_Property *newproperty(js_State *J, const char *name)
{
	js_Property *node = malloc(sizeof(js_Property));
	node->name = js_intern(J, name);
	node->left = node->right = &sentinel;
	node->level = 1;
	node->atts = 0;
	node->value.type = JS_TUNDEFINED;
	node->value.u.number = 0;
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

static js_Property *insert(js_State *J, js_Property *node, const char *name, js_Property **result)
{
	if (node != &sentinel) {
		int c = strcmp(name, node->name);
		if (c < 0)
			node->left = insert(J, node->left, name, result);
		else if (c > 0)
			node->right = insert(J, node->right, name, result);
		else
			return *result = node;
		node = skew(node);
		node = split(node);
		return node;
	}
	return *result = newproperty(J, name);
}

js_Object *jsV_newobject(js_State *J, js_Class type, js_Object *prototype)
{
	js_Object *obj = calloc(sizeof(js_Object), 1);
	obj->gcmark = 0;
	obj->gcnext = J->gcobj;
	J->gcobj = obj;
	++J->gccounter;

	obj->type = type;
	obj->properties = &sentinel;
	obj->prototype = prototype;
	return obj;
}

js_Property *jsV_getownproperty(js_State *J, js_Object *obj, const char *name)
{
	return lookup(obj->properties, name);
}

js_Property *jsV_getproperty(js_State *J, js_Object *obj, const char *name)
{
	do {
		js_Property *ref = lookup(obj->properties, name);
		if (ref)
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

static js_Property *jsV_getenumproperty(js_State *J, js_Object *obj, const char *name)
{
	do {
		js_Property *ref = lookup(obj->properties, name);
		if (ref && !(ref->atts & JS_DONTENUM))
			return ref;
		obj = obj->prototype;
	} while (obj);
	return NULL;
}

js_Property *jsV_setproperty(js_State *J, js_Object *obj, const char *name)
{
	js_Property *result;
	obj->properties = insert(J, obj->properties, name, &result);
	return result;
}

/* Flatten hierarchy of enumerable properties into an iterator object */

static js_Iterator *itwalk(js_State *J, js_Iterator *iter, js_Property *prop, js_Object *seen)
{
	if (prop->right != &sentinel)
		iter = itwalk(J, iter, prop->right, seen);
	if (!seen || !jsV_getenumproperty(J, seen, prop->name)) {
		js_Iterator *head = malloc(sizeof *head);
		head->name = prop->name;
		head->next = iter;
		iter = head;
	}
	if (prop->left != &sentinel)
		iter = itwalk(J, iter, prop->left, seen);
	return iter;
}

static js_Iterator *itflatten(js_State *J, js_Object *obj)
{
	js_Iterator *iter = NULL;
	if (obj->prototype)
		iter = itflatten(J, obj->prototype);
	if (obj->properties != &sentinel)
		iter = itwalk(J, iter, obj->properties, obj->prototype);
	return iter;
}

js_Object *jsV_newiterator(js_State *J, js_Object *obj)
{
	js_Object *iobj = jsV_newobject(J, JS_CITERATOR, NULL);
	iobj->u.iterator.head = itflatten(J, obj);
	iobj->u.iterator.next = iobj->u.iterator.head;
	return iobj;
}

const char *jsV_nextiterator(js_State *J, js_Object *iobj)
{
	js_Iterator *result;
	if (iobj->type != JS_CITERATOR)
		js_typeerror(J, "not an iterator");
	result = iobj->u.iterator.next;
	if (result) {
		iobj->u.iterator.next = result->next;
		return result->name;
	}
	return NULL;
}
