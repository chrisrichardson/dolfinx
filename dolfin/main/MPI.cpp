// Copyright (C) 2007 Magnus Vikstrøm.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Garth N. Wells, 2007, 2008.
// Modified by Anders Logg, 2007-2009.
// Modified by Ola Skavhaug, 2008-2009.
//
// First added:  2007-11-30
// Last changed: 2009-06-20

#include <dolfin/log/dolfin_log.h>
#include "mpiutils.h"
#include "SubSystemsManager.h"
#include "MPI.h"

#ifdef HAS_MPI

#include <mpi.h>

using MPI::COMM_WORLD;

//-----------------------------------------------------------------------------
dolfin::MPICommunicator::MPICommunicator()
{
  MPI_Comm_dup(MPI_COMM_WORLD, &communicator);
}
//-----------------------------------------------------------------------------
dolfin::MPICommunicator::~MPICommunicator()
{
  MPI_Comm_free(&communicator);
}
//-----------------------------------------------------------------------------
MPI_Comm& dolfin::MPICommunicator::operator*()
{
  return communicator;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::process_number()
{
  SubSystemsManager::init_mpi();
  return static_cast<uint>(COMM_WORLD.Get_rank());
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::num_processes()
{
  SubSystemsManager::init_mpi();
  return static_cast<uint>(COMM_WORLD.Get_size());
}
//-----------------------------------------------------------------------------
bool dolfin::MPI::is_broadcaster()
{
  // Always broadcast from processor number 0
  return num_processes() > 1 && process_number() == 0;
}
//-----------------------------------------------------------------------------
bool dolfin::MPI::is_receiver()
{
  // Always receive on processors with numbers > 0
  return num_processes() > 1 && process_number() > 0;
}
//-----------------------------------------------------------------------------
void dolfin::MPI::distribute(std::vector<uint>& values,
                             std::vector<uint>& partition)
{
  dolfin::distribute(values, partition);
}
//-----------------------------------------------------------------------------
void dolfin::MPI::distribute(std::vector<double>& values,
                             std::vector<uint>& partition)
{
  dolfin::distribute(values, partition);
}
//-----------------------------------------------------------------------------
void dolfin::MPI::scatter(std::vector<uint>& values, uint sending_process)
{
  // Prepare receive buffer (size 1)
  int receive_buffer = 0;

  /// Create communicator (copy of MPI_COMM_WORLD)
  MPICommunicator comm;
  
  // Prepare arguments differently depending on whether we're sending
  if (process_number() == sending_process)
  {
    // Check size of values
    if (values.size() != num_processes())
      error("Number of values to scatter must be equal to the number of processes.");

    // Prepare send buffer
    uint* send_buffer = new uint[values.size()];
    for (uint i = 0; i < values.size(); i++)
      send_buffer[i] = values[i];
    
    // Call MPI to send values
    MPI_Scatter(send_buffer,
                values.size(),
                MPI_UNSIGNED,
                &receive_buffer,
                1,
                MPI_UNSIGNED,
                sending_process,
                *comm);

    // Cleanup
    delete [] send_buffer;
  }
  else
  {
    // Call MPI to send values
    MPI_Scatter(0,
                0,
                MPI_UNSIGNED,
                &receive_buffer,
                1,
                MPI_UNSIGNED,
                sending_process,
                *comm);
  }

  // Collect values
  values.clear();
  values.push_back(receive_buffer);
}
//-----------------------------------------------------------------------------
void dolfin::MPI::scatter(std::vector<std::vector<uint> >& values,
                          uint sending_process)
{
  /*
  dolfin_assert(values.size() == num_processes());



  

  // Prepare array containing size of data to send to each process
  uint total_size = 1;
  int* sizes = new int[values.size()];
  for (uint i = 0; i < values.size(); i++)
  {
    total_size += values[i].size();
    sizes[i] = values[i].size();
  }

  // Prepare array containing offsets into send data
  int* offsets = new int[values.size()];
  offsets[0] = 0;
  for (uint i = 1; i < values.size(); i++)
    offsets[i] = offsets[i - 1] + sizes[i - 1];

  // Prepare array of values to send
  int* send_buffer = new int[total_size];
  uint k = 0;
  for (uint i = 0; i < values.size(); i++)
    for (uint j = 0; j < values[i].size(); j++)
      send_buffer[k++] = values[i][j];

  MPI_Scatterv(send_buffer,
               send_counts,
               offsets,
               MPI_UNSIGNED, 
               void *recvbuf, 
               int recvcnt,  
               MPI_Datatype recvtype, 
               static_cast<uint>(sending_process),
               *comm);

  // Cleanup arrays
  delete [] send_counts;
  delete [] offsets;
  */
}
//-----------------------------------------------------------------------------
void dolfin::MPI::gather(std::vector<uint>& values)
{
  assert(values.size() == num_processes());

  // Prepare arrays
  uint send_value = values[process_number()];
  uint* received_values = new uint[values.size()];

  /// Create communicator (copy of MPI_COMM_WORLD)
  MPICommunicator comm;

  // Call MPI
  MPI_Allgather(&send_value,     1, MPI_UNSIGNED,
                received_values, 1, MPI_UNSIGNED, *comm);

  // Copy values
  for (uint i = 0; i < values.size(); i++)
    values[i] = received_values[i];

  // Cleanup
  delete [] received_values;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::global_maximum(uint size)
{
  uint recv_size = 0;
  /// Create communicator (copy of MPI_COMM_WORLD)
  MPICommunicator comm;
  MPI_Allreduce(&size, &recv_size, 1, MPI_UNSIGNED, MPI_MAX, *comm);
  return recv_size;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::send_recv(uint* send_buffer, uint send_size, uint dest,
                                    uint* recv_buffer, uint recv_size, uint source)
{
  MPI_Status status;

  /// Create communicator (copy of MPI_COMM_WORLD)
  MPICommunicator comm;

  // Send and receive data
  MPI_Sendrecv(send_buffer, static_cast<int>(send_size), MPI_UNSIGNED, static_cast<int>(dest), 0,
               recv_buffer, static_cast<int>(recv_size), MPI_UNSIGNED, static_cast<int>(source),  0,
               *comm, &status);

  // Check number of received values
  int num_received = 0;
  MPI_Get_count(&status, MPI_UNSIGNED, &num_received);
  assert(num_received >= 0);

  return static_cast<uint>(num_received);
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::send_recv(double* send_buffer, uint send_size, uint dest,
                                    double* recv_buffer, uint recv_size, uint source)
{
  MPI_Status status;

  /// Create communicator (copy of MPI_COMM_WORLD)
  MPICommunicator comm;

  // Send and receive data
  MPI_Sendrecv(send_buffer, static_cast<int>(send_size), MPI_DOUBLE, static_cast<int>(dest), 0,
               recv_buffer, static_cast<int>(recv_size), MPI_DOUBLE, static_cast<int>(source),  0,
               *comm, &status);

  // Check number of received values
  int num_received = 0;
  MPI_Get_count(&status, MPI_DOUBLE, &num_received);
  assert(num_received >= 0);

  return static_cast<uint>(num_received);
}
//-----------------------------------------------------------------------------
std::pair<dolfin::uint, dolfin::uint> dolfin::MPI::local_range(uint N)
{
  // Get number of processes and process number
  const uint _num_processes = num_processes();
  const uint _process_number = process_number();

  // Compute number of items per process and remainder
  const uint n = N / _num_processes;
  const uint r = N % _num_processes;

  // Compute local range
  std::pair<uint, uint> range;
  if (_process_number < r)
  {
    range.first = _process_number*(n + 1);
    range.second = range.first + n + 1;
  }
  else
  {
    range.first = _process_number*n + r;
    range.second = range.first + n;
  }

  return range;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::index_owner(uint index, uint N)
{
  assert(index < N);

  // Get number of processes
  const uint _num_processes = num_processes();

  // Compute number of items per process and remainder
  const uint n = N / _num_processes;
  const uint r = N % _num_processes;
  
  // First r processes own n + 1 indices
  if (index < r * (n + 1))
    return index / (n + 1);

  // Remaining processes own n indices
  return r + (index - r * (n + 1)) / n;
}
//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::process_number()
{
  return 0;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::num_processes()
{
  return 1;
}
//-----------------------------------------------------------------------------
bool dolfin::MPI::broadcast()
{
  return false;
}
//-----------------------------------------------------------------------------
bool dolfin::MPI::receive()
{
  return false;
}
//-----------------------------------------------------------------------------
void dolfin::MPI::distribute(std::vector<uint>& values,
                             std::vector<uint>& partition)
{
  error("MPI::distribute() requires MPI.");
}
//-----------------------------------------------------------------------------
void dolfin::MPI::distribute(std::vector<double>& values,
                             std::vector<uint>& partition)
{
  error("MPI::distribute() requires MPI.");
}
//-----------------------------------------------------------------------------
void dolfin::MPI::gather(std::vector<uint>& values)
{
  error("MPI::gather() requires MPI.");
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::global_maximum(uint size)
{
  error("MPI::global_maximum() requires MPI.");
  return 0;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::send_recv(uint* send_buffer, uint send_size, uint dest,
                                    uint* recv_buffer, uint recv_size, uint source)
{
  error("MPI::send_recv() requires MPI.");
  return 0;
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::send_recv(double* send_buffer, uint send_size, uint dest,
                                    double* recv_buffer, uint recv_size, uint source)
{
  error("MPI::send_recv() requires MPI.");
  return 0;
}
//-----------------------------------------------------------------------------
std::pair<dolfin::uint, dolfin::uint> dolfin::MPI::local_range(uint N)
{
  return std::make_pair(0, N);
}
//-----------------------------------------------------------------------------
dolfin::uint dolfin::MPI::index_owner(uint i, uint N)
{
  assert(i < N);
  return 0;
}
//-----------------------------------------------------------------------------

#endif
