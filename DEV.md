
```
$ python3 -m venv pyenv
$ ./pyenv/bin/pip install --upgrade pip setuptools wheel
$ ./pyenv/bin/pip install -r requirements.txt
```

```
$ rustup update stable
$ rustup install 1.94.0
$ rustup default 1.94.0
# make sure that rustc version equals to 1.94.0
$ rustc --version

$ rustup target add x86_64-unknown-linux-musl
$ rustup target add aarch64-unknown-none
```

```
$ git submodule update --init --recursive
$ echo PATCHES=$PWD/patches
$ cd microkit-multikernel
$ git am ../patches/multikernel/*
$ cd ..
$ cd microkit-smp
$ git am ../patches/smp/*
$ cd ..
$ cd microkit-unicore
$ git am ../patches/unicore/*
$ cd ..
```

```
$ echo MICROKIT_CONFIG=debug
$ echo MICROKIT_BOARD=odroidc4
$ cd microkit-multikernel
$ python3 build_sdk.py --sel4=../seL4-multikernel --boards $(MICROKIT_BOARD)_multikernel --configs $(MICROKIT_CONFIG) --skip-docs --skip-tar
$ python3 dev_build.py --rebuild --example benchmarks --board $(MICROKIT_BOARD)_multikernel
$ cd ..
$ cd microkit-smp
$ python3 build_sdk.py --sel4=../seL4-mainline --boards $(MICROKIT_BOARD) --configs $(MICROKIT_CONFIG) --skip-docs --skip-tar
$ python3 dev_build.py --rebuild --example benchmarks --board $(MICROKIT_BOARD)
$ cd ..
$ cd microkit-unicore
$ python3 build_sdk.py --sel4=../seL4-mainline --boards $(MICROKIT_BOARD) --configs $(MICROKIT_CONFIG) --skip-docs --skip-tar
$ python3 dev_build.py --rebuild --example benchmarks --board $(MICROKIT_BOARD)
$ cd ..
```

```
$ echo MQ=~/wsp/machine_queue
$ cd microkit-multikernel
$ $(MQ)/mq.sh run -s odroidc4_pool -f ./tmp_build/loader.img -c "All is well in the universe"
$ cd ..
$ cd microkit-smp
$ $(MQ)/mq.sh run -s odroidc4_pool -f ./tmp_build/loader.img -c "All is well in the universe"
$ cd microkit-unicore
$ $(MQ)/mq.sh run -s odroidc4_pool -f ./tmp_build/loader.img -c "All is well in the universe"
```