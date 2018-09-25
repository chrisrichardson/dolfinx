"""This demo program plots a bunch of nonlinear
special functions that are available in UFL."""

# Copyright (C) 2010 Martin S. Alnaes
#
# This file is part of DOLFIN (https://www.fenicsproject.org)
#
# SPDX-License-Identifier:    LGPL-3.0-or-later

import os, matplotlib
if 'DISPLAY' not in os.environ:
    matplotlib.use('agg')

from dolfin import *
from dolfin import cpp
import dolfin
import matplotlib.pyplot as plt
from dolfin.plotting import plot

# Form compiler options
# parameters["form_compiler"]["optimize"]     = True
# parameters["form_compiler"]["cpp_optimize"] = True

n = 100
eps = 1e-8

mesh = IntervalMesh.create(MPI.comm_world, n, [-2.0, +2.0], cpp.mesh.GhostMode.none)
mesh.geometry.coord_mapping = dolfin.fem.create_coordinate_map(mesh)

x = SpatialCoordinate(mesh)[0]

plot(cos(x), title='cos', mesh=mesh)
plot(sin(x), title='sin', mesh=mesh)
plot(tan(x), title='tan', mesh=mesh)

mesh = IntervalMesh.create(MPI.comm_world, n, [0.0+eps, 1.0-eps], cpp.mesh.GhostMode.none)
mesh.geometry.coord_mapping = dolfin.fem.create_coordinate_map(mesh)

x = SpatialCoordinate(mesh)[0]

plt.figure(); plot(acos(x), title='acos', mesh=mesh)
plt.figure(); plot(asin(x), title='asin', mesh=mesh)
plt.figure(); plot(atan(x), title='atan', mesh=mesh)
plt.figure(); plot(exp(x), title='exp', mesh=mesh)
plt.figure(); plot(ln(x), title='ln', mesh=mesh)
plt.figure(); plot(sqrt(x), title='sqrt', mesh=mesh)
plt.figure(); plot(bessel_J(0, x), title='bessel_J(0, x)', mesh=mesh)
plt.savefig("a.pdf")
plt.figure(); plot(bessel_J(1, x), title='bessel_J(1, x)', mesh=mesh)
plt.figure(); plot(bessel_Y(0, x), title='bessel_Y(0, x)', mesh=mesh)
plt.figure(); plot(bessel_Y(1, x), title='bessel_Y(1, x)', mesh=mesh)
# plt.figure(); plot(bessel_I(0, x), title='bessel_I(0, x)', mesh=mesh)
# plt.figure(); plot(bessel_I(1, x), title='bessel_I(1, x)', mesh=mesh)
# plt.figure(); plot(bessel_K(0, x), title='bessel_K(0, x)', mesh=mesh)
# plt.figure(); plot(bessel_K(1, x), title='bessel_K(1, x)', mesh=mesh)

plt.show()
