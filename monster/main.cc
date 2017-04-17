#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

#include <setjmp.h>

extern "C" {
#include "mujs.h"
#include "jsi.h"
#include "jsparse.h"
}

string read(std::istream &in) {
    vector<char> v;
    char c;
    while ((c = in.get()) != EOF) {
        v.push_back(c);
    }
    return string(v.begin(), v.end());
}

int do_ast(js_State *state, js_Ast *ast) {
    jsP_dumplist(state, ast);
    return 0;
}

int dump_ast(const string &code) {
    js_State *state;
    state = js_newstate(NULL, NULL, JS_STRICT);

    if (js_try(state)) {
        jsP_freeparse(state);
        js_throw(state);
    }

    js_Ast *ast = jsP_parse(state, NULL, code.c_str());
    do_ast(state, ast);
    jsP_freeparse(state);

    js_endtry(state);

    js_freestate(state);
    return 0;
}

int main(int argc, char *argv[]) {
    std::ifstream fin;
    if (argc == 2) {
        fin.open(argv[1]);
        if (!fin) {
            return -1;
        }
    }
    string buffer = read(argc == 2 ? fin : std::cin);
    dump_ast(buffer);
    if (fin) {
        fin.close();
    }
        //js_State *J;
        //J = js_newstate(NULL, NULL, JS_STRICT);
        //jsY_initlex(J, argv[1], NULL);
        //jsY_lex(J);
        //js_freestate(J);
    return 0;
}
