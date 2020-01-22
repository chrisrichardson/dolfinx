#include "hyperelasticity.h"
#include <cfloat>
#include <dolfin.h>

using namespace dolfin;

// Next:
//
// .. code-block:: cpp

class HyperElasticProblem : public nls::NonlinearProblem
{
public:
  HyperElasticProblem(std::shared_ptr<function::Function> u,
                      std::shared_ptr<fem::Form> L,
                      std::shared_ptr<fem::Form> J,
                      std::vector<std::shared_ptr<const fem::DirichletBC>> bcs)
      : _u(u), _l(L), _j(J), _bcs(bcs),
        _b(*L->function_space(0)->dofmap()->index_map),
        _matA(fem::create_matrix(*J))
  {
    // Do nothing
  }

  /// Destructor
  virtual ~HyperElasticProblem() = default;

  void form(Vec x) final
  {
    la::PETScVector _x(x, true);
    _x.update_ghosts();
  }

  /// Compute F at current point x
  Vec F(const Vec x) final
  {
    // Assemble b
    la::VecWrapper b_wrapper(_b.vec());
    b_wrapper.x.setZero();
    b_wrapper.restore();

    assemble_vector(_b.vec(), *_l);
    _b.apply_ghosts();

    // Set bcs
    set_bc(_b.vec(), _bcs, x, -1);

    return _b.vec();
  }

  /// Compute J = F' at current point x
  Mat J(const Vec x) final
  {
    MatZeroEntries(_matA.mat());
    assemble_matrix(_matA.mat(), *_j, _bcs);
    add_diagonal(_matA.mat(), *_j->function_space(0), _bcs);
    _matA.apply(la::PETScMatrix::AssemblyType::FINAL);
    return _matA.mat();
  }

private:
  std::shared_ptr<function::Function> _u;
  std::shared_ptr<fem::Form> _l, _j;
  std::vector<std::shared_ptr<const fem::DirichletBC>> _bcs;

  la::PETScVector _b;
  la::PETScMatrix _matA;
};

int main(int argc, char* argv[])
{
  common::SubSystemsManager::init_logging(argc, argv);
  common::SubSystemsManager::init_petsc(argc, argv);

  // Inside the ``main`` function, we begin by defining a tetrahedral mesh
  // of the domain and the function space on this mesh. Here, we choose to
  // create a unit cube mesh with 25 ( = 24 + 1) vertices in one direction
  // and 17 ( = 16 + 1) vertices in the other two directions. With this
  // mesh, we initialize the (finite element) function space defined by the
  // generated code.
  //
  // .. code-block:: cpp

  // Create mesh and define function space
  std::array<Eigen::Vector3d, 2> pt;
  pt[0] << 0., 0., 0.;
  pt[1] << 1., 1., 1.;

  auto mesh = std::make_shared<mesh::Mesh>(generation::BoxMesh::create(
      MPI_COMM_WORLD, pt, {{8, 8, 8}}, mesh::CellType::tetrahedron,
      mesh::GhostMode::none));

  auto V
      = fem::create_functionspace(hyperelasticity_functionspace_create, mesh);

  // Define solution function
  auto u = std::make_shared<function::Function>(V);

  std::shared_ptr<fem::Form> a
      = fem::create_form(hyperelasticity_bilinearform_create, {V, V});

  std::shared_ptr<fem::Form> L
      = fem::create_form(hyperelasticity_linearform_create, {V});

  // Attach 'coordinate mapping' to mesh
  auto cmap = a->coordinate_mapping();
  mesh->geometry().coord_mapping = cmap;

  auto u_rotation = std::make_shared<function::Function>(V);
  u_rotation->interpolate([](auto& x) {
    const double scale = 0.005;

    // Center of rotation
    const double y0 = 0.5;
    const double z0 = 0.5;

    // Large angle of rotation (60 degrees)
    double theta = 1.04719755;

    Eigen::Array<PetscScalar, 3, Eigen::Dynamic, Eigen::RowMajor> values(
        3, x.cols());
    for (int i = 0; i < x.cols(); ++i)
    {
      // New coordinates
      double y = y0 + (x(1, i) - y0) * cos(theta) - (x(2, i) - z0) * sin(theta);
      double z = z0 + (x(1, i) - y0) * sin(theta) + (x(2, i) - z0) * cos(theta);

      // Rotate at right end
      values(0, i) = 0.0;
      values(1, i) = scale * (y - x(1, i));
      values(2, i) = scale * (z - x(2, i));
    }

    return values;
  });

  auto u_clamp = std::make_shared<function::Function>(V);
  u_clamp->interpolate([](auto& x) {
    return Eigen::Array<PetscScalar, 3, Eigen::Dynamic, Eigen::RowMajor>::Zero(
        3, x.cols());
  });

  L->set_coefficients({{"u", u}});
  a->set_coefficients({{"u", u}});

  // Create Dirichlet boundary conditions
  auto u0 = std::make_shared<function::Function>(V);
  std::vector<std::shared_ptr<const fem::DirichletBC>> bcs
      = {std::make_shared<fem::DirichletBC>(
             V, u_clamp, [](auto x) { return x.row(0) < DBL_EPSILON; }),
         std::make_shared<fem::DirichletBC>(V, u_rotation, [](auto& x) {
           return (x.row(0) - 1.0).abs() < DBL_EPSILON;
         })};

  HyperElasticProblem problem(u, L, a, bcs);
  nls::NewtonSolver newton_solver(MPI_COMM_WORLD);
  newton_solver.solve(problem, u->vector().vec());

  // Save solution in VTK format
  io::VTKFile file("u.pvd");
  file.write(*u);

  // fem::DirichletBC bcl(V, c, left);
  // fem::DirichletBC bcr(V, r, right);
  // std::vector<const DirichletBC*> bcs = {{&bcl, &bcr}};

  // // The two boundary conditions are collected in the container
  // ``bcs``.
  // //
  // // We use two instances of the class :cpp:class:`Constant` to define
  // the
  // // source ``B`` and the traction ``T``.
  // //
  // // .. code-block:: cpp

  //   // Define source and boundary traction functions
  //   auto B = std::make_shared<Constant>(0.0, -0.5, 0.0);
  //   auto T = std::make_shared<Constant>(0.1,  0.0, 0.0);

  // // The solution for the displacement will be an instance of the class
  // // :cpp:class:`Function`, living in the function space ``V``; we
  // define
  // // it here:
  // //
  // // .. code-block:: cpp

  //   // Define solution function
  //   auto u = std::make_shared<Function>(V);

  // // Next, we set the material parameters
  // //
  // // .. code-block:: cpp

  //   // Set material parameters
  //   const double E  = 10.0;
  //   const double nu = 0.3;
  //   auto mu = std::make_shared<Constant>(E/(2*(1 + nu)));
  //   auto lambda = std::make_shared<Constant>(E*nu/((1 + nu)*(1 -
  //   2*nu)));

  // // Now, we can initialize the bilinear and linear forms (``a``,
  // ``L``)
  // // using the previously defined :cpp:class:`FunctionSpace` ``V``. We
  // // attach the material parameters and previously initialized
  // functions to
  // // the forms.
  // //
  // // .. code-block:: cpp

  //   // Create (linear) form defining (nonlinear) variational problem
  //   HyperElasticity::ResidualForm F(V);
  //   F.mu = mu; F.lmbda = lambda; F.u = u;
  //   F.B = B; F.T = T;

  //   // Create Jacobian dF = F' (for use in nonlinear solver).
  //   HyperElasticity::JacobianForm J(V, V);
  //   J.mu = mu; J.lmbda = lambda; J.u = u;

  // // Now, we have specified the variational forms and can consider the
  // // solution of the variational problem.
  // //
  // // .. code-block:: cpp

  //   // Solve nonlinear variational problem F(u; v) = 0
  //   solve(F == 0, *u, bcs, J);

  // // Finally, the solution ``u`` is saved to a file named
  // // ``displacement.pvd`` in VTK format.
  // //
  // // .. code-block:: cpp

  //   // Save solution in VTK format
  //   File file("displacement.pvd");
  //   file << *u;

  return 0;
}
