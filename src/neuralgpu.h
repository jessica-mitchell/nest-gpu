/*
Copyright (C) 2020 Bruno Golosio
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NEURALGPUCLASSH
#define NEURALGPUCLASSH

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "ngpu_exception.h"
#include "node_group.h"
#include "base_neuron.h"
#include "connect_spec.h"

class PoissonGenerator;
class SpikeGenerator;
class Multimeter;
class NetConnection;
class ConnectMpi;
struct curandGenerator_st;
typedef struct curandGenerator_st* curandGenerator_t;
//struct RemoteNode;
//struct RemoteNodePt;
class ConnSpec;
class SynSpec;

class Sequence
{
 public:
  int i0;
  int n;
  
 Sequence(int i0=0, int n=0) : i0(i0), n(n) {}
  
  inline int operator[](int i) {
    if (i<0) {
      throw ngpu_exception("Sequence index cannot be negative");
    }
    if (i>=n) {
      throw ngpu_exception("Sequence index out of range");
    }
    return i0 + i;
  }

  inline Sequence Subseq(int first, int last) {
    if (first<0 || first>last) {
      throw ngpu_exception("Sequence subset range error");
    }
    if (last>=n) {
      throw ngpu_exception("Sequence subset out of range");
    }
    return Sequence(i0 + first, last - first + 1);
  }

  // https://stackoverflow.com/questions/18625223
  inline std::vector<int> ToVector() {
    std::vector<int> v;
    v.reserve(n);
    std::generate_n(std::back_inserter(v), n, [&](){ return i0 + v.size(); });
    return v;
  }
};

typedef Sequence NodeSeq;

enum {ON_EXCEPTION_EXIT=0, ON_EXCEPTION_HANDLE};

class NeuralGPU
{
  float time_resolution_; // time resolution in ms
  curandGenerator_t *random_generator_;
  bool calibrate_flag_; // becomes true after calibration
  bool mpi_flag_; // true if MPI is initialized

  PoissonGenerator *poiss_generator_;
  SpikeGenerator *spike_generator_;
  Multimeter *multimeter_;
  std::vector<BaseNeuron*> node_vect_; // -> node_group_vect
  
  NetConnection *net_connection_;
  ConnectMpi *connect_mpi_;

  //std::vector<NodeGroup> node_group_vect_; RIMOSSO
  std::vector<signed char> node_group_map_;
  signed char *d_node_group_map_;


  int max_spike_buffer_size_;
  int max_spike_num_;
  int max_spike_per_host_;

  float t_min_;
  float neural_time_; // Neural activity time
  float sim_time_; // Simulation time in ms
  int n_poiss_nodes_;
  int n_spike_gen_nodes_;

  double start_real_time_;
  double build_real_time_;
  double end_real_time_;

  bool error_flag_;
  std::string error_message_;
  unsigned char error_code_;
  int on_exception_;
  
  int CreateNodeGroup(int n_neurons, int n_ports);
  int CheckUncalibrated(std::string message);
  double *InitGetSpikeArray(int n_nodes, int n_ports);
  int NodeGroupArrayInit();
  int ClearGetSpikeArrays();
  int FreeGetSpikeArrays();
  int FreeNodeGroupMap();


  template <class T1, class T2>
    int _Connect(T1 source, int n_source, T2 target, int n_target,
		 ConnSpec &conn_spec, SynSpec &syn_spec);
  
  template<class T1, class T2>
    int _SingleConnect(T1 source, int i_source, T2 target, int i_target,
		       int i_array, SynSpec &syn_spec);
  template<class T1, class T2>
    int _SingleConnect(T1 source, int i_source, T2 target, int i_target,
		       float weight, float delay, int i_array,
		       SynSpec &syn_spec);

  template <class T1, class T2>
    int _ConnectOneToOne(T1 source, T2 target, int n_nodes, SynSpec &syn_spec);

  template <class T1, class T2>
    int _ConnectAllToAll
    (T1 source, int n_source, T2 target, int n_target, SynSpec &syn_spec);

  template <class T1, class T2>
    int _ConnectFixedTotalNumber
    (T1 source, int n_source, T2 target, int n_target, int n_conn,
     SynSpec &syn_spec);

  template <class T1, class T2>
    int _ConnectFixedIndegree
    (
     T1 source, int n_source, T2 target, int n_target, int indegree,
     SynSpec &syn_spec
     );

  template <class T1, class T2>
    int _ConnectFixedOutdegree
    (
     T1 source, int n_source, T2 target, int n_target, int outdegree,
     SynSpec &syn_spec
     );

  template <class T1, class T2>
    int _RemoteConnect(RemoteNode<T1> source, int n_source,
		       RemoteNode<T2> target, int n_target,
		       ConnSpec &conn_spec, SynSpec &syn_spec);
  
  template <class T1, class T2>
    int _RemoteConnectOneToOne
    (RemoteNode<T1> source, RemoteNode<T2> target, int n_nodes,
     SynSpec &syn_spec);
  
  template <class T1, class T2>
    int _RemoteConnectAllToAll
    (RemoteNode<T1> source, int n_source, RemoteNode<T2> target, int n_target,
     SynSpec &syn_spec);

  template <class T1, class T2>
    int _RemoteConnectFixedTotalNumber
    (RemoteNode<T1> source, int n_source, RemoteNode<T2> target, int n_target,
     int n_conn, SynSpec &syn_spec);
  
  template <class T1, class T2>
    int _RemoteConnectFixedIndegree
    (RemoteNode<T1> source, int n_source, RemoteNode<T2> target, int n_target,
     int indegree, SynSpec &syn_spec);

  template <class T1, class T2>
    int _RemoteConnectFixedOutdegree
    (RemoteNode<T1> source, int n_source, RemoteNode<T2> target, int n_target,
     int outdegree, SynSpec &syn_spec);

    
 public:
  NeuralGPU();

  ~NeuralGPU();

  int SetRandomSeed(unsigned long long seed);

  int SetTimeResolution(float time_res);
  
  inline float GetTimeResolution() {
    return time_resolution_;
  }

  inline int SetSimTime(float sim_time) {
    sim_time_ = sim_time;
    return 0;
  }

  inline float GetSimTime() {
    return sim_time_;
  }

  int SetMaxSpikeBufferSize(int max_size);
  int GetMaxSpikeBufferSize();
  NodeSeq Create(std::string model_name, int n_neurons, int n_ports);
  NodeSeq CreatePoissonGenerator(int n_nodes, float rate);
  NodeSeq CreateSpikeGenerator(int n_nodes);
  int CreateRecord(std::string file_name, std::string *var_name_arr,
		   int *i_node_arr, int n_nodes);  
  int CreateRecord(std::string file_name, std::string *var_name_arr,
		   int *i_node_arr, int *i_port_arr, int n_nodes);
  std::vector<std::vector<float>> *GetRecordData(int i_record);

  int SetNeuronParam(std::string param_name, int i_node, int n_neurons,
		      float val);

  int SetNeuronParam(std::string param_name, int *i_node, int n_neurons,
		     float val);

  int SetNeuronParam(std::string param_name, int i_node, int n_neurons,
			  float *params, int vect_size);

  int SetNeuronParam(std::string param_name, int *i_node, int n_neurons,
			  float *params, int vect_size);

  int SetNeuronParam(std::string param_name, NodeSeq nodes, float val) {
    return SetNeuronParam(param_name, nodes.i0, nodes.n, val);
  }

  int SetNeuronParam(std::string param_name, NodeSeq nodes, float *params,
		      int vect_size) {
    return SetNeuronParam(param_name, nodes.i0, nodes.n, params, vect_size);
  }
  
  int SetNeuronParam(std::string param_name, std::vector<int> nodes,
		     float val) {
    return SetNeuronParam(param_name, nodes.data(), nodes.size(), val);
  }

  int SetNeuronParam(std::string param_name, std::vector<int> nodes,
		     float *params, int vect_size) {
    return SetNeuronParam(param_name, nodes.data(), nodes.size(), params,
			  vect_size);
  }

  int GetNodeSequenceOffset(int i_node, int n_nodes, int &i_group);

  std::vector<int> GetNodeArrayWithOffset(int *i_node, int n_nodes,
					  int &i_group);

  int IsNeuronScalParam(std::string param_name, int i_node);

  int IsNeuronVectParam(std::string param_name, int i_node);

  int SetSpikeGenerator(int i_node, int n_spikes, float *spike_time,
			float *spike_height);

  int Calibrate();
  int Simulate();

  int ConnectMpiInit(int argc, char *argv[]);

  int MpiId();

  int MpiNp();

  int ProcMaster();

  int MpiFinalize();

  void SetErrorFlag(bool error_flag) {error_flag_ = error_flag;}
  
  void SetErrorMessage(std::string error_message) { error_message_
      = error_message; }

  void SetErrorCode(unsigned char error_code) {error_code_ = error_code;}

  void SetOnException(int on_exception) {on_exception_ = on_exception;}

  bool GetErrorFlag() {return error_flag_;}

  char *GetErrorMessage() {return &error_message_[0];}

  unsigned char GetErrorCode() {return error_code_;}

  int OnException() {return on_exception_;}

  unsigned int *RandomInt(size_t n);
  
  float *RandomUniform(size_t n);

  float *RandomNormal(size_t n, float mean, float stddev);

  float *RandomNormalClipped(size_t n, float mean, float stddev, float vmin,
			     float vmax);  

  int Connect
    (
     int i_source_node, int i_target_node, unsigned char i_port,
     float weight, float delay
     );

  int Connect(int i_source, int n_source, int i_target, int n_target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(int i_source, int n_source, int* target, int n_target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(int* source, int n_source, int i_target, int n_target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(int* source, int n_source, int* target, int n_target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(NodeSeq source, NodeSeq target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(NodeSeq source, std::vector<int> target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(std::vector<int> source, NodeSeq target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int Connect(std::vector<int> source, std::vector<int> target,
	      ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, int i_source, int n_source,
		    int i_target_host, int i_target, int n_target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, int i_source, int n_source,
		    int i_target_host, int* target, int n_target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, int* source, int n_source,
		    int i_target_host, int i_target, int n_target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, int* source, int n_source,
		    int i_target_host, int* target, int n_target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, NodeSeq source,
		    int i_target_host, NodeSeq target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, NodeSeq source,
		    int i_target_host, std::vector<int> target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, std::vector<int> source,
		    int i_target_host, NodeSeq target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);

  int RemoteConnect(int i_source_host, std::vector<int> source,
		    int i_target_host, std::vector<int> target,
		    ConnSpec &conn_spec, SynSpec &syn_spec);


};



#endif
