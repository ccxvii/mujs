import Fuzzilli

let mujsProfile = Profile(
    processArguments: [],

    processEnv: ["UBSAN_OPTIONS":"handle_segv=1", "ASAN_OPTIONS":"detect_leaks=0, abort_on_error=1"],

    codePrefix: """
                function main() {
                """,

    codeSuffix: """
                }
                main();
                """,

    ecmaVersion: ECMAScriptVersion.es5,

    crashTests: ["fuzzilli('FUZZILLI_CRASH', 0)", "fuzzilli('FUZZILLI_CRASH', 1)", "fuzzilli('FUZZILLI_CRASH', 2)"],

    additionalCodeGenerators: WeightedList<CodeGenerator>([]),

    additionalProgramTemplates: WeightedList<ProgramTemplate>([]),

    disabledCodeGenerators: [],

    additionalBuiltins: [
        "gc"                : .function([] => .undefined),
        "print"             : .function([] => .undefined),
    ]
)
