# ============================================================
# Configuration variables (override via CLI, e.g. make MICROKIT_BOARD=rpi4)
# ============================================================
MICROKIT_CONFIG ?= debug
MICROKIT_BOARD  ?= odroidc4
MQ              ?= $(HOME)/wsp/machine_queue
PATCHES         := $(CURDIR)/patches

PYTHON := $(CURDIR)/pyenv/bin/python3
PIP    := $(CURDIR)/pyenv/bin/pip

MICROKIT_MULTIKERNEL 		:= $(CURDIR)/microkit-multikernel
MICROKIT_CAPDL_MULTIKERNEL 	:= $(CURDIR)/microkit-capdl-multikernel
MICROKIT_SMP      			:= $(CURDIR)/microkit-smp
MICROKIT_UNICORE     		:= $(CURDIR)/microkit-unicore

RUST_SEL4 					:= $(CURDIR)/rust-seL4

KERNEL_CAPDL_MULTIKERNEL 	:= $(CURDIR)/seL4-capdl-multikernel
KERNEL_MAINLINE 			:= $(CURDIR)/seL4-mainline
KERNEL_MULTIKERNEL			:= $(CURDIR)/seL4-multikernel

# ============================================================
# Top-level targets
# ============================================================
.PHONY: all multikernel smp unicore capdl-multikernel \
        setup setup-python setup-rust setup-submodules \
        run run-multikernel run-smp run-unicore run-capdl-multikernel \
        clean reset setup-capdl-multikernel \
		build-capdl-multikernel test-capdl-multikernel \
		link link-multikernel link-smp link-unicore link-capdl-multikernel

all: multikernel smp unicore capdl-multikernel

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

# Example to symlink into each microkit's example/ directory
EXAMPLE ?= ppc-no-interference
# EXAMPLE_PASSIVE_SERVER ?= multikernel-passive-server
EXAMPLE_PASSIVE_SERVER ?= smp-passive-server
EXAMPLE := $(EXAMPLE_PASSIVE_SERVER)

# ============================================================
# Symlink helper (used by link-* targets)
# Usage: $(call create-symlink, <microkit-dir>, <example-name>)
# ============================================================
define create-symlink
	@echo ">>> Creating symlink: $(1)/example/$(2) -> $(CURDIR)/$(2)..."
	@if [ ! -d "$(CURDIR)/$(2)" ] && [ ! -f "$(CURDIR)/$(2)" ]; then \
		echo "ERROR: Source '$(CURDIR)/$(2)' does not exist, aborting."; \
		exit 1; \
	elif [ -L "$(1)/example/$(2)" ]; then \
		echo "INFO: Symlink already exists in $(1)/example/, skipping."; \
	elif [ -e "$(1)/example/$(2)" ]; then \
		echo "ERROR: '$(1)/example/$(2)' exists but is not a symlink, please remove it manually."; \
		exit 1; \
	else \
		ln -s $(CURDIR)/$(2) $(1)/example/$(2); \
		echo "INFO: Symlink created in $(1)/example/."; \
	fi
endef

# ============================================================
# Symlink targets
# ============================================================
.PHONY: link link-multikernel link-smp link-unicore link-capdl-multikernel

link: link-multikernel link-smp link-unicore link-capdl-multikernel

link-multikernel:
	$(call create-symlink,$(MICROKIT_MULTIKERNEL),$(EXAMPLE))

link-smp:
	$(call create-symlink,$(MICROKIT_SMP),$(EXAMPLE))

link-unicore:
	$(call create-symlink,$(MICROKIT_UNICORE),$(EXAMPLE))

link-capdl-multikernel:
	$(call create-symlink,$(MICROKIT_CAPDL_MULTIKERNEL),$(EXAMPLE_PASSIVE_SERVER))

# ============================================================
# Submodule setup (calls link after patching)
# ============================================================
setup-submodules:
	@echo ">>> Initializing Git submodules and applying patches..."
	git submodule update --init --recursive
	cd $(MICROKIT_MULTIKERNEL) 	&& git am $(PATCHES)/multikernel/*
	cd $(MICROKIT_SMP)        	&& git am $(PATCHES)/smp/*
	cd $(MICROKIT_UNICORE)    	&& git am $(PATCHES)/unicore/*
#	cd $(MICROKIT_CAPDL_MULTIKERNEL) && git am $(PATCHES)/capdl-multikernel/*
	cd $(KERNEL_MAINLINE)       && git am $(PATCHES)/kernel/mainline/*.patch
	cd $(KERNEL_MULTIKERNEL)	&& git am $(PATCHES)/kernel/multikernel/*.patch
	$(MAKE) link

# ============================================================
# Build targets
# ============================================================
SYSTEM_FILE := benchmarks-three-cores.system
export SYSTEM_FILE

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
			--example $(EXAMPLE) \
			--board $(MICROKIT_BOARD)_multikernel \
			--config $(MICROKIT_CONFIG)

capdl-multikernel: setup-capdl-multikernel
	@echo ">>> Building capdl-multikernel (board=$(MICROKIT_BOARD)_multikernel, config=$(MICROKIT_CONFIG))..."
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && \
		$(PYTHON) build_sdk.py \
			--sel4=$(KERNEL_CAPDL_MULTIKERNEL) \
			--boards $(MICROKIT_BOARD)_multikernel \
			--configs $(MICROKIT_CONFIG) \
			--skip-docs --skip-tar && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example $(EXAMPLE_PASSIVE_SERVER) \
			--board $(MICROKIT_BOARD)_multikernel \
			--config $(MICROKIT_CONFIG)

build-capdl-multikernel:
	@echo ">>> Building capdl-multikernel (board=$(MICROKIT_BOARD)_multikernel, config=$(MICROKIT_CONFIG))..."
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && \
		$(PYTHON) build_sdk.py \
			--sel4=$(KERNEL_CAPDL_MULTIKERNEL) \
			--boards $(MICROKIT_BOARD)_multikernel \
			--configs $(MICROKIT_CONFIG) \
			--skip-docs --skip-tar && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example $(EXAMPLE_PASSIVE_SERVER) \
			--board $(MICROKIT_BOARD)_multikernel \
			--config $(MICROKIT_CONFIG)

test-capdl-multikernel:
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && \
		$(PYTHON) dev_build.py \
			--rebuild \
			--example $(EXAMPLE_PASSIVE_SERVER) \
			--board $(MICROKIT_BOARD)_multikernel \
			--config $(MICROKIT_CONFIG)

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
			--example $(EXAMPLE) \
			--board $(MICROKIT_BOARD) \
			--config $(MICROKIT_CONFIG)

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
			--example $(EXAMPLE) \
			--board $(MICROKIT_BOARD) \
			--config $(MICROKIT_CONFIG)

# ============================================================
# Run targets
# ============================================================
run: run-multikernel run-smp run-unicore run-capdl-multikernel

run-multikernel:
	@echo ">>> Running multikernel..."
	cd $(MICROKIT_MULTIKERNEL) && \
		$(MQ)/mq.sh run -s $(MICROKIT_BOARD)_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

run-capdl-multikernel:
	@echo ">>> Running capdl-multikernel..."
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && \
		$(MQ)/mq.sh run -s $(MICROKIT_BOARD)_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

run-smp:
	@echo ">>> Running smp..."
	cd $(MICROKIT_SMP) && \
		$(MQ)/mq.sh run -s $(MICROKIT_BOARD)_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

run-unicore:
	@echo ">>> Running unicore..."
	cd $(MICROKIT_UNICORE) && \
		$(MQ)/mq.sh run -s $(MICROKIT_BOARD)_pool \
			-f ./tmp_build/loader.img \
			-c "All is well in the universe"

# ============================================================
# Clean
# ============================================================
clean:
	rm -rf $(MICROKIT_MULTIKERNEL)/build $(MICROKIT_MULTIKERNEL)/release
	rm -rf $(MICROKIT_CAPDL_MULTIKERNEL)/build $(MICROKIT_CAPDL_MULTIKERNEL)/release
	rm -rf $(MICROKIT_SMP)/build $(MICROKIT_SMP)/release
	rm -rf $(MICROKIT_UNICORE)/build $(MICROKIT_UNICORE)/release
	rm -rf $(MICROKIT_MULTIKERNEL)/tmp_build
	rm -rf $(MICROKIT_CAPDL_MULTIKERNEL)/tmp_build
	rm -rf $(MICROKIT_SMP)/tmp_build
	rm -rf $(MICROKIT_UNICORE)/tmp_build

setup-capdl-multikernel:
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && git am --abort || true
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && git reset --hard HEAD
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && git clean -fdx
	cd $(CURDIR) && rm -rf $(MICROKIT_CAPDL_MULTIKERNEL)/tmp_build
	@echo ">>> Removing symlink from $(MICROKIT_CAPDL_MULTIKERNEL)/example/$(EXAMPLE)..."
	@if [ -L "$(MICROKIT_CAPDL_MULTIKERNEL)/example/$(EXAMPLE)" ]; then \
		rm -v $(MICROKIT_CAPDL_MULTIKERNEL)/example/$(EXAMPLE); \
	else \
		echo "INFO: No symlink found in $(MICROKIT_CAPDL_MULTIKERNEL)/example/, skipping."; \
	fi
	git submodule update --init --recursive --force -- $(MICROKIT_CAPDL_MULTIKERNEL)
	cd $(MICROKIT_CAPDL_MULTIKERNEL) && git am $(PATCHES)/capdl-multikernel/*
	git submodule update --init --recursive --force -- $(KERNEL_CAPDL_MULTIKERNEL)
#	cd $(KERNEL_CAPDL_MULTIKERNEL) && git am $(PATCHES)/kernel/capdl-multikernel/*
	git submodule update --init --recursive --force -- $(RUST_SEL4)
	$(call create-symlink,$(MICROKIT_CAPDL_MULTIKERNEL),$(EXAMPLE_PASSIVE_SERVER))
	@echo ">>> Reset complete for capdl-multikernel."

reset: clean
	git submodule foreach --recursive 'git am --abort || true'
	git submodule update --init --recursive --force
	@echo ">>> Removing symlinks from submodule example directories..."
	@for dir in $(MICROKIT_MULTIKERNEL) $(MICROKIT_SMP) $(MICROKIT_UNICORE) $(MICROKIT_CAPDL_MULTIKERNEL); do \
		if [ -d "$$dir/example" ]; then \
			find "$$dir/example" -maxdepth 1 -type l -exec rm -v {} \; ; \
		else \
			echo "INFO: $$dir/example does not exist, skipping."; \
		fi \
	done
	@echo ">>> Reset complete."
