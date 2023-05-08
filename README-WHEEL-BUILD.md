# Instructions for creating s2geometry wheels in Docker

## Build and run s2build container
```shell
$ docker compose build s2build
$ docker compose run s2build
```

## Build s2geometry wheel in s2build container
```shell
# python setup.py bdist_wheel
```

The resulting wheel will be in the `dist` directory of `/use/src/s2geometry` of the container,
which is a mount of the current directory of the host.
