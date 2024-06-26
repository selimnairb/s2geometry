import sys
from pathlib import Path

import cmake_build_extension
import setuptools

PY_VERSION_MIN = f"{sys.version_info.major}.{sys.version_info.minor}"
PY_VERSION_MAX = f"{sys.version_info.major}.{sys.version_info.minor+1}"
PY_VERSION = f"{PY_VERSION_MIN}...<{PY_VERSION_MAX}"

setuptools.setup(
    ext_modules=[
        cmake_build_extension.CMakeExtension(
            # This could be anything you like, it is used to create build folders
            name="SwigBindings",
            # Name of the resulting package name (import s2geometry)
            install_prefix="s2geometry",
            # Selects the folder where the main CMakeLists.txt is stored
            # (it could be a subfolder)
            source_dir=str(Path(__file__).parent.absolute()),
            cmake_configure_options=[
                                        # This option points CMake to the right Python interpreter, and helps
                                        # the logic of FindPython3.cmake to find the active version
                                        f"-DPython_LOOKUP_VERSION={PY_VERSION}",
                                        '-DPython3_FIND_VIRTUALENV:BOOL=ON',
                                        '-DPython3_FIND_STRATEGY=LOCATION',
                                        '-DCALL_FROM_SETUP_PY:BOOL=ON',
                                        '-DBUILD_SHARED_LIBS:BOOL=OFF',
                                        '-DCMAKE_CXX_STANDARD=17',
                                        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
                                        '-DBUILD_TESTS=OFF',
                                        '-DWITH_PYTHON=ON'
                                    ]
        )
    ],
    cmdclass=dict(
        # Enable the CMakeExtension entries defined above
        build_ext=cmake_build_extension.BuildExtension,
    ),
)
