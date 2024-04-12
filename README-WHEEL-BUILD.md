# Instructions for creating s2geometry wheels in Docker

## Build linux wheels for Python 3.11

### Build and run s2build container
```shell
$ docker compose build s2build
$ docker compose run --remove-orphans s2build
```

### Build s2geometry wheel in s2build container
```shell
# python setup.py bdist_wheel
```

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.


## Build linux wheels for Python 3.11 (Linux/AMD64)

### Build and run s2build-amd64 container
```shell
$ docker compose -f docker-compose-amd64.yml build s2build-amd64
$ docker compose -f docker-compose-amd64.yml run --remove-orphans s2build-amd64
```

### Build s2geometry wheel in s2build-amd64 container
```shell
# CXX=clang++ python setup.py bdist_wheel
```

> Note: We use `clang` rather than `gcc` due to seeming bugs in the C++17 
> implementation of GCC 12.2 under Debian 12.

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.


## Build linux wheels for Python 3.12 (Linux/AMD64)

### Build and run s2build-amd64-py312 container
```shell
$ docker compose -f docker-compose-amd64.yml build --build-arg 'PYTHON_IMG_TAG=3.12-slim-bookworm' s2build-amd64
$ docker compose -f docker-compose-amd64.yml run --remove-orphans s2build-amd64
```

### Build s2geometry wheel in s2build-amd64 container
```shell
# CXX=clang++ python setup.py bdist_wheel
```

> Note: We use `clang` rather than `gcc` due to seeming bugs in the C++17
> implementation of GCC 12.2 under Debian 12.

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.


## Build linux wheels for Python 3.8 (Linux/AMD64)

### Build and run s2build-p3y8 container
```shell
$ docker compose -f docker-compose-py38.yml build s2build-py38
$ docker compose -f docker-compose-py38.yml run --remove-orphans s2build-py38
```

### Build s2geometry wheel in s2build container
```shell
# python setup.py bdist_wheel
```

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.