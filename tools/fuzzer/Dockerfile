FROM swift

RUN apt-get update && \
    apt-get install -y git build-essential clang libreadline-dev

RUN git clone https://github.com/googleprojectzero/fuzzilli

ADD . /fuzzilli/Targets/mujs

RUN git -C /fuzzilli/Targets/mujs apply /fuzzilli/Targets/mujs/tools/fuzzer/fuzz.patch

RUN make -C /fuzzilli/Targets/mujs \
    CC=clang \
    XCFLAGS="-DFUZZILLI -pipe -fsanitize=address -fno-omit-frame-pointer -fsanitize-coverage=trace-pc-guard" \
    LDFLAGS="-g -fsanitize=address -fsanitize-coverage=trace-pc-guard"

RUN cp /fuzzilli/Targets/mujs/tools/fuzzer/mujsProfile.swift /fuzzilli/Sources/FuzzilliCli/Profiles

RUN sed -i 's/jerryscript/mujs/g' /fuzzilli/Sources/FuzzilliCli/Profiles/Profile.swift

RUN mkdir -p /fuzzilli/FuzzStorage/old_corpus

WORKDIR /fuzzilli

RUN swift build

ENV NUM_CORES=1
ENV DURATION=0

CMD ["/bin/sh", "-c", "timeout $DURATION swift run FuzzilliCli --profile=mujs --storagePath=FuzzStorage/ --jobs=$NUM_CORES --resume Targets/mujs/build/release/mujs"]
