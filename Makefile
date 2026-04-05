# ============================================================
# Configuration variables (override via CLI, e.g. make MICROKIT_BOARD=rpi4)
# ============================================================
MICROKIT_CONFIG ?= debug
MICROKIT_BOARD  ?= odroidc4
MQ              ?= $(HOME)/wsp/machine_queue
PATCHES         := $(CURDIR)/patches

PYTHON := $(CURDIR)/pyenv/bin/python3
PIP    := $(CURDIR)/pyenv/bin/pip

MICROKIT_MULTIKERNEL 	:= $(CURDIR)/microkit-multikernel
MICROKIT_SMP      		:= $(CURDIR)/microkit-smp
MICROKIT_UNICORE     	:= $(CURDIR)/microkit-unicore

# ============================================================
# Top-level targets
# ============================================================
.PHONY: all multikernel smp unicore \
        setup setup-python setup-rust setup-submodules \
        run run-multikernel run-smp run-unicore \
        clean reset

all: multikernel smp unicore

# ============================================================
# Environment setup
# ============================================================
setup: setup-python setup-rust setup-submodules

setup-python:
	@echo ">>> Setting up Python virtual environment..."
	@if [ -d "pyenv" ] && [ -f "pyenv/bin/activate" ]; then \
		echo "INFO: Virtual environment already exists, skipping creation."; \
	else \
		echo "INFO: Creating virtual environment..."; \
		python3 -m venv pyenv; \
	fi
	$(PIP) install --upgrade pip setuptools wheel
	$(PIP) install -r requirements.txt

REQUIRED_RUSTC := 1.94.0

setup-rust:
	@echo ">>> Setting up Rust toolchain..."
	rustup update stable
	rustup install 1.94.0
	rustup default 1.94.0
	@echo ">>> Verifying rustc version (required >= $(REQUIRED_RUSTC))..."
	@ACTUAL=$$(rustc --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1); \
	printf '%s\n%s\n' "$(REQUIRED_RUSTC)" "$$ACTUAL" | sort -V -C || \
		{ echo "ERROR: rustc $$ACTUAL is older than required $(REQUIRED_RUSTC)"; exit 1; }; \
	echo "OK: rustc $$ACTUAL >= $(REQUIRED_RUSTC)"
	rustup target add x86_64-unknown-linux-musl
	rustup target add aarch64-unknown-none
	rustup component add rust-src --toolchain 1.94.0-x86_64-unknown-linux-gnu

setup-submodules:
	@echo ">>> Initializing Git submodules and applying patches..."
	git submodule update --init --recursive
	cd $(MICROKIT_MULTIKERNEL) 	&& git am $(PATCHES)/multikernel/*
	cd $(MICROKIT_SMP)        	&& git am $(PATCHES)/smp/*
	cd $(MICROKIT_UNICORE)    	&& git am $(PATCHES)/unicore/*

# ============================================================
# Build targets
# ============================================================
multikernel: setup-submodules
	@echo ">>> Building multikernel (board=$(MICROKIT_BOARD)_multikernel, config=$(MICROKIT_CONFIG))..."
	cd $(MICROKIT_MULTIKERNEL) && \
		$(PYTHON) build_sdk.py \
			--sel4=../seL4-multikernel \
			--boards $(MICROKIT_BOARD)_multikernel \
			--configs $(MICROKIT_CONFIG) \
			--skip-docs --skip-tar && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example benchmarks \
			--board $(MICROKIT_BOARD)_multikernel

smp: setup-submodules
	@echo ">>> Building smp (board=$(MICROKIT_BOARD), config=$(MICROKIT_CONFIG))..."
	cd $(MICROKIT_SMP) && \
		$(PYTHON) build_sdk.py \
			--sel4=../seL4-mainline \
			--boards $(MICROKIT_BOARD) \
			--configs $(MICROKIT_CONFIG) \
			--skip-docs --skip-tar && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example benchmarks \
			--board $(MICROKIT_BOARD)

unicore: setup-submodules
	@echo ">>> Building unicore (board=$(MICROKIT_BOARD), config=$(MICROKIT_CONFIG))..."
	cd $(MICROKIT_UNICORE) && \
		$(PYTHON) build_sdk.py \
			--sel4=../seL4-mainline \
			--boards $(MICROKIT_BOARD) \
			--configs $(MICROKIT_CONFIG) \
			--skip-docs --skip-tar && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example benchmarks \
			--board $(MICROKIT_BOARD)

# ============================================================
# Run targets
# ============================================================
run: run-multikernel run-smp run-unicore

run-multikernel:
	@echo ">>> Running multikernel..."
	cd $(MICROKIT_MULTIKERNEL) && \
		$(MQ)/mq.sh run -s odroidc4_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

run-smp:
	@echo ">>> Running smp..."
	cd $(MICROKIT_SMP) && \
		$(MQ)/mq.sh run -s odroidc4_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

run-unicore:
	@echo ">>> Running unicore..."
	cd $(MICROKIT_UNICORE) && \
		$(MQ)/mq.sh run -s odroidc4_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

# ============================================================
# Clean
# ============================================================
clean:
	rm -rf $(MICROKIT_MULTIKERNEL)/build $(MICROKIT_MULTIKERNEL)/release
	rm -rf $(MICROKIT_SMP)/build $(MICROKIT_SMP)/release
	rm -rf $(MICROKIT_UNICORE)/build $(MICROKIT_UNICORE)/release
	rm -rf $(MICROKIT_MULTIKERNEL)/tmp_build
	rm -rf $(MICROKIT_SMP)/tmp_build
	rm -rf $(MICROKIT_UNICORE)/tmp_build

reset: clean
	git submodule foreach --recursive 'git am --abort || true'
	git submodule update --init --recursive --force
