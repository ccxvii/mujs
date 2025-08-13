# Fuzzer

Fuzz MuJS with [fuzzilli](https://github.com/googleprojectzero/fuzzilli), a (coverage-)guided fuzzer for dynamic language interpreters based on a custom intermediate language ("FuzzIL") which can be mutated and translated to JavaScript.

## Usage

From the root of the git repository:

```sh
docker build -t fuzz-mujs -f tools/fuzzer/Dockerfile .
docker run -it --rm -e NUM_CORES=1 -e DURATION=0 fuzz-mujs
```

You can modify `NUM_CORES` and `DURATION`.
`NUM_CORES` specifies the number of cores that will be used while fuzzing.
`DURATION` specifies how long the fuzzing will run, with 0 indicating that it will run indefinitely.
