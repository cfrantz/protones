package(default_visibility = ["//visibility:public"])

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

[genrule(
    name = "fix_" + F.replace('/', '_').replace('.', '_'),
    srcs = [ 'include/' + F ],
    outs = [ 'fixed/' + F ],
    cmd = """
        sed \
            -e "s|Python\.h|python3.7m/Python.h|g" \
            -e "s|frameobject\.h|python3.7m/frameobject.h|g" \
            -e "s|pythread\.h|python3.7m/pythread.h|g" \
            < $(<) > $(@)
    """,
) for F in HEADERS]

#            -e 's|#include "\.\./|#include "|g' \
#            -e 's|#include "|#include "pybind11/|g' \

cc_library(
    name = "pybind11",
    includes = [
        "fixed",
    ],
    hdrs = [ 'fixed/' + F for F in HEADERS ],
)
