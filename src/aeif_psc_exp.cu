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

#include <math.h>
#include <iostream>
#include "aeif_psc_exp_kernel.h"
#include "rk5.h"
#include "aeif_psc_exp.h"

namespace aeif_psc_exp_ns
{

__device__
void NodeInit(int n_var, int n_param, float x, float *y, float *param,
	      aeif_psc_exp_rk5 data_struct)
{
  //int array_idx = threadIdx.x + blockIdx.x * blockDim.x;
  int n_port = (n_var-N_SCAL_VAR)/N_PORT_VAR;

  V_th = -50.4;
  Delta_T = 2.0;
  g_L = 30.0;
  E_L = -70.6;
  C_m = 281.0;
  a = 4.0;
  b = 80.5;
  tau_w = 144.0;
  I_e = 0.0;
  V_peak = 0.0;
  V_reset = -60.0;
  t_ref = 0.0;
  
  V_m = E_L;
  w = 0;
  refractory_step = 0;
  for (int i = 0; i<n_port; i++) {
    tau_syn(i) = 0.2;
    I_syn(i) = 0;
  }
}

__device__
void NodeCalibrate(int n_var, int n_param, float x, float *y,
		       float *param, aeif_psc_exp_rk5 data_struct)
{
  //int array_idx = threadIdx.x + blockIdx.x * blockDim.x;
  //int n_port = (n_var-N_SCAL_VAR)/N_PORT_VAR;

  refractory_step = 0;
  // set the right threshold depending on Delta_T
  if (Delta_T <= 0.0) {
    V_peak = V_th; // same as IAF dynamics for spikes if Delta_T == 0.
  }
}

}

__device__
void NodeInit(int n_var, int n_param, float x, float *y,
	     float *param, aeif_psc_exp_rk5 data_struct)
{
    aeif_psc_exp_ns::NodeInit(n_var, n_param, x, y, param, data_struct);
}

__device__
void NodeCalibrate(int n_var, int n_param, float x, float *y,
		  float *param, aeif_psc_exp_rk5 data_struct)

{
    aeif_psc_exp_ns::NodeCalibrate(n_var, n_param, x, y, param, data_struct);
}

using namespace aeif_psc_exp_ns;

int aeif_psc_exp::Init(int i_node_0, int n_node, int n_port,
			 int i_group, unsigned long long *seed) {
  BaseNeuron::Init(i_node_0, n_node, n_port, i_group, seed);
  h_min_=1.0e-4;
  h_ = 1.0e-2;
  node_type_ = i_aeif_psc_exp_model;
  n_scal_var_ = N_SCAL_VAR;
  n_port_var_ = N_PORT_VAR;
  n_scal_param_ = N_SCAL_PARAM;
  n_port_param_ = N_PORT_PARAM;

  n_var_ = n_scal_var_ + n_port_var_*n_port;
  n_param_ = n_scal_param_ + n_port_param_*n_port;

  scal_var_name_ = aeif_psc_exp_scal_var_name;
  port_var_name_= aeif_psc_exp_port_var_name;
  scal_param_name_ = aeif_psc_exp_scal_param_name;
  port_param_name_ = aeif_psc_exp_port_param_name;
  //rk5_data_struct_.node_type_ = i_aeif_psc_exp_model;
  rk5_data_struct_.i_node_0_ = i_node_0_;

  rk5_.Init(n_node, n_var_, n_param_, 0.0, h_, rk5_data_struct_);
  var_arr_ = rk5_.GetYArr();
  param_arr_ = rk5_.GetParamArr();

  // multiplication factor of input signal is always 1 for all nodes
  float input_weight = 1.0;
  gpuErrchk(cudaMalloc(&port_weight_arr_, sizeof(float)));
  gpuErrchk(cudaMemcpy(port_weight_arr_, &input_weight,
			 sizeof(float), cudaMemcpyHostToDevice));
  port_weight_arr_step_ = 0;
  port_weight_port_step_ = 0;

  port_input_arr_ = GetVarArr() + n_scal_var_
    + GetPortVarIdx("I_syn");
  port_input_arr_step_ = n_var_;
  port_input_port_step_ = n_port_var_;

  return 0;
}

int aeif_psc_exp::Calibrate(float time_min, float /*time_resolution*/)
{
  rk5_.Calibrate(time_min, h_, rk5_data_struct_);
  
  return 0;
}

template <>
int aeif_psc_exp::UpdateNR<0>(int it, float t1)
{
  return 0;
}

int aeif_psc_exp::Update(int it, float t1) {
  UpdateNR<MAX_PORT_NUM>(it, t1);

  return 0;
}
