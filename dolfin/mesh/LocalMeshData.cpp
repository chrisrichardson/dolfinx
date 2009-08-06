// Copyright (C) 2008 Ola Skavhaug.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2008-11-28
// Last changed: 2009-08-06
//
// Modified by Anders Logg, 2008-2009.

#include <dolfin/log/log.h>
#include <dolfin/main/MPI.h>
#include "Mesh.h"
#include "Vertex.h"
#include "Cell.h"
#include "LocalMeshData.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
LocalMeshData::LocalMeshData()
  : num_global_vertices(0), num_global_cells(0),
    gdim(0), tdim(0)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
LocalMeshData::LocalMeshData(const Mesh& mesh)
  : num_global_vertices(0), num_global_cells(0),
    gdim(0), tdim(0)
{
  error("This should not be called");
  dolfin_debug("check");

  // Extract data on main process and split among processes
  if (MPI::is_broadcaster())
  {
    extract_mesh_data(mesh);
    broadcast_mesh_data();
  }
  else
  {
    receive_mesh_data();
  }
}
//-----------------------------------------------------------------------------
LocalMeshData::~LocalMeshData()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void LocalMeshData::clear()
{
  vertex_coordinates.clear();
  vertex_indices.clear();
  cell_vertices.clear();
  num_global_vertices = 0;
  num_global_cells = 0;
  gdim = 0;
  tdim = 0;
}
//-----------------------------------------------------------------------------
void LocalMeshData::extract_mesh_data(const Mesh& mesh)
{
  // Clear old data
  clear();

  // Set scalar data
  gdim = mesh.geometry().dim();
  tdim = mesh.topology().dim();
  num_global_vertices = mesh.num_vertices();
  num_global_cells = mesh.num_cells();

  /// Get coordinates for all vertices stored on local processor
  vertex_coordinates.reserve(mesh.num_vertices());
  for (VertexIterator vertex(mesh); !vertex.end(); ++vertex)
  {
    std::vector<double> coordinates(gdim);
    for (uint i = 0; i < gdim; ++i)
      coordinates[i] = vertex->x()[i];
    vertex_coordinates.push_back(coordinates);
  }
  
  /// Get global vertex indices for all vertices stored on local processor
  vertex_indices.reserve(mesh.num_vertices());
  for (VertexIterator vertex(mesh); !vertex.end(); ++vertex)
    vertex_indices.push_back(vertex->index());
  
  /// Get global vertex indices for all cells stored on local processor
  cell_vertices.reserve(mesh.num_cells());
  for (CellIterator cell(mesh); !cell.end(); ++cell)
  {
    std::vector<uint> vertices(cell->num_entities(0));
    for (uint i = 0; i < cell->num_entities(0); ++i)
    {
      vertices[i] = cell->entities(0)[i];
    }
    cell_vertices.push_back(vertices);
  }
}
//-----------------------------------------------------------------------------
void LocalMeshData::broadcast_mesh_data()
{
  // Get local ranges


  
}
//-----------------------------------------------------------------------------
void LocalMeshData::receive_mesh_data()
{


}
//-----------------------------------------------------------------------------
