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

#include <config.h>
#include <cmath>
#include <iostream>
#include "izhikevich.h"
#include "spike_buffer.h"

using namespace izhikevich_ns;

extern __constant__ float NeuronGPUTimeResolution;

#define I_syn var[i_I_syn]
#define V_m var[i_V_m]
#define u var[i_u]
#define refractory_step var[i_refractory_step]
#define I_e param[i_I_e]
#define den_delay param[i_den_delay]

#define V_th_ group_param_[i_V_th]
#define a_ group_param_[i_a]
#define b_ group_param_[i_b]
#define c_ group_param_[i_c]
#define d_ group_param_[i_d]
#define t_ref_ group_param_[i_t_ref]

__global__ void izhikevich_Update
( int n_node, int i_node_0, float *var_arr, float *param_arr, int n_var,
  int n_param, float V_th, float a, float b, float c, float d,
  int n_refractory_steps, float h)
{
  int i_neuron = threadIdx.x + blockIdx.x * blockDim.x;
  if (i_neuron<n_node) {
    float *var = var_arr + n_var*i_neuron;
    float *param = param_arr + n_param*i_neuron;
    
    if ( refractory_step > 0.0 ) {
      // neuron is absolute refractory
      refractory_step -= 1.0;
    }
    else { // neuron is not refractory, so evolve V and u
      float v_old = V_m;
      float u_old = u;

      V_m += h*(0.04 * v_old * v_old + 5.0 * v_old + 140.0 - u_old
		+ I_e) + I_syn;
      u += h*a*(b*v_old - u_old);
    }
    I_syn = 0;
    
    if ( V_m >= V_th ) { // send spike
      PushSpike(i_node_0 + i_neuron, 1.0);
      V_m = c;
      u += d; // spike-driven adaptation
      refractory_step = n_refractory_steps;
      if (refractory_step<0) {
	refractory_step = 0;
      }
    }
  }
}


izhikevich::~izhikevich()
{
  FreeVarArr();
  FreeParamArr();
}

int izhikevich::Init(int i_node_0, int n_node, int /*n_port*/,
			   int i_group, unsigned long long *seed)
{
  BaseNeuron::Init(i_node_0, n_node, 1 /*n_port*/, i_group, seed);
  node_type_ = i_izhikevich_model;

  n_scal_var_ = N_SCAL_VAR;
  n_var_ = n_scal_var_;
  n_scal_param_ = N_SCAL_PARAM;
  n_group_param_ = N_GROUP_PARAM;
  n_param_ = n_scal_param_;
  
  AllocParamArr();
  AllocVarArr();
  group_param_ = new float[N_GROUP_PARAM];

  scal_var_name_ = izhikevich_scal_var_name;
  scal_param_name_ = izhikevich_scal_param_name;
  group_param_name_ = izhikevich_group_param_name;

  SetScalParam(0, n_node, "I_e", 0.0 );              // in pA
  SetScalParam(0, n_node, "den_delay", 0.0 );        // in ms
  
  SetScalVar(0, n_node, "I_syn", 0.0 );
  SetScalVar(0, n_node, "V_m", -70.0 ); // in mV
  SetScalVar(0, n_node, "u", -70.0*0.2 );
  SetScalVar(0, n_node, "refractory_step", 0 );

  SetGroupParam("V_th", 30.0);
  SetGroupParam("a", 0.02);
  SetGroupParam("b", 0.2);
  SetGroupParam("c", -65.0);
  SetGroupParam("d", 8.0);
  SetGroupParam("t_ref", 0.0);

  // multiplication factor of input signal is always 1 for all nodes
  float input_weight = 1.0;
  gpuErrchk(cudaMalloc(&port_weight_arr_, sizeof(float)));
  gpuErrchk(cudaMemcpy(port_weight_arr_, &input_weight,
			 sizeof(float), cudaMemcpyHostToDevice));
  port_weight_arr_step_ = 0;
  port_weight_port_step_ = 0;
  
  // input spike signal is stored in I_syn
  port_input_arr_ = GetVarArr() + GetScalVarIdx("I_syn");
  port_input_arr_step_ = n_var_;
  port_input_port_step_ = 0;

  return 0;
}

int izhikevich::Update(long long it, double t1)
{
  // std::cout << "izhikevich neuron update\n";
  float h = time_resolution_;
  int n_refractory_steps = int(round(t_ref_ / h));

  izhikevich_Update<<<(n_node_+1023)/1024, 1024>>>
    (n_node_, i_node_0_, var_arr_, param_arr_, n_var_, n_param_,
     V_th_, a_, b_, c_, d_, n_refractory_steps, h);
  //gpuErrchk( cudaDeviceSynchronize() );
  
  return 0;
}

int izhikevich::Free()
{
  FreeVarArr();  
  FreeParamArr();
  delete[] group_param_;
  
  return 0;
}
