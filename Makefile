.PHONY: build
build:
	python3 -m venv .venv
	.venv/bin/pip install --disable-pip-version-check build
	.venv/bin/python -m build
