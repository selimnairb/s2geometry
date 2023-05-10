# Instructions for creating s2geometry wheels in Docker

## Build linux wheels for Python 3.11

### Build and run s2build container
```shell
$ docker compose build s2build
$ docker compose run s2build
```

### Build s2geometry wheel in s2build container
```shell
# python setup.py bdist_wheel
```

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.


## Build linux wheels for Python 3.8 (Linux/AMD64)

### Build and run s2build container
```shell
$ docker compose -f docker-compose-py38.yml build s2build-py38
$ docker compose -f docker-compose-py38.yml run s2build-py38
```

### Build s2geometry wheel in s2build container
```shell
# python setup.py bdist_wheel
```

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.