package(default_visibility = ["//visibility:public"])

# Configuration variables for python
# The include path: /usr/include/<python-dir>/...
PYTHON_INCLUDE="python3.7m"
# The python library to link against.
PYTHON_LIB="-lpython3.7m"

config_setting(
    name = "windows",
    values = {
        "crosstool_top": "@mxebzl//tools/windows:toolchain",
    }
)

HEADERS = [
    "pybind11/options.h",
    "pybind11/common.h",
    "pybind11/numpy.h",
    "pybind11/eval.h",
    "pybind11/pybind11.h",
    "pybind11/iostream.h",
    "pybind11/attr.h",
    "pybind11/chrono.h",
    "pybind11/functional.h",

    "pybind11/detail/common.h",
    "pybind11/detail/init.h",
    "pybind11/detail/typeid.h",
    "pybind11/detail/class.h",
    "pybind11/detail/descr.h",
    "pybind11/detail/internals.h",

    "pybind11/buffer_info.h",
    "pybind11/pytypes.h",
    "pybind11/cast.h",
    "pybind11/embed.h",
    "pybind11/stl.h",
    "pybind11/stl_bind.h",
    "pybind11/eigen.h",
    "pybind11/complex.h",
    "pybind11/operators.h",
]

# This is mildly awful, but I don't know how to solve this in a better way.
# Depending on the configuration, rewrite the include paths in pybind11 to
# the correct system path for that configuration.
#
# The windows configuration is based on where the pseudo mxe package installs
# include files into the MXE tree.
#
# The default configuration is based on the include paths on arch linux.
[genrule(
    name = "fix_" + F.replace('/', '_').replace('.', '_'),
    srcs = [ 'include/' + F ],
    outs = [ 'fixed/' + F ],
    cmd = select({
        ":windows": """
            sed \
                -e "s|Python\.h|python37/Python.h|g" \
                -e "s|frameobject\.h|python37/frameobject.h|g" \
                -e "s|pythread\.h|python37/pythread.h|g" \
                < $(<) > $(@)
            """,
        "//conditions:default": """
            sed \
                -e "s|Python\.h|{python}/Python.h|g" \
                -e "s|frameobject\.h|{python}/frameobject.h|g" \
                -e "s|pythread\.h|{python}/pythread.h|g" \
                < $(<) > $(@)
            """.format(python=PYTHON_INCLUDE)
    }),

) for F in HEADERS]

# And again, the library link specifications are based on how each respective
# system names its libraries.
cc_library(
    name = "pybind11",
    includes = [
        "fixed",
    ],
    hdrs = [ 'fixed/' + F for F in HEADERS ],
    linkopts = select({
        ":windows": [
            "-lpython",
        ],
        "//conditions:default": [
            PYTHON_LIB,
        ]
    }),
)
