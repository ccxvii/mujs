#include "js.h"
#include "jscompile.h"
#include "jsrun.h"
#include "jsobject.h"
#include "jsstate.h"

static void jsG_freefunction(js_State *J, js_Function *fun)
{
	free(fun->params);
	free(fun->funtab);
	free(fun->numtab);
	free(fun->strtab);
	free(fun->code);
	free(fun);
}

static void jsG_freeenvironment(js_State *J, js_Environment *env)
{
	free(env);
}

static void jsG_freeproperty(js_State *J, js_Property *node)
{
	if (node->left->level) jsG_freeproperty(J, node->left);
	if (node->right->level) jsG_freeproperty(J, node->right);
	free(node);
}

static void jsG_freeobject(js_State *J, js_Object *obj)
{
	if (obj->properties->level)
		jsG_freeproperty(J, obj->properties);
	free(obj);
}

void js_gc(js_State *J)
{
}

void js_close(js_State *J)
{
	js_Function *fun, *nextfun;
	js_Object *obj, *nextobj;
	js_Environment *env, *nextenv;

	for (env = J->gcenv; env; env = nextenv)
		nextenv = env->gcnext, jsG_freeenvironment(J, env);
	for (fun = J->gcfun; fun; fun = nextfun)
		nextfun = fun->gcnext, jsG_freefunction(J, fun);
	for (obj = J->gcobj; obj; obj = nextobj)
		nextobj = obj->gcnext, jsG_freeobject(J, obj);

	js_freestrings(J);

	free(J->buf.text);
	free(J);
}
