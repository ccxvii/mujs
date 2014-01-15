#ifndef js_run_h
#define js_run_h

void jsR_error(js_State *J, const char *fmt, ...);
void jsR_runfunction(js_State *J, js_Function *F);

#endif
